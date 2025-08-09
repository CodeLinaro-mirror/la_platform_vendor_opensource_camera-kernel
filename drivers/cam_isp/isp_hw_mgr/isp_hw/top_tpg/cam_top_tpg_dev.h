/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _CAM_TOP_TPG_DEV_H_
#define _CAM_TOP_TPG_DEV_H_

int cam_top_tpg_probe(struct platform_device *pdev);

#if KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE
int cam_top_tpg_remove(struct platform_device *pdev);
#else
void cam_top_tpg_remove(struct platform_device *pdev);
#endif

#endif /*_CAM_TOP_TPG_DEV_H_ */
