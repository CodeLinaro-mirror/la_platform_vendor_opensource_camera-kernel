/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/module.h>
#include <linux/firmware.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_trace.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include "cam_hdmi_bdg_uxe_core.h"
#include <linux/crc8.h>

DECLARE_CRC8_TABLE(lt6911uxe_crc_table);

static struct cam_sensor_ctrl_t *cam_hdmi_bdg_uxe_cam_ctrl = NULL;
static bool bInitCRC = false;

enum lt6911uxe_fw_status {
	UPDATE_SUCCESS = 0,
	UPDATE_RUNNING = 1,
	UPDATE_FAILED = 2,
};

// LT6911UXE irq type shows below:
// 0xE084 = 0 : Disconnected
// 0xE084 = 1 : video ready
// 0xE084 = 2 : audio disappear
// 0xE084 = 3 : audio ready
enum lt6911uxe_irq_type {
	LT6911UXE_HDMI_DISCONNECT = 0,
	LT6911UXE_HDMI_VIDEO_READY = 1,
	LT6911UXE_HDMI_AUDIO_DISAPPEAR = 2,
	LT6911UXE_HDMI_AUDIO_READY = 3,
};

enum lt6911uxe_fw_status lt6911uxe_fw_status;

int lt6911uxe_get_fw_state(void)
{
	return lt6911uxe_fw_status;
}

void lt6911uxe_set_fw_state(int fw_status)
{
	lt6911uxe_fw_status = fw_status;
}

