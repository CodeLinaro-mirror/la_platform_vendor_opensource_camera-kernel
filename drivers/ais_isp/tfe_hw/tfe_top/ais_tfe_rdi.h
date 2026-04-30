/* Copyright (c) 2017-2018, 2020, The Linux Foundation. All rights reserved.
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

#ifndef _AIS_TFE_RDI_H_
#define _AIS_TFE_RDI_H_

#include "ais_tfe_top.h"

#define AIS_VFE_RDI_VER2_MAX  4

struct ais_tfe_rdi_reg {
    uint32_t     rdi_hw_version;
    uint32_t     rdi_hw_status;
    uint32_t     rdi_module_config;
    uint32_t     rdi_skip_period;
    uint32_t     rdi_irq_subsample_pattern;
    uint32_t     rdi_epoch_irq;
    uint32_t     rdi_debug_1;
    uint32_t     rdi_debug_0;
    uint32_t     rdi_test_bus_ctrl;
    uint32_t     rdi_spare;
    uint32_t     reg_update_cmd;
};


struct ais_vfe_rdi_reg_data {
	uint32_t     reg_update_cmd_data;
	uint32_t     sof_irq_mask;
	uint32_t     reg_update_irq_mask;
	uint32_t     pixel_pattern_shift;
	uint32_t     pixel_pattern_mask;
	uint32_t     rdi_out_enable_shift;
	uint32_t     epoch_line_cfg;
	uint32_t     subscribe_irq_mask[3];
};

struct ais_tfe_rdi_ver2_hw_info {
	struct ais_vfe_top_ver2_reg_offset_common  *common_reg;
	struct ais_tfe_rdi_reg       *rdi_reg[AIS_VFE_RDI_VER2_MAX];
	struct ais_vfe_rdi_reg_data  *reg_data[AIS_VFE_RDI_VER2_MAX];
};

#endif /* _AIS_TFE_RDI_H_ */
