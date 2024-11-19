// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include "cam_cci_core.h"
#include "cam_cci_dev.h"
#include "cam_req_mgr_worker_wrapper.h"
#include "cam_common_util.h"

static uint32_t cam_cci_retry(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue);

static int32_t cam_cci_convert_type_to_num_bytes(
	enum camera_sensor_i2c_type type)
{
	int32_t num_bytes;

	switch (type) {
	case CAMERA_SENSOR_I2C_TYPE_BYTE:
		num_bytes = 1;
		break;
	case CAMERA_SENSOR_I2C_TYPE_WORD:
		num_bytes = 2;
		break;
	case CAMERA_SENSOR_I2C_TYPE_3B:
		num_bytes = 3;
		break;
	case CAMERA_SENSOR_I2C_TYPE_DWORD:
		num_bytes = 4;
		break;
	default:
		CAM_ERR(CAM_CCI, "Wrong Sensor I2c Type: %d", type);
		num_bytes = 0;
		break;
	}
	return num_bytes;
}

static int cam_cci_get_context_id(
	struct cci_device *cci_dev,
	uint8_t active_trigger_sensor,
	enum cci_i2c_master_t master,
	struct cam_cci_trigger_data *ctx)
{
	int32_t rc = 0, i = 0;
	struct cci_trigger_data *trigger_data =
		kzalloc(sizeof(struct cci_trigger_data), GFP_KERNEL);
	struct cam_cci_master_info *cci_master_info;
	struct cam_cci_gpio_info *cci_gpio_info;

	cci_master_info = &cci_dev->cci_master_info[master];
	cci_gpio_info = &cci_dev->cci_gpio_info;

	if (!trigger_data) {
		CAM_ERR(CAM_CCI, "[%d] alloc failed", cci_dev->soc_info.index);
		return -ENOMEM;
	}
	for (i = 0; i < NUM_QUEUES; i++) {
		if (mutex_trylock(&cci_master_info->mutex_q[i])) {
			if (cci_dev->cci_i2c_queue_info[master][i].queue_status ==
					QUEUE_STATE_FREE) {
				trigger_data->i2cqueue = i;
				cci_dev->cci_i2c_queue_info[master][i].queue_status =
					QUEUE_STATE_BUSY;
				mutex_unlock(&cci_master_info->mutex_q[i]);
				break;
			}
			mutex_unlock(&cci_master_info->mutex_q[i]);
		}
	}

	if (i == NUM_QUEUES) {
		CAM_ERR(CAM_CCI, "[%d] i2c queue not available", cci_dev->soc_info.index);
		return -EINVAL;
	}

	for (i = 0; i < NUM_GPIO_QUEUES; i++) {
		if (mutex_trylock(&cci_gpio_info->mutex_q[i])) {
			if (cci_dev->cci_gpio_queue_info[i].queue_status == QUEUE_STATE_FREE) {
				trigger_data->gpioqueue = i;
				cci_dev->cci_gpio_queue_info[i].queue_status = QUEUE_STATE_BUSY;
				mutex_unlock(&cci_gpio_info->mutex_q[i]);
				break;
			}
			mutex_unlock(&cci_gpio_info->mutex_q[i]);
		}
	}

	if (i == NUM_GPIO_QUEUES) {
		CAM_ERR(CAM_CCI, "[%d] GPIO queue not available", cci_dev->soc_info.index);
		cci_dev->cci_i2c_queue_info[master][trigger_data->i2cqueue].queue_status =
			QUEUE_STATE_FREE;
		return -EINVAL;
	}

	for (i = 0 ; i < CONTEXT_ID_MAX; i++) {
		if (!cci_dev->is_contextid_acquire[i]) {
			cci_dev->is_contextid_acquire[i] = true;
			ctx->context_id = i;
			break;
		}
	}

	if (i == CONTEXT_ID_MAX) {
		CAM_ERR(CAM_CCI, "[%d]  All context are acquired", cci_dev->soc_info.index);
		cci_dev->cci_i2c_queue_info[master][trigger_data->i2cqueue].queue_status =
			QUEUE_STATE_FREE;
		cci_dev->cci_i2c_queue_info[master][trigger_data->gpioqueue].queue_status =
			QUEUE_STATE_FREE;
		return -EINVAL;
	}

	switch (active_trigger_sensor) {
	case CCI_SET_CID_SYNC_TIMER_0:
		trigger_data->idx = CCI_SET_CID_SYNC_TIMER_0;
		break;
	case CCI_SET_CID_SYNC_TIMER_1:
		trigger_data->idx = CCI_SET_CID_SYNC_TIMER_1;
		break;
	case CCI_SET_CID_SYNC_TIMER_2:
		trigger_data->idx = CCI_SET_CID_SYNC_TIMER_2;
		break;
	case CCI_SET_CID_SYNC_TIMER_3:
		trigger_data->idx = CCI_SET_CID_SYNC_TIMER_3;
		break;
	default:
		CAM_ERR(CAM_CCI, "Invalid idmap");
		cci_dev->cci_i2c_queue_info[master][trigger_data->i2cqueue].queue_status =
			QUEUE_STATE_FREE;
		cci_dev->cci_i2c_queue_info[master][trigger_data->gpioqueue].queue_status =
			QUEUE_STATE_FREE;
		cci_dev->is_contextid_acquire[ctx->context_id] = false;
		return -EINVAL;
	}
	trigger_data->csid = ctx->csid;
	trigger_data->cid = ctx->cid;
	trigger_data->contextId  = ctx->context_id;
	trigger_data->master = master;
	CAM_DBG(CAM_CCI, "csid: %d cid: %d context_id %d", ctx->csid, ctx->cid, ctx->context_id);
	CAM_DBG(CAM_CCI, " i2cqueue %d master %d gpioqueue %d",
		trigger_data->i2cqueue, trigger_data->master, trigger_data->gpioqueue);
	list_add_tail(&(trigger_data->list), &cci_dev->trigger_ctx_array[trigger_data->contextId]);
	cci_dev->cci_gpio_info.is_initilized = false;
	return rc;
}

static struct cci_trigger_data *cam_cci_get_ctx(
	struct cci_device *cci_dev,
	uint16_t idx)
{
	int i = 0;
	struct list_head *pos = NULL, *pos_next = NULL;
	struct cci_trigger_data *entry = NULL;

	if (cci_dev->num_active_trigger_sensor > 0) {
		mutex_lock(&cci_dev->ctx_mutex);
		for (i = 0; i < CONTEXT_ID_MAX; i++) {
			list_for_each_safe(pos,
			pos_next, &cci_dev->trigger_ctx_array[i]) {
				entry = list_entry(pos, struct cci_trigger_data, list);
				if (entry->idx == idx) {
					mutex_unlock(&cci_dev->ctx_mutex);
					return entry;
				}
			}
		}
		mutex_unlock(&cci_dev->ctx_mutex);
	}
	return entry;
}

static enum cci_gpio_queue_t cam_cci_get_gpio_queue(
	struct cci_device *cci_dev,
	enum cci_i2c_queue_t queue)
{
	int i = 0;
	struct list_head *pos = NULL, *pos_next = NULL;
	struct cci_trigger_data *entry = NULL;

	if (cci_dev->num_active_trigger_sensor > 0) {
		for (i = 0; i < CONTEXT_ID_MAX; i++) {
			list_for_each_safe(pos,
			pos_next, &cci_dev->trigger_ctx_array[i]) {
				entry = list_entry(pos, struct cci_trigger_data, list);
				if (entry->i2cqueue == queue)
					return entry->gpioqueue;
			}
		}
	}
	return GPIOQUEUE_INVALID;
}

static struct cci_trigger_data *cam_cci_get_context(
	struct cci_device *cci_dev,
	int contextId)
{
	struct list_head *pos = NULL, *pos_next = NULL;
	struct cci_trigger_data *entry = NULL;
	struct cci_trigger_data *matched_entry = NULL;

	CAM_DBG(CAM_CCI, "contextId: %d", contextId);

	if (contextId < CONTEXT_ID_MAX) {
		mutex_lock(&cci_dev->ctx_mutex);
		list_for_each_safe(pos,
		pos_next, &cci_dev->trigger_ctx_array[contextId]) {
			entry = list_entry(pos, struct cci_trigger_data, list);
			if (entry->contextId == contextId)
				matched_entry = entry;
		}
		mutex_unlock(&cci_dev->ctx_mutex);
	} else {
		CAM_ERR(CAM_CCI, "Invalid contextId: %d", contextId);
		return NULL;
	}
	return matched_entry;
}

static int cam_cci_release_trigger_data(
	struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	int contextId)
{
	struct list_head *pos = NULL, *pos_next = NULL;
	struct cci_trigger_data *entry = NULL;
	enum cci_i2c_queue_t i2cqueue;
	struct cci_trigger_data *get_ctx = cam_cci_get_context(cci_dev, contextId);

	if (!get_ctx) {
		CAM_ERR(CAM_CCI, "Invalid contextId");
		return -EINVAL;
	}

	i2cqueue = get_ctx->i2cqueue;
	CAM_DBG(CAM_CCI, "contextId: %d", contextId);

	mutex_lock(&cci_dev->ctx_mutex);
	list_for_each_safe(pos,
	pos_next, &cci_dev->trigger_ctx_array[contextId]) {
		entry = list_entry(pos, struct cci_trigger_data, list);
		if (entry->contextId == contextId) {
			cci_dev->is_contextid_acquire[contextId] = false;
			cci_dev->cci_i2c_queue_info[master][i2cqueue].queue_status =
				QUEUE_STATE_FREE;
			cci_dev->cci_gpio_queue_info[get_ctx->gpioqueue].queue_status =
				QUEUE_STATE_FREE;
			list_del(&entry->list);
			kfree(entry);
		}
	}
	mutex_unlock(&cci_dev->ctx_mutex);

	return 0;
}

static int cam_gpio_init(
	struct cam_cci_ctrl *c_ctrl,
	struct cci_device *cci_dev, enum cci_gpio_queue_t queue,
	struct cci_trigger_data  *get_ctx)
{
	int rc = 0, i = 0;
	uint32_t offset = 0, gpiooffset = 0;
	void __iomem *base = NULL;
	void __iomem *base1 = NULL;
	struct cam_hw_soc_info *soc_info = NULL;
	struct device_node *of_node = NULL;

	c_ctrl->cci_info->id_map = get_ctx->idx;

	soc_info = &cci_dev->soc_info;
	base = soc_info->reg_map[0].mem_base;

	if (!soc_info || !base) {
		CAM_ERR(CAM_CCI,
			"CCI%d_GPIOQ%d failed: invalid params soc_info:%pK, base:%pK",
			cci_dev->soc_info.index, queue, soc_info, base);
		rc = -EINVAL;
		return rc;
	}

	CAM_DBG(CAM_CCI,
		"CCI%d_GPIOQ%d gpio_mask 0x%x",
		cci_dev->soc_info.index, queue, c_ctrl->cfg.gpio_cfg.reg_setting->gpio_mask);


	if (cci_dev->num_active_trigger_sensor == 1) {
		/* reinit the reports for the queue */
		for (i = 0; i < NUM_GPIO_QUEUES; i++) {
			reinit_completion(
				&cci_dev->cci_gpio_info.report_q[i]);
			reinit_completion(
				&cci_dev->cci_gpio_info.reset_complete[i]);
		}

		/* Setting up the queue size */
		cci_dev->cci_gpio_queue_info[GPIOQUEUE_0].max_queue_size
				= CCI_GPIO_QUEUE_0_SIZE;
		cci_dev->cci_gpio_queue_info[GPIOQUEUE_1].max_queue_size
				= CCI_GPIO_QUEUE_1_SIZE;
		cci_dev->cci_gpio_queue_info[GPIOQUEUE_2].max_queue_size
				= CCI_GPIO_QUEUE_2_SIZE;
	}

	/* Set reset pending flag to true */
	cci_dev->cci_gpio_info.reset_pending[queue] = true;
	cci_dev->cci_gpio_info.status = 0;
	cam_io_w_mb((1 | (1 << (CCI_GPIO_RESET_RMSK + queue))),
		base + CCI_RESET_CMD_ADDR);


	if (!cam_common_wait_for_completion_timeout(
		&cci_dev->cci_gpio_info.reset_complete[queue],
		CCI_TIMEOUT)) {
		CAM_ERR(CAM_CCI,
			"CCI%d_GPIOQ%d Failed: reset complete timeout",
			cci_dev->soc_info.index, queue);
		rc = -ETIMEDOUT;
		return rc;
	}

	base1 = soc_info->reg_map[1].mem_base;
	of_node = cci_dev->v4l2_dev_str.pdev->dev.of_node;
	rc = of_property_read_u32(of_node, "cpas-mux-en-offset", &offset);
	if (rc) {
		CAM_ERR(CAM_CCI, "failed to read cpas-mux-en-offset");
		rc = -EINVAL;
		return rc;
	}

	CAM_DBG(CAM_CCI, "cpas-mux-en-offset:0x%x csid:%d cid:%d num_active_trigger_sensor %d",
		offset, get_ctx->csid, get_ctx->cid, cci_dev->num_active_trigger_sensor);
	if (base1 && rc == 0) {
		if (get_ctx->csid <= 3) {
			cam_io_w_mb(get_ctx->csid * CCI_SET_CSID_OFFSET,
				base1 + offset);
		} else {
			cam_io_w_mb((get_ctx->csid + 1) * CCI_SET_CSID_OFFSET,
				base1 + offset);
		}
	}
	rc = of_property_read_u32(of_node, "cpas-mux-en-gpio-offset", &gpiooffset);
	if (rc) {
		CAM_ERR(CAM_CCI, "failed to read cpas-mux-en-gpio-offset");
		rc = -EINVAL;
		return rc;
	}
	CAM_DBG(CAM_CCI, "cpas-mux-en-gpio-offset 0x%x", gpiooffset);

