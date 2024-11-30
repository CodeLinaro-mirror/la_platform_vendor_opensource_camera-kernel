/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights
 * reserved.
 */
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include "cam_hdmi_bdg_uxe_dev.h"
#include "cam_hdmi_bdg_uxe_core.h"

#define HDMI_BDG_IRQ_HANDLER_DEVNAME             "hdmi_bdg_uxe_irq_handler"
#define HDMI_BDG_IRQ_HANDLER_MAGIC_NUM           0xee
#define HDMI_BDG_IRQ_HANDLER_IOCTL_GETRES        0x01
#define HDMI_BDG_IRQ_HANDLER_IOCTL_GETRES_DIR    0x02
#define HDMI_BDG_IRQ_HANDLER_IOCTL_RST           0x03
#define HDMI_BDG_IRQ_HANDLER_IOCTL_UPGRADE_FW    0x04

struct hdmi_bdg_res_info {
	int32_t width;
	int32_t height;
	bool have_hdmi_signal;
	int32_t id;
};

#define HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_GETRES                                  \
	_IOR(HDMI_BDG_IRQ_HANDLER_MAGIC_NUM,                                   \
	     HDMI_BDG_IRQ_HANDLER_IOCTL_GETRES, struct hdmi_bdg_res_info)
#define HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_GETRES_DIR                              \
	_IOR(HDMI_BDG_IRQ_HANDLER_MAGIC_NUM,                                   \
	     HDMI_BDG_IRQ_HANDLER_IOCTL_GETRES_DIR, struct hdmi_bdg_res_info)
#define HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_RST                                     \
	_IO(HDMI_BDG_IRQ_HANDLER_MAGIC_NUM, HDMI_BDG_IRQ_HANDLER_IOCTL_RST)
#define HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_UPGRADE_FW                              \
	_IO(HDMI_BDG_IRQ_HANDLER_MAGIC_NUM,                                    \
	    HDMI_BDG_IRQ_HANDLER_IOCTL_UPGRADE_FW)

static const struct of_device_id hdmi_bdg_uxe_irq_handler_dev_id[] = {
	{ .compatible = "hdmi_bdg_uxe_irq_handler" },
	{},
};

static bool is_irq_happen;
static int hdmi_bdg_uxe_irq;
static int hdmi_bdg_uxe_irq_gpio;
static wait_queue_head_t hdmi_bdg_uxe_read_wq;
static struct hdmi_bdg_res_info s_hdmi_bdg_res_info;
static struct cdev *hdmi_bdg_uxe_irq_handler_cdev;
static dev_t hdmi_bdg_uxe_irq_handler_dev;
static struct class *hdmi_bdg_uxe_irq_handler_class;
static struct device *hdmi_bdg_uxe_irq_handler_device;
static uint32_t img_fwrev;
static uint32_t reset_gpio;

MODULE_DEVICE_TABLE(of, hdmi_bdg_uxe_irq_handler_dev_id);

void workqueue_fw_upgrade(struct work_struct *work);
DECLARE_DELAYED_WORK(workqueue, workqueue_fw_upgrade);

static void lt6911uxe_reset(bool on_off)
{
	CAM_INFO(CAM_SENSOR, "lt6911uxe_reset reset: %d", on_off);
	if (on_off) {
		gpio_set_value(reset_gpio, 1);
		msleep(20);
		gpio_set_value(reset_gpio, 0);
		msleep(100);
		gpio_set_value(reset_gpio, 1);
		/* Need longer time to make sure LT8711UXD initialized. */
		msleep(150);
	} else {
		gpio_set_value(reset_gpio, 0);
	}
}

void workqueue_fw_upgrade(struct work_struct *work)
{
	uint32_t revision = 0;
	CAM_INFO(CAM_SENSOR, "LT6911UXE workqueue_fw_upgrade");

	revision = cam_hdmi_bdg_uxe_get_fw_version();
	if (revision && img_fwrev <= revision && revision != 0xc0043804) {
		CAM_INFO(CAM_SENSOR,
			"LT6911UXE works fw: 0x%x => 0x%x, , no need to upgrade FW",
			revision, img_fwrev);
	} else {
		CAM_INFO(CAM_SENSOR, "LT6911UXE update fw: 0x%x => 0x%x", revision,
			 img_fwrev);
		cam_hdmi_bdg_uxe_upgrade_firmware();
		CAM_INFO(CAM_SENSOR, "LT6911UXE update done reset");
		lt6911uxe_reset(true);
	}
}

static ssize_t get_lt6911uxe_firmware_version_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	CAM_INFO(CAM_SENSOR, "LT6911UXE firmare version: 0x%08x",
		 cam_hdmi_bdg_uxe_get_fw_version());
	return scnprintf(buf, 9, "%x\n", cam_hdmi_bdg_uxe_get_fw_version());
}

