/* Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/ratelimit.h>
#include "cam_mem_mgr_api.h"
#include "cam_req_mgr_workq.h"
#include "ais_tfe_soc.h"
#include "ais_tfe_core.h"
#include "cam_debug_util.h"
#include "ais_isp_trace.h"

/*TFE TOP DEFINITIONS*/
#define AIS_TFE_HW_RESET_HW_AND_REG_VAL       0x1
#define AIS_TFE_HW_RESET_HW_VAL               0x10000

#define AIS_VFE_IRQ_MASK0 0x5C
#define AIS_VFE_IRQ_MASK1 0x60
#define AIS_VFE_IRQ_CLEAR0 0x64
#define AIS_VFE_IRQ_CLEAR1 0x68
#define AIS_VFE_IRQ_STATUS0 0x6C
#define AIS_VFE_IRQ_STATUS1 0x70

#define AIS_VFE_STATUS0_RDI_SOF_IRQ_SHFT 27
#define AIS_VFE_STATUS0_RDI_SOF_IRQ_MSK  0xF
#define AIS_VFE_STATUS0_RDI_REGUP_IRQ_SHFT 5
#define AIS_VFE_STATUS0_RDI_REGUP_IRQ_MSK  0xF
#define AIS_VFE_STATUS1_RDI_OVERFLOW_IRQ_SHFT 2
#define AIS_VFE_STATUS1_RDI_OVERFLOW_IRQ_MSK  0xF

#define AIS_VFE_MASK0_RDI 0x780001E0
#define AIS_VFE_MASK1_RDI 0x000000BC

#define AIS_VFE_MASK1_RDI_OVERFLOW_SHT 2

#define AIS_VFE_STATUS0_BUS_WR_IRQ  (1 << 9)
#define AIS_VFE_STATUS0_RDI_OVERFLOW_IRQ  \
	(0xF << AIS_VFE_STATUS1_RDI_OVERFLOW_IRQ_SHFT)
#define AIS_VFE_STATUS0_RESET_ACK_IRQ  (1 << 31)
#define AIS_VFE_GLOBAL_RESET_CMD_RDI_0_RESET_SHFT (10)

#define AIS_VFE_REGUP_RDI_SHIFT 1
#define AIS_VFE_REGUP_RDI_ALL 0x1E

/*TFE BUS DEFINITIONS*/
#define AIS_VFE_BUS_STATUS0_ERROR_MASK AIS_VFE_BUS_STATUS0_VIOLATION
#define AIS_VFE_BUS_STATUS0_VIOLATION  (1 << 14)

#define AIS_VFE_BUS_SET_DEBUG_REG                0x82

#define AIS_VFE_RDI_BUS_DEFAULT_WIDTH               0xFFFF
#define AIS_VFE_RDI_BUS_DEFAULT_STRIDE              0xFFFF

#define AIS_VFE_BUS_VER2_MAX_CLIENTS 24
#define AIS_VFE_BUS_ADDR_NO_SYNC_DEFAULT_VAL \
	((1 << AIS_VFE_BUS_VER2_MAX_CLIENTS) - 1)

#define AIS_ISP_TFE_WM_LINE_BASED_MODE  0
#define AIS_ISP_TFE_WM_FRAME_BASED_MODE 1

#define ALIGNUP(value, alignment) \
    ((value + alignment - 1) / alignment * alignment)

#define AIS_TFE_BUS_IRQ_REG0            0
#define AIS_TFE_BUS_IRQ_REG1            1

int ais_tfe_irq_config(struct ais_vfe_hw_core_info *core_info,
		uint32_t  *irq_mask, uint32_t num_reg, bool enable)
{
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	void __iomem                   *mem_base;
	bool                            need_lock;
	unsigned long                   flags = 0;
	uint32_t i, val;

	if (!core_info) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "Invalid core data");
		return -EINVAL;
	}

	mem_base = core_info->mem_base;
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;

	need_lock = !in_irq();
	if (need_lock)
		spin_lock_irqsave(&core_info->spin_lock, flags);

	for (i = 0; i < num_reg; i++) {
		val = cam_io_r_mb(mem_base  +
			top_irq_reg->irq_reg_set[i].mask_reg_offset);
		if (enable)
			val |= irq_mask[i];
		else
			val &= ~irq_mask[i];
		cam_io_w_mb(val, mem_base +
			top_irq_reg->irq_reg_set[i].mask_reg_offset);
	}
	if (need_lock)
		spin_unlock_irqrestore(&core_info->spin_lock, flags);

	return 0;
}

static void ais_clear_rdi_path(struct ais_tfe_rdi_output *rdi_path)
{
	int i;

	rdi_path->frame_cnt = 0;

	rdi_path->num_buffer_hw_q = 0;
	INIT_LIST_HEAD(&rdi_path->buffer_q);
	INIT_LIST_HEAD(&rdi_path->buffer_hw_q);
	INIT_LIST_HEAD(&rdi_path->free_buffer_list);
	for (i = 0; i < AIS_VFE_MAX_BUF; i++) {
		INIT_LIST_HEAD(&rdi_path->buffers[i].list);
		list_add_tail(&rdi_path->buffers[i].list,
				&rdi_path->free_buffer_list);
	}

	memset(&rdi_path->last_sof_info, 0, sizeof(rdi_path->last_sof_info));

	rdi_path->num_sof_info_q = 0;
	INIT_LIST_HEAD(&rdi_path->sof_info_q);
	INIT_LIST_HEAD(&rdi_path->free_sof_info_list);
	for (i = 0; i < AIS_VFE_MAX_SOF_INFO; i++) {
		INIT_LIST_HEAD(&rdi_path->sof_info[i].list);
		list_add_tail(&rdi_path->sof_info[i].list,
				&rdi_path->free_sof_info_list);
	}
}

static int ais_tfe_bus_hw_init(struct ais_vfe_hw_core_info *core_info)
{
	struct ais_tfe_bus_ver2_hw_info    *bus_hw_info = NULL;
	struct ais_irq_register_set        *bus_hw_irq_regs = NULL;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	uint32_t i;
	uint32_t top_irq_reg_mask[3] = {0};

	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	bus_hw_irq_regs = bus_hw_info->common_reg.irq_reg_info.irq_reg_set;
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;

	top_irq_reg_mask[0] = (1 << bus_hw_info->top_bus_wr_irq_shift);

	ais_tfe_irq_config(core_info, top_irq_reg_mask,
			top_irq_reg->num_registers, true);

	/* configure the error irq */
	for (i = 0; i < bus_hw_info->common_reg.irq_reg_info.num_registers; i++)
		cam_io_w(bus_hw_info->bus_irq_error_set_mask[i],
				core_info->mem_base + bus_hw_irq_regs[0].mask_reg_offset);

#if 0  //[TODO] for ais: need to remove??
	/*set IRQ mask for BUS WR*/
	core_info->irq_mask0 |= AIS_VFE_STATUS0_BUS_WR_IRQ;
	cam_io_w_mb(core_info->irq_mask0,
		core_info->mem_base + AIS_VFE_IRQ_MASK0);

	cam_io_w_mb(0x7801,
		core_info->mem_base + bus_hw_irq_regs[0].mask_reg_offset);
	cam_io_w_mb(0x0,
		core_info->mem_base + bus_hw_irq_regs[1].mask_reg_offset);
	cam_io_w_mb(0x0,
		core_info->mem_base + bus_hw_irq_regs[2].mask_reg_offset);

	/*Set Debug Registers*/
	cam_io_w_mb(AIS_VFE_BUS_SET_DEBUG_REG, core_info->mem_base +
		bus_hw_info->common_reg.debug_status_cfg);

	/* BUS_WR_INPUT_IF_ADDR_SYNC_FRAME_HEADER */
	cam_io_w_mb(0x0, core_info->mem_base +
		bus_hw_info->common_reg.addr_sync_frame_hdr);

	/* no clock gating at bus input */
	cam_io_w_mb(0xFFFFF, core_info->mem_base + 0x0000200C);

	/* BUS_WR_TEST_BUS_CTRL */
	cam_io_w_mb(0x0, core_info->mem_base + 0x0000211C);

	/* if addr_no_sync has default value then config the addr no sync reg */
	cam_io_w_mb(AIS_VFE_BUS_ADDR_NO_SYNC_DEFAULT_VAL,
		core_info->mem_base +
		bus_hw_info->common_reg.addr_sync_no_sync);
#endif

	return 0;
}

static int ais_tfe_bus_hw_deinit(struct ais_vfe_hw_core_info *core_info)
{
	struct ais_tfe_bus_ver2_hw_info    *bus_hw_info = NULL;
	struct ais_irq_register_set        *bus_hw_irq_regs = NULL;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	uint32_t i = 0;
	uint32_t top_irq_reg_mask[3] = {0};

	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	bus_hw_irq_regs = bus_hw_info->common_reg.irq_reg_info.irq_reg_set;
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;

	/*set IRQ mask for BUS WR*/
	top_irq_reg_mask[0] = 1 << bus_hw_info->top_bus_wr_irq_shift;

	ais_tfe_irq_config(core_info, top_irq_reg_mask,
			top_irq_reg->num_registers, false);

	/* configure the error irq */
	for (i = 0; i < bus_hw_info->common_reg.irq_reg_info.num_registers; i++)
		cam_io_w(0, core_info->mem_base + bus_hw_irq_regs[0].mask_reg_offset);

	return 0;
}

int ais_tfe_get_hw_caps(void *hw_priv, void *get_hw_cap_args, uint32_t arg_size)
{
	struct cam_hw_info                *vfe_dev = hw_priv;
	struct ais_vfe_hw_core_info       *core_info = NULL;
	int rc = 0;

	CAM_DBG(CAM_ISP, "Enter");
	if (!hw_priv) {
		CAM_ERR(CAM_ISP, "Invalid arguments");
		return -EINVAL;
	}

	core_info = (struct ais_vfe_hw_core_info *)vfe_dev->core_info;

	CAM_WARN(CAM_ISP, "TFE%d get_hw_caps not implemented",
			core_info->vfe_idx);

	rc = -EPERM;

	CAM_DBG(CAM_ISP, "Exit");
	return rc;
}

