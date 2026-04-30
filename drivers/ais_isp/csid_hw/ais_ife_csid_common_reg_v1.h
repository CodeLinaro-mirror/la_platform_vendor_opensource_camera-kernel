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

#ifndef _AIS_IFE_CSID_COMMON_REG_V1_H_
#define _AIS_IFE_CSID_COMMON_REG_V1_H_

#include "ais_ife_csid_core_ver2.h"

static struct ais_ife_csid_ver2_rx_debug_mask
	ais_ife_csid_common_reg_v1_rx_debug_mask = {
	.evt_bitmap = {
		BIT_ULL(AIS_IFE_CSID_RX_DL0_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL1_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL2_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL3_EOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL0_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL1_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL2_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_DL3_SOT_CAPTURED) |
			BIT_ULL(AIS_IFE_CSID_RX_WARNING_ECC) |
			BIT_ULL(AIS_IFE_CSID_RX_ERROR_CPHY_PH_CRC) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE0_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE1_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE2_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_LANE3_FIFO_OVERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_ERROR_CRC) |
			BIT_ULL(AIS_IFE_CSID_RX_ERROR_ECC) |
			BIT_ULL(AIS_IFE_CSID_RX_MMAPPED_VC_DT) |
			BIT_ULL(AIS_IFE_CSID_RX_UNMAPPED_VC_DT) |
			BIT_ULL(AIS_IFE_CSID_RX_STREAM_UNDERFLOW) |
			BIT_ULL(AIS_IFE_CSID_RX_RX2_IRQ) |
			BIT_ULL(AIS_IFE_CSID_RX_DL0_EOT_LOST) |
			BIT_ULL(AIS_IFE_CSID_RX_DL1_EOT_LOST) |
			BIT_ULL(AIS_IFE_CSID_RX_DL2_EOT_LOST) |
			BIT_ULL(AIS_IFE_CSID_RX_DL3_EOT_LOST) |
			BIT_ULL(AIS_IFE_CSID_RX_DL0_SOT_LOST) |
			BIT_ULL(AIS_IFE_CSID_RX_DL1_SOT_LOST) |
			BIT_ULL(AIS_IFE_CSID_RX_DL2_SOT_LOST) |
			BIT_ULL(AIS_IFE_CSID_RX_DL3_SOT_LOST),

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
	.bit_pos[AIS_IFE_CSID_RX_WARNING_ECC] = 8,
	.bit_pos[AIS_IFE_CSID_RX_LONG_PKT_CAPTURED] = 0,
	.bit_pos[AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED] = 1,
	.bit_pos[AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED] = 2,
	.bit_pos[AIS_IFE_CSID_RX_ERROR_CPHY_PH_CRC] = 24,
	.bit_pos[AIS_IFE_CSID_RX_LANE0_FIFO_OVERFLOW] = 20,
	.bit_pos[AIS_IFE_CSID_RX_LANE1_FIFO_OVERFLOW] = 21,
	.bit_pos[AIS_IFE_CSID_RX_LANE2_FIFO_OVERFLOW] = 22,
	.bit_pos[AIS_IFE_CSID_RX_LANE3_FIFO_OVERFLOW] = 23,
	.bit_pos[AIS_IFE_CSID_RX_ERROR_CRC] = 25,
	.bit_pos[AIS_IFE_CSID_RX_ERROR_ECC] = 26,
	.bit_pos[AIS_IFE_CSID_RX_MMAPPED_VC_DT] = 27,
	.bit_pos[AIS_IFE_CSID_RX_UNMAPPED_VC_DT] = 28,
	.bit_pos[AIS_IFE_CSID_RX_STREAM_UNDERFLOW] = 29,
	.bit_pos[AIS_IFE_CSID_RX_RX2_IRQ] = 31,
	.bit_pos[AIS_IFE_CSID_RX_DL0_EOT_LOST] = 12,
	.bit_pos[AIS_IFE_CSID_RX_DL1_EOT_LOST] = 13,
	.bit_pos[AIS_IFE_CSID_RX_DL2_EOT_LOST] = 14,
	.bit_pos[AIS_IFE_CSID_RX_DL3_EOT_LOST] = 15,
	.bit_pos[AIS_IFE_CSID_RX_DL0_SOT_LOST] = 16,
	.bit_pos[AIS_IFE_CSID_RX_DL1_SOT_LOST] = 17,
	.bit_pos[AIS_IFE_CSID_RX_DL2_SOT_LOST] = 18,
	.bit_pos[AIS_IFE_CSID_RX_DL3_SOT_LOST] = 19,
};

