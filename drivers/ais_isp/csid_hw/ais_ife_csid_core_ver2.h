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

#ifndef _AIS_IFE_CSID_VER2_HW_H_
#define _AIS_IFE_CSID_VER2_HW_H_

#include "cam_hw.h"
#include "ais_ife_csid_common.h"
#include "ais_ife_csid_hw_intf.h"
#include "ais_ife_csid_soc.h"

/*csid top irq cnt*/
#define CSID_TOP_IRQ_CNT 2
/*csid rx irq cnt*/
#define CSID_RX_IRQ_CNT  2

#define AIS_IFE_CSID_CAP_SKIP_PATH_CFG1           BIT(0)
#define AIS_IFE_CSID_CAP_SPLIT_RUP_AUP            BIT(1)

#define AIS_IFE_CSID_HW_NUM_MAX                        8

#define AIS_IFE_CSID_WORK_MAX_DATA_SIZE                32

#define AIS_CSID_VER2_PATH_INFO_INPUT_EOF                  BIT(9)
#define AIS_CSID_VER2_PATH_INFO_INPUT_SOF                  BIT(12)
#define AIS_CSID_VER2_PATH_ERROR_CCIF_VIOLATION            BIT(29)

struct ais_ife_csid_ver2_hw;

/* enum for multiple mem base in some of the targets */
enum ais_ife_csid_ver2_mem_base_id {
	AIS_IFE_CSID_VER2_CLC_MEM_BASE_ID,
	AIS_IFE_CSID_VER2_TOP_MEM_BASE_ID,
	AIS_IFE_CSID_VER2_SEC_MEM_BASE_ID,
	AIS_IFE_CSID_VER2_MAX_MEM_BASE_ID,
};

enum ais_ife_csid_ver2_csid_reset_cmd {
	AIS_IFE_CSID_VER2_RESET_CMD_IRQ_CTRL,
	AIS_IFE_CSID_VER2_RESET_CMD_SW_RST,
	AIS_IFE_CSID_VER2_RESET_CMD_HW_RST,
	AIS_IFE_CSID_VER2_RESET_CMD_HW_MAX,
};

enum cam_ife_csid_ver2_csid_reset_loc {
	AIS_IFE_CSID_VER2_RESET_LOC_PATH_ONLY,
	AIS_IFE_CSID_VER2_RESET_LOC_COMPLETE,
	AIS_IFE_CSID_VER2_RESET_LOC_MAX,
};

/**
 *enum ais_csid_ver2_irq_status - csid irq status to keep track
 * various status registers
 */
enum ais_csid_ver2_irq_status {
	CSID_VER2_IRQ_STATUS_TOP,
	CSID_VER2_IRQ_STATUS_TOP_1,
	CSID_VER2_IRQ_STATUS_RX,
	CSID_VER2_IRQ_STATUS_RX_1,
	CSID_VER2_IRQ_STATUS_RDI0,
	CSID_VER2_IRQ_STATUS_RDI1,
	CSID_VER2_IRQ_STATUS_RDI2,
	CSID_VER2_IRQ_STATUS_RDI3,
	CSID_VER2_IRQ_STATUS_BUF_DONE,
	CSID_VER2_IRQ_STATUS_MAX,
};

enum ais_ife_csid_ver2_input_core_sel {
	AIS_IFE_CSID_INPUT_CORE_SEL_NONE,
	AIS_IFE_CSID_INPUT_CORE_SEL_INTERNAL,
	AIS_IFE_CSID_INPUT_CORE_SEL_SFE_0,
	AIS_IFE_CSID_INPUT_CORE_SEL_SFE_1,
	AIS_IFE_CSID_INPUT_CORE_SEL_SFE_2,
	AIS_IFE_CSID_INPUT_CORE_SEL_CUST_NODE_0,
	AIS_IFE_CSID_INPUT_CORE_SEL_CUST_NODE_1,
	AIS_IFE_CSID_INPUT_CORE_SEL_CUST_NODE_2,
	AIS_IFE_CSID_INPUT_CORE_SEL_MAX,
};