static int ais_tfe_reset(void *hw_priv,
	void *reset_core_args, uint32_t arg_size)
{
	struct cam_hw_info                 *vfe_hw  = hw_priv;
	struct cam_hw_soc_info             *soc_info = NULL;
	struct ais_vfe_hw_core_info        *core_info = NULL;
	struct ais_tfe_top_ver2_hw_info    *top_hw_info = NULL;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	uint32_t *reset_reg_args = reset_core_args;
	uint32_t  reset_reg_val;
	int rc = 0;
	int i;
	uint32_t irq_status[3] = {0};

	CAM_DBG(CAM_ISP, "Enter");

	if (!hw_priv) {
		CAM_ERR(CAM_ISP, "Invalid input arguments");
		return -EINVAL;
	}

	soc_info = &vfe_hw->soc_info;
	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	top_hw_info = core_info->vfe_hw_info->top_hw_info;
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;

	for (i = 0; i < top_irq_reg->num_registers; i++)
		irq_status[i] = cam_io_r(core_info->mem_base +
				top_irq_reg->irq_reg_set[i].status_reg_offset);

	for (i = 0; i < top_irq_reg->num_registers; i++)
		cam_io_w(irq_status[i], core_info->mem_base +
				top_irq_reg->irq_reg_set[i].clear_reg_offset);

	cam_io_w(top_irq_reg->global_clear_bitmask,
			core_info->mem_base + top_irq_reg->global_clear_offset);

	/* Mask all irq registers */
	for (i = 0; i < top_irq_reg->num_registers; i++)
		cam_io_w(0, core_info->mem_base +
				top_irq_reg->irq_reg_set[i].mask_reg_offset);

	ais_tfe_irq_config(core_info, top_hw_info->top_irq_reset_mask,
			top_irq_reg->num_registers, true);

	reinit_completion(&vfe_hw->hw_complete);

	CAM_DBG(CAM_ISP, "calling RESET on tfe %d", soc_info->index);

	switch (*reset_reg_args) {
	case AIS_VFE_HW_RESET_HW_AND_REG:
		reset_reg_val = AIS_TFE_HW_RESET_HW_AND_REG_VAL;
		break;
	default:
		reset_reg_val = AIS_TFE_HW_RESET_HW_VAL;
		break;
	}

	/* Reset HW */
	cam_io_w_mb(reset_reg_val,
		core_info->mem_base +
		top_hw_info->common_reg->global_reset_cmd);

	CAM_DBG(CAM_ISP, "waiting for tfe reset complete");

	/* Wait for Completion or Timeout of 500ms */
	rc = cam_common_wait_for_completion_timeout(
			&vfe_hw->hw_complete, msecs_to_jiffies(500));
	if (rc) {
		rc = 0;
	} else {
		CAM_ERR(CAM_ISP, "Error! Reset Timeout");
		rc = EFAULT;
	}

	CAM_DBG(CAM_ISP, "reset complete done (%d)", rc);

	core_info->irq_mask0 = 0x0;
	ais_tfe_irq_config(core_info, top_hw_info->top_irq_reset_mask,
			top_irq_reg->num_registers, false);

	for (i = 0; i < AIS_IFE_PATH_MAX; i++) {
		ais_clear_rdi_path(&core_info->rdi_out[i]);
		core_info->rdi_out[i].state = AIS_ISP_RESOURCE_STATE_AVAILABLE;
	}

	CAM_DBG(CAM_ISP, "Exit");
	return rc;
}


static void ais_tfe_reset_rdi(void *hw_priv,
	enum ais_ife_output_path_id path)
{
	struct cam_hw_info *vfe_hw  = hw_priv;
	struct ais_tfe_top_ver2_hw_info *top_hw_info = NULL;
	struct ais_vfe_hw_core_info *core_info = NULL;
	uint32_t  reset_reg_val = 0;
	int rc = 0;

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	top_hw_info = core_info->vfe_hw_info->top_hw_info;

	cam_io_w_mb(AIS_VFE_STATUS0_RESET_ACK_IRQ,
		core_info->mem_base + AIS_VFE_IRQ_MASK0);

	reinit_completion(&vfe_hw->hw_complete);

	reset_reg_val = 1 << (AIS_VFE_GLOBAL_RESET_CMD_RDI_0_RESET_SHFT + path);

	/* Reset HW */
	cam_io_w_mb(reset_reg_val,
		core_info->mem_base +
		top_hw_info->common_reg->global_reset_cmd);

	CAM_DBG(CAM_ISP, "waiting for tfe reset complete");

	/* Wait for Completion or Timeout of 500ms */
	rc = wait_for_completion_timeout(&vfe_hw->hw_complete,
					msecs_to_jiffies(500));

	CAM_INFO(CAM_ISP, "reset IFE%d RDI%d complete done (%d)", core_info->vfe_idx, path, rc);

	if (rc)
		rc = 0;
	else
		CAM_ERR(CAM_ISP, "Error! Reset Timeout");

	core_info->irq_mask0 = 0x0;
	cam_io_w_mb(0x0, core_info->mem_base + AIS_VFE_IRQ_MASK0);
}

int ais_tfe_init_hw(void *hw_priv, void *init_hw_args, uint32_t arg_size)
{
	struct cam_hw_info                *vfe_hw = hw_priv;
	struct cam_hw_soc_info            *soc_info = NULL;
	struct ais_vfe_hw_core_info       *core_info = NULL;
	int rc = 0;
	unsigned long                      max_clk_rate = 0;
	uint32_t reset_core_args = AIS_VFE_HW_RESET_HW_AND_REG;

	CAM_DBG(CAM_ISP, "Enter");
	if (!hw_priv) {
		CAM_ERR(CAM_ISP, "Invalid arguments");
		return -EINVAL;
	}

	mutex_lock(&vfe_hw->hw_mutex);
	vfe_hw->open_count++;
	if (vfe_hw->open_count > 1) {
		mutex_unlock(&vfe_hw->hw_mutex);
		CAM_DBG(CAM_ISP, "TFE has already been initialized cnt %d",
			vfe_hw->open_count);
		return 0;
	}
	mutex_unlock(&vfe_hw->hw_mutex);

	soc_info = &vfe_hw->soc_info;
	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;

	/* Turn ON Regulators, Clocks and other SOC resources */
	rc = ais_tfe_enable_soc_resources(soc_info, max_clk_rate);
	if (rc) {
		CAM_ERR(CAM_ISP, "Enable SOC failed");
		rc = -EFAULT;
		goto decrement_open_cnt;
	}

	CAM_DBG(CAM_ISP, "Enable soc done");

	/* Do HW Reset */
	rc = ais_tfe_reset(hw_priv, &reset_core_args, sizeof(uint32_t));
	if (rc) {
		CAM_ERR(CAM_ISP, "Reset Failed rc=%d", rc);
		goto disable_soc;
	}

	rc = ais_tfe_bus_hw_init(core_info);
	if (rc) {
		CAM_ERR(CAM_ISP, "Reset Failed rc=%d", rc);
		goto disable_soc;
	}

	vfe_hw->hw_state = CAM_HW_STATE_POWER_UP;
	return rc;

disable_soc:
	ais_tfe_disable_soc_resources(soc_info);
decrement_open_cnt:
	mutex_lock(&vfe_hw->hw_mutex);
	vfe_hw->open_count--;
	mutex_unlock(&vfe_hw->hw_mutex);
	return rc;
}

int ais_tfe_deinit_hw(void *hw_priv, void *deinit_hw_args, uint32_t arg_size)
{
	struct cam_hw_info                *vfe_hw = hw_priv;
	struct cam_hw_soc_info            *soc_info = NULL;
	struct ais_vfe_hw_core_info       *core_info = NULL;
	int rc = 0;
	uint32_t                           reset_core_args =
					AIS_VFE_HW_RESET_HW_AND_REG;

	CAM_DBG(CAM_ISP, "Enter");
	if (!hw_priv) {
		CAM_ERR(CAM_ISP, "Invalid arguments");
		return -EINVAL;
	}

	mutex_lock(&vfe_hw->hw_mutex);
	if (!vfe_hw->open_count) {
		mutex_unlock(&vfe_hw->hw_mutex);
		CAM_ERR(CAM_ISP, "Error! Unbalanced deinit");
		return -EFAULT;
	}
	vfe_hw->open_count--;
	if (vfe_hw->open_count) {
		mutex_unlock(&vfe_hw->hw_mutex);
		CAM_DBG(CAM_ISP, "open_cnt non-zero =%d", vfe_hw->open_count);
		return 0;
	}
	mutex_unlock(&vfe_hw->hw_mutex);

	soc_info = &vfe_hw->soc_info;
	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;

	rc = ais_tfe_bus_hw_deinit(core_info);
	if (rc)
		CAM_ERR(CAM_ISP, "Bus HW deinit Failed rc=%d", rc);

	rc = ais_tfe_reset(hw_priv, &reset_core_args, sizeof(uint32_t));

	/* Turn OFF Regulators, Clocks and other SOC resources */
	CAM_DBG(CAM_ISP, "Disable SOC resource");
	rc = ais_tfe_disable_soc_resources(soc_info);
	if (rc)
		CAM_ERR(CAM_ISP, "Disable SOC failed");

	vfe_hw->hw_state = CAM_HW_STATE_POWER_DOWN;

	CAM_DBG(CAM_ISP, "Exit");
	return rc;
}

int ais_tfe_force_reset(void *hw_priv, void *reset_core_args, uint32_t arg_size)
{
	struct cam_hw_info                *vfe_hw = hw_priv;
	bool require_deinit = false;
	int rc = 0;

	mutex_lock(&vfe_hw->hw_mutex);
	if (vfe_hw->open_count) {
		vfe_hw->open_count = 1;
		require_deinit = true;

	}
	mutex_unlock(&vfe_hw->hw_mutex);

	if (require_deinit) {
		CAM_INFO(CAM_ISP, "tfe deinit HW");
		rc = ais_tfe_deinit_hw(vfe_hw, NULL, 0);
	}

	CAM_DBG(CAM_ISP, "Exit (%d)", rc);

	return rc;
}

void ais_isp_tfe_hw_get_timestamp(struct ais_isp_timestamp *time_stamp)
{
	struct timespec64 ts;

	ktime_get_boottime_ts64(&ts);
	time_stamp->mono_time.tv_sec    = ts.tv_sec;
	time_stamp->mono_time.tv_nsec   = ts.tv_nsec;
	time_stamp->time_usecs =  ts.tv_sec * 1000000 +
				(time_stamp->mono_time.tv_nsec / NSEC_PER_USEC);
}

static int ais_tfe_config_rdi_wm(struct ais_vfe_hw_core_info *core_info,
		struct ais_tfe_rdi_output *rdi_path,
		struct ais_ife_rdi_init_args *rdi_cfg)
{
	struct ais_tfe_bus_ver2_hw_info *bus_hw_info = NULL;
	int pack_fmt = 0;
	int rdi_width = 0;
	int mode_cfg_shift = 0;

	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	rdi_width = bus_hw_info->rdi_width;
	mode_cfg_shift = bus_hw_info->mode_cfg_shift;

	if (rdi_width == 64)
		pack_fmt = 0xa;
	else if (rdi_width == 128)
		pack_fmt = 0x0;

	switch (rdi_cfg->out_cfg.format) {
		case CAM_FORMAT_MIPI_RAW_10:
			rdi_path->pack_fmt = pack_fmt;
			if (rdi_cfg->out_cfg.mode == AIS_ISP_TFE_WM_FRAME_BASED_MODE) {
				/*frame base mode*/
				rdi_path->en_cfg = (0x1 << mode_cfg_shift) | 0x1;
				rdi_path->height = 0;
				rdi_path->width = AIS_VFE_RDI_BUS_DEFAULT_WIDTH;
				rdi_path->stride = AIS_VFE_RDI_BUS_DEFAULT_STRIDE;
			} else {
				/*line base mode*/
				rdi_path->en_cfg =  0x1;
				rdi_path->width =
					ALIGNUP(rdi_cfg->out_cfg.width * 10, rdi_width) /
					rdi_width;
			}
			break;
		case CAM_FORMAT_YUV422:
			rdi_path->pack_fmt = pack_fmt;
			if (rdi_cfg->out_cfg.mode == AIS_ISP_TFE_WM_FRAME_BASED_MODE) {
				/*frame base mode*/
				rdi_path->en_cfg = (0x1 << mode_cfg_shift) | 0x1;
				rdi_path->height = 0;
				rdi_path->width = AIS_VFE_RDI_BUS_DEFAULT_WIDTH;
				rdi_path->stride = AIS_VFE_RDI_BUS_DEFAULT_STRIDE;
			} else {
				/*line base mode*/
				rdi_path->en_cfg =  0x1;
				rdi_path->width =
					ALIGNUP(rdi_cfg->out_cfg.width * 8, rdi_width) /
					rdi_width;
			}
			break;
		default:
			CAM_ERR(CAM_ISP, "do not support format = 0x%x", rdi_cfg->out_cfg.format);
			break;
	}