	if (cci_dev->soc_info.index == 1 || cci_dev->soc_info.index == 3 ||
			cci_dev->soc_info.index == 5) {
		CAM_DBG(CAM_CCI, "value 0x%x", 0x1f &
			(1 <<  c_ctrl->cfg.gpio_cfg.reg_setting->gpio_mask));
		cam_io_w_mb(0x1f &
			(1 <<  c_ctrl->cfg.gpio_cfg.reg_setting->gpio_mask), base1 + gpiooffset);
	} else {
		CAM_DBG(CAM_CCI, "value 0x%x", 0x1f &
			~(1 <<  c_ctrl->cfg.gpio_cfg.reg_setting->gpio_mask));
		cam_io_w_mb(0x1f &
			~(1 <<  c_ctrl->cfg.gpio_cfg.reg_setting->gpio_mask), base1 + gpiooffset);
	}

	cam_io_w_mb(get_ctx->cid,
		base + CCI_SET_CID_SYNC_TIMER_ADDR +
		get_ctx->idx *
		CCI_SET_CID_SYNC_TIMER_OFFSET);

	CAM_DBG(CAM_CCI, "CCI%d_GPIO: Q0: %d Q1: %d Q2: %d",
		cci_dev->soc_info.index,
		cci_dev->cci_gpio_queue_info[GPIOQUEUE_0].max_queue_size,
		cci_dev->cci_gpio_queue_info[GPIOQUEUE_1].max_queue_size,
		cci_dev->cci_gpio_queue_info[GPIOQUEUE_2].max_queue_size);

	cci_dev->cci_gpio_info.status = 0;
	cci_dev->cci_gpio_info.is_initilized = true;
	return rc;
}

static void cam_cci_gpio_flush_queue(struct cci_device *cci_dev,
	enum cci_i2c_queue_t queue)
{
	int32_t rc = 0;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;
	enum cci_gpio_queue_t gpioqueue;

	mutex_lock(&cci_dev->ctx_mutex);
	gpioqueue = cam_cci_get_gpio_queue(cci_dev, queue);
	mutex_unlock(&cci_dev->ctx_mutex);

	if (gpioqueue == GPIOQUEUE_INVALID) {
		CAM_ERR(CAM_CCI, "invalid gpio queue");
		rc = -EINVAL;
	} else {
		cam_io_w_mb(1 << (2 + gpioqueue), base + CCI_HALT_REQ_ADDR);
		if (!cci_dev->cci_gpio_info.status)
			reinit_completion(&cci_dev->cci_gpio_info
				.reset_complete[gpioqueue]);
		if (!cam_common_wait_for_completion_timeout(
			&cci_dev->cci_gpio_info.reset_complete[gpioqueue],
			CCI_TIMEOUT)) {
			CAM_DBG(CAM_CCI,
				"CCI%d_GPIO wait timeout for reset complete",
				cci_dev->soc_info.index);

			/* Set reset pending flag to true */
			cci_dev->cci_gpio_info.reset_pending[gpioqueue] = true;
			cci_dev->cci_gpio_info.status = 0;

			/* Set proper mask to RESET CMD address based on MASTER */
			cam_io_w_mb(1 | (1 << (CCI_GPIO_RESET_RMSK + gpioqueue)),
				base + CCI_RESET_CMD_ADDR);

			/* wait for reset done irq */
			if (!cam_common_wait_for_completion_timeout(
				&cci_dev->cci_gpio_info.reset_complete[gpioqueue],
				CCI_TIMEOUT)) {
				rc = -EINVAL;
				CAM_ERR(CAM_CCI,
					"CCI%d_GPIO Retry:: wait timeout for reset complete",
					cci_dev->soc_info.index);
			}
			cci_dev->cci_gpio_info.status = 0;
		}
	}
	if (!rc)
		CAM_DBG(CAM_CCI,
			"CCI%d_GPIO Success: Reset complete",
			cci_dev->soc_info.index);
}

static void cam_cci_flush_queue(struct cci_device *cci_dev,
	enum cci_i2c_master_t master)
{
	int32_t rc = 0;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;

	cam_io_w_mb(1 << master, base + CCI_HALT_REQ_ADDR);
	if (!cci_dev->cci_master_info[master].status)
		reinit_completion(&cci_dev->cci_master_info[master]
			.reset_complete);
	if (!cam_common_wait_for_completion_timeout(
		&cci_dev->cci_master_info[master].reset_complete,
		CCI_TIMEOUT)) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d wait timeout for reset complete",
			cci_dev->soc_info.index, master);

		/* Set reset pending flag to true */
		cci_dev->cci_master_info[master].reset_pending = true;
		cci_dev->cci_master_info[master].status = 0;

		/* Set proper mask to RESET CMD address based on MASTER */
		if (master == MASTER_0)
			cam_io_w_mb(CCI_M0_RESET_RMSK,
				base + CCI_RESET_CMD_ADDR);
		else
			cam_io_w_mb(CCI_M1_RESET_RMSK,
				base + CCI_RESET_CMD_ADDR);

		/* wait for reset done irq */
		if (!cam_common_wait_for_completion_timeout(
			&cci_dev->cci_master_info[master].reset_complete,
			CCI_TIMEOUT)) {
			rc = -EINVAL;
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d Retry:: wait timeout for reset complete",
				cci_dev->soc_info.index, master);
		}
		cci_dev->cci_master_info[master].status = 0;
	}

	if (!rc)
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d Success: Reset complete",
			cci_dev->soc_info.index, master);
}

static int32_t cam_cci_validate_gpio_queue(struct cci_device *cci_dev,
	uint32_t len,
	enum cci_gpio_queue_t queue)
{
	int32_t rc = 0;
	uint32_t read_val = 0;
	uint32_t reg_offset = queue * 0x100;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;
	unsigned long flags;

	read_val = cam_io_r_mb(base +
		CCI_GPIO_Q0_CUR_WORD_CNT_ADDR + reg_offset);
	CAM_DBG(CAM_CCI,
		"CCI%d_GPIO_Q%d_CUR_WORD_CNT_ADDR %d len %d max %d",
		cci_dev->soc_info.index, queue, read_val, len,
		cci_dev->cci_gpio_queue_info[queue].max_queue_size);
	if ((read_val + len + 1) >
		cci_dev->cci_gpio_queue_info[queue].max_queue_size) {
		uint32_t reg_val = 0;

		cam_io_w_mb(read_val, base +
			CCI_GPIO_Q0_EXEC_WORD_CNT_ADDR + reg_offset);
		reg_val = 1 << (GPIO_QUEUE_START_OFFSET + queue);
		CAM_DBG(CAM_CCI, "CCI%d_GPIO_Q%d_START_ADDR", cci_dev->soc_info.index, queue);
		spin_lock_irqsave(
			&cci_dev->cci_gpio_info.lock_q[queue], flags);
		cam_io_w_mb(reg_val, base + CCI_QUEUE_START_ADDR);
		spin_unlock_irqrestore(
			&cci_dev->cci_gpio_info.lock_q[queue], flags);
		rc = cci_dev->cci_gpio_info.status;
		if (rc < 0) {
			CAM_ERR(CAM_CCI, "CCI%d_GPIO_Q%d is in error state",
				cci_dev->soc_info.index, queue);
			cci_dev->cci_gpio_info.status = 0;
		}
	}

	return rc;
}

static int32_t cam_cci_validate_queue(struct cci_device *cci_dev,
	uint32_t len,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	int32_t rc = 0;
	uint32_t read_val = 0;
	uint32_t reg_offset = master * 0x200 + queue * 0x100;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;
	unsigned long flags;

	read_val = cam_io_r_mb(base +
		CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR + reg_offset);
	CAM_DBG(CAM_CCI,
		"CCI%d_I2C_M%d_Q%d_CUR_WORD_CNT_ADDR %d len %d max %d",
		cci_dev->soc_info.index, master, queue, read_val, len,
		cci_dev->cci_i2c_queue_info[master][queue].max_queue_size);
	if ((read_val + len + 1) >
		cci_dev->cci_i2c_queue_info[master][queue].max_queue_size) {
		uint32_t reg_val = 0;
		uint32_t report_id =
			cci_dev->cci_i2c_queue_info[master][queue].report_id;
		uint32_t report_val = CCI_I2C_REPORT_CMD | (1 << 8) |
			(1 << 9) | (report_id << 4);

		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d_REPORT_CMD",
			cci_dev->soc_info.index, master, queue);
		cam_io_w_mb(report_val,
			base + CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
			reg_offset);
		read_val++;
		cci_dev->cci_i2c_queue_info[master][queue].report_id++;
		if (cci_dev->cci_i2c_queue_info[master][queue].report_id == REPORT_IDSIZE)
			cci_dev->cci_i2c_queue_info[master][queue].report_id = 0;

		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d_EXEC_WORD_CNT_ADDR %d",
			cci_dev->soc_info.index, master, queue, read_val);
		cam_io_w_mb(read_val, base +
			CCI_I2C_M0_Q0_EXEC_WORD_CNT_ADDR + reg_offset);
		reg_val = 1 << ((master * 2) + queue);
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d_START_ADDR", cci_dev->soc_info.index, master, queue);
		spin_lock_irqsave(
			&cci_dev->cci_master_info[master].lock_q[queue], flags);
		atomic_set(
			&cci_dev->cci_master_info[master].done_pending[queue],
			1);
		cam_io_w_mb(reg_val, base + CCI_QUEUE_START_ADDR);
		atomic_set(&cci_dev->cci_master_info[master].q_free[queue], 1);
		spin_unlock_irqrestore(
			&cci_dev->cci_master_info[master].lock_q[queue], flags);
		rc = cam_common_wait_for_completion_timeout(
			&cci_dev->cci_master_info[master].report_q[queue],
			CCI_TIMEOUT);
		if (rc <= 0) {
			CAM_ERR(CAM_CCI, "Wait_for_completion_timeout: rc: %d",
				rc);
			rc = cam_cci_retry(cci_dev, master, queue);
			if (!rc)
				CAM_INFO(CAM_CCI,
					"CCI%d_I2C_M%d_Q%d retry success",
					soc_info->index, master, queue);
			else
				return rc;
		}
		rc = cci_dev->cci_master_info[master].status;
		if (rc < 0) {
			CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d is in error state",
				cci_dev->soc_info.index, master, queue);
			cci_dev->cci_master_info[master].status = 0;
		}
	}

	return rc;
}

static int32_t cam_cci_write_i2c_queue(struct cci_device *cci_dev,
	uint32_t val,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	int32_t rc = 0;
	uint32_t reg_offset = master * 0x200 + queue * 0x100;
	struct cam_hw_soc_info *soc_info = NULL;
	void __iomem *base = NULL;

	if (!cci_dev) {
		CAM_ERR(CAM_CCI, "cci_dev NULL");
		return -EINVAL;
	}

	soc_info = &cci_dev->soc_info;
	base = soc_info->reg_map[0].mem_base;

	rc = cam_cci_validate_queue(cci_dev, 1, master, queue);
	if (rc < 0) {
		CAM_ERR(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to validate:: rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		return rc;
	}
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d_LOAD_DATA_ADDR:val 0x%x:0x%x ",
		cci_dev->soc_info.index, master, queue,
		CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
		reg_offset, val);
	cam_io_w_mb(val, base + CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
		reg_offset);

	return rc;
}

static int32_t cam_cci_lock_queue(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue, uint32_t en)
{
	uint32_t val;

	if (queue != PRIORITY_QUEUE)
		return 0;

	val = en ? CCI_I2C_LOCK_CMD : CCI_I2C_UNLOCK_CMD;
	return cam_cci_write_i2c_queue(cci_dev, val, master, queue);
}


void cam_cci_dump_registers(struct cci_device *cci_dev,
	enum cci_i2c_master_t master, enum cci_i2c_queue_t queue)
{
	uint32_t dump_en = 0;
	uint32_t read_val = 0;
	uint32_t i = 0;
	uint32_t reg_offset = 0;
	void __iomem *base = cci_dev->soc_info.reg_map[0].mem_base;

