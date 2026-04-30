// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */


#include <linux/module.h>
#include "ais_tfe_csid_core.h"
#include "ais_tfe_csid665.h"
#include "ais_tfe_csid_dev.h"

#define AIS_TFE_CSID_DRV_NAME                    "ais-tfe-csid"
#define AIS_TFE_CSID_VERSION_V665                 0x70065000

static struct ais_ife_csid_hw_info ais_tfe_csid665_hw_info = {
	.csid_reg = &ais_tfe_csid_665_reg_offset,
	.hw_dts_version = AIS_TFE_CSID_VERSION_V665,
};

static const struct of_device_id ais_tfe_csid_dt_match[] = {
	{
		.compatible = "qcom,ais-csid665",
		.data = &ais_tfe_csid665_hw_info,
	},
	{}
};

MODULE_DEVICE_TABLE(of, ais_tfe_csid_dt_match);

struct platform_driver ais_tfe_csid_driver = {
	.probe = ais_tfe_csid_probe,
	.remove = ais_tfe_csid_remove,
	.driver = {
		.name = AIS_TFE_CSID_DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ais_tfe_csid_dt_match,
		.suppress_bind_attrs = true,
	},
};

int ais_tfe_csid_init_module(void)
{
	return platform_driver_register(&ais_tfe_csid_driver);
}

void ais_tfe_csid_exit_module(void)
{
	platform_driver_unregister(&ais_tfe_csid_driver);
}

MODULE_DESCRIPTION("AIS TFE CSID driver");
MODULE_LICENSE("GPL v2");
