// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>
#include "ais_ife_csid_core.h"
#include "ais_ife_csid_core_ver2.h"
#include "ais_ife_csid_dev.h"
#include "ais_ife_csid_hw_intf.h"
#include "cam_debug_util.h"
#include "cam_compat.h"

static struct cam_hw_intf *ais_ife_csid_hw_list[AIS_IFE_CSID_HW_RES_MAX] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static char csid_dev_name[8];

static int ais_ife_csid_dev_component_bind(struct device *dev,
	struct device *master_dev, void *data)
{

	struct cam_hw_intf                  *csid_hw_intf;
	struct cam_hw_info                  *csid_hw_info;
	const struct of_device_id           *match_dev = NULL;
	struct ais_ife_csid_hw_data         *csid_hw_data = NULL;
	struct ais_ife_csid_hw              *csid_dev = NULL;
	struct ais_ife_csid_ver2_hw         *csid_ver2_dev = NULL;
	struct ais_ife_csid_hw_info         *csid_hw = NULL;
	struct ais_ife_csid_ver2_hw_info    *csid_ver2_hw = NULL;
	uint32_t                            csid_dev_idx;
	int                                 rc = 0;
	struct platform_device              *pdev = to_platform_device(dev);

	CAM_DBG(CAM_ISP, "CSID component bind called");

	csid_hw_intf = kzalloc(sizeof(*csid_hw_intf), GFP_KERNEL);
	if (!csid_hw_intf) {
		rc = -ENOMEM;
		goto err;
	}

	csid_hw_info = kzalloc(sizeof(struct cam_hw_info), GFP_KERNEL);
	if (!csid_hw_info) {
		rc = -ENOMEM;
		goto free_hw_intf;
	}

	/* get ife csid hw index */
	of_property_read_u32(pdev->dev.of_node, "cell-index", &csid_dev_idx);
	/* get ife csid hw information */
	match_dev = of_match_device(pdev->dev.driver->of_match_table,
		&pdev->dev);
	if (!match_dev) {
		CAM_ERR(CAM_ISP, "No matching table for the IFE CSID HW!");
		rc = -EINVAL;
		goto free_hw_info;
	}

	CAM_DBG(CAM_ISP, "CSID%d component bind called", csid_dev_idx);

	memset(csid_dev_name, 0, sizeof(csid_dev_name));
	snprintf(csid_dev_name, sizeof(csid_dev_name),
		"csid%1u", csid_dev_idx);

	csid_hw_intf->hw_idx = csid_dev_idx;
	csid_hw_intf->hw_type = AIS_ISP_HW_TYPE_IFE_CSID;
	csid_hw_intf->hw_priv = csid_hw_info;

	csid_hw_info->soc_info.pdev = pdev;
	csid_hw_info->soc_info.dev = &pdev->dev;
	csid_hw_info->soc_info.dev_name = csid_dev_name;
	csid_hw_info->soc_info.index = csid_dev_idx;

	csid_hw_data = (struct ais_ife_csid_hw_data  *)match_dev->data;

	csid_hw_intf->hw_version = csid_hw_data->hw_dts_version;
	if (csid_hw_data->hw_dts_version == AIS_CSID_VERSION_V200) {
		csid_ver2_dev = kzalloc(sizeof(struct ais_ife_csid_ver2_hw), GFP_KERNEL);
		if (!csid_ver2_dev) {
			rc = -ENOMEM;
			goto free_hw_info;
		}
		csid_ver2_hw = (struct ais_ife_csid_ver2_hw_info  *)match_dev->data;

		csid_ver2_dev->hw_version = csid_ver2_hw->hw_dts_version;

		/* need to setup the pdev before call the ife hw probe init */
		csid_ver2_dev->csid_info = csid_ver2_hw;

		csid_hw_info->core_info = csid_ver2_dev;

		rc = ais_ife_csid_ver2_hw_probe_init(csid_hw_intf, csid_dev_idx);
		if (rc)
			goto free_dev;

		platform_set_drvdata(pdev, csid_ver2_dev);
	} else {
		csid_dev = kzalloc(sizeof(struct ais_ife_csid_hw), GFP_KERNEL);
		if (!csid_dev) {
			rc = -ENOMEM;
			goto free_hw_info;
		}

		csid_hw = (struct ais_ife_csid_hw_info  *)match_dev->data;

		csid_dev->hw_version = csid_hw->hw_dts_version;

		/* need to setup the pdev before call the ife hw probe init */
		csid_dev->csid_info = csid_hw;

		csid_hw_info->core_info = csid_dev;

		rc = ais_ife_csid_hw_probe_init(csid_hw_intf, csid_dev_idx);
		if (rc)
			goto free_dev;

		platform_set_drvdata(pdev, csid_dev);
	}
	CAM_DBG(CAM_ISP, "CSID:%d component bound successful",
		csid_hw_intf->hw_idx);

