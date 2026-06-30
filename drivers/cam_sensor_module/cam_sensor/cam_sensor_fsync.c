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

static int cam_sensor_convert_sync_info_to_cmd_buf(
	struct cci_sync_info *sync_info,
	struct cam_cci_gpio_cmd_buf *cmd_buf)
{
	struct cci_gpio_timing_schema *schema = NULL;
	int rc;

	if (!sync_info || !cmd_buf) {
		CAM_ERR(CAM_SENSOR,
			"Invalid args sync_info=%pK cmd_buf=%pK",
			sync_info, cmd_buf);
		return -EINVAL;
	}

	schema = &sync_info->cci_timer_info_flex[0].timer_info;

	rc = cam_cci_timing_schema_to_cmd_buf(schema, cmd_buf);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "Failed to convert timing schema: %d", rc);
		return rc;
	}

	return 0;
}

static int cam_sensor_load_gpio_queue_from_cmd_buf(
	struct cam_sensor_ctrl_t *s_ctrl,
	struct cam_cci_gpio_cmd_buf *cmd_buf,
	int64_t req_id)
{
	int rc = 0;
	uint32_t idx;

	if (!s_ctrl || !cmd_buf) {
		CAM_ERR(CAM_SENSOR,
			"Invalid args s_ctrl=%pK cmd_buf=%pK",
			s_ctrl, cmd_buf);
		return -EINVAL;
	}

	if (!cmd_buf->cmd_buf_ready || cmd_buf->cmd_count == 0) {
		CAM_ERR(CAM_SENSOR,
			"CMD buffer not ready or empty: ready=%d count=%d",
			cmd_buf->cmd_buf_ready, cmd_buf->cmd_count);
		return -EINVAL;
	}

	if (s_ctrl->io_master_info.master_type != CCI_MASTER) {
		CAM_ERR(CAM_SENSOR,
			"GPIO queue loading only supported on CCI master");
		return -EINVAL;
	}

	if (!s_ctrl->io_master_info.cci_client) {
		CAM_ERR(CAM_SENSOR, "cci_client is NULL");
		return -EINVAL;
	}

	/* Copy cmd_buf to cci_client */
	memcpy(&s_ctrl->io_master_info.cci_client->cmd_buf,
	       cmd_buf,
	       sizeof(struct cam_cci_gpio_cmd_buf));

	/* Also need to copy sync_cfg for CPAS configuration */
	idx = req_id % MAX_PER_FRAME_ARRAY;
	if (s_ctrl->per_frame_sync_info &&
	    s_ctrl->per_frame_sync_info[idx].is_settings_valid &&
	    s_ctrl->per_frame_sync_info[idx].request_id == req_id) {
		memcpy(&s_ctrl->io_master_info.cci_client->sync_cfg,
		       &s_ctrl->per_frame_sync_info[idx].sync_info,
		       sizeof(s_ctrl->io_master_info.cci_client->sync_cfg));
	} else {
		CAM_ERR(CAM_SENSOR,
			"No sync_info found for req_id=%lld", req_id);
		return -EINVAL;
	}

	/* Call MSM_CCI_TIMER_FSYNC_INDEPENDENT but skip conversion
	 * since cmd_buf is already populated */
	rc = cam_sensor_cci_i2c_util(
		&s_ctrl->io_master_info,
		MSM_CCI_TIMER_FSYNC_INDEPENDENT);
	if (rc == 0) {
		CAM_DBG(CAM_SENSOR,
			"Loaded GPIO queue with %d commands for req_id=%lld",
			cmd_buf->cmd_count, req_id);
	} else {
		CAM_ERR(CAM_SENSOR, "Failed to load GPIO queue: %d", rc);
	}

	return rc;
}

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

	/* Step 5: Mode-specific cross-field validation */
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

			rc = cam_sensor_convert_sync_info_to_cmd_buf(
				sync_info, &s_ctrl->per_frame_cmd_buf[idx]);
			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"Failed to convert sync_info to cmd_buf: %d", rc);
				return rc;
			}
			CAM_DBG(CAM_SENSOR,
				"SYNC_INFO: Converted to cmd_buf and stored in per_frame[%u]",
				idx);
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

	/* Step 6: Cache validated sync config into s_ctrl */
	if (s_ctrl->per_frame_sync_info) {
		uint32_t idx = s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY;
		struct sync_info_data *sync_data = &s_ctrl->per_frame_sync_info[idx];

		memcpy(&sync_data->sync_info, sync_info, sizeof(struct cci_sync_info));
		sync_data->request_id = s_ctrl->last_updated_req;
		sync_data->is_settings_valid = 1;

		/* Also cache in s_ctrl for backward compatibility */
		memcpy(&s_ctrl->sync_cfg, sync_info, sizeof(s_ctrl->sync_cfg));
		s_ctrl->fsync_blob_ready = true;
		s_ctrl->is_fsync_active = true;
	} else {
		CAM_ERR(CAM_SENSOR, "SYNC_INFO: per_frame_sync_info not allocated");
		return -ENOMEM;
	}

	CAM_INFO(CAM_SENSOR,
		 "SYNC_INFO: decode OK - mode=%d num_timers=%u req_id=%lld "
		 "cached into per_frame[%u]",
		 sync_info->operational_mode,
		 num_timers,
		 s_ctrl->last_updated_req,
		 (uint32_t)(s_ctrl->last_updated_req % MAX_PER_FRAME_ARRAY));

	return 0;
}

int cam_sensor_fsync_apply(struct cam_sensor_ctrl_t *s_ctrl, int64_t req_id)
{
	uint32_t offset = req_id % MAX_PER_FRAME_ARRAY;
	int rc = 0;

	if (!s_ctrl->per_frame_sync_info ||
	    !s_ctrl->per_frame_sync_info[offset].is_settings_valid ||
	    s_ctrl->per_frame_sync_info[offset].request_id != req_id) {
		CAM_ERR(CAM_SENSOR, "Sensor[%s] invalid sync info for request id: %lld",
			s_ctrl->sensor_name, req_id);
		return -EINVAL;
	}

	CAM_DBG(CAM_SENSOR, "Sensor[%s] fsync apply for request id: %lld",
		s_ctrl->sensor_name, req_id);

	if (s_ctrl->per_frame_cmd_buf &&
	    s_ctrl->per_frame_cmd_buf[offset].cmd_buf_ready) {
		rc = cam_sensor_load_gpio_queue_from_cmd_buf(
			s_ctrl,
			&s_ctrl->per_frame_cmd_buf[offset],
			req_id);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"Failed to load GPIO queue from cmd_buf: %d", rc);
			goto clear;
		}

		rc = cam_sensor_cci_i2c_util(&s_ctrl->io_master_info, MSM_CCI_GPIO_QUEUE_START);
		if (rc < 0)
			CAM_ERR(CAM_SENSOR, "GPIO queue start failed: %d", rc);
	} else {
		CAM_ERR(CAM_SENSOR, "Sensor[%s] no cmd_buf ready for req_id=%lld",
			s_ctrl->sensor_name, req_id);
		rc = -EINVAL;
	}

clear:
	/* Clear the applied entry regardless of success or failure */
	s_ctrl->per_frame_sync_info[offset].is_settings_valid = 0;
	s_ctrl->per_frame_sync_info[offset].request_id = 0;

	if (s_ctrl->per_frame_cmd_buf) {
		s_ctrl->per_frame_cmd_buf[offset].cmd_buf_ready = false;
		s_ctrl->per_frame_cmd_buf[offset].cmd_count = 0;
	}

	return rc;
}