static int lt6911uxe_flash_write_en(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[6] = {
		{ .reg_addr = 0xFF,
		  .reg_data = 0xE1,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x03,
		  .reg_data = 0x3F,
		  .delay = 0x14,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x03,
		  .reg_data = 0xFF,
		  .delay = 0x14,
		  .data_mask = 0x00 },
		{ .reg_addr = 0xFF,
		  .reg_data = 0xE0,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x04,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 6;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to flash write enable rc %d", rc);

	return rc;
}

static int lt6911uxe_flash_write_config(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[4] = {
		{ .reg_addr = 0x5E,
		  .reg_data = 0xDF,
		  .delay = 0x14,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x20,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x58,
		  .reg_data = 0x21,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};

	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 4;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to flash write config rc %d", rc);
	return rc;
}

static int lt6911uxe_write(struct cam_sensor_ctrl_t *s_ctrl, u8 reg, const u8 *buf,
			int size)
{
	int32_t rc = 0;
	u8 i2c_wbuf[64];
	int i;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[32];
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	memset(i2c_wbuf, 0, 64);
	memcpy(i2c_wbuf, buf, size);
	for (i = 0; i < size; i++) {
		m_i2c_write_regs[i].reg_addr = reg;
		m_i2c_write_regs[i].reg_data = i2c_wbuf[i];
		m_i2c_write_regs[i].delay = 0x00;
		m_i2c_write_regs[i].data_mask = 0x00;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	/* burst setting. */
	rc = camera_io_dev_write_continuous(&(s_ctrl->io_master_info),
					    &m_i2c_write_settings,
					    CAM_SENSOR_I2C_WRITE_BURST);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to write data rc %d", rc);

	return rc;
}

u8 lt6911uxe_calculate_crc8(const u8 *f_data, int size)
{
	u8 crc = 0;
	u16 block_size = 0x8000;
	u8 *crc_data = NULL;
	int i = 0;

	crc_data = kzalloc(block_size, GFP_KERNEL);
	if (!crc_data)
		return -ENOMEM;

	for (i = 0; i < size; i++)
		crc_data[i] = f_data[i];

	for (i = size; i < block_size; i++)
		crc_data[i] = 0xFF;

	crc = crc8(lt6911uxe_crc_table, crc_data, block_size - 1, crc);

	kfree(crc_data);
	return crc;
}

static int lt6911uxe_flash_write_addr_set(struct cam_sensor_ctrl_t *s_ctrl,
				       u32 addr)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[5] = {
		{ .reg_addr = 0x5B,
		  .reg_data = (addr & 0xFF0000) >> 16,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5C,
		  .reg_data = (addr & 0xFF00) >> 8,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5D,
		  .reg_data = addr & 0xFF,
		  .delay = 0x14,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x10,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};

	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 5;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to flash write addr rc %d", rc);

	return rc;
}

static int lt6911uxe_write_over_config(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[2] = {
		{ .reg_addr = 0x5A,
		  .reg_data = 0x08,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};

	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 2;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to write over config rc %d", rc);

	return rc;
}

static int lt6911uxe_firmware_write(struct cam_sensor_ctrl_t *s_ctrl,
				 const u8 *f_data, int size, int addr)
{
	int32_t rc = 0;
	u8 last_buf[32];
	int i = 0, page_size = 32;
	int start_addr = addr, total_page = 0, rest_data = 0;
	u32 crc_addr = ((addr & 0xFF0000) | 0x007FFF);
	const u8 *f_data_start_addr = f_data;
	u8 crc_buf[2];

	total_page = size / page_size;
	rest_data = size % page_size;

	for (i = 0; i < total_page; i++) {
		/* reset fifo */
		rc = lt6911uxe_flash_write_en(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to flash write enable rc %d", rc);
			return rc;
		}

		rc = lt6911uxe_flash_write_config(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to flash write config rc %d", rc);
			break;
		}
		rc = lt6911uxe_write(s_ctrl, 0x59, f_data, page_size);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed to write data rc %d", rc);
			break;
		}
		rc = lt6911uxe_flash_write_addr_set(s_ctrl, start_addr);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed to flash write addr rc %d",
				rc);
			break;
		}
		start_addr += page_size;
		f_data += page_size;
		msleep(20);
	}
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to write firmware rc %d", rc);
		return rc;
	}

	if (rest_data > 0) {
		memset(last_buf, 0xFF, 32);
		memcpy(last_buf, f_data, rest_data);

		/* reset fifo */
		rc = lt6911uxe_flash_write_en(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to flash write enable rc %d", rc);
			return rc;
		}

		rc = lt6911uxe_flash_write_config(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to flash write config rc %d", rc);
			return rc;
		}
		rc = lt6911uxe_write(s_ctrl, 0x59, last_buf, rest_data);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed to write data rc %d", rc);
			return rc;
		}
		rc = lt6911uxe_flash_write_addr_set(s_ctrl, start_addr);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed to flash write addr rc %d",
				rc);
			return rc;
		}
		msleep(20);
	}

	// write crc8
	memset(crc_buf, 0xFF, 2);
	crc_buf[0] = lt6911uxe_calculate_crc8(f_data_start_addr, size);
	lt6911uxe_flash_write_en(s_ctrl);
	lt6911uxe_flash_write_config(s_ctrl);
	lt6911uxe_write(s_ctrl, 0x59, crc_buf, 1);
	lt6911uxe_flash_write_addr_set(s_ctrl, crc_addr);

	rc = lt6911uxe_write_over_config(s_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to write over config rc %d", rc);
		return rc;
	}
	msleep(20);

	CAM_INFO(CAM_SENSOR,
		 "LT6911UXE FW write over, total size: %d, "
		 "page: %d, rest: %d",
		 size, total_page, rest_data);
	return rc;
}

static int lt6911uxe_block_erase(struct cam_sensor_ctrl_t *s_ctrl, int addr)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[9] = {
		{ .reg_addr = 0xFF,
		  .reg_data = 0xE0,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0xEE,
		  .reg_data = 0x01,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x04,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5B,
		  .reg_data = (addr & 0xFF0000) >> 16,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5C,
		  .reg_data = (addr & 0xFF00) >> 8,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5D,
		  .reg_data = (addr & 0xFF),
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x01,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};

	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 9;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to erase block rc %d", rc);

	msleep(3000);
	return rc;
}

static int lt6911uxe_config(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[8] = {
		{ .reg_addr = 0xFF,
		  .reg_data = 0xE0,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0xEE,
		  .reg_data = 0x01,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5E,
		  .reg_data = 0xC1,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x58,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x59,
		  .reg_data = 0x50,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x10,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x58,
		  .reg_data = 0x21,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};

	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 8;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to write config settings rc = %d",
			rc);

	return rc;
}

