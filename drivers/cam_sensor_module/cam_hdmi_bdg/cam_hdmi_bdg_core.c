/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_trace.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include "cam_hdmi_bdg_core.h"
#include "hdmi_lt6911gxc_sensor.h"
#include "hdmi_lt6911uxc_sensor.h"

static struct cam_sensor_ctrl_t *cam_hdmi_bdg_cam_ctrl = NULL;

enum lt6911_fw_status lt6911_fw_status;

static struct lt6911_reg_settings lt6911_reg_settings;

static void lt6911_assign_reg_settings(bool is_uxc)
{
	if (is_uxc) {
		lt6911_reg_settings.is_uxc = true;
		lt6911_reg_settings.write_en_regs = NULL;
		lt6911_reg_settings.write_en_size = 0;
		lt6911_reg_settings.write_config_regs = uxc_write_config_regs;
		lt6911_reg_settings.write_config_size = ARRAY_SIZE(uxc_write_config_regs);
		lt6911_reg_settings.write_addr_set_regs = uxc_write_addr_set_regs;
		lt6911_reg_settings.write_addr_set_size = ARRAY_SIZE(uxc_write_addr_set_regs);
		lt6911_reg_settings.write_addr_data_index = 1;
		lt6911_reg_settings.write_over_regs = uxc_write_over_regs;
		lt6911_reg_settings.write_over_size = ARRAY_SIZE(uxc_write_over_regs);
		lt6911_reg_settings.block_erase_regs = uxc_block_erase_regs;
		lt6911_reg_settings.block_erase_size = ARRAY_SIZE(uxc_block_erase_regs);
		lt6911_reg_settings.block_erase_delay = 1000;
		lt6911_reg_settings.gxc_block_erase_ext_regs = NULL;
		lt6911_reg_settings.gxc_block_erase_ext_size = 0;
		lt6911_reg_settings.config_regs = uxc_config_regs;
		lt6911_reg_settings.config_size = ARRAY_SIZE(uxc_config_regs);
		lt6911_reg_settings.read_addr_regs = uxc_read_addr_regs;
		lt6911_reg_settings.read_addr_size = ARRAY_SIZE(uxc_read_addr_regs);
		lt6911_reg_settings.read_addr_data_index = 4;
		lt6911_reg_settings.get_fw_regs = uxc_get_fw_regs;
		lt6911_reg_settings.get_fw_size = ARRAY_SIZE(uxc_get_fw_regs);
		lt6911_reg_settings.erase_time = 3;
	} else {
		lt6911_reg_settings.is_uxc = false;
		lt6911_reg_settings.write_en_regs = gxc_write_en_regs;
		lt6911_reg_settings.write_en_size = ARRAY_SIZE(gxc_write_en_regs);
		lt6911_reg_settings.write_config_regs = gxc_write_config_regs;
		lt6911_reg_settings.write_config_size = ARRAY_SIZE(gxc_write_config_regs);
		lt6911_reg_settings.write_addr_set_regs = gxc_write_addr_set_regs;
		lt6911_reg_settings.write_addr_set_size = ARRAY_SIZE(gxc_write_addr_set_regs);
		lt6911_reg_settings.write_addr_data_index = 0;
		lt6911_reg_settings.write_over_regs = gxc_write_over_regs;
		lt6911_reg_settings.write_over_size = ARRAY_SIZE(gxc_write_over_regs);
		lt6911_reg_settings.block_erase_regs = gxc_block_erase_regs;
		lt6911_reg_settings.block_erase_size = ARRAY_SIZE(gxc_block_erase_regs);
		lt6911_reg_settings.block_erase_delay = 500;
		lt6911_reg_settings.gxc_block_erase_ext_regs = gxc_block_erase_ext_regs;
		lt6911_reg_settings.gxc_block_erase_ext_size = ARRAY_SIZE(gxc_block_erase_ext_regs);
		lt6911_reg_settings.config_regs = gxc_config_regs;
		lt6911_reg_settings.config_size = ARRAY_SIZE(gxc_config_regs);
		lt6911_reg_settings.read_addr_regs = gxc_read_addr_regs;
		lt6911_reg_settings.read_addr_size = ARRAY_SIZE(gxc_read_addr_regs);
		lt6911_reg_settings.read_addr_data_index = 3;
		lt6911_reg_settings.get_fw_regs = gxc_get_fw_regs;
		lt6911_reg_settings.get_fw_size = ARRAY_SIZE(gxc_get_fw_regs);
		lt6911_reg_settings.erase_time = 2;
	}
}

