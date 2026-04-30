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

#ifndef _AIS_TFE1080_H_
#define _AIS_TFE1080_H_

#include "ais_vfe_camif_ver2.h"
#include "ais_vfe_camif_lite_ver2.h"
#include "ais_vfe_bus_ver2.h"
#include "ais_vfe_top_ver2.h"
#include "ais_vfe_core.h"

static struct ais_irq_register_set tfe1080_top_irq_reg_set[1] = {
	{
		.mask_reg_offset   = 0x000001D4,
		.clear_reg_offset  = 0x000001D8,
		.status_reg_offset = 0x000001DC,
	},
};

static struct ais_irq_controller_reg_info tfe1080_top_irq_reg_info = {
	.num_registers = 1,
	.irq_reg_set = tfe1080_top_irq_reg_set,
	.global_clear_offset  = 0x0000001D0,
	.global_clear_bitmask = 0x00000001,
};

static struct ais_vfe_top_ver2_reg_offset_common tfe1080_top_common_reg = {
	.hw_version               = 0x00000000,
	.hw_capability            = 0x00000004,
	.lens_feature             = 0x00000008,
	.stats_feature            = 0x0000000C,
	.color_feature            = 0x00000010,
	.zoom_feature             = 0x00000014,
	.global_reset_cmd         = 0x000001D0,
	.module_ctrl              = {
		NULL,
	},
	.bus_cgc_ovd              = 0x0000003C,
	.core_cfg                 = 0x00000050,
	.three_D_cfg              = 0x00000054,
	.violation_status         = 0x0000007C,
	.reg_update_cmd           = 0x000004AC,
};

