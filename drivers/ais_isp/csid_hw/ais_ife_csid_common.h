/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _AIS_IFE_CSID_COMMON_H_
#define _AIS_IFE_CSID_COMMON_H_

#define AIS_CSID_VERSION_V170                 0x10070000
#define AIS_CSID_VERSION_V175                 0x10070050
#define AIS_CSID_VERSION_V200                 0x20000000

#define AIS_IFE_CSID_HW_RES_MAX      8
#define AIS_IFE_CSID_CID_RES_MAX     4
#define AIS_IFE_CSID_RDI_MAX         4
#define AIS_CSID_WORKQ_NUM_TASK      10

#define CSID_PATH_INFO_RST_DONE                   BIT(1)
#define CSID_PATH_ERROR_FIFO_OVERFLOW             BIT(2)
#define CSID_PATH_INFO_SUBSAMPLED_EOF             BIT(3)
#define CSID_PATH_INFO_SUBSAMPLED_SOF             BIT(4)
#define CSID_PATH_INFO_FRAME_DROP_EOF             BIT(5)
#define CSID_PATH_INFO_FRAME_DROP_EOL             BIT(6)
#define CSID_PATH_INFO_FRAME_DROP_SOL             BIT(7)
#define CSID_PATH_INFO_FRAME_DROP_SOF             BIT(8)
#define CSID_PATH_INFO_INPUT_EOF                  BIT(9)
#define CSID_PATH_INFO_INPUT_EOL                  BIT(10)
#define CSID_PATH_INFO_INPUT_SOL                  BIT(11)
#define CSID_PATH_INFO_INPUT_SOF                  BIT(12)
#define CSID_PATH_ERROR_PIX_COUNT                 BIT(13)
#define CSID_PATH_ERROR_LINE_COUNT                BIT(14)
#define CSID_PATH_ERROR_CCIF_VIOLATION            BIT(15)
#define CSID_PATH_ERROR_REC_OVERFLOW              BIT(19)
#define CSID_PATH_ERROR_CAMIF_CCIF_VIOLATION      BIT(20)

/*
 * Debug values enable the corresponding interrupts and debug logs provide
 * necessary information
 */
#define CSID_DEBUG_ENABLE_SOF_IRQ                 BIT(0)
#define CSID_DEBUG_ENABLE_EOF_IRQ                 BIT(1)
#define CSID_DEBUG_ENABLE_SOT_IRQ                 BIT(2)
#define CSID_DEBUG_ENABLE_EOT_IRQ                 BIT(3)
#define CSID_DEBUG_ENABLE_SHORT_PKT_CAPTURE       BIT(4)
#define CSID_DEBUG_ENABLE_LONG_PKT_CAPTURE        BIT(5)
#define CSID_DEBUG_ENABLE_CPHY_PKT_CAPTURE        BIT(6)
#define CSID_DEBUG_ENABLE_HBI_VBI_INFO            BIT(7)
#define CSID_DEBUG_DISABLE_EARLY_EOF              BIT(8)
#define CSID_DEBUG_ENABLE_UNMAPPED_VC_DT_IRQ      BIT(9)
#define CSID_DEBUG_ENABLE_VOTE_UP_IRQ             BIT(10)
#define CSID_DEBUG_ENABLE_VOTE_DN_IRQ             BIT(11)
#define CSID_DEBUG_ENABLE_ERR_NO_VOTE_DN_IRQ      BIT(12)


/* enum ais_csid_path_halt_mode select the path halt mode control */
enum ais_csid_path_halt_mode {
	CSID_HALT_MODE_INTERNAL,
	CSID_HALT_MODE_GLOBAL,
	CSID_HALT_MODE_MASTER,
	CSID_HALT_MODE_SLAVE,
};

/**
 *enum ais_csid_path_timestamp_stb_sel - select the sof/eof strobes used to
 *        capture the timestamp
 */
enum ais_csid_path_timestamp_stb_sel {
	CSID_TIMESTAMP_STB_PRE_HALT,
	CSID_TIMESTAMP_STB_POST_HALT,
	CSID_TIMESTAMP_STB_POST_IRQ,
	CSID_TIMESTAMP_STB_MAX,
};

/**
 * struct ais_ife_csid_csi2_rx_cfg- csid csi2 rx configuration data
 * @phy_sel:     input resource type for sensor only
 * @lane_type:   lane type: c-phy or d-phy
 * @lane_num :   active lane number
 * @lane_cfg:    lane configurations: 4 bits per lane
 *
 */
struct ais_ife_csid_csi2_rx_cfg  {
	uint32_t                        phy_sel;
	uint32_t                        lane_type;
	uint32_t                        lane_num;
	uint32_t                        lane_cfg;
};

struct ais_ife_csid_hw_data {
	void        *csid_reg;
	uint32_t    hw_dts_version;
	uint32_t    reserve;

};
#endif /* _AIS_IFE_CSID_COMMON_H_ */
