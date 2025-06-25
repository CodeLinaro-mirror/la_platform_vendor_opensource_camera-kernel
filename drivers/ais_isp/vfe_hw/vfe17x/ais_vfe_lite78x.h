/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */


#ifndef _CAM_VFE_LITE78X_H_
#define _CAM_VFE_LITE78X_H_

#include "ais_vfe_camif_ver2.h"
#include "ais_vfe_camif_lite_ver2.h"
#include "ais_vfe_bus_ver2.h"
#include "ais_vfe_top_ver2.h"
#include "ais_vfe_core.h"

static struct ais_irq_register_set vfe78x_top_irq_reg_set[2] = {
	{
		.mask_reg_offset   = 0x00001024,
		.clear_reg_offset  = 0x0000102C,
		.status_reg_offset = 0x0000101C,
	},
	{
		.mask_reg_offset   = 0x00001028,
		.clear_reg_offset  = 0x00001030,
		.status_reg_offset = 0x00001020,
	},
};

static struct ais_irq_controller_reg_info vfe78x_top_irq_reg_info = {
	.num_registers = 2,
	.irq_reg_set = vfe78x_top_irq_reg_set,
	.global_clear_offset  = 0x00001038,
	.global_clear_bitmask = 0x00000001,
};

static struct ais_irq_register_set vfe78x_bus_irq_reg[3] = {
		{
			.mask_reg_offset   = 0x00001218,
			.clear_reg_offset  = 0x00001220,
			.status_reg_offset = 0x00001228,
		},
};

