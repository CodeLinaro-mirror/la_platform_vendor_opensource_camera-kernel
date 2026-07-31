// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/module.h>
#include "cam_sensor_fsync.h"
#include "cam_sensor_dev.h"
#include "cam_sensor_io.h"
#include "cam_debug_util.h"
#include "cam_common_util.h"
#include "cam_sensor_util.h"

int cam_sensor_fsync_handle_blob(uint8_t *blob_data, uint32_t blob_size,
	struct cam_sensor_ctrl_t *s_ctrl)
{
	struct cci_sync_info      *sync_info  = NULL;
	struct cci_timer_sync_info *timer      = NULL;
	uint32_t                   min_size   = 0;
	uint32_t                   num_timers = 0;
	uint32_t                   expected_size = 0;
	uint32_t                   i;
	int                        rc;

	/* Step 1: Minimum size check */
	min_size = sizeof(__u32) + sizeof(struct cci_timer_sync_info);

	if (blob_size < min_size) {
		CAM_ERR(CAM_SENSOR,
			"SYNC_INFO: blob too small: got=%u need>=%u",
			blob_size, min_size);
		return -EINVAL;
	}

	sync_info = (struct cci_sync_info *)blob_data;

	/* Step 2: Validate operational_mode */
	if (sync_info->operational_mode == CCI_TIMER_MODE_NO_OP ||
	    sync_info->operational_mode >= CCI_TIMER_MODE_SYNC_MAX) {
		CAM_ERR(CAM_SENSOR,
			"SYNC_INFO: invalid operational_mode=%u (valid range: %d..%d)",
			sync_info->operational_mode,
			CCI_TIMER_MODE_NO_OP + 1,
			CCI_TIMER_MODE_SYNC_MAX - 1);
		return -EINVAL;
	}

	/* Step 3: Derive number of timer entries from blob_size */
	num_timers = (blob_size - sizeof(sync_info->operational_mode)) /
		      sizeof(struct cci_timer_sync_info);

	if (num_timers == 0) {
		CAM_ERR(CAM_SENSOR,
			"SYNC_INFO: blob_size=%u yields 0 timer entries",
			blob_size);
		return -EINVAL;
	}

	expected_size = sizeof(sync_info->operational_mode) +
			num_timers * sizeof(struct cci_timer_sync_info);

	if (blob_size < expected_size) {
		CAM_ERR(CAM_SENSOR,
			"SYNC_INFO: blob_size=%u < expected=%u for %u timer(s)",
			blob_size, expected_size, num_timers);
		return -EINVAL;
	}

	CAM_DBG(CAM_SENSOR,
		 "SYNC_INFO: operational_mode=%d num_timers=%u blob_size=%u",
		 sync_info->operational_mode, num_timers, blob_size);

	/* Step 4: Decode each cci_timer_sync_info entry */
	for (i = 0; i < num_timers; i++) {
		uint64_t frame_time_us;

		timer = &sync_info->cci_timer_info_flex[i];

		if (timer->trigger_cntrl_info.tpoint_info.tp.tpoint_perframe_info >= CCI_TIMER_PERFRAME_MAX) {
			CAM_ERR(CAM_SENSOR,
				"SYNC_INFO: timer[%u] invalid tpoint_perframe_info=%u (max=%d)",
				i, timer->trigger_cntrl_info.tpoint_info.tp.tpoint_perframe_info,
				CCI_TIMER_PERFRAME_MAX - 1);
			return -EINVAL;
		}

		if (timer->trigger_cntrl_info.repeat_freq_info.freq_mode >= CCI_TIMER_FREQ_MODE_MAX) {
			CAM_ERR(CAM_SENSOR,
				"SYNC_INFO: timer[%u] invalid freq_mode=%u (max=%d)",
				i, timer->trigger_cntrl_info.repeat_freq_info.freq_mode,
				CCI_TIMER_FREQ_MODE_MAX - 1);
			return -EINVAL;
		}

		/* If no FPS is available, use 30 as default */
		if (s_ctrl->sensor_res[s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY].fps > 0)
			frame_time_us = 1000000 /
				s_ctrl->sensor_res[s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY].fps;
		else
			frame_time_us = 33333;

		if (timer->timer_info.event_count == 0 ||
		    timer->timer_info.event_count > CAM_CCI_TIMER_MAX_EVENTS) {
			CAM_ERR(CAM_SENSOR,
				"SYNC_INFO: timer[%u] invalid event_count=%u (valid: 1..%d)",
				i, timer->timer_info.event_count,
				CAM_CCI_TIMER_MAX_EVENTS);
			return -EINVAL;
		}

		CAM_DBG(CAM_SENSOR,
			 "SYNC_INFO: timer[%u] event_count=%u start_ctrl=%d",
			 i, timer->timer_info.event_count,
			 timer->trigger_cntrl_info.tpoint_info.tp.tpoint_perframe_info);

		rc = cam_sensor_util_validate_pulse_durations(&timer->timer_info,
			frame_time_us);
		if (rc < 0)
			return rc;

		CAM_DBG(CAM_SENSOR,
			 "SYNC_INFO: timer[%u] freq_mode=%d [%s] number_of_frames=%u",
			 i,
			 timer->trigger_cntrl_info.repeat_freq_info.freq_mode,
			 (timer->trigger_cntrl_info.repeat_freq_info.freq_mode ==
				CCI_TIMER_INFINITE_FRAME) ? "INFINITE" : "FINITE",
			 timer->trigger_cntrl_info.repeat_freq_info.number_of_frames);
	}

