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

#ifndef _AIS_IFE_CSID_LITE_COMMON_REG_V1_H_
#define _AIS_IFE_CSID_LITE_COMMON_REG_V1_H_

#include "ais_ife_csid_core_ver2.h"

static struct ais_ife_csid_ver2_rx_debug_mask
	ais_ife_csid_lite_common_reg_v1_rx_debug_mask = {
	.evt_bitmap = {
		BIT_ULL(AIS_IFE_CSID_RX_DL0_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL1_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL2_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL3_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL0_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL1_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL2_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL3_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_CPHY_EOT_RECEPTION) |
			BIT_ULL(AIS_IFE_CSID_RX_CPHY_SOT_RECEPTION) |
			BIT_ULL(AIS_IFE_CSID_RX_ERROR_CPHY_PH_CRC) |
			BIT_ULL(AIS_IFE_CSID_RX_WARNING_ECC) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE0_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE1_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE2_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE3_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_ERROR_CRC) |
			BIT_ULL(AIS_IFE_CSID_RX_ERROR_ECC) |
			BIT_ULL(AIS_IFE_CSID_RX_MMAPPED_VC_DT) |
			BIT_ULL(AIS_IFE_CSID_RX_UNMAPPED_VC_DT) |
			BIT_ULL(AIS_IFE_CSID_RX_STREAM_UNDERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_UNBOUNDED_FRAME),

		BIT_ULL(AIS_IFE_CSID_RX_LONG_PKT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED),
	},

	.bit_pos[AIS_IFE_CSID_RX_DL0_EOT_CAPTURED] = 0,
	.bit_pos[AIS_IFE_CSID_RX_DL1_EOT_CAPTURED] = 1,
	.bit_pos[AIS_IFE_CSID_RX_DL2_EOT_CAPTURED] = 2,
	.bit_pos[AIS_IFE_CSID_RX_DL3_EOT_CAPTURED] = 3,
	.bit_pos[AIS_IFE_CSID_RX_DL0_SOT_CAPTURED] = 4,
	.bit_pos[AIS_IFE_CSID_RX_DL1_SOT_CAPTURED] = 5,
	.bit_pos[AIS_IFE_CSID_RX_DL2_SOT_CAPTURED] = 6,
	.bit_pos[AIS_IFE_CSID_RX_DL3_SOT_CAPTURED] = 7,
	.bit_pos[AIS_IFE_CSID_RX_LONG_PKT_CAPTURED] = 0,
	.bit_pos[AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED] = 1,
	.bit_pos[AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED] = 2,
	.bit_pos[AIS_IFE_CSID_RX_CPHY_EOT_RECEPTION] = 11,
	.bit_pos[AIS_IFE_CSID_RX_CPHY_SOT_RECEPTION] = 12,
	.bit_pos[AIS_IFE_CSID_RX_ERROR_CPHY_PH_CRC] = 13,
	.bit_pos[AIS_IFE_CSID_RX_WARNING_ECC] = 14,
	.bit_pos[AIS_IFE_CSID_RX_LANE0_FIFO_OVERFLOW] = 15,
	.bit_pos[AIS_IFE_CSID_RX_LANE1_FIFO_OVERFLOW] = 16,
	.bit_pos[AIS_IFE_CSID_RX_LANE2_FIFO_OVERFLOW] = 17,
	.bit_pos[AIS_IFE_CSID_RX_LANE3_FIFO_OVERFLOW] = 18,
	.bit_pos[AIS_IFE_CSID_RX_ERROR_CRC] = 19,
	.bit_pos[AIS_IFE_CSID_RX_ERROR_ECC] = 20,
	.bit_pos[AIS_IFE_CSID_RX_MMAPPED_VC_DT] = 21,
	.bit_pos[AIS_IFE_CSID_RX_UNMAPPED_VC_DT] = 22,
	.bit_pos[AIS_IFE_CSID_RX_STREAM_UNDERFLOW] = 23,
	.bit_pos[AIS_IFE_CSID_RX_UNBOUNDED_FRAME] = 24,
};

