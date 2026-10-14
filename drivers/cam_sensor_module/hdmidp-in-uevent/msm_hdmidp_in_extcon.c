/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/extcon-provider.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/spinlock.h>

#include "msm_hdmidp_in_extcon.h"

/*
 * Same cable table as msm_ext_display.c.
 * IN/OUT direction is distinguished by the extcon device name (NAME field),
 * not by the cable ID.
 */
static const unsigned int msm_in_supported_cable[] = {
	EXTCON_DISP_DP,     /* cable[0]: DP   -> STATE=DP=1   */
	EXTCON_DISP_HDMI,   /* cable[1]: HDMI -> STATE=HDMI=1 */
	EXTCON_NONE,
};

static struct extcon_dev *g_edev;
static DEFINE_SPINLOCK(g_edev_lock);

/* ======================================================================
 * External API: called by lt6911 (HDMI-in) and lt7911 (DP-in) drivers
 *
 * UEvent emitted by the kernel extcon framework:
 *   SUBSYSTEM=extcon
 *   NAME=msm-intl-disp        <- differs from OUT side NAME=msm-ext-disp
 *   STATE=HDMI=1 DP=0         <- example: HDMI-in connected
 *
 * WiredAccessoryManager uses the NAME field to distinguish IN from OUT.
 * ====================================================================== */
int msm_hdmidp_in_notify(enum msm_in_type type, bool connected)
{
	unsigned int cable_id;
	int cur_state, ret;
	struct extcon_dev *edev;

	spin_lock_bh(&g_edev_lock);
	edev = g_edev;
	spin_unlock_bh(&g_edev_lock);
	if (!edev) {
		pr_err("driver not probed yet\n");
		return -ENODEV;
	}

	cable_id = (type == MSM_IN_HDMI) ? EXTCON_DISP_HDMI : EXTCON_DISP_DP;

	/* Skip if state has not changed (mirrors msm_ext_disp_process_audio) */
	cur_state = extcon_get_state(edev, cable_id);
	if (cur_state == (int)connected) {
		pr_debug("same state (%d), skip\n", cur_state);
		return -EEXIST;
	}

	ret = extcon_set_state_sync(edev, cable_id, connected);
	if (ret)
		pr_err("extcon_set_state_sync failed: %d\n", ret);
	else
		pr_info("cable_id=%u connected=%d (NAME=msm-intl-disp)\n",
			cable_id, connected);

	return ret;
}
EXPORT_SYMBOL(msm_hdmidp_in_notify);

/* ======================================================================
 * Sysfs debug node: simulate_connect
 *
 * Usage:
 *   echo "0,1" > /sys/devices/platform/soc/<dev>/simulate_connect
 *
 * Format: "type,state"
 *   type  : 0 = DP-in (EXTCON_DISP_DP), 1 = HDMI-in (EXTCON_DISP_HDMI)
 *   state : 0 = disconnect, 1 = connect
 * ====================================================================== */
static ssize_t simulate_connect_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	int type_val, state_val;
	enum msm_in_type type;
	bool connected;
	int ret;

	/* Parse "type,state" */
	if (sscanf(buf, "%d,%d", &type_val, &state_val) != 2) {
		dev_err(dev, "invalid format, use: echo \"type,state\" (e.g. \"0,1\")\n");
		return -EINVAL;
	}

	if (type_val != 0 && type_val != 1) {
		dev_err(dev, "invalid type %d, must be 0 (DP-in) or 1 (HDMI-in)\n", type_val);
		return -EINVAL;
	}
	if (state_val != 0 && state_val != 1) {
		dev_err(dev, "invalid state %d, must be 0 (disconnect) or 1 (connect)\n", state_val);
		return -EINVAL;
	}

	type      = (type_val == 0) ? MSM_IN_DP : MSM_IN_HDMI;
	connected = (state_val == 1);

	dev_info(dev, "simulate: %s %s\n",
		 type == MSM_IN_HDMI ? "HDMI-in" : "DP-in",
		 connected ? "connect" : "disconnect");

	ret = msm_hdmidp_in_notify(type, connected);
	if (ret && ret != -EEXIST)
		return ret;

	return count;
}
static DEVICE_ATTR_WO(simulate_connect);