static int lt6911uxe_flash_read_addr_set(struct cam_sensor_ctrl_t *s_ctrl,
				      u32 addr)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[9] = {
		{ .reg_addr = 0x5E,
		  .reg_data = 0x5F,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x20,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5B,
		  .reg_data = (addr & 0xFF0000) >> 16,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5C,
		  .reg_data = (addr & 0xFF00) >> 8,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5D,
		  .reg_data = addr & 0xFF,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x10,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x5A,
		  .reg_data = 0x00,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0x58,
		  .reg_data = 0x21,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};

	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 9;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to flash read addr rc %d", rc);

	return rc;
}

static int lt6911uxe_read(struct cam_sensor_ctrl_t *s_ctrl, u8 reg, char *buf,
		       u32 size)
{
	uint8_t data[32];
	int rc = 0;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	memset(data, 0x0, size);
	rc = camera_io_dev_read_seq(&(s_ctrl->io_master_info), reg, data,
				    CAMERA_SENSOR_I2C_TYPE_BYTE,
				    CAMERA_SENSOR_I2C_TYPE_BYTE, size);
	if (!rc)
		memcpy(buf, data, size);

	return rc;
}

static int lt6911uxe_fw_read_back(struct cam_sensor_ctrl_t *s_ctrl, u8 *buff,
			       int size, int addr)
{
	u8 page_data[32];
	int page_number = 0, i = 0;
	int rest_data = 0;
	int rc = 0;
	struct cam_sensor_i2c_reg_array m_i2c_write_regs[2] = {
		{ .reg_addr = 0xFF,
		  .reg_data = 0xE0,
		  .delay = 0x01,
		  .data_mask = 0x00 },
		{ .reg_addr = 0xEE,
		  .reg_data = 0x01,
		  .delay = 0x01,
		  .data_mask = 0x00 }
	};

	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	/*
	 * Read 32 bytes once.
	 */
	page_number = size / 32;
	rest_data = size % 32;

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 2;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);

	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to write settings rc %d", rc);
		return rc;
	}

	for (i = 0; i < page_number; i++) {
		memset(page_data, 0x0, 32);
		rc = lt6911uxe_flash_read_addr_set(s_ctrl, addr);
		if (rc < 0)
			return rc;

		rc = lt6911uxe_read(s_ctrl, 0x5F, page_data, 32);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed to read rc %d", rc);
			return rc;
		}
		memcpy(buff, page_data, 32);
		buff += 32;
		addr += 32;
	}

	if (rest_data != 0) {
		memset(page_data, 0x0, rest_data);
		rc = lt6911uxe_flash_read_addr_set(s_ctrl, addr);
		if (rc < 0)
			return rc;
		rc = lt6911uxe_read(s_ctrl, 0x5F, page_data, rest_data);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed to read rc %d", rc);
			return rc;
		}
		memcpy(buff, page_data, rest_data);
		buff += rest_data;
		addr += rest_data;
	}

	/* disable external i2c control */
	m_i2c_write_regs[0].reg_addr = 0xFF;
	m_i2c_write_regs[0].reg_data = 0xE0;
	m_i2c_write_regs[1].reg_addr = 0xEE;
	m_i2c_write_regs[1].reg_data = 0x00;

	m_i2c_write_settings.reg_setting = m_i2c_write_regs;
	m_i2c_write_settings.size = 2;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to write settings rc %d", rc);
		return rc;
	}

	CAM_INFO(CAM_SENSOR, "lt6911uxe_fw_read_back:read data size : %d", addr);

	return rc;
}

