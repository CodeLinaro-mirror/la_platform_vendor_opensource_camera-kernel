/* Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
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

#include <linux/iopoll.h>
#include <linux/slab.h>
#include <media/cam_isp.h>
#include <media/cam_defs.h>

#include "cam_req_mgr_workq.h"
#include "ais_ife_csid_core_ver2.h"
#include "ais_isp_hw.h"
#include "cam_soc_util.h"
#include "cam_io_util.h"
#include "cam_debug_util.h"
#include "cam_cpas_api.h"

#define RUP_CONFIG_IN_IRQ

/* Timeout value in msec */
#define IFE_CSID_TIMEOUT                               1000

/* Timeout values in usec */
#define AIS_IFE_CSID_TIMEOUT_SLEEP_US                  1000
#define AIS_IFE_CSID_TIMEOUT_ALL_US                    100000

#define AIS_IFE_CSID_RESET_TIMEOUT_MS                  100

/*
 * Constant Factors needed to change QTimer ticks to nanoseconds
 * QTimer Freq = 19.2 MHz
 * Time(us) = ticks/19.2
 * Time(ns) = ticks/19.2 * 1000
 */
#define AIS_IFE_CSID_QTIMER_MUL_FACTOR                 10000
#define AIS_IFE_CSID_QTIMER_DIV_FACTOR                 192

/* Max number of sof irq's triggered in case of SOF freeze */
#define AIS_CSID_IRQ_SOF_DEBUG_CNT_MAX 12

/* Max CSI Rx irq error count threshold value */
#define AIS_IFE_CSID_MAX_IRQ_ERROR_COUNT               5

static int ais_ife_csid_ver2_wait_for_reset(
		struct ais_ife_csid_ver2_hw *csid_hw)
{
	unsigned long rem_jiffies = 0;
	int rc = 0;

	rem_jiffies = wait_for_completion_timeout(
			&csid_hw->hw_info->hw_complete,
			msecs_to_jiffies(AIS_IFE_CSID_RESET_TIMEOUT_MS));

	if (rem_jiffies == 0) {
		rc = -ETIMEDOUT;
		CAM_ERR(CAM_ISP, "CSID[%u] reset timed out",
					csid_hw->hw_intf->hw_idx);

		//ais_ife_csid_ver2_dump_imp_regs(csid_hw);
	} else
		CAM_DBG(CAM_ISP,
			"CSID[%u] reset success",
			csid_hw->hw_intf->hw_idx);

	return rc;
}

static int ais_ife_csid_ver2_internal_reset(struct ais_ife_csid_ver2_hw *csid_hw,
		uint32_t rst_cmd, uint32_t rst_location, uint32_t rst_mode)
{
	uint32_t val = 0;
	struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct cam_hw_soc_info                *soc_info;
	struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg;
	void __iomem *mem_base;
	int rc = 0;

	csid_reg = (struct ais_ife_csid_ver2_reg_offset *)
		csid_hw->csid_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;

	soc_info = &csid_hw->hw_info->soc_info;
	mem_base = soc_info->reg_map[AIS_IFE_CSID_VER2_CLC_MEM_BASE_ID].mem_base;

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID[%u] powered down state",
						csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	/* Input cut off prior to SW reset */
	if (rst_cmd == AIS_IFE_CSID_VER2_RESET_CMD_SW_RST) {
		cam_io_w_mb(0x0, mem_base + csi2_reg->csid_csi2_rx_cfg0_addr);
		cam_io_w_mb(0x0, mem_base + csi2_reg->csid_csi2_rx_cfg1_addr);
	}

	reinit_completion(&csid_hw->hw_info->hw_complete);

	/* Program the reset location */
	if (rst_location == AIS_IFE_CSID_VER2_RESET_LOC_PATH_ONLY)
		val |= (csid_reg->cmn_reg->rst_loc_path_only_val <<
				csid_reg->cmn_reg->rst_location_shift_val);
	else if (rst_location == AIS_IFE_CSID_VER2_RESET_LOC_COMPLETE)
		val |= (csid_reg->cmn_reg->rst_loc_complete_csid_val <<
				csid_reg->cmn_reg->rst_location_shift_val);

	/*Program the mode */
	if (rst_mode == AIS_CSID_HALT_AT_FRAME_BOUNDARY)
		val |= (csid_reg->cmn_reg->rst_mode_frame_boundary_val <<
				csid_reg->cmn_reg->rst_mode_shift_val);
	else if (rst_mode == AIS_CSID_HALT_IMMEDIATELY)
		val |= (csid_reg->cmn_reg->rst_mode_immediate_val <<
				csid_reg->cmn_reg->rst_mode_shift_val);

	cam_io_w_mb(val, mem_base + csid_reg->cmn_reg->csid_reset_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] reset_cfg: 0x%x",
				csid_hw->hw_intf->hw_idx, val);

	val = 0;
	/*Program the cmd */
	if (rst_cmd == AIS_IFE_CSID_VER2_RESET_CMD_IRQ_CTRL)
		val = csid_reg->cmn_reg->rst_cmd_irq_ctrl_only_val;
	else if (rst_cmd == AIS_IFE_CSID_VER2_RESET_CMD_HW_RST)
		val = csid_reg->cmn_reg->rst_cmd_hw_reset_complete_val;
	else if (rst_cmd == AIS_IFE_CSID_VER2_RESET_CMD_SW_RST)
		val = csid_reg->cmn_reg->rst_cmd_sw_reset_complete_val;

	cam_io_w_mb(val, mem_base + csid_reg->cmn_reg->csid_rst_strobes_addr);
	CAM_DBG(CAM_ISP, "CSID[%u] reset_cmd: 0x%x",
				csid_hw->hw_intf->hw_idx, val);

	if (rst_cmd == AIS_IFE_CSID_VER2_RESET_CMD_IRQ_CTRL)
		return 0;

	rc = ais_ife_csid_ver2_wait_for_reset(csid_hw);

	if (rc)
		CAM_ERR(CAM_ISP,
				"CSID[%u] Reset failed mode %d cmd %d loc %d",
				csid_hw->hw_intf->hw_idx,
				rst_mode, rst_cmd, rst_location);
	reinit_completion(&csid_hw->hw_info->hw_complete);
	return rc;
}

static int ais_ife_csid_clear_all_irq(struct ais_ife_csid_ver2_hw *csid_hw)
{
	struct cam_hw_soc_info                *soc_info;
	const struct ais_ife_csid_ver2_reg_offset  *csid_reg;
	int rc = 0;
	int i = 0;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = csid_hw->csid_info->csid_reg;

	for (i = 0; i < csid_reg->cmn_reg->num_top_irq; i++)
		cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
			csid_reg->cmn_reg->csid_top_irq_clear_addr[i]);

	for (i = 0; i < csid_reg->csi2_reg->num_rx_irq; i++)
		cam_io_w_mb(csid_reg->csi2_reg->csi2_irq_mask_all,
			soc_info->reg_map[0].mem_base +
			csid_reg->csi2_reg->csid_csi2_rx_irq_clear_addr[i]);

	if (csid_reg->csi2_reg->num_rx_irq > 1) {
		cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
			csid_reg->cmn_reg->csid_top_irq_clear_addr[1]);
		cam_io_w_mb(csid_reg->csi2_reg->csi2_irq_mask_all,
			soc_info->reg_map[0].mem_base +
			csid_reg->csi2_reg->csid_csi2_rx_irq_clear_addr[1]);
	}

	for (i = 0 ; i < csid_reg->cmn_reg->num_rdis; i++)
		cam_io_w_mb(csid_reg->cmn_reg->rdi_irq_mask_all,
			soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[i]->csid_rdi_irq_clear_addr);

	return rc;
}

static int ais_ife_csid_global_reset(struct ais_ife_csid_ver2_hw *csid_hw)
{
	struct cam_hw_soc_info                *soc_info;
	const struct ais_ife_csid_ver2_reg_offset  *csid_reg;
	struct ais_ife_csid_ver2_common_reg_offset *cmn_reg;
	uint32_t irq_mask = 0;
	int rc = 0;
	uint32_t i;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = csid_hw->csid_info->csid_reg;
	cmn_reg = csid_reg->cmn_reg;

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid HW State:%d",
			csid_hw->hw_intf->hw_idx,
			csid_hw->hw_info->hw_state);
		return -EINVAL;
	}

	CAM_DBG(CAM_ISP, "CSID:%d global reset",
		csid_hw->hw_intf->hw_idx);

	/* Mask all interrupts */
	for (i = 0; i < csid_reg->csi2_reg->num_rx_irq; i++) {
		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->csi2_reg->csid_csi2_rx_irq_mask_addr[i]);
	}

	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[i]->csid_rdi_irq_mask_addr);

		/* Configure the halt mode */
		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[i]->csid_rdi_ctrl_addr);

		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[i]->csid_rdi_cfg0_addr);
	}

	ais_ife_csid_clear_all_irq(csid_hw);


	/* set reset done for top irq */
	irq_mask =
		cmn_reg->top_reset_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0];
	cam_io_w_mb(irq_mask, soc_info->reg_map[0].mem_base +
		cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]);

	ais_ife_csid_ver2_internal_reset(csid_hw,
			AIS_IFE_CSID_VER2_RESET_CMD_SW_RST,
			AIS_IFE_CSID_VER2_RESET_LOC_COMPLETE,
			AIS_CSID_HALT_IMMEDIATELY);

	csid_hw->error_irq_count = 0;

	for (i = 0 ; i < AIS_IFE_CSID_RDI_MAX; i++) {
		csid_hw->rdi_cfg[i].state = AIS_ISP_RESOURCE_STATE_AVAILABLE;
		csid_hw->rdi_cfg[i].sof_cnt = 0;
		csid_hw->rdi_cfg[i].prev_sof_hw_ts = 0;
		csid_hw->rdi_cfg[i].prev_sof_boot_ts = 0;
		csid_hw->rdi_cfg[i].measure_cfg.measure_enabled = 0;
	}

	return rc;
}

static int ais_ife_csid_path_reset(struct ais_ife_csid_ver2_hw *csid_hw,
		enum ais_ife_output_path_id  reset_path)
{
	int rc = 0;
	struct cam_hw_soc_info                    *soc_info;
	const struct ais_ife_csid_ver2_reg_offset      *csid_reg;
	uint32_t id;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid hw state :%d",
			csid_hw->hw_intf->hw_idx,
			csid_hw->hw_info->hw_state);
		return -EINVAL;
	}

	if (reset_path >= AIS_IFE_CSID_RDI_MAX) {
		CAM_DBG(CAM_ISP, "CSID:%d Invalid res id%d",
			csid_hw->hw_intf->hw_idx, reset_path);
		rc = -EINVAL;
		goto end;
	}

	CAM_DBG(CAM_ISP, "CSID:%d resource:%d",
		csid_hw->hw_intf->hw_idx, reset_path);

	id = reset_path;
	if (!csid_reg->rdi_reg[id]) {
		CAM_ERR(CAM_ISP, "CSID:%d RDI res not supported :%d",
				csid_hw->hw_intf->hw_idx,
				reset_path);
		return -EINVAL;
	}
	/* Reset the corresponding ife csid path */
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[id]->csid_rdi_irq_mask_addr);

end:
	return rc;

}

static int ais_ife_csid_ver2_program_top(
    struct ais_ife_csid_ver2_hw *csid_hw)
{
	const struct ais_ife_csid_ver2_top_reg_info *top_reg;
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct cam_hw_soc_info *soc_info;
	int input_core_sel;
	uint32_t val;

	csid_reg = csid_hw->csid_info->csid_reg;

	if (!csid_reg->need_top_cfg) {
		CAM_DBG(CAM_ISP, "CSID %d top not supported",
					csid_hw->hw_intf->hw_idx);
		return 0;
	}

	top_reg  = csid_reg->top_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	/* Porgram top parameters */
	input_core_sel = csid_reg->input_core_sel[csid_hw->hw_intf->hw_idx]
										[AIS_IFE_CSID_INPUT_CORE_SEL_INTERNAL];

	CAM_DBG(CAM_ISP, "CSID[%d] input_core_sel %d",
				csid_hw->hw_intf->hw_idx, input_core_sel);

	if (input_core_sel == -1) {
		CAM_ERR(CAM_ISP, "csid[%d] invalid top input_core_type 1",
					csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	val = (uint32_t)input_core_sel << top_reg->input_core_type_shift_val;
	//sfe: do not use it
	val |= 0 << top_reg->sfe_offline_en_shift_val;
	//output ife en = 1
	val |= 1 << top_reg->out_ife_en_shift_val;
	//lcr en: set it 0
	val |= 0x0;

	cam_io_w_mb(val, soc_info->reg_map[1].mem_base +
			top_reg->io_path_cfg0_addr[csid_hw->hw_intf->hw_idx]);

	return 0;
}

static int ais_ife_csid_enable_hw(struct ais_ife_csid_ver2_hw  *csid_hw)
{
	int rc = 0;
	const struct ais_ife_csid_ver2_reg_offset      *csid_reg;
	struct cam_hw_soc_info                         *soc_info;
	uint32_t val;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	/* overflow check before increment */
	if (csid_hw->hw_info->open_count == UINT_MAX) {
		CAM_ERR(CAM_ISP, "CSID:%d Open count reached max",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	/* Increment ref Count */
	csid_hw->hw_info->open_count++;
	if (csid_hw->hw_info->open_count > 1) {
		CAM_DBG(CAM_ISP, "CSID hw has already been enabled");
		return rc;
	}

	CAM_DBG(CAM_ISP, "CSID:%d init CSID HW",
		csid_hw->hw_intf->hw_idx);

	rc = ais_ife_csid_enable_soc_resources(soc_info, CAM_TURBO_VOTE);
	if (rc) {
		CAM_ERR(CAM_ISP, "CSID:%d Enable SOC failed",
			csid_hw->hw_intf->hw_idx);
		goto err;
	}

	csid_hw->hw_info->hw_state = CAM_HW_STATE_POWER_UP;

	/* Reset CSID top */
	rc = ais_ife_csid_global_reset(csid_hw);
	if (rc)
		goto disable_soc;

	ais_ife_csid_clear_all_irq(csid_hw);

	reinit_completion(&csid_hw->hw_info->hw_complete);

	ais_ife_csid_ver2_program_top(csid_hw);

	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csid_reg->cmn_reg->csid_hw_version_addr);
	CAM_DBG(CAM_ISP, "CSID:%d CSID HW version: 0x%x",
		csid_hw->hw_intf->hw_idx, val);

	return 0;

disable_soc:
	ais_ife_csid_disable_soc_resources(soc_info);
	csid_hw->hw_info->hw_state = CAM_HW_STATE_POWER_DOWN;
err:
	csid_hw->hw_info->open_count--;
	return rc;
}

static int ais_ife_csid_disable_hw(struct ais_ife_csid_ver2_hw *csid_hw)
{
	int rc = -EINVAL;
	uint32_t i;
	struct cam_hw_soc_info                        *soc_info;
	const struct ais_ife_csid_ver2_reg_offset     *csid_reg;
	unsigned long                                 flags;

	/* Check for refcount */
	if (!csid_hw->hw_info->open_count) {
		CAM_WARN(CAM_ISP, "Unbalanced disable_hw");
		return rc;
	}

	/*  Decrement ref Count */
	csid_hw->hw_info->open_count--;

	if (csid_hw->hw_info->open_count) {
		rc = 0;
		return rc;
	}

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = csid_hw->csid_info->csid_reg;

	/* clear rdi path */
	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++)
		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[i]->csid_rdi_irq_mask_addr);

	/* disable buf done*/
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
	                csid_reg->cmn_reg->buf_done_irq_mask_addr);