enum ais_ife_csid_ver2_rx_irq_status {
	AIS_IFE_CSI_VER2_RX_IRQ_STATUS_0,
	AIS_IFE_CSI_VER2_RX_IRQ_STATUS_1,
	AIS_IFE_CSI_VER2_RX_IRQ_STATUS_MAX,
};

enum ais_ife_csid_ver2_top_irq_status {
	AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_0,
	AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_1,
	AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_MAX,
};

/**
 * enum ais_ife_csid_hw_rx_events - Specify the rx irq events
 */
enum ais_ife_csid_ver2_hw_rx_events {
	AIS_IFE_CSID_RX_DL0_EOT_CAPTURED,
	AIS_IFE_CSID_RX_DL1_EOT_CAPTURED,
	AIS_IFE_CSID_RX_DL2_EOT_CAPTURED,
	AIS_IFE_CSID_RX_DL3_EOT_CAPTURED,
	AIS_IFE_CSID_RX_DL0_SOT_CAPTURED,
	AIS_IFE_CSID_RX_DL1_SOT_CAPTURED,
	AIS_IFE_CSID_RX_DL2_SOT_CAPTURED,
	AIS_IFE_CSID_RX_DL3_SOT_CAPTURED,
	AIS_IFE_CSID_RX_LONG_PKT_CAPTURED,
	AIS_IFE_CSID_RX_SHORT_PKT_CAPTURED,
	AIS_IFE_CSID_RX_CPHY_PKT_HDR_CAPTURED,
	AIS_IFE_CSID_RX_CPHY_EOT_RECEPTION,
	AIS_IFE_CSID_RX_CPHY_SOT_RECEPTION,
	AIS_IFE_CSID_RX_ERROR_CPHY_PH_CRC,
	AIS_IFE_CSID_RX_WARNING_ECC,
	AIS_IFE_CSID_RX_LANE0_FIFO_OVERFLOW,
	AIS_IFE_CSID_RX_LANE1_FIFO_OVERFLOW,
	AIS_IFE_CSID_RX_LANE2_FIFO_OVERFLOW,
	AIS_IFE_CSID_RX_LANE3_FIFO_OVERFLOW,
	AIS_IFE_CSID_RX_ERROR_CRC,
	AIS_IFE_CSID_RX_ERROR_ECC,
	AIS_IFE_CSID_RX_MMAPPED_VC_DT,
	AIS_IFE_CSID_RX_UNMAPPED_VC_DT,
	AIS_IFE_CSID_RX_STREAM_UNDERFLOW,
	AIS_IFE_CSID_RX_UNBOUNDED_FRAME,
	AIS_IFE_CSID_RX_ERROR_ILLEGAL_PROGRAMMING,
	AIS_IFE_CSID_RX_INFO_SENSOR_MODE_ID_CHANGE,
	AIS_IFE_CSID_RX_RX2_IRQ,
	AIS_IFE_CSID_RX_DL0_EOT_LOST,
	AIS_IFE_CSID_RX_DL1_EOT_LOST,
	AIS_IFE_CSID_RX_DL2_EOT_LOST,
	AIS_IFE_CSID_RX_DL3_EOT_LOST,
	AIS_IFE_CSID_RX_DL0_SOT_LOST,
	AIS_IFE_CSID_RX_DL1_SOT_LOST,
	AIS_IFE_CSID_RX_DL2_SOT_LOST,
	AIS_IFE_CSID_RX_DL3_SOT_LOST,
	AIS_IFE_CSID_RX_REG_IRQ_EVENTS_MAX,
};

/**
 * enum ais_ife_csid_hw_top_events - Specify the top irq events
 */
enum ais_ife_csid_hw_top_events {
	AIS_IFE_CSID_TOP_INFO_VOTE_UP,
	AIS_IFE_CSID_TOP_INFO_VOTE_DN,
	AIS_IFE_CSID_TOP_ERR_NO_VOTE_DN,
	AIS_IFE_CSID_TOP_ERR_VOTE_UP_LATE,
	AIS_IFE_CSID_TOP_ERR_RDI_LINE_BUFFER_CONFLICT,
	AIS_IFE_CSID_TOP_ERR_SENSOR_HBI,
	AIS_IFE_CSID_TOP_REG_IRQ_EVENTS_MAX,
};