int lt6911uxe_firmware_upgrade(struct cam_sensor_ctrl_t *s_ctrl,
			    const struct firmware *cfg)
{
	int i = 0;
	u8 *fw_read_data = NULL;
	int data_len = 0;
	int rc = -1;

	if (NULL == cfg) {
		CAM_ERR(CAM_SENSOR, "Invalid args");
		return -EINVAL;
	}

	data_len = (int)cfg->size;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	fw_read_data = kzalloc(ALIGN(data_len, 32), GFP_KERNEL);
	if (!fw_read_data)
		return rc;

	lt6911uxe_fw_status = UPDATE_RUNNING;
	rc = lt6911uxe_config(s_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to write config settings rc %d",
			rc);
		goto end;
	}

	/*
	* Need erase block 2 times here.
	* Sometimes, erase can fail.
	*/
	for (i = 0; i < 2; i++) {
		rc = lt6911uxe_block_erase(s_ctrl, 0x0);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,
				"Failed to erase block rc %d, retrying...", rc);
		else
			break;
	}

	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to erase blocks rc %d", rc);
		goto end;
	}

	rc = lt6911uxe_firmware_write(s_ctrl, cfg->data, data_len, 0x0);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to write firmware rc %d", rc);
		goto end;
	}
	msleep(20);

	rc = lt6911uxe_fw_read_back(s_ctrl, fw_read_data, data_len, 0x0);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to read back firmware rc %d", rc);
		goto end;
	}
	if (!memcmp(cfg->data, fw_read_data, data_len)) {
		lt6911uxe_fw_status = UPDATE_SUCCESS;
		CAM_INFO(CAM_SENSOR, "LT6911 Firmware upgrade success.");
		kfree(fw_read_data);
		return rc;
	}
end:
	lt6911uxe_fw_status = UPDATE_FAILED;
	kfree(fw_read_data);
	return rc;
}

static void lt6911uxe_firmware_cb(const struct firmware *cfg, void *data)
{
	struct cam_sensor_ctrl_t *s_ctrl = (struct cam_sensor_ctrl_t *)data;
	int rc = 0;

	if (!cfg) {
		CAM_ERR(CAM_SENSOR, "LT6911UXE get firmware failed");
		lt6911uxe_set_fw_state(UPDATE_FAILED);
		return;
	}

	rc = lt6911uxe_firmware_upgrade(s_ctrl, cfg);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to upgrade firmware rc %d", rc);
	release_firmware(cfg);
}

uint32_t cam_hdmi_bdg_uxe_get_fw_version(void)
{
	struct cam_sensor_ctrl_t *s_ctrl = cam_hdmi_bdg_uxe_cam_ctrl;
	uint32_t version = 0;
	uint32_t buf = 0;
	int rc = 0;

	struct cam_sensor_i2c_reg_array m_i2cWriteRegArray;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, "LT6911 is not ready.");
		goto end;
	}

	// Initialize
	m_i2cWriteRegArray.reg_addr = 0;
	m_i2cWriteRegArray.reg_data = 0;
	m_i2cWriteRegArray.delay = 0;
	m_i2cWriteRegArray.data_mask = 0;

	m_i2c_write_settings.reg_setting = &m_i2cWriteRegArray;
	m_i2c_write_settings.size = 1;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;
	m_i2cWriteRegArray.reg_addr = 0xFF;
	m_i2cWriteRegArray.reg_data = 0xE0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				 &m_i2c_write_settings);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to write registers rc %d", rc);
		goto end;
	}
	/* version byte 1 read */
	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x82, &buf,
				CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE, true);
	if (!rc)
		version += (buf & 0xFF) << 8;
	else {
		CAM_ERR(CAM_SENSOR, "LT6911UXE get revison failed");
		goto end;
	}
	/* version byte 2 read */
	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x83, &buf,
				CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE, true);
	if (!rc)
		version += buf & 0xFF;
	else {
		CAM_ERR(CAM_SENSOR, "LT6911UXE get revison failed");
		goto end;
	}
	return version;
end:
	version = 0;
	return version;
}

int cam_hdmi_bdg_uxe_set_cam_ctrl(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct cam_camera_slave_info *slave_info;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed s_ctrl: %pK", s_ctrl);
		return -EINVAL;
	}
	if (s_ctrl->sensordata == NULL) {
		CAM_ERR(CAM_SENSOR, " failed sensordata: %pK",
			s_ctrl->sensordata);
		return -EINVAL;
	}
	if (s_ctrl->sensordata == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl->sensordata);
		return -EINVAL;
	}
	slave_info = &(s_ctrl->sensordata->slave_info);
	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed slave_info: %pK", slave_info);
		return -EINVAL;
	}
	if (slave_info->sensor_id == HDMI_BDG_UXE_SENSOR_ID) {
		cam_hdmi_bdg_uxe_cam_ctrl = s_ctrl;
		CAM_ERR(CAM_SENSOR, "Setting sctrl for HDMI");
		rc = 0;
	} else {
		cam_hdmi_bdg_uxe_cam_ctrl = NULL;
		rc = -EINVAL;
	}
	return rc;
}