	if (csid_reg->cmn_reg->num_top_irq > 1)
		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
				csid_reg->cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1]);

	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
			csid_reg->cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]);

	ais_ife_csid_ver2_internal_reset(csid_hw,
			AIS_IFE_CSID_VER2_RESET_CMD_SW_RST,
			AIS_IFE_CSID_VER2_RESET_LOC_COMPLETE,
			AIS_CSID_HALT_IMMEDIATELY);

	CAM_DBG(CAM_ISP, "CSID:%d De-init CSID HW",
		csid_hw->hw_intf->hw_idx);

	rc = ais_ife_csid_disable_soc_resources(soc_info);
	if (rc)
		CAM_ERR(CAM_ISP, "CSID:%d Disable CSID SOC failed",
			csid_hw->hw_intf->hw_idx);

	spin_lock_irqsave(&csid_hw->lock_state, flags);
	csid_hw->device_enabled = 0;
	spin_unlock_irqrestore(&csid_hw->lock_state, flags);
	for (i = 0; i < AIS_IFE_CSID_RDI_MAX; i++) {
		csid_hw->rdi_cfg[i].state = AIS_ISP_RESOURCE_STATE_AVAILABLE;
		csid_hw->rdi_cfg[i].sof_cnt = 0;
		csid_hw->rdi_cfg[i].prev_sof_boot_ts = 0;
		csid_hw->rdi_cfg[i].prev_sof_hw_ts = 0;
		csid_hw->rdi_cfg[i].measure_cfg.measure_enabled = 0;
	}

	csid_hw->hw_info->hw_state = CAM_HW_STATE_POWER_DOWN;
	csid_hw->error_irq_count = 0;
	csid_hw->fatal_err_detected = false;


	return rc;
}

static int ais_ife_csid_ver2_enable_rx2_irq(
			struct ais_ife_csid_ver2_hw *csid_hw)
{
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg;
	struct cam_hw_soc_info                      *soc_info;
	uint32_t val = 0;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	csi2_reg = csid_reg->csi2_reg;

	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
		csi2_reg->csid_csi2_rx_irq_mask_addr[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0]);

	val |= csi2_reg->rx2_irq_mask;

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csi2_reg->csid_csi2_rx_irq_mask_addr[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0]);

	val = csi2_reg->fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1] |
			csi2_reg->part_fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1] |
			csi2_reg->non_fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1];

	val |= csid_hw->debug_info.rx_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1];

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csi2_reg->csid_csi2_rx_irq_mask_addr[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1]);

	return 0;
}

static int ais_ife_csid_ver2_enable_rx_irq(
			struct ais_ife_csid_ver2_hw *csid_hw)
{
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg;
	struct cam_hw_soc_info                      *soc_info;
	uint32_t val = 0;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	csi2_reg = csid_reg->csi2_reg;

	val = csi2_reg->fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0] |
			csi2_reg->part_fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0] |
			csi2_reg->non_fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0];

	/* Enable the interrupt based on csid debug info set */
	val |= csid_hw->debug_info.rx_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0];

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csi2_reg->csid_csi2_rx_irq_mask_addr[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0]);

	if (csi2_reg->num_rx_irq > 1) {
		/* Enabel csi rx2 irq*/
		ais_ife_csid_ver2_enable_rx2_irq(csid_hw);
	}

	return 0;
}

static int ais_ife_csid_enable_csi2(
	struct ais_ife_csid_ver2_hw          *csid_hw,
	struct ais_ife_csid_csi_info         *csi_info)
{
	int rc = 0;
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct cam_hw_soc_info                      *soc_info;
	uint32_t val = 0;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	CAM_DBG(CAM_ISP, "CSID:%d count:%d config csi2 rx",
		csid_hw->hw_intf->hw_idx, csid_hw->csi2_cfg_cnt);

	/* overflow check before increment */
	if (csid_hw->csi2_cfg_cnt == UINT_MAX) {
		CAM_ERR(CAM_ISP, "CSID:%d Open count reached max",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	csid_hw->csi2_cfg_cnt++;
	if (csid_hw->csi2_cfg_cnt > 1)
		return rc;

	//[TODO] for ais: 8838 need csiphy id + 1 ?
	csid_hw->csi2_rx_cfg.phy_sel = csi_info->csiphy_id + 1;
	csid_hw->csi2_rx_cfg.lane_num = csi_info->num_lanes;
	csid_hw->csi2_rx_cfg.lane_cfg = csi_info->lane_assign;
	csid_hw->csi2_rx_cfg.lane_type = csi_info->is_3Phase;

	/* rx cfg0 */
	val = ((csid_hw->csi2_rx_cfg.lane_num - 1) & 0x3)  |
		((csid_hw->csi2_rx_cfg.lane_cfg & 0xFFFF) << 4) |
		((csid_hw->csi2_rx_cfg.lane_type & 0x1) << 24);
	val |= (csid_hw->csi2_rx_cfg.phy_sel &
		csid_reg->csi2_reg->csi2_rx_phy_num_mask) << 20;

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csid_reg->csi2_reg->csid_csi2_rx_cfg0_addr);

	/* rx cfg1*/
	/* enable packet ecc correction and misr*/
	val = 0x1 | (1 << csid_reg->csi2_reg->csi2_misr_enable_shift_val);

	/* enable vcx if required */
	if (csi_info->vcx_mode)
		val |= (1 << csid_reg->csi2_reg->csi2_vc_mode_shift_val);

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csid_reg->csi2_reg->csid_csi2_rx_cfg1_addr);

	ais_ife_csid_ver2_enable_rx_irq(csid_hw);

	return 0;
}

static int ais_ife_csid_disable_csi2(struct ais_ife_csid_ver2_hw *csid_hw)
{
	int rc = 0;
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct cam_hw_soc_info               *soc_info;
	int i = 0;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	CAM_DBG(CAM_ISP, "CSID:%d cnt : %d Disable csi2 rx",
		csid_hw->hw_intf->hw_idx, csid_hw->csi2_cfg_cnt);

	if (csid_hw->csi2_cfg_cnt)
		csid_hw->csi2_cfg_cnt--;

	if (csid_hw->csi2_cfg_cnt)
		return 0;

	/* Disable the CSI2 rx inerrupts */
	for (i = 0; i < csid_reg->csi2_reg->num_rx_irq; i++)
		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->csi2_reg->csid_csi2_rx_irq_mask_addr[i]);

	return rc;
}

static void ais_ife_csid_halt_csi2(
	struct ais_ife_csid_ver2_hw          *csid_hw)
{
	const struct ais_ife_csid_ver2_reg_offset      *csid_reg;
	struct cam_hw_soc_info                    *soc_info;
	int i = 0;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	for (i = 0; i < csid_reg->csi2_reg->num_rx_irq; i++)
		cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
			csid_reg->csi2_reg->csid_csi2_rx_irq_mask_addr[i]);

	/* Reset the Rx CFG registers */
	cam_io_w(0, soc_info->reg_map[0].mem_base +
		csid_reg->csi2_reg->csid_csi2_rx_cfg0_addr);
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
		csid_reg->csi2_reg->csid_csi2_rx_cfg1_addr);
}

static int ais_ife_csid_ver2_config_camif(
		struct ais_ife_csid_ver2_hw *csid_hw,
		struct ais_ife_csid_ver2_path_cfg *path_cfg,
		int id)
{
	struct cam_hw_soc_info                      *soc_info;
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	path_cfg->epoch_cfg = (path_cfg->end_line  - path_cfg->start_line - 1) *
						csid_reg->cmn_reg->epoch_factor / 100;

	if (path_cfg->epoch_cfg > path_cfg->end_line)
		path_cfg->epoch_cfg = path_cfg->end_line;

	/*Program the camif part */
	cam_io_w_mb(path_cfg->epoch_cfg << csid_reg->rdi_reg[id]->epoch0_shift_val,
					soc_info->reg_map[0].mem_base +
					csid_reg->rdi_reg[id]->csid_rdi_epoch_irq_cfg_addr);

	CAM_DBG(CAM_ISP, "CSID[%d] epoch factor: 0x%x",
	csid_hw->hw_intf->hw_idx, path_cfg->epoch_cfg);

	return 0;
}

static void ais_ife_csid_ver2_path_rup_aup(
	struct ais_ife_csid_ver2_hw *csid_hw,
	uint32_t enable_flag,
	uint32_t path,
	struct ais_ife_csid_ver2_rup_aup_mask *rup_aup_mask)
{
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct ais_ife_csid_ver2_rdi_reg_offset *path_reg;
	struct ais_ife_csid_ver2_path_cfg *path_data;

	csid_reg = csid_hw->csid_info->csid_reg;
	path_reg = csid_reg->rdi_reg[path];
	path_data = (struct ais_ife_csid_ver2_path_cfg *)&csid_hw->rdi_cfg[path];

	if (path_data->state != AIS_ISP_RESOURCE_STATE_STREAMING) {
		CAM_DBG(CAM_ISP, "CSID%d RDI%d drop rup",
			csid_hw->hw_intf->hw_idx,
			path);
		return;
	}

	if (enable_flag) {
		if (csid_reg->cmn_reg->capabilities & AIS_IFE_CSID_CAP_SPLIT_RUP_AUP) {
			rup_aup_mask->rup_mask |= path_reg->rup_mask;
			rup_aup_mask->aup_mask |= path_reg->aup_mask;
		} else {
			rup_aup_mask->rup_mask |= path_reg->rup_aup_mask;
		}
	} else {
		if (csid_reg->cmn_reg->capabilities & AIS_IFE_CSID_CAP_SPLIT_RUP_AUP) {
			rup_aup_mask->rup_mask &= ~path_reg->rup_mask;
			rup_aup_mask->aup_mask &= ~path_reg->aup_mask;
		} else {
			rup_aup_mask->rup_mask &= ~path_reg->rup_aup_mask;
		}
	}

	if (csid_reg->cmn_reg->capabilities & AIS_IFE_CSID_CAP_SPLIT_RUP_AUP
		&& rup_aup_mask->rup_mask) {
		rup_aup_mask->rup_aup_set_mask = 1;
	} else {
		rup_aup_mask->rup_aup_set_mask = 0;
	}

	CAM_DBG(CAM_ISP, "CSID %d enable_flag: 0x%x, path = %d, "
		"rup_mask = 0x%x, aup_mask = 0X%x, rup_aup_set_mask = 0x%x",
		csid_hw->hw_intf->hw_idx,
		enable_flag, path,
		rup_aup_mask->rup_mask,
		rup_aup_mask->aup_mask,
		rup_aup_mask->rup_aup_set_mask);
}

static void ais_ife_csid_ver2_config_rup_aup(
	struct ais_ife_csid_ver2_hw *csid_hw,
	struct ais_ife_csid_ver2_rup_aup_mask *rup_aup_mask)
{
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct cam_hw_soc_info                    *soc_info;
	unsigned long flags;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	spin_lock_irqsave(&csid_hw->lock_rup, flags);
	if (csid_reg->cmn_reg->capabilities & AIS_IFE_CSID_CAP_SPLIT_RUP_AUP) {
		cam_io_w_mb(rup_aup_mask->rup_mask,
				soc_info->reg_map[0].mem_base + csid_reg->cmn_reg->rup_cmd_addr);
		cam_io_w_mb(rup_aup_mask->aup_mask,
					soc_info->reg_map[0].mem_base + csid_reg->cmn_reg->aup_cmd_addr);
		cam_io_w_mb(rup_aup_mask->rup_aup_set_mask,
				soc_info->reg_map[0].mem_base + csid_reg->cmn_reg->rup_aup_cmd_addr);
	} else {
		cam_io_w_mb(rup_aup_mask->rup_mask,
				soc_info->reg_map[0].mem_base + csid_reg->cmn_reg->rup_aup_cmd_addr);
	}
	spin_unlock_irqrestore(&csid_hw->lock_rup, flags);
	CAM_DBG(CAM_ISP, "CSID %d config RUP "
		"rup_mask = 0x%x, aup_mask = 0X%x, rup_aup_set_mask = 0x%x",
		csid_hw->hw_intf->hw_idx,
		rup_aup_mask->rup_mask,
		rup_aup_mask->aup_mask,
		rup_aup_mask->rup_aup_set_mask);
}