	CAM_DBG(CAM_ISP, "out_format = 0x%x, mode = 0x%x, pack_fmt = 0x%x, width = %d, height = %d, stride = %d, en_cfg= 0x%x",
			rdi_cfg->out_cfg.format,
			rdi_cfg->out_cfg.mode,
			rdi_path->pack_fmt,
			rdi_path->width,
			rdi_path->height,
			rdi_path->stride,
			rdi_path->en_cfg);
	return 0;
}

int ais_tfe_reserve(void *hw_priv, void *reserve_args, uint32_t arg_size)
{
	struct ais_vfe_hw_core_info       *core_info = NULL;
	struct cam_hw_info                *vfe_hw = hw_priv;
	struct ais_tfe_rdi_output         *rdi_path = NULL;
	struct ais_ife_rdi_init_args      *rdi_cfg;
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_tfe_top_ver2_hw_info   *top_hw_info = NULL;
	struct ais_tfe_rdi_ver2_hw_info   *rdi_hw_info = NULL;
	struct ais_vfe_rdi_reg_data       *rdi_reg_data = NULL;
	struct ais_tfe_rdi_reg            *rdi_reg = NULL;
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	int rc = 0;

	if (!hw_priv || !reserve_args || (arg_size !=
		sizeof(struct ais_ife_rdi_init_args))) {
		CAM_ERR(CAM_ISP, "Invalid input arguments");
		return -EINVAL;
	}

	rdi_cfg = (struct ais_ife_rdi_init_args *)reserve_args;
	if (rdi_cfg->path >= AIS_IFE_PATH_MAX) {
		CAM_ERR(CAM_ISP, "Invalid output path %d", rdi_cfg->path);
		return -EINVAL;
	}

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	rdi_path = &core_info->rdi_out[rdi_cfg->path];
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	client_regs = &bus_hw_info->bus_client_reg[rdi_cfg->path];
	top_hw_info = core_info->vfe_hw_info->top_hw_info;
	rdi_hw_info = &top_hw_info->rdi_hw_info;
	rdi_reg = rdi_hw_info->rdi_reg[rdi_cfg->path];
	rdi_reg_data = rdi_hw_info->reg_data[rdi_cfg->path];

	CAM_DBG(CAM_ISP, "Config RDI%d", rdi_cfg->path);

	mutex_lock(&vfe_hw->hw_mutex);

	if (rdi_path->state >= AIS_ISP_RESOURCE_STATE_INIT_HW) {
		CAM_ERR(CAM_ISP, "RDI%d invalid state %d", rdi_cfg->path,
				rdi_path->state);
		rc = -EINVAL;
		goto EXIT;
	}

	rdi_path->secure_mode = rdi_cfg->out_cfg.secure_mode;
	rdi_path->width = rdi_cfg->out_cfg.width;
	rdi_path->height = rdi_cfg->out_cfg.height;
	rdi_path->stride = rdi_cfg->out_cfg.stride;
	rdi_path->pix_pattern = rdi_cfg->out_cfg.pix_pattern;

	ais_tfe_config_rdi_wm(core_info, rdi_path, rdi_cfg);

	cam_io_w(rdi_path->pack_fmt, core_info->mem_base + client_regs->packer_cfg);

	cam_io_w((rdi_path->height << bus_hw_info->height_shift) | rdi_path->width,
			core_info->mem_base + client_regs->buffer_width_cfg);
	
	cam_io_w_mb(rdi_path->stride,
			core_info->mem_base + client_regs->stride);
	
	if (client_regs->framedrop_period) {
	    cam_io_w_mb(rdi_cfg->out_cfg.frame_drop_period,
	        core_info->mem_base + client_regs->framedrop_period);
	    cam_io_w_mb(rdi_cfg->out_cfg.frame_drop_pattern,
	        core_info->mem_base + client_regs->framedrop_pattern);
	}

#if 0
	cam_io_w(0xf, core_info->mem_base + client_regs->burst_limit);

	cam_io_w_mb(rdi_cfg->out_cfg.frame_drop_period,
		core_info->mem_base + client_regs->framedrop_period);
	cam_io_w_mb(rdi_cfg->out_cfg.frame_drop_pattern,
		core_info->mem_base + client_regs->framedrop_pattern);
#endif

	cam_io_w_mb(rdi_path->pix_pattern << rdi_reg_data->pixel_pattern_shift,
			core_info->mem_base + rdi_reg->rdi_module_config);

	/* Epoch config */
	cam_io_w_mb(rdi_reg_data->epoch_line_cfg,
			core_info->mem_base + rdi_reg->rdi_epoch_irq);


	rdi_path->batchConfig = rdi_cfg->out_cfg.batch_config;
	if (rdi_path->batchConfig.numBatchFrames < 1 || rdi_path->batchConfig.numBatchFrames > 4) {
		CAM_ERR(CAM_ISP, "invalid numBatchFrames %d", rdi_path->batchConfig.numBatchFrames);
		rdi_path->batchConfig.numBatchFrames = 1; //set numBatchFrames as default
	}

	rdi_path->state = AIS_ISP_RESOURCE_STATE_INIT_HW;

EXIT:
	mutex_unlock(&vfe_hw->hw_mutex);

	return rc;
}


int ais_tfe_release(void *hw_priv, void *release_args, uint32_t arg_size)
{
	struct ais_vfe_hw_core_info       *core_info = NULL;
	struct cam_hw_info                *vfe_hw  = hw_priv;
	struct ais_tfe_rdi_output         *rdi_path = NULL;
	struct ais_ife_rdi_deinit_args    *deinit_cmd;

	int rc = 0;

	if (!hw_priv || !release_args ||
		(arg_size != sizeof(struct ais_ife_rdi_deinit_args))) {
		CAM_ERR(CAM_ISP, "Invalid input arguments");
		return -EINVAL;
	}

	deinit_cmd = (struct ais_ife_rdi_deinit_args *)release_args;

	if (deinit_cmd->path >= AIS_IFE_PATH_MAX) {
		CAM_ERR(CAM_ISP, "Invalid output path %d", deinit_cmd->path);
		return -EINVAL;
	}

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	rdi_path = &core_info->rdi_out[deinit_cmd->path];

	mutex_lock(&vfe_hw->hw_mutex);

	if (rdi_path->state < AIS_ISP_RESOURCE_STATE_INIT_HW) {
		CAM_ERR(CAM_ISP, "RDI%d invalid state %d", deinit_cmd->path,
				rdi_path->state);
		rc = -EINVAL;
		goto EXIT;
	}

	rdi_path->state = AIS_ISP_RESOURCE_STATE_AVAILABLE;

EXIT:
	mutex_unlock(&vfe_hw->hw_mutex);

	return rc;
}


int ais_tfe_start(void *hw_priv, void *start_args, uint32_t arg_size)
{
	struct ais_vfe_hw_core_info       *core_info = NULL;
	struct cam_hw_info                *vfe_hw  = hw_priv;
	struct ais_ife_rdi_start_args     *start_cmd;
	struct ais_tfe_rdi_output         *rdi_path;
	struct ais_tfe_top_ver2_hw_info   *top_hw_info = NULL;
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_irq_register_set       *bus_hw_irq_regs = NULL;
	struct ais_tfe_rdi_ver2_hw_info   *rdi_hw_info = NULL;
	struct ais_vfe_rdi_reg_data       *rdi_reg_data = NULL;
	struct ais_tfe_rdi_reg            *rdi_reg = NULL;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	struct ais_tfe_bus_ver2_vfe_out_hw_info *tfe_out_hw_info = NULL;
	uint32_t rdi_comp_done_shift = 0;
	uint32_t val = 0;
	int rc = 0;

	if (!hw_priv || !start_args ||
		(arg_size != sizeof(struct ais_ife_rdi_start_args))) {
		CAM_ERR(CAM_ISP, "Invalid input arguments");
		return -EINVAL;
	}

	start_cmd = (struct ais_ife_rdi_start_args *)start_args;
	if (start_cmd->path >= AIS_IFE_PATH_MAX) {
		CAM_ERR(CAM_ISP, "Invalid output path %d", start_cmd->path);
		return -EINVAL;
	}

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	rdi_path = &core_info->rdi_out[start_cmd->path];
	top_hw_info = core_info->vfe_hw_info->top_hw_info;
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	bus_hw_irq_regs = bus_hw_info->common_reg.irq_reg_info.irq_reg_set;
	client_regs = &bus_hw_info->bus_client_reg[start_cmd->path];
	tfe_out_hw_info = &bus_hw_info->vfe_out_hw_info[start_cmd->path];
	rdi_hw_info = &top_hw_info->rdi_hw_info;
	rdi_reg = rdi_hw_info->rdi_reg[start_cmd->path];
	rdi_reg_data = rdi_hw_info->reg_data[start_cmd->path];
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;

	mutex_lock(&vfe_hw->hw_mutex);

	if (rdi_path->state != AIS_ISP_RESOURCE_STATE_INIT_HW) {
		CAM_ERR(CAM_ISP, "RDI%d invalid state %d", start_cmd->path,
				rdi_path->state);
		rc = -EINVAL;
		goto EXIT;
	}

	val = cam_io_r_mb(core_info->mem_base + rdi_reg->rdi_module_config);
	val |= 0x1 << rdi_reg_data->rdi_out_enable_shift;
	cam_io_w_mb(val, core_info->mem_base + rdi_reg->rdi_module_config);

	/*Enable bus WR mask*/
	rdi_comp_done_shift = client_regs->comp_group +
						bus_hw_info->comp_done_shift;
	core_info->bus_wr_mask0 |= (1 << rdi_comp_done_shift);
	/* Update the composite regupdate mask in bus irq mask*/
	core_info->bus_wr_mask0 |= (1 << tfe_out_hw_info->rup_group_id);
	cam_io_w_mb(core_info->bus_wr_mask0,
		core_info->mem_base +
		bus_hw_irq_regs[AIS_TFE_BUS_IRQ_REG0].mask_reg_offset);

	/*Update TFE mask*/
	core_info->irq_mask0 |= rdi_reg_data->subscribe_irq_mask[0];
	core_info->irq_mask1 |= rdi_reg_data->subscribe_irq_mask[1];
	ais_tfe_irq_config(core_info, rdi_reg_data->subscribe_irq_mask,
			top_irq_reg->num_registers, true);

	/* Enable WM and reg-update*/
	cam_io_w_mb(rdi_path->en_cfg, core_info->mem_base + client_regs->cfg);
	val = cam_io_r_mb(core_info->mem_base + rdi_reg->reg_update_cmd);
	val |= rdi_reg_data->reg_update_cmd_data;
	cam_io_w_mb(val, core_info->mem_base + rdi_reg->reg_update_cmd);

	rdi_path->state = AIS_ISP_RESOURCE_STATE_STREAMING;

EXIT:
	mutex_unlock(&vfe_hw->hw_mutex);

	return rc;
}