enum ais_ife_csid_hw_path_events {
	AIS_IFE_CSID_PATH_INFO_INPUT_EOF,
	AIS_IFE_CSID_PATH_INFO_INPUT_SOF,
	AIS_IFE_CSID_PATH_REG_IRQ_EVENTS_MAX,
};

enum ais_ife_csid_ver2_cb_type {
	AIS_CSID_VER2_IRQ_CB_IFE_WORK,
	AIS_CSID_VER2_IRQ_CB_CSID_WORK,
};

enum ais_ife_csid_ver2_csid_work {
	AIS_IFE_MSG_CSID_RUP_DONE,
};

enum ais_ife_csid_ver2_irq_event {
	AIS_CSID_VER2_IRQ_EV_SOF,
	AIS_CSID_VER2_IRQ_EV_BUF_DONE,
};

/*
 *struct ais_ife_csid_ver2_rup_aup_mask: place holder for rup/aup mask parameter
 *
 * @rup_mask:                This stores value of rup mask  when rup and aup are split,
 *                           else it store value of unified rup aup mask.
 * @aup_mask:                This stores value of aup mask if rup and aup is split, else
 *                           this is to be ignored.
 * @rup_aup_set_mask:        This is used to store rup_aup_sync register mask when rup
 *                           and aup are split,else this value is to be ignored.
 */
struct ais_ife_csid_ver2_rup_aup_mask {
	uint32_t rup_mask;
	uint32_t aup_mask;
	uint32_t rup_aup_set_mask;
};

struct ais_csid_ver2_irq_event {
	int rdi_num;
	enum ais_ife_csid_ver2_irq_event event;
	uint32_t status[CSID_VER2_IRQ_STATUS_MAX];
	struct ais_ife_csid_ver2_hw *csid_hw;
	void *priv;
};

struct ais_ife_csid_ver2_rx_debug_mask {
	uint64_t evt_bitmap[AIS_IFE_CSI_VER2_RX_IRQ_STATUS_MAX];
	uint8_t bit_pos[AIS_IFE_CSID_RX_REG_IRQ_EVENTS_MAX];
};

struct ais_ife_csid_ver2_top_debug_mask {
	uint64_t evt_bitmap[AIS_IFE_CSI_VER2_TOP_IRQ_STATUS_MAX];
	uint8_t bit_pos[AIS_IFE_CSID_TOP_REG_IRQ_EVENTS_MAX];
};

struct ais_ife_csid_ver2_path_debug_mask {
	uint8_t bit_pos[AIS_IFE_CSID_PATH_REG_IRQ_EVENTS_MAX];
};

/**
 * struct ais_csid_hw_work_data- work data for csid
 * Later other fields can be added to this data
 * @evt_type   : Event type from CSID
 * @irq_status : IRQ Status register
 *
 */
struct ais_csid_ver2_hw_work_data {
	uint64_t timestamp;
	uint32_t evt_type;
	uint32_t irq_status[CSID_VER2_IRQ_STATUS_MAX];
	uint8_t data[AIS_IFE_CSID_WORK_MAX_DATA_SIZE];
};

struct ais_ife_csid_ver2_rdi_reg_offset {
	uint32_t csid_rdi_irq_status_addr;
	uint32_t csid_rdi_irq_mask_addr;
	uint32_t csid_rdi_irq_clear_addr;
	uint32_t csid_rdi_irq_set_addr;