static struct ais_ife_csid_ver2_top_debug_mask
	ais_ife_csid_ver2_common_reg_v1_top_debug_mask = {
	.evt_bitmap = {
		0ULL,

		BIT_ULL(AIS_IFE_CSID_TOP_INFO_VOTE_UP) |
		BIT_ULL(AIS_IFE_CSID_TOP_INFO_VOTE_DN) |
		BIT_ULL(AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN),
	},
	.bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_UP] = 0,
	.bit_pos[AIS_IFE_CSID_TOP_INFO_VOTE_DN] = 1,
	.bit_pos[AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN] = 2,
};

static struct ais_ife_csid_ver2_path_debug_mask
	ais_ife_csid_ver2_common_reg_v1_path_debug_mask = {
	.bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_EOF] = 9,
	.bit_pos[AIS_IFE_CSID_PATH_INFO_INPUT_SOF] = 12,
};

static struct ais_ife_csid_ver2_rdi_reg_offset
	ais_ife_csid_common_reg_v1_rdi_0_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x0224,
	.csid_rdi_irq_mask_addr                   = 0x0228,
	.csid_rdi_irq_clear_addr                  = 0x022C,
	.csid_rdi_irq_set_addr                    = 0x0230,
	.csid_rdi_cfg0_addr                       = 0x5480,
	.csid_rdi_cfg1_addr                       = 0x5494,
	.csid_rdi_ctrl_addr                       = 0x5488,
	.csid_rdi_epoch_irq_cfg_addr              = 0x54B4,
	.csid_rdi_frm_drop_pattern_addr           = 0x54CC,
	.csid_rdi_frm_drop_period_addr            = 0x54D0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x54D4,
	.csid_rdi_irq_subsample_period_addr       = 0x54D8,
	.csid_rdi_rpp_hcrop_addr                  = 0x54DC,
	.csid_rdi_rpp_vcrop_addr                  = 0x54E0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x54E4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x54E8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x54EC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x54F0,
	.csid_rdi_rst_strobes_addr                = 0x0000,
	.csid_rdi_status_addr                     = 0x0000,
	.csid_rdi_misr_val0_addr                  = 0x54FC,
	.csid_rdi_misr_val1_addr                  = 0x5500,
	.csid_rdi_misr_val2_addr                  = 0x5504,
	.csid_rdi_misr_val3_addr                  = 0x5508,
	.csid_rdi_format_measure_cfg0_addr        = 0x550C,
	.csid_rdi_format_measure_cfg1_addr        = 0x5510,
	.csid_rdi_format_measure_cfg2_addr        = 0x5514,
	.csid_rdi_format_measure0_addr            = 0x5518,
	.csid_rdi_format_measure1_addr            = 0x551C,
	.csid_rdi_format_measure2_addr            = 0x5520,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x5524,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x5528,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x552C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x5530,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x5534,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x5538,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x553C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x5540,
	.csid_rdi_byte_cntr_ping_addr             = 0x54A8,
	.csid_rdi_byte_cntr_pong_addr             = 0x54AC,

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
	ais_ife_csid_common_reg_v1_rdi_1_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x0234,
	.csid_rdi_irq_mask_addr                   = 0x0238,
	.csid_rdi_irq_clear_addr                  = 0x023C,
	.csid_rdi_irq_set_addr                    = 0x0240,
	.csid_rdi_cfg0_addr                       = 0x5680,
	.csid_rdi_cfg1_addr                       = 0x5694,
	.csid_rdi_ctrl_addr                       = 0x5688,
	.csid_rdi_epoch_irq_cfg_addr              = 0x56B4,
	.csid_rdi_frm_drop_pattern_addr           = 0x56CC,
	.csid_rdi_frm_drop_period_addr            = 0x56D0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x56D4,
	.csid_rdi_irq_subsample_period_addr       = 0x56D8,
	.csid_rdi_rpp_hcrop_addr                  = 0x56DC,
	.csid_rdi_rpp_vcrop_addr                  = 0x56E0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x56E4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x56E8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x56EC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x56F0,
	.csid_rdi_rst_strobes_addr                = 0x0000,
	.csid_rdi_status_addr                     = 0x0000,
	.csid_rdi_misr_val0_addr                  = 0x56FC,
	.csid_rdi_misr_val1_addr                  = 0x5700,
	.csid_rdi_misr_val2_addr                  = 0x5704,
	.csid_rdi_misr_val3_addr                  = 0x5708,
	.csid_rdi_format_measure_cfg0_addr        = 0x570C,
	.csid_rdi_format_measure_cfg1_addr        = 0x5710,
	.csid_rdi_format_measure_cfg2_addr        = 0x5714,
	.csid_rdi_format_measure0_addr            = 0x5718,
	.csid_rdi_format_measure1_addr            = 0x571C,
	.csid_rdi_format_measure2_addr            = 0x5720,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x5724,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x5728,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x572C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x5730,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x5734,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x5738,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x573C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x5740,
	.csid_rdi_byte_cntr_ping_addr             = 0x57A8,
	.csid_rdi_byte_cntr_pong_addr             = 0x57AC,

	.epoch0_shift_val                         = 16,
	.timestamp_en_shift_val                   = 6,

	.top_irq_mask                             = {0x20000,},

	.fatal_err_mask                           = 0x2c1c6001,
	.non_fatal_err_mask                       = 0x12000000,

	.rup_mask                                     = 0x200,
	.aup_mask                                     = 0x200,
	.rup_aup_set_mask                             = 0x1,
};