static int ais_ife_csid_config_rdi_path(
	struct ais_ife_csid_ver2_hw          *csid_hw,
	struct ais_ife_rdi_init_args         *res)
{
	int rc = 0;
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct cam_hw_soc_info                 *soc_info;
	struct ais_ife_csid_ver2_path_cfg           *path_cfg;
	uint32_t val = 0, cfg0 = 0, cfg1 = 0, id = 0;
	uint32_t format_measure_addr;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	id = res->path;
	if (id >= AIS_IFE_CSID_RDI_MAX || id >= csid_reg->cmn_reg->num_rdis ||
		!csid_reg->rdi_reg[id]) {
		CAM_ERR(CAM_ISP, "CSID:%d RDI:%d is not supported on HW",
			 csid_hw->hw_intf->hw_idx, id);
		return -EINVAL;
	}

	path_cfg = &csid_hw->rdi_cfg[id];
	path_cfg->vc = res->csi_cfg.vc;
	path_cfg->dt = res->csi_cfg.dt;
	path_cfg->cid = res->csi_cfg.dt_id;
	path_cfg->in_format = res->in_cfg.format;
	path_cfg->out_format = res->out_cfg.format;
	path_cfg->crop_enable = res->in_cfg.crop_enable;
	path_cfg->start_pixel = res->in_cfg.crop_left;
	path_cfg->end_pixel = res->in_cfg.crop_right;
	path_cfg->start_line = res->in_cfg.crop_top;
	path_cfg->end_line = res->in_cfg.crop_bottom;
	path_cfg->decode_fmt = res->in_cfg.decode_format;
	path_cfg->plain_fmt = res->in_cfg.pack_type;
	path_cfg->init_frame_drop = res->in_cfg.init_frame_drop;
	path_cfg->vfr_en = 0;
	path_cfg->frame_id_dec_en = 0;

	if (path_cfg->decode_fmt == 0xF)
		path_cfg->pix_enable = false;
	else
		path_cfg->pix_enable = true;

	ais_ife_csid_ver2_config_camif(csid_hw, path_cfg, id);

	/* [TODO] corp: disable first, have not verity yet */
	path_cfg->crop_enable = 0;
	if (path_cfg->crop_enable) {
		val = (((path_cfg->end_pixel & 0xFFFF) <<
			csid_reg->cmn_reg->crop_shift) |
			(path_cfg->start_pixel & 0xFFFF));

		cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[id]->csid_rdi_rpp_hcrop_addr);
		CAM_DBG(CAM_ISP, "CSID:%d Horizontal crop config val: 0x%x",
			csid_hw->hw_intf->hw_idx, val);

		val = (((path_cfg->end_line & 0xFFFF) <<
			csid_reg->cmn_reg->crop_shift) |
			(path_cfg->start_line & 0xFFFF));

		cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[id]->csid_rdi_rpp_vcrop_addr);
		CAM_DBG(CAM_ISP, "CSID:%d Vertical Crop config val: 0x%x",
			csid_hw->hw_intf->hw_idx, val);

		val = 0;
	}

#if 0 //[TODO] for ais: check if need
	/* configure pixel format measure */
	if (csid_hw->rdi_cfg[id].measure_cfg.measure_enabled) {
		val = ((csid_hw->rdi_cfg[id].measure_cfg.height &
		csid_reg->cmn_reg->format_measure_height_mask_val) <<
		csid_reg->cmn_reg->format_measure_height_shift_val);

		if (path_cfg->decode_fmt == 0xF)
			val |= (((csid_hw->rdi_cfg[id].measure_cfg.width *
					path_cfg->in_bpp) / 8) &
			csid_reg->cmn_reg->format_measure_width_mask_val);
		else
			val |= (csid_hw->rdi_cfg[id].measure_cfg.width &
			csid_reg->cmn_reg->format_measure_width_mask_val);

		CAM_DBG(CAM_ISP, "CSID:%d format measure cfg1 value : 0x%x",
			csid_hw->hw_intf->hw_idx, val);

		cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_format_measure_cfg1_addr);

		/* enable pixel and line counter */
		cam_io_w_mb(3, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_format_measure_cfg0_addr);
	}

	/* set frame drop pattern to 0 and period to 1 */
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_frm_drop_period_addr);
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_frm_drop_pattern_addr);
	/* set IRQ sum sabmple */
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_irq_subsample_period_addr);
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_irq_subsample_pattern_addr);

	/* set pixel drop pattern to 0 and period to 1 */
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_rpp_pix_drop_pattern_addr);
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_rpp_pix_drop_period_addr);
	/* set line drop pattern to 0 and period to 1 */
	cam_io_w_mb(0, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_rpp_line_drop_pattern_addr);
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_rpp_line_drop_period_addr);

#endif

	/* RDI path config and enable*/
	/*Configure cfg0:
	 * VC
	 * DT
	 * Timestamp enable and strobe selection for v780
	 * DT_ID cobination
	 * Decode Format
	 * Frame_id_dec_en
	 * VFR en
	 */
	cfg0 = (path_cfg->vc << csid_reg->cmn_reg->vc_shift_val) |
		(path_cfg->dt << csid_reg->cmn_reg->dt_shift_val) |
		(path_cfg->decode_fmt << csid_reg->cmn_reg->fmt_shift_val);

	if (!csid_reg->cmn_reg->direct_cid_config)
		cfg0 |= path_cfg->cid << csid_reg->cmn_reg->dt_id_shift_val;

	if (csid_reg->cmn_reg->vfr_supported)
		cfg0 |= path_cfg->vfr_en << csid_reg->cmn_reg->vfr_en_shift_val;

	if (csid_reg->cmn_reg->frame_id_dec_supported)
		cfg0 |= path_cfg->frame_id_dec_en <<
			csid_reg->cmn_reg->frame_id_decode_en_shift_val;

	if (csid_reg->cmn_reg->timestamp_enabled_in_cfg0)
		cfg0 |= (1 << csid_reg->rdi_reg[id]->timestamp_en_shift_val) |
			(csid_reg->cmn_reg->timestamp_strobe_val <<
				csid_reg->cmn_reg->timestamp_stb_sel_shift_val);

	CAM_DBG(CAM_ISP, "CSID:%d RDI:%d cfg0 = 0x%x",
		csid_hw->hw_intf->hw_idx, id, cfg0);

	/* For some targets, cfg1 register is programmed as part of cdm buffer, so skip here */
	if (!(csid_reg->cmn_reg->capabilities & AIS_IFE_CSID_CAP_SKIP_PATH_CFG1)) {
		/*configure cfg1 addr
		 * Crop/Drop parameters
		 * Timestamp enable and strobe selection
		 * Plain format
		 * Packing format
		 */
		cfg1 = (path_cfg->crop_enable << csid_reg->cmn_reg->crop_h_en_shift_val) |
			(path_cfg->crop_enable << csid_reg->cmn_reg->crop_v_en_shift_val);

		cfg1 |= (path_cfg->plain_fmt <<
				csid_reg->cmn_reg->plain_fmt_shift_val);

		if (csid_hw->debug_info.debug_val &
			CSID_DEBUG_ENABLE_HBI_VBI_INFO)
			cfg1 |= 1 << csid_reg->cmn_reg->format_measure_en_shift_val;

		cam_io_w_mb(cfg1, soc_info->reg_map[0].mem_base +
						csid_reg->rdi_reg[id]->csid_rdi_cfg1_addr);

		CAM_DBG(CAM_ISP, "CSID:%u RDI:%d cfg1: 0x%x",
			csid_hw->hw_intf->hw_idx, id, cfg1);
	}

	/* Enable the HBI/VBI counter */
	if (csid_hw->debug_info.debug_val & CSID_DEBUG_ENABLE_HBI_VBI_INFO) {
		format_measure_addr =
			csid_reg->rdi_reg[id]->csid_rdi_format_measure_cfg0_addr;

		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			format_measure_addr);
		val |= csid_reg->cmn_reg->measure_en_hbi_vbi_cnt_mask;
		cam_io_w_mb(val,
			soc_info->reg_map[0].mem_base + format_measure_addr);
	}

	val = 0;
	/* configure the rx packet capture based on csid debug set */
	if (csid_hw->debug_info.debug_val & CSID_DEBUG_ENABLE_SHORT_PKT_CAPTURE)
		val |= ((1 <<
			csid_reg->csi2_reg->csi2_capture_short_pkt_en_shift) |
			(path_cfg->vc <<
			csid_reg->csi2_reg->csi2_capture_short_pkt_vc_shift));

	if (csid_hw->debug_info.debug_val & CSID_DEBUG_ENABLE_LONG_PKT_CAPTURE)
		val |= ((1 <<
			csid_reg->csi2_reg->csi2_capture_long_pkt_en_shift) |
			(path_cfg->dt <<
			csid_reg->csi2_reg->csi2_capture_long_pkt_dt_shift) |
			(path_cfg->vc <<
			csid_reg->csi2_reg->csi2_capture_long_pkt_vc_shift));

	if (csid_hw->debug_info.debug_val & CSID_DEBUG_ENABLE_CPHY_PKT_CAPTURE)
		val |= ((1 <<
			csid_reg->csi2_reg->csi2_capture_cphy_pkt_en_shift) |
			(path_cfg->dt <<
			csid_reg->csi2_reg->csi2_capture_cphy_pkt_dt_shift) |
			(path_cfg->vc <<
			csid_reg->csi2_reg->csi2_capture_cphy_pkt_vc_shift));

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csid_reg->csi2_reg->csid_csi2_rx_capture_ctrl_addr);

	/* Enable the RDI path */
	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[id]->csid_rdi_cfg0_addr);
	val |= (1 << csid_reg->cmn_reg->path_en_shift_val);

	cam_io_w_mb(val | cfg0, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[id]->csid_rdi_cfg0_addr);

	path_cfg->state = AIS_ISP_RESOURCE_STATE_INIT_HW;

	CAM_DBG(CAM_ISP, "CSID:%d RDI%d configured vc:%d dt:%d",
		csid_hw->hw_intf->hw_idx, id, path_cfg->vc, path_cfg->dt);

	return rc;
}

static int ais_ife_csid_deinit_rdi_path(
	struct ais_ife_csid_ver2_hw          *csid_hw,
	enum ais_ife_output_path_id    id)
{
	int rc = 0;
	uint32_t val, format_measure_addr;
	const struct ais_ife_csid_ver2_reg_offset      *csid_reg;
	struct cam_hw_soc_info                    *soc_info;
	struct ais_ife_csid_ver2_path_cfg              *path_cfg;

	path_cfg = &csid_hw->rdi_cfg[id];
	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	format_measure_addr =
		csid_reg->rdi_reg[id]->csid_rdi_format_measure_cfg0_addr;

	if (csid_hw->csid_debug & CSID_DEBUG_ENABLE_HBI_VBI_INFO) {
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[id]->csid_rdi_cfg0_addr);
		val &= ~csid_reg->cmn_reg->format_measure_en_val;
		cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[id]->csid_rdi_cfg0_addr);

		/* Disable the HBI/VBI counter */
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			format_measure_addr);
		val &= ~csid_reg->cmn_reg->measure_en_hbi_vbi_cnt_mask;
		cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
			format_measure_addr);
	}

	path_cfg->state = AIS_ISP_RESOURCE_STATE_AVAILABLE;

	return rc;
}

static int ais_ife_csid_enable_rdi_path(
	struct ais_ife_csid_ver2_hw          *csid_hw,
	struct ais_ife_rdi_start_args   *start_cmd)
{
	const struct ais_ife_csid_ver2_reg_offset      *csid_reg;
	struct cam_hw_soc_info                         *soc_info;
	struct ais_ife_csid_ver2_path_cfg              *path_data;
	struct ais_ife_csid_ver2_common_reg_offset     *cmn_reg;
	struct ais_ife_csid_ver2_rup_aup_mask          rup_aup_mask;
	uint32_t id, val;

	if (start_cmd->path >= AIS_IFE_CSID_RDI_MAX) {
		CAM_ERR(CAM_ISP, "RDI:%d path is not supported", start_cmd->path);
		return -EINVAL;
	}

	id = start_cmd->path;
	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	cmn_reg = csid_reg->cmn_reg;
	path_data = &csid_hw->rdi_cfg[id];
	path_data->sof_cnt = 0;

	/* enable rdi path in top irq*/
	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
		cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]);
	val |=
		csid_reg->rdi_reg[id]->top_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0];
	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]);

	/* enable buffer done irq */
	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
					cmn_reg->buf_done_irq_mask_addr);
	val |= BIT(cmn_reg->buf_done_shift[id]);
	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
					cmn_reg->buf_done_irq_mask_addr);

	/* set path debug irq */
	val = csid_hw->debug_info.path_mask;

	/* need sof trigger */
	val |= AIS_CSID_VER2_PATH_INFO_INPUT_SOF;

	/* enable error irq */
	val |= csid_reg->rdi_reg[id]->fatal_err_mask |
				csid_reg->rdi_reg[id]->non_fatal_err_mask;

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_irq_mask_addr);

	path_data->state = AIS_ISP_RESOURCE_STATE_STREAMING;

	ais_ife_csid_ver2_path_rup_aup(csid_hw, 1, id, &rup_aup_mask);
	ais_ife_csid_ver2_config_rup_aup(csid_hw, &rup_aup_mask);

	/*resume at frame boundary */
	if (!path_data->init_frame_drop) {
		CAM_DBG(CAM_ISP, "Start RDI:%d path", id);
		cam_io_w_mb(0x1,
				soc_info->reg_map[0].mem_base +
				csid_reg->rdi_reg[id]->csid_rdi_ctrl_addr);
	}

	return 0;
}


static int ais_ife_csid_disable_rdi_path(
	struct ais_ife_csid_ver2_hw          *csid_hw,
	struct ais_ife_rdi_stop_args    *stop_args,
	enum ais_ife_csid_halt_cmd       stop_cmd)
{
	int rc = 0;
	uint32_t id, val = 0;
	const struct ais_ife_csid_ver2_reg_offset       *csid_reg;
	struct cam_hw_soc_info                     *soc_info;
	struct ais_ife_csid_ver2_path_cfg               *path_data;

