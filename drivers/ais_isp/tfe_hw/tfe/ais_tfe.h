/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _AIS_TFE_H_
#define _AIS_TFE_H_

/**
 * @brief : API to register TFE HW to platform framework.
 * @return struct platform_device pointer on success, or ERR_PTR() on error.
 */
int ais_tfe_init_module(void);

/**
 * @brief : API to remove TFE HW from platform framework.
 */
void ais_tfe_exit_module(void);

#endif /* _AIS_TFE_H_ */