static struct ais_ife_csid_ver2_top_debug_mask
	ais_ife_csid_ver2_lite_common_reg_v1_top_debug_mask = {
	.evt_bitmap = {
		BIT_ULL(AIS_IFE_CSID_TOP_INFO_VOTE_UP) |
		BIT_ULL(AIS_IFE_CSID_TOP_INFO_VOTE_DN) |
		BIT_ULL(AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN),
	},
	.bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_UP] = 16,
	.bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_DN] = 17,
	.bit_pos[AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN] = 18,
};

static struct ais_ife_csid_ver2_path_debug_mask
	ais_ife_csid_ver2_lite_common_reg_v1_path_debug_mask = {
	.bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_EOF] = 9,
	.bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_SOF] = 12,
};

static struct ais_ife_csid_ver2_rdi_reg_offset
	ais_ife_csid_lite_common_reg_v1_rdi_0_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x0224,
	.csid_rdi_irq_mask_addr                   = 0x0228,
	.csid_rdi_irq_clear_addr                  = 0x022C,
	.csid_rdi_irq_set_addr                    = 0x0230,
	.csid_rdi_cfg0_addr                       = 0x3080,
	.csid_rdi_cfg1_addr                       = 0x3094,
	.csid_rdi_ctrl_addr                       = 0x3088,
	.csid_rdi_epoch_irq_cfg_addr              = 0x30B4,
	.csid_rdi_frm_drop_pattern_addr           = 0x30CC,
	.csid_rdi_frm_drop_period_addr            = 0x30D0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x30D4,
	.csid_rdi_irq_subsample_period_addr       = 0x30D8,
	.csid_rdi_rpp_hcrop_addr                  = 0x30DC,
	.csid_rdi_rpp_vcrop_addr                  = 0x30E0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x30E4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x30E8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x30EC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x30F0,
	.csid_rdi_rst_strobes_addr                = 0x3000,
	.csid_rdi_status_addr                     = 0x3000,
	.csid_rdi_misr_val0_addr                  = 0x30FC,
	.csid_rdi_misr_val1_addr                  = 0x3100,
	.csid_rdi_misr_val2_addr                  = 0x3104,
	.csid_rdi_misr_val3_addr                  = 0x3108,
	.csid_rdi_format_measure_cfg0_addr        = 0x310C,
	.csid_rdi_format_measure_cfg1_addr        = 0x3110,
	.csid_rdi_format_measure_cfg2_addr        = 0x3114,
	.csid_rdi_format_measure0_addr            = 0x3118,
	.csid_rdi_format_measure1_addr            = 0x311C,
	.csid_rdi_format_measure2_addr            = 0x3120,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x3124,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x3128,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x312C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x3130,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x3134,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x3138,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x313C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x3140,
	.csid_rdi_byte_cntr_ping_addr             = 0x31A8,
	.csid_rdi_byte_cntr_pong_addr             = 0x31AC,

	.epoch0_shift_val                         = 16,
	.timestamp_en_shift_val                   = 6,

	.top_irq_mask                             = {0x10000,},

	.fatal_err_mask                           = 0x2c1c6081,
	.non_fatal_err_mask                       = 0x12000000,

	.rup_mask                                     = 0x100,
	.aup_mask                                     = 0x100,
	.rup_aup_set_mask                             = 0x1,
};