	/* Step 5: Mode-specific cross-field validation and cmd_buf conversion */
	switch (sync_info->operational_mode) {
	case CCI_TIMER_MODE_SYNC_WITH_SINGLE_QUEUE:
		if (num_timers > 3) {
			CAM_ERR(CAM_SENSOR,
				"SYNC_INFO: CCI_TIMER_MODE_SYNC_WITH_SINGLE_QUEUE "
				"max 3 timers, got %u", num_timers);
			return -EINVAL;
		}

		if (s_ctrl->io_master_info.master_type == CCI_MASTER) {
			uint32_t idx = s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY;
			struct cam_sensor_fsync_slot *slot;

			if (!s_ctrl->per_frame_fsync) {
				CAM_ERR(CAM_SENSOR,
					"SYNC_INFO: per_frame_fsync not allocated");
				return -ENOMEM;
			}

			slot = &s_ctrl->per_frame_fsync[idx];

			/*
			 * D1 (deferred): only timer[0] is converted here.
			 * Multi-timer support (timers 1 and 2) will be added
			 * as part of the fsync object redesign.
			 */
			rc = cam_cci_timing_schema_to_cmd_buf(
				&sync_info->cci_timer_info_flex[0].timer_info,
				&slot->cmd_buf[0]);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"Failed to convert timing schema: %d", rc);
				return rc;
			}

			slot->num_queues = 1;
			slot->request_id = s_ctrl->last_updated_req;
			slot->is_valid = true;

			CAM_DBG(CAM_SENSOR,
				"SYNC_INFO: converted to cmd_buf[0] (%u cmds), "
				"stored in per_frame_fsync[%u] req_id=%lld",
				slot->cmd_buf[0].cmd_count, idx,
				s_ctrl->last_updated_req);
		} else {
			CAM_INFO(CAM_SENSOR,
				"SYNC_INFO: SENSOR [%s] is not on CCI master, "
				"do not trigger SYNC CFG!", s_ctrl->sensor_name);
		}
		break;

	case CCI_TIMER_MODE_SYNC_INDEPENDENT:
	case CCI_TIMER_MODE_SYNC_WITH_MULTI_QUEUE:
	case CCI_TIMER_MODE_SYNC_WITH_ASYNC_CID_INPUT:
	case CCI_TIMER_MODE_SYNC_WITH_ASYNC_GPIO_INPUT:
	case CCI_TIMER_MODE_SYNC_WITH_ASYNC_I2C_QUEUE_INPUT:
		CAM_ERR(CAM_SENSOR, "SYNC_INFO: unsupported operationalMode %d",
			sync_info->operational_mode);
		return -EOPNOTSUPP;
	default:
		CAM_ERR(CAM_SENSOR, "SYNC_INFO: invalid operationalMode=%d",
			sync_info->operational_mode);
		return -EINVAL;
	}

	s_ctrl->fsync_blob_ready = true;
	s_ctrl->is_fsync_active = true;

	CAM_INFO(CAM_SENSOR,
		 "SYNC_INFO: decode OK - mode=%d num_timers=%u req_id=%lld "
		 "cached into per_frame_fsync[%u]",
		 sync_info->operational_mode,
		 num_timers,
		 s_ctrl->last_updated_req,
		 (uint32_t)(s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY));

	return 0;
}

int cam_sensor_fsync_apply(struct cam_sensor_ctrl_t *s_ctrl, int64_t req_id)
{
	uint32_t idx = req_id % MAX_PER_FRAME_ARRAY;
	struct cam_sensor_fsync_slot *slot;
	struct cam_sensor_cci_client *cci_client;
	int rc = 0;

	if (!s_ctrl->per_frame_fsync) {
		CAM_ERR(CAM_SENSOR, "Sensor[%s] per_frame_fsync not allocated",
			s_ctrl->sensor_name);
		return -EINVAL;
	}

	slot = &s_ctrl->per_frame_fsync[idx];

	if (!slot->is_valid || slot->request_id != req_id) {
		CAM_ERR(CAM_SENSOR,
			"Sensor[%s] no valid fsync slot for req_id=%lld "
			"(slot: valid=%d req_id=%lld)",
			s_ctrl->sensor_name, req_id,
			slot->is_valid, slot->request_id);
		return -EINVAL;
	}

	if (s_ctrl->io_master_info.master_type != CCI_MASTER ||
	    !s_ctrl->io_master_info.cci_client) {
		CAM_ERR(CAM_SENSOR,
			"Sensor[%s] GPIO queue only supported on CCI master",
			s_ctrl->sensor_name);
		rc = -EINVAL;
		goto clear;
	}

	cci_client = s_ctrl->io_master_info.cci_client;

	CAM_DBG(CAM_SENSOR, "Sensor[%s] fsync apply req_id=%lld num_queues=%u",
		s_ctrl->sensor_name, req_id, slot->num_queues);

	/*
	 * Pass cmd_buf[0] directly to the CCI layer via the cci_client
	 * staging field. This will be removed once cam_cci_load_gpio_queue()
	 * is updated to accept a cmd_buf pointer directly (item 6 of the
	 * fsync redesign).
	 */
	cci_client->cmd_buf = slot->cmd_buf[0];

	/*
	 * MSM_CCI_TIMER_FSYNC_INDEPENDENT now handles load + start +
	 * transient queue release in a single call.
	 */
	rc = cam_sensor_cci_i2c_util(&s_ctrl->io_master_info,
		MSM_CCI_TIMER_FSYNC_INDEPENDENT);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR,
			"Sensor[%s] GPIO fsync failed rc=%d req_id=%lld",
			s_ctrl->sensor_name, rc, req_id);

clear:
	slot->is_valid = false;
	slot->request_id = 0;
	slot->num_queues = 0;

	return rc;
}
