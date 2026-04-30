// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/module.h>
#include "ais_tfe665.h"
#include "ais_vfe_hw_intf.h"
#include "ais_tfe_core.h"
#include "ais_tfe_dev.h"

static const struct of_device_id ais_tfe_dt_match[] = {
	{
		.compatible = "qcom,ais-tfe665",
		.data = &ais_tfe665_hw_info,
	},
	{}
};
MODULE_DEVICE_TABLE(of, ais_tfe_dt_match);

struct platform_driver ais_tfe_driver = {
	.probe = ais_tfe_probe,
	.remove = ais_tfe_remove,
	.driver = {
		.name = "ais_tfe",
		.owner = THIS_MODULE,
		.of_match_table = ais_tfe_dt_match,
		.suppress_bind_attrs = true,
	},
};

int ais_tfe_init_module(void)
{
	return platform_driver_register(&ais_tfe_driver);
}

void ais_tfe_exit_module(void)
{
	platform_driver_unregister(&ais_tfe_driver);
}

MODULE_DESCRIPTION("AIS TFE driver");
MODULE_LICENSE("GPL v2");