static struct ais_ife_csid_ver2_rdi_reg_offset
	ais_ife_csid_common_reg_v1_rdi_2_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x244,
	.csid_rdi_irq_mask_addr                   = 0x248,
	.csid_rdi_irq_clear_addr                  = 0x24C,
	.csid_rdi_irq_set_addr                    = 0x250,
	.csid_rdi_cfg0_addr                       = 0x5880,
	.csid_rdi_cfg1_addr                       = 0x5894,
	.csid_rdi_ctrl_addr                       = 0x5888,
	.csid_rdi_epoch_irq_cfg_addr              = 0x58B4,
	.csid_rdi_frm_drop_pattern_addr           = 0x58CC,
	.csid_rdi_frm_drop_period_addr            = 0x58D0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x58D4,
	.csid_rdi_irq_subsample_period_addr       = 0x58D8,
	.csid_rdi_rpp_hcrop_addr                  = 0x58DC,
	.csid_rdi_rpp_vcrop_addr                  = 0x58E0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x58E4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x58E8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x58EC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x58F0,
	.csid_rdi_rst_strobes_addr                = 0x0000,
	.csid_rdi_status_addr                     = 0x0000,
	.csid_rdi_misr_val0_addr                  = 0x58FC,
	.csid_rdi_misr_val1_addr                  = 0x5900,
	.csid_rdi_misr_val2_addr                  = 0x5904,
	.csid_rdi_misr_val3_addr                  = 0x5908,
	.csid_rdi_format_measure_cfg0_addr        = 0x590C,
	.csid_rdi_format_measure_cfg1_addr        = 0x5910,
	.csid_rdi_format_measure_cfg2_addr        = 0x5914,
	.csid_rdi_format_measure0_addr            = 0x5918,
	.csid_rdi_format_measure1_addr            = 0x591C,
	.csid_rdi_format_measure2_addr            = 0x5920,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x5924,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x5928,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x592C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x5930,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x5934,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x5938,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x593C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x5940,
	.csid_rdi_byte_cntr_ping_addr             = 0x59A8,
	.csid_rdi_byte_cntr_pong_addr             = 0x59AC,

	.epoch0_shift_val                         = 16,
	.timestamp_en_shift_val                   = 6,

	.top_irq_mask                             = {0x40000,},

	.fatal_err_mask                           = 0x2c1c6001,
	.non_fatal_err_mask                       = 0x12000000,

	.rup_mask                                     = 0x400,
	.aup_mask                                     = 0x400,
	.rup_aup_set_mask                             = 0x1,
};

