/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef _CAMHDMIBDGCORE_H_
#define _CAMHDMIBDGCORE_H_

#include <linux/firmware.h>
#include "cam_sensor_dev.h"

#define HDMI_GXC_SENSOR_ID 0x1901
#define HDMI_UXC_SENSOR_ID 0x1704
#define HDMI_BDG_HDMI_CONNECTED    0x01
#define HDMI_BDG_HDMI_DISCONNECTED 0x00
#define HDMI_GXC_SENSOR_NAME "lt6911gxc"
#define HDMI_UXC_SENSOR_NAME "lt6911uxc"
#define HDMI_UXC_SENSOR_SLAVE_ADDR 0x56

struct cam_sensor_i2c_reg_array;

struct lt6911_reg_settings {
	struct cam_sensor_i2c_reg_array *write_en_regs;
	int write_en_size;
	struct cam_sensor_i2c_reg_array *write_config_regs;
	int write_config_size;
	struct cam_sensor_i2c_reg_array *write_addr_set_regs;
	int write_addr_set_size;
	struct cam_sensor_i2c_reg_array *write_over_regs;
	int write_over_size;
	struct cam_sensor_i2c_reg_array *block_erase_regs;
	int block_erase_size;
	uint32_t block_erase_delay;
	struct cam_sensor_i2c_reg_array *gxc_block_erase_ext_regs;
	int gxc_block_erase_ext_size;
	struct cam_sensor_i2c_reg_array *config_regs;
	int config_size;
	struct cam_sensor_i2c_reg_array *read_addr_regs;
	int read_addr_size;
	struct cam_sensor_i2c_reg_array *get_fw_regs;
	int get_fw_size;
	int erase_time;
};

enum lt6911_fw_status {
	UPDATE_SUCCESS = 0,
	UPDATE_RUNNING = 1,
	UPDATE_FAILED = 2,
};

/**
 * This API upgrade lt6911 firmware.
 */
int cam_hdmi_bdg_upgrade_firmware(void);

/**
 * @s_ctrl: Sensor ctrl structure
 *
 * This API set lt6911 camera struct for irq handle.
 */
int cam_hdmi_bdg_set_cam_ctrl(struct cam_sensor_ctrl_t *s_ctrl);

/**
 * This API unset lt6911 camera struct for irq handle.
 */
void cam_hdmi_bdg_unset_cam_ctrl(void);

int cam_hdmi_bdg_get_src_resolution(bool *signal_stable,
		int *width, int *height, int *id);

uint32_t cam_hdmi_bdg_get_fw_version(void);

/**
 * @brief : API to register HMDI dev to platform framework.
 * @return struct platform_device pointer on on success, or ERR_PTR() on error.
 */
int hdmi_bdg_irq_handler_init(void);

/**
 * @brief : API to remove HMDI dev from platform framework.
 */
void hdmi_bdg_irq_handler_exit(void);

#endif /* _CAMHDMIBDGCORE_H_ */