int lt6911_get_fw_state(void)
{
	return lt6911_fw_status;
}

static void lt6911_set_fw_state(int fw_status)
{
	lt6911_fw_status = fw_status;
}

static int lt6911_disable_i2c(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc;
	struct cam_sensor_i2c_reg_array disable_regs[2] = {
		{ .reg_addr = 0xFF },
		{ .reg_addr = 0xEE, .reg_data = 0x00 },
	};
	struct cam_sensor_i2c_reg_setting settings = {
		.reg_setting = disable_regs,
		.size = ARRAY_SIZE(disable_regs),
		.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = 0,
	};

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	disable_regs[0].reg_data = lt6911_reg_settings.is_uxc ? 0x80 : 0xE0;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "Failed to disable LT6911 external I2C rc %d",
				rc);

	return rc;
}

static int lt6911uxc_check_chip_id(struct cam_sensor_ctrl_t *s_ctrl)
{
	uint32_t chip_id_h = 0, chip_id_l = 0;
	int32_t rc, cleanup_rc;
	struct cam_sensor_i2c_reg_setting settings = {
		.reg_setting = uxc_get_fw_regs,
		.size = ARRAY_SIZE(uxc_get_fw_regs),
		.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = 0,
	};

	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto disable_i2c;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x00,
			&chip_id_h, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		goto disable_i2c;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x01,
			&chip_id_l, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		goto disable_i2c;

	if (((chip_id_h << 8) | chip_id_l) != HDMI_UXC_SENSOR_ID) {
		CAM_ERR(CAM_SENSOR, "Unexpected LT6911UXC chip ID: 0x%02x%02x",
				chip_id_h, chip_id_l);
		rc = -ENODEV;
	}

disable_i2c:
	cleanup_rc = lt6911_disable_i2c(s_ctrl);
	if (!rc && cleanup_rc < 0)
		rc = cleanup_rc;
	return rc;
}

static int lt6911_flash_write_en(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint32_t reg_data = 0;
	struct cam_sensor_i2c_reg_array uxc_wren_regs[6] = {
		{ .reg_addr = 0xFF, .reg_data = 0x81 },
		{ .reg_addr = 0x08 },
		{ .reg_addr = 0x08 },
		{ .reg_addr = 0xFF, .reg_data = 0x80 },
		{ .reg_addr = 0x5A, .reg_data = 0x04 },
		{ .reg_addr = 0x5A, .reg_data = 0x00 },
	};
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	if (lt6911_reg_settings.is_uxc) {
		m_i2c_write_settings.reg_setting = uxc_wren_regs;
		m_i2c_write_settings.size = 1;
	} else {
		m_i2c_write_settings.reg_setting = lt6911_reg_settings.write_en_regs;
		m_i2c_write_settings.size = lt6911_reg_settings.write_en_size;
	}
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
			&m_i2c_write_settings);
	if (rc < 0)
		goto end;

	if (lt6911_reg_settings.is_uxc) {
		rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x08,
				&reg_data, CAMERA_SENSOR_I2C_TYPE_BYTE,
				CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0)
			goto end;

		reg_data &= 0xBF;
		uxc_wren_regs[1].reg_data = reg_data;
		uxc_wren_regs[2].reg_data = reg_data | 0x40;
		m_i2c_write_settings.reg_setting = &uxc_wren_regs[1];
		m_i2c_write_settings.size = ARRAY_SIZE(uxc_wren_regs) - 1;
		rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				&m_i2c_write_settings);
	}

end:
	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to flash write enable rc %d", rc);

	return rc;
}

static int lt6911_flash_write_config(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = lt6911_reg_settings.write_config_regs;
	m_i2c_write_settings.size = lt6911_reg_settings.write_config_size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
			&m_i2c_write_settings);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to flash write config rc %d", rc);
	return rc;
}