	dump_en = cci_dev->dump_en;
	if (!(dump_en & CAM_CCI_NACK_DUMP_EN) &&
		!(dump_en & CAM_CCI_TIMEOUT_DUMP_EN)) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Nack and Timeout dump is not enabled",
			cci_dev->soc_info.index, master, queue);
		return;
	}

	CAM_INFO(CAM_CCI, "**** CCI%d_I2C_M%d_Q%d register dump ****",
		cci_dev->soc_info.index, master, queue);

	/* CCI Top Registers */
	CAM_INFO(CAM_CCI, "**** CCI TOP Registers ****");
	for (i = 0; i < DEBUG_TOP_REG_COUNT; i++) {
		reg_offset = DEBUG_TOP_REG_START + i * 4;
		read_val = cam_io_r_mb(base + reg_offset);
		CAM_INFO(CAM_CCI, "offset = 0x%X value = 0x%X",
			reg_offset, read_val);
	}

	/* CCI Master registers */
	CAM_INFO(CAM_CCI, "**** CCI%d_I2C_M%d Registers ****",
		cci_dev->soc_info.index, master);
	for (i = 0; i < DEBUG_MASTER_REG_COUNT; i++) {
		if ((i * 4) == 0x18)
			continue;

		reg_offset = DEBUG_MASTER_REG_START + master*0x100 + i * 4;
		read_val = cam_io_r_mb(base + reg_offset);
		CAM_INFO(CAM_CCI, "offset = 0x%X value = 0x%X",
			reg_offset, read_val);
	}

	/* CCI Master Queue registers */
	CAM_INFO(CAM_CCI, " **** CCI%d_I2C_M%d_Q%d Registers ****",
		cci_dev->soc_info.index, master, queue);
	for (i = 0; i < DEBUG_MASTER_QUEUE_REG_COUNT; i++) {
		reg_offset = DEBUG_MASTER_QUEUE_REG_START +  master*0x200 +
			queue*0x100 + i * 4;
		read_val = cam_io_r_mb(base + reg_offset);
		CAM_INFO(CAM_CCI, "offset = 0x%X value = 0x%X",
			reg_offset, read_val);
	}

	/* CCI GPIO Queue registers */
	CAM_INFO(CAM_CCI, " **** CCI%d_GPIO_Q%d Registers ****",
		cci_dev->soc_info.index, queue);

	for (i = 0; i < DEBUG_GPIO_QUEUE_REG_COUNT; i++) {
		reg_offset = DEBUG_GPIO_QUEUE_REG_START +
			queue*0x100 + i * 4;
		read_val = cam_io_r_mb(base + reg_offset);
		CAM_INFO(CAM_CCI, "offset = 0x%X value = 0x%X",
			reg_offset, read_val);
	}

	/* CCI Interrupt registers */
	CAM_INFO(CAM_CCI, " ****CCI Interrupt Registers****");
	for (i = 0; i < DEBUG_INTR_REG_COUNT; i++) {
		reg_offset = DEBUG_INTR_REG_START + i * 4;
		read_val = cam_io_r_mb(base + reg_offset);
		CAM_INFO(CAM_CCI, "offset = 0x%X value = 0x%X",
			reg_offset, read_val);
	}
}
EXPORT_SYMBOL(cam_cci_dump_registers);

static uint32_t cam_cci_retry(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	int32_t rc = 0;
	uint32_t retry = 50;
	uint32_t read_val0 = 0, read_val1 = 0;
	uint32_t reg_offset = master * 0x200 + queue * 0x100;
	struct cam_cci_master_info *cci_master_info = NULL;
	struct cam_hw_soc_info *soc_info = &cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;

	cci_master_info = &cci_dev->cci_master_info[master];

	while (retry && (cci_master_info->status == 0)) {
		read_val0 = cam_io_r_mb(base +
			CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR + reg_offset);
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d_CUR_WORD_CNT_ADDR %d",
			soc_info->index, master, queue, read_val0);

		if (read_val0) {
			usleep_range(1000, 1010);
			read_val1 = cam_io_r_mb(base +
				CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR + reg_offset);
			CAM_DBG(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d_CUR_WORD_CNT_ADDR %d",
				soc_info->index, master, queue, read_val1);

			if (read_val1 == 0)
				/* No pending cmd in the CCI */
				break;
			else if (read_val0 == read_val1) {
				/* queue is stable now */
				CAM_DBG(CAM_CCI,
					"CCI%d_I2C_M%d_Q%d_EXEC_WORD_CNT_ADDR %d",
					soc_info->index, master,
					queue, read_val0);
				cam_io_w_mb(read_val0, base +
					CCI_I2C_M0_Q0_EXEC_WORD_CNT_ADDR +
					reg_offset);

				cam_io_w_mb(1 << ((master * 2) + queue),
					base + CCI_QUEUE_START_ADDR);

				CAM_INFO(CAM_CCI,
					"CCI%d_I2C_M%d_Q%d restart the queue",
					soc_info->index, master, queue);

				rc = wait_for_completion_timeout(
					&cci_master_info->report_q[queue],
					CCI_TIMEOUT);

				if (rc <= 0) {
					rc = -ETIMEDOUT;
					cam_cci_flush_queue(cci_dev, master);
					if (cci_dev->cci_gpio_info.is_initilized)
						cam_cci_gpio_flush_queue(cci_dev, queue);
					break;
				}
			} else {
				retry--;
				CAM_INFO(CAM_CCI,
					"CCI%d_I2C_M%d_Q%d_CURR_WORD_CNT isn't stable retry:%d",
					soc_info->index, master, queue, retry);
				continue;
			}
		} else {
			CAM_INFO(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d_CURR_WORD_CNT is 0, treat it as normal",
				soc_info->index, master, queue);
			break;
		}
	}

	if (retry == 0) {
		rc = -ETIMEDOUT;
		cam_cci_flush_queue(cci_dev, master);
		if (cci_dev->cci_gpio_info.is_initilized)
			cam_cci_gpio_flush_queue(cci_dev, queue);
	}

	return rc;
}

static uint32_t cam_cci_wait(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	int32_t rc = 0;
	struct cam_hw_soc_info *soc_info = NULL;

	if (!cci_dev) {
		CAM_ERR(CAM_CCI, "cci_dev pointer is NULL");
		return -EINVAL;
	}
	soc_info = &cci_dev->soc_info;


	rc = wait_for_completion_timeout(
		&cci_dev->cci_master_info[master].report_q[queue], CCI_TIMEOUT);
	CAM_DBG(CAM_CCI, "wait DONE_for_completion_timeout, rc=%d", rc);

	if (rc <= 0) {
#ifdef DUMP_CCI_REGISTERS
		cam_cci_dump_registers(cci_dev, master, queue);
#endif
		CAM_ERR(CAM_CCI, "wait for queue: %d", queue);
		if (rc == 0) {
			rc = cam_cci_retry(cci_dev, master, queue);
			if (!rc)
				CAM_INFO(CAM_CCI,
					"CCI%d_I2C_M%d_Q%d retry success",
					soc_info->index, master, queue);
			else
				return rc;
		}
	}

	rc = cci_dev->cci_master_info[master].status;
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q% is in error state",
			cci_dev->soc_info.index, master, queue);
		cci_dev->cci_master_info[master].status = 0;
		return rc;
	}

	return 0;
}

static void cam_cci_gpio_load_report_cmd(struct cci_device *cci_dev,
	enum cci_gpio_queue_t queue)
{
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;

	uint32_t reg_offset = queue * 0x100;
	uint32_t read_val = cam_io_r_mb(base +
		CCI_GPIO_Q0_CUR_WORD_CNT_ADDR + reg_offset);
	uint32_t report_id =
		cci_dev->cci_gpio_queue_info[queue].report_id;
	uint32_t report_val = CCI_GPIO_REPORT_CMD | (1 << 8) |
		(1 << 9) | (report_id << 4);

	CAM_DBG(CAM_CCI,
		"CCI%d_GPIO_Q%d_REPORT_CMD curr_w_cnt: %d report_id %d",
		cci_dev->soc_info.index, queue, read_val, report_id);
	cam_io_w_mb(report_val,
		base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset);
	read_val++;

	cci_dev->cci_gpio_queue_info[queue].report_id++;
	if (cci_dev->cci_gpio_queue_info[queue].report_id == REPORT_IDSIZE)
		cci_dev->cci_gpio_queue_info[queue].report_id = 0;

	CAM_DBG(CAM_CCI, "CCI%d_GPIO_Q%d_EXEC_WORD_CNT_ADDR %d",
		cci_dev->soc_info.index, queue, read_val);
	cam_io_w_mb(read_val, base +
		CCI_GPIO_Q0_EXEC_WORD_CNT_ADDR + reg_offset);
}

static void cam_cci_load_report_cmd(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;

	uint32_t reg_offset = master * 0x200 + queue * 0x100;
	uint32_t read_val = cam_io_r_mb(base +
		CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR + reg_offset);
	uint32_t report_id =
		cci_dev->cci_i2c_queue_info[master][queue].report_id;
	uint32_t report_val = CCI_I2C_REPORT_CMD | (1 << 8) |
		(1 << 9) | (report_id << 4);

	CAM_DBG(CAM_CCI,
		"CCI%d_I2C_M%d_Q%d_REPORT_CMD curr_w_cnt: %d report_id %d",
		cci_dev->soc_info.index, master, queue, read_val, report_id);
	cam_io_w_mb(report_val,
		base + CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
		reg_offset);
	read_val++;

	cci_dev->cci_i2c_queue_info[master][queue].report_id++;
	if (cci_dev->cci_i2c_queue_info[master][queue].report_id == REPORT_IDSIZE)
		cci_dev->cci_i2c_queue_info[master][queue].report_id = 0;

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d_EXEC_WORD_CNT_ADDR %d",
		cci_dev->soc_info.index, master, queue, read_val);
	cam_io_w_mb(read_val, base +
		CCI_I2C_M0_Q0_EXEC_WORD_CNT_ADDR + reg_offset);
}

static int32_t cam_cci_gpio_queue_execute_cmd(struct cci_device *cci_dev,
	enum cci_gpio_queue_t queue)
{
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;

	uint32_t reg_val = 1 << (4 + queue);

	cam_io_w_mb(reg_val, base + CCI_QUEUE_START_ADDR);
	cci_dev->cci_gpio_info.status = 0;

	return 0;
}

static int32_t cam_cci_wait_report_cmd(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	unsigned long flags;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;

	uint32_t reg_val = 1 << ((master * 2) + queue);

	cam_cci_load_report_cmd(cci_dev, master, queue);
	spin_lock_irqsave(
		&cci_dev->cci_master_info[master].lock_q[queue], flags);
	atomic_set(&cci_dev->cci_master_info[master].q_free[queue], 1);
	atomic_set(&cci_dev->cci_master_info[master].done_pending[queue], 1);
	spin_unlock_irqrestore(
		&cci_dev->cci_master_info[master].lock_q[queue], flags);
	cam_io_w_mb(reg_val, base + CCI_QUEUE_START_ADDR);

	return cam_cci_wait(cci_dev, master, queue);
}

static int32_t cam_cci_transfer_end(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	int32_t rc = 0;
	unsigned long flags;

	spin_lock_irqsave(
		&cci_dev->cci_master_info[master].lock_q[queue], flags);
	if (atomic_read(&cci_dev->cci_master_info[master].q_free[queue]) == 0) {
		spin_unlock_irqrestore(
			&cci_dev->cci_master_info[master].lock_q[queue], flags);
		rc = cam_cci_lock_queue(cci_dev, master, queue, 0);
		if (rc < 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Failed to lock for rc: %d",
				cci_dev->soc_info.index, master, queue, rc);
			return rc;
		}
		rc = cam_cci_wait_report_cmd(cci_dev, master, queue);
		if (rc < 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Failed for wait_report_cmd for rc: %d",
				cci_dev->soc_info.index, master, queue, rc);
			return rc;
		}
	} else {
		atomic_set(
			&cci_dev->cci_master_info[master].done_pending[queue],
			1);
		spin_unlock_irqrestore(
			&cci_dev->cci_master_info[master].lock_q[queue], flags);
		rc = cam_cci_wait(cci_dev, master, queue);
		if (rc < 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Failed with cci_wait for rc: %d",
				cci_dev->soc_info.index, master, queue, rc);
			return rc;
		}
		rc = cam_cci_lock_queue(cci_dev, master, queue, 0);
		if (rc < 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Failed to lock_queue for rc: %d",
				cci_dev->soc_info.index, master, queue, rc);
			return rc;
		}
		rc = cam_cci_wait_report_cmd(cci_dev, master, queue);
		if (rc < 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Failed in wait_report_cmd for rc: %d",
				cci_dev->soc_info.index, master, queue, rc);
			return rc;
		}
	}

	return rc;
}

static int32_t cam_cci_get_queue_free_size(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	uint32_t read_val = 0;
	uint32_t reg_offset = master * 0x200 + queue * 0x100;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;

	read_val = cam_io_r_mb(base +
		CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR + reg_offset);
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d_CUR_WORD_CNT_ADDR %d max %d",
		cci_dev->soc_info.index, master, queue, read_val,
		cci_dev->cci_i2c_queue_info[master][queue].max_queue_size);
	return ((cci_dev->cci_i2c_queue_info[master][queue].max_queue_size) -
			read_val);
}

static void cam_cci_process_half_q(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	unsigned long flags;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;
	uint32_t reg_val = 1 << ((master * 2) + queue);

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d ENTER", cci_dev->soc_info.index, master, queue);

	spin_lock_irqsave(&cci_dev->cci_master_info[master].lock_q[queue],
		flags);
	if (atomic_read(&cci_dev->cci_master_info[master].q_free[queue]) == 0) {
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d is free", cci_dev->soc_info.index, master, queue);
		cam_cci_load_report_cmd(cci_dev, master, queue);
		atomic_set(&cci_dev->cci_master_info[master].q_free[queue], 1);
		cam_io_w_mb(reg_val, base +
			CCI_QUEUE_START_ADDR);
	}
	spin_unlock_irqrestore(&cci_dev->cci_master_info[master].lock_q[queue],
		flags);
}

static int32_t cam_cci_process_full_q(struct cci_device *cci_dev,
	enum cci_i2c_master_t master,
	enum cci_i2c_queue_t queue)
{
	int32_t rc = 0;
	unsigned long flags;

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d ENTER", cci_dev->soc_info.index, master, queue);
	spin_lock_irqsave(&cci_dev->cci_master_info[master].lock_q[queue],
		flags);
	if (atomic_read(&cci_dev->cci_master_info[master].q_free[queue]) == 1) {
		atomic_set(
			&cci_dev->cci_master_info[master].done_pending[queue],
			1);
		spin_unlock_irqrestore(
			&cci_dev->cci_master_info[master].lock_q[queue], flags);
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d is set to 1", cci_dev->soc_info.index, master, queue);
		rc = cam_cci_wait(cci_dev, master, queue);
		if (rc < 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d cci_wait failed for rc: %d",
				cci_dev->soc_info.index, master, queue, rc);
			return rc;
		}
	} else {
		spin_unlock_irqrestore(
			&cci_dev->cci_master_info[master].lock_q[queue], flags);
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d is set to 0", cci_dev->soc_info.index, master, queue);
		rc = cam_cci_wait_report_cmd(cci_dev, master, queue);
		if (rc < 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Failed in wait_report for rc: %d",
				cci_dev->soc_info.index, master, queue, rc);
			return rc;
		}
	}

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d EXIT", cci_dev->soc_info.index, master, queue);

	return rc;
}