int ais_tfe_stop(void *hw_priv, void *stop_args, uint32_t arg_size)
{
	struct ais_vfe_hw_core_info       *core_info = NULL;
	struct cam_hw_info                *vfe_hw  = hw_priv;
	struct ais_ife_rdi_stop_args      *stop_cmd;
	struct ais_tfe_rdi_output         *rdi_path;
	struct ais_tfe_top_ver2_hw_info   *top_hw_info = NULL;
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_irq_register_set       *bus_hw_irq_regs = NULL;
	struct ais_tfe_rdi_ver2_hw_info   *rdi_hw_info = NULL;
	struct ais_vfe_rdi_reg_data       *rdi_reg_data = NULL;
	struct ais_tfe_rdi_reg            *rdi_reg = NULL;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	struct ais_tfe_bus_ver2_vfe_out_hw_info *tfe_out_hw_info = NULL;
	uint32_t rdi_comp_done_shift = 0;
	uint32_t val = 0;
	int rc = 0;

	if (!hw_priv || !stop_args ||
		(arg_size != sizeof(struct ais_ife_rdi_stop_args))) {
		CAM_ERR(CAM_ISP, "Invalid input arguments");
		return -EINVAL;
	}

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	stop_cmd = (struct ais_ife_rdi_stop_args  *)stop_args;

	if (stop_cmd->path >= AIS_IFE_PATH_MAX) {
		CAM_ERR(CAM_ISP, "Invalid output path %d", stop_cmd->path);
		return -EINVAL;
	}

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	rdi_path = &core_info->rdi_out[stop_cmd->path];
	top_hw_info = core_info->vfe_hw_info->top_hw_info;
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	bus_hw_irq_regs = bus_hw_info->common_reg.irq_reg_info.irq_reg_set;
	client_regs = &bus_hw_info->bus_client_reg[stop_cmd->path];
	tfe_out_hw_info = &bus_hw_info->vfe_out_hw_info[stop_cmd->path];
	rdi_hw_info = &top_hw_info->rdi_hw_info;
	rdi_reg = rdi_hw_info->rdi_reg[stop_cmd->path];
	rdi_reg_data = rdi_hw_info->reg_data[stop_cmd->path];
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;

	mutex_lock(&vfe_hw->hw_mutex);

	if (rdi_path->state != AIS_ISP_RESOURCE_STATE_STREAMING &&
		rdi_path->state != AIS_ISP_RESOURCE_STATE_ERROR) {
		CAM_ERR(CAM_ISP, "RDI%d invalid state %d", stop_cmd->path,
				rdi_path->state);
		rc = -EINVAL;
		goto EXIT;
	}

	spin_lock(&rdi_path->buffer_lock);
	ais_clear_rdi_path(rdi_path);
	spin_unlock(&rdi_path->buffer_lock);

	/* Disable WM and reg-update*/
	val = cam_io_r_mb(core_info->mem_base + rdi_reg->reg_update_cmd);
	val &= ~rdi_reg_data->reg_update_cmd_data;
	cam_io_w_mb(val, core_info->mem_base + rdi_reg->reg_update_cmd);

	rdi_path->en_cfg = 0;
	cam_io_w_mb(rdi_path->en_cfg, core_info->mem_base + client_regs->cfg);

	/*Update TFE mask*/
	core_info->irq_mask0 &= ~rdi_reg_data->subscribe_irq_mask[0];
	core_info->irq_mask1 &= ~rdi_reg_data->subscribe_irq_mask[1];
	ais_tfe_irq_config(core_info, rdi_reg_data->subscribe_irq_mask,
			top_irq_reg->num_registers, false);

	/*Disable bus WR mask*/
	rdi_comp_done_shift = stop_cmd->path +
						bus_hw_info->rdi_client_offset +
						bus_hw_info->comp_done_shift;
	core_info->bus_wr_mask0 &= ~(1 << rdi_comp_done_shift);
	/* Update the composite regupdate mask in bus irq mask*/
	core_info->bus_wr_mask0 &= ~(1 << tfe_out_hw_info->rup_group_id);
	cam_io_w_mb(core_info->bus_wr_mask0,
		core_info->mem_base +
		bus_hw_irq_regs[AIS_TFE_BUS_IRQ_REG0].mask_reg_offset);

	/* issue bus wr reset and wait for reset ack */
	reinit_completion(&vfe_hw->hw_complete);

	/* reset the rdi path */
	cam_io_w_mb((top_hw_info->rdi_path_reset_val << stop_cmd->path),
			core_info->mem_base + top_hw_info->common_reg->global_reset_cmd);

	/* Wait for completion or timeout of 100ms */
	rc = wait_for_completion_timeout(&vfe_hw->hw_complete,
					msecs_to_jiffies(100));
	if (rc) {
		if (rc < 50)
			CAM_WARN(CAM_ISP,
				"System getting overload. Bus WR reset left time %d ms",
				rc);
		rc = 0;
	} else {
		CAM_WARN(CAM_ISP, "Reset Bus WR timeout");
	}

	/*
	 * For now just when ERROR state do reset_rdi to clear IFE overflow error.
	 * TBD: If INIT/AVAILABLE state do reset_rdi, in multi-stream start/stop
	 * test lead frame freeze, need to check further why freeze.
	 */
	if (rdi_path->state == AIS_ISP_RESOURCE_STATE_ERROR)
		ais_tfe_reset_rdi(vfe_hw, stop_cmd->path);

	rdi_path->state = AIS_ISP_RESOURCE_STATE_INIT_HW;

EXIT:
	mutex_unlock(&vfe_hw->hw_mutex);

	return rc;
}

int ais_tfe_read(void *hw_priv, void *read_args, uint32_t arg_size)
{
	return -EPERM;
}

int ais_tfe_write(void *hw_priv, void *write_args, uint32_t arg_size)
{
	return -EPERM;
}

static void ais_tfe_q_bufs_to_hw(struct ais_vfe_hw_core_info *core_info,
		enum ais_ife_output_path_id path)
{
	struct ais_tfe_rdi_output *rdi_path = NULL;
	struct ais_vfe_buffer_t *vfe_buf = NULL;
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	bool is_full = false;
	struct ais_ife_rdi_get_timestamp_args get_ts;
	uint32_t frame_inc = 0;

	rdi_path = &core_info->rdi_out[path];
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	client_regs = &bus_hw_info->bus_client_reg[path];

	is_full = (rdi_path->num_buffer_hw_q >= bus_hw_info->max_fifo_num);

	while (!is_full) {
		if (list_empty(&rdi_path->buffer_q))
			break;

		vfe_buf = list_first_entry(&rdi_path->buffer_q,
				struct ais_vfe_buffer_t, list);
		list_del_init(&vfe_buf->list);

		get_ts.path = path;
		get_ts.ts = &vfe_buf->ts_hw;
		core_info->csid_hw->hw_ops.process_cmd(
			core_info->csid_hw->hw_priv,
			AIS_IFE_CSID_CMD_GET_TIME_STAMP,
			&get_ts,
			sizeof(get_ts));


		CAM_DBG(CAM_ISP, "IFE%d|RDI%d: Q %d(0x%x) FIFO:%d ts %llu",
			core_info->vfe_idx, path,
			vfe_buf->bufIdx, vfe_buf->iova_addr,
			rdi_path->num_buffer_hw_q, vfe_buf->ts_hw.cur_sof_ts);

		frame_inc = rdi_path->stride * rdi_path->height;

		cam_io_w_mb(vfe_buf->iova_addr,
			core_info->mem_base + client_regs->image_addr);

		cam_io_w_mb(frame_inc,
			core_info->mem_base + client_regs->frame_inc);


		list_add_tail(&vfe_buf->list, &rdi_path->buffer_hw_q);
		++rdi_path->num_buffer_hw_q;

		is_full = (rdi_path->num_buffer_hw_q >= bus_hw_info->max_fifo_num);

		//trace_ais_isp_tfe_enq_buf_hw(core_info->vfe_idx, path,
		//	vfe_buf->bufIdx, rdi_path->num_buffer_hw_q, is_full);
	}

	if (rdi_path->num_buffer_hw_q > bus_hw_info->max_fifo_num)
		CAM_WARN(CAM_ISP, "Excessive number of buffers in SW FIFO (%d)",
			rdi_path->num_buffer_hw_q);
}

void ais_tfe_ife_discard_old_frame_done_event(struct ais_vfe_hw_core_info *core_info,
					struct ais_ife_event_data *evt_data)
{
	uint8_t path = 0;
	uint32_t buf_idx = 0;
	struct ais_vfe_buffer_t *vfe_buf = NULL;
	struct ais_vfe_buffer_t *tmp_vfe_buf = NULL;
	struct ais_tfe_rdi_output *rdi_path = NULL;
	int rc = -1;

	if (core_info == NULL || evt_data == NULL)
		return;

	path = evt_data->msg.path;
	buf_idx = evt_data->u.frame_msg.buf_idx;

	if (path >= AIS_IFE_PATH_MAX) {
		CAM_WARN(CAM_ISP, "Invalid path:%d", path);
		return;
	}

	rdi_path = &core_info->rdi_out[path];
	if (rdi_path->state != AIS_ISP_RESOURCE_STATE_STREAMING) {
		CAM_WARN(CAM_ISP, "Not streaming state:%d", rdi_path->state);
		return;
	}

	spin_lock(&rdi_path->buffer_lock);
	if (list_empty(&rdi_path->free_buffer_list)) {
		spin_unlock(&rdi_path->buffer_lock);
		return;
	}

	list_for_each_entry_safe(vfe_buf, tmp_vfe_buf,
				&rdi_path->free_buffer_list, list) {
		if ((vfe_buf->bufIdx == buf_idx) &&
				(vfe_buf->mem_handle != 0) &&
				(vfe_buf->iova_addr != 0) &&
				(vfe_buf->iova_addr >> 32 == 0)) {
			list_del_init(&vfe_buf->list);
			list_add_tail(&vfe_buf->list, &rdi_path->buffer_q);

			rc = 0;
			break;
		}
	}
	spin_unlock(&rdi_path->buffer_lock);

	if (rc == 0 && vfe_buf != NULL) {
		CAM_WARN(CAM_ISP, "I%d|R%d discard old frame done buffer:%d",
				core_info->vfe_idx, path, vfe_buf->bufIdx);
	} else {
		CAM_WARN(CAM_ISP, "I%d|R%d can't find old frame done buffer:%d",
			core_info->vfe_idx, path, buf_idx);
	}
}

static int ais_tfe_cmd_enq_buf(struct ais_vfe_hw_core_info *core_info,
		struct ais_ife_enqueue_buffer_args *enq_buf)
{
	int rc;
	struct ais_vfe_buffer_t *vfe_buf[4] = {};
	struct ais_tfe_rdi_output *rdi_path = NULL;
	int32_t mmu_hdl;
	size_t  src_buf_size;
	uint32_t i = 0;
	uint32_t batch_id = 0;
	uint64_t base_addr = 0;

	if (enq_buf->path >= AIS_IFE_PATH_MAX) {
		CAM_ERR(CAM_ISP, "Invalid output path %d", enq_buf->path);
		rc = -EINVAL;
		goto EXIT;
	}

	rdi_path = &core_info->rdi_out[enq_buf->path];
	if (rdi_path->state < AIS_ISP_RESOURCE_STATE_RESERVED) {
		CAM_ERR(CAM_ISP, "RDI%d invalid state %d", enq_buf->path,
				rdi_path->state);
		rc = -EINVAL;
		goto EXIT;
	}