static int lt6911_write(struct cam_sensor_ctrl_t *s_ctrl, u8 reg,
			const u8 *buf, int size)
{
	int32_t rc = 0;
	u8 i2c_wbuf[64];
	int i;
	struct cam_sensor_i2c_reg_array  m_i2c_write_regs[32];
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	memset(i2c_wbuf, 0, 64);
	if (size <= 0 || size > 32) {
	        CAM_ERR(CAM_SENSOR, "Invalid write size : %d", size);
	        return -EINVAL;
	}
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
			&m_i2c_write_settings, CAM_SENSOR_I2C_WRITE_BURST);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to write data rc %d", rc);

	return rc;
}

static int lt6911_flash_write_addr_set(struct cam_sensor_ctrl_t *s_ctrl, u32 addr)
{
	int32_t rc = 0;
	int index;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	index = lt6911_reg_settings.write_addr_data_index;
	if (index < 0 || index + 2 >= lt6911_reg_settings.write_addr_set_size) {
		CAM_ERR(CAM_SENSOR, "write_addr_data_index out of bounds: %d", index);
		return -EINVAL;
	}
	lt6911_reg_settings.write_addr_set_regs[index].reg_data =
		(addr & 0xFF0000) >> 16;
	lt6911_reg_settings.write_addr_set_regs[index + 1].reg_data =
		(addr & 0x00FF00) >> 8;
	lt6911_reg_settings.write_addr_set_regs[index + 2].reg_data =
		addr & 0x0000FF;
	m_i2c_write_settings.reg_setting = lt6911_reg_settings.write_addr_set_regs;
	m_i2c_write_settings.size = lt6911_reg_settings.write_addr_set_size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
			&m_i2c_write_settings);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to flash write addr rc %d", rc);

	return rc;
}

static int lt6911_write_over_config(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = lt6911_reg_settings.write_over_regs;
	m_i2c_write_settings.size = lt6911_reg_settings.write_over_size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
			&m_i2c_write_settings);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to write over config rc %d", rc);

	return rc;
}

static int lt6911_firmware_write(struct cam_sensor_ctrl_t *s_ctrl, const u8 *f_data,
			int size)
{
	int32_t rc = 0;
	u8 last_buf[32];
	int i = 0, page_size = 32;
	int start_addr = 0, total_page = 0, rest_data = 0;

	total_page = size / page_size;
	rest_data = size % page_size;

	for (i = 0; i < total_page; i++) {
		/* reset fifo */
		rc = lt6911_flash_write_en(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to flash write enable rc %d", rc);
			return rc;
		}

		rc = lt6911_flash_write_config(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to flash write config rc %d", rc);
			break;
		}
		rc = lt6911_write(s_ctrl, 0x59, f_data, page_size);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to write data rc %d", rc);
			break;
		}
		rc = lt6911_flash_write_addr_set(s_ctrl, start_addr);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to flash write addr rc %d", rc);
			break;
		}
		if (lt6911_reg_settings.is_uxc)
			usleep_range(200, 250);
		start_addr += page_size;
		f_data += page_size;
	}
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to write firmware rc %d", rc);
		return rc;
	}

	if (rest_data > 0) {
		memset(last_buf, 0xFF, 32);
		memcpy(last_buf, f_data, rest_data);

		rc = lt6911_flash_write_en(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to flash write enable rc %d", rc);
			return rc;
		}

		rc = lt6911_flash_write_config(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to flash write config rc %d", rc);
			return rc;
		}
		rc = lt6911_write(s_ctrl, 0x59, last_buf, rest_data);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to write data rc %d", rc);
			return rc;
		}
		rc = lt6911_flash_write_addr_set(s_ctrl, start_addr);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to flash write addr rc %d", rc);
			return rc;
		}
		if (lt6911_reg_settings.is_uxc)
			usleep_range(200, 250);
	}

	rc = lt6911_write_over_config(s_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to write over config rc %d", rc);
		return rc;
	}

	CAM_INFO(CAM_SENSOR, "LT6911 FW write over, total size: %d, "
			"page: %d, reset: %d", size, total_page, rest_data);
	return rc;
}