static int32_t cam_cci_calc_cmd_len(struct cci_device *cci_dev,
	struct cam_cci_ctrl *c_ctrl, uint32_t cmd_size,
	 struct cam_sensor_i2c_reg_array *i2c_cmd, uint32_t *pack)
{
	uint8_t i;
	uint32_t len = 0;
	uint8_t data_len = 0, addr_len = 0;
	uint8_t pack_max_len;
	struct cam_sensor_i2c_reg_setting *msg;
	struct cam_sensor_i2c_reg_array *cmd = i2c_cmd;
	uint32_t size = cmd_size;

	if (!cci_dev || !c_ctrl) {
		CAM_ERR(CAM_CCI, "Invalid arguments cci_dev:%p, c_ctrl:%p",
			cci_dev, c_ctrl);
		return -EINVAL;
	}

	msg = &c_ctrl->cfg.cci_i2c_write_cfg;
	*pack = 0;

	if (c_ctrl->cmd == MSM_CCI_I2C_WRITE_SEQ ||
		c_ctrl->cmd == MSM_CCI_I2C_WRITE_BURST) {
		addr_len = cam_cci_convert_type_to_num_bytes(msg->addr_type);
		len = (size + addr_len) <= (cci_dev->payload_size) ?
			(size + addr_len):cci_dev->payload_size;
	} else {
		addr_len = cam_cci_convert_type_to_num_bytes(msg->addr_type);
		data_len = cam_cci_convert_type_to_num_bytes(msg->data_type);
		len = data_len + addr_len;
		pack_max_len = size < (cci_dev->payload_size-len) ?
			size : (cci_dev->payload_size-len);
		for (i = 0; i < pack_max_len;) {
			if (cmd->delay || ((cmd - i2c_cmd) >= (cmd_size - 1)))
				break;
			if (cmd->reg_addr + 1 ==
				(cmd+1)->reg_addr) {
				len += data_len;
				if (len > cci_dev->payload_size) {
					len = len - data_len;
					break;
				}
				(*pack)++;
			} else {
				break;
			}
			i += data_len;
			cmd++;
		}
	}

	if (len > cci_dev->payload_size) {
		CAM_ERR(CAM_CCI, "Len error: len: %u expected_len: %u",
			len, cci_dev->payload_size);
		return -EINVAL;
	}

	len += 1; /*add i2c WR command*/
	len = len/4 + 1;

	return len;
}

static uint32_t cam_cci_cycles_per_ms(unsigned long clk)
{
	uint32_t cycles_per_us;

	if (clk) {
		cycles_per_us = ((clk/1000)*256)/1000;
	} else {
		CAM_ERR(CAM_CCI, "Failed: Can use default: %d",
			CYCLES_PER_MICRO_SEC_DEFAULT);
		cycles_per_us = CYCLES_PER_MICRO_SEC_DEFAULT;
	}

	return cycles_per_us;
}

void cam_cci_get_clk_rates(struct cci_device *cci_dev,
	struct cam_cci_ctrl *c_ctrl)

{
	int32_t src_clk_idx, j;
	uint32_t cci_clk_src;
	unsigned long clk;
	struct cam_cci_clk_params_t *clk_params = NULL;

	enum i2c_freq_mode i2c_freq_mode = c_ctrl->cci_info->i2c_freq_mode;
	struct cam_hw_soc_info *soc_info = &cci_dev->soc_info;

	if (i2c_freq_mode >= I2C_MAX_MODES ||
		i2c_freq_mode < I2C_STANDARD_MODE) {
		CAM_ERR(CAM_CCI, "Invalid frequency mode: %d",
			(int32_t)i2c_freq_mode);
		cci_dev->clk_level_index = -1;
		return;
	}

	clk_params = &cci_dev->cci_clk_params[i2c_freq_mode];
	cci_clk_src = clk_params->cci_clk_src;

	src_clk_idx = soc_info->src_clk_idx;

	if (src_clk_idx < 0) {
		cci_dev->cycles_per_us = CYCLES_PER_MICRO_SEC_DEFAULT;
		cci_dev->clk_level_index = 0;
		return;
	}

	if (cci_clk_src == 0) {
		clk = soc_info->clk_rate[0][src_clk_idx];
		cci_dev->cycles_per_us = cam_cci_cycles_per_ms(clk);
		cci_dev->clk_level_index = 0;
		return;
	}

	for (j = 0; j < CAM_MAX_VOTE; j++) {
		clk = soc_info->clk_rate[j][src_clk_idx];
		if (clk == cci_clk_src) {
			cci_dev->cycles_per_us = cam_cci_cycles_per_ms(clk);
			cci_dev->clk_level_index = j;
			return;
		}
	}
}

static int32_t cam_cci_set_clk_param(struct cci_device *cci_dev,
	struct cam_cci_ctrl *c_ctrl)
{
	struct cam_cci_clk_params_t *clk_params = NULL;
	enum cci_i2c_master_t master = c_ctrl->cci_info->cci_i2c_master;
	enum i2c_freq_mode i2c_freq_mode = c_ctrl->cci_info->i2c_freq_mode;
	void __iomem *base = cci_dev->soc_info.reg_map[0].mem_base;
	struct cam_cci_master_info *cci_master =
		&cci_dev->cci_master_info[master];

	if ((i2c_freq_mode >= I2C_MAX_MODES) || (i2c_freq_mode < 0)) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d invalid i2c_freq_mode = %d",
			cci_dev->soc_info.index, master, i2c_freq_mode);
		return -EINVAL;
	}
	/*
	 * If no change in i2c freq, then acquire semaphore only for the first
	 * i2c transaction to indicate I2C transaction is in progress, else
	 * always try to acquire semaphore, to make sure that no other I2C
	 * transaction is in progress.
	 */
	mutex_lock(&cci_master->mutex);
	if (i2c_freq_mode == cci_dev->i2c_freq_mode[master]) {
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d, curr_freq: %d", cci_dev->soc_info.index, master,
			i2c_freq_mode);
		mutex_lock(&cci_master->freq_cnt_lock);
		if (cci_master->freq_ref_cnt == 0)
			down(&cci_master->master_sem);
		cci_master->freq_ref_cnt++;
		mutex_unlock(&cci_master->freq_cnt_lock);
		mutex_unlock(&cci_master->mutex);
		return 0;
	}
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d, curr_freq: %d, req_freq: %d",
		cci_dev->soc_info.index, master, cci_dev->i2c_freq_mode[master], i2c_freq_mode);
	down(&cci_master->master_sem);

	mutex_lock(&cci_master->freq_cnt_lock);
	cci_master->freq_ref_cnt++;
	mutex_unlock(&cci_master->freq_cnt_lock);

	clk_params = &cci_dev->cci_clk_params[i2c_freq_mode];

	if (master == MASTER_0) {
		cam_io_w_mb(clk_params->hw_thigh << 16 |
			clk_params->hw_tlow,
			base + CCI_I2C_M0_SCL_CTL_ADDR);
		cam_io_w_mb(clk_params->hw_tsu_sto << 16 |
			clk_params->hw_tsu_sta,
			base + CCI_I2C_M0_SDA_CTL_0_ADDR);
		cam_io_w_mb(clk_params->hw_thd_dat << 16 |
			clk_params->hw_thd_sta,
			base + CCI_I2C_M0_SDA_CTL_1_ADDR);
		cam_io_w_mb(clk_params->hw_tbuf,
			base + CCI_I2C_M0_SDA_CTL_2_ADDR);
		cam_io_w_mb(clk_params->hw_scl_stretch_en << 8 |
			clk_params->hw_trdhld << 4 | clk_params->hw_tsp,
			base + CCI_I2C_M0_MISC_CTL_ADDR);
	} else if (master == MASTER_1) {
		cam_io_w_mb(clk_params->hw_thigh << 16 |
			clk_params->hw_tlow,
			base + CCI_I2C_M1_SCL_CTL_ADDR);
		cam_io_w_mb(clk_params->hw_tsu_sto << 16 |
			clk_params->hw_tsu_sta,
			base + CCI_I2C_M1_SDA_CTL_0_ADDR);
		cam_io_w_mb(clk_params->hw_thd_dat << 16 |
			clk_params->hw_thd_sta,
			base + CCI_I2C_M1_SDA_CTL_1_ADDR);
		cam_io_w_mb(clk_params->hw_tbuf,
			base + CCI_I2C_M1_SDA_CTL_2_ADDR);
		cam_io_w_mb(clk_params->hw_scl_stretch_en << 8 |
			clk_params->hw_trdhld << 4 | clk_params->hw_tsp,
			base + CCI_I2C_M1_MISC_CTL_ADDR);
	}
	cci_dev->i2c_freq_mode[master] = i2c_freq_mode;

	mutex_unlock(&cci_master->mutex);
	return 0;
}

static int32_t cam_cci_gpio_data_queue(struct cci_device *cci_dev,
	struct cam_cci_ctrl *c_ctrl, enum cci_gpio_queue_t queue,
	enum cci_i2c_sync sync_en,
	uint8_t trig_i2c_queue)
{
	int32_t rc = 0;
	uint32_t reg_offset, val;
	uint32_t max_queue_size, queue_size = 0;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;
	unsigned long flags;
	uint32_t gpio_mask = c_ctrl->cfg.gpio_cfg.reg_setting->gpio_mask;
	uint64_t qtimer_timestamp = c_ctrl->cfg.gpio_cfg.reg_setting->timestamp;
	uint32_t pulse_width = c_ctrl->cfg.gpio_cfg.reg_setting->pulse_width;

	CAM_DBG(CAM_CCI, "delay_cycle: %llu gpio_mask %d pulse_width %d",
			qtimer_timestamp, gpio_mask, pulse_width);

	reg_offset = queue * 0x100;

	val = CCI_GPIO_SET_PARAM_CMD |  c_ctrl->cci_info->id_map << 4;

	CAM_DBG(CAM_CCI, "CCI%d_GPIO_Q%d_LOAD_DATA_ADDR:val 0x%x:0x%x",
		cci_dev->soc_info.index, queue, CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset, val);
	cam_io_w_mb(val, base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset);

	spin_lock_irqsave(&cci_dev->cci_gpio_info.lock_q[queue],
		flags);
	spin_unlock_irqrestore(&cci_dev->cci_gpio_info.lock_q[queue],
		flags);

	if (!(c_ctrl->cfg.gpio_cfg.reg_setting->is_nop)) {
		val = CCI_GPIO_WAIT_I2C_Q_TRIG_EVENT_CMD | trig_i2c_queue << 4;
		cam_io_w_mb(val, base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
			reg_offset);
	}

	val = (uint32_t) (CCI_GPIO_WAIT_QTIMER_CMD | 1 << 4 | qtimer_timestamp << 8);
	cam_io_w_mb(val, base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset);

	val = (uint32_t) ((qtimer_timestamp & (0xFFFFFFFFFF000000)) >> 24);
	cam_io_w_mb(val, base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset);

	cam_cci_gpio_load_report_cmd(cci_dev, queue);

	val = CCI_GPIO_OUT_CMD | 1 << (4 + gpio_mask) |
		1 << (12 + gpio_mask);
	cam_io_w_mb(val, base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset);

	val = CCI_GPIO_WAIT_CMD | (cci_dev->cycles_per_us/256) * pulse_width << 4;
	cam_io_w_mb(val, base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset);

	val = CCI_GPIO_OUT_CMD | 1 << (4 + gpio_mask);
	cam_io_w_mb(val, base + CCI_GPIO_Q0_LOAD_DATA_ADDR +
		reg_offset);

	max_queue_size =
		cci_dev->cci_gpio_queue_info[queue].max_queue_size;

	queue_size = cam_io_r_mb(base +
		CCI_GPIO_Q0_CUR_WORD_CNT_ADDR + reg_offset);
	CAM_DBG(CAM_CCI, "CCI%d_GPIO_Q%d CUR_WORD_CNT_ADDR %d max %d",
		cci_dev->soc_info.index, queue, queue_size, max_queue_size);

	cam_io_w_mb(queue_size, base +
		CCI_GPIO_Q0_EXEC_WORD_CNT_ADDR + reg_offset);

	if (queue_size < max_queue_size) {
		rc = cam_cci_gpio_queue_execute_cmd(cci_dev, queue);
	} else {
		CAM_ERR(CAM_CCI, "CCI%d_GPIO_Q%d queue full, current queue size %d max %d",
			cci_dev->soc_info.index, queue, queue_size, max_queue_size);
		rc = -EINVAL;
	}

	return rc;
}