	spin_lock(&rdi_path->buffer_lock);
	for (batch_id = 0; batch_id < rdi_path->batchConfig.numBatchFrames; batch_id++) {
		if (!list_empty(&rdi_path->free_buffer_list)) {
			vfe_buf[batch_id] = list_first_entry(&rdi_path->free_buffer_list,
				struct ais_vfe_buffer_t, list);
			list_del_init(&vfe_buf[batch_id]->list);
		}
		if (!vfe_buf[batch_id]) {
			CAM_ERR(CAM_ISP, "RDI%d No more free buffers!", enq_buf->path);
			for (i = 0; i < batch_id; i++)
				list_add_tail(&vfe_buf[i]->list, &rdi_path->free_buffer_list);
			spin_unlock(&rdi_path->buffer_lock);
			return -ENOMEM;
		}
	}
	spin_unlock(&rdi_path->buffer_lock);


	vfe_buf[0]->mem_handle = enq_buf->buffer.mem_handle;

	mmu_hdl = core_info->iommu_hdl;

	if (cam_mem_is_secure_buf(vfe_buf[0]->mem_handle) || rdi_path->secure_mode)
		mmu_hdl = core_info->iommu_hdl_secure;

	rc = cam_mem_get_io_buf(vfe_buf[0]->mem_handle,
		mmu_hdl, &vfe_buf[0]->iova_addr, &src_buf_size,
		NULL, NULL);
	if (rc < 0) {
		CAM_ERR(CAM_ISP,
			"get src buf address fail mem_handle 0x%x",
			vfe_buf[0]->mem_handle);
	}
	if (vfe_buf[0]->iova_addr >> 32) {
		CAM_ERR(CAM_ISP, "Invalid mapped address");
		rc = -EINVAL;
	}

	if (enq_buf->buffer.offset >= src_buf_size) {
		CAM_ERR(CAM_ISP, "Invalid buffer offset");
		rc = -EINVAL;
	}

	//if any error, return buffer list object to being free
	if (rc) {
		spin_lock(&rdi_path->buffer_lock);
		for (batch_id = 0; batch_id < rdi_path->batchConfig.numBatchFrames; batch_id++)
			list_add_tail(&vfe_buf[batch_id]->list, &rdi_path->free_buffer_list);
		spin_unlock(&rdi_path->buffer_lock);
	} else {
		base_addr  = vfe_buf[0]->iova_addr + enq_buf->buffer.offset;
		spin_lock(&rdi_path->buffer_lock);
		for (batch_id = 0; batch_id < rdi_path->batchConfig.numBatchFrames; batch_id++) {
			vfe_buf[batch_id]->bufIdx = enq_buf->buffer.idx;

			vfe_buf[batch_id]->iova_addr = base_addr +
				batch_id * rdi_path->batchConfig.frameIncrement;

			vfe_buf[batch_id]->batchId = batch_id;

			//trace_ais_isp_tfe_enq_req(core_info->vfe_idx, enq_buf->path,
			//		enq_buf->buffer.idx);

			list_add_tail(&vfe_buf[batch_id]->list, &rdi_path->buffer_q);
			}
			spin_unlock(&rdi_path->buffer_lock);

			if (rdi_path->state < AIS_ISP_RESOURCE_STATE_STREAMING)
				ais_tfe_q_bufs_to_hw(core_info, enq_buf->path);
	}

EXIT:
	return rc;
}

int ais_tfe_process_cmd(void *hw_priv, uint32_t cmd_type,
	void *cmd_args, uint32_t arg_size)
{
	struct cam_hw_info                *vfe_hw = hw_priv;
	struct cam_hw_soc_info            *soc_info = NULL;
	struct ais_vfe_hw_core_info       *core_info = NULL;
	struct ais_vfe_hw_info            *hw_info = NULL;
	int rc = 0;

	if (!hw_priv) {
		CAM_ERR(CAM_ISP, "Invalid arguments");
		return -EINVAL;
	}

	soc_info = &vfe_hw->soc_info;
	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	hw_info = core_info->vfe_hw_info;

	mutex_lock(&vfe_hw->hw_mutex);

	switch (cmd_type) {
	case AIS_VFE_CMD_ENQ_BUFFER: {
		struct ais_ife_enqueue_buffer_args *enq_buf =
			(struct ais_ife_enqueue_buffer_args *)cmd_args;
		if (arg_size != sizeof(*enq_buf))
			rc = -EINVAL;
		else
			rc = ais_tfe_cmd_enq_buf(core_info, enq_buf);
		break;
	}
	default:
		CAM_ERR(CAM_ISP, "Invalid cmd type:%d", cmd_type);
		rc = -EINVAL;
		break;
	}

	mutex_unlock(&vfe_hw->hw_mutex);

	return rc;
}

static uint8_t ais_tfe_get_num_missed_sof(
	uint64_t cur_sof,
	uint64_t prev_sof,
	uint64_t last_sof,
	uint64_t ts_delta)
{
	uint8_t miss_sof = 0;

	if (prev_sof == last_sof) {
		miss_sof = 0;
	} else if (prev_sof < last_sof) {
		//rollover case
		miss_sof = (int)(((U64_MAX - last_sof) + prev_sof + 1 +
				ts_delta/2) / ts_delta);
	} else {
		miss_sof = (int)((prev_sof - last_sof + ts_delta/2) / ts_delta);
	}

	return miss_sof;
}

static int ais_tfe_q_sof(struct ais_vfe_hw_core_info *core_info,
	enum ais_ife_output_path_id path,
	struct ais_sof_info_t *p_sof)
{
	struct ais_tfe_rdi_output *p_rdi = &core_info->rdi_out[path];
	struct ais_sof_info_t *p_sof_info = NULL;
	int rc = 0;

	if (!list_empty(&p_rdi->free_sof_info_list)) {
		spin_lock_bh(&p_rdi->buffer_lock);
		p_sof_info = list_first_entry(&p_rdi->free_sof_info_list,
			struct ais_sof_info_t, list);
		list_del_init(&p_sof_info->list);
		p_sof_info->frame_cnt = p_sof->frame_cnt;
		p_sof_info->sof_ts = p_sof->sof_ts;
		p_sof_info->cur_sof_hw_ts = p_sof->cur_sof_hw_ts;
		p_sof_info->prev_sof_hw_ts = p_sof->prev_sof_hw_ts;
		list_add_tail(&p_sof_info->list, &p_rdi->sof_info_q);
		p_rdi->num_sof_info_q++;
		spin_unlock_bh(&p_rdi->buffer_lock);

		//trace_ais_isp_tfe_q_sof(core_info->vfe_idx, path,
		//	p_sof->frame_cnt, p_sof->cur_sof_hw_ts);

		CAM_DBG(CAM_ISP, "I%d|R%d|F%llu: sof %llu",
			core_info->vfe_idx, path, p_sof->frame_cnt,
			p_sof_info->cur_sof_hw_ts);
	} else {
		rc = -1;

		CAM_DBG(CAM_ISP,
			"I%d|R%d|F%llu: free timestamp empty (%d) sof %llu",
			core_info->vfe_idx, path, p_sof->frame_cnt,
			p_rdi->num_buffer_hw_q, p_sof->cur_sof_hw_ts);
	}

	return rc;
}


static void ais_tfe_handle_sof_rdi(struct ais_vfe_hw_core_info *core_info,
		struct ais_vfe_hw_work_data *work_data,
		enum ais_ife_output_path_id path)
{
	struct ais_tfe_rdi_output *p_rdi = &core_info->rdi_out[path];
	uint64_t cur_sof_hw_ts = work_data->ts_hw[path].cur_sof_ts;
	uint64_t prev_sof_hw_ts = work_data->ts_hw[path].prev_sof_ts;

	p_rdi->frame_cnt++;
	core_info->event.msg.reserved = sizeof(struct ais_ife_event_data);
	if (p_rdi->num_buffer_hw_q) {
		struct ais_sof_info_t sof = {};
		uint64_t ts_delta;
		uint8_t miss_sof = 0;

		if (cur_sof_hw_ts < prev_sof_hw_ts)
			ts_delta = cur_sof_hw_ts +
				(U64_MAX - prev_sof_hw_ts);
		else
			ts_delta = cur_sof_hw_ts - prev_sof_hw_ts;


		//check any missing SOFs
		if (p_rdi->frame_cnt > 1) {
			if (ts_delta == 0) {
				CAM_ERR(CAM_ISP, "IFE%d RDI%d ts_delta is 0",
						core_info->vfe_idx, path);
			} else {
				miss_sof = ais_tfe_get_num_missed_sof(
					cur_sof_hw_ts,
					prev_sof_hw_ts,
					p_rdi->last_sof_info.cur_sof_hw_ts,
					ts_delta);

				CAM_DBG(CAM_ISP,
					"I%d R%d miss_sof %u prev %llu last %llu cur %llu",
					core_info->vfe_idx, path,
					miss_sof, prev_sof_hw_ts,
					p_rdi->last_sof_info.cur_sof_hw_ts,
					cur_sof_hw_ts);
			}
		}

		//trace_ais_isp_tfe_sof(core_info->vfe_idx, path,
		//		&work_data->ts_hw[path],
		//		p_rdi->num_buffer_hw_q, miss_sof);

		if (p_rdi->frame_cnt == 1 && prev_sof_hw_ts != 0) {
			//enq missed first frame
			sof.sof_ts = work_data->ts;
			sof.cur_sof_hw_ts = prev_sof_hw_ts;
			sof.frame_cnt = p_rdi->frame_cnt++;

			ais_tfe_q_sof(core_info, path, &sof);
		} else if (miss_sof > 0) {
			if (miss_sof > 1) {
				int i = 0;
				int miss_idx = miss_sof - 1;

				for (i = 0; i < (miss_sof - 1); i++) {

					sof.sof_ts = work_data->ts;
					sof.cur_sof_hw_ts = prev_sof_hw_ts -
						(ts_delta * miss_idx);
					sof.frame_cnt = p_rdi->frame_cnt++;

					ais_tfe_q_sof(core_info, path, &sof);

					miss_idx--;
				}
			}

			//enq prev
			sof.sof_ts = work_data->ts;
			sof.cur_sof_hw_ts = prev_sof_hw_ts;
			sof.frame_cnt = p_rdi->frame_cnt++;

			ais_tfe_q_sof(core_info, path, &sof);
		}

		//enq curr
		sof.sof_ts = work_data->ts;
		sof.cur_sof_hw_ts = cur_sof_hw_ts;
		sof.frame_cnt = p_rdi->frame_cnt;

		ais_tfe_q_sof(core_info, path, &sof);

	} else {
		//trace_ais_isp_tfe_sof(core_info->vfe_idx, path,
		//			&work_data->ts_hw[path],
		//			p_rdi->num_buffer_hw_q, 0);

		CAM_DBG(CAM_ISP, "I%d R%d Flush SOF (%d) HW Q empty",
				core_info->vfe_idx, path,
				p_rdi->num_sof_info_q);

		if (p_rdi->num_sof_info_q) {
			struct ais_sof_info_t *p_sof_info;

			spin_lock_bh(&p_rdi->buffer_lock);
			while (!list_empty(&p_rdi->sof_info_q)) {
				p_sof_info = list_first_entry(
					&p_rdi->sof_info_q,
					struct ais_sof_info_t, list);
				list_del_init(&p_sof_info->list);
				list_add_tail(&p_sof_info->list,
						&p_rdi->free_sof_info_list);
			}
			p_rdi->num_sof_info_q = 0;
			spin_unlock_bh(&p_rdi->buffer_lock);
		}

		//trace_ais_isp_tfe_error(core_info->vfe_idx,
		//				path, 1, 0);

		//send warning
		core_info->event.msg.type = AIS_IFE_MSG_OUTPUT_WARNING;
		core_info->event.msg.path = path;
		core_info->event.u.err_msg.reserved = 0;

		core_info->event_cb(core_info->event_cb_priv,
			&core_info->event);

	}