static int lt6911_block_erase(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = lt6911_reg_settings.block_erase_regs;
	m_i2c_write_settings.size = lt6911_reg_settings.block_erase_size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = lt6911_reg_settings.block_erase_delay;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
			&m_i2c_write_settings);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to erase block rc %d", rc);

	if (lt6911_reg_settings.gxc_block_erase_ext_regs) {
		m_i2c_write_settings.reg_setting =
			lt6911_reg_settings.gxc_block_erase_ext_regs;
		m_i2c_write_settings.size =
			lt6911_reg_settings.gxc_block_erase_ext_size;
		m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
		m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
		m_i2c_write_settings.delay =
			lt6911_reg_settings.block_erase_delay;

		rc = camera_io_dev_write(&(s_ctrl->io_master_info),
				&m_i2c_write_settings);

		if (rc < 0)
			CAM_ERR(CAM_SENSOR,"Failed to erase block rc %d", rc);
	}

	return rc;
}

static int lt6911_config(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = lt6911_reg_settings.config_regs;
	m_i2c_write_settings.size = lt6911_reg_settings.config_size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
			&m_i2c_write_settings);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to write config settings rc = %d", rc);

	return rc;
}

static int lt6911_flash_read_addr_set(struct cam_sensor_ctrl_t *s_ctrl, u32 addr)
{
	int32_t rc = 0;
	int index;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	index = lt6911_reg_settings.read_addr_data_index;
	if (index < 0 || index + 2 >= lt6911_reg_settings.read_addr_size) {
		CAM_ERR(CAM_SENSOR, "Invalid read address index %d, array size %d",
			index, lt6911_reg_settings.read_addr_size);
		return -EINVAL;
	}
	lt6911_reg_settings.read_addr_regs[index].reg_data =
		(addr & 0xFF0000) >> 16;
	lt6911_reg_settings.read_addr_regs[index + 1].reg_data =
		(addr & 0x00FF00) >> 8;
	lt6911_reg_settings.read_addr_regs[index + 2].reg_data =
		addr & 0x0000FF;
	m_i2c_write_settings.reg_setting = lt6911_reg_settings.read_addr_regs;
	m_i2c_write_settings.size = lt6911_reg_settings.read_addr_size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info),
			&m_i2c_write_settings);

	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to flash read addr rc %d", rc);

	return rc;
}

static int lt6911_read(struct cam_sensor_ctrl_t *s_ctrl, u8 reg,
			char *buf, u32 size)
{
	uint8_t data[32];
	int rc = 0;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	memset(data, 0x0, size);
	rc = camera_io_dev_read_seq(&(s_ctrl->io_master_info), reg, data,
			CAMERA_SENSOR_I2C_TYPE_BYTE, CAMERA_SENSOR_I2C_TYPE_BYTE, size);
	if (!rc)
		memcpy(buf, data, size);

	return rc;
}

static int lt6911_fw_read_back(struct cam_sensor_ctrl_t *s_ctrl, u8 *buff, int size)
{
	u8 page_data[32];
	int page_number = 0, i = 0, addr = 0;
	int rest_data = 0;
	int rc = 0, cleanup_rc;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", s_ctrl);
		return -EINVAL;
	}

	/*
	 * Read 32 bytes once.
	 */
	page_number = size / 32;
	rest_data = size % 32;

	for (i = 0; i < page_number; i++) {
		memset(page_data, 0x0, 32);
		if (lt6911_reg_settings.is_uxc) {
			rc = lt6911_flash_write_en(s_ctrl);
			if (rc < 0)
				goto cleanup;
		}
		rc = lt6911_flash_read_addr_set(s_ctrl, addr);
		if (rc < 0)
			goto cleanup;

		rc = lt6911_read(s_ctrl, 0x5F, page_data, 32);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to read rc %d", rc);
			goto cleanup;
		}
		memcpy(buff, page_data, 32);
		if (lt6911_reg_settings.is_uxc)
			usleep_range(150, 200);
		buff += 32;
		addr += 32;
	}

	if ((rest_data > 0) && (rest_data < 32)) {
		memset(page_data, 0x0, rest_data);
		if (lt6911_reg_settings.is_uxc) {
			rc = lt6911_flash_write_en(s_ctrl);
			if (rc < 0)
				goto cleanup;
		}
		rc = lt6911_flash_read_addr_set(s_ctrl, addr);
		if (rc < 0)
			goto cleanup;
		rc = lt6911_read(s_ctrl, 0x5F, page_data, rest_data);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,"Failed to read rc %d", rc);
			goto cleanup;
		}
		memcpy(buff, page_data, rest_data);
		if (lt6911_reg_settings.is_uxc)
			usleep_range(150, 200);
		buff += rest_data;
		addr += rest_data;
	}