static int32_t cam_cci_data_queue(struct cci_device *cci_dev,
	struct cam_cci_ctrl *c_ctrl, enum cci_i2c_queue_t queue,
	enum cci_i2c_sync sync_en)
{
	uint16_t i = 0, j = 0, k = 0, h = 0, len = 0;
	int32_t rc = 0, free_size = 0, en_seq_write = 0;
	uint8_t data[12];
	struct cam_sensor_i2c_reg_setting *i2c_msg = &c_ctrl->cfg.cci_i2c_write_cfg;
	struct cam_sensor_i2c_reg_array *i2c_cmd = i2c_msg->reg_setting;
	enum cci_i2c_master_t master = c_ctrl->cci_info->cci_i2c_master;
	uint16_t reg_addr = 0, cmd_size = i2c_msg->size;
	uint32_t read_val = 0, reg_offset, val, delay = 0;
	uint32_t max_queue_size, queue_size = 0, cmd = 0;
	struct cam_hw_soc_info *soc_info =
		&cci_dev->soc_info;
	void __iomem *base = soc_info->reg_map[0].mem_base;
	unsigned long flags;
	struct cci_trigger_data  *get_ctx = cam_cci_get_ctx(cci_dev, c_ctrl->cci_info->id_map);

	if (i2c_cmd == NULL) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Failed: i2c cmd is NULL",
			cci_dev->soc_info.index, master, queue);
		return -EINVAL;
	}

	if ((!cmd_size) || (cmd_size > CCI_I2C_MAX_WRITE)) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d failed: invalid cmd_size %d",
			cci_dev->soc_info.index, master, queue, cmd_size);
		return -EINVAL;
	}

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d addr type %d data type %d cmd_size %d",
		cci_dev->soc_info.index, master, queue, i2c_msg->addr_type, i2c_msg->data_type, cmd_size);

	if (i2c_msg->addr_type >= CAMERA_SENSOR_I2C_TYPE_MAX) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d failed: invalid addr_type 0x%X",
			cci_dev->soc_info.index, master, queue, i2c_msg->addr_type);
		return -EINVAL;
	}
	if (i2c_msg->data_type >= CAMERA_SENSOR_I2C_TYPE_MAX) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d failed: invalid data_type 0x%X",
			cci_dev->soc_info.index, master, queue, i2c_msg->data_type);
		return -EINVAL;
	}
	reg_offset = master * 0x200 + queue * 0x100;

	cam_io_w_mb(cci_dev->cci_wait_sync_cfg.cid,
		base + CCI_SET_CID_SYNC_TIMER_ADDR +
		cci_dev->cci_wait_sync_cfg.csid *
		CCI_SET_CID_SYNC_TIMER_OFFSET);

	val = CCI_I2C_SET_PARAM_CMD | c_ctrl->cci_info->sid << 4 |
		c_ctrl->cci_info->retries << 16 |
		c_ctrl->cci_info->id_map << 18;

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d_LOAD_DATA_ADDR:val 0x%x:0x%x",
		cci_dev->soc_info.index, master, queue, CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
		reg_offset, val);
	cam_io_w_mb(val, base + CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
		reg_offset);

	spin_lock_irqsave(&cci_dev->cci_master_info[master].lock_q[queue],
		flags);
	atomic_set(&cci_dev->cci_master_info[master].q_free[queue], 0);
	spin_unlock_irqrestore(&cci_dev->cci_master_info[master].lock_q[queue],
		flags);

	max_queue_size =
		cci_dev->cci_i2c_queue_info[master][queue].max_queue_size;

	if ((c_ctrl->cmd == MSM_CCI_I2C_WRITE_SEQ) ||
		(c_ctrl->cmd == MSM_CCI_I2C_WRITE_BURST))
		queue_size = max_queue_size;
	else
		queue_size = max_queue_size / 2;
	reg_addr = i2c_cmd->reg_addr;

	if (sync_en == MSM_SYNC_ENABLE && cci_dev->valid_sync &&
		cmd_size < max_queue_size) {
		val = CCI_I2C_WAIT_SYNC_CMD |
			((cci_dev->cci_wait_sync_cfg.line) << 4);
		cam_io_w_mb(val,
			base + CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
			reg_offset);
	}

	rc = cam_cci_lock_queue(cci_dev, master, queue, 1);
	if (rc < 0) {
		CAM_ERR(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to lock_queue for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		return rc;
	}

	while (cmd_size) {
		uint32_t pack = 0;

		len = cam_cci_calc_cmd_len(cci_dev, c_ctrl, cmd_size,
			i2c_cmd, &pack);
		if (len <= 0) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Calculate command len failed, len: %d",
				cci_dev->soc_info.index, master, queue, len);
			return -EINVAL;
		}

		read_val = cam_io_r_mb(base +
			CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR + reg_offset);
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d CUR_WORD_CNT_ADDR %d len %d max %d",
			cci_dev->soc_info.index, master, queue, read_val, len, max_queue_size);
		/* + 1 - space alocation for Report CMD */
		if ((read_val + len + 1) > queue_size) {
			if ((read_val + len + 1) > max_queue_size) {
				rc = cam_cci_process_full_q(cci_dev,
					master, queue);
				if (rc < 0) {
					CAM_ERR(CAM_CCI,
						"CCI%d_I2C_M%d_Q%d Failed to process full queue rc: %d",
						cci_dev->soc_info.index, master, queue, rc);
					return rc;
				}
				continue;
			}
			cam_cci_process_half_q(cci_dev, master, queue);
		}

		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d cmd_size %d addr 0x%x data 0x%x",
			cci_dev->soc_info.index, master, queue, cmd_size, i2c_cmd->reg_addr, i2c_cmd->reg_data);
		delay = i2c_cmd->delay;
		i = 0;
		data[i++] = CCI_I2C_WRITE_CMD;

		/*
		 * in case of multiple command
		 * MSM_CCI_I2C_WRITE : address is not continuous, so update
		 *	address for a new packet.
		 * MSM_CCI_I2C_WRITE_SEQ : address is continuous, need to keep
		 *	the incremented address for a
		 *	new packet
		 */
		if (c_ctrl->cmd == MSM_CCI_I2C_WRITE ||
			c_ctrl->cmd == MSM_CCI_I2C_WRITE_ASYNC ||
			c_ctrl->cmd == MSM_CCI_I2C_WRITE_SYNC ||
			c_ctrl->cmd == MSM_CCI_I2C_WRITE_SYNC_BLOCK)
			reg_addr = i2c_cmd->reg_addr;

		if (en_seq_write == 0) {
			/* either byte or word addr */
			if (i2c_msg->addr_type == CAMERA_SENSOR_I2C_TYPE_BYTE)
				data[i++] = reg_addr;
			else {
				data[i++] = (reg_addr & 0xFF00) >> 8;
				data[i++] = reg_addr & 0x00FF;
			}
		}
		/* max of 10 data bytes */
		do {
			if (i2c_msg->data_type == CAMERA_SENSOR_I2C_TYPE_BYTE) {
				data[i++] = i2c_cmd->reg_data;
				if (c_ctrl->cmd == MSM_CCI_I2C_WRITE_SEQ)
					reg_addr++;
			} else {
				if ((i + 1) <= cci_dev->payload_size) {
					switch (i2c_msg->data_type) {
					case CAMERA_SENSOR_I2C_TYPE_DWORD:
						data[i++] = (i2c_cmd->reg_data &
							0xFF000000) >> 24;
						/* fallthrough */
						fallthrough;
					case CAMERA_SENSOR_I2C_TYPE_3B:
						data[i++] = (i2c_cmd->reg_data &
							0x00FF0000) >> 16;
						/* fallthrough */
						fallthrough;
					case CAMERA_SENSOR_I2C_TYPE_WORD:
						data[i++] = (i2c_cmd->reg_data &
							0x0000FF00) >> 8;
						/* fallthrough */
						fallthrough;
					case CAMERA_SENSOR_I2C_TYPE_BYTE:
						data[i++] = i2c_cmd->reg_data &
							0x000000FF;
						break;
					default:
						CAM_ERR(CAM_CCI,
							"CCI%d_I2C_M%d_Q%d invalid data type: %d",
							cci_dev->soc_info.index, master, queue, i2c_msg->data_type);
						return -EINVAL;
					}

					if (c_ctrl->cmd ==
						MSM_CCI_I2C_WRITE_SEQ)
						reg_addr++;
				} else
					break;
			}
			CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d cmd_size %d addr 0x%x data 0x%x",
				cci_dev->soc_info.index, master, queue, cmd_size,
				i2c_cmd->reg_addr, i2c_cmd->reg_data);
			i2c_cmd++;
			--cmd_size;
		} while (((c_ctrl->cmd == MSM_CCI_I2C_WRITE_SEQ ||
			c_ctrl->cmd == MSM_CCI_I2C_WRITE_BURST) || pack--) &&
				(cmd_size > 0) && (i <= cci_dev->payload_size));
		free_size = cam_cci_get_queue_free_size(cci_dev, master,
				queue);
		if ((c_ctrl->cmd == MSM_CCI_I2C_WRITE_SEQ ||
			c_ctrl->cmd == MSM_CCI_I2C_WRITE_BURST) &&
			((i-1) == MSM_CCI_WRITE_DATA_PAYLOAD_SIZE_11) &&
			cci_dev->support_seq_write && cmd_size > 0 &&
			free_size > BURST_MIN_FREE_SIZE) {
			data[0] |= 0xF0;
			en_seq_write = 1;
		} else {
			data[0] |= ((i-1) << 4);
			en_seq_write = 0;
		}
		len = ((i-1)/4) + 1;

		read_val = cam_io_r_mb(base +
			CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR + reg_offset);
		for (h = 0, k = 0; h < len; h++) {
			cmd = 0;
			for (j = 0; (j < 4 && k < i); j++)
				cmd |= (data[k++] << (j * 8));
			CAM_DBG(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d LOAD_DATA_ADDR 0x%x, len:%d, cnt: %d",
				cci_dev->soc_info.index, master, queue, cmd, len, read_val);
			cam_io_w_mb(cmd, base +
				CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
				master * 0x200 + queue * 0x100);

			read_val += 1;

		}

		cam_io_w_mb(read_val, base +
			CCI_I2C_M0_Q0_EXEC_WORD_CNT_ADDR + reg_offset);

		if ((delay > 0) && (delay < CCI_MAX_DELAY) &&
			en_seq_write == 0) {
			cmd = (uint32_t)((delay * cci_dev->cycles_per_us) /
				0x100);
			cmd <<= 4;
			cmd |= CCI_I2C_WAIT_CMD;
			CAM_DBG(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d_LOAD_DATA_ADDR 0x%x",
				cci_dev->soc_info.index, master, queue, cmd);
			cam_io_w_mb(cmd, base +
				CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
				master * 0x200 + queue * 0x100);
			read_val += 1;
			cam_io_w_mb(read_val, base +
				CCI_I2C_M0_Q0_EXEC_WORD_CNT_ADDR + reg_offset);
		}
	}

	if (get_ctx && get_ctx->is_trigger_mode) {
		CAM_DBG(CAM_CCI, "is_trigger_mode %d", get_ctx->is_trigger_mode);
		val = CCI_I2C_TRIG_I2C_EVENT_CMD;
		cam_io_w_mb(val,
			base + CCI_I2C_M0_Q0_LOAD_DATA_ADDR +
			reg_offset);
	}

	rc = cam_cci_transfer_end(cci_dev, master, queue);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Slave: 0x%x failed rc %d",
			cci_dev->soc_info.index, master, queue, (c_ctrl->cci_info->sid << 1), rc);
		return rc;
	}

	return rc;
}

