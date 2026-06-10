/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef _CAMDPBDGLT7911DCORE_H_
#define _CAMDPBDGLT7911DCORE_H_

#include <linux/firmware.h>
#include "cam_sensor_dev.h"

#define DP_BDG_LT7911D_ID 0x1605
#define DP_BDG_LT7911D_CONNECTED    0x01
#define DP_BDG_LT7911D_DISCONNECTED 0x00
#define DP_SENSOR_LT7911D_NAME "lt7911d"

enum lt7911d_fw_status {
	LT7911D_UPDATE_SUCCESS = 0,
	LT7911D_UPDATE_RUNNING = 1,
	LT7911D_UPDATE_FAILED  = 2,
};

/**
 * This API upgrade lt7911 firmware.
 */
int cam_dp_bdg_lt7911d_upgrade_firmware(void);

/**
 * @s_ctrl: Sensor ctrl structure
 *
 * This API set lt7911 camera struct for irq handle.
 */
int cam_dp_bdg_lt7911d_set_cam_ctrl(struct cam_sensor_ctrl_t *s_ctrl);

/**
 * This API unset lt7911 camera struct for irq handle.
 */
void cam_dp_bdg_lt7911d_unset_cam_ctrl(void);

int cam_dp_bdg_lt7911d_get_src_resolution(bool *signal_stable,
		int *width, int *height, int *id);

uint32_t cam_dp_bdg_lt7911d_get_fw_version(void);

/**
 * @brief : API to register DP dev to platform framework.
 * @return struct platform_device pointer on on success, or ERR_PTR() on error.
 */
int dp_bdg_lt7911d_irq_handler_init(void);

/**
 * @brief : API to remove DP dev from platform framework.
 */
void dp_bdg_lt7911d_irq_handler_exit(void);

/**
 * @brief : API to reset lt7911d chip.
 */
void lt7911d_chip_reset(void);

#endif /* _CAMDPBDGLT7911DCORE_H_ */