static struct ais_ife_csid_ver2_rdi_reg_offset
	ais_ife_csid_lite_common_reg_v1_rdi_1_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x0234,
	.csid_rdi_irq_mask_addr                   = 0x0238,
	.csid_rdi_irq_clear_addr                  = 0x023C,
	.csid_rdi_irq_set_addr                    = 0x0240,
	.csid_rdi_cfg0_addr                       = 0x3280,
	.csid_rdi_cfg1_addr                       = 0x3294,
	.csid_rdi_ctrl_addr                       = 0x3288,
	.csid_rdi_epoch_irq_cfg_addr              = 0x32B4,
	.csid_rdi_frm_drop_pattern_addr           = 0x32CC,
	.csid_rdi_frm_drop_period_addr            = 0x32D0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x32D4,
	.csid_rdi_irq_subsample_period_addr       = 0x32D8,
	.csid_rdi_rpp_hcrop_addr                  = 0x32DC,
	.csid_rdi_rpp_vcrop_addr                  = 0x32E0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x32E4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x32E8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x32EC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x32F0,
	.csid_rdi_rst_strobes_addr                = 0x0000,
	.csid_rdi_status_addr                     = 0x0000,
	.csid_rdi_misr_val0_addr                  = 0x33FC,
	.csid_rdi_misr_val1_addr                  = 0x3300,
	.csid_rdi_misr_val2_addr                  = 0x3304,
	.csid_rdi_misr_val3_addr                  = 0x3308,
	.csid_rdi_format_measure_cfg0_addr        = 0x330C,
	.csid_rdi_format_measure_cfg1_addr        = 0x3310,
	.csid_rdi_format_measure_cfg2_addr        = 0x3314,
	.csid_rdi_format_measure0_addr            = 0x3318,
	.csid_rdi_format_measure1_addr            = 0x331C,
	.csid_rdi_format_measure2_addr            = 0x3320,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x3324,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x3328,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x332C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x3330,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x3334,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x3338,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x333C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x3340,
	.csid_rdi_byte_cntr_ping_addr             = 0x33A8,
	.csid_rdi_byte_cntr_pong_addr             = 0x33AC,

	.epoch0_shift_val                         = 16,
	.timestamp_en_shift_val                   = 6,

	.top_irq_mask                             = {0x20000,},

	.fatal_err_mask                           = 0x2c1c6081,
	.non_fatal_err_mask                       = 0x12000000,

	.rup_mask                                     = 0x200,
	.aup_mask                                     = 0x200,
	.rup_aup_set_mask                             = 0x1,
};

static struct ais_ife_csid_ver2_rdi_reg_offset
	ais_ife_csid_lite_common_reg_v1_rdi_2_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x244,
	.csid_rdi_irq_mask_addr                   = 0x248,
	.csid_rdi_irq_clear_addr                  = 0x24C,
	.csid_rdi_irq_set_addr                    = 0x250,
	.csid_rdi_cfg0_addr                       = 0x3480,
	.csid_rdi_cfg1_addr                       = 0x3494,
	.csid_rdi_ctrl_addr                       = 0x3488,
	.csid_rdi_epoch_irq_cfg_addr              = 0x34B4,
	.csid_rdi_frm_drop_pattern_addr           = 0x34CC,
	.csid_rdi_frm_drop_period_addr            = 0x34D0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x34D4,
	.csid_rdi_irq_subsample_period_addr       = 0x34D8,
	.csid_rdi_rpp_hcrop_addr                  = 0x34DC,
	.csid_rdi_rpp_vcrop_addr                  = 0x34E0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x34E4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x34E8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x34EC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x34F0,
	.csid_rdi_rst_strobes_addr                = 0x0000,
	.csid_rdi_status_addr                     = 0x0000,
	.csid_rdi_misr_val0_addr                  = 0x35FC,
	.csid_rdi_misr_val1_addr                  = 0x3500,
	.csid_rdi_misr_val2_addr                  = 0x3504,
	.csid_rdi_misr_val3_addr                  = 0x3508,
	.csid_rdi_format_measure_cfg0_addr        = 0x350C,
	.csid_rdi_format_measure_cfg1_addr        = 0x3510,
	.csid_rdi_format_measure_cfg2_addr        = 0x3514,
	.csid_rdi_format_measure0_addr            = 0x3518,
	.csid_rdi_format_measure1_addr            = 0x351C,
	.csid_rdi_format_measure2_addr            = 0x3520,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x3524,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x3528,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x352C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x3530,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x3534,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x3538,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x353C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x3540,
	.csid_rdi_byte_cntr_ping_addr             = 0x35A8,
	.csid_rdi_byte_cntr_pong_addr             = 0x35AC,

	.epoch0_shift_val                         = 16,
	.timestamp_en_shift_val                   = 6,

	.top_irq_mask                             = {0x40000,},

	.fatal_err_mask                           = 0x2c1c6081,
	.non_fatal_err_mask                       = 0x12000000,

	.rup_mask                                     = 0x400,
	.aup_mask                                     = 0x400,
	.rup_aup_set_mask                             = 0x1,
};