static int32_t cam_cci_burst_read(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl)
{
	int32_t rc = 0;
	uint32_t val = 0, i = 0, j = 0, irq_mask_update = 0;
	unsigned long rem_jiffies, flags;
	int32_t read_words = 0, exp_words = 0;
	int32_t index = 0, first_byte = 0, total_read_words = 0;
	enum cci_i2c_master_t master;
	enum cci_i2c_queue_t queue = QUEUE_1;
	struct cci_device                  *cci_dev = NULL;
	struct cam_cci_read_cfg            *read_cfg = NULL;
	struct cam_hw_soc_info             *soc_info = NULL;
	void __iomem                       *base = NULL;

	cci_dev = v4l2_get_subdevdata(sd);
	master = c_ctrl->cci_info->cci_i2c_master;
	read_cfg = &c_ctrl->cfg.cci_i2c_read_cfg;

	if (c_ctrl->cci_info->cci_i2c_master >= MASTER_MAX
		|| c_ctrl->cci_info->cci_i2c_master < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Invalid I2C master addr",
			cci_dev->soc_info.index, master, queue);
		return -EINVAL;
	}

	/* Set the I2C Frequency */
	rc = cam_cci_set_clk_param(cci_dev, c_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d cam_cci_set_clk_param failed rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		return rc;
	}

	mutex_lock(&cci_dev->cci_master_info[master].mutex_q[queue]);
	cci_dev->is_burst_read[master] = true;
	reinit_completion(&cci_dev->cci_master_info[master].report_q[queue]);

	soc_info = &cci_dev->soc_info;
	base = soc_info->reg_map[0].mem_base;

	/*
	 * Call validate queue to make sure queue is empty before starting.
	 * If this call fails, don't proceed with i2c_read call. This is to
	 * avoid overflow / underflow of queue
	 */
	rc = cam_cci_validate_queue(cci_dev,
		cci_dev->cci_i2c_queue_info[master][queue].max_queue_size - 1,
		master, queue);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Initial validataion failed rc:%d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	if (c_ctrl->cci_info->retries > CCI_I2C_READ_MAX_RETRIES) {
		CAM_ERR(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Invalid read retries info retries from slave: %d, max retries: %d",
			cci_dev->soc_info.index, master, queue, c_ctrl->cci_info->retries, CCI_I2C_READ_MAX_RETRIES);
		goto rel_mutex_q;
	}

	if (read_cfg->data == NULL) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Data ptr is NULL",
			cci_dev->soc_info.index, master, queue);
		goto rel_mutex_q;
	}

	if (read_cfg->addr_type >= CAMERA_SENSOR_I2C_TYPE_MAX) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d failed : Invalid addr type: %u",
			cci_dev->soc_info.index, master, queue, read_cfg->addr_type);
		rc = -EINVAL;
		goto rel_mutex_q;
	}

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d set param sid 0x%x retries %d id_map %d",
		cci_dev->soc_info.index, master, queue, c_ctrl->cci_info->sid, c_ctrl->cci_info->retries,
		c_ctrl->cci_info->id_map);
	val = CCI_I2C_SET_PARAM_CMD | c_ctrl->cci_info->sid << 4 |
		c_ctrl->cci_info->retries << 16 |
		c_ctrl->cci_info->id_map << 18;
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write param_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = CCI_I2C_LOCK_CMD;
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d failed to write lock_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = CCI_I2C_WRITE_DISABLE_P_CMD | (read_cfg->addr_type << 4);
	for (i = 0; i < read_cfg->addr_type; i++) {
		val |= ((read_cfg->addr >> (i << 3)) & 0xFF)  <<
		((read_cfg->addr_type - i) << 3);
	}

	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write disable cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = CCI_I2C_READ_CMD | (read_cfg->num_byte << 4);
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write read_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = CCI_I2C_UNLOCK_CMD;
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write unlock_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = cam_io_r_mb(base + CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR
			+ master * 0x200 + queue * 0x100);
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d cur word cnt 0x%x",
		cci_dev->soc_info.index, master, queue, val);
	cam_io_w_mb(val, base + CCI_I2C_M0_Q0_EXEC_WORD_CNT_ADDR
			+ master * 0x200 + queue * 0x100);

	val = 1 << ((master * 2) + queue);
	cam_io_w_mb(val, base + CCI_QUEUE_START_ADDR);

	exp_words = ((read_cfg->num_byte / 4) + 1);
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d waiting for threshold [exp_words %d]",
		cci_dev->soc_info.index, master, queue, exp_words);

	while (total_read_words != exp_words) {
		rem_jiffies = cam_common_wait_for_completion_timeout(
			&cci_dev->cci_master_info[master].th_complete,
			CCI_TIMEOUT);
		if (!rem_jiffies) {
			rc = -ETIMEDOUT;
			val = cam_io_r_mb(base +
				CCI_I2C_M0_READ_BUF_LEVEL_ADDR +
				master * 0x100);
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d wait timeout for th_complete, FIFO buf_lvl:0x%x, rc: %d",
				cci_dev->soc_info.index, master, queue, val, rc);
			cam_cci_dump_registers(cci_dev, master, queue);

			cam_cci_flush_queue(cci_dev, master);
			goto rel_mutex_q;
		}

		if (cci_dev->cci_master_info[master].status) {
			CAM_ERR(CAM_CCI,
				"CCI%d_I2C_M%d_Q%d Error with Slave: 0x%x",
				cci_dev->soc_info.index, master, queue, (c_ctrl->cci_info->sid << 1));
			rc = -EINVAL;
			cci_dev->cci_master_info[master].status = 0;
			goto rel_mutex_q;
		}

		read_words = cam_io_r_mb(base +
			CCI_I2C_M0_READ_BUF_LEVEL_ADDR + master * 0x100);
		if (read_words <= 0) {
			CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d FIFO Buffer lvl is 0",
				cci_dev->soc_info.index, master, queue);
			goto enable_irq;
		}

read_again:
		j++;
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d Iteration: %u read_words %d",
			cci_dev->soc_info.index, master, queue, j, read_words);

		total_read_words += read_words;
		while (read_words > 0) {
			val = cam_io_r_mb(base +
				CCI_I2C_M0_READ_DATA_ADDR + master * 0x100);
			for (i = 0; (i < 4) &&
				(index < read_cfg->num_byte); i++) {
				CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d i:%d index:%d",
					cci_dev->soc_info.index, master, queue, i, index);
				if (!first_byte) {
					CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d sid 0x%x",
						cci_dev->soc_info.index, master, queue, val & 0xFF);
					first_byte++;
				} else {
					read_cfg->data[index] =
						(val  >> (i * 8)) & 0xFF;
					CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d data[%d] 0x%x", 
						cci_dev->soc_info.index, master, queue, index, read_cfg->data[index]);
					index++;
				}
			}
			read_words--;
		}

		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d Iteration:%u total_read_words %d",
			cci_dev->soc_info.index, master, queue, j, total_read_words);

		read_words = cam_io_r_mb(base +
			CCI_I2C_M0_READ_BUF_LEVEL_ADDR + master * 0x100);
		if (read_words > 0) {
			CAM_DBG(CAM_CCI, "FIFO Buffer lvl is %d", read_words);
			goto read_again;
		}

enable_irq:
		spin_lock_irqsave(&cci_dev->lock_status, flags);
		if (cci_dev->irqs_disabled) {
			irq_mask_update =
				cam_io_r_mb(base + CCI_IRQ_MASK_1_ADDR);
			if (master == MASTER_0 && cci_dev->irqs_disabled &
				CCI_IRQ_STATUS_1_I2C_M0_RD_THRESHOLD)
				irq_mask_update |=
					CCI_IRQ_STATUS_1_I2C_M0_RD_THRESHOLD;
			else if (master == MASTER_1 && cci_dev->irqs_disabled &
				CCI_IRQ_STATUS_1_I2C_M1_RD_THRESHOLD)
				irq_mask_update |=
					CCI_IRQ_STATUS_1_I2C_M1_RD_THRESHOLD;
			cam_io_w_mb(irq_mask_update,
				base + CCI_IRQ_MASK_1_ADDR);
		}
		spin_unlock_irqrestore(&cci_dev->lock_status, flags);

		if (total_read_words == exp_words) {
		   /*
		    * This wait is for RD_DONE irq, if RD_DONE is
		    * triggered we will call complete on both threshold
		    * & read done waits. As part of the threshold wait
		    * we will be draining the entire buffer out. This
		    * wait is to compensate for the complete invoked for
		    * RD_DONE exclusively.
		    */
			rem_jiffies = cam_common_wait_for_completion_timeout(
			&cci_dev->cci_master_info[master].rd_done,
			CCI_TIMEOUT);
			if (!rem_jiffies) {
				rc = -ETIMEDOUT;
				val = cam_io_r_mb(base +
					CCI_I2C_M0_READ_BUF_LEVEL_ADDR +
					master * 0x100);
				CAM_ERR(CAM_CCI,
					"CCI%d_I2C_M%d_Q%d wait timeout for RD_DONE irq for rc = %d FIFO buf_lvl:0x%x, rc: %d",
					cci_dev->soc_info.index, master, queue,
					val, rc);
				cam_cci_dump_registers(cci_dev,
						master, queue);

				cam_cci_flush_queue(cci_dev, master);
				goto rel_mutex_q;
			}

			if (cci_dev->cci_master_info[master].status) {
				CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Error with Slave 0x%x",
					cci_dev->soc_info.index, master, queue, (c_ctrl->cci_info->sid << 1));
				rc = -EINVAL;
				cci_dev->cci_master_info[master].status = 0;
				goto rel_mutex_q;
			}
			break;
		}
	}

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d Burst read successful words_read %d",
		cci_dev->soc_info.index, master, queue, total_read_words);

rel_mutex_q:
	mutex_unlock(&cci_dev->cci_master_info[master].mutex_q[queue]);

	mutex_lock(&cci_dev->cci_master_info[master].freq_cnt_lock);
	if (--cci_dev->cci_master_info[master].freq_ref_cnt == 0)
		up(&cci_dev->cci_master_info[master].master_sem);
	mutex_unlock(&cci_dev->cci_master_info[master].freq_cnt_lock);
	return rc;
}

static int32_t cam_cci_read(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl)
{
	int32_t rc = 0;
	uint32_t val = 0;
	int32_t read_words = 0, exp_words = 0;
	int32_t index = 0, first_byte = 0;
	uint32_t i = 0;
	enum cci_i2c_master_t master;
	enum cci_i2c_queue_t queue = QUEUE_1;
	struct cci_device *cci_dev = NULL;
	struct cam_cci_read_cfg *read_cfg = NULL;
	struct cam_hw_soc_info *soc_info = NULL;
	void __iomem *base = NULL;

	cci_dev = v4l2_get_subdevdata(sd);
	master = c_ctrl->cci_info->cci_i2c_master;
	read_cfg = &c_ctrl->cfg.cci_i2c_read_cfg;

	if (c_ctrl->cci_info->cci_i2c_master >= MASTER_MAX
		|| c_ctrl->cci_info->cci_i2c_master < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Invalid I2C master addr:%d",
			cci_dev->soc_info.index, master, queue, c_ctrl->cci_info->cci_i2c_master);
		return -EINVAL;
	}

	/* Set the I2C Frequency */
	rc = cam_cci_set_clk_param(cci_dev, c_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "cam_cci_set_clk_param failed rc = %d", rc);
		return rc;
	}

	mutex_lock(&cci_dev->cci_master_info[master].mutex_q[queue]);
	cci_dev->is_burst_read[master] = false;
	reinit_completion(&cci_dev->cci_master_info[master].report_q[queue]);

	soc_info = &cci_dev->soc_info;
	base = soc_info->reg_map[0].mem_base;

	/*
	 * Call validate queue to make sure queue is empty before starting.
	 * If this call fails, don't proceed with i2c_read call. This is to
	 * avoid overflow / underflow of queue
	 */
	rc = cam_cci_validate_queue(cci_dev,
		cci_dev->cci_i2c_queue_info[master][queue].max_queue_size - 1,
		master, queue);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Initial validataion failed rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	if (c_ctrl->cci_info->retries > CCI_I2C_READ_MAX_RETRIES) {
		CAM_ERR(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Invalid read retries info retries from slave: %d, max retries: %d",
			cci_dev->soc_info.index, master, queue, c_ctrl->cci_info->retries, CCI_I2C_READ_MAX_RETRIES);
		goto rel_mutex_q;
	}

	if (read_cfg->data == NULL) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Data ptr is NULL",
			cci_dev->soc_info.index, master, queue);
		goto rel_mutex_q;
	}

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d set param sid 0x%x retries %d id_map %d",
		cci_dev->soc_info.index, master, queue, c_ctrl->cci_info->sid, c_ctrl->cci_info->retries,
		c_ctrl->cci_info->id_map);
	val = CCI_I2C_SET_PARAM_CMD | c_ctrl->cci_info->sid << 4 |
		c_ctrl->cci_info->retries << 16 |
		c_ctrl->cci_info->id_map << 18;
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write param_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = CCI_I2C_LOCK_CMD;
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write lock_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	if (read_cfg->addr_type >= CAMERA_SENSOR_I2C_TYPE_MAX) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Failed : Invalid addr type: %u",
			cci_dev->soc_info.index, master, queue, read_cfg->addr_type);
		rc = -EINVAL;
		goto rel_mutex_q;
	}

	val = CCI_I2C_WRITE_DISABLE_P_CMD | (read_cfg->addr_type << 4);
	for (i = 0; i < read_cfg->addr_type; i++) {
		val |= ((read_cfg->addr >> (i << 3)) & 0xFF)  <<
		((read_cfg->addr_type - i) << 3);
	}

	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write disable_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = CCI_I2C_READ_CMD | (read_cfg->num_byte << 4);
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write read_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = CCI_I2C_UNLOCK_CMD;
	rc = cam_cci_write_i2c_queue(cci_dev, val, master, queue);
	if (rc < 0) {
		CAM_DBG(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed to write unlock_cmd for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto rel_mutex_q;
	}

	val = cam_io_r_mb(base + CCI_I2C_M0_Q0_CUR_WORD_CNT_ADDR
			+ master * 0x200 + queue * 0x100);
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d cur word cnt 0x%x",
		cci_dev->soc_info.index, master, queue, val);
	cam_io_w_mb(val, base + CCI_I2C_M0_Q0_EXEC_WORD_CNT_ADDR
			+ master * 0x200 + queue * 0x100);

	val = 1 << ((master * 2) + queue);
	cam_io_w_mb(val, base + CCI_QUEUE_START_ADDR);
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d exp_words to be read: %d",
		cci_dev->soc_info.index, master, queue, ((read_cfg->num_byte / 4) + 1));

	if (!cam_common_wait_for_completion_timeout(
		&cci_dev->cci_master_info[master].rd_done, CCI_TIMEOUT)) {
		cam_cci_dump_registers(cci_dev, master, queue);

		rc = -ETIMEDOUT;
		val = cam_io_r_mb(base +
			CCI_I2C_M0_READ_BUF_LEVEL_ADDR + master * 0x100);
		CAM_ERR(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d wait timeout rd_done for cci: %d, master: %d, queue: %d, FIFO buf_lvl: 0x%x, rc: %d",
			cci_dev->soc_info.index, master, queue, val, rc);
		cam_cci_flush_queue(cci_dev, master);
		goto rel_mutex_q;
	}

	if (cci_dev->cci_master_info[master].status) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d ERROR with Slave 0x%x",
			cci_dev->soc_info.index, master, queue, (c_ctrl->cci_info->sid << 1));
		rc = -EINVAL;
		cci_dev->cci_master_info[master].status = 0;
		goto rel_mutex_q;
	}

	read_words = cam_io_r_mb(base +
		CCI_I2C_M0_READ_BUF_LEVEL_ADDR + master * 0x100);
	exp_words = ((read_cfg->num_byte / 4) + 1);
	if (read_words != exp_words) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d read_words: %d, exp words: %d",
			cci_dev->soc_info.index, master, queue, read_words, exp_words);
		memset(read_cfg->data, 0, read_cfg->num_byte);
		rc = -EINVAL;
		goto rel_mutex_q;
	}
	index = 0;
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d index: %d, num_type: %d",
		cci_dev->soc_info.index, master, queue, index, read_cfg->num_byte);
	first_byte = 0;
	while (read_words > 0) {
		val = cam_io_r_mb(base +
			CCI_I2C_M0_READ_DATA_ADDR + master * 0x100);
		CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d read val: 0x%x",
			cci_dev->soc_info.index, master, queue, val);
		for (i = 0; (i < 4) && (index < read_cfg->num_byte); i++) {
			CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d i: %d, index: %d",
				cci_dev->soc_info.index, master, queue, i, index);
			if (!first_byte) {
				CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d sid: 0x%x",
					cci_dev->soc_info.index, master, queue, val & 0xFF);
				first_byte++;
			} else {
				read_cfg->data[index] =
					(val  >> (i * 8)) & 0xFF;
				CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d data[%d] 0x%x",
					cci_dev->soc_info.index, master, queue, index,
					read_cfg->data[index]);
				index++;
			}
		}
		read_words--;
	}