cleanup:
	cleanup_rc = lt6911_write_over_config(s_ctrl);
	if (cleanup_rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to write over config rc %d", cleanup_rc);
		if (!rc)
			rc = cleanup_rc;
	}

	if (!rc)
		CAM_INFO(CAM_SENSOR,"lt6911_fw_read_back:read data size : %d", addr);

	return rc;
}

static int lt6911_firmware_upgrade(struct cam_sensor_ctrl_t *s_ctrl,
			const struct firmware *cfg)
{
	int i = 0;
	u8 *fw_read_data = NULL;
	int erase_time;
	int data_len = 0;
	int rc = -1, cleanup_rc;

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
	if (!fw_read_data) {
	        lt6911_fw_status = UPDATE_FAILED;
	        return -ENOMEM;
	}

	lt6911_fw_status = UPDATE_RUNNING;
	if (lt6911_reg_settings.is_uxc) {
		rc = lt6911uxc_check_chip_id(s_ctrl);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "Failed to check LT6911UXC chip ID rc %d", rc);
			lt6911_fw_status = UPDATE_FAILED;
			kfree(fw_read_data);
			return rc;
		}
	}
	rc = lt6911_config(s_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to write config settings rc %d", rc);
		goto end;
	}

	/*
	* Need erase block 2 times here.
	* lt6911uxc need erase 3 times.
	* Sometimes, erase can fail.
	*/
	erase_time = lt6911_reg_settings.erase_time;
	for (i = 0; i < erase_time; i++) {
		rc = lt6911_block_erase(s_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR,"Failed to erase block rc %d, retrying...", rc);
		else
			break;
	}

	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to erase blocks rc %d", rc);
		goto end;
	}

	rc = lt6911_firmware_write(s_ctrl, cfg->data, data_len);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to write firmware rc %d", rc);
		goto end;
	}

	rc = lt6911_fw_read_back(s_ctrl, fw_read_data, data_len);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to read back firmware rc %d", rc);
		goto end;
	}
	if (!memcmp(cfg->data, fw_read_data, data_len)) {
		lt6911_fw_status = UPDATE_SUCCESS;
		CAM_INFO(CAM_SENSOR, "LT6911 Firmware upgrade success.");
	} else {
		lt6911_fw_status = UPDATE_FAILED;
		if (lt6911_reg_settings.is_uxc) {
			CAM_ERR(CAM_SENSOR, "LT6911 firmware read-back mismatch");
			rc = -EIO;
		}
	}
end:
	cleanup_rc = lt6911_disable_i2c(s_ctrl);
	if (!rc && cleanup_rc < 0)
		rc = cleanup_rc;

	if (rc < 0)
		lt6911_fw_status = UPDATE_FAILED;
	kfree(fw_read_data);
	return rc;
}

static void lt6911_firmware_cb(const struct firmware *cfg, void *data)
{
	struct cam_sensor_ctrl_t *s_ctrl = (struct cam_sensor_ctrl_t *)data;
	int rc = 0;

	if (!cfg) {
		CAM_ERR(CAM_SENSOR,"LT6911 get firmware failed");
		lt6911_set_fw_state(UPDATE_FAILED);
		return;
	}

	rc = lt6911_firmware_upgrade(s_ctrl, cfg);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR,"Failed to upgrade firmware rc %d", rc);
	release_firmware(cfg);
}

