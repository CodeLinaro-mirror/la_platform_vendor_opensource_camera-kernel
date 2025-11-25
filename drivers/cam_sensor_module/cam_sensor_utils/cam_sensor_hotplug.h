/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _CAM_SENSOR_HOTPLUG_H_
#define _CAM_SENSOR_HOTPLUG_H_

#define SENSOR_PLUG_OUT  0
#define SENSOR_PLUG_IN   1

enum cam_hotplug_sensor_t {
    CAM_HOTPLUG_IMAGE_SENSOR,
    CAM_HOTPLUG_GMSL_SENSOR,
    CAM_HOTPLUG_HDMI_SENSOR
};

/**
 * @brief: Sends hotplug notification message
 *
 * @type         : type of the hotplug sensor
 * @status       : One for plug in and zero for plug out event
 * @slot_id      : The slotid of the sensor
 *
 * @return Status of operation. Negative in case of error. Zero otherwise.
 */
int cam_notify_hotplug_event(enum cam_hotplug_sensor_t type, uint32_t status, uint32_t slot_id);

/**
 * @brief: Sends hotplug notification message for image sensor type
 *
 * @status       : One for plug in and zero for plug out event
 * @slot_id      : The slotid of the sensor
 *
 * @return Status of operation. Negative in case of error. Zero otherwise.
 */
int cam_sensor_notify_hotplug_event(uint32_t status, uint32_t slot_id);

#endif /* _CAM_SENSOR_HOTPLUG_H_ */