	if (stop_args->path >= AIS_IFE_CSID_RDI_MAX) {
		CAM_ERR(CAM_ISP, "RDI:%d path is not supported", stop_args->path);
		return -EINVAL;
	}

	id = stop_args->path;
	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	path_data = &csid_hw->rdi_cfg[id];

	if (path_data->state != AIS_ISP_RESOURCE_STATE_STREAMING) {
		CAM_ERR_RATE_LIMIT(CAM_ISP,
			"CSID:%d RDI:%d already in stopped state:%d",
			csid_hw->hw_intf->hw_idx,
			id, path_data->state);
		return -EINVAL;
	}

	if (stop_cmd != AIS_CSID_HALT_AT_FRAME_BOUNDARY &&
		stop_cmd != AIS_CSID_HALT_IMMEDIATELY) {
		CAM_ERR(CAM_ISP, "CSID:%d un supported stop command:%d",
			csid_hw->hw_intf->hw_idx, stop_cmd);
		return -EINVAL;
	}

	CAM_DBG(CAM_ISP, "CSID:%d RDI:%d",
		csid_hw->hw_intf->hw_idx, id);

	/* Halt the RDI path */
	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
	        csid_reg->rdi_reg[id]->csid_rdi_ctrl_addr);
	val &= ~0x3;
	val |= stop_cmd;
	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
	        csid_reg->rdi_reg[id]->csid_rdi_ctrl_addr);

	/*
	 * for common register we should disable it for this RDI path
	 * 1. buf done
	 * 2. top irq
	 */

	/* buf done */
	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
	                csid_reg->cmn_reg->buf_done_irq_mask_addr);
	val &= ~(BIT(csid_reg->cmn_reg->buf_done_shift[id]));
	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
	                csid_reg->cmn_reg->buf_done_irq_mask_addr);

	/* top irq*/
	val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
	    csid_reg->cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]);
	val &= ~csid_reg->rdi_reg[id]->top_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0];
	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
	    csid_reg->cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]);

	path_data->state = AIS_ISP_RESOURCE_STATE_INIT_HW;

	return rc;
}

static int ais_ife_csid_poll_stop_status(
	struct ais_ife_csid_ver2_hw          *csid_hw,
	uint32_t                         res_mask)
{
	int rc = 0;
	uint32_t csid_status_addr = 0, val = 0, res_id = 0;
	const struct ais_ife_csid_ver2_reg_offset       *csid_reg;
	struct cam_hw_soc_info                     *soc_info;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	for (; res_id < AIS_IFE_CSID_RDI_MAX; res_id++, res_mask >>= 1) {
		if ((res_mask & 0x1) == 0)
			continue;
		val = 0;

		csid_status_addr =
			csid_reg->rdi_reg[res_id]->csid_rdi_status_addr;

		CAM_DBG(CAM_ISP, "start polling CSID:%d res_id:%d",
			csid_hw->hw_intf->hw_idx, res_id);

		rc = readl_poll_timeout(soc_info->reg_map[0].mem_base +
			csid_status_addr, val, (val & 0x1) == 0x1,
			AIS_IFE_CSID_TIMEOUT_SLEEP_US,
			AIS_IFE_CSID_TIMEOUT_ALL_US);
		if (rc < 0) {
			CAM_ERR(CAM_ISP, "CSID:%d res:%d halt failed rc %d",
				csid_hw->hw_intf->hw_idx, res_id, rc);
			rc = -ETIMEDOUT;
			break;
		}
		CAM_DBG(CAM_ISP, "End polling CSID:%d res_id:%d",
			csid_hw->hw_intf->hw_idx, res_id);
	}

	return rc;
}


static int ais_ife_csid_get_time_stamp(
	struct ais_ife_csid_ver2_hw   *csid_hw,
	void *cmd_args)
{
	struct ais_ife_rdi_get_timestamp_args      *p_timestamp;
	const struct ais_ife_csid_ver2_reg_offset       *csid_reg;
	struct cam_hw_soc_info                     *soc_info;
	const struct ais_ife_csid_ver2_rdi_reg_offset   *rdi_reg;
	uint32_t  time_32_lsb, time_32_msb, id;
	uint64_t  time_64;

	p_timestamp = (struct ais_ife_rdi_get_timestamp_args *)cmd_args;
	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	id = p_timestamp->path;

	if (id >= AIS_IFE_CSID_RDI_MAX || !csid_reg->rdi_reg[id]) {
		CAM_DBG(CAM_ISP, "CSID:%d Invalid RDI%d",
			csid_hw->hw_intf->hw_idx, id);
		return -EINVAL;
	}

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid dev state :%d",
			csid_hw->hw_intf->hw_idx,
			csid_hw->hw_info->hw_state);
		return -EINVAL;
	}
	rdi_reg = csid_reg->rdi_reg[id];

	time_32_lsb = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			rdi_reg->csid_rdi_timestamp_curr0_sof_addr);
	time_32_msb = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			rdi_reg->csid_rdi_timestamp_curr1_sof_addr);
	time_64 = ((uint64_t)time_32_msb << 32) | (uint64_t)time_32_lsb;
	p_timestamp->ts->cur_sof_ts = mul_u64_u32_div(time_64,
		AIS_IFE_CSID_QTIMER_MUL_FACTOR,
		AIS_IFE_CSID_QTIMER_DIV_FACTOR);

	time_32_lsb = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			rdi_reg->csid_rdi_timestamp_prev0_sof_addr);
	time_32_msb = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			rdi_reg->csid_rdi_timestamp_prev1_sof_addr);
	time_64 = ((uint64_t)time_32_msb << 32) | (uint64_t)time_32_lsb;
	p_timestamp->ts->prev_sof_ts = mul_u64_u32_div(time_64,
		AIS_IFE_CSID_QTIMER_MUL_FACTOR,
		AIS_IFE_CSID_QTIMER_DIV_FACTOR);

	csid_hw->rdi_cfg[id].prev_sof_hw_ts = p_timestamp->ts->cur_sof_ts;

	return 0;
}

static int ais_ife_csid_ver2_set_debug(
	struct ais_ife_csid_ver2_hw *csid_hw,
	uint32_t debug_val)
{
	int                                  bit_pos = 0;
	uint32_t                             val;
	int                                  i = 0;
	uint8_t                             *dbg_bit_pos = NULL;
	uint64_t                            *evt_bitmap = NULL;
	struct ais_ife_csid_ver2_reg_offset *csid_reg = NULL;
	struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg = NULL;
	struct ais_ife_csid_ver2_common_reg_offset *cmn_reg = NULL;

	csid_reg = csid_hw->csid_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;
	cmn_reg = csid_reg->cmn_reg;

	memset(&csid_hw->debug_info, 0,
			sizeof(struct ais_ife_csid_ver2_debug_info));

	csid_hw->debug_info.debug_val = debug_val;

	debug_val = csid_hw->debug_info.debug_val;
	while (debug_val) {

		if (!(debug_val & 0x1)) {
			debug_val >>= 1;
			bit_pos++;
			continue;
		}

		val = BIT(bit_pos);

		switch (val) {
		case CSID_DEBUG_ENABLE_SOF_IRQ:
			dbg_bit_pos = csid_reg->path_debug_mask->bit_pos;
			csid_hw->debug_info.path_mask |=
				BIT(dbg_bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_SOF]);
			break;
		case CSID_DEBUG_ENABLE_EOF_IRQ:
			dbg_bit_pos = csid_reg->path_debug_mask->bit_pos;
			csid_hw->debug_info.path_mask |=
				BIT(dbg_bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_EOF]);
			break;
		case CSID_DEBUG_ENABLE_SOT_IRQ:
			evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;

			for (i = 0; i < csi2_reg->num_rx_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_RX_DL0_SOT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL0_SOT_CAPTURED]) |
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL1_SOT_CAPTURED]) |
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL2_SOT_CAPTURED]) |
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL3_SOT_CAPTURED]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_EOT_IRQ:
			evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;

			for (i = 0; i < csi2_reg->num_rx_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_RX_DL0_EOT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL0_EOT_CAPTURED]) |
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL1_EOT_CAPTURED]) |
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL2_EOT_CAPTURED]) |
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_DL3_EOT_CAPTURED]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_SHORT_PKT_CAPTURE:
			evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;

			for (i = 0; i < csi2_reg->num_rx_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos
							[AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_LONG_PKT_CAPTURE:
			evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;

			for (i = 0; i < csi2_reg->num_rx_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_RX_LONG_PKT_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_LONG_PKT_CAPTURED]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_CPHY_PKT_CAPTURE:
			evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;

			for (i = 0; i < csi2_reg->num_rx_irq; i++) {
				if (evt_bitmap[i] &
					BIT_ULL(AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos
							[AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_UNMAPPED_VC_DT_IRQ:
			evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->rx_debug_mask->bit_pos;

			for (i = 0; i < csi2_reg->num_rx_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_RX_UNMAPPED_VC_DT)) {
					csid_hw->debug_info.rx_mask[i] |=
						BIT(dbg_bit_pos[AIS_IFE_CSID_RX_UNMAPPED_VC_DT]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_VOTE_UP_IRQ:
			evt_bitmap = csid_reg->top_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->top_debug_mask->bit_pos;

			for (i = 0; i < cmn_reg->num_top_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_TOP_INFO_VOTE_UP)) {
					csid_hw->debug_info.top_mask[i] |=
						BIT(dbg_bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_UP]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_VOTE_DN_IRQ:
			evt_bitmap = csid_reg->top_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->top_debug_mask->bit_pos;

			for (i = 0; i < cmn_reg->num_top_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_TOP_INFO_VOTE_DN)) {
					csid_hw->debug_info.top_mask[i] |=
						BIT(dbg_bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_DN]);
				}
			}
			break;
		case CSID_DEBUG_ENABLE_ERR_NO_VOTE_DN_IRQ:
			evt_bitmap = csid_reg->top_debug_mask->evt_bitmap;
			dbg_bit_pos = csid_reg->top_debug_mask->bit_pos;

			for (i = 0; i < cmn_reg->num_top_irq; i++) {
				if (evt_bitmap[i] & BIT_ULL(AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN)) {
					csid_hw->debug_info.top_mask[i] |=
						BIT(dbg_bit_pos[AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN]);
				}
			}
			break;
		default:
			break;
		}

		debug_val >>= 1;
		bit_pos++;
	}

	return 0;
}

static int ais_ife_csid_set_csid_debug(struct ais_ife_csid_ver2_hw   *csid_hw,
	void *cmd_args)
{
	uint32_t  *csid_debug;

	csid_debug = (uint32_t  *) cmd_args;
	csid_hw->csid_debug = *csid_debug;
	CAM_DBG(CAM_ISP, "CSID:%d set csid debug value:%d",
		csid_hw->hw_intf->hw_idx, csid_hw->csid_debug);

	return 0;
}

static int ais_ife_csid_get_hw_caps(void *hw_priv,
	void *get_hw_cap_args, uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_hw_caps           *hw_caps;
	struct ais_ife_csid_ver2_hw                *csid_hw;
	struct cam_hw_info                    *csid_hw_info;
	const struct ais_ife_csid_ver2_reg_offset  *csid_reg;

	if (!hw_priv || !get_hw_cap_args) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;
	csid_reg = csid_hw->csid_info->csid_reg;
	hw_caps = (struct ais_ife_csid_hw_caps *) get_hw_cap_args;

	hw_caps->num_rdis = csid_reg->cmn_reg->num_rdis;
	hw_caps->num_pix = csid_reg->cmn_reg->num_pix;
	hw_caps->major_version = csid_reg->cmn_reg->major_version;
	hw_caps->minor_version = csid_reg->cmn_reg->minor_version;
	hw_caps->version_incr = csid_reg->cmn_reg->version_incr;

	CAM_DBG(CAM_ISP,
		"CSID:%d No rdis:%d, no pix:%d, major:%d minor:%d ver :%d",
		csid_hw->hw_intf->hw_idx, hw_caps->num_rdis,
		hw_caps->num_pix, hw_caps->major_version,
		hw_caps->minor_version, hw_caps->version_incr);

	return rc;
}

static int ais_ife_csid_force_reset(void *hw_priv,
	void *reset_args, uint32_t arg_size)
{
	struct ais_ife_csid_ver2_hw          *csid_hw;
	struct cam_hw_info              *csid_hw_info;
	int rc = 0;

	if (!hw_priv) {
		CAM_ERR(CAM_ISP, "CSID:Invalid args");
		return -EINVAL;
	}

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;

	mutex_lock(&csid_hw->hw_info->hw_mutex);

	/* Disable CSID HW if necessary */
	if (csid_hw_info->open_count) {
		csid_hw_info->open_count = 1;

		CAM_DBG(CAM_ISP, "Disabling CSID Hw = %d", csid_hw->hw_intf->hw_idx);
		rc = ais_ife_csid_disable_hw(csid_hw);
		csid_hw->csi2_cfg_cnt = 0;
	}

	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	CAM_INFO(CAM_ISP, "Exit (%d)", rc);

	return rc;
}

#if 0
static int ais_ife_csid_reset_retain_sw_reg(
	struct ais_ife_csid_ver2_hw *csid_hw)
{
	int rc = 0;
#if 0 //[TODO] for ais
	uint32_t status;
#endif
	const struct ais_ife_csid_ver2_reg_offset *csid_reg =
		csid_hw->csid_info->csid_reg;
	struct cam_hw_soc_info          *soc_info;

	soc_info = &csid_hw->hw_info->soc_info;
#if 0 //[TODO] for ais
	/* clear the top interrupt first */
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->csid_top_irq_clear_addr);
#endif
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->csid_irq_cmd_addr);

	usleep_range(3000, 3010);

	cam_io_w_mb(csid_reg->cmn_reg->csid_rst_stb,
		soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->csid_rst_strobes_addr);
