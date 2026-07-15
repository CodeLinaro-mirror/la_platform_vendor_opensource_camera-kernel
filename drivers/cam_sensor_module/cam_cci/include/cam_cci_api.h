/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef _CAM_CCI_API_H_
#define _CAM_CCI_API_H_

#include <media/v4l2-subdev.h>
#include "cam_cci_dev.h"

/**
 * struct cam_cci_client_iface - ops table attached to a CCI v4l2_subdev.
 *
 * Stored in sd hostdata by cam_cci_component_bind() and never modified
 * afterwards.  External drivers retrieve it as:
 *
 *   struct cam_cci_client_iface *iface = v4l2_get_subdev_hostdata(sd);
 *
 * @cci_ops:    Dispatch a cam_cci_ctrl command to the CCI sub-device.
 *              Equivalent to v4l2_subdev_call(sd, core, ioctl,
 *              VIDIOC_MSM_CCI_CFG, cci_ctrl) but without the V4L2 ioctl
 *              dispatch overhead.  Maps to cam_cci_client_ops().
 */
struct cam_cci_client_iface {
	int32_t (*cci_ops)(struct v4l2_subdev *sd, unsigned int cmd,
			   struct cam_cci_ctrl *cci_ctrl);
};

/**
 * cam_cci_client_ops()
 *
 * @brief:      Direct kernel-internal API for CCI operations.
 *              Calls cam_cci_core_cfg() directly, bypassing the ioctl
 *              dispatch path and video_usercopy entirely.
 *
 * @sd:         V4L2 sub-device pointer for the CCI subdev
 * @cmd:        CCI command identifier
 * @cci_ctrl:   Pointer to a kernel-owned cam_cci_ctrl struct
 *
 * WARNING:    The returned iface pointer from v4l2_get_subdev_hostdata()
 *             is only valid while the CCI module is loaded. External
 *             drivers must not cache the iface pointer across module
 *             unload/reload cycles. Always retrieve it fresh via
 *             v4l2_get_subdev_hostdata() before each use.
 */

int32_t cam_cci_client_ops(struct v4l2_subdev *sd, unsigned int cmd,
	struct cam_cci_ctrl *cci_ctrl);

#endif /* _CAM_CCI_API_H_ */