static ssize_t lt6911uxe_firmware_upgrade_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	int ret = 0;
	int get;

	pr_info("lt6911uxe_firmware_upgrade_store \n");

	sscanf(buf, "%d", &get);

	switch (get) {
	case 0:
		// Upgrade Main F/W SOP
		ret = cam_hdmi_bdg_uxe_upgrade_firmware();
		if (ret)
			pr_err("Failed to invoke cam_hdmi_bdg_uxe_upgrade_firmware\n", ret);
		else
			lt6911uxe_reset(true);
		break;
	case 1:
		lt6911uxe_reset(true);
		break;
	default:
		break;
	}

	return count;
}

static ssize_t lt6911uxe_firmware_upgrade_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", lt6911uxe_get_fw_state());
}

static DEVICE_ATTR_RO(get_lt6911uxe_firmware_version);
static DEVICE_ATTR_RW(lt6911uxe_firmware_upgrade);

static struct attribute *hdmi_bdg_uxe_sysfs_attrs[] = {
	&dev_attr_lt6911uxe_firmware_upgrade.attr,
	&dev_attr_get_lt6911uxe_firmware_version.attr,
	NULL,
};
static struct attribute_group hdmi_bdg_uxe_sysfs_attr_grp = {
	.attrs = hdmi_bdg_uxe_sysfs_attrs,
};

static int hdmi_bdg_uxe_sysfs_init(struct device *dev)
{
	int rc = 0;
	if (!dev) {
		pr_err("%s: Invalid params\n", __func__);
		return -EINVAL;
	}

	rc = sysfs_create_group(&dev->kobj, &hdmi_bdg_uxe_sysfs_attr_grp);
	if (rc)
		pr_err("%s: sysfs group creation failed %d\n", __func__, rc);

	return rc;
}

static void hdmi_bdg_uxe_sysfs_remove(struct device *dev)
{
	if (!dev) {
		pr_err("%s: Invalid params\n", __func__);
		return;
	}

	sysfs_remove_group(&dev->kobj, &hdmi_bdg_uxe_sysfs_attr_grp);
}
static int hdmi_bdg_uxe_irq_handler_dev_open(struct inode *inodp, struct file *filp)
{
	return 0;
}
static int hdmi_bdg_uxe_irq_handler_dev_release(struct inode *inodp,
					    struct file *filp)
{
	wake_up_all(&hdmi_bdg_uxe_read_wq);
	return 0;
}
static ssize_t hdmi_bdg_uxe_irq_handler_dev_read(struct file *filp,
					     char __user *buf, size_t n_read,
					     loff_t *off)
{
	CAM_INFO(CAM_SENSOR, "hdmi_bdg_uxe_irq_handler: wait IRQ happened");
	wait_event_interruptible(hdmi_bdg_uxe_read_wq, is_irq_happen);
	CAM_INFO(CAM_SENSOR, "hdmi_bdg_uxe_irq_handler: IRQ happened");
	is_irq_happen = false;
	return 0;
}

static long hdmi_bdg_uxe_irq_handler_dev_ioctl(struct file *filp, unsigned int cmd,
					   unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_GETRES:
		rc = copy_to_user((struct hdmi_bdg_res_info *)arg,
				  &s_hdmi_bdg_res_info,
				  sizeof(s_hdmi_bdg_res_info));
		if (rc != 0)
			CAM_ERR(CAM_SENSOR,
				"hdmi_bdg_uxe_irq_handler: IOCTL read error!");
		break;
	case HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_GETRES_DIR:
		CAM_INFO(CAM_SENSOR, "LT6911UXE firmare version: 0x%08x",
			 cam_hdmi_bdg_uxe_get_fw_version());
		rc = cam_hdmi_bdg_uxe_get_src_resolution(
			&s_hdmi_bdg_res_info.have_hdmi_signal,
			&s_hdmi_bdg_res_info.width, &s_hdmi_bdg_res_info.height,
			&s_hdmi_bdg_res_info.id);
		if (!rc) {
			CAM_INFO(CAM_SENSOR,
				 "HDMI_BDG Input resolution = %d x %d",
				 s_hdmi_bdg_res_info.width,
				 s_hdmi_bdg_res_info.height);
		}
		rc = copy_to_user((struct hdmi_bdg_res_info *)arg,
				  &s_hdmi_bdg_res_info,
				  sizeof(s_hdmi_bdg_res_info));
		if (rc != 0)
			CAM_ERR(CAM_SENSOR,
				"hdmi_bdg_uxe_irq_handler: IOCTL read error!");
		break;
	case HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_RST:
		s_hdmi_bdg_res_info.width = -1;
		s_hdmi_bdg_res_info.height = -1;
		s_hdmi_bdg_res_info.have_hdmi_signal = false;
		s_hdmi_bdg_res_info.id = 0;
		break;
	case HDMI_BDG_IRQ_HANDLER_IOCTL_CMD_UPGRADE_FW:
		CAM_INFO(CAM_SENSOR,
			 "hdmi_bdg_uxe_irq_handler: Upgrading firmware...");
		rc = cam_hdmi_bdg_uxe_upgrade_firmware();
		CAM_INFO(CAM_SENSOR, "hdmi_bdg_uxe_irq_handler: rc %d", rc);
		break;
	default:
		CAM_ERR(CAM_SENSOR,
			"hdmi_bdg_uxe_irq_handler: IOCTL unknown command!");
		break;
	}
	return rc;
}