static struct ais_vfe_bus_ver2_hw_info vfe78x_bus_hw_info = {
	.common_reg = {
		.hw_version                   = 0x00001200,
		.hw_capability                = 0x00000004,
		.sw_reset                     = 0x00001204,
		.cgc_ovd                      = 0x00001208,
		.pwr_iso_cfg                  = 0x0000125C,
		.dual_master_comp_cfg         = 0x00000000,
		.irq_reg_info = {
			.num_registers        = 1,
			.irq_reg_set          = vfe78x_bus_irq_reg,
			.global_clear_offset  = 0x00001230,
			.global_clear_bitmask = 0x00000001,
		},
		.comp_error_status            = 0x00000000,
		.comp_ovrwr_status            = 0x00000000,
		.dual_comp_error_status       = 0x00000000,
		.dual_comp_ovrwr_status       = 0x00000000,
		.addr_sync_cfg                = 0x00000000,
		.addr_sync_frame_hdr          = 0x00000000,
		.addr_sync_no_sync            = 0x00000000,
		.addr_fifo_status             = 0x00000000,
		.buf_done_shitf_val           = 14,
		.buf_done_rdi_mask            = 0x1C000,
	},
	.num_rid = 4,
	.num_client = 4,
	.is_lite = 1,
	.bus_client_reg = {
		/* BUS Client 0 */
		{
			.status0                  = 0x00001474,
			.status1                  = 0x00001478,
			.cfg                      = 0x00001400,
			.header_addr              = 0x00001420,
			.header_cfg               = 0x00001428,
			.image_addr               = 0x00001404,
			.image_addr_offset        = 0x00001408,
			.buffer_width_cfg         = 0x0000140C,
			.buffer_height_cfg        = 0x0000140C,
			.packer_cfg               = 0x00001418,
			.stride                   = 0x00001414,
			.irq_subsample_period     = 0x00001430,
			.irq_subsample_pattern    = 0x00001434,
			.framedrop_period         = 0x00001438,
			.framedrop_pattern        = 0x0000143C,
			.frame_inc                = 0x00001408,
			.burst_limit              = 0x0000141C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 1 */
		{
			.status0                  = 0x00001574,
			.status1                  = 0x00001578,
			.cfg                      = 0x00001500,
			.header_addr              = 0x00001520,
			.header_cfg               = 0x00001528,
			.image_addr               = 0x00001504,
			.image_addr_offset        = 0x00001508,
			.buffer_width_cfg         = 0x0000150C,
			.buffer_height_cfg        = 0x0000150C,
			.packer_cfg               = 0x00001518,
			.stride                   = 0x00001514,
			.irq_subsample_period     = 0x00001530,
			.irq_subsample_pattern    = 0x00001534,
			.framedrop_period         = 0x00001538,
			.framedrop_pattern        = 0x0000153C,
			.frame_inc                = 0x00001508,
			.burst_limit              = 0x0000151C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 2 */
		{
			.status0                  = 0x00001674,
			.status1                  = 0x00001678,
			.cfg                      = 0x00001600,
			.header_addr              = 0x00001620,
			.header_cfg               = 0x00001628,
			.image_addr               = 0x00001604,
			.image_addr_offset        = 0x00001608,
			.buffer_width_cfg         = 0x0000160C,
			.buffer_height_cfg        = 0x0000160C,
			.packer_cfg               = 0x00001618,
			.stride                   = 0x00001614,
			.irq_subsample_period     = 0x00001630,
			.irq_subsample_pattern    = 0x00001634,
			.framedrop_period         = 0x00001638,
			.framedrop_pattern        = 0x0000163C,
			.frame_inc                = 0x00001608,
			.burst_limit              = 0x0000161C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 3 */
		{
			.status0                  = 0x00001774,
			.status1                  = 0x00001778,
			.cfg                      = 0x00001700,
			.header_addr              = 0x00001720,
			.header_cfg               = 0x00001728,
			.image_addr               = 0x00001704,
			.image_addr_offset        = 0x00001708,
			.buffer_width_cfg         = 0x0000170C,
			.buffer_height_cfg        = 0x0000170C,
			.packer_cfg               = 0x00001718,
			.stride                   = 0x00001714,
			.irq_subsample_period     = 0x00001730,
			.irq_subsample_pattern    = 0x00001734,
			.framedrop_period         = 0x00001738,
			.framedrop_pattern        = 0x0000173C,
			.frame_inc                = 0x00001708,
			.burst_limit              = 0x0000171C,
			.ubwc_regs                = NULL,
		},
	},
	.comp_grp_reg = {
		/* AIS_VFE_BUS_VER2_COMP_GRP_0 */
		{
			.comp_mask                    = 0x00002010,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_1 */
		{
			.comp_mask                    = 0x00002014,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_2 */
		{
			.comp_mask                    = 0x00002018,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_3 */
		{
			.comp_mask                    = 0x0000201C,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_4 */
		{
			.comp_mask                    = 0x00002020,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_5 */
		{
			.comp_mask                    = 0x00002024,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_DUAL_0 */
		{
			.comp_mask                    = 0x0000202C,
			.addr_sync_mask               = 0x00002088,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_DUAL_1 */
		{
			.comp_mask                    = 0x00002030,
			.addr_sync_mask               = 0x0000208C,

		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_DUAL_2 */
		{
			.comp_mask                    = 0x00002034,
			.addr_sync_mask               = 0x00002090,

		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_DUAL_3 */
		{
			.comp_mask                    = 0x00002038,
			.addr_sync_mask               = 0x00002094,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_DUAL_4 */
		{
			.comp_mask                    = 0x0000203C,
			.addr_sync_mask               = 0x00002098,
		},
		/* AIS_VFE_BUS_VER2_COMP_GRP_DUAL_5 */
		{
			.comp_mask                    = 0x00002040,
			.addr_sync_mask               = 0x0000209C,
		},
	},
	.num_out = 4,
	.vfe_out_hw_info = {
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_RDI0,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_RDI1,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_RDI2,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_RDI3,
			.max_width     = -1,
			.max_height    = -1,
		},
	},
};

static struct ais_vfe_top_ver2_reg_offset_common vfe78x_top_common_reg = {
	.hw_version               = 0x00001000,
	.hw_capability            = 0x00001004,
	.lens_feature             = 0x00000000,
	.stats_feature            = 0x00000000,
	.color_feature            = 0x00000000,
	.zoom_feature             = 0x00000000,
	.global_reset_cmd         = 0x00001030,
	.module_ctrl              = {
		NULL,
		NULL,
		NULL,
		NULL,
	},
	.bus_cgc_ovd              = 0x00000000,
	.core_cfg                 = 0x00000000,
	.three_D_cfg              = 0x00000000,
	.violation_status         = 0x00001264,
	.reg_update_cmd           = 0x00000000,
};

static struct ais_vfe_top_ver2_hw_info vfe78x_top_hw_info = {
	.common_reg = &vfe78x_top_common_reg,
	.camif_hw_info = {
		.common_reg = NULL,
		.camif_reg  = NULL,
		.reg_data   = NULL,
		},
	.rdi_hw_info = {
		.common_reg = NULL,
		.rdi_reg    = NULL,
		.reg_data = {
			NULL,
			NULL,
			NULL,
			NULL,
			},
		},
	.mux_type = {
		AIS_VFE_RDI_VER_1_0,
		AIS_VFE_RDI_VER_1_0,
		AIS_VFE_RDI_VER_1_0,
		AIS_VFE_RDI_VER_1_0,
	},
};

static struct ais_vfe_hw_info ais_vfe_lite78x_hw_info = {
	.irq_reg_info                  = &vfe78x_top_irq_reg_info,

	.bus_version                   = AIS_VFE_BUS_VER_3_0,
	.bus_hw_info                   = &vfe78x_bus_hw_info,

	.top_version                   = AIS_VFE_TOP_VER_4_0,
	.top_hw_info                   = &vfe78x_top_hw_info,

};
#endif /* _CAM_VFE_LITE78X_H_ */