	/*RDI N register address */
	uint32_t csid_rdi_cfg0_addr;
	uint32_t csid_rdi_cfg1_addr;
	uint32_t csid_rdi_ctrl_addr;
	uint32_t csid_rdi_epoch_irq_cfg_addr;
	uint32_t csid_rdi_frm_drop_pattern_addr;
	uint32_t csid_rdi_frm_drop_period_addr;
	uint32_t csid_rdi_irq_subsample_pattern_addr;
	uint32_t csid_rdi_irq_subsample_period_addr;
	uint32_t csid_rdi_rpp_hcrop_addr;
	uint32_t csid_rdi_rpp_vcrop_addr;
	uint32_t csid_rdi_rpp_pix_drop_pattern_addr;
	uint32_t csid_rdi_rpp_pix_drop_period_addr;
	uint32_t csid_rdi_rpp_line_drop_pattern_addr;
	uint32_t csid_rdi_rpp_line_drop_period_addr;
	uint32_t csid_rdi_yuv_chroma_conversion_addr;
	uint32_t csid_rdi_rst_strobes_addr;
	uint32_t csid_rdi_status_addr;
	uint32_t csid_rdi_misr_val0_addr;
	uint32_t csid_rdi_misr_val1_addr;
	uint32_t csid_rdi_misr_val2_addr;
	uint32_t csid_rdi_misr_val3_addr;
	uint32_t csid_rdi_format_measure_cfg0_addr;
	uint32_t csid_rdi_format_measure_cfg1_addr;
	uint32_t csid_rdi_format_measure_cfg2_addr;
	uint32_t csid_rdi_format_measure0_addr;
	uint32_t csid_rdi_format_measure1_addr;
	uint32_t csid_rdi_format_measure2_addr;
	uint32_t csid_rdi_timestamp_curr0_sof_addr;
	uint32_t csid_rdi_timestamp_curr1_sof_addr;
	uint32_t csid_rdi_timestamp_prev0_sof_addr;
	uint32_t csid_rdi_timestamp_prev1_sof_addr;
	uint32_t csid_rdi_timestamp_curr0_eof_addr;
	uint32_t csid_rdi_timestamp_curr1_eof_addr;
	uint32_t csid_rdi_timestamp_prev0_eof_addr;
	uint32_t csid_rdi_timestamp_prev1_eof_addr;
	uint32_t csid_rdi_byte_cntr_ping_addr;
	uint32_t csid_rdi_byte_cntr_pong_addr;

	/* configuration */
	uint32_t packing_format;
	uint32_t ccif_violation_en;
	uint32_t epoch0_shift_val;
	uint32_t timestamp_en_shift_val;

	uint32_t top_irq_mask[CSID_TOP_IRQ_CNT];

	uint32_t fatal_err_mask;
	uint32_t non_fatal_err_mask;

	uint32_t rup_mask;
	uint32_t aup_mask;
	uint32_t rup_aup_set_mask;
	uint32_t rup_aup_mask;
};

struct ais_ife_csid_ver2_csi2_rx_reg_offset {
	uint32_t csid_csi2_rx_irq_status_addr[CSID_RX_IRQ_CNT];
	uint32_t csid_csi2_rx_irq_mask_addr[CSID_RX_IRQ_CNT];
	uint32_t csid_csi2_rx_irq_clear_addr[CSID_RX_IRQ_CNT];
	uint32_t csid_csi2_rx_irq_set_addr[CSID_RX_IRQ_CNT];

