/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#ifndef _CAMHDMIBDGUXECORE_H_
#define _CAMHDMIBDGUXECORE_H_

#include <linux/firmware.h>
#include "cam_sensor_dev.h"

#define HDMI_BDG_UXE_SENSOR_ID 0x2102
#define HDMI_BDG_UXE_HDMI_CONNECTED    0x55
#define HDMI_BDG_UXE_HDMI_DISCONNECTED 0x88

/**
 * This API upgrade lt6911 firmware.
 */
int cam_hdmi_bdg_uxe_upgrade_firmware(void);

/**
 * @s_ctrl: Sensor ctrl structure
 *
 * This API set lt6911 camera struct for irq handle.
 */
int cam_hdmi_bdg_uxe_set_cam_ctrl(struct cam_sensor_ctrl_t *s_ctrl);

/**
 * This API unset lt6911 camera struct for irq handle.
 */
void cam_hdmi_bdg_uxe_unset_cam_ctrl(void);

int cam_hdmi_bdg_uxe_get_src_resolution(bool *signal_stable, int *width,
				    int *height, int *id);

uint32_t cam_hdmi_bdg_uxe_get_fw_version(void);
int lt6911uxe_get_fw_state(void);

#endif /* _CAMHDMIBDGUXECORE_H_ */