rel_mutex_q:
	mutex_unlock(&cci_dev->cci_master_info[master].mutex_q[queue]);

	mutex_lock(&cci_dev->cci_master_info[master].freq_cnt_lock);
	if (--cci_dev->cci_master_info[master].freq_ref_cnt == 0)
		up(&cci_dev->cci_master_info[master].master_sem);
	mutex_unlock(&cci_dev->cci_master_info[master].freq_cnt_lock);
	return rc;
}

static int32_t cam_cci_gpio_write(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl, enum cci_gpio_queue_t queue,
	enum cci_i2c_sync sync_en, struct cci_trigger_data  *get_ctx)
{
	int32_t rc = 0;
	struct cci_device *cci_dev = v4l2_get_subdevdata(sd);
	uint8_t trig_i2c_queue;

	if (cci_dev->cci_state != CCI_STATE_ENABLED) {
		CAM_ERR(CAM_CCI, "invalid cci: %d state: %d",
		cci_dev->soc_info.index, cci_dev->cci_state);
		return -EINVAL;
	}
	if (!cci_dev->cci_gpio_info.is_initilized) {
		rc = cam_gpio_init(c_ctrl, cci_dev, queue, get_ctx);
		if (rc < 0) {
			CAM_ERR(CAM_CCI, "gpio_init fail");
			return rc;
		}
	}
	trig_i2c_queue = get_ctx->i2cqueue | get_ctx->master << 1;
	CAM_DBG(CAM_CCI, "trig_i2c_queue 0x%x", trig_i2c_queue);

	CAM_DBG(CAM_CCI, "CCI%d_GPIO_Q%d set param sid 0x%x retries %d id_map %d",
		cci_dev->soc_info.index, queue, c_ctrl->cci_info->sid, c_ctrl->cci_info->retries,
		 c_ctrl->cci_info->id_map);
	reinit_completion(&cci_dev->cci_gpio_info.report_q[queue]);

	/*
	 * Call validate queue to make sure queue is empty before starting.
	 * If this call fails, don't proceed with i2c_write call. This is to
	 * avoid overflow / underflow of queue
	 */
	rc = cam_cci_validate_gpio_queue(cci_dev,
		cci_dev->cci_gpio_queue_info[queue].max_queue_size-1,
		queue);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_GPIO_Q%d Initial validataion failed rc %d",
			cci_dev->soc_info.index, queue, rc);
		return -EINVAL;
	}

	if (c_ctrl->cci_info->retries > CCI_I2C_READ_MAX_RETRIES) {
		CAM_ERR(CAM_CCI,
			"CCI%d_GPIO_Q%d Invalid read retries from slave %d max retries %d",
			cci_dev->soc_info.index, queue,
			c_ctrl->cci_info->retries, CCI_I2C_READ_MAX_RETRIES);
		return -EINVAL;
	}

	rc = cam_cci_gpio_data_queue(cci_dev, c_ctrl, queue, sync_en, trig_i2c_queue);
	if (rc < 0) {
		CAM_ERR(CAM_CCI,
			"CCI%d_GPIO_Q%d Failed in queueing the data for rc: %d",
			cci_dev->soc_info.index, queue, rc);
		return -EINVAL;
	}
	return rc;
}

static int32_t cam_cci_i2c_write(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl, enum cci_i2c_queue_t queue,
	enum cci_i2c_sync sync_en)
{
	int32_t rc = 0;
	struct cci_device *cci_dev;
	enum cci_i2c_master_t master;

	cci_dev = v4l2_get_subdevdata(sd);

	if (cci_dev->cci_state != CCI_STATE_ENABLED) {
		CAM_ERR(CAM_CCI, "invalid cci: %d state: %d",
			cci_dev->soc_info.index, cci_dev->cci_state);
		return -EINVAL;
	}
	master = c_ctrl->cci_info->cci_i2c_master;
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d_Q%d set param sid 0x%x retries %d id_map %d",
		cci_dev->soc_info.index, master, queue, c_ctrl->cci_info->sid, c_ctrl->cci_info->retries,
		c_ctrl->cci_info->id_map);

	/* Set the I2C Frequency */
	rc = cam_cci_set_clk_param(cci_dev, c_ctrl);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d cam_cci_set_clk_param failed rc %d",
			cci_dev->soc_info.index, master, queue, rc);
		return rc;
	}
	reinit_completion(&cci_dev->cci_master_info[master].report_q[queue]);
	/*
	 * Call validate queue to make sure queue is empty before starting.
	 * If this call fails, don't proceed with i2c_write call. This is to
	 * avoid overflow / underflow of queue
	 */
	rc = cam_cci_validate_queue(cci_dev,
		cci_dev->cci_i2c_queue_info[master][queue].max_queue_size-1,
		master, queue);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Initial validataion failed rc %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto ERROR;
	}
	if (c_ctrl->cci_info->retries > CCI_I2C_READ_MAX_RETRIES) {
		CAM_ERR(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Invalid read retries info retries from slave: %d, max retries: %d",
			cci_dev->soc_info.index, master, queue, c_ctrl->cci_info->retries, CCI_I2C_READ_MAX_RETRIES);
		goto ERROR;
	}
	rc = cam_cci_data_queue(cci_dev, c_ctrl, queue, sync_en);
	if (rc < 0) {
		CAM_ERR(CAM_CCI,
			"CCI%d_I2C_M%d_Q%d Failed in queueing the data for rc: %d",
			cci_dev->soc_info.index, master, queue, rc);
		goto ERROR;
	}

ERROR:
	mutex_lock(&cci_dev->cci_master_info[master].freq_cnt_lock);
	if (--cci_dev->cci_master_info[master].freq_ref_cnt == 0)
		up(&cci_dev->cci_master_info[master].master_sem);
	mutex_unlock(&cci_dev->cci_master_info[master].freq_cnt_lock);
	return rc;
}

static void cam_cci_write_async_helper(struct work_struct *work)
{
	int rc;
	struct cci_device *cci_dev;
	struct cci_write_async *write_async =
		container_of(work, struct cci_write_async, work);
	struct cam_sensor_i2c_reg_setting *i2c_msg;
	enum cci_i2c_master_t master;
	struct cam_cci_master_info *cci_master_info;

	cam_common_util_thread_switch_delay_detect(
		"CCI worker schedule",
		write_async->worker_scheduled_ts,
		CAM_WORKER_SCHEDULE_TIME_THRESHOLD);
	cci_dev = write_async->cci_dev;
	i2c_msg = &write_async->c_ctrl.cfg.cci_i2c_write_cfg;
	master = write_async->c_ctrl.cci_info->cci_i2c_master;
	cci_master_info = &cci_dev->cci_master_info[master];

	mutex_lock(&cci_master_info->mutex_q[write_async->queue]);
	rc = cam_cci_i2c_write(&(cci_dev->v4l2_dev_str.sd),
		&write_async->c_ctrl, write_async->queue, write_async->sync_en);
	mutex_unlock(&cci_master_info->mutex_q[write_async->queue]);
	if (rc < 0)
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Failed rc: %d",
		cci_dev->soc_info.index, master, write_async->queue, rc);

	kfree(write_async->c_ctrl.cfg.cci_i2c_write_cfg.reg_setting);
	kfree(write_async);
}

static int32_t cam_cci_i2c_write_async(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl, enum cci_i2c_queue_t queue,
	enum cci_i2c_sync sync_en)
{
	int32_t rc = 0;
	struct cci_write_async *write_async;
	struct cci_device *cci_dev;
	struct cam_sensor_i2c_reg_setting *cci_i2c_write_cfg;
	struct cam_sensor_i2c_reg_setting *cci_i2c_write_cfg_w;

	cci_dev = v4l2_get_subdevdata(sd);

	write_async = kzalloc(sizeof(*write_async), GFP_KERNEL);
	if (!write_async) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Memory allocation failed for write_async",
			cci_dev->soc_info.index, c_ctrl->cci_info->cci_i2c_master, queue);
		return -ENOMEM;
	}


	INIT_WORK(&write_async->work, cam_cci_write_async_helper);
	write_async->cci_dev = cci_dev;
	write_async->c_ctrl = *c_ctrl;
	write_async->queue = queue;
	write_async->sync_en = sync_en;

	cci_i2c_write_cfg = &c_ctrl->cfg.cci_i2c_write_cfg;
	cci_i2c_write_cfg_w = &write_async->c_ctrl.cfg.cci_i2c_write_cfg;

	if (cci_i2c_write_cfg->size == 0) {
		kfree(write_async);
		return -EINVAL;
	}

	cci_i2c_write_cfg_w->reg_setting =
		kzalloc(sizeof(struct cam_sensor_i2c_reg_array)*
		cci_i2c_write_cfg->size, GFP_KERNEL);
	if (!cci_i2c_write_cfg_w->reg_setting) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d_Q%d Couldn't allocate memory for reg_setting",
			cci_dev->soc_info.index, c_ctrl->cci_info->cci_i2c_master, queue);
		kfree(write_async);
		return -ENOMEM;
	}
	memcpy(cci_i2c_write_cfg_w->reg_setting,
		cci_i2c_write_cfg->reg_setting,
		(sizeof(struct cam_sensor_i2c_reg_array)*
						cci_i2c_write_cfg->size));

	cci_i2c_write_cfg_w->addr_type = cci_i2c_write_cfg->addr_type;
	cci_i2c_write_cfg_w->addr_type = cci_i2c_write_cfg->addr_type;
	cci_i2c_write_cfg_w->data_type = cci_i2c_write_cfg->data_type;
	cci_i2c_write_cfg_w->size = cci_i2c_write_cfg->size;
	cci_i2c_write_cfg_w->delay = cci_i2c_write_cfg->delay;

	write_async->worker_scheduled_ts = ktime_get();
	queue_work(cci_dev->write_wq[write_async->queue], &write_async->work);

	return rc;
}

static int32_t cam_cci_read_bytes_v_1_2(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl)
{
	int32_t rc = 0;
	struct cci_device *cci_dev = NULL;
	enum cci_i2c_master_t master;
	struct cam_cci_read_cfg *read_cfg = NULL;
	uint16_t read_bytes = 0;

	if (!sd || !c_ctrl) {
		CAM_ERR(CAM_CCI, "sd %pK c_ctrl %pK",
			sd, c_ctrl);
		return -EINVAL;
	}
	if (!c_ctrl->cci_info) {
		CAM_ERR(CAM_CCI, "cci_info NULL");
		return -EINVAL;
	}
	cci_dev = v4l2_get_subdevdata(sd);
	if (!cci_dev) {
		CAM_ERR(CAM_CCI, "cci_dev NULL");
		return -EINVAL;
	}
	if (cci_dev->cci_state != CCI_STATE_ENABLED) {
		CAM_ERR(CAM_CCI, "invalid CCI:%d state %d",
			cci_dev->soc_info.index, cci_dev->cci_state);
		return -EINVAL;
	}

	if (c_ctrl->cci_info->cci_i2c_master >= MASTER_MAX
			|| c_ctrl->cci_info->cci_i2c_master < 0) {
		CAM_ERR(CAM_CCI, "Invalid I2C master addr");
		return -EINVAL;
	}

	master = c_ctrl->cci_info->cci_i2c_master;
	read_cfg = &c_ctrl->cfg.cci_i2c_read_cfg;
	if ((!read_cfg->num_byte) || (read_cfg->num_byte > CCI_I2C_MAX_READ)) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d read num bytes 0",
			cci_dev->soc_info.index, master);
		rc = -EINVAL;
		goto ERROR;
	}

	reinit_completion(&cci_dev->cci_master_info[master].rd_done);
	read_bytes = read_cfg->num_byte;
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d Bytes to read %u",
		cci_dev->soc_info.index, master, read_bytes);
	do {
		if (read_bytes >= CCI_READ_MAX_V_1_2)
			read_cfg->num_byte = CCI_READ_MAX_V_1_2;
		else
			read_cfg->num_byte = read_bytes;

		cci_dev->is_burst_read[master] = false;
		rc = cam_cci_read(sd, c_ctrl);
		if (rc) {
			CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d failed to read rc: %d",
				cci_dev->soc_info.index, master, rc);
			goto ERROR;
		}

		if (read_bytes >= CCI_READ_MAX_V_1_2) {
			read_cfg->addr += CCI_READ_MAX_V_1_2;
			read_cfg->data += CCI_READ_MAX_V_1_2;
			read_bytes -= CCI_READ_MAX_V_1_2;
		} else {
			read_bytes = 0;
		}
	} while (read_bytes);