	uint32_t csid_csi2_rx_cfg0_addr;
	uint32_t csid_csi2_rx_cfg1_addr;
	uint32_t csid_csi2_rx_capture_ctrl_addr;
	uint32_t csid_csi2_rx_rst_strobes_addr;
	uint32_t csid_csi2_rx_de_scramble_cfg0_addr;
	uint32_t csid_csi2_rx_de_scramble_cfg1_addr;
	uint32_t csid_csi2_rx_cap_unmap_long_pkt_hdr_0_addr;
	uint32_t csid_csi2_rx_cap_unmap_long_pkt_hdr_1_addr;
	uint32_t csid_csi2_rx_captured_short_pkt_0_addr;
	uint32_t csid_csi2_rx_captured_short_pkt_1_addr;
	uint32_t csid_csi2_rx_captured_long_pkt_0_addr;
	uint32_t csid_csi2_rx_captured_long_pkt_1_addr;
	uint32_t csid_csi2_rx_captured_long_pkt_ftr_addr;
	uint32_t csid_csi2_rx_captured_cphy_pkt_hdr_addr;
	uint32_t csid_csi2_rx_lane0_misr_addr;
	uint32_t csid_csi2_rx_lane1_misr_addr;
	uint32_t csid_csi2_rx_lane2_misr_addr;
	uint32_t csid_csi2_rx_lane3_misr_addr;
	uint32_t csid_csi2_rx_total_pkts_rcvd_addr;
	uint32_t csid_csi2_rx_stats_ecc_addr;
	uint32_t csid_csi2_rx_total_crc_err_addr;
	uint32_t csid_csi2_rx_de_scramble_type3_cfg0_addr;
	uint32_t csid_csi2_rx_de_scramble_type3_cfg1_addr;
	uint32_t csid_csi2_rx_de_scramble_type2_cfg0_addr;
	uint32_t csid_csi2_rx_de_scramble_type2_cfg1_addr;
	uint32_t csid_csi2_rx_de_scramble_type1_cfg0_addr;
	uint32_t csid_csi2_rx_de_scramble_type1_cfg1_addr;
	uint32_t csid_csi2_rx_de_scramble_type0_cfg0_addr;
	uint32_t csid_csi2_rx_de_scramble_type0_cfg1_addr;

	/*configurations */
	uint32_t csi2_rst_srb_all;
	uint32_t csi2_rst_done_shift_val;
	uint32_t csi2_irq_mask_all;
	uint32_t csi2_misr_enable_shift_val;
	uint32_t csi2_vc_mode_shift_val;
	uint32_t csi2_capture_long_pkt_en_shift;
	uint32_t csi2_capture_short_pkt_en_shift;
	uint32_t csi2_capture_cphy_pkt_en_shift;
	uint32_t csi2_capture_long_pkt_dt_shift;
	uint32_t csi2_capture_long_pkt_vc_shift;
	uint32_t csi2_capture_short_pkt_vc_shift;
	uint32_t csi2_capture_cphy_pkt_dt_shift;
	uint32_t csi2_capture_cphy_pkt_vc_shift;
	uint32_t csi2_rx_phy_num_mask;

	uint32_t vc_mask;
	uint32_t dt_mask;
	uint32_t wc_mask;
	uint32_t vc_shift;
	uint32_t dt_shift;
	uint32_t wc_shift;

	uint32_t long_pkt_strobe_rst_shift;
	uint32_t short_pkt_strobe_rst_shift;
	uint32_t cphy_pkt_strobe_rst_shift;
	uint32_t unmapped_pkt_strobe_rst_shift;

	uint32_t num_rx_irq;
	uint32_t fatal_err_mask[CSID_RX_IRQ_CNT];
	uint32_t part_fatal_err_mask[CSID_RX_IRQ_CNT];
	uint32_t non_fatal_err_mask[CSID_RX_IRQ_CNT];
	uint32_t top_irq_mask[CSID_TOP_IRQ_CNT];
	uint32_t rx2_irq_mask;
};

struct ais_ife_csid_ver2_common_reg_offset {
	/* MIPI CSID registers */
	uint32_t csid_hw_version_addr;
	uint32_t csid_cfg0_addr;
	uint32_t csid_ctrl_addr;
	uint32_t csid_reset_addr;
	uint32_t csid_rst_strobes_addr;

	uint32_t csid_test_bus_ctrl_addr;
	uint32_t csid_top_irq_status_addr[CSID_TOP_IRQ_CNT];
	uint32_t csid_top_irq_mask_addr[CSID_TOP_IRQ_CNT];
	uint32_t csid_top_irq_clear_addr[CSID_TOP_IRQ_CNT];
	uint32_t csid_top_irq_set_addr[CSID_TOP_IRQ_CNT];

	uint32_t csid_irq_cmd_addr;

	uint32_t buf_done_irq_status_addr;
	uint32_t buf_done_irq_mask_addr;
	uint32_t buf_done_irq_clear_addr;
	uint32_t buf_done_irq_set_addr;