void cam_hdmi_bdg_uxe_unset_cam_ctrl(void)
{
	cam_hdmi_bdg_uxe_cam_ctrl = NULL;
}

int cam_hdmi_bdg_uxe_upgrade_firmware(void)
{
	int32_t rc = 0;
	struct cam_camera_slave_info *slave_info;
	struct cam_sensor_ctrl_t *s_ctrl = cam_hdmi_bdg_uxe_cam_ctrl;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, "LT6911UXE is not ready.");
		return -EINVAL;
	}
	if (s_ctrl->sensordata == NULL) {
		CAM_ERR(CAM_SENSOR, " failed sensordata: %pK",
			s_ctrl->sensordata);
		return -EINVAL;
	}

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", slave_info);
		return -EINVAL;
	}

	if(!bInitCRC)
	{
		// create crc8 table
		CAM_INFO(CAM_SENSOR, "crc8_populate_msb ");
		crc8_populate_msb(lt6911uxe_crc_table, 0x31);
		bInitCRC = true;
	}
	if (slave_info->sensor_id == HDMI_BDG_UXE_SENSOR_ID) {
		int max_wait_times = 25;

		/* wait 1 sec for hdmi bdg initial code is ready */
		msleep(1000);
		CAM_INFO(CAM_SENSOR,
			 "LT6911UXE firmare version before upgrade: 0x%08x",
			 cam_hdmi_bdg_uxe_get_fw_version());

		rc = request_firmware_nowait(THIS_MODULE, true, "lt6911uxe_fw.bin",
					     s_ctrl->soc_info.dev, GFP_KERNEL,
					     s_ctrl, lt6911uxe_firmware_cb);
		if (rc < 0)
			return rc;

		/* wait for firmware upgrade over, max wait time is 50 seconds */
		CAM_INFO(CAM_SENSOR, "LT6911UXE starts upgrade, "
				     "waiting for about 50s...");

		while (max_wait_times >= 0) {
			msleep(2000);
			if (lt6911uxe_get_fw_state() != UPDATE_RUNNING)
				break;
			max_wait_times--;

			CAM_INFO(CAM_SENSOR,
				 "LT6911UXE fw upgrade: "
				 "%d seconds have passed,most often %d seconds "
				 "have to be waited...",
				 ((25 - max_wait_times) * 2),
				 (max_wait_times * 2));
		}
	}
	if (lt6911uxe_get_fw_state() != UPDATE_SUCCESS)
		rc = -1;

	return rc;
}