static struct ais_ife_csid_ver2_rdi_reg_offset
	ais_ife_csid_lite_common_reg_v1_rdi_3_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x254,
	.csid_rdi_irq_mask_addr                   = 0x258,
	.csid_rdi_irq_clear_addr                  = 0x25C,
	.csid_rdi_irq_set_addr                    = 0x260,
	.csid_rdi_cfg0_addr                       = 0x3680,
	.csid_rdi_cfg1_addr                       = 0x3694,
	.csid_rdi_ctrl_addr                       = 0x3688,
	.csid_rdi_epoch_irq_cfg_addr              = 0x36B4,
	.csid_rdi_frm_drop_pattern_addr           = 0x36CC,
	.csid_rdi_frm_drop_period_addr            = 0x36D0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x36D4,
	.csid_rdi_irq_subsample_period_addr       = 0x36D8,
	.csid_rdi_rpp_hcrop_addr                  = 0x36DC,
	.csid_rdi_rpp_vcrop_addr                  = 0x36E0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x36E4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x36E8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x36EC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x36F0,
	.csid_rdi_rst_strobes_addr                = 0x0000,
	.csid_rdi_status_addr                     = 0x0000,
	.csid_rdi_misr_val0_addr                  = 0x37FC,
	.csid_rdi_misr_val1_addr                  = 0x3700,
	.csid_rdi_misr_val2_addr                  = 0x3704,
	.csid_rdi_misr_val3_addr                  = 0x3708,
	.csid_rdi_format_measure_cfg0_addr        = 0x370C,
	.csid_rdi_format_measure_cfg1_addr        = 0x3710,
	.csid_rdi_format_measure_cfg2_addr        = 0x3714,
	.csid_rdi_format_measure0_addr            = 0x3718,
	.csid_rdi_format_measure1_addr            = 0x371C,
	.csid_rdi_format_measure2_addr            = 0x3720,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x3724,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x3728,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x372C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x3730,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x3734,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x3738,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x373C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x3740,
	.csid_rdi_byte_cntr_ping_addr             = 0x37A8,
	.csid_rdi_byte_cntr_pong_addr             = 0x37AC,

	.epoch0_shift_val                         = 16,
	.timestamp_en_shift_val                   = 6,

	.top_irq_mask                             = {0x80000,},

	.fatal_err_mask                           = 0x2c1c6081,
	.non_fatal_err_mask                       = 0x12000000,

	.rup_mask                                     = 0x800,
	.aup_mask                                     = 0x800,
	.rup_aup_set_mask                             = 0x1,
};