static struct ais_vfe_top_ver2_hw_info tfe1080_top_hw_info = {
	.common_reg = &tfe1080_top_common_reg,
	.camif_hw_info = {
		.common_reg     = NULL,
		.camif_reg      = NULL,
		.reg_data       = NULL,
		},
	.camif_lite_hw_info = {
		.common_reg     = NULL,
		.camif_lite_reg = NULL,
		.reg_data       = NULL,
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
	.dump_data = {
		.num_reg_dump_entries  =  0,
		.num_lut_dump_entries  =  0,
		.dmi_cfg               =  0x000,
		.dmi_addr              =  0x000,
		.dmi_data_path_hi      =  0x000,
		.dmi_data_path_lo      =  0x000,
		.reg_entry = {
			{
				.reg_dump_start = 0x0,
				.reg_dump_end   = 0x0000,
			},
			{
				.reg_dump_start = 0x0000,
				.reg_dump_end   = 0x0000,
			},
		},
		.lut_entry = {
			{
				.lut_word_size = 0,
				.lut_bank_sel  = 0x00,
				.lut_addr_size = 0,
			},
		},
	},
	.mux_type = {
		AIS_VFE_CAMIF_VER_2_0,
		AIS_VFE_RDI_VER_1_0,
		AIS_VFE_RDI_VER_1_0,
		AIS_VFE_RDI_VER_1_0,
		AIS_VFE_CAMIF_LITE_VER_2_0,
	},
};

static struct ais_irq_register_set tfe1080_bus_irq_reg[2] = {
		{
			.mask_reg_offset   = 0x00001018,
			.clear_reg_offset  = 0x00001020,
			.status_reg_offset = 0x00001038,
		},
		{
			.mask_reg_offset   = 0x0000101C,
			.clear_reg_offset  = 0x00001024,
			.status_reg_offset = 0x0000102C,
		},
};

static uint32_t tfe1080_bus_mask_val[2] = {0xD0000000, 0};

static struct ais_vfe_bus_ver2_hw_info tfe1080_bus_hw_info = {
	.common_reg = {
		.hw_version                   = 0x00001000,
		.hw_capability                = 0x00000000,
		.sw_reset                     = 0x00001004,
		.cgc_ovd                      = 0x00001008,
		.pwr_iso_cfg                  = 0x0000105C,
		.dual_master_comp_cfg         = 0x00000000,
		.irq_reg_info = {
			.num_registers        = 2,
			.irq_reg_set          = tfe1080_bus_irq_reg,
			.global_clear_offset  = 0x00001030,
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
		.debug_status_cfg             = 0x000010F0,
		.debug_status_0               = 0x000010F4,
		.buf_done_shitf_val           = 16,
		.bus_mask_val                 = tfe1080_bus_mask_val,
	},
	.num_client = 3,
	.is_lite = 0,
	.bus_client_reg = {
		/* BUS Client 0 */
		{
			.status0                  = 0x00002C90,
			.status1                  = 0x00002C94,
			.cfg                      = 0x00002C00,
			.header_addr              = 0x00002C20,
			.header_cfg               = 0x00002C28,
			.image_addr               = 0x00002C04,
			.image_addr_offset        = 0x00002C70,
			.buffer_width_cfg         = 0x00002C0C,
			.buffer_height_cfg        = 0x00002C0C,
			.packer_cfg               = 0x00002C18,
			.stride                   = 0x00002C14,
			.irq_subsample_period     = 0x00002C30,
			.irq_subsample_pattern    = 0x00002C34,
			.framedrop_period         = 0x00002C38,
			.framedrop_pattern        = 0x00002C3C,
			.frame_inc                = 0x00002C08,
			.burst_limit              = 0x00002C1C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 1 */
		{
			.status0                  = 0x00002D90,
			.status1                  = 0x00002D94,
			.cfg                      = 0x00002D00,
			.header_addr              = 0x00002D20,
			.header_cfg               = 0x00002D28,
			.image_addr               = 0x00002D04,
			.image_addr_offset        = 0x00002D70,
			.buffer_width_cfg         = 0x00002D0C,
			.buffer_height_cfg        = 0x00002D0C,
			.packer_cfg               = 0x00002D18,
			.stride                   = 0x00002D14,
			.irq_subsample_period     = 0x00002D30,
			.irq_subsample_pattern    = 0x00002D34,
			.framedrop_period         = 0x00002D38,
			.framedrop_pattern        = 0x00002D3C,
			.frame_inc                = 0x00002D08,
			.burst_limit              = 0x00002D1C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 2 */
		{
			.status0                  = 0x00002E90,
			.status1                  = 0x00002E94,
			.cfg                      = 0x00002E00,
			.header_addr              = 0x00002E20,
			.header_cfg               = 0x00002E28,
			.image_addr               = 0x00002E04,
			.image_addr_offset        = 0x00002E70,
			.buffer_width_cfg         = 0x00002E0C,
			.buffer_height_cfg        = 0x00002E0C,
			.packer_cfg               = 0x00002E18,
			.stride                   = 0x00002E14,
			.irq_subsample_period     = 0x00002E30,
			.irq_subsample_pattern    = 0x00002E34,
			.framedrop_period         = 0x00002E38,
			.framedrop_pattern        = 0x00002E3C,
			.frame_inc                = 0x00002E08,
			.burst_limit              = 0x00002E1C,
			.ubwc_regs                = NULL,
		},
		/* BUS Client 3 */
		{
			.status0                  = 0x00002F90,
			.status1                  = 0x00002F94,
			.cfg                      = 0x00002F00,
			.header_addr              = 0x00002F20,
			.header_cfg               = 0x00002F28,
			.image_addr               = 0x00002F04,
			.image_addr_offset        = 0x00002F70,
			.buffer_width_cfg         = 0x00002F0C,
			.buffer_height_cfg        = 0x00002F0C,
			.packer_cfg               = 0x00002F18,
			.stride                   = 0x00002F14,
			.irq_subsample_period     = 0x00002F30,
			.irq_subsample_pattern    = 0x00002F34,
			.framedrop_period         = 0x00002F38,
			.framedrop_pattern        = 0x00002F3C,
			.frame_inc                = 0x00002F08,
			.burst_limit              = 0x00002F1C,
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
	.num_out = 0,
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
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_FULL,
			.max_width     = 4096,
			.max_height    = 4096,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_DS4,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_DS16,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_RAW_DUMP,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_FD,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_PDAF,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_HDR_BE,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_HDR_BHIST,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_TL_BG,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_BF,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_AWB_BG,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_BHIST,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_RS,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_CS,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_VFE_BUS_VER2_VFE_OUT_STATS_IHIST,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_FULL_DISP,
			.max_width     = 4096,
			.max_height    = 4096,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_DS4_DISP,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_DS16_DISP,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_VFE_BUS_VER2_VFE_OUT_2PD,
			.max_width     = 1920,
			.max_height    = 1080,
		},
	},
	.reg_data = {
		.ubwc_10bit_threshold_lossy_0 = 0x8330002,
		.ubwc_10bit_threshold_lossy_1 = 0x20204,
		.ubwc_8bit_threshold_lossy_0 = 0x6210022,
		.ubwc_8bit_threshold_lossy_1 = 0xE0E,
	},
	.stats_data = NULL,
};

static struct ais_vfe_hw_info ais_tfe1080_hw_info = {
	.irq_reg_info                  = &tfe1080_top_irq_reg_info,

	.bus_version                   = AIS_VFE_BUS_VER_3_0,
	.bus_hw_info                   = &tfe1080_bus_hw_info,

	.top_version                   = AIS_VFE_TOP_VER_4_0,
	.top_hw_info                   = &tfe1080_top_hw_info,
};

#endif /* _AIS_TFE1080_H_ */