int cam_hdmi_bdg_uxe_get_src_resolution(bool *signal_stable, int *width,
				    int *height, int *id)
{
	int rc = 0, ret = 0;
	u32 hactive_h, hactive_l;
	u32 vactive_h, vactive_l;
	u32 hdmi_signal_status;
	u32 audio_sample_rate;
	struct cam_sensor_i2c_reg_setting m_i2cWriteSettings;
	struct cam_sensor_i2c_reg_array m_i2cWriteRegArray;

	if (!cam_hdmi_bdg_uxe_cam_ctrl) {
		CAM_ERR(CAM_SENSOR, "LT6911UXE is not ready.");
		*signal_stable = false;
		*height = -1;
		*width = -1;
		return -EINVAL;
	}
	/* camera_io_init(&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info));*/
	/* cam_sensor_power_up(cam_hdmi_bdg_uxe_cam_ctrl);*/
	mutex_lock(&(cam_hdmi_bdg_uxe_cam_ctrl->cam_sensor_mutex));
	memset(&m_i2cWriteSettings, 0, sizeof(m_i2cWriteSettings));

	m_i2cWriteSettings.reg_setting = &m_i2cWriteRegArray;
	m_i2cWriteSettings.size = 1;
	m_i2cWriteSettings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2cWriteSettings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2cWriteSettings.delay = 0;

	m_i2cWriteRegArray.reg_addr = 0xff;
	m_i2cWriteRegArray.reg_data = 0xe0;

	rc = camera_io_dev_write(&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info),
				 &m_i2cWriteSettings);
	if (rc < 0)
		goto fail;

	/* Check HDMI signal first*/
	rc = camera_io_dev_read(&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info), 0x84,
				&hdmi_signal_status,
				CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE, true);
	CAM_INFO(CAM_SENSOR, "LT6911UXE signal stable %x", hdmi_signal_status);
	if (rc < 0)
		goto fail;

	switch (hdmi_signal_status) {
		case LT6911UXE_HDMI_DISCONNECT:
			*signal_stable = false;
			*height = 0;
			*width = 0;
			*id = cam_hdmi_bdg_uxe_cam_ctrl->id;
			ret = LT6911UXE_VIDEO_EVENT;
			goto end;
		case LT6911UXE_HDMI_VIDEO_READY:
			rc = camera_io_dev_read(
				&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info),
				0x8c, &hactive_h, CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE, true);
			if (rc < 0)
				goto fail;

			rc = camera_io_dev_read(
				&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info),
				0x8d, &hactive_l, CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE, true);
			if (rc < 0)
				goto fail;

			rc = camera_io_dev_read(
				&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info),
				0x8e, &vactive_h, CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE, true);
			if (rc < 0)
				goto fail;

			rc = camera_io_dev_read(
				&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info),
				0x8f, &vactive_l, CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE, true);
			if (rc < 0)
				goto fail;

			*signal_stable = true;
			*height = (vactive_h << 8) | vactive_l;
			*width = (hactive_h << 8) | hactive_l;
			*width = *width * 2;
			*id = cam_hdmi_bdg_uxe_cam_ctrl->id;
			CAM_INFO(CAM_SENSOR, "signal stable %d %d, x %d id %d", *signal_stable,
				*width, *height, *id);
			ret = LT6911UXE_VIDEO_EVENT;
			break;
		case LT6911UXE_HDMI_AUDIO_DISAPPEAR:
			*id = cam_hdmi_bdg_uxe_cam_ctrl->id;
			CAM_INFO(CAM_SENSOR, "Currently not handling audio event.");
			ret = LT6911UXE_AUDIO_EVENT;
			break;
		case LT6911UXE_HDMI_AUDIO_READY:
			*id = cam_hdmi_bdg_uxe_cam_ctrl->id;
			// check audio signal
			rc = camera_io_dev_read(
					&(cam_hdmi_bdg_uxe_cam_ctrl->io_master_info),
					0x91, &audio_sample_rate,
					CAMERA_SENSOR_I2C_TYPE_BYTE,
					CAMERA_SENSOR_I2C_TYPE_BYTE, true);
			if (rc < 0)
				goto fail;
			CAM_INFO(CAM_SENSOR, "audio sample rate: %d",
					audio_sample_rate);
			CAM_INFO(CAM_SENSOR, "Currently not handling audio event.");
			ret = LT6911UXE_AUDIO_EVENT;
			break;
		default:
			CAM_ERR(CAM_SENSOR, "Invalid signal status: %d",
					hdmi_signal_status);
			*signal_stable = false;
			*height = -1;
			*width = -1;
			*id = cam_hdmi_bdg_uxe_cam_ctrl->id;
			goto fail;
	}

end:
	mutex_unlock(&(cam_hdmi_bdg_uxe_cam_ctrl->cam_sensor_mutex));
	if (rc < 0) {
		ret = LT6911UXE_INVALID_EVENT;
	}
	return ret;

fail:
	ret = LT6911UXE_INVALID_EVENT;
	*signal_stable = false;
	*height = -1;
	*width = -1;
	mutex_unlock(&(cam_hdmi_bdg_uxe_cam_ctrl->cam_sensor_mutex));
	return ret;
}
EXPORT_SYMBOL(cam_hdmi_bdg_uxe_get_src_resolution);
EXPORT_SYMBOL(cam_hdmi_bdg_uxe_get_fw_version);
