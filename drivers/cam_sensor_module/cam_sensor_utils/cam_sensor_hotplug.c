// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "cam_sensor_util.h"
#include "cam_req_mgr.h"
#include "cam_req_mgr_dev.h"
#include "cam_sensor_hotplug.h"

int cam_notify_hotplug_event(
        enum cam_hotplug_sensor_t type, uint32_t status, uint32_t slot_id)
{
  int rc;
  struct cam_req_mgr_message req_msg = {0};

  req_msg.u.hotplug_msg.status = status;
  req_msg.u.hotplug_msg.slot_id = slot_id;
  req_msg.u.hotplug_msg.type = type;

  rc = cam_req_mgr_notify_message(&req_msg,
      V4L_EVENT_CAM_REQ_MGR_HOTPLUG_EVT,
      V4L_EVENT_CAM_REQ_MGR_EVENT);

  if (rc) {
      CAM_ERR(CAM_SENSOR,
              "Error when notifying v4l2 hotplug_event for sensor type %d status %d slot_id %d",
              type, status, slot_id);
  }

  return rc;
}

int cam_sensor_notify_hotplug_event(uint32_t status, uint32_t slot_id)
{
    int rc;

    if (status > SENSOR_PLUG_IN) {
        CAM_ERR(CAM_SENSOR,
                "Invalid hotplug status: %u for slot_id: %u",
                status, slot_id);
        return -EINVAL;
    }

    rc = cam_notify_hotplug_event(
                CAM_HOTPLUG_IMAGE_SENSOR, status, slot_id);

    if (rc) {
        CAM_ERR(CAM_SENSOR,
                "Failed to notify hotplug event for slot_id: %u status: %u rc: %d",
                slot_id, status, rc);
        return rc;
    }

    return 0;
}

EXPORT_SYMBOL_GPL(cam_sensor_notify_hotplug_event);