static struct ais_ife_csid_ver2_rdi_reg_offset
	ais_ife_csid_common_reg_v1_rdi_3_reg_offset = {
	.csid_rdi_irq_status_addr                 = 0x254,
	.csid_rdi_irq_mask_addr                   = 0x258,
	.csid_rdi_irq_clear_addr                  = 0x25C,
	.csid_rdi_irq_set_addr                    = 0x260,
	.csid_rdi_cfg0_addr                       = 0x5A80,
	.csid_rdi_cfg1_addr                       = 0x5A94,
	.csid_rdi_ctrl_addr                       = 0x5A88,
	.csid_rdi_epoch_irq_cfg_addr              = 0x5AB4,
	.csid_rdi_frm_drop_pattern_addr           = 0x5ACC,
	.csid_rdi_frm_drop_period_addr            = 0x5AD0,
	.csid_rdi_irq_subsample_pattern_addr      = 0x5AD4,
	.csid_rdi_irq_subsample_period_addr       = 0x5AD8,
	.csid_rdi_rpp_hcrop_addr                  = 0x5ADC,
	.csid_rdi_rpp_vcrop_addr                  = 0x5AE0,
	.csid_rdi_rpp_pix_drop_pattern_addr       = 0x5AE4,
	.csid_rdi_rpp_pix_drop_period_addr        = 0x5AE8,
	.csid_rdi_rpp_line_drop_pattern_addr      = 0x5AEC,
	.csid_rdi_rpp_line_drop_period_addr       = 0x5AF0,
	.csid_rdi_rst_strobes_addr                = 0x0000,
	.csid_rdi_status_addr                     = 0x0000,
	.csid_rdi_misr_val0_addr                  = 0x5AFC,
	.csid_rdi_misr_val1_addr                  = 0x5B00,
	.csid_rdi_misr_val2_addr                  = 0x5B04,
	.csid_rdi_misr_val3_addr                  = 0x5B08,
	.csid_rdi_format_measure_cfg0_addr        = 0x5B0C,
	.csid_rdi_format_measure_cfg1_addr        = 0x5B10,
	.csid_rdi_format_measure_cfg2_addr        = 0x5B14,
	.csid_rdi_format_measure0_addr            = 0x5B18,
	.csid_rdi_format_measure1_addr            = 0x5B1C,
	.csid_rdi_format_measure2_addr            = 0x5B20,
	.csid_rdi_timestamp_curr0_sof_addr        = 0x5B24,
	.csid_rdi_timestamp_curr1_sof_addr        = 0x5B28,
	.csid_rdi_timestamp_prev0_sof_addr        = 0x5B2C,
	.csid_rdi_timestamp_prev1_sof_addr        = 0x5B30,
	.csid_rdi_timestamp_curr0_eof_addr        = 0x5B34,
	.csid_rdi_timestamp_curr1_eof_addr        = 0x5B38,
	.csid_rdi_timestamp_prev0_eof_addr        = 0x5B3C,
	.csid_rdi_timestamp_prev1_eof_addr        = 0x5B40,
	.csid_rdi_byte_cntr_ping_addr             = 0x5BA8,
	.csid_rdi_byte_cntr_pong_addr             = 0x5BAC,

	.epoch0_shift_val                         = 16,
	.timestamp_en_shift_val                   = 6,

	.top_irq_mask                             = {0x80000,},

	.fatal_err_mask                           = 0x2c1c6001,
	.non_fatal_err_mask                       = 0x12000000,

	.rup_mask                                     = 0x800,
	.aup_mask                                     = 0x800,
	.rup_aup_set_mask                             = 0x1,
};

static struct ais_ife_csid_ver2_csi2_rx_reg_offset
	ais_ife_csid_common_reg_v1_csi2_reg_offset = {
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

	.csi2_rst_srb_all                             = 0x0000,
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
			ais_ife_csid_common_reg_v1_cmn_reg_offset = {
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
	.csid_reg_rst_stb                             = 2,
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
	.format_measure_en_shift_val                  = 4,

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
	.top_err_irq_mask                             = {0x00000002, 0x18},
	.top_reset_irq_mask                           = {0x1,},

	.rup_cmd_addr                                 = 0x0114,
	.aup_cmd_addr                                 = 0x0118,
	.rup_aup_cmd_addr                             = 0x011C,

	.buf_done_shift                               = {16, 17, 18, 19},
};

static struct ais_ife_csid_ver2_reg_offset ais_ife_csid_common_reg_v1_reg_offset = {
	.cmn_reg          = &ais_ife_csid_common_reg_v1_cmn_reg_offset,
	.csi2_reg         = &ais_ife_csid_common_reg_v1_csi2_reg_offset,
	.rdi_reg = {
		&ais_ife_csid_common_reg_v1_rdi_0_reg_offset,
		&ais_ife_csid_common_reg_v1_rdi_1_reg_offset,
		&ais_ife_csid_common_reg_v1_rdi_2_reg_offset,
		&ais_ife_csid_common_reg_v1_rdi_3_reg_offset,
		},
	.path_debug_mask = &ais_ife_csid_ver2_common_reg_v1_path_debug_mask,
	.rx_debug_mask = &ais_ife_csid_common_reg_v1_rx_debug_mask,
	.top_debug_mask = &ais_ife_csid_ver2_common_reg_v1_top_debug_mask,
	.top_reg = NULL,
	.need_top_cfg = 0x0,
	.input_core_sel = {0},
};

#endif /*_AIS_IFE_CSID_COMMON_REG_V1_H_ */