#if 0 //[TODO] for ais
	rc = readl_poll_timeout(soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->csid_top_irq_status_addr,
			status, (status & 0x1) == 0x1,
		AIS_IFE_CSID_TIMEOUT_SLEEP_US, AIS_IFE_CSID_TIMEOUT_ALL_US);
	if (rc < 0) {
		CAM_ERR(CAM_ISP, "CSID:%d csid_reset fail rc = %d",
			  csid_hw->hw_intf->hw_idx, rc);
		rc = -ETIMEDOUT;
	} else {
		CAM_DBG(CAM_ISP, "CSID:%d hw reset completed %d",
			csid_hw->hw_intf->hw_idx, rc);
		rc = 0;
	}
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->csid_top_irq_clear_addr);
#endif
	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
		csid_reg->cmn_reg->csid_irq_cmd_addr);

	return rc;
}
#endif

static int ais_ife_csid_ver2_enable_top2_irq(struct ais_ife_csid_ver2_hw *csid_hw)
{
	const struct ais_ife_csid_ver2_reg_offset     *csid_reg;
	struct cam_hw_soc_info                        *soc_info;
	struct ais_ife_csid_ver2_common_reg_offset    *cmn_reg;
	uint32_t val = 0;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	cmn_reg = csid_reg->cmn_reg;

	/* enable top2 error irq */
	val = cmn_reg->top_err_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1];

	/* enabel top2 debug irq*/
	val |= csid_hw->debug_info.top_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1];

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1]);

	return 0;
}

static int ais_ife_csid_ver2_enable_top_irq(struct ais_ife_csid_ver2_hw *csid_hw)
{

	const struct ais_ife_csid_ver2_reg_offset      *csid_reg;
	struct cam_hw_soc_info                         *soc_info;
	struct ais_ife_csid_ver2_common_reg_offset     *cmn_reg;
	uint32_t val = 0;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	cmn_reg = csid_reg->cmn_reg;

	/* top2 enable */
	val = cmn_reg->top_top2_irq_mask;

	/* csi2 enable */
	val |= csid_reg->csi2_reg->top_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0];

	/* enbale buffer done irq */
	val |= cmn_reg->top_buf_done_irq_mask;

	/* enable error irq*/
	val |= cmn_reg->top_err_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0];

	/* enable debug irq */
	val |= csid_hw->debug_info.top_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0];

	cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
		cmn_reg->csid_top_irq_mask_addr[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]);

	if (csid_reg->cmn_reg->num_top_irq > 1)
		ais_ife_csid_ver2_enable_top2_irq(csid_hw);

	return 0;
}

static int ais_ife_csid_reserve(void *hw_priv,
	void *reserve_args, uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_ver2_hw               *csid_hw;
	struct cam_hw_info                        *csid_hw_info;
	struct ais_ife_rdi_init_args              *rdi_cfg;
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	unsigned long                              flags;

	if (!hw_priv || !reserve_args ||
		(arg_size != sizeof(struct ais_ife_rdi_init_args))) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;
	rdi_cfg = (struct ais_ife_rdi_init_args *)reserve_args;
	csid_reg = csid_hw->csid_info->csid_reg;

	mutex_lock(&csid_hw->hw_info->hw_mutex);
	if (rdi_cfg->path >= AIS_IFE_CSID_RDI_MAX) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid RDI%d",
			csid_hw->hw_intf->hw_idx, rdi_cfg->path);
		rc = -EINVAL;
		goto end;
	}

	if (csid_hw->rdi_cfg[rdi_cfg->path].state !=
		AIS_ISP_RESOURCE_STATE_AVAILABLE) {
		CAM_ERR(CAM_ISP,
			"CSID:%d RDI%d Invalid state %d",
			csid_hw->hw_intf->hw_idx,
			rdi_cfg->path, csid_hw->rdi_cfg[rdi_cfg->path].state);
		rc = -EINVAL;
		goto end;
	}

	CAM_DBG(CAM_ISP, "CSID:%d res path :%d",
		csid_hw->hw_intf->hw_idx, rdi_cfg->path);

	rc = ais_ife_csid_enable_csi2(csid_hw, &rdi_cfg->csi_cfg);
	if (rc)
		goto end;

	if (csid_hw->device_enabled == 0) {
#if 0   // [TODO]  for ais: global reset done, sw reset need???
		rc = ais_ife_csid_reset_retain_sw_reg(csid_hw);
		if (rc < 0) {
			CAM_ERR(CAM_ISP, "CSID: Failed in SW reset");
			goto disable_csi2;
		} else {
			CAM_DBG(CAM_ISP, "CSID: SW reset Successful");
			spin_lock_irqsave(&csid_hw->lock_state, flags);
			csid_hw->device_enabled = 1;
			spin_unlock_irqrestore(&csid_hw->lock_state, flags);
		}
#else
		ais_ife_csid_ver2_enable_top_irq(csid_hw);

		CAM_DBG(CAM_ISP, "CSID: SW reset Successful");
		spin_lock_irqsave(&csid_hw->lock_state, flags);
		csid_hw->device_enabled = 1;
		spin_unlock_irqrestore(&csid_hw->lock_state, flags);
#endif
	}

	rc = ais_ife_csid_config_rdi_path(csid_hw, rdi_cfg);
	if (rc)
		goto disable_csi2;

disable_csi2:
	if (rc)
		ais_ife_csid_disable_csi2(csid_hw);
end:
	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	return rc;
}

static int ais_ife_csid_release(void *hw_priv,
	void *release_args, uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_ver2_hw                 *csid_hw;
	struct cam_hw_info                     *csid_hw_info;
	struct ais_ife_rdi_deinit_args         *rdi_deinit;

	if (!hw_priv || !release_args ||
		(arg_size != sizeof(struct ais_ife_rdi_deinit_args))) {
		CAM_ERR(CAM_ISP, "CSID:Invalid arguments");
		return -EINVAL;
	}

	CAM_DBG(CAM_ISP, "Enter");
	rdi_deinit = (struct ais_ife_rdi_deinit_args *)release_args;
	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;

	mutex_lock(&csid_hw->hw_info->hw_mutex);
	if (rdi_deinit->path >= AIS_IFE_CSID_RDI_MAX) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid path:%d",
			csid_hw->hw_intf->hw_idx, rdi_deinit->path);
		rc = -EINVAL;
		goto end;
	}

	if (csid_hw->rdi_cfg[rdi_deinit->path].state <
		AIS_ISP_RESOURCE_STATE_INIT_HW) {
		CAM_ERR(CAM_ISP,
			"CSID:%d path:%d Invalid state %d",
			csid_hw->hw_intf->hw_idx,
			rdi_deinit->path,
			csid_hw->rdi_cfg[rdi_deinit->path].state);
		rc = -EINVAL;
		goto end;
	}

	CAM_DBG(CAM_ISP, "De-Init RDI Path: %d", rdi_deinit->path);
	rc = ais_ife_csid_deinit_rdi_path(csid_hw, rdi_deinit->path);

	CAM_DBG(CAM_ISP, "De-Init ife_csid");
	rc |= ais_ife_csid_disable_csi2(csid_hw);

	CAM_DBG(CAM_ISP, "Exit");

end:
	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	return rc;
}

static int ais_ife_csid_init_hw(void *hw_priv,
	void *init_args, uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_ver2_hw                 *csid_hw;
	struct cam_hw_info                     *csid_hw_info;

	if (!hw_priv || !init_args ||
		(arg_size != sizeof(struct ais_ife_rdi_init_args))) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;

	mutex_lock(&csid_hw->hw_info->hw_mutex);

	/* Initialize the csid hardware */
	rc = ais_ife_csid_enable_hw(csid_hw);

	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	CAM_DBG(CAM_ISP, "Exit (%d)", rc);

	return rc;
}

static int ais_ife_csid_deinit_hw(void *hw_priv,
	void *deinit_args, uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_ver2_hw                 *csid_hw;
	struct cam_hw_info                     *csid_hw_info;
	struct ais_ife_rdi_deinit_args         *deinit;

	if (!hw_priv || !deinit_args ||
		(arg_size != sizeof(struct ais_ife_rdi_deinit_args))) {
		CAM_ERR(CAM_ISP, "CSID:Invalid arguments");
		return -EINVAL;
	}

	CAM_DBG(CAM_ISP, "Enter");

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;
	deinit = (struct ais_ife_rdi_deinit_args *)deinit_args;

	mutex_lock(&csid_hw->hw_info->hw_mutex);

	ais_ife_csid_path_reset(csid_hw, deinit->path);

	/* Disable CSID HW */
	CAM_DBG(CAM_ISP, "Disabling CSID Hw");
	rc = ais_ife_csid_disable_hw(csid_hw);

	mutex_unlock(&csid_hw->hw_info->hw_mutex);

	CAM_DBG(CAM_ISP, "Exit");

	return rc;
}

static int ais_ife_csid_start(void *hw_priv, void *start_args,
			uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_ver2_hw               *csid_hw;
	struct cam_hw_info                        *csid_hw_info;
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct ais_ife_rdi_start_args             *start_cmd;

	if (!hw_priv || !start_args ||
		(arg_size != sizeof(struct ais_ife_rdi_start_args))) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;
	csid_reg = csid_hw->csid_info->csid_reg;
	start_cmd = (struct ais_ife_rdi_start_args *)start_args;

	if (start_cmd->path >= csid_reg->cmn_reg->num_rdis ||
		!csid_reg->rdi_reg[start_cmd->path]) {
		CAM_ERR(CAM_ISP, "CSID:%d RDI:%d is not supported on HW",
			csid_hw->hw_intf->hw_idx, start_cmd->path);
		rc = -EINVAL;
		goto end;
	}

	/* Reset sof irq debug fields */
	csid_hw->sof_irq_triggered = false;
	csid_hw->irq_debug_cnt = 0;

	CAM_DBG(CAM_ISP, "CSID:%d res_id:%d",
		csid_hw->hw_intf->hw_idx, start_cmd->path);

	rc = ais_ife_csid_enable_rdi_path(csid_hw, start_cmd);
end:
	return rc;
}

static int ais_ife_csid_stop(void *hw_priv,
	void *stop_args, uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_ver2_hw               *csid_hw;
	struct cam_hw_info                   *csid_hw_info;
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct ais_ife_rdi_stop_args         *stop_cmd;
	uint32_t  res_mask = 0;

	if (!hw_priv || !stop_args ||
		(arg_size != sizeof(struct ais_ife_rdi_stop_args))) {
		CAM_ERR(CAM_ISP, "CSID: Invalid args");
		return -EINVAL;
	}

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;
	csid_reg = csid_hw->csid_info->csid_reg;
	stop_cmd = (struct ais_ife_rdi_stop_args  *) stop_args;

	if (stop_cmd->path >= csid_reg->cmn_reg->num_rdis ||
		!csid_reg->rdi_reg[stop_cmd->path]) {
		CAM_ERR(CAM_ISP, "CSID:%d RDI:%d is not supported on HW",
			csid_hw->hw_intf->hw_idx, stop_cmd->path);
		rc = -EINVAL;
		goto end;
	}

	CAM_DBG(CAM_ISP, "CSID:%d RDI %d",
		csid_hw->hw_intf->hw_idx,
		stop_cmd->path);

	/* Stop the resource first */
	rc = ais_ife_csid_disable_rdi_path(csid_hw, stop_cmd,
			AIS_CSID_HALT_AT_FRAME_BOUNDARY);

	if (res_mask)
		rc = ais_ife_csid_poll_stop_status(csid_hw, res_mask);

end:
	CAM_DBG(CAM_ISP,  "Exit (%d)", rc);

	return rc;

}

static int ais_ife_csid_read(void *hw_priv,
	void *read_args, uint32_t arg_size)
{
	CAM_ERR(CAM_ISP, "CSID: un supported");

	return -EINVAL;
}

static int ais_ife_csid_write(void *hw_priv,
	void *write_args, uint32_t arg_size)
{
	CAM_ERR(CAM_ISP, "CSID: un supported");
	return -EINVAL;
}

static int ais_ife_csid_sof_irq_debug(
	struct ais_ife_csid_ver2_hw *csid_hw, void *cmd_args)
{
	int i = 0;
	uint32_t val = 0;
	bool sof_irq_enable = false;
	const struct ais_ife_csid_ver2_reg_offset    *csid_reg;
	struct cam_hw_soc_info                  *soc_info;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (*((uint32_t *)cmd_args) == 1)
		sof_irq_enable = true;

	if (csid_hw->hw_info->hw_state ==
		CAM_HW_STATE_POWER_DOWN) {
		CAM_WARN(CAM_ISP,
			"CSID powered down unable to %s sof irq",
			(sof_irq_enable == true) ? "enable" : "disable");
		return 0;
	}

	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csid_reg->rdi_reg[i]->csid_rdi_irq_mask_addr);
		if (val) {
			if (sof_irq_enable)
				val |= CSID_PATH_INFO_INPUT_SOF;
			else
				val &= ~CSID_PATH_INFO_INPUT_SOF;

			cam_io_w_mb(val, soc_info->reg_map[0].mem_base +
				csid_reg->rdi_reg[i]->csid_rdi_irq_mask_addr);
			val = 0;
		}
	}

	if (sof_irq_enable) {
		csid_hw->csid_debug |= CSID_DEBUG_ENABLE_SOF_IRQ;
		csid_hw->sof_irq_triggered = true;
	} else {
		csid_hw->csid_debug &= ~CSID_DEBUG_ENABLE_SOF_IRQ;
		csid_hw->sof_irq_triggered = false;
	}

	CAM_INFO(CAM_ISP, "SOF freeze: CSID SOF irq %s",
		(sof_irq_enable == true) ? "enabled" : "disabled");

	return 0;
}

static int ais_ife_csid_set_csid_clock(
	struct ais_ife_csid_ver2_hw *csid_hw, void *cmd_args)
{
	struct ais_ife_csid_clock_update_args *clk_update = NULL;

	if (!csid_hw)
		return -EINVAL;

	clk_update =
		(struct ais_ife_csid_clock_update_args *)cmd_args;