	if (csid_hw_intf->hw_idx < AIS_IFE_CSID_HW_RES_MAX)
		ais_ife_csid_hw_list[csid_hw_intf->hw_idx] = csid_hw_intf;
	else
		goto free_dev;

	return 0;

free_dev:
	if (csid_dev)
		kfree(csid_dev);

	if (csid_ver2_dev)
		kfree(csid_ver2_dev);

free_hw_info:
	kfree(csid_hw_info);
free_hw_intf:
	kfree(csid_hw_intf);
err:
	return rc;
}

static void ais_ife_csid_dev_component_unbind(struct device *dev,
	struct device *master_dev, void *data)
{
	struct ais_ife_csid_hw         *csid_dev = NULL;
	struct ais_ife_csid_ver2_hw    *csid_ver2_dev = NULL;
	struct cam_hw_intf             *csid_hw_intf;
	struct cam_hw_info             *csid_hw_info;
	struct platform_device *pdev = to_platform_device(dev);
	uint32_t                       *hw_version = NULL;


	hw_version = (uint32_t*)platform_get_drvdata(pdev);

	if (*hw_version == AIS_CSID_VERSION_V200) {
		csid_ver2_dev = (struct ais_ife_csid_ver2_hw *)platform_get_drvdata(pdev);
		csid_hw_intf = csid_ver2_dev->hw_intf;
		csid_hw_info = csid_ver2_dev->hw_info;
		ais_ife_csid_ver2_hw_deinit(csid_ver2_dev);

		CAM_DBG(CAM_ISP, "CSID VER2:%d remove",
			csid_ver2_dev->hw_intf->hw_idx);

		/*release the csid device memory */
		kfree(csid_ver2_dev);
	} else {
		csid_dev = (struct ais_ife_csid_hw *)platform_get_drvdata(pdev);
		csid_hw_intf = csid_dev->hw_intf;
		csid_hw_info = csid_dev->hw_info;
		ais_ife_csid_hw_deinit(csid_dev);

		CAM_DBG(CAM_ISP, "CSID:%d remove",
			csid_dev->hw_intf->hw_idx);

		/*release the csid device memory */
		kfree(csid_dev);
	}

	/*release the csid device memory */
	kfree(csid_hw_info);
	kfree(csid_hw_intf);
}

int ais_ife_csid_hw_init(struct cam_hw_intf **ife_csid_hw,
		struct ais_isp_hw_init_args *init)
{
	int rc = 0;

	if (ais_ife_csid_hw_list[init->hw_idx]) {
		struct cam_hw_info *csid_hw_info = NULL;
		struct ais_ife_csid_hw *core_info = NULL;
		struct ais_ife_csid_ver2_hw    *core_info_ver2 = NULL;

		*ife_csid_hw = ais_ife_csid_hw_list[init->hw_idx];
		csid_hw_info = ais_ife_csid_hw_list[init->hw_idx]->hw_priv;
		if ((*ife_csid_hw)->hw_version == AIS_CSID_VERSION_V200) {
			core_info_ver2 = (struct ais_ife_csid_ver2_hw *)csid_hw_info->core_info;

			core_info_ver2->event_cb = init->event_cb;
			core_info_ver2->event_cb_priv = init->event_cb_priv;
		} else {
			core_info = (struct ais_ife_csid_hw *)csid_hw_info->core_info;

			core_info->event_cb = init->event_cb;
			core_info->event_cb_priv = init->event_cb_priv;
		}
	} else {
		*ife_csid_hw = NULL;
		rc = -1;
	}

	return rc;
}

const static struct component_ops ais_ife_csid_dev_component_ops = {
	.bind = ais_ife_csid_dev_component_bind,
	.unbind = ais_ife_csid_dev_component_unbind,
};

int ais_ife_csid_probe(struct platform_device *pdev)
{
	int rc = 0;

	CAM_DBG(CAM_ISP, "Adding CSID dev component");

	rc = component_add(&pdev->dev, &ais_ife_csid_dev_component_ops);
	if (rc)
		CAM_ERR(CAM_ISP, "failed to add component rc: %d", rc);

	return rc;
}

#if KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE
int ais_ife_csid_remove(struct platform_device *pdev)
#else
void ais_ife_csid_remove(struct platform_device *pdev)
#endif
{
	component_del(&pdev->dev, &ais_ife_csid_dev_component_ops);

#if KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE
	return 0;
#endif
}