	p_rdi->last_sof_info.cur_sof_hw_ts = cur_sof_hw_ts;

	//send sof only for current frame
	core_info->event.msg.type = AIS_IFE_MSG_SOF;
	core_info->event.msg.path = path;
	core_info->event.msg.frame_id = p_rdi->frame_cnt;
	core_info->event.u.sof_msg.hw_ts = cur_sof_hw_ts;

	core_info->event_cb(core_info->event_cb_priv,
		&core_info->event);

}

static int ais_tfe_handle_sof(
	struct ais_vfe_hw_core_info *core_info,
	struct ais_vfe_hw_work_data *work_data)
{
	struct ais_tfe_rdi_output *p_rdi;
	int path =  0;
	int rc = 0;

	CAM_DBG(CAM_ISP, "IFE%d SOF RDIs 0x%x", core_info->vfe_idx,
			work_data->path);
	for (path = 0; path < AIS_IFE_PATH_MAX; path++) {

		if (!(work_data->path & (1 << path)))
			continue;

		p_rdi = &core_info->rdi_out[path];
		if (p_rdi->state != AIS_ISP_RESOURCE_STATE_STREAMING)
			continue;

		//AIS_ATRACE_BEGIN("SOF_%u_%u_%lu",
		//	core_info->vfe_idx, path, p_rdi->frame_cnt);
		ais_tfe_handle_sof_rdi(core_info, work_data, path);
		//AIS_ATRACE_END("SOF_%u_%u_%lu",
		//	core_info->vfe_idx, path, p_rdi->frame_cnt);

		//enq buffers
		spin_lock_bh(&p_rdi->buffer_lock);
		ais_tfe_q_bufs_to_hw(core_info, path);
		spin_unlock_bh(&p_rdi->buffer_lock);
	}

	return rc;
}

static int ais_tfe_handle_error(
	struct ais_vfe_hw_core_info *core_info,
	struct ais_vfe_hw_work_data *work_data)
{
	struct ais_tfe_top_ver2_hw_info   *top_hw_info = NULL;
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_irq_register_set       *bus_hw_irq_regs = NULL;
	struct ais_vfe_rdi_reg_data       *rdi_reg_data = NULL;
	struct ais_tfe_rdi_ver2_hw_info   *rdi_hw_info = NULL;
	struct ais_tfe_rdi_reg            *rdi_reg = NULL;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	struct ais_tfe_bus_ver2_vfe_out_hw_info   *tfe_out_hw_info = NULL;
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	struct ais_tfe_rdi_output *p_rdi;
	uint32_t rdi_comp_done_shift = 0;
	uint32_t val = 0;
	int path =  0;
	int rc = 0;

	CAM_ERR(CAM_ISP, "IFE%d ERROR on RDIs 0x%x", core_info->vfe_idx,
					work_data->path);

	//trace_ais_isp_tfe_error(core_info->vfe_idx,
	//		work_data->path, 0, 0);

	top_hw_info = core_info->vfe_hw_info->top_hw_info;
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	rdi_hw_info = &top_hw_info->rdi_hw_info;
	bus_hw_irq_regs = bus_hw_info->common_reg.irq_reg_info.irq_reg_set;

	for (path = 0; path < top_irq_reg->num_registers; path++) {

		if (!(work_data->path & (1 << path)))
			continue;

		p_rdi = &core_info->rdi_out[path];

		if (p_rdi->state != AIS_ISP_RESOURCE_STATE_STREAMING)
			continue;

		CAM_ERR(CAM_ISP, "IFE%d Turn off RDI %d",
                       core_info->vfe_idx, path);

		p_rdi->state = AIS_ISP_RESOURCE_STATE_ERROR;

		client_regs = &bus_hw_info->bus_client_reg[path];
		rdi_reg_data = rdi_hw_info->reg_data[path];
		rdi_reg = rdi_hw_info->rdi_reg[path];
		tfe_out_hw_info = &bus_hw_info->vfe_out_hw_info[path];

		rdi_comp_done_shift = path + bus_hw_info->rdi_client_offset +
			bus_hw_info->comp_done_shift;

		core_info->bus_wr_mask0 &= ~(1 << rdi_comp_done_shift);
		/* Update the composite regupdate mask in bus irq mask*/
		core_info->bus_wr_mask0 &= ~(1 << tfe_out_hw_info->rup_group_id);
		cam_io_w_mb(core_info->bus_wr_mask0,
			core_info->mem_base +
			bus_hw_irq_regs[AIS_TFE_BUS_IRQ_REG0].mask_reg_offset);

		/* Disable WM and reg-update */
		p_rdi->en_cfg = 0;
		cam_io_w_mb(p_rdi->en_cfg, core_info->mem_base + client_regs->cfg);

		val = cam_io_r_mb(core_info->mem_base + rdi_reg->reg_update_cmd);
		val &= ~rdi_reg_data->reg_update_cmd_data;
		cam_io_w_mb(val, core_info->mem_base + rdi_reg->reg_update_cmd);

		/* reset the rdi path */
		cam_io_w_mb((top_hw_info->rdi_path_reset_val << path),
			core_info->mem_base + top_hw_info->common_reg->global_reset_cmd);

		core_info->event.msg.type = AIS_IFE_MSG_OUTPUT_ERROR;
		core_info->event.msg.path = path;
		core_info->event.msg.reserved = sizeof(struct ais_ife_event_data);

		core_info->event_cb(core_info->event_cb_priv,
				&core_info->event);
	}

	return rc;
}

static void ais_tfe_bus_handle_client_frame_done(
	struct ais_vfe_hw_core_info *core_info,
	enum ais_ife_output_path_id path,
	uint32_t last_addr)
{
	struct ais_tfe_rdi_output         *rdi_path = NULL;
	struct ais_vfe_buffer_t           *vfe_buf = NULL;
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	uint64_t                           frame_cnt = 0;
	uint64_t                           sof_ts;
	uint64_t                           cur_sof_hw_ts;
	bool last_addr_match = false;
	uint32_t i = 0;

	CAM_DBG(CAM_ISP, "I%d|R%d last_addr 0x%x",
			core_info->vfe_idx, path, last_addr);

	if (last_addr == 0) {
		CAM_ERR(CAM_ISP, "I%d|R%d null last_addr",
				core_info->vfe_idx, path);
		return;
	}

	rdi_path = &core_info->rdi_out[path];
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;

	core_info->event.msg.type = AIS_IFE_MSG_FRAME_DONE;
	core_info->event.msg.path = path;
	core_info->event.msg.reserved = sizeof(struct ais_ife_event_data);

	while (rdi_path->num_buffer_hw_q && !last_addr_match) {
		struct ais_sof_info_t *p_sof_info = NULL;
		bool is_sof_match = false;

		spin_lock_bh(&rdi_path->buffer_lock);
		if (list_empty(&rdi_path->buffer_hw_q)) {
			CAM_DBG(CAM_ISP, "I%d|R%d: FD while HW Q empty",
				core_info->vfe_idx, path);
			spin_unlock_bh(&rdi_path->buffer_lock);
			break;
		}

		vfe_buf = list_first_entry(&rdi_path->buffer_hw_q,
				struct ais_vfe_buffer_t, list);
		list_del_init(&vfe_buf->list);
		--rdi_path->num_buffer_hw_q;

		if (last_addr == vfe_buf->iova_addr)
			last_addr_match = true;
		else
			CAM_WARN(CAM_ISP, "IFE%d buf %d did not match addr",
				core_info->vfe_idx, vfe_buf->bufIdx);

		CAM_DBG(CAM_ISP, "I%d|R%d BUF DQ %d (0x%x) FIFO:%d|0x%x",
			core_info->vfe_idx, path,
			vfe_buf->bufIdx, vfe_buf->iova_addr,
			rdi_path->num_buffer_hw_q, last_addr);

		if (!list_empty(&rdi_path->sof_info_q)) {
			while (!is_sof_match &&
				!list_empty(&rdi_path->sof_info_q)) {
				p_sof_info =
					list_first_entry(&rdi_path->sof_info_q,
						struct ais_sof_info_t, list);
				list_del_init(&p_sof_info->list);
				rdi_path->num_sof_info_q--;
				if (p_sof_info->cur_sof_hw_ts >
					vfe_buf->ts_hw.cur_sof_ts) {
					is_sof_match = true;
					break;
				}
				list_add_tail(&p_sof_info->list,
					&rdi_path->free_sof_info_list);
			}

			if (!is_sof_match) {
				p_sof_info = NULL;
				CAM_ERR(CAM_ISP,
					"I%d|R%d: can't find the match sof",
					core_info->vfe_idx, path);
			}

		} else
			CAM_ERR(CAM_ISP, "I%d|R%d: SOF info Q is empty",
				core_info->vfe_idx, path);

		if (p_sof_info) {
			frame_cnt = p_sof_info->frame_cnt;
			sof_ts = p_sof_info->sof_ts;
			cur_sof_hw_ts = p_sof_info->cur_sof_hw_ts;
			list_add_tail(&p_sof_info->list,
					&rdi_path->free_sof_info_list);
		} else {
			frame_cnt = sof_ts = cur_sof_hw_ts = 0;
		}

		list_add_tail(&vfe_buf->list, &rdi_path->free_buffer_list);
		spin_unlock_bh(&rdi_path->buffer_lock);

		//trace_ais_isp_tfe_buf_done(core_info->vfe_idx, path,
		//		vfe_buf->bufIdx,
		//		frame_cnt,
		//		rdi_path->num_buffer_hw_q,
		//		last_addr_match);

		rdi_path->batchFrameInfo[vfe_buf->batchId].batchId = vfe_buf->batchId;
		rdi_path->batchFrameInfo[vfe_buf->batchId].frameId = frame_cnt;
		rdi_path->batchFrameInfo[vfe_buf->batchId].hwTimestamp = cur_sof_hw_ts;

		if (vfe_buf->batchId == (rdi_path->batchConfig.numBatchFrames - 1)) {
			core_info->event.u.frame_msg.buf_idx = vfe_buf->bufIdx;
			core_info->event.u.frame_msg.num_batch_frames =
				rdi_path->batchConfig.numBatchFrames;
			core_info->event.u.frame_msg.ts = sof_ts;
			core_info->event.msg.frame_id =
				rdi_path->batchFrameInfo[i].frameId;
			for (i = 0; i < rdi_path->batchConfig.numBatchFrames; i++) {
				core_info->event.u.frame_msg.hw_ts[i] =
					rdi_path->batchFrameInfo[i].hwTimestamp;
			}
			core_info->event_cb(core_info->event_cb_priv,
					&core_info->event);

			CAM_DBG(CAM_ISP, "I%d|R%d|F%llu: si [%llu, %llu]",
				core_info->vfe_idx, path,
				core_info->event.msg.frame_id,
				sof_ts,
				core_info->event.u.frame_msg.hw_ts[0]);
		}
	}

	if (!last_addr_match) {
		CAM_ERR(CAM_ISP, "IFE%d BUF| RDI%d NO MATCH addr 0x%x",
			core_info->vfe_idx, path, last_addr);

		//trace_ais_isp_tfe_error(core_info->vfe_idx, path, 1, 1);

		//send warning
		core_info->event.msg.type = AIS_IFE_MSG_OUTPUT_WARNING;
		core_info->event.msg.path = path;
		core_info->event.u.err_msg.reserved = 1;

		core_info->event_cb(core_info->event_cb_priv,
			&core_info->event);
	}

	/* Flush SOF info Q if HW Buffer Q is empty */
	if (rdi_path->num_buffer_hw_q == 0) {
		struct ais_sof_info_t *p_sof_info = NULL;

		CAM_DBG(CAM_ISP, "I%d|R%d|F%llu: Flush SOF (%d) HW Q empty",
			core_info->vfe_idx, path, frame_cnt,
			rdi_path->num_sof_info_q);

		spin_lock_bh(&rdi_path->buffer_lock);
		while (!list_empty(&rdi_path->sof_info_q)) {
			p_sof_info = list_first_entry(&rdi_path->sof_info_q,
					struct ais_sof_info_t, list);
			list_del_init(&p_sof_info->list);
			list_add_tail(&p_sof_info->list,
				&rdi_path->free_sof_info_list);
		}

		rdi_path->num_sof_info_q = 0;
		spin_unlock_bh(&rdi_path->buffer_lock);

		//trace_ais_isp_tfe_error(core_info->vfe_idx, path, 1, 0);

		//send warning
		core_info->event.msg.type = AIS_IFE_MSG_OUTPUT_WARNING;
		core_info->event.msg.path = path;
		core_info->event.u.err_msg.reserved = 0;

		core_info->event_cb(core_info->event_cb_priv,
			&core_info->event);
	}

	spin_lock_bh(&rdi_path->buffer_lock);

	ais_tfe_q_bufs_to_hw(core_info, path);

	spin_unlock_bh(&rdi_path->buffer_lock);
}