	/*configurations */
	uint32_t major_version;
	uint32_t minor_version;
	uint32_t version_incr;
	uint32_t num_rdis;
	uint32_t num_pix;
	uint32_t num_ppp;
	uint32_t csid_reg_rst_stb;
	uint32_t csid_rst_stb;
	uint32_t csid_rst_stb_sw_all;
	uint32_t path_rst_stb_all;
	uint32_t path_rst_done_shift_val;
	uint32_t path_en_shift_val;
	uint32_t dt_id_shift_val;
	uint32_t vfr_en_shift_val;
	uint32_t vc_shift_val;
	uint32_t dt_shift_val;
	uint32_t fmt_shift_val;
	uint32_t plain_fmt_shift_val;
	uint32_t crop_v_en_shift_val;
	uint32_t crop_h_en_shift_val;
	uint32_t crop_shift;
	uint32_t ipp_irq_mask_all;
	uint32_t rdi_irq_mask_all;
	uint32_t ppp_irq_mask_all;
	uint32_t measure_en_hbi_vbi_cnt_mask;
	uint32_t format_measure_en_val;
	uint32_t format_measure_width_shift_val;
	uint32_t format_measure_width_mask_val;
	uint32_t format_measure_height_shift_val;
	uint32_t format_measure_height_mask_val;
	uint32_t timestamp_stb_sel_shift_val;
	uint32_t frame_id_decode_en_shift_val;
	uint32_t format_measure_en_shift_val;

	uint32_t rst_loc_path_only_val;
	uint32_t rst_loc_complete_csid_val;
	uint32_t rst_mode_frame_boundary_val;
	uint32_t rst_mode_immediate_val;
	uint32_t rst_cmd_hw_reset_complete_val;
	uint32_t rst_cmd_sw_reset_complete_val;
	uint32_t rst_cmd_irq_ctrl_only_val;
	uint32_t rst_location_shift_val;
	uint32_t rst_mode_shift_val;

	uint32_t epoch_factor;
	uint32_t timestamp_strobe_val;

	uint32_t num_top_irq;
	uint32_t capabilities;
	bool direct_cid_config;
	bool vfr_supported;
	bool frame_id_dec_supported;
	bool timestamp_enabled_in_cfg0;

	uint32_t top_top2_irq_mask;
	uint32_t top_buf_done_irq_mask;
	uint32_t top_err_irq_mask[CSID_TOP_IRQ_CNT];
	uint32_t top_reset_irq_mask[CSID_TOP_IRQ_CNT];

	uint32_t rup_cmd_addr;
	uint32_t aup_cmd_addr;
	uint32_t rup_aup_cmd_addr;

	uint32_t buf_done_shift[4];
};

struct ais_ife_csid_ver2_top_reg_info {
	uint32_t io_path_cfg0_addr[AIS_IFE_CSID_HW_NUM_MAX];
	uint32_t dual_csid_cfg0_addr[AIS_IFE_CSID_HW_NUM_MAX];
	uint32_t input_core_type_shift_val;
	uint32_t sfe_offline_en_shift_val;
	uint32_t out_ife_en_shift_val;
	uint32_t dual_sync_sel_shift_val;
	uint32_t dual_en_shift_val;
	uint32_t master_slave_sel_shift_val;
	uint32_t rdi_lcr_shift_val;
	uint32_t master_sel_val;
	uint32_t slave_sel_val;
	uint32_t io_path_cfg_rst_val;
	uint32_t dual_cfg_rst_val;
};

/**
 * struct ais_ife_csid_ver2_reg_offset- CSID instance register info
 *
 * @cmn_reg:  csid common registers info
 *
 */