uint32_t cam_hdmi_bdg_get_fw_version(void)
{
	struct cam_sensor_ctrl_t *s_ctrl = cam_hdmi_bdg_cam_ctrl;
	uint32_t version = 0;
	uint32_t buf = 0;
	int rc = 0, cleanup_rc;
	struct cam_sensor_i2c_reg_setting m_i2c_write_settings;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, "LT6911 is not ready.");
		return -EINVAL;
	}
	if (s_ctrl->sensordata == NULL) {
		CAM_ERR(CAM_SENSOR, " failed sensordata: %pK", s_ctrl->sensordata);
		return -EINVAL;
	}

	m_i2c_write_settings.reg_setting = lt6911_reg_settings.get_fw_regs;
	m_i2c_write_settings.size = lt6911_reg_settings.get_fw_size;
	m_i2c_write_settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	m_i2c_write_settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &m_i2c_write_settings);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,"Failed to write registers rc %d", rc);
		goto end;
	}

	/* version byte 1 read */
	rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			0x80,
			&buf,
			CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if(!rc)
		version += (buf & 0xFF) << 24;
	else {
		CAM_ERR(CAM_SENSOR,"LT6911 get revison failed");
		goto end;
	}


	/* version byte 2 read */
	rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			0x81,
			&buf,
			CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if(!rc)
		version += (buf & 0xFF) << 16;
	else {
		CAM_ERR(CAM_SENSOR,"LT6911 get revison failed");
		goto end;
	}

	/* version byte 3 read */
	rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			0x82,
			&buf,
			CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if(!rc)
		version += (buf & 0xFF) << 8;
	else {
		CAM_ERR(CAM_SENSOR,"LT6911 get revison failed");
		goto end;
	}

	/* version byte 4 read */
	rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			0x83,
			&buf,
			CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if(!rc)
		version += buf & 0xFF;
	else {
		CAM_ERR(CAM_SENSOR,"LT6911 get revison failed");
		goto end;
	}

end:
	cleanup_rc = lt6911_disable_i2c(s_ctrl);
	if (rc < 0 || cleanup_rc < 0)
		version = 0;
	return version;
}

int cam_hdmi_bdg_set_cam_ctrl(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	struct cam_camera_slave_info *slave_info;

	if (!s_ctrl || !s_ctrl->sensordata) {
		CAM_ERR(CAM_SENSOR, "Invalid ptr");
		return -EINVAL;
	}

	slave_info = &(s_ctrl->sensordata->slave_info);
	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed slave_info: %pK", slave_info);
		return -EINVAL;
	}
	if (slave_info->sensor_id == HDMI_GXC_SENSOR_ID ||
		slave_info->sensor_id == HDMI_UXC_SENSOR_ID) {
		cam_hdmi_bdg_cam_ctrl = s_ctrl;
		if (s_ctrl->sensordata->slave_info.sensor_id == HDMI_UXC_SENSOR_ID &&
			s_ctrl->sensordata->slave_info.sensor_slave_addr == HDMI_UXC_SENSOR_SLAVE_ADDR) {
			lt6911_assign_reg_settings(true);
		} else {
			lt6911_assign_reg_settings(false);
		}
		CAM_INFO(CAM_SENSOR, "set_cam_ctrl for HDMI done");
		rc = 0;
	} else {
		cam_hdmi_bdg_cam_ctrl = NULL;
		rc = -EINVAL;
	}
	return rc;
}
EXPORT_SYMBOL(cam_hdmi_bdg_set_cam_ctrl);

void cam_hdmi_bdg_unset_cam_ctrl(void)
{
	cam_hdmi_bdg_cam_ctrl = NULL;
}