	csid_hw->clk_rate = clk_update->clk_rate;
	CAM_INFO(CAM_ISP, "CSID clock rate %llu", csid_hw->clk_rate);

	return 0;
}

static int ais_ife_csid_dump_hw(
	struct ais_ife_csid_ver2_hw *csid_hw, void *cmd_args)
{
	struct cam_hw_soc_info                         *soc_info;
	struct ais_isp_hw_dump_args *dump_args =
		(struct ais_isp_hw_dump_args *)cmd_args;
	int i;
	uint32_t *addr, *start;
	uint32_t num_reg;
	struct ais_isp_hw_dump_header *hdr;
	uint8_t *dst;

	if (!dump_args->cpu_addr || !dump_args->buf_len) {
		CAM_ERR(CAM_ISP,
			"lnvalid len %zu ", dump_args->buf_len);
		return -EINVAL;
	}
	soc_info = &csid_hw->hw_info->soc_info;
	/*100 bytes we store the meta info of the dump data*/
	if ((dump_args->buf_len - dump_args->offset) <
			soc_info->reg_map[0].size + 100) {
		CAM_ERR(CAM_ISP, "Dump buffer exhaust");
		return 0;
	}
	dst = (char *)dump_args->cpu_addr + dump_args->offset;
	hdr = (struct ais_isp_hw_dump_header *)dst;
	snprintf(hdr->tag, AIS_ISP_HW_DUMP_TAG_MAX_LEN,
		"CSID_REG:");
	addr = (uint32_t *)(dst + sizeof(struct ais_isp_hw_dump_header));

	start = addr;
	num_reg = soc_info->reg_map[0].size/4;
	hdr->word_size = sizeof(uint32_t);
	*addr = soc_info->index;
	addr++;
	for (i = 0; i < num_reg; i++) {
		addr[0] = soc_info->mem_block[0]->start + (i*4);
		addr[1] = cam_io_r(soc_info->reg_map[0].mem_base
			+ (i*4));
		addr += 2;
	}
	hdr->size = hdr->word_size * (addr - start);
	dump_args->offset +=  hdr->size +
		sizeof(struct ais_isp_hw_dump_header);
	CAM_DBG(CAM_ISP, "offset %d", dump_args->offset);
	return 0;
}

static int ais_ife_csid_get_total_pkts(
	struct ais_ife_csid_ver2_hw *csid_hw, void *cmd_args)
{
	struct ais_ife_diag_info                       *ife_diag;

	struct cam_hw_soc_info                         *soc_info;
	const struct ais_ife_csid_ver2_reg_offset           *csid_reg;

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid hw state :%d",
			csid_hw->hw_intf->hw_idx,
			csid_hw->hw_info->hw_state);
		return -EINVAL;
	}

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	ife_diag = (struct ais_ife_diag_info *)cmd_args;

	ife_diag->pkts_rcvd = cam_io_r_mb(soc_info->reg_map[0].mem_base +
		csid_reg->csi2_reg->csid_csi2_rx_total_pkts_rcvd_addr);

	return 0;
}

static int ais_ife_csid_print_overflow_info(
	struct ais_ife_csid_ver2_hw *csid_hw, void *cmd_args)
{
	int rc = 0;
	uint32_t byteCntr[2] = {};
	int *path;
	int id;
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct cam_hw_soc_info                 *soc_info;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	path = (int *)cmd_args;
	id = *path;
	byteCntr[0] = cam_io_r_mb(soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_byte_cntr_ping_addr);
	byteCntr[1] = cam_io_r_mb(soc_info->reg_map[0].mem_base +
		csid_reg->rdi_reg[id]->csid_rdi_byte_cntr_pong_addr);

	CAM_ERR(CAM_ISP, "CSID:%d RDI:%d byteCntr[2] %u %u",
		csid_hw->hw_intf->hw_idx, id, byteCntr[0], byteCntr[1]);

	return rc;
}

static int ais_ife_csid_process_cmd(void *hw_priv,
	uint32_t cmd_type, void *cmd_args, uint32_t arg_size)
{
	int rc = 0;
	struct ais_ife_csid_ver2_hw               *csid_hw;
	struct cam_hw_info                   *csid_hw_info;

	if (!hw_priv || !cmd_args) {
		CAM_ERR(CAM_ISP, "CSID: Invalid arguments");
		return -EINVAL;
	}

	csid_hw_info = (struct cam_hw_info  *)hw_priv;
	csid_hw = (struct ais_ife_csid_ver2_hw   *)csid_hw_info->core_info;

	switch (cmd_type) {
	case AIS_IFE_CSID_CMD_GET_TIME_STAMP:
		rc = ais_ife_csid_get_time_stamp(csid_hw, cmd_args);
		break;
	case AIS_IFE_CSID_SET_CSID_DEBUG:
		rc = ais_ife_csid_set_csid_debug(csid_hw, cmd_args);
		break;
	case AIS_IFE_CSID_SOF_IRQ_DEBUG:
		rc = ais_ife_csid_sof_irq_debug(csid_hw, cmd_args);
		break;
	case AIS_ISP_HW_CMD_CSID_CLOCK_UPDATE:
		rc = ais_ife_csid_set_csid_clock(csid_hw, cmd_args);
		break;
	case AIS_ISP_HW_CMD_DUMP_HW:
		rc = ais_ife_csid_dump_hw(csid_hw, cmd_args);
		break;
	case AIS_IFE_CSID_CMD_DIAG_INFO:
		rc = ais_ife_csid_get_total_pkts(csid_hw, cmd_args);
		break;
	case AIS_IFE_CSID_CMD_OVERFLOW_INFO:
		rc = ais_ife_csid_print_overflow_info(csid_hw, cmd_args);
		break;
	default:
		CAM_ERR(CAM_ISP, "CSID:%d unsupported cmd:%d",
			csid_hw->hw_intf->hw_idx, cmd_type);
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int ais_csid_event_dispatch_process(void *priv, void *data)
{
	struct ais_ife_event_data evt_payload = {};
	struct ais_ife_csid_ver2_hw *csid_hw;
	struct ais_csid_ver2_hw_work_data *work_data;
	int rc = 0;

	csid_hw = (struct ais_ife_csid_ver2_hw *)priv;
	if (!csid_hw) {
		CAM_ERR(CAM_ISP, "Invalid parameters");
		return -EINVAL;
	}
	if (!csid_hw->event_cb) {
		CAM_ERR(CAM_ISP, "CSID%d Error Cb not registered",
			csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}
	work_data = (struct ais_csid_ver2_hw_work_data *)data;

	CAM_ERR_RATE_LIMIT(CAM_ISP, "CSID%d [%d %d] TOP:0x%x RX:0x%x",
			csid_hw->hw_intf->hw_idx,
			work_data->evt_type,
			csid_hw->csi2_rx_cfg.phy_sel,
			work_data->irq_status[CSID_VER2_IRQ_STATUS_TOP],
			work_data->irq_status[CSID_VER2_IRQ_STATUS_RX]);

	CAM_ERR_RATE_LIMIT(CAM_ISP, " RDIs 0x%x 0x%x 0x%x 0x%x",
		work_data->irq_status[CSID_VER2_IRQ_STATUS_RDI0],
		work_data->irq_status[CSID_VER2_IRQ_STATUS_RDI1],
		work_data->irq_status[CSID_VER2_IRQ_STATUS_RDI2],
		work_data->irq_status[CSID_VER2_IRQ_STATUS_RDI3]);

	evt_payload.msg.idx = csid_hw->hw_intf->hw_idx;
	evt_payload.msg.boot_ts = work_data->timestamp;
	evt_payload.msg.path = 0xF;
	evt_payload.msg.reserved = sizeof(struct ais_ife_event_data);
	evt_payload.u.err_msg.reserved =
		work_data->irq_status[CSID_VER2_IRQ_STATUS_RX];

	switch (work_data->evt_type) {
	case AIS_IFE_MSG_CSID_ERROR:
		if (csid_hw->fatal_err_detected)
			break;
		csid_hw->fatal_err_detected = true;

		evt_payload.msg.type = AIS_IFE_MSG_CSID_ERROR;

		rc = csid_hw->event_cb(csid_hw->event_cb_priv, &evt_payload);
		break;

	case AIS_IFE_MSG_CSID_WARNING:
		break;
	default:
		CAM_DBG(CAM_ISP, "CSID[%d] invalid error type %d",
			csid_hw->hw_intf->hw_idx,
			work_data->evt_type);
		break;
	}
	return rc;
}

static int ais_ife_csid_update_rup(
		struct ais_ife_csid_ver2_hw *csid_hw,
		struct ais_ife_csid_ver2_rup_aup_mask *rup_aup_mask)
{

	const struct ais_ife_csid_ver2_reg_offset  *csid_reg;
	struct cam_hw_soc_info                     *soc_info;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (csid_hw->hw_info->hw_state != CAM_HW_STATE_POWER_UP) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid dev state :%d",
								csid_hw->hw_intf->hw_idx,
								csid_hw->hw_info->hw_state);
		return -EINVAL;
	}

	if (!rup_aup_mask) {
		CAM_ERR(CAM_ISP, "CSID:%d Invalid input data", csid_hw->hw_intf->hw_idx);
		return -EINVAL;
	}

	ais_ife_csid_ver2_config_rup_aup(csid_hw, rup_aup_mask);

    return 0;
}

static int ais_ife_csid_event_csid_work(void *priv, void *data)
{
	struct ais_ife_csid_ver2_hw *csid_hw;
	struct ais_csid_ver2_hw_work_data *work_data;
	int rc = 0;

	csid_hw = (struct ais_ife_csid_ver2_hw *)priv;
	if (!csid_hw) {
		CAM_ERR(CAM_ISP, "Invalid parameters");
		return -EINVAL;
	}

	work_data = (struct ais_csid_ver2_hw_work_data *)data;

	switch (work_data->evt_type) {
	case AIS_IFE_MSG_CSID_RUP_DONE:
		ais_ife_csid_update_rup(csid_hw,
			(struct ais_ife_csid_ver2_rup_aup_mask *)
			work_data->data);
		break;
	default:
		CAM_DBG(CAM_ISP, "CSID[%d] invalid error type %d",
			csid_hw->hw_intf->hw_idx,
			work_data->evt_type);
		break;
	}
	return rc;
}

static void ais_ife_csid_ver2_top_debug_top_half(
	struct ais_ife_csid_ver2_hw *csid_hw,
	uint32_t top_irq, uint32_t top2_irq)
{
	uint32_t top_mask = csid_hw->
						debug_info.top_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1];
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	uint8_t *debug_bit_pos = NULL;

	csid_reg = csid_hw->csid_info->csid_reg;
	debug_bit_pos = csid_reg->top_debug_mask->bit_pos;

	if ((top_mask &
		BIT(debug_bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_UP])) &&
		(top2_irq &
		BIT(debug_bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_UP]))) {
		CAM_INFO(CAM_ISP, "CSID:%d INFO_VOTE_UP", csid_hw->hw_intf->hw_idx);
	}

	if ((top_mask &
		BIT(debug_bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_DN])) &&
		(top2_irq &
		BIT(debug_bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_DN]))) {
		CAM_INFO(CAM_ISP, "CSID:%d INFO_VOTE_UP", csid_hw->hw_intf->hw_idx);
	}

	if ((top_mask &
		BIT(debug_bit_pos[AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN])) &&
		(top2_irq &
		BIT(debug_bit_pos[AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN]))) {
		CAM_INFO(CAM_ISP, "CSID:%d ERR_NO_VOTE_DN", csid_hw->hw_intf->hw_idx);
	}
}

static void ais_ife_csid_ver2_handle_rx_debug_event(
	struct ais_ife_csid_ver2_hw *csid_hw, uint32_t rx_idx,
	uint32_t bit_pos, uint32_t *rst_strobe_val)
{
	uint32_t val = 0;
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg;
	struct cam_hw_soc_info *soc_info;
	uint8_t *debug_bit_pos = NULL;
	uint64_t *evt_bitmap = NULL;
	uint32_t mask = 0;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = csid_hw->csid_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;

	mask = BIT(bit_pos);
	debug_bit_pos = csid_reg->rx_debug_mask->bit_pos;
	evt_bitmap = csid_reg->rx_debug_mask->evt_bitmap;

	/* csi rx debug process */
	if ((evt_bitmap[rx_idx] &
		 BIT_ULL(AIS_IFE_CSID_RX_DL0_EOT_CAPTURED)) &&
		(mask ==
		 BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL0_EOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL0_EOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_DL1_EOT_CAPTURED)) &&
				(mask ==
				 BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL1_EOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL1_EOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_DL2_EOT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL2_EOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL2_EOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_DL3_EOT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL3_EOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL3_EOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_DL0_SOT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL0_SOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL0_SOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_DL1_SOT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL1_SOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL1_SOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_DL2_SOT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL2_SOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL2_SOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_DL3_SOT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_DL3_SOT_CAPTURED])))
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d PHY_DL3_SOT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
	else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_LONG_PKT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_LONG_PKT_CAPTURED]))) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%d LONG_PKT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->csid_csi2_rx_captured_long_pkt_0_addr);
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d long packet VC :%d DT:%d WC:%d",
			csid_hw->hw_intf->hw_idx,
			((val & csi2_reg->vc_mask) >> csi2_reg->vc_shift),
			((val & csi2_reg->dt_mask) >> csi2_reg->dt_shift),
			((val & csi2_reg->wc_mask) >> csi2_reg->wc_shift));

		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->csid_csi2_rx_captured_long_pkt_1_addr);
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%d long packet ECC :%d",
			csid_hw->hw_intf->hw_idx, val);
#if 0 // [TODO] for ais: crc error
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->csid_csi2_rx_captured_long_pkt_ftr_addr);
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d long pkt cal CRC:%d expected CRC:%d",
			csid_hw->hw_intf->hw_idx, (val >> 16), (val & 0xFFFF));