struct ais_ife_csid_ver2_reg_offset {
	struct ais_ife_csid_ver2_common_reg_offset   *cmn_reg;
	struct ais_ife_csid_ver2_csi2_rx_reg_offset  *csi2_reg;
	struct ais_ife_csid_ver2_rdi_reg_offset      *rdi_reg[AIS_IFE_CSID_RDI_MAX];
	struct ais_ife_csid_csi2_tpg_reg_offset      *tpg_reg;
	struct ais_ife_csid_ver2_path_debug_mask     *path_debug_mask;
	struct ais_ife_csid_ver2_rx_debug_mask       *rx_debug_mask;
	struct ais_ife_csid_ver2_top_debug_mask      *top_debug_mask;
	struct ais_ife_csid_ver2_top_reg_info        *top_reg;
	uint32_t                                     need_top_cfg;
	int                                          input_core_sel[
	          AIS_IFE_CSID_HW_NUM_MAX][AIS_IFE_CSID_INPUT_CORE_SEL_MAX];
};


/**
 * struct ais_ife_csid_ver2_hw_info- CSID HW info
 *
 * @csid_reg:        csid register offsets
 * @hw_dts_version:  HW DTS version
 * @csid_max_clk:    maximim csid clock
 *
 */
struct ais_ife_csid_ver2_hw_info {
	struct ais_ife_csid_ver2_reg_offset        *csid_reg;
	uint32_t                                    hw_dts_version;
	uint32_t                                    csid_max_clk;

};

/**
 * struct ais_ife_csid_ver2_path_cfg- csid path configuration details. It is stored
 *                          as private data for IPP/ RDI paths
 * @vc :            Virtual channel number
 * @dt :            Data type number
 * @cid             cid number, it is same as DT_ID number in HW
 * @in_format:      input decode format
 * @out_format:     output format
 * @crop_enable:    crop is enable or disabled, if enabled
 *                  then remaining parameters are valid.
 * @start_pixel:    start pixel
 * @end_pixel:      end_pixel
 * @width:          width
 * @start_line:     start line
 * @end_line:       end_line
 * @height:         heigth
 * @sync_mode:       Applicable for IPP/RDI path reservation
 *                  Reserving the path for master IPP or slave IPP
 *                  master (set value 1), Slave ( set value 2)
 *                  for RDI, set  mode to none
 * @master_idx:     For Slave reservation, Give master IFE instance Index.
 *                  Slave will synchronize with master Start and stop operations
 * @clk_rate        Clock rate
 * @usage_type      Usage type ie dual or single ife usecase
 * @init_frame_drop init frame drop value. In dual ife case rdi need to drop one
 *                  more frame than pix.
 *
 */
struct ais_ife_csid_ver2_path_cfg {
	enum ais_isp_resource_state     state;
	uint32_t                        vc;
	uint32_t                        dt;
	uint32_t                        cid;
	uint32_t                        in_format;
	uint32_t                        out_format;

	bool                            pix_enable;
	uint32_t                        decode_fmt;
	uint32_t                        plain_fmt;
	uint32_t                        in_bpp;

	bool                            crop_enable;
	uint32_t                        start_pixel;
	uint32_t                        end_pixel;
	uint32_t                        width;
	uint32_t                        start_line;
	uint32_t                        end_line;
	uint32_t                        height;

	struct cam_isp_sensor_dimension measure_cfg;

	uint32_t                        master_idx;
	uint64_t                        clk_rate;
	uint32_t                        usage_type;

	uint32_t                        init_frame_drop;

	uint32_t                        sof_cnt;
	uint32_t                        prev_sof_hw_ts;
	uint32_t                        prev_sof_boot_ts;
	uint32_t                        epoch_cfg;
	uint32_t                        vfr_en;
	uint32_t                        frame_id_dec_en;
};

/*
 * @rx_mask:               Debug mask for rx irq
 */
struct ais_ife_csid_ver2_debug_info {
	uint32_t                        top_mask[CSID_TOP_IRQ_CNT];
	uint32_t                        rx_mask[CSID_RX_IRQ_CNT];
	uint32_t                        path_mask;
	uint32_t                        debug_val;
};

