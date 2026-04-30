/* Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
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

#ifndef _AIS_TFE665_H_
#define _AIS_TFE665_H_

#include "ais_tfe_camif_ver2.h"
#include "ais_tfe_camif_lite_ver2.h"
#include "ais_tfe_bus_ver2.h"
#include "ais_tfe_top_ver2.h"
#include "ais_tfe_core.h"

static struct ais_irq_register_set tfe665_top_irq_reg_set[3] = {
	{
		.mask_reg_offset   = 0x00001834,
		.clear_reg_offset  = 0x00001840,
		.status_reg_offset = 0x0000184C,
	},
	{
		.mask_reg_offset   = 0x00001838,
		.clear_reg_offset  = 0x00001844,
		.status_reg_offset = 0x00001850,
	},
	{
		.mask_reg_offset   = 0x0000183C,
		.clear_reg_offset  = 0x00001848,
		.status_reg_offset = 0x00001854,
	},
};

static struct ais_irq_controller_reg_info tfe665_top_irq_reg_info = {
	.num_registers = 3,
	.irq_reg_set = tfe665_top_irq_reg_set,
	.global_clear_offset  = 0x00001830,
	.global_clear_bitmask = 0x00000001,
};

static struct ais_vfe_top_ver2_reg_offset_common tfe665_top_common_reg = {
	.hw_version               = 0x00001800,
	.hw_capability            = 0x00001804,
	.lens_feature             = 0x00001808,
	.stats_feature            = 0x0000180C,
	.color_feature            = 0x00000000,
	.zoom_feature             = 0x00001810,
	.global_reset_cmd         = 0x00001814,
	.module_ctrl              = {
		NULL,
		NULL,
		NULL,
		NULL,
	},
	.bus_cgc_ovd              = 0x00000000,
	.core_cfg                 = 0x00001824,
	.three_D_cfg              = 0x00000000,
	.violation_status         = 0x00000000,
	.reg_update_cmd           = 0x0000182C,
};

static struct ais_tfe_rdi_reg  tfe665_rdi_0_reg = {
	.rdi_hw_version              = 0x00001E00,
	.rdi_hw_status               = 0x00001E04,
	.rdi_module_config           = 0x00001E60,
	.rdi_skip_period             = 0x00001E68,
	.rdi_irq_subsample_pattern   = 0x00001E6C,
	.rdi_epoch_irq               = 0x00001E70,
	.rdi_debug_1                 = 0x00001FF0,
	.rdi_debug_0                 = 0x00001FF4,
	.rdi_test_bus_ctrl           = 0x00001FF8,
	.rdi_spare                   = 0x00001FFC,
	.reg_update_cmd              = 0x0000182C,
};

static struct ais_vfe_rdi_reg_data  tfe_665_rdi_0_data = {
	.reg_update_cmd_data      = 0x2,
	.sof_irq_mask             = 0x00000010,
	.reg_update_irq_mask      = 0x20,

	.pixel_pattern_shift      = 24,
	.pixel_pattern_mask       = 0x07000000,
	.rdi_out_enable_shift     = 0,
	.epoch_line_cfg           = 0x00140014,
	.subscribe_irq_mask       = {
		0x00000000,
		0x00000030,
		0x00000000,
	},
};

static struct ais_tfe_rdi_reg  tfe665_rdi_1_reg = {
	.rdi_hw_version              = 0x00002000,
	.rdi_hw_status               = 0x00002004,
	.rdi_module_config           = 0x00002060,
	.rdi_skip_period             = 0x00002068,
	.rdi_irq_subsample_pattern   = 0x0000206C,
	.rdi_epoch_irq               = 0x00002070,
	.rdi_debug_1                 = 0x000021F0,
	.rdi_debug_0                 = 0x000021F4,
	.rdi_test_bus_ctrl           = 0x000021F8,
	.rdi_spare                   = 0x000021FC,
	.reg_update_cmd              = 0x0000182C,
};

static struct ais_vfe_rdi_reg_data  tfe_665_rdi_1_data = {
	.reg_update_cmd_data      = 0x4,
	.sof_irq_mask             = 0x00000100,
	.reg_update_irq_mask      = 0x40,

	.pixel_pattern_shift      = 24,
	.pixel_pattern_mask       = 0x07000000,
	.rdi_out_enable_shift     = 0,
	.epoch_line_cfg           = 0x00140014,
	.subscribe_irq_mask       = {
		0x00000000,
		0x00000300,
		0x00000000,
	},
};

static struct ais_tfe_rdi_reg  tfe665_rdi_2_reg = {
	.rdi_hw_version              = 0x00002200,
	.rdi_hw_status               = 0x00002204,
	.rdi_module_config           = 0x00002260,
	.rdi_skip_period             = 0x00002268,
	.rdi_irq_subsample_pattern   = 0x0000226C,
	.rdi_epoch_irq               = 0x00002270,
	.rdi_debug_1                 = 0x000023F0,
	.rdi_debug_0                 = 0x000023F4,
	.rdi_test_bus_ctrl           = 0x000023F8,
	.rdi_spare                   = 0x000023FC,
	.reg_update_cmd              = 0x0000182C,
};

static struct ais_vfe_rdi_reg_data  tfe_665_rdi_2_data = {
	.reg_update_cmd_data      = 0x8,
	.sof_irq_mask             = 0x00001000,
	.reg_update_irq_mask      = 0x80,

	.pixel_pattern_shift      = 24,
	.pixel_pattern_mask       = 0x07000000,
	.rdi_out_enable_shift     = 0,
	.epoch_line_cfg           = 0x00140014,
	.subscribe_irq_mask       = {
		0x00000000,
		0x00003000,
		0x00000000,
	},
};

static struct ais_tfe_top_ver2_hw_info tfe665_top_hw_info = {
	.common_reg = &tfe665_top_common_reg,
	.camif_hw_info = {
		.common_reg     = &tfe665_top_common_reg,
		.camif_reg      = NULL,
		.reg_data       = NULL,
		},
	.camif_lite_hw_info = {
		.common_reg     = &tfe665_top_common_reg,
		.camif_lite_reg = NULL,
		.reg_data       = NULL,
		},
	.rdi_hw_info = {
		.common_reg = &tfe665_top_common_reg,
		.rdi_reg  = {
			&tfe665_rdi_0_reg,
			&tfe665_rdi_1_reg,
			&tfe665_rdi_2_reg,
			NULL,
		},
		.reg_data = {
			&tfe_665_rdi_0_data,
			&tfe_665_rdi_1_data,
			&tfe_665_rdi_2_data,
			NULL,
		},
	},
	.dump_data = {
		.num_reg_dump_entries  =  0,
		.num_lut_dump_entries  =  0,
		.dmi_cfg               =  0x0,
		.dmi_addr              =  0x0,
		.dmi_data_path_hi      =  0x0,
		.dmi_data_path_lo      =  0x0,
		.reg_entry = {
			{
				.reg_dump_start = 0x0,
				.reg_dump_end   = 0x0,
			},
			{
				.reg_dump_start = 0x0,
				.reg_dump_end   = 0x0,
			},
		},
		.lut_entry = {
			{
				.lut_word_size = 0,
				.lut_bank_sel  = 0x0,
				.lut_addr_size = 0,
			},
		},
	},
	.mux_type = {
		AIS_TFE_CAMIF_VER_2_0,
		AIS_TFE_RDI_VER_1_0,
		AIS_TFE_RDI_VER_1_0,
		AIS_TFE_RDI_VER_1_0,
		AIS_TFE_CAMIF_LITE_VER_2_0,
	},
	.top_irq_reset_mask = {
		0x00000001,
		0x00000000,
		0x00000000,
	},
	.bus_reg_irq_mask = {
		0x00000002,
		0x00000000,
		0x00000000,
	},
	.error_irq_mask = {
		0x001F1F00,
		0x00000000,
		0x000002FF,
	},
	.non_fatal_error_irq_mask = {
		0x00200000,
		0x00000000,
		0x00000000,
	},
	.rdi_path_reset_val = 0x2000100,
	.sof_irq_mask       = 0x0001110,
	.wr_bus_mask        = 0x0000002,
	.rdi_overflow_mask  = 0x00E0000,
	.rdi_overflow_shift = 17,
};

static struct ais_irq_register_set tfe665_bus_irq_reg[2] = {
		{
			.mask_reg_offset   = 0x00003018,
			.clear_reg_offset  = 0x00003020,
			.status_reg_offset = 0x00003028,
		},
		{
			.mask_reg_offset   = 0x0000301C,
			.clear_reg_offset  = 0x00003024,
			.status_reg_offset = 0x0000302C,
		},
};

static struct ais_tfe_bus_ver2_hw_info tfe665_bus_hw_info = {
	.common_reg = {
		.hw_version                   = 0x00003000,
		.hw_capability                = 0x00000000,
		.sw_reset                     = 0x00000000,
		.cgc_ovd                      = 0x00003008,
		.pwr_iso_cfg                  = 0x0000305C,
		.dual_master_comp_cfg         = 0x00000000,
		.irq_reg_info = {
			.num_registers        = 2,
			.irq_reg_set          = tfe665_bus_irq_reg,
			.global_clear_offset  = 0x00003030,
			.global_clear_bitmask = 0x00000001,
		},
		.bus_violation_reg            = 0x00003064,
		.bus_overflow_reg             = 0x00003068,
		.bus_image_size_vilation_reg  = 0x00003070,
		.bus_overflow_clear_cmd       = 0x00003060,
		.comp_error_status            = 0x00000000,
		.comp_ovrwr_status            = 0x00000000,
		.dual_comp_error_status       = 0x00000000,
		.dual_comp_ovrwr_status       = 0x00000000,
		.addr_sync_cfg                = 0x00000000,
		.addr_sync_frame_hdr          = 0x00000000,
		.addr_sync_no_sync            = 0x00000000,
		.addr_fifo_status             = 0x00000000,
		.debug_status_cfg             = 0x00000000,
		.debug_status_0               = 0x00000000,
	},
	.num_client = 3,
	.is_lite = 0,
	.bus_client_reg = {
		/* BUS Client 7 RDI0 */
		{
			.status0                  = 0x00003968,
			.status1                  = 0x0000396C,
			.cfg                      = 0x00003900,
			.header_addr              = 0x00003920,
			.header_cfg               = 0x00003928,
			.image_addr               = 0x00003904,
			.image_addr_offset        = 0x00000000,
			.buffer_width_cfg         = 0x0000390C,
			.buffer_height_cfg        = 0x0000390C,
			.packer_cfg               = 0x00003918,
			.stride                   = 0x00003914,
			.irq_subsample_period     = 0x00003930,
			.irq_subsample_pattern    = 0x00003934,
			.framedrop_period         = 0x00003938,
			.framedrop_pattern        = 0x0000393C,
			.frame_inc                = 0x00003908,
			.burst_limit              = 0x0000391C,
			.ubwc_regs                = NULL,
			.comp_group               = AIS_TFE_BUS_COMP_GRP_5,
		},
		/* BUS Client 8 RDI1 */
		{
			.status0                  = 0x00003A68,
			.status1                  = 0x00003A6C,
			.cfg                      = 0x00003A00,
			.header_addr              = 0x00003A20,
			.header_cfg               = 0x00003A28,
			.image_addr               = 0x00003A04,
			.image_addr_offset        = 0x00000000,
			.buffer_width_cfg         = 0x00003A0C,
			.buffer_height_cfg        = 0x00003A0C,
			.packer_cfg               = 0x00003A18,
			.stride                   = 0x00003A14,
			.irq_subsample_period     = 0x00003A30,
			.irq_subsample_pattern    = 0x00003A34,
			.framedrop_period         = 0x00003A38,
			.framedrop_pattern        = 0x00003A3C,
			.frame_inc                = 0x00003A08,
			.burst_limit              = 0x00003A1C,
			.ubwc_regs                = NULL,
			.comp_group               = AIS_TFE_BUS_COMP_GRP_6,
		},
		/* BUS Client 9 RDI2 */
		{
			.status0                  = 0x00003B68,
			.status1                  = 0x00003B6C,
			.cfg                      = 0x00003B00,
			.header_addr              = 0x00003B20,
			.header_cfg               = 0x00003B28,
			.image_addr               = 0x00003B04,
			.image_addr_offset        = 0x00000000,
			.buffer_width_cfg         = 0x00003B0C,
			.buffer_height_cfg        = 0x00003B0C,
			.packer_cfg               = 0x00003B18,
			.stride                   = 0x00003B14,
			.irq_subsample_period     = 0x00003B30,
			.irq_subsample_pattern    = 0x00003B34,
			.framedrop_period         = 0x00003B38,
			.framedrop_pattern        = 0x00003B3C,
			.frame_inc                = 0x00003B08,
			.burst_limit              = 0x00003B1C,
			.ubwc_regs                = NULL,
			.comp_group               = AIS_TFE_BUS_COMP_GRP_7,
		},
	},
	.num_out = 19,
	.vfe_out_hw_info = {
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_RDI0,
			.max_width     = -1,
			.max_height    = -1,
			.rup_group_id  = AIS_TFE_BUS_RUP_GRP_1,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_RDI1,
			.max_width     = -1,
			.max_height    = -1,
			.rup_group_id  = AIS_TFE_BUS_RUP_GRP_2,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_RDI2,
			.max_width     = -1,
			.max_height    = -1,
			.rup_group_id  = AIS_TFE_BUS_RUP_GRP_3,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_FULL,
			.max_width     = 4096,
			.max_height    = 4096,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_DS4,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_DS16,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_RAW_DUMP,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_FD,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_PDAF,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_HDR_BE,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_HDR_BHIST,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_TL_BG,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_BF,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_AWB_BG,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_BHIST,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_RS,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_CS,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  =
				AIS_TFE_BUS_VER2_VFE_OUT_STATS_IHIST,
			.max_width     = -1,
			.max_height    = -1,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_FULL_DISP,
			.max_width     = 4096,
			.max_height    = 4096,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_DS4_DISP,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_DS16_DISP,
			.max_width     = 1920,
			.max_height    = 1080,
		},
		{
			.vfe_out_type  = AIS_TFE_BUS_VER2_VFE_OUT_2PD,
			.max_width     = 1920,
			.max_height    = 1080,
		},
	},
	.reg_data = {
		.ubwc_10bit_threshold_lossy_0 = 0,
		.ubwc_10bit_threshold_lossy_1 = 0,
		.ubwc_8bit_threshold_lossy_0 = 0,
		.ubwc_8bit_threshold_lossy_1 = 0,
	},
	.stats_data = NULL,
	.top_bus_wr_irq_shift = 1,
	.bus_irq_error_set_mask = {
		0xD0000000,
		0x00000000,
	},
	.bus_error_irq_mask = {
		0xC0000000,
		0x00000000,
	},
	.rdi_width = 128,
	.mode_cfg_shift = 16,
	.height_shift = 16,
	.rdi_client_offset = 7,
	.comp_done_shift = 8,
	.comp_buf_done_mask = 0xFFF00,
	.comp_rup_done_mask = 0xF,
	.max_fifo_num = 2,
};

static struct ais_vfe_hw_info ais_tfe665_hw_info = {
	.irq_reg_info                  = &tfe665_top_irq_reg_info,

	.bus_version                   = AIS_TFE_BUS_VER_1_0,
	.bus_hw_info                   = &tfe665_bus_hw_info,

	.top_version                   = AIS_TFE_TOP_VER_1_0,
	.top_hw_info                   = &tfe665_top_hw_info,
};

#endif /* _AIS_TFE665_H_ */
