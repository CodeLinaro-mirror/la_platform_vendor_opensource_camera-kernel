/* Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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

#ifndef _AIS_VFE_LITE1080_H_
#define _AIS_VFE_LITE1080_H_

#include "ais_vfe_core.h"

static struct ais_irq_register_set vfe1080_top_irq_reg_set[2] = {
	{
		.mask_reg_offset   = 0x00000164,
		.clear_reg_offset  = 0x00000174,
		.status_reg_offset = 0x00000154,
	},
	{
		.mask_reg_offset   = 0x00000168,
		.clear_reg_offset  = 0x00000178,
		.status_reg_offset = 0x00000158,
	},
};

static struct ais_irq_controller_reg_info vfe1080_top_irq_reg_info = {
	.num_registers = 2,
	.irq_reg_set = vfe1080_top_irq_reg_set,
	.global_clear_offset  = 0x00000188,
	.global_clear_bitmask = 0x00000001,
};

static struct ais_vfe_top_ver2_reg_offset_common vfe1080_top_common_reg = {
	.hw_version               = 0x00000000,
	.hw_capability            = 0x0,
	.lens_feature             = 0x00000000,
	.stats_feature            = 0x00000000,
	.color_feature            = 0x00000000,
	.zoom_feature             = 0x00000000,
	.global_reset_cmd         = 0x0000188,
	.module_ctrl              = {
		NULL,
		NULL,
		NULL,
		NULL,
	},
	.bus_cgc_ovd              = 0x00000000,
	.core_cfg                 = 0x00000000,
	.three_D_cfg              = 0x00000000,
	.violation_status         = 0x00009064,
	.reg_update_cmd           = 0x00000000,
};

static struct ais_vfe_top_ver2_hw_info vfe1080_top_hw_info = {
	.common_reg = &vfe1080_top_common_reg,
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

static struct ais_irq_register_set vfe1080_bus_irq_reg[1] = {
		{
			.mask_reg_offset   = 0x00009818,
			.clear_reg_offset  = 0x00009820,
			.status_reg_offset = 0x00009828,
		},
};

static uint32_t vfe1080_bus_mask_val[1] = {0xD0000000};

static struct ais_vfe_bus_ver2_hw_info vfe1080_bus_hw_info = {
	.common_reg = {
		.hw_version                   = 0x00009800,
		.hw_capability                = 0x00000000,
		.sw_reset                     = 0x00009804,
		.cgc_ovd                      = 0x00009808,
		.pwr_iso_cfg                  = 0x0000985C,
		.dual_master_comp_cfg         = 0x00000000,
		.irq_reg_info = {
			.num_registers        = 1,
			.irq_reg_set          = vfe1080_bus_irq_reg,
			.global_clear_offset  = 0x00009830,
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
		.buf_done_shitf_val           = 16,
		.bus_mask_val                 = vfe1080_bus_mask_val,
	},
	.num_client = 4,
	.is_lite = 1,
	.bus_client_reg = {
		/* BUS Client 0 */
		{
			.status0                  = 0x00009D90,
			.status1                  = 0x00009D94,
			.cfg                      = 0x00009D00,
			.header_addr              = 0x00009D20,
			.header_cfg               = 0x00009D20,
			.image_addr               = 0x00009D04,
			.image_addr_offset        = 0x00009D70,
			.buffer_width_cfg         = 0x00009D0C,
			.buffer_height_cfg        = 0x00009D0C,
			.packer_cfg               = 0x00009D18,
			.stride                   = 0x00009D14,
			.irq_subsample_period     = 0x00009D30,
			.irq_subsample_pattern    = 0x00009D34,
			.framedrop_period         = 0x00000000,
			.framedrop_pattern        = 0x00000000,
			.frame_inc                = 0x00009D08,
			.burst_limit              = 0x00009D1C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 1 */
		{
			.status0                  = 0x00009E90,
			.status1                  = 0x00009E94,
			.cfg                      = 0x00009E00,
			.header_addr              = 0x00009E20,
			.header_cfg               = 0x00009E20,
			.image_addr               = 0x00009E04,
			.image_addr_offset        = 0x00009E70,
			.buffer_width_cfg         = 0x00009E0C,
			.buffer_height_cfg        = 0x00009E0C,
			.packer_cfg               = 0x00009E18,
			.stride                   = 0x00009E14,
			.irq_subsample_period     = 0x00009E30,
			.irq_subsample_pattern    = 0x00009E34,
			.framedrop_period         = 0x00000000,
			.framedrop_pattern        = 0x00000000,
			.frame_inc                = 0x00009E08,
			.burst_limit              = 0x00009E1C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 2 */
		{
			.status0                  = 0x00009F90,
			.status1                  = 0x00009F94,
			.cfg                      = 0x00009F00,
			.header_addr              = 0x00009F20,
			.header_cfg               = 0x00009F20,
			.image_addr               = 0x00009F04,
			.image_addr_offset        = 0x00009F70,
			.buffer_width_cfg         = 0x00009F0C,
			.buffer_height_cfg        = 0x00009F0C,
			.packer_cfg               = 0x00009F18,
			.stride                   = 0x00009F14,
			.irq_subsample_period     = 0x00009F30,
			.irq_subsample_pattern    = 0x00009F34,
			.framedrop_period         = 0x00000000,
			.framedrop_pattern        = 0x00000000,
			.frame_inc                = 0x00009F08,
			.burst_limit              = 0x00009F1C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 3 */
		{
			.status0                  = 0x0000A090,
			.status1                  = 0x0000A094,
			.cfg                      = 0x0000A000,
			.header_addr              = 0x0000A020,
			.header_cfg               = 0x0000A020,
			.image_addr               = 0x0000A004,
			.image_addr_offset        = 0x0000A070,
			.buffer_width_cfg         = 0x0000A00C,
			.buffer_height_cfg        = 0x0000A00C,
			.packer_cfg               = 0x0000A018,
			.stride                   = 0x0000A014,
			.irq_subsample_period     = 0x0000A030,
			.irq_subsample_pattern    = 0x0000A034,
			.framedrop_period         = 0x00000000,
			.framedrop_pattern        = 0x00000000,
			.frame_inc                = 0x0000A008,
			.burst_limit              = 0x0000A01C,
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

static struct ais_vfe_hw_info ais_vfe_lite1080_hw_info = {
	.irq_reg_info                  = &vfe1080_top_irq_reg_info,

	.bus_version                   = AIS_VFE_BUS_VER_3_0,
	.bus_hw_info                   = &vfe1080_bus_hw_info,

	.top_version                   = AIS_VFE_TOP_VER_4_0,
	.top_hw_info                   = &vfe1080_top_hw_info,
};

#endif /* _AIS_VFE_LITE1080_H_ */
