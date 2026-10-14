/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CAM_GPIO_COMPAT_H_
#define _CAM_GPIO_COMPAT_H_

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
/* The legacy camera GPIO table type was removed from the kernel GPIO API. */
struct gpio {
	unsigned int gpio;
	unsigned long flags;
	const char *label;
};
#endif

#endif