static struct ais_ife_csid_ver2_csi2_rx_reg_offset
	ais_ife_csid_lite_common_reg_v1_csi2_reg_offset = {
	.csid_csi2_rx_irq_status_addr                 = {0x1B0, 0x1C0},
	.csid_csi2_rx_irq_mask_addr                   = {0x1B4, 0x1C4},
	.csid_csi2_rx_irq_clear_addr                  = {0x1B8, 0x1C8},
	.csid_csi2_rx_irq_set_addr                    = {0x1BC, 0x1CC},

	/*CSI2 rx control */
	.csid_csi2_rx_cfg0_addr                       = 0x0880,
	.csid_csi2_rx_cfg1_addr                       = 0x0884,
	.csid_csi2_rx_capture_ctrl_addr               = 0x0888,
	.csid_csi2_rx_rst_strobes_addr                = 0x088C,
	.csid_csi2_rx_de_scramble_cfg0_addr           = 0x0000,
	.csid_csi2_rx_de_scramble_cfg1_addr           = 0x0000,
	.csid_csi2_rx_cap_unmap_long_pkt_hdr_0_addr   = 0x0890,
	.csid_csi2_rx_cap_unmap_long_pkt_hdr_1_addr   = 0x0894,
	.csid_csi2_rx_captured_short_pkt_0_addr       = 0x0898,
	.csid_csi2_rx_captured_short_pkt_1_addr       = 0x089C,
	.csid_csi2_rx_captured_long_pkt_0_addr        = 0x08A0,
	.csid_csi2_rx_captured_long_pkt_1_addr        = 0x08A4,
	.csid_csi2_rx_captured_long_pkt_ftr_addr      = 0x08A8,
	.csid_csi2_rx_captured_cphy_pkt_hdr_addr      = 0x08AC,
	.csid_csi2_rx_lane0_misr_addr                 = 0x08B0,
	.csid_csi2_rx_lane1_misr_addr                 = 0x08B4,
	.csid_csi2_rx_lane2_misr_addr                 = 0x08B8,
	.csid_csi2_rx_lane3_misr_addr                 = 0x08BC,
	.csid_csi2_rx_total_pkts_rcvd_addr            = 0x08C0,
	.csid_csi2_rx_stats_ecc_addr                  = 0x08C4,
	.csid_csi2_rx_total_crc_err_addr              = 0x08C8,

	.csi2_rst_srb_all                             = 0x000,
	.csi2_rst_done_shift_val                      = 0,
	.csi2_irq_mask_all                            = 0xFFFFFFF,
	.csi2_misr_enable_shift_val                   = 6,
	.csi2_vc_mode_shift_val                       = 2,
	.csi2_capture_long_pkt_en_shift               = 0,
	.csi2_capture_short_pkt_en_shift              = 1,
	.csi2_capture_cphy_pkt_en_shift               = 2,
	.csi2_capture_long_pkt_dt_shift               = 4,
	.csi2_capture_long_pkt_vc_shift               = 10,
	.csi2_capture_short_pkt_vc_shift              = 15,
	.csi2_capture_cphy_pkt_dt_shift               = 20,
	.csi2_capture_cphy_pkt_vc_shift               = 26,
	.csi2_rx_phy_num_mask                         = 0xf,

	.vc_mask                                      = 0x7C00000,
	.dt_mask                                      = 0x3f0000,
	.wc_mask                                      = 0xffff,
	.vc_shift                                     = 0x16,
	.dt_shift                                     = 0x10,
	.wc_shift                                     = 0,
	.long_pkt_strobe_rst_shift                    = 0,
	.short_pkt_strobe_rst_shift                   = 1,
	.cphy_pkt_strobe_rst_shift                    = 2,
	.unmapped_pkt_strobe_rst_shift                = 3,

	.num_rx_irq                                   = CSID_RX_IRQ_CNT,
	.fatal_err_mask                               = {0x25fff000, 0x0},
	.part_fatal_err_mask                          = {0x02000000, 0x0},
	.non_fatal_err_mask                           = {0x08000000, 0x0},
	.top_irq_mask                                 = {0x4, 0x0},
	.rx2_irq_mask                                 = 0x80000000,
};

