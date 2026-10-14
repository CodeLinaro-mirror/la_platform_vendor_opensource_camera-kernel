/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _MSM_HDMIDP_IN_EXTCON_H
#define _MSM_HDMIDP_IN_EXTCON_H

/**
 * enum msm_in_type - input port type
 * @MSM_IN_HDMI: HDMI receive port (used by lt6911 driver)
 * @MSM_IN_DP:   DisplayPort receive port (used by lt7911 driver)
 */
enum msm_in_type {
	MSM_IN_HDMI = 0,
	MSM_IN_DP   = 1,
};

/**
 * msm_hdmidp_in_notify() - report HDMI-in / DP-in connection state change
 * @type:      port type (MSM_IN_HDMI or MSM_IN_DP)
 * @connected: true = source connected, false = source disconnected
 *
 * Internally calls extcon_get_state() to suppress duplicate events, then
 * calls extcon_set_state_sync() to update the state and trigger a UEvent.
 *
 * The UEvent emitted by the extcon framework:
 *   SUBSYSTEM=extcon
 *   NAME=msm-intl-disp        <- distinguishes IN from OUT (NAME=msm-ext-disp)
 *   STATE=HDMI=1 DP=0         <- example: HDMI-in connected
 *
 * Return: 0 on success, -EEXIST if state unchanged, negative on error.
 *
 * Usage in lt6911 driver:
 *   msm_hdmidp_in_notify(MSM_IN_HDMI, true);   // HDMI source connected
 *   msm_hdmidp_in_notify(MSM_IN_HDMI, false);  // HDMI source disconnected
 *
 * Usage in lt7911 driver:
 *   msm_hdmidp_in_notify(MSM_IN_DP, true);
 *   msm_hdmidp_in_notify(MSM_IN_DP, false);
 */
int msm_hdmidp_in_notify(enum msm_in_type type, bool connected);

/**
 * @brief : API to register exton driver
 * @return struct platform_device pointer on on success, or ERR_PTR() on error.
 */
int msm_hdmidp_in_init(void);
/**
 * @brief : API to remove exton driver
 */
void msm_hdmidp_in_exit(void);
#endif /* _MSM_HDMIDP_IN_EXTCON_H */