static int ais_tfe_bus_handle_frame_done(
	struct ais_vfe_hw_core_info *core_info,
	struct ais_vfe_hw_work_data *work_data)
{
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_tfe_rdi_output *p_rdi = &core_info->rdi_out[0];
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	uint32_t client_mask = work_data->bus_wr_status[0];
	uint32_t client;
	int rc = 0;

	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;

	CAM_DBG(CAM_ISP, "TFE%d Frame Done clients 0x%x",
		core_info->vfe_idx, client_mask);

	for (client = 0 ; client < AIS_IFE_PATH_MAX; client++) {
		p_rdi = &core_info->rdi_out[client];

		if (p_rdi->state != AIS_ISP_RESOURCE_STATE_STREAMING)
			continue;

		client_regs = &bus_hw_info->bus_client_reg[client];
		if (client_mask &
				(0x1 << (client_regs->comp_group +
					 bus_hw_info->comp_done_shift))) {
			//process frame done
			//AIS_ATRACE_BEGIN("FD_%u_%u_%lu",
			//	core_info->vfe_idx, client, p_rdi->frame_cnt);
			ais_tfe_bus_handle_client_frame_done(core_info,
				client, work_data->last_addr[client]);
			//AIS_ATRACE_END("FD_%u_%u_%lu",
			//	core_info->vfe_idx, client, p_rdi->frame_cnt);
		}
	}

	return rc;
}

static int ais_tfe_irq_fill_bus_wr_status(
	struct ais_vfe_hw_core_info *core_info,
	struct ais_vfe_hw_work_data *work_data)
{
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_irq_register_set       *bus_hw_irq_regs = NULL;
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	uint32_t client = 0;
	int buf_done_trigger = 1;

	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	bus_hw_irq_regs = bus_hw_info->common_reg.irq_reg_info.irq_reg_set;

	if (work_data->bus_wr_status[0] &
					bus_hw_info->comp_buf_done_mask) {
		struct ais_tfe_rdi_output *p_rdi;

		for (client = 0 ; client < AIS_IFE_PATH_MAX; client++) {
			client_regs = &bus_hw_info->bus_client_reg[client];
			if (work_data->bus_wr_status[0] &
					(0x1 << (client_regs->comp_group +
					 bus_hw_info->comp_done_shift))) {
				p_rdi = &core_info->rdi_out[client];
				client_regs =
					&bus_hw_info->bus_client_reg[client];
				work_data->last_addr[client] = cam_io_r(
					core_info->mem_base + client_regs->status0);
				buf_done_trigger = 0;
			}
		}
	}

	return buf_done_trigger;
}

static int ais_tfe_handle_bus_wr_irq(struct cam_hw_info *vfe_hw,
	struct ais_vfe_hw_core_info *core_info,
	struct ais_vfe_hw_work_data *work_data)
{
	int rc = 0;
	struct ais_tfe_bus_ver2_hw_info   *bus_hw_info = NULL;
	struct ais_irq_register_set       *bus_hw_irq_regs = NULL;
	struct ais_tfe_bus_reg_offset_bus_client  *client_regs = NULL;
	uint32_t client = 0;

	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	bus_hw_irq_regs = bus_hw_info->common_reg.irq_reg_info.irq_reg_set;
	client_regs = &bus_hw_info->bus_client_reg[client];


	CAM_DBG(CAM_ISP, "TFE%d BUS status 0x%x 0x%x", core_info->vfe_idx,
		work_data->bus_wr_status[0],
		work_data->bus_wr_status[1]);

	if (work_data->bus_wr_status[0] & bus_hw_info->comp_buf_done_mask) {
		//AIS_ATRACE_BEGIN("FD_%d", core_info->vfe_idx);
		ais_tfe_bus_handle_frame_done(core_info, work_data);
		//AIS_ATRACE_END("FD_%d", core_info->vfe_idx);
	}

#if 0
	if (work_data->bus_wr_status[0] & AIS_VFE_BUS_STATUS0_VIOLATION) {
		CAM_ERR(CAM_ISP, "TFE%d: WR BUS violation status = 0x%x",
			core_info->vfe_idx, work_data->bus_wr_status[0]);
		work_data->path = 0xF;
		rc = ais_tfe_handle_error(core_info, work_data);
	}

	if (work_data->bus_wr_status[0] & 0x1) {
		CAM_DBG(CAM_ISP, "TFE%d: WR BUS reset completed",
			core_info->vfe_idx);
		complete(&vfe_hw->hw_complete);
	}
#endif

	return rc;
}

static int ais_tfe_process_irq_bh(void *priv, void *data)
{
	struct ais_vfe_hw_work_data   *work_data;
	struct cam_hw_info            *vfe_hw;
	struct ais_vfe_hw_core_info   *core_info;
	int rc = 0;

	vfe_hw = (struct cam_hw_info *)priv;
	if (!vfe_hw) {
		CAM_ERR(CAM_ISP, "Invalid parameters");
		return -EINVAL;
	}

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	if (!core_info->event_cb) {
		CAM_ERR(CAM_ISP, "hw_idx %d Error Cb not registered",
			core_info->vfe_idx);
		return -EINVAL;
	}

	work_data = (struct ais_vfe_hw_work_data *)data;

	//trace_ais_isp_irq_process(core_info->vfe_idx, work_data->evt_type, 1);
	CAM_DBG(CAM_ISP, "TFE[%d] event %d",
		core_info->vfe_idx, work_data->evt_type);

	core_info->event.msg.idx = core_info->vfe_idx;
	core_info->event.msg.boot_ts = work_data->ts;

	switch (work_data->evt_type) {
	case AIS_VFE_HW_IRQ_EVENT_SOF:
		//AIS_ATRACE_BEGIN("SOF_%d", core_info->vfe_idx);
		rc = ais_tfe_handle_sof(core_info, work_data);
		//AIS_ATRACE_END("SOF_%d", core_info->vfe_idx);
		break;
	case AIS_VFE_HW_IRQ_EVENT_BUS_WR:
		rc = ais_tfe_handle_bus_wr_irq(vfe_hw, core_info, work_data);
		break;
	case AIS_VFE_HW_IRQ_EVENT_ERROR:
		rc = ais_tfe_handle_error(core_info, work_data);
		break;
	default:
		CAM_ERR(CAM_ISP, "TFE[%d] invalid event type %d",
			core_info->vfe_idx, work_data->evt_type);
		break;
	}

	//trace_ais_isp_irq_process(core_info->vfe_idx, work_data->evt_type, 2);

	return rc;
}

static bool ais_tfe_irq_cancel_task_filter(void *priv, void *data)
{
	struct ais_vfe_hw_work_data   *work_data;
	struct cam_hw_info            *vfe_hw;
	struct ais_vfe_hw_core_info   *core_info;
	bool is_discard = false;

	if (priv == NULL || data == NULL)
		return true;

	vfe_hw = (struct cam_hw_info *)priv;
	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	if (!core_info)
		return true;

	work_data = (struct ais_vfe_hw_work_data *)data;
	if (work_data->evt_type == AIS_VFE_HW_IRQ_EVENT_SOF)
		is_discard = true;

	return is_discard;
}

static int ais_tfe_dispatch_irq(struct cam_hw_info *vfe_hw,
		struct ais_vfe_hw_work_data *p_work)
{
	struct ais_vfe_hw_core_info *core_info;
	struct ais_vfe_hw_work_data *work_data;
	struct crm_workq_task *task;
	int rc = 0;

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;

	CAM_DBG(CAM_ISP, "TFE[%d] event %d",
		core_info->vfe_idx, p_work->evt_type);

	task = cam_req_mgr_workq_get_task(core_info->workq);
	if (!task) {
		CAM_ERR_RATE_LIMIT(CAM_ISP, "I%d Can not get task for worker, cancel SOF evt", core_info->vfe_idx);

		cam_req_mgr_workq_cancel_task(core_info->workq,
						ais_tfe_irq_cancel_task_filter);

		if (p_work->evt_type == AIS_VFE_HW_IRQ_EVENT_SOF) {
			CAM_DBG(CAM_ISP, "I%d So discard this SOF event", core_info->vfe_idx);
			return -ENOMEM;
		}

		task = cam_req_mgr_workq_get_task(core_info->workq);
		if (!task) {
			CAM_ERR_RATE_LIMIT(CAM_ISP, "I%d Still can not get task for worker", core_info->vfe_idx);
			return -ENOMEM;
		}
	}
	work_data = (struct ais_vfe_hw_work_data *)task->payload;
	*work_data = *p_work;

	//trace_ais_isp_irq_process(core_info->vfe_idx, p_work->evt_type, 0);

	task->process_cb = ais_tfe_process_irq_bh;
	rc = cam_req_mgr_workq_enqueue_task(task, vfe_hw,
		CRM_TASK_PRIORITY_0);

	return rc;
}