int cam_hdmi_bdg_upgrade_firmware(void)
{
	int32_t rc = 0;
	struct cam_camera_slave_info *slave_info;
	struct cam_sensor_ctrl_t *s_ctrl = cam_hdmi_bdg_cam_ctrl;

	if (s_ctrl == NULL) {
		CAM_ERR(CAM_SENSOR, "LT6911 is not ready.");
		return -EINVAL;
	}
	if (s_ctrl->sensordata == NULL) {
		CAM_ERR(CAM_SENSOR, " failed sensordata: %pK", s_ctrl->sensordata);
		return -EINVAL;
	}

	slave_info = &(s_ctrl->sensordata->slave_info);

	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed: %pK", slave_info);
		return -EINVAL;
	}

	if (slave_info->sensor_id == HDMI_GXC_SENSOR_ID ||
		slave_info->sensor_id == HDMI_UXC_SENSOR_ID) {
		int max_wait_times = 25;

		CAM_INFO(CAM_SENSOR,"LT6911 firmare version before upgrade: 0x%08x",
			cam_hdmi_bdg_get_fw_version());

		rc = request_firmware_nowait(THIS_MODULE, true,
				"lt6911_fw.bin", s_ctrl->soc_info.dev, GFP_KERNEL,
				s_ctrl, lt6911_firmware_cb);
		if (rc < 0)
			return rc;

		/* wait for firmware upgrade over, max wait time is 50 seconds */
		CAM_INFO(CAM_SENSOR, "LT6911 starts upgrade, "
				"waiting for about 50s...");

		while (max_wait_times >= 0) {
			msleep(2000);
			if (lt6911_get_fw_state() != UPDATE_RUNNING)
				break;
			max_wait_times--;

			CAM_INFO(CAM_SENSOR, "LT6911 fw upgrade: "
				"%d seconds have passed,most often %d seconds "
				"have to be waited...",((25-max_wait_times)*2),
				(max_wait_times*2));
		}
	}
	if (lt6911_get_fw_state() != UPDATE_SUCCESS)
		rc = -1;

	return rc;
}

static int lt6911gxc_get_src_resolution(struct cam_sensor_ctrl_t *s_ctrl,
	bool *signal_stable, int *width, int *height, int *id)
{
	int rc = 0, cleanup_rc;
	u32 hactive_h, hactive_l;
	u32 vactive_h, vactive_l;
	u32 hdmi_signal_status;
	struct cam_sensor_i2c_reg_setting settings;
	struct cam_sensor_i2c_reg_array reg;

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	memset(&settings, 0, sizeof(settings));
	settings.reg_setting = &reg;
	settings.size = 1;
	settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	settings.delay = 0;
	*signal_stable = false;
	*height = -1;
	*width = -1;
	*id = -1;

	reg.reg_addr = 0xFF;
	reg.reg_data = 0xE0;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	reg.reg_addr = 0xEE;
	reg.reg_data = 0x01;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	reg.reg_addr = 0xFF;
	reg.reg_data = 0xE0;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	reg.reg_addr = 0xB0;
	reg.reg_data = 0x01;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	/* Check the GXC HDMI signal-status register first. */
	reg.reg_addr = 0xFF;
	reg.reg_data = 0xE0;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x84,
			&hdmi_signal_status, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	CAM_INFO(CAM_SENSOR, "lt6911 signal stable %x", hdmi_signal_status);
	if (rc < 0)
		goto fail;

	if (hdmi_signal_status == HDMI_BDG_HDMI_DISCONNECTED) {
		*signal_stable = false;
		*height = 0;
		*width = 0;
		*id = s_ctrl->id;
		goto cleanup;
	}
	if (hdmi_signal_status > 0xFF) {
		*signal_stable = false;
		*height = -1;
		*width = -1;
		*id = s_ctrl->id;
		goto cleanup;
	}

	reg.reg_addr = 0xFF;
	reg.reg_data = 0xE0;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x8C,
			&hactive_h, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		goto fail;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x8D,
			&hactive_l, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		goto fail;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x8E,
			&vactive_h, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		goto fail;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0x8F,
			&vactive_l, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		goto fail;

	*signal_stable = true;
	*height = (vactive_h << 8) | vactive_l;
	*width = (hactive_h << 8) | hactive_l;
	*id = s_ctrl->id;
	CAM_INFO(CAM_SENSOR, "signal stable %d %d, x %d id %d",
			*signal_stable, *width, *height, *id);
	goto cleanup;

fail:
	*signal_stable = false;
	*height = -1;
	*width = -1;
	*id = -1;

cleanup:
	cleanup_rc = lt6911_disable_i2c(s_ctrl);
	if (!rc && cleanup_rc < 0)
		rc = cleanup_rc;
	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;
}