static struct ais_ife_csid_ver2_common_reg_offset
	ais_ife_csid_lite_common_reg_v1_cmn_reg_offset = {
	.csid_hw_version_addr                         = 0x0,
	.csid_cfg0_addr                               = 0x100,
	.csid_ctrl_addr                               = 0x104,
	.csid_reset_addr                              = 0x108,
	.csid_rst_strobes_addr                        = 0x10C,

	.csid_test_bus_ctrl_addr                      = 0x7DC,

	.csid_top_irq_status_addr                     = {0x180, 0x190},
	.csid_top_irq_mask_addr                       = {0x184, 0x194},
	.csid_top_irq_clear_addr                      = {0x188, 0x198},
	.csid_top_irq_set_addr                        = {0x18C, 0x19C},

	.csid_irq_cmd_addr                            = 0x110,

	.buf_done_irq_status_addr                     = 0x01A0,
	.buf_done_irq_mask_addr                       = 0x01A4,
	.buf_done_irq_clear_addr                      = 0x01A8,
	.buf_done_irq_set_addr                        = 0x01AC,

	/*configurations */
	.major_version                                = 10,
	.minor_version                                = 8,
	.version_incr                                 = 0,
	.num_rdis                                     = 4,
	.num_pix                                      = 0,
	.num_ppp                                      = 0,
	.csid_reg_rst_stb                             = 1,
	.csid_rst_stb                                 = 0x1,
	.csid_rst_stb_sw_all                          = 0x0,
	.path_rst_stb_all                             = 0x0,
	.path_rst_done_shift_val                      = 1,
	.path_en_shift_val                            = 31,
	.vfr_en_shift_val                             = 0,
	.vc_shift_val                                 = 22,
	.dt_shift_val                                 = 16,
	.fmt_shift_val                                = 12,
	.plain_fmt_shift_val                          = 12,
	.crop_v_en_shift_val                          = 16,
	.crop_h_en_shift_val                          = 16,
	.crop_shift                                   = 0,
	.ipp_irq_mask_all                             = 0x0,
	.rdi_irq_mask_all                             = 0x7FFF,
	.ppp_irq_mask_all                             = 0x0,
	.measure_en_hbi_vbi_cnt_mask                  = 0xC,
	.format_measure_en_val                        = 1,
	.format_measure_height_mask_val               = 0xFFFF,
	.format_measure_height_shift_val              = 0x10,
	.format_measure_width_mask_val                = 0xFFFF,
	.format_measure_width_shift_val               = 0x0,
	.timestamp_stb_sel_shift_val                  = 8,
	.frame_id_decode_en_shift_val                 = 1,
	.format_measure_en_shift_val                  = 3,

	.rst_loc_path_only_val                        = 0x0,
	.rst_loc_complete_csid_val                    = 0x1,
	.rst_mode_frame_boundary_val                  = 0x0,
	.rst_mode_immediate_val                       = 0x1,
	.rst_cmd_hw_reset_complete_val                = 0x1,
	.rst_cmd_sw_reset_complete_val                = 0x2,
	.rst_cmd_irq_ctrl_only_val                    = 0x4,

	.rst_location_shift_val                       = 0x4,
	.rst_mode_shift_val                           = 0x0,

	.epoch_factor                                 = 50,
	.timestamp_strobe_val                         = 0x2,

	.num_top_irq                                  = CSID_TOP_IRQ_CNT,
	.capabilities                                 = AIS_IFE_CSID_CAP_SKIP_PATH_CFG1 | AIS_IFE_CSID_CAP_SPLIT_RUP_AUP,
	.direct_cid_config                            = true,
	.vfr_supported                                = true,
	.frame_id_dec_supported                       = true,
	.timestamp_enabled_in_cfg0                    = true,

	.top_top2_irq_mask                            = 0x80000000,
	.top_buf_done_irq_mask                        = 0x8,
	.top_err_irq_mask                             = {0x00000002, 0x20},
	.top_reset_irq_mask                           = {0x1,},

	.rup_cmd_addr                                 = 0x0114,
	.aup_cmd_addr                                 = 0x0118,
	.rup_aup_cmd_addr                             = 0x011C,

	.buf_done_shift                               = {16, 17, 18, 19},
};

static struct ais_ife_csid_ver2_reg_offset ais_ife_csid_lite_common_reg_v1_reg_offset = {
	.cmn_reg          = &ais_ife_csid_lite_common_reg_v1_cmn_reg_offset,
	.csi2_reg         = &ais_ife_csid_lite_common_reg_v1_csi2_reg_offset,
	.rdi_reg = {
		&ais_ife_csid_lite_common_reg_v1_rdi_0_reg_offset,
		&ais_ife_csid_lite_common_reg_v1_rdi_1_reg_offset,
		&ais_ife_csid_lite_common_reg_v1_rdi_2_reg_offset,
		&ais_ife_csid_lite_common_reg_v1_rdi_3_reg_offset,
		},
	.path_debug_mask = &ais_ife_csid_ver2_lite_common_reg_v1_path_debug_mask,
	.rx_debug_mask = &ais_ife_csid_lite_common_reg_v1_rx_debug_mask,
	.top_debug_mask = &ais_ife_csid_ver2_lite_common_reg_v1_top_debug_mask,
	.top_reg = NULL,
	.need_top_cfg = 0x0,
	.input_core_sel = {0},
};

#endif /*_AIS_IFE_CSID_LITE_COMMON_REG_V1_H_ */