/**
 * struct ais_ife_csid_hw- csid hw device resources data
 *
 * @hw_intf:                  contain the csid hw interface information
 * @hw_info:                  csid hw device information
 * @csid_info:                csid hw specific information
 * @res_type:                 CSID in resource type
 * @csi2_rx_cfg:              Csi2 rx decoder configuration for csid
 * @csi2_rx_reserve_cnt:      CSI2 reservations count value
 * @csi2_cfg_cnt:             csi2 configuration count
 * @rdi_res:                  raw dump image path resources
 * @cid_res:                  cid resources state
 * @csid_top_reset_complete:  csid top reset completion
 * @csid_csi2_reset_complete: csi2 reset completion
 * @csid_rdin_reset_complete: rdi n completion
 * @csid_debug:               csid debug information to enable the SOT, EOT,
 *                            SOF, EOF, measure etc in the csid hw
 * @clk_rate                  Clock rate
 * @rdi_path                  RDI path configuration
 * @hbi                       Horizontal blanking
 * @vbi                       Vertical blanking
 * @sof_irq_triggered:        Flag is set on receiving event to enable sof irq
 *                            incase of SOF freeze.
 * @irq_debug_cnt:            Counter to track sof irq's when above flag is set.
 * @error_irq_count           Error IRQ count, if continuous error irq comes
 *                            need to stop the CSID and mask interrupts.
 * @device_enabled            Device enabled will set once CSID powered on and
 *                            initial configuration are done.
 * @lock_state                csid spin lock
 * @dual_usage                usage type, dual ife or single ife
 * @init_frame_drop           Initial frame drop number
 * @res_sof_cnt               path resource sof count value. it used for initial
 *                            frame drop
 * @prev_boot_timestamp       first bootime stamp at the start
 * @prev_qtimer_ts            stores csid timestamp
 * @fatal_err_detected        flag to indicate fatal errror is reported
 * @ctx                       Hw manager context
 * @work                      Work queue to handle CSID IRQ work
 * @work_data                 Work data to be passed to work queue
 * @event_cb                  Callback to hw manager if CSID event reported
 *
 */
struct ais_ife_csid_ver2_hw {
	uint32_t                            hw_version;   /*this should first member*/
	struct cam_hw_intf                  *hw_intf;
	struct cam_hw_info                  *hw_info;
	struct ais_ife_csid_ver2_hw_info    *csid_info;

	enum ais_isp_resource_state         state;
	struct ais_ife_csid_ver2_path_cfg   rdi_cfg[AIS_IFE_CSID_RDI_MAX];

	uint32_t                            res_type;
	struct ais_ife_csid_csi2_rx_cfg     csi2_rx_cfg;
	uint32_t                            csi2_reserve_cnt;
	uint32_t                            csi2_cfg_cnt;
	struct completion                   csid_top_complete;
	struct completion                   csid_csi2_complete;
	struct completion                   csid_rdi_complete[AIS_IFE_CSID_RDI_MAX];
	struct ais_ife_csid_ver2_debug_info debug_info;
	uint64_t                            csid_debug;
	uint64_t                            clk_rate;

	bool                                sof_irq_triggered;
	uint32_t                            irq_debug_cnt;
	uint32_t                            error_irq_count;
	bool                                fatal_err_detected;
	uint32_t                            device_enabled;
	spinlock_t                          lock_state;
	uint32_t                            dual_usage;
	uint32_t                            init_frame_drop;

	void                                *ctx;
	struct cam_req_mgr_core_workq       *work;
	struct ais_csid_ver2_hw_work_data   work_data[AIS_CSID_WORKQ_NUM_TASK];
	ais_ife_event_cb_func               event_cb;
	void                                *event_cb_priv;

	irqreturn_t                         (*vfe_event_cb)(struct ais_csid_ver2_irq_event *irq_event);
	void*                               vfe_hw_info;

	uint32_t                            rup_val_set;
	spinlock_t                          lock_rup;

};

int ais_ife_csid_ver2_hw_probe_init(struct cam_hw_intf  *csid_hw_intf,
	uint32_t csid_idx);

int ais_ife_csid_ver2_hw_deinit(struct ais_ife_csid_ver2_hw *ife_csid_hw);

#endif /* _AIS_IFE_CSID_VER2_HW_H_ */