ERROR:
	return rc;
}

static int32_t cam_cci_read_bytes(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl)
{
	int32_t rc = 0;
	struct cci_device *cci_dev = NULL;
	enum cci_i2c_master_t master;
	struct cam_cci_read_cfg *read_cfg = NULL;
	uint16_t read_bytes = 0;

	if (!sd || !c_ctrl) {
		CAM_ERR(CAM_CCI, "Invalid arg sd %pK c_ctrl %pK",
			sd, c_ctrl);
		return -EINVAL;
	}
	if (!c_ctrl->cci_info) {
		CAM_ERR(CAM_CCI, "cci_info NULL");
		return -EINVAL;
	}
	cci_dev = v4l2_get_subdevdata(sd);
	if (!cci_dev) {
		CAM_ERR(CAM_CCI, "cci_dev NULL");
		return -EINVAL;
	}
	if (cci_dev->cci_state != CCI_STATE_ENABLED) {
		CAM_ERR(CAM_CCI, "invalid CCI:%d state %d",
			cci_dev->soc_info.index, cci_dev->cci_state);
		return -EINVAL;
	}

	if (c_ctrl->cci_info->cci_i2c_master >= MASTER_MAX
			|| c_ctrl->cci_info->cci_i2c_master < 0) {
		CAM_ERR(CAM_CCI, "Invalid I2C master addr");
		return -EINVAL;
	}

	master = c_ctrl->cci_info->cci_i2c_master;
	read_cfg = &c_ctrl->cfg.cci_i2c_read_cfg;
	if ((!read_cfg->num_byte) || (read_cfg->num_byte > CCI_I2C_MAX_READ)) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d read num bytes 0",
			cci_dev->soc_info.index, master);
		rc = -EINVAL;
		goto ERROR;
	}

	read_bytes = read_cfg->num_byte;

	/*
	 * To avoid any conflicts due to back to back trigger of
	 * THRESHOLD irq's, we reinit the threshold wait before
	 * we load the burst read cmd.
	 */
	mutex_lock(&cci_dev->cci_master_info[master].mutex_q[QUEUE_1]);
	reinit_completion(&cci_dev->cci_master_info[master].rd_done);
	reinit_completion(&cci_dev->cci_master_info[master].th_complete);
	mutex_unlock(&cci_dev->cci_master_info[master].mutex_q[QUEUE_1]);

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d Bytes to read %u",
		cci_dev->soc_info.index, master, read_bytes);
	do {
		if (read_bytes >= CCI_I2C_MAX_BYTE_COUNT)
			read_cfg->num_byte = CCI_I2C_MAX_BYTE_COUNT;
		else
			read_cfg->num_byte = read_bytes;

		if (read_cfg->num_byte >= CCI_READ_MAX) {
			rc = cam_cci_burst_read(sd, c_ctrl);
		} else {
			rc = cam_cci_read(sd, c_ctrl);
		}
		if (rc) {
			CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d Failed to read rc:%d",
				cci_dev->soc_info.index, master, rc);
			goto ERROR;
		}

		if (read_bytes >= CCI_I2C_MAX_BYTE_COUNT) {
			read_cfg->addr += (CCI_I2C_MAX_BYTE_COUNT /
				read_cfg->data_type);
			read_cfg->data += CCI_I2C_MAX_BYTE_COUNT;
			read_bytes -= CCI_I2C_MAX_BYTE_COUNT;
		} else {
			read_bytes = 0;
		}
	} while (read_bytes);

ERROR:
	return rc;
}

static int32_t cam_cci_i2c_set_sync_prms(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl)
{
	int32_t rc = 0;
	struct cci_device *cci_dev;

	cci_dev = v4l2_get_subdevdata(sd);
	if (!cci_dev || !c_ctrl) {
		CAM_ERR(CAM_CCI,
			"Failed: invalid params cci_dev:%pK, c_ctrl:%pK",
			cci_dev, c_ctrl);
		rc = -EINVAL;
		return rc;
	}
	cci_dev->cci_wait_sync_cfg = c_ctrl->cfg.cci_wait_sync_cfg;
	cci_dev->valid_sync = cci_dev->cci_wait_sync_cfg.csid < 0 ? 0 : 1;

	return rc;
}

static int32_t cam_cci_release(struct v4l2_subdev *sd,
	enum cci_i2c_master_t master)
{
	uint8_t rc = 0;
	struct cci_device *cci_dev;

	cci_dev = v4l2_get_subdevdata(sd);

	rc = cam_cci_soc_release(cci_dev, master);
	if (rc < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d Failed in releasing the rc: %d",
			cci_dev->soc_info.index, master, rc);
		return rc;
	}

	return rc;
}

static int32_t cam_cci_write(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *c_ctrl)
{
	int32_t rc = 0;
	struct cci_device *cci_dev;
	enum cci_i2c_master_t master;
	struct cam_cci_master_info *cci_master_info;
	struct cam_cci_gpio_info *cci_gpio_info;
	uint32_t i = 0;

	cci_dev = v4l2_get_subdevdata(sd);
	if (!cci_dev || !c_ctrl) {
		CAM_ERR(CAM_CCI,
			"Failed: invalid params cci_dev:%pK, c_ctrl:%pK",
			cci_dev, c_ctrl);
		rc = -EINVAL;
		return rc;
	}

	master = c_ctrl->cci_info->cci_i2c_master;
	cci_gpio_info = &cci_dev->cci_gpio_info;

	if (c_ctrl->cci_info->cci_i2c_master >= MASTER_MAX
		|| c_ctrl->cci_info->cci_i2c_master < 0) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d Invalid I2C master addr",
			cci_dev->soc_info.index, master);
		return -EINVAL;
	}

	cci_master_info = &cci_dev->cci_master_info[master];

	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d ctrl_cmd = %d",
		cci_dev->soc_info.index, master, c_ctrl->cmd);
	switch (c_ctrl->cmd) {
	case MSM_CCI_I2C_WRITE_SYNC_BLOCK:
		mutex_lock(&cci_master_info->mutex_q[SYNC_QUEUE]);
		rc = cam_cci_i2c_write(sd, c_ctrl,
			SYNC_QUEUE, MSM_SYNC_ENABLE);
		mutex_unlock(&cci_master_info->mutex_q[SYNC_QUEUE]);
		break;
	case MSM_CCI_I2C_WRITE_SYNC:
		rc = cam_cci_i2c_write_async(sd, c_ctrl,
			SYNC_QUEUE, MSM_SYNC_ENABLE);
		break;
	case MSM_CCI_I2C_WRITE:
	case MSM_CCI_I2C_WRITE_SEQ:
	case MSM_CCI_I2C_WRITE_BURST:
	{
		struct cci_trigger_data  *get_ctx =
			cam_cci_get_ctx(cci_dev, c_ctrl->cci_info->id_map);

		if (get_ctx) {
			mutex_lock(&cci_master_info->mutex_q[get_ctx->i2cqueue]);
			rc = cam_cci_i2c_write(sd, c_ctrl, get_ctx->i2cqueue,
				MSM_SYNC_DISABLE);
			get_ctx->is_trigger_mode = false;
			mutex_unlock(&cci_master_info->mutex_q[get_ctx->i2cqueue]);
		} else {
			for (i = 0; i < NUM_QUEUES; i++) {
				if (mutex_trylock(&cci_master_info->mutex_q[i])) {
					struct cam_cci_i2c_queue_info *i2c_q_info =
						&(cci_dev->cci_i2c_queue_info[master][i]);

					if (i2c_q_info->queue_status == QUEUE_STATE_FREE) {
						i2c_q_info->queue_status = QUEUE_STATE_BUSY;
						rc = cam_cci_i2c_write(sd, c_ctrl, i,
							MSM_SYNC_DISABLE);
						i2c_q_info->queue_status = QUEUE_STATE_FREE;
						mutex_unlock(&cci_master_info->mutex_q[i]);
						return rc;
					}
					mutex_unlock(&cci_master_info->mutex_q[i]);
				}
			}
			mutex_lock(&cci_master_info->mutex_q[PRIORITY_QUEUE]);
			cci_dev->cci_i2c_queue_info[master][PRIORITY_QUEUE].queue_status =
				QUEUE_STATE_BUSY;
			rc = cam_cci_i2c_write(sd, c_ctrl,
				PRIORITY_QUEUE, MSM_SYNC_DISABLE);
			cci_dev->cci_i2c_queue_info[master][PRIORITY_QUEUE].queue_status =
				QUEUE_STATE_FREE;
			mutex_unlock(&cci_master_info->mutex_q[PRIORITY_QUEUE]);
		}
		break;
	}
	case MSM_CCI_GPIO_WRITE: {
		struct cci_trigger_data *get_ctx =
			cam_cci_get_context(cci_dev, c_ctrl->cfg.gpio_cfg.reg_setting->contextid);

		if (!get_ctx) {
			CAM_ERR(CAM_CCI, "Invalid contextId");
			return -EINVAL;
		}
		mutex_lock(&cci_gpio_info->mutex_q[get_ctx->gpioqueue]);
		rc = cam_cci_gpio_write(sd, c_ctrl, get_ctx->gpioqueue,
			MSM_SYNC_DISABLE, get_ctx);
		get_ctx->is_trigger_mode = true;
		mutex_unlock(&cci_gpio_info->mutex_q[get_ctx->gpioqueue]);
		break;
	}
	case MSM_CCI_I2C_WRITE_ASYNC:
		rc = cam_cci_i2c_write_async(sd, c_ctrl,
			PRIORITY_QUEUE, MSM_SYNC_DISABLE);
		break;
	default:
		rc = -ENOIOCTLCMD;
	}

	return rc;
}

int32_t cam_cci_core_cfg(struct v4l2_subdev *sd,
	struct cam_cci_ctrl *cci_ctrl)
{
	int32_t rc = 0;
	struct cci_device *cci_dev = v4l2_get_subdevdata(sd);
	enum cci_i2c_master_t master = MASTER_MAX;

	if (!cci_dev) {
		CAM_ERR(CAM_CCI, "CCI_DEV is null");
		return -EINVAL;
	}

	if (!cci_ctrl) {
		CAM_ERR(CAM_CCI, "CCI%d_I2C_M%d CCI_CTRL IS NULL", cci_dev->soc_info.index, master);
		return -EINVAL;
	}

	master = cci_ctrl->cci_info->cci_i2c_master;
	if (master >= MASTER_MAX) {
		CAM_ERR(CAM_CCI, "INVALID MASTER: %d", master);
		return -EINVAL;
	}

	if (cci_dev->cci_master_info[master].status < 0) {
		CAM_WARN(CAM_CCI, "CCI hardware is resetting");
		return -EAGAIN;
	}
	CAM_DBG(CAM_CCI, "CCI%d_I2C_M%d cmd = %d", cci_dev->soc_info.index, master, cci_ctrl->cmd);

	switch (cci_ctrl->cmd) {
	case MSM_CCI_GET_CONTEXT_ID:
		mutex_lock(&cci_dev->ctx_mutex);
		rc = cam_cci_get_context_id(cci_dev, cci_dev->num_active_trigger_sensor,
			master, &(cci_ctrl->cfg.trigger_data));
		if (rc == 0)
			++cci_dev->num_active_trigger_sensor;
		mutex_unlock(&cci_dev->ctx_mutex);
		break;
	case MSM_CCI_RELEASE_CONTEXT_ID:
		if ((&cci_ctrl->cfg.trigger_data)->context_id < CONTEXT_ID_MAX) {
			rc = cam_cci_release_trigger_data(cci_dev, master,
				(&cci_ctrl->cfg.trigger_data)->context_id);
			if (rc == 0)
				--cci_dev->num_active_trigger_sensor;
		} else {
			CAM_ERR(CAM_CCI, "invalid context_id");
			rc = -EINVAL;
		}
		break;
	case MSM_CCI_INIT:
		mutex_lock(&cci_dev->init_mutex);
		rc = cam_cci_init(sd, cci_ctrl);
		mutex_unlock(&cci_dev->init_mutex);
		break;
	case MSM_CCI_RELEASE:
		mutex_lock(&cci_dev->init_mutex);
		rc = cam_cci_release(sd, master);
		mutex_unlock(&cci_dev->init_mutex);
		break;
	case MSM_CCI_I2C_READ:
		/*
		 * CCI version 1.2 does not support burst read
		 * due to the absence of the read threshold register
		 */
		if (cci_dev->hw_version == CCI_VERSION_1_2_9) {
			CAM_DBG(CAM_CCI, "cci-v1.2 no burst read");
			rc = cam_cci_read_bytes_v_1_2(sd, cci_ctrl);
		} else {
			rc = cam_cci_read_bytes(sd, cci_ctrl);
		}
		break;
	case MSM_CCI_I2C_WRITE:
	case MSM_CCI_I2C_WRITE_SEQ:
	case MSM_CCI_I2C_WRITE_BURST:
	case MSM_CCI_I2C_WRITE_SYNC:
	case MSM_CCI_I2C_WRITE_ASYNC:
	case MSM_CCI_I2C_WRITE_SYNC_BLOCK:
		rc = cam_cci_write(sd, cci_ctrl);
		break;
	case MSM_CCI_GPIO_WRITE:
		rc = cam_cci_write(sd, cci_ctrl);
		break;
	case MSM_CCI_SET_SYNC_CID:
		rc = cam_cci_i2c_set_sync_prms(sd, cci_ctrl);
		break;

	default:
		rc = -ENOIOCTLCMD;
	}

	cci_ctrl->status = rc;

	return rc;
}