static int lt6911uxc_get_src_resolution(struct cam_sensor_ctrl_t *s_ctrl,
	bool *signal_stable, int *width, int *height, int *id)
{
	int rc = 0, cleanup_rc;
	u8 timing[8] = { 0 };
	u32 hactive, vactive, htotal, vtotal;
	struct cam_sensor_i2c_reg_array regs[2] = { 0 };
	struct cam_sensor_i2c_reg_setting settings;
	u32 hdmi_signal_status;

	mutex_lock(&(s_ctrl->cam_sensor_mutex));
	*id = s_ctrl->id;
	memset(&settings, 0, sizeof(settings));
	regs[0].reg_addr = 0xFF;
	regs[0].reg_data = 0x80;
	regs[1].reg_addr = 0xEE;
	regs[1].reg_data = 0x01;
	settings.reg_setting = regs;
	settings.size = 2;
	settings.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	settings.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	settings.delay = 0;

	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	regs[0].reg_addr = 0xFF;
	regs[0].reg_data = 0x86;
	settings.size = 1;
	rc = camera_io_dev_write(&(s_ctrl->io_master_info), &settings);
	if (rc < 0)
		goto fail;

	rc = camera_io_dev_read(&(s_ctrl->io_master_info), 0xA3,
			&hdmi_signal_status, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0)
		goto fail;
	CAM_INFO(CAM_SENSOR, "lt6911 signal stable %x", hdmi_signal_status);
	*signal_stable = (hdmi_signal_status == HDMI_UXC_HDMI_CONNECTED);
	if (!*signal_stable) {
		if (hdmi_signal_status != HDMI_UXC_HDMI_DISCONNECTED)
			CAM_INFO(CAM_SENSOR, "Unexpected LT6911UXC HDMI status 0x%x", hdmi_signal_status);
		*width = 0;
		*height = 0;
		goto disable_i2c;
	}

	/* Vtotal, Htotal, Vactive and Hactive are contiguous. */
	rc = camera_io_dev_read_seq(&(s_ctrl->io_master_info), 0x7A,
			timing, CAMERA_SENSOR_I2C_TYPE_BYTE,
			CAMERA_SENSOR_I2C_TYPE_BYTE, ARRAY_SIZE(timing));
	if (rc < 0)
		goto fail;

	vtotal = (timing[0] << 8) | timing[1];
	/* UXC reports horizontal timing in half-pixel units. */
	htotal = (((timing[2] & 0x0F) << 8) | timing[3]) << 1;
	vactive = (timing[4] << 8) | timing[5];
	hactive = (((timing[6] & 0x0F) << 8) | timing[7]) << 1;
	*width = hactive;
	*height = vactive;
	CAM_INFO(CAM_SENSOR,
		"LT6911UXC timing: total %ux%u, active %ux%u, stable %d",
		htotal, vtotal, hactive, vactive, *signal_stable);
	CAM_INFO(CAM_SENSOR, "signal stable %d %d, x %d id %d",
			*signal_stable, *width, *height, *id);

disable_i2c:
	cleanup_rc = lt6911_disable_i2c(s_ctrl);
	if (!rc && cleanup_rc < 0)
		rc = cleanup_rc;

	mutex_unlock(&(s_ctrl->cam_sensor_mutex));
	return rc;

fail:
	*signal_stable = false;
	*height = -1;
	*width = -1;
	*id = s_ctrl->id;
	goto disable_i2c;
}

int cam_hdmi_bdg_get_src_resolution(bool *signal_stable,
	int *width, int *height, int *id)
{
	struct cam_sensor_ctrl_t *s_ctrl = cam_hdmi_bdg_cam_ctrl;

	if (!signal_stable || !width || !height || !id)
		return -EINVAL;

	if (!s_ctrl) {
		CAM_ERR(CAM_SENSOR, "LT6911 is not ready.");
		*signal_stable = false;
		*height = -1;
		*width = -1;
		*id = -1;
		return -EINVAL;
	}

	if (lt6911_reg_settings.is_uxc)
		return lt6911uxc_get_src_resolution(s_ctrl, signal_stable,
				width, height, id);

	return lt6911gxc_get_src_resolution(s_ctrl, signal_stable,
			width, height, id);
}
EXPORT_SYMBOL(cam_hdmi_bdg_get_src_resolution);
EXPORT_SYMBOL(cam_hdmi_bdg_get_fw_version);