#endif
		*rst_strobe_val |= (1 << csi2_reg->long_pkt_strobe_rst_shift);
	} else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED]))) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%d SHORT_PKT_CAPTURED",
			csid_hw->hw_intf->hw_idx);
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->csid_csi2_rx_captured_short_pkt_0_addr);
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d short pkt VC :%d DT:%d LC:%d",
			csid_hw->hw_intf->hw_idx,
			((val & csi2_reg->vc_mask) >> csi2_reg->vc_shift),
			((val & csi2_reg->dt_mask) >> csi2_reg->dt_shift),
			((val & csi2_reg->wc_mask) >> csi2_reg->wc_shift));
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->csid_csi2_rx_captured_short_pkt_1_addr);
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%d short packet ECC :%d",
			csid_hw->hw_intf->hw_idx, val);

		*rst_strobe_val |= (1 << csi2_reg->short_pkt_strobe_rst_shift);
	} else if ((evt_bitmap[rx_idx] &
				BIT_ULL(AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED)) &&
				(mask ==
				BIT(debug_bit_pos[AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED]))) {
		CAM_INFO_RATE_LIMIT(CAM_ISP, "CSID:%d CPHY_PKT_HDR_CAPTURED",
			csid_hw->hw_intf->hw_idx);
		val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
			csi2_reg->csid_csi2_rx_captured_cphy_pkt_hdr_addr);
		CAM_INFO_RATE_LIMIT(CAM_ISP,
			"CSID:%d cphy packet VC :%d DT:%d WC:%d",
			csid_hw->hw_intf->hw_idx,
			((val & csi2_reg->vc_mask) >> csi2_reg->vc_shift),
			((val & csi2_reg->dt_mask) >> csi2_reg->dt_shift),
			((val & csi2_reg->wc_mask) >> csi2_reg->wc_shift));

		*rst_strobe_val |= (1 << csi2_reg->cphy_pkt_strobe_rst_shift);
	}
}

static void ais_ife_csid_ver2_rx_debug_top_half(
						struct ais_ife_csid_ver2_hw *csid_hw,
						uint32_t *irq_status)
{
	uint32_t rx_status = irq_status[CSID_VER2_IRQ_STATUS_RX];
	uint32_t rst_strobe_val = 0;
	uint32_t bit_pos = 0, bit_set = 0;

	if (rx_status &
			csid_hw->debug_info.rx_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0]) {
		while (rx_status) {
			bit_set = rx_status & 1;
			if ((bit_set) && (BIT(bit_pos) &
				csid_hw->debug_info.rx_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0]))
				ais_ife_csid_ver2_handle_rx_debug_event(csid_hw,
										AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0,
										bit_pos, &rst_strobe_val);
			bit_pos++;
			rx_status >>= 1;
		}
	}

	rx_status = irq_status[CSID_VER2_IRQ_STATUS_RX_1];
	if (rx_status &
			csid_hw->debug_info.rx_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1]) {
		while (rx_status) {
			bit_set = rx_status & 1;
			if ((bit_set) && (BIT(bit_pos) &
				csid_hw->debug_info.rx_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1]))
				ais_ife_csid_ver2_handle_rx_debug_event(csid_hw,
										AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1,
										bit_pos, &rst_strobe_val);
			bit_pos++;
			rx_status >>= 1;
		}
	}

	/* Reset strobes for next set of pkts */
	if (rst_strobe_val) {
		struct cam_hw_soc_info *soc_info = &csid_hw->hw_info->soc_info;
		const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
		struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg;

		csid_reg = csid_hw->csid_info->csid_reg;
		csi2_reg = csid_reg->csi2_reg;

		cam_io_w_mb(rst_strobe_val, soc_info->reg_map[0].mem_base +
				csi2_reg->csid_csi2_rx_rst_strobes_addr);
	}
}

static void ais_ife_csid_ver2_path_debug_top_half(
	struct ais_ife_csid_ver2_hw *csid_hw,
	uint32_t* irq_status)
{
	struct cam_hw_soc_info *soc_info;
	const struct ais_ife_csid_ver2_reg_offset   *csid_reg;
	struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg;
	struct ais_ife_csid_ver2_path_cfg *path_data;
	uint8_t *debug_bit_pos = NULL;
	int i = 0;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = csid_hw->csid_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;

	debug_bit_pos = csid_reg->path_debug_mask->bit_pos;

	/* RDI path debug process */
	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		path_data = (struct ais_ife_csid_ver2_path_cfg *)
			&csid_hw->rdi_cfg[i];

		if ((irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] &
			BIT(debug_bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_SOF])) &&
			(csid_hw->debug_info.path_mask &
			 BIT(debug_bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_SOF]))) {
			CAM_INFO(CAM_ISP,
				"CSID %u RDI:%d SOF received",
				csid_hw->hw_intf->hw_idx, i);
			if (csid_hw->sof_irq_triggered)
				csid_hw->irq_debug_cnt++;
		}

		if ((irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] &
			BIT(debug_bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_EOF])) &&
			(csid_hw->debug_info.path_mask &
			 BIT(debug_bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_EOF])))
			CAM_INFO(CAM_ISP,
				"CSID %u RDI:%d EOF received",
				csid_hw->hw_intf->hw_idx, i);
	}
}

static int ais_csid_dispatch_irq(struct ais_ife_csid_ver2_hw *csid_hw,
	enum ais_ife_csid_ver2_cb_type cb_type,
	int evt_type, uint32_t *irq_status, uint64_t timestamp,
	void* data, int size)
{
	struct crm_workq_task *task = NULL;
	struct ais_csid_ver2_hw_work_data *work_data;
	int rc = 0;

	task = cam_req_mgr_workq_get_task(csid_hw->work);
	if (!task) {
		CAM_ERR(CAM_ISP, "Can not get task for worker[0x%x]", cb_type);
		return -ENOMEM;
	}

	work_data = (struct ais_csid_ver2_hw_work_data *)task->payload;
	work_data->evt_type = evt_type;
	work_data->timestamp = timestamp;

	CAM_DBG(CAM_ISP, "CSID[%d] cb_type %d, ev_type %d",
		csid_hw->hw_intf->hw_idx, cb_type, evt_type);

	switch (cb_type) {
		case AIS_CSID_VER2_IRQ_CB_IFE_WORK:
			memcpy(work_data->irq_status, irq_status, sizeof(uint32_t) * CSID_VER2_IRQ_STATUS_MAX);
			task->process_cb = ais_csid_event_dispatch_process;
			break;
		case AIS_CSID_VER2_IRQ_CB_CSID_WORK:
			if (size > AIS_IFE_CSID_WORK_MAX_DATA_SIZE) {
				CAM_ERR(CAM_ISP, "CSID Work size too larger[%d]", size);
				return -EINVAL;
			}

			memcpy(work_data->data, data, size);
			task->process_cb = ais_ife_csid_event_csid_work;
			break;
		default:
			break;
	}

	if (task)
		rc = cam_req_mgr_workq_enqueue_task(task, csid_hw,
			CRM_TASK_PRIORITY_0);

	return rc;
}

static void ais_ife_csid_ver2_read_irq(
	struct ais_ife_csid_ver2_hw *csid_hw, uint32_t *irq_status)
{
	struct cam_hw_soc_info                              *soc_info;
	const struct ais_ife_csid_ver2_reg_offset           *csid_reg;
	const struct ais_ife_csid_ver2_csi2_rx_reg_offset   *csi2_reg;
	const struct ais_ife_csid_ver2_rdi_reg_offset       *rdi_reg;
	struct ais_ife_csid_ver2_common_reg_offset          *cmn_reg;
	uint32_t i;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	csi2_reg = csid_reg->csi2_reg;
	cmn_reg = csid_reg->cmn_reg;

	/* read irq */
	/* TOP */
	for (i = 0; i < cmn_reg->num_top_irq; i++) {
		irq_status[CSID_VER2_IRQ_STATUS_TOP + i] =
			cam_io_r_mb(soc_info->reg_map[0].mem_base +
						cmn_reg->csid_top_irq_status_addr[i]);
	}

	/* Buf Done */
	if (irq_status[CSID_VER2_IRQ_STATUS_TOP] &
							cmn_reg->top_buf_done_irq_mask)
		irq_status[CSID_VER2_IRQ_STATUS_BUF_DONE] =
					cam_io_r_mb(soc_info->reg_map[0].mem_base +
							cmn_reg->buf_done_irq_status_addr);

	/* RX */
	for (i = 0; i < csi2_reg->num_rx_irq; i++) {
		irq_status[CSID_VER2_IRQ_STATUS_RX + i] =
			cam_io_r_mb(soc_info->reg_map[0].mem_base +
						csi2_reg->csid_csi2_rx_irq_status_addr[i]);
	}

	/* RDI path */
	for (i = 0; i < cmn_reg->num_rdis; i++) {
		rdi_reg = csid_reg->rdi_reg[i];
		irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] =
			cam_io_r_mb(soc_info->reg_map[0].mem_base +
						rdi_reg->csid_rdi_irq_status_addr);
	}

	/* clear irq */
	for (i = 0; i < cmn_reg->num_rdis; i++) {
		rdi_reg = csid_reg->rdi_reg[i];
		cam_io_w_mb(irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i],
			soc_info->reg_map[0].mem_base +
			rdi_reg->csid_rdi_irq_clear_addr);
	}

	for (i = 0; i < csi2_reg->num_rx_irq; i++)
		cam_io_w_mb(irq_status[CSID_VER2_IRQ_STATUS_RX + i],
			soc_info->reg_map[0].mem_base +
			csi2_reg->csid_csi2_rx_irq_clear_addr[i]);

	cam_io_w_mb(irq_status[CSID_VER2_IRQ_STATUS_BUF_DONE],
			soc_info->reg_map[0].mem_base +
			cmn_reg->buf_done_irq_clear_addr);

	for (i = 0; i < cmn_reg->num_top_irq; i++)
		cam_io_w_mb(irq_status[CSID_VER2_IRQ_STATUS_TOP + i],
				soc_info->reg_map[0].mem_base +
				cmn_reg->csid_top_irq_clear_addr[i]);

	cam_io_w_mb(1, soc_info->reg_map[0].mem_base +
			cmn_reg->csid_irq_cmd_addr);
}

static int ais_ife_csid_ver2_irq_err_top_half(
	struct ais_ife_csid_ver2_hw *csid_hw,
	uint32_t *irq_status,
	struct timespec64 *ts)
{
	struct cam_hw_soc_info *soc_info;
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct ais_ife_csid_ver2_common_reg_offset *cmn_reg;
	const struct ais_ife_csid_ver2_csi2_rx_reg_offset *csi2_reg;
	uint32_t warn_cnt = 0;
	bool fatal_err_detected = false;
	unsigned long flags;
	int rc = 0;
	int i = 0;

	soc_info = &csid_hw->hw_info->soc_info;
	csid_reg = csid_hw->csid_info->csid_reg;
	csi2_reg = csid_reg->csi2_reg;
	cmn_reg = csid_reg->cmn_reg;

	/* error detect */
	spin_lock_irqsave(&csid_hw->lock_state, flags);
	if (csid_hw->device_enabled == 1) {
		/* top irq error */
		if (irq_status[CSID_VER2_IRQ_STATUS_TOP] &
			cmn_reg->top_err_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0]) {
			fatal_err_detected = true;
		}

		/* top2 irq erro */
		if ((cmn_reg->num_top_irq > 1) &&
			(irq_status[CSID_VER2_IRQ_STATUS_TOP_1] &
			cmn_reg->top_err_irq_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1])) {
			warn_cnt++;
		}


		/* rx irq error */
		if (irq_status[CSID_VER2_IRQ_STATUS_RX] &
			csi2_reg->fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0]) {
			fatal_err_detected = true;
		}

		if (irq_status[CSID_VER2_IRQ_STATUS_RX] &
			csi2_reg->non_fatal_err_mask[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0]) {
			warn_cnt++;
		}

	}
	csid_hw->error_irq_count += warn_cnt;

	spin_unlock_irqrestore(&csid_hw->lock_state, flags);

	if (csid_hw->error_irq_count >
		AIS_IFE_CSID_MAX_IRQ_ERROR_COUNT) {
		spin_lock_irqsave(&csid_hw->lock_state, flags);
		fatal_err_detected = true;
		csid_hw->error_irq_count = 0;
		spin_unlock_irqrestore(&csid_hw->lock_state, flags);
	} else if (warn_cnt) {
		uint64_t timestamp;

		timestamp = (uint64_t)((ts->tv_sec * 1000000000) + ts->tv_nsec);
		ais_csid_dispatch_irq(csid_hw,
			AIS_CSID_VER2_IRQ_CB_IFE_WORK,
			AIS_IFE_MSG_CSID_WARNING,
			irq_status, timestamp, NULL, 0);

		rc = -1;
	}

	if (fatal_err_detected) {
		uint64_t timestamp;

		timestamp = (uint64_t)((ts->tv_sec * 1000000000) + ts->tv_nsec);
		CAM_INFO(CAM_ISP,
			"CSID: %d cnt: %d Halt csi2 rx irq_status_rx:0x%x",
			csid_hw->hw_intf->hw_idx, csid_hw->csi2_cfg_cnt,
			irq_status[CSID_VER2_IRQ_STATUS_RX]);
		ais_ife_csid_halt_csi2(csid_hw);
		ais_csid_dispatch_irq(csid_hw,
			AIS_CSID_VER2_IRQ_CB_IFE_WORK,
			AIS_IFE_MSG_CSID_ERROR,
			irq_status, timestamp, NULL, 0);

		rc = -2;
	}

	/* RDI path err process */
	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		struct ais_ife_csid_ver2_rdi_reg_offset *rdi_reg = NULL;
		uint32_t val = 0;
		uint32_t val2 = 0;
		rdi_reg = csid_reg->rdi_reg[i];
		if ((irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] &
			CSID_PATH_ERROR_REC_OVERFLOW) ||
			(irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] &
			 CSID_PATH_ERROR_CAMIF_CCIF_VIOLATION)) {
			CAM_ERR_RATE_LIMIT(CAM_ISP,
				"CSID:%d irq_status_rdi[%d]:0x%x",
				csid_hw->hw_intf->hw_idx, i,
				irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i]);
			/* Stop RDI path immediately */
			cam_io_w_mb(0x1,
				soc_info->reg_map[0].mem_base +
				rdi_reg->csid_rdi_ctrl_addr);
		}

		if ((irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] &
					CSID_PATH_ERROR_PIX_COUNT) ||
			(irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] &
					CSID_PATH_ERROR_LINE_COUNT)) {
			val = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				rdi_reg->csid_rdi_format_measure0_addr);
			val2 = cam_io_r_mb(soc_info->reg_map[0].mem_base +
				rdi_reg->csid_rdi_format_measure_cfg1_addr);
			CAM_ERR(CAM_ISP,
				"CSID:%d irq_status_rdi[%d]:0x%x",
				csid_hw->hw_intf->hw_idx, i,
				irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i]);
			CAM_ERR(CAM_ISP,
				"Expected sz 0x%x*0x%x actual sz 0x%x*0x%x",
				((val2 >>
				csid_reg->cmn_reg->format_measure_height_shift_val) &
				csid_reg->cmn_reg->format_measure_height_mask_val),
				val2 &
				csid_reg->cmn_reg->format_measure_width_mask_val,
				((val >>
				csid_reg->cmn_reg->format_measure_height_shift_val) &
				csid_reg->cmn_reg->format_measure_height_mask_val),
				val &
				csid_reg->cmn_reg->format_measure_width_mask_val);
		}
	}

	return rc;
}

