/* Copyright (c) 2017-2018, 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _AIS_TFE_DEV_H_
#define _AIS_TFE_DEV_H_

#include <linux/platform_device.h>
#include <linux/version.h>

/*
 * ais_tfe_probe()
 *
 * @brief:                   Driver probe function called on Boot
 *
 * @pdev:                    Platform Device pointer
 *
 * @Return:                  0: Success
 *                           Non-zero: Failure
 */
int ais_tfe_probe(struct platform_device *pdev);

/*
 * ais_tfe_remove()
 *
 * @brief:                   Driver remove function
 *
 * @pdev:                    Platform Device pointer
 *
 * @Return:                  0: Success
 *                           Non-zero: Failure
 */
#if KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE
int ais_tfe_remove(struct platform_device *pdev);
#else
void ais_tfe_remove(struct platform_device *pdev);
#endif

#endif /* _AIS_TFE_DEV_H_ */