static int ais_tfe_check_top_irq_error(
		struct cam_hw_info *vfe_hw,
		uint32_t *top_irq_status, uint32_t *bus_irq_status)
{
	struct ais_vfe_hw_core_info   *core_info;
	struct ais_tfe_top_ver2_hw_info    *top_hw_info = NULL;
	struct ais_tfe_bus_ver2_hw_info    *bus_hw_info = NULL;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	struct ais_vfe_rdi_reg_data        *rdi_reg_data = NULL;
	struct ais_tfe_rdi_ver2_hw_info    *rdi_hw_info = NULL;
	struct ais_vfe_hw_work_data work_data;
	struct timespec64 ts;
	uint32_t path = 0;

	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	top_hw_info = core_info->vfe_hw_info->top_hw_info;
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;
	rdi_hw_info = &top_hw_info->rdi_hw_info;

	ktime_get_boottime_ts64(&ts);
	work_data.ts =
		(uint64_t)((ts.tv_sec * 1000000000) + ts.tv_nsec);

	if ((top_irq_status[0] & top_hw_info->error_irq_mask[0]) ||
		(top_irq_status[2] & top_hw_info->error_irq_mask[2]) ||
		(bus_irq_status[0] & bus_hw_info->bus_error_irq_mask[0])) {
		CAM_ERR(CAM_ISP,
			"Encountered Error: tfe:%d: Irq_status0=0x%x status2=0x%x",
			core_info->vfe_idx, top_irq_status[0],
			top_irq_status[2]);
		CAM_ERR(CAM_ISP,
			"Encountered Error: tfe:%d:BUS Irq_status0=0x%x",
			core_info->vfe_idx, bus_irq_status[0]);

		if (top_irq_status[0] & top_hw_info->rdi_overflow_mask) {
			work_data.path = (top_irq_status[0] &
						top_hw_info->rdi_overflow_mask) >>
						top_hw_info->rdi_overflow_shift;

			for (path = 0; path < top_irq_reg->num_registers; path++) {

				if (!(work_data.path & (1 << path)))
					continue;

				rdi_reg_data = rdi_hw_info->reg_data[path];

				/* Disable rdi* overflow irq mask*/
				core_info->irq_mask0 &= ~rdi_reg_data->subscribe_irq_mask[0];
				cam_io_w_mb(core_info->irq_mask0,
						core_info->mem_base +
						top_irq_reg->irq_reg_set[0].mask_reg_offset);

				/* Print RDI Overflow Error Info*/
				core_info->csid_hw->hw_ops.process_cmd(
					core_info->csid_hw->hw_priv,
					AIS_IFE_CSID_CMD_OVERFLOW_INFO,
					&path, sizeof(path));
			}

			CAM_ERR_RATE_LIMIT(CAM_ISP, "IFE%d Overflow 0x%x",
					core_info->vfe_idx, work_data.path);
			work_data.evt_type = AIS_VFE_HW_IRQ_EVENT_ERROR;
			ais_tfe_dispatch_irq(vfe_hw, &work_data);
		}
	}

	return 0;
}

irqreturn_t ais_tfe_irq(int irq_num, void *data)
{
	struct cam_hw_info            *vfe_hw;
	struct ais_vfe_hw_core_info   *core_info;
	struct ais_irq_controller_reg_info *top_irq_reg = NULL;
	struct ais_irq_controller_reg_info *bus_irq_reg = NULL;
	struct ais_tfe_rdi_ver2_hw_info    *rdi_hw_info = NULL;
	struct ais_tfe_top_ver2_hw_info    *top_hw_info = NULL;
	struct ais_tfe_bus_ver2_hw_info    *bus_hw_info = NULL;
	uint32_t ife_status[3] = {};
	uint32_t bus_status[2] = {};
	uint32_t ccif_violation = 0;
	uint32_t overflow_status = 0;
	uint32_t image_sz_violation = 0;
	int i = 0;

	if (!data)
		return IRQ_NONE;

	vfe_hw = (struct cam_hw_info *)data;
	core_info = (struct ais_vfe_hw_core_info *)vfe_hw->core_info;
	top_hw_info = core_info->vfe_hw_info->top_hw_info;
	top_irq_reg = core_info->vfe_hw_info->irq_reg_info;
	bus_hw_info = core_info->vfe_hw_info->bus_hw_info;
	bus_irq_reg = &bus_hw_info->common_reg.irq_reg_info;
	rdi_hw_info = &top_hw_info->rdi_hw_info;

	/* Read and Clear all IFE status regs */
	for (i = 0; i < top_irq_reg->num_registers; i++)
		ife_status[i] = cam_io_r_mb(core_info->mem_base +
				top_irq_reg->irq_reg_set[i].status_reg_offset);

	for (i = 0; i < top_irq_reg->num_registers; i++)
		cam_io_w_mb(ife_status[i],
				core_info->mem_base +
				top_irq_reg->irq_reg_set[i].clear_reg_offset);

	cam_io_w_mb(top_irq_reg->global_clear_bitmask,
				core_info->mem_base + top_irq_reg->global_clear_offset);

	//trace_ais_isp_tfe_irq_activated(core_info->vfe_idx,
	//		ife_status[0], ife_status[1]);
	CAM_DBG(CAM_ISP, "TFE%d TOP IRQ status 0x%x 0x%x 0x%x", core_info->vfe_idx,
			ife_status[0], ife_status[1], ife_status[2]);

	if (ife_status[0] & top_hw_info->wr_bus_mask) {
		//BUS_WR IRQ
		for (i = 0; i < bus_irq_reg->num_registers; i++)
			bus_status[i] = cam_io_r_mb(core_info->mem_base +
								bus_irq_reg->irq_reg_set[i].status_reg_offset);

		for (i = 0; i < bus_irq_reg->num_registers; i++)
			cam_io_w_mb(bus_status[i],
					core_info->mem_base +
					bus_irq_reg->irq_reg_set[i].clear_reg_offset);

		ccif_violation = cam_io_r_mb(core_info->mem_base +
							bus_hw_info->common_reg.bus_violation_reg);
		overflow_status = cam_io_r_mb(core_info->mem_base +
							bus_hw_info->common_reg.bus_overflow_reg);
		image_sz_violation = cam_io_r_mb(core_info->mem_base +
							bus_hw_info->common_reg.bus_image_size_vilation_reg);

		cam_io_w_mb(bus_irq_reg->global_clear_bitmask,
				core_info->mem_base + bus_irq_reg->global_clear_offset);

		CAM_DBG(CAM_ISP, "TFE%d BUS IRQ status 0x%x 0x%x",
				core_info->vfe_idx,
				bus_status[0],
				bus_status[1]);
	}

	/* check reset */
	if ((ife_status[0] & top_hw_info->top_irq_reset_mask[0]) ||
		(ife_status[1] & top_hw_info->top_irq_reset_mask[1]) ||
		(ife_status[2] & top_hw_info->top_irq_reset_mask[1])) {
		CAM_DBG(CAM_ISP, "TFE%d reset Success!", core_info->vfe_idx);
		complete(&vfe_hw->hw_complete);
		return IRQ_HANDLED;
	} else {
		struct ais_ife_rdi_get_timestamp_args get_ts;
		struct ais_vfe_hw_work_data work_data;
		struct timespec64 ts;
		int i;

		ktime_get_boottime_ts64(&ts);
		work_data.ts =
			(uint64_t)((ts.tv_sec * 1000000000) + ts.tv_nsec);

		if (ife_status[1] & top_hw_info->sof_irq_mask) {
			//RDI SOF
			//fill HW timestamp for each RDI path
			for (i = 0; i < bus_hw_info->num_client; i++) {
				if (!(ife_status[1] & rdi_hw_info->reg_data[i]->sof_irq_mask))
					continue;

				work_data.path |= (1 << i);

				get_ts.path = i;
				get_ts.ts = &work_data.ts_hw[i];
				core_info->csid_hw->hw_ops.process_cmd(
					core_info->csid_hw->hw_priv,
					AIS_IFE_CSID_CMD_GET_TIME_STAMP,
					&get_ts,
					sizeof(get_ts));
			}

			CAM_DBG(CAM_ISP, "IFE%d SOF 0x%x",
				core_info->vfe_idx, work_data.path);

			work_data.evt_type = AIS_VFE_HW_IRQ_EVENT_SOF;
			ais_tfe_dispatch_irq(vfe_hw, &work_data);
		}

		if (ife_status[0] & top_hw_info->wr_bus_mask) {
			int rc = 0;
			for (i = 0; i < bus_irq_reg->num_registers; i++)
				work_data.bus_wr_status[i] = bus_status[i];

			work_data.evt_type = AIS_VFE_HW_IRQ_EVENT_BUS_WR;
			rc = ais_tfe_irq_fill_bus_wr_status(core_info, &work_data);
			/* buf done trigger*/
			if (!rc)
				ais_tfe_dispatch_irq(vfe_hw, &work_data);
		}

		ais_tfe_check_top_irq_error(vfe_hw, ife_status, bus_status);

	}

	return IRQ_HANDLED;
}

static void cam_req_mgr_process_workq_cam_tfe_worker(struct work_struct *w)
{
	cam_req_mgr_process_workq(w);
}

int ais_tfe_core_init(struct ais_vfe_hw_core_info  *core_info,
	struct cam_hw_soc_info                     *soc_info,
	struct cam_hw_intf                         *hw_intf,
	struct ais_vfe_hw_info                     *vfe_hw_info)
{
	int rc = 0;
	int i;
	char worker_name[128];

	CAM_DBG(CAM_ISP, "Enter");

	core_info->vfe_idx = soc_info->index;
	core_info->mem_base =
		CAM_SOC_GET_REG_MAP_START(soc_info, VFE_CORE_BASE_IDX);

	spin_lock_init(&core_info->spin_lock);

	for (i = 0; i < AIS_IFE_PATH_MAX; i++) {
		struct ais_tfe_rdi_output *p_rdi = &core_info->rdi_out[i];

		spin_lock_init(&p_rdi->buffer_lock);
		ais_clear_rdi_path(p_rdi);
		p_rdi->state = AIS_ISP_RESOURCE_STATE_AVAILABLE;
	}


	scnprintf(worker_name, sizeof(worker_name),
		"vfe%u_worker", core_info->vfe_idx);
	CAM_DBG(CAM_ISP, "Create TFE worker %s", worker_name);
	rc = cam_req_mgr_workq_create(worker_name,
		AIS_VFE_WORKQ_NUM_TASK,
		&core_info->workq, CRM_WORKQ_USAGE_IRQ, 0,
		cam_req_mgr_process_workq_cam_tfe_worker);
	if (rc) {
		CAM_ERR(CAM_ISP, "Unable to create a workq, rc=%d", rc);
		goto EXIT;
	}

	for (i = 0; i < AIS_VFE_WORKQ_NUM_TASK; i++)
		core_info->workq->task.pool[i].payload =
			&core_info->work_data[i];

EXIT:
	return rc;
}

int ais_tfe_core_deinit(struct ais_vfe_hw_core_info  *core_info,
	struct ais_vfe_hw_info                       *vfe_hw_info)
{
	int                rc = -EINVAL;
	int                i;
	unsigned long      flags;

	spin_lock_irqsave(&core_info->spin_lock, flags);

	cam_req_mgr_workq_destroy(&core_info->workq);

	for (i = 0; i < AIS_IFE_PATH_MAX; i++) {
		struct ais_tfe_rdi_output *p_rdi = &core_info->rdi_out[i];

		ais_clear_rdi_path(p_rdi);
		p_rdi->state = AIS_ISP_RESOURCE_STATE_AVAILABLE;
	}

	spin_unlock_irqrestore(&core_info->spin_lock, flags);

	return rc;
}