static void ais_ife_csid_ver2_irq_debug_top_half(
	struct ais_ife_csid_ver2_hw *csid_hw,
	uint32_t *irq_status)
{
	if (!csid_hw->debug_info.debug_val)
		return;

	/* top irq debug */
	if ((csid_hw->debug_info.top_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0] &
			irq_status[CSID_VER2_IRQ_STATUS_TOP]) ||
		(csid_hw->debug_info.top_mask[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1] &
			irq_status[CSID_VER2_IRQ_STATUS_TOP_1])) {
		ais_ife_csid_ver2_top_debug_top_half(csid_hw,
									irq_status[CSID_VER2_IRQ_STATUS_TOP],
									irq_status[CSID_VER2_IRQ_STATUS_TOP_1]);
	}

	/* rx irq debug */
	ais_ife_csid_ver2_rx_debug_top_half(csid_hw, irq_status);

	/* path irq debug */
	if (csid_hw->debug_info.path_mask) {
		ais_ife_csid_ver2_path_debug_top_half(csid_hw, irq_status);
	}

#if 0 //[TODO] for ais:
	if (csid_hw->irq_debug_cnt >= AIS_CSID_IRQ_SOF_DEBUG_CNT_MAX) {
		ais_ife_csid_sof_irq_debug(csid_hw, &sof_irq_debug_en);
		csid_hw->irq_debug_cnt = 0;
	}
#endif
}

static void ais_ife_csid_ver2_irq_process_top_half(
								struct ais_ife_csid_ver2_hw *csid_hw,
								uint32_t *irq_status,
								struct timespec64 *ts)
{
	const struct ais_ife_csid_ver2_reg_offset *csid_reg;
	struct ais_csid_ver2_irq_event irq_event = {0};
	struct ais_ife_csid_ver2_rup_aup_mask rup_aup_mask = {0};
	struct ais_ife_csid_ver2_path_cfg *path_data = NULL;
	struct ais_ife_csid_ver2_rdi_reg_offset *rdi_reg = NULL;
	struct cam_hw_soc_info *soc_info;
	uint32_t i;

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;

	if (csid_hw->vfe_event_cb) {
		irq_event.priv = csid_hw->vfe_hw_info;
		irq_event.rdi_num = csid_reg->cmn_reg->num_rdis;
		irq_event.csid_hw = csid_hw;
		memcpy(irq_event.status, irq_status, sizeof(uint32_t) * CSID_VER2_IRQ_STATUS_MAX);

		/* SOF irq trigger vfe process */
		irq_event.event = AIS_CSID_VER2_IRQ_EV_SOF;
		csid_hw->vfe_event_cb(&irq_event);

		/* buffer done trigger vfe process */
		if (irq_status[CSID_VER2_IRQ_STATUS_BUF_DONE]) {
			irq_event.event = AIS_CSID_VER2_IRQ_EV_BUF_DONE;
			csid_hw->vfe_event_cb(&irq_event);
		}
	}

	for (i = 0; i < csid_reg->cmn_reg->num_rdis; i++) {
		path_data = &csid_hw->rdi_cfg[i];
		rdi_reg = csid_reg->rdi_reg[i];

		if (irq_status[CSID_VER2_IRQ_STATUS_RDI0 + i] &
				AIS_CSID_VER2_PATH_INFO_INPUT_SOF) {
			if ((path_data->init_frame_drop) &&
					(path_data->state == AIS_ISP_RESOURCE_STATE_STREAMING)) {
				path_data->sof_cnt++;
				CAM_DBG(CAM_ISP,
						"CSID:%d RDI:%d SOF cnt:%d init_frame_drop:%d",
						csid_hw->hw_intf->hw_idx, i,
						path_data->sof_cnt,
						path_data->init_frame_drop);

				if (path_data->sof_cnt == path_data->init_frame_drop) {
					cam_io_w_mb(0x1, soc_info->reg_map[0].mem_base +
							rdi_reg->csid_rdi_ctrl_addr);
					path_data->init_frame_drop = 0;
				}
			} else if (!path_data->init_frame_drop)
				ais_ife_csid_ver2_path_rup_aup(csid_hw, 1, i, &rup_aup_mask);
		}
	}

	/* RUP AUP cmd */
	if (rup_aup_mask.rup_mask) {
#ifdef RUP_CONFIG_IN_IRQ
		ais_ife_csid_ver2_config_rup_aup(csid_hw, &rup_aup_mask);
#else
	    uint64_t timestamp;

	    timestamp = (uint64_t)((ts->tv_sec * 1000000000) + ts->tv_nsec);

	    ais_csid_dispatch_irq(csid_hw,
			AIS_CSID_VER2_IRQ_CB_CSID_WORK,
			AIS_IFE_MSG_CSID_RUP_DONE,
			irq_status, timestamp, &rup_aup_mask,
			sizeof(struct ais_ife_csid_ver2_rup_aup_mask));
#endif
	}
}

static irqreturn_t ais_ife_csid_irq(int irq_num, void *data)
{
	struct ais_ife_csid_ver2_hw                         *csid_hw;
	struct cam_hw_soc_info                              *soc_info;
	const struct ais_ife_csid_ver2_reg_offset           *csid_reg;
	const struct ais_ife_csid_ver2_csi2_rx_reg_offset   *csi2_reg;
	struct ais_ife_csid_ver2_common_reg_offset          *cmn_reg;
	uint32_t irq_status[CSID_VER2_IRQ_STATUS_MAX] = {0};
	struct timespec64 ts;
	int rc = 0;

	if (!data) {
		CAM_ERR(CAM_ISP, "CSID: Invalid arguments");
		return IRQ_HANDLED;
	}

	ktime_get_boottime_ts64(&ts);

	csid_hw = (struct ais_ife_csid_ver2_hw *)data;
	CAM_DBG(CAM_ISP, "CSID %d IRQ Handling", csid_hw->hw_intf->hw_idx);

	csid_reg = csid_hw->csid_info->csid_reg;
	soc_info = &csid_hw->hw_info->soc_info;
	csi2_reg = csid_reg->csi2_reg;
	cmn_reg = csid_reg->cmn_reg;

	ais_ife_csid_ver2_read_irq(csid_hw, irq_status);

	if (irq_status[CSID_VER2_IRQ_STATUS_TOP] &
		BIT(csi2_reg->csi2_rst_done_shift_val)) {
		CAM_DBG(CAM_ISP, "csid reset complete");
		complete(&csid_hw->hw_info->hw_complete);
	}

	if (csid_hw->device_enabled == 0) {
	    CAM_DBG(CAM_ISP, "CSID %d IRQ trigger, but device is disabled",
	            csid_hw->hw_intf->hw_idx);
	    return IRQ_HANDLED;
	}

	/* error process */
	rc = ais_ife_csid_ver2_irq_err_top_half(csid_hw, irq_status, &ts);
	if (rc == -2)
		return IRQ_HANDLED;

	/* top half process */
	ais_ife_csid_ver2_irq_process_top_half(csid_hw, irq_status, &ts);

	/* debug process */
	ais_ife_csid_ver2_irq_debug_top_half(csid_hw, irq_status);

	CAM_DBG(CAM_ISP, "IRQ Handling exit");
	return IRQ_HANDLED;
}

static void cam_req_mgr_process_workq_csid_worker(struct work_struct *w)
{
	cam_req_mgr_process_workq(w);
}

int ais_ife_csid_ver2_hw_probe_init(struct cam_hw_intf  *csid_hw_intf,
	uint32_t csid_idx)
{
	int rc = -EINVAL;
	uint32_t i;
	struct cam_hw_info                   *csid_hw_info = NULL;
	struct ais_ife_csid_ver2_hw          *ife_csid_hw = NULL;
	char worker_name[128];
	uint32_t debug_val = 0;

	if (csid_idx >= AIS_IFE_CSID_HW_RES_MAX) {
		CAM_ERR(CAM_ISP, "Invalid csid index:%d", csid_idx);
		return rc;
	}

	csid_hw_info = (struct cam_hw_info  *) csid_hw_intf->hw_priv;
	ife_csid_hw  = (struct ais_ife_csid_ver2_hw  *) csid_hw_info->core_info;

	ife_csid_hw->hw_intf = csid_hw_intf;
	ife_csid_hw->hw_info = csid_hw_info;

	CAM_DBG(CAM_ISP, "type %d index %d",
		ife_csid_hw->hw_intf->hw_type, csid_idx);

	ife_csid_hw->device_enabled = 0;
	ife_csid_hw->hw_info->hw_state = CAM_HW_STATE_POWER_DOWN;
	mutex_init(&ife_csid_hw->hw_info->hw_mutex);
	spin_lock_init(&ife_csid_hw->hw_info->hw_lock);
	spin_lock_init(&ife_csid_hw->lock_state);
	spin_lock_init(&ife_csid_hw->lock_rup);
	init_completion(&ife_csid_hw->hw_info->hw_complete);

	init_completion(&ife_csid_hw->csid_top_complete);
	init_completion(&ife_csid_hw->csid_csi2_complete);
	for (i = 0; i < AIS_IFE_CSID_RDI_MAX; i++)
		init_completion(&ife_csid_hw->csid_rdi_complete[i]);

	rc = ais_ife_csid_init_soc_resources(&ife_csid_hw->hw_info->soc_info,
			ais_ife_csid_irq, ife_csid_hw);
	if (rc < 0) {
		CAM_ERR(CAM_ISP, "CSID:%d Failed to init_soc", csid_idx);
		goto err;
	}

	ife_csid_hw->hw_intf->hw_ops.get_hw_caps = ais_ife_csid_get_hw_caps;
	ife_csid_hw->hw_intf->hw_ops.init        = ais_ife_csid_init_hw;
	ife_csid_hw->hw_intf->hw_ops.deinit      = ais_ife_csid_deinit_hw;
	ife_csid_hw->hw_intf->hw_ops.reset       = ais_ife_csid_force_reset;
	ife_csid_hw->hw_intf->hw_ops.reserve     = ais_ife_csid_reserve;
	ife_csid_hw->hw_intf->hw_ops.release     = ais_ife_csid_release;
	ife_csid_hw->hw_intf->hw_ops.start       = ais_ife_csid_start;
	ife_csid_hw->hw_intf->hw_ops.stop        = ais_ife_csid_stop;
	ife_csid_hw->hw_intf->hw_ops.read        = ais_ife_csid_read;
	ife_csid_hw->hw_intf->hw_ops.write       = ais_ife_csid_write;
	ife_csid_hw->hw_intf->hw_ops.process_cmd = ais_ife_csid_process_cmd;

	/* Initialize the RDI resource */
	for (i = 0; i < ife_csid_hw->csid_info->csid_reg->cmn_reg->num_rdis;
				i++) {
		ife_csid_hw->rdi_cfg[i].state =
				AIS_ISP_RESOURCE_STATE_AVAILABLE;
	}

	ife_csid_hw->csid_debug = 0;
	ife_csid_hw->error_irq_count = 0;

	scnprintf(worker_name, sizeof(worker_name),
		"csid%u_worker", ife_csid_hw->hw_intf->hw_idx);
	CAM_DBG(CAM_ISP, "Create CSID worker %s", worker_name);
	rc = cam_req_mgr_workq_create(worker_name,
		AIS_CSID_WORKQ_NUM_TASK,
		&ife_csid_hw->work, CRM_WORKQ_USAGE_IRQ, 0,
		cam_req_mgr_process_workq_csid_worker);
	if (rc) {
		CAM_ERR(CAM_ISP, "Unable to create a workq, rc=%d", rc);
		goto err_deinit_soc;
	}

	for (i = 0; i < AIS_CSID_WORKQ_NUM_TASK; i++)
		ife_csid_hw->work->task.pool[i].payload =
			&ife_csid_hw->work_data[i];

	/* init debug mask */
	ais_ife_csid_ver2_set_debug(ife_csid_hw, debug_val);

	return 0;

err_deinit_soc:
	ais_ife_csid_deinit_soc_resources(&ife_csid_hw->hw_info->soc_info);
err:
	return rc;
}

int ais_ife_csid_ver2_hw_deinit(struct ais_ife_csid_ver2_hw *ife_csid_hw)
{
	int rc = 0;

	if (ife_csid_hw) {
		ais_ife_csid_deinit_soc_resources(
			&ife_csid_hw->hw_info->soc_info);
		cam_req_mgr_workq_destroy(&ife_csid_hw->work);
	} else {
		CAM_ERR(CAM_ISP, "Invalid param");
		rc = -EINVAL;
	}

	return rc;
}