static struct attribute *msm_hdmidp_in_attrs[] = {
	&dev_attr_simulate_connect.attr,
	NULL,
};

static const struct attribute_group msm_hdmidp_in_attr_group = {
	.attrs = msm_hdmidp_in_attrs,
};

/* ======================================================================
 * probe / remove
 * ====================================================================== */
static int msm_hdmidp_in_probe(struct platform_device *pdev)
{
	struct extcon_dev *edev;
	int ret;

	if (!pdev || !pdev->dev.of_node) {
		pr_err("No platform device or device node\n");
		return -ENODEV;
	}

	/*
	 * Allocate and register the extcon device.
	 * edev->name defaults to the parent device name ("msm-intl-disp"),
	 * which becomes the NAME= field in the UEvent and allows
	 * WiredAccessoryManager to distinguish this device from the OUT side.
	 *
	 * Initial state: all cables disconnected (extcon default).
	 */
	edev = devm_extcon_dev_allocate(&pdev->dev,
				        msm_in_supported_cable);
	if (IS_ERR(edev)) {
		pr_err("extcon alloc failed\n");
		return PTR_ERR(edev);
	}

	ret = devm_extcon_dev_register(&pdev->dev, edev);
	if (ret) {
		pr_err("extcon register failed: %d\n", ret);
		return ret;
	}

	/* Explicitly set initial state to disconnected */
	extcon_set_state_sync(edev, EXTCON_DISP_HDMI, false);
	extcon_set_state_sync(edev, EXTCON_DISP_DP,   false);

	/* Create sysfs debug node */
	ret = sysfs_create_group(&pdev->dev.kobj, &msm_hdmidp_in_attr_group);
	if (ret)
		pr_warn("failed to create sysfs group: %d (non-fatal)\n", ret);

	spin_lock_bh(&g_edev_lock);
	g_edev = edev;
	spin_unlock_bh(&g_edev_lock);

	pr_info("probe done (extcon NAME=msm-intl-disp, HDMI-in=disconnected, DP-in=disconnected)\n");
	return 0;
}

#if (KERNEL_VERSION(6, 10, 0) <= LINUX_VERSION_CODE)
static void msm_hdmidp_in_remove(struct platform_device *pdev)
#else
static int msm_hdmidp_in_remove(struct platform_device *pdev)
#endif
{
	struct extcon_dev *edev;

	spin_lock_bh(&g_edev_lock);
	edev = g_edev;
	spin_unlock_bh(&g_edev_lock);

	if (edev) {
		/* Report disconnected before unloading to avoid stale state */
		if (extcon_get_state(edev, EXTCON_DISP_HDMI) == 1)
			extcon_set_state_sync(edev, EXTCON_DISP_HDMI, false);
		if (extcon_get_state(edev, EXTCON_DISP_DP) == 1)
			extcon_set_state_sync(edev, EXTCON_DISP_DP, false);
	}

	sysfs_remove_group(&pdev->dev.kobj, &msm_hdmidp_in_attr_group);
	spin_lock_bh(&g_edev_lock);
	g_edev = NULL;
	spin_unlock_bh(&g_edev_lock);
	pr_info("removed\n");

#if (KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE)
	return 0;
#endif
}

static const struct of_device_id msm_hdmidp_in_dt_match[] = {
	{ .compatible = "qcom,msm-intl-disp" },
	{ /* Sentinel */ },
};
MODULE_DEVICE_TABLE(of, msm_hdmidp_in_dt_match);

static struct platform_driver msm_hdmidp_in_driver = {
	.probe  = msm_hdmidp_in_probe,
	.remove = msm_hdmidp_in_remove,
	.driver = {
		.name = "msm-intl-disp",   /* becomes extcon NAME= field */
		.of_match_table = msm_hdmidp_in_dt_match,
	},
};

int msm_hdmidp_in_init(void)
{
	int ret;

	ret = platform_driver_register(&msm_hdmidp_in_driver);
	if (ret)
		pr_err("platform_driver_register failed: %d\n", ret);
	return ret;
}

void msm_hdmidp_in_exit(void)
{
	platform_driver_unregister(&msm_hdmidp_in_driver);
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM HDMI-in / DP-in extcon driver");