#ifdef CONFIG_COMPAT
static long hdmi_bdg_uxe_irq_handler_dev_compat_ioctl(struct file *filp,
						  unsigned int cmd,
						  unsigned long arg)
{
	return hdmi_bdg_uxe_irq_handler_dev_ioctl(filp, cmd,
					      (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations hdmi_bdg_uxe_irq_handler_dev_fops = {
	.owner = THIS_MODULE,
	.read = hdmi_bdg_uxe_irq_handler_dev_read,
	.open = hdmi_bdg_uxe_irq_handler_dev_open,
	.unlocked_ioctl = hdmi_bdg_uxe_irq_handler_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = hdmi_bdg_uxe_irq_handler_dev_compat_ioctl,
#endif
	.release = hdmi_bdg_uxe_irq_handler_dev_release,
};

static irqreturn_t hdmi_bdg_uxe_irq_handler(int irq, void *p)
{
	int rc = 0;

	CAM_INFO(CAM_SENSOR, "HDMI hotplug happened");
	rc = cam_hdmi_bdg_uxe_get_src_resolution(
		&s_hdmi_bdg_res_info.have_hdmi_signal,
		&s_hdmi_bdg_res_info.width, &s_hdmi_bdg_res_info.height,
		&s_hdmi_bdg_res_info.id);
	switch (rc) {
		case LT6911UXE_VIDEO_EVENT:
			if (s_hdmi_bdg_res_info.have_hdmi_signal)
				CAM_INFO(CAM_SENSOR, "HDMI_BDG Input resolution = %d x %d",
					s_hdmi_bdg_res_info.width, s_hdmi_bdg_res_info.height);
			is_irq_happen = true;
			wake_up_all(&hdmi_bdg_uxe_read_wq);
			break;
		case LT6911UXE_AUDIO_EVENT:
			CAM_INFO(CAM_SENSOR, "Audio event, currently not handling.");
			break;
		default:
			CAM_ERR(CAM_SENSOR, "Get resolution failed!");
			break;
	}
	return IRQ_HANDLED;
}

static int hdmi_bdg_uxe_irq_handler_probe(struct platform_device *pdev)
{
	int ret = 0;

	enum of_gpio_flags flags;

	// AmTRAN, get image fw version
	if (of_property_read_u32(pdev->dev.of_node, "fwrev", &img_fwrev) < 0) {
		CAM_ERR(CAM_SENSOR, "LT6911UXE Failed reading IMG FW revision\n");
		img_fwrev = 0;
	} else {
		CAM_INFO(CAM_SENSOR, "LT6911UXE img_fw: 0x%x", img_fwrev);
	}

	//AmTRAN, Get LT6911UXE RST_N GPIO
	reset_gpio = of_get_named_gpio(pdev->dev.of_node, "lt,reset-gpio", 0);
	if (!gpio_is_valid(reset_gpio)) {
		CAM_ERR(CAM_SENSOR, "LT6911UXE reset gpio not specified");
		ret = -EINVAL;
	}
	if (gpio_request(reset_gpio, "lt6911-reset-gpio")) {
		CAM_ERR(CAM_SENSOR,"LT6911UXE reset gpio request failed");
		goto fail_exit;
	}
	if (gpio_direction_output(reset_gpio, 1)) {
		CAM_ERR(CAM_SENSOR, "LT6911UXE reset gpio direction failed");
		goto fail_exit;
	}

	gpio_set_value(reset_gpio, 1);
	CAM_INFO(CAM_SENSOR, "LT6911UXE reset gpio config done");

	hdmi_bdg_uxe_irq_gpio = of_get_named_gpio_flags(
		pdev->dev.of_node, "hdmi_bdg_irq_pin", 0, &flags);
	ret = gpio_request(hdmi_bdg_uxe_irq_gpio, "hdmi_bdg_irq_pin");
	if (ret) {
		CAM_ERR(CAM_SENSOR, "Error! can't request irq pin %x",
			hdmi_bdg_uxe_irq_gpio);
		ret = -EINVAL;
		goto fail_exit;
	}
	hdmi_bdg_uxe_irq = gpio_to_irq(hdmi_bdg_uxe_irq_gpio);
	ret = request_threaded_irq(hdmi_bdg_uxe_irq, NULL, hdmi_bdg_uxe_irq_handler,
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   "hdmi_bdg_uxe_irq", NULL);
	if (ret) {
		CAM_ERR(CAM_SENSOR, "Failed to request irq");
		ret = -EINVAL;
		goto fail_exit;
	}

	hdmi_bdg_uxe_irq_handler_cdev = cdev_alloc();
	if (!hdmi_bdg_uxe_irq_handler_cdev) {
		ret = -ENOMEM;
		goto fail_exit;
	}
	hdmi_bdg_uxe_irq_handler_cdev->ops = &hdmi_bdg_uxe_irq_handler_dev_fops;
	hdmi_bdg_uxe_irq_handler_cdev->owner = THIS_MODULE;
	ret = alloc_chrdev_region(&hdmi_bdg_uxe_irq_handler_dev, 0, 1,
				  HDMI_BDG_IRQ_HANDLER_DEVNAME);
	if (ret) {
		CAM_ERR(CAM_SENSOR,
			"alloc_chrdev_region failed with error code %d", ret);
		goto fail_cdev_del;
	}
	ret = cdev_add(hdmi_bdg_uxe_irq_handler_cdev, hdmi_bdg_uxe_irq_handler_dev, 1);
	if (ret)
		goto fail_cdev_del;
	hdmi_bdg_uxe_irq_handler_class =
		class_create(THIS_MODULE, HDMI_BDG_IRQ_HANDLER_DEVNAME);
	if (IS_ERR(hdmi_bdg_uxe_irq_handler_class)) {
		ret = PTR_ERR(hdmi_bdg_uxe_irq_handler_class);
		goto fail_cdev_del;
	}
	hdmi_bdg_uxe_irq_handler_device =
		device_create(hdmi_bdg_uxe_irq_handler_class, NULL,
			      hdmi_bdg_uxe_irq_handler_dev, NULL,
			      HDMI_BDG_IRQ_HANDLER_DEVNAME);
	if (IS_ERR(hdmi_bdg_uxe_irq_handler_device)) {
		ret = PTR_ERR(hdmi_bdg_uxe_irq_handler_device);
		goto fail_class_destroy;
	}
	ret = hdmi_bdg_uxe_sysfs_init(&pdev->dev);
	if (ret) {
		pr_err("sysfs init failed\n");
		goto fail_class_destroy;
	}
	init_waitqueue_head(&hdmi_bdg_uxe_read_wq);

	// AmTRAN, Check FW version to upgrade FW in 60 seconds
	schedule_delayed_work(&workqueue, HZ * 60);

	return 0;

fail_class_destroy:
	class_destroy(hdmi_bdg_uxe_irq_handler_class);
fail_cdev_del:
	cdev_del(hdmi_bdg_uxe_irq_handler_cdev);
fail_exit:
	gpio_free(reset_gpio);
	return ret;
}

static int hdmi_bdg_uxe_irq_handler_remove(struct platform_device *pdev)
{
	gpio_free(hdmi_bdg_uxe_irq_gpio);
	gpio_free(reset_gpio);
	disable_irq(hdmi_bdg_uxe_irq);
	wake_up_all(&hdmi_bdg_uxe_read_wq);

	cdev_del(hdmi_bdg_uxe_irq_handler_cdev);
	device_destroy(hdmi_bdg_uxe_irq_handler_class, hdmi_bdg_uxe_irq_handler_dev);
	class_destroy(hdmi_bdg_uxe_irq_handler_class);
	unregister_chrdev_region(hdmi_bdg_uxe_irq_handler_dev, 1);
	hdmi_bdg_uxe_sysfs_remove(&pdev->dev);
	return 0;
}

static struct platform_driver hdmi_bdg_uxe_irq_handler_drv = {
    .probe = hdmi_bdg_uxe_irq_handler_probe,
    .remove = hdmi_bdg_uxe_irq_handler_remove,
    .suspend = NULL,
    .resume = NULL,
    .driver =
        {
            .name = HDMI_BDG_IRQ_HANDLER_DEVNAME,
            .of_match_table = of_match_ptr(hdmi_bdg_uxe_irq_handler_dev_id),
        },
};

int32_t cam_hdmi_bdg_uxe_init_module(void)
{
	return platform_driver_register(&hdmi_bdg_uxe_irq_handler_drv);
}

void cam_hdmi_bdg_uxe_exit_module(void)
{
	platform_driver_unregister(&hdmi_bdg_uxe_irq_handler_drv);
}

MODULE_DESCRIPTION("CAM HDMI BDG");
MODULE_LICENSE("GPL v2");
