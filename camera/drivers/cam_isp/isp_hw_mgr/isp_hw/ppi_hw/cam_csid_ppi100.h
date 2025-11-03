/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _CAM_CSID_PPI_100_H_
#define _CAM_CSID_PPI_100_H_

#include "cam_csid_ppi_core.h"

/**
 * @brief : API to register PPI Dev to platform framework.
 * @return struct platform_device pointer on on success, or ERR_PTR() on error.
 */
int cam_csid_ppi100_init_module(void);

/**
 * @brief : API to remove PPI Dev from platform framework.
 */
void cam_csid_ppi100_exit_module(void);
#endif /*_CAM_CSID_PPI_100_H_ */
