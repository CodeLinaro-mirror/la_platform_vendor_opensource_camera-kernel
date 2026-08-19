/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef HDMI_LT6911UXC_SENSOR_H
#define HDMI_LT6911UXC_SENSOR_H

#include <cam_sensor_cmn_header.h>

struct cam_sensor_i2c_reg_array  uxc_write_config_regs [3] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5E,
    .reg_data = 0xDF,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x21,
    .delay = 0x00,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  uxc_write_addr_set_regs [6] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5B,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5C,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5D,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x10,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  uxc_write_over_regs [4] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x08,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  uxc_block_erase_regs [8] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x04,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5B,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5C,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5D,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x01,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array uxc_config_regs[8] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0xEE,
    .reg_data = 0x01,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5E,
    .reg_data = 0xDF,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x59,
    .reg_data = 0x50,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x10,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x21,
    .delay = 0x00,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  uxc_read_addr_regs [11] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5E,
    .reg_data = 0x5F,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x20,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5B,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5C,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5D,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x10,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x21,
    .delay = 0x00,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  uxc_get_fw_regs [3] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0x80,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0xEE,
    .reg_data = 0x01,
    .delay = 0x00,
    .data_mask = 0x00
},
{
    .reg_addr = 0xFF,
    .reg_data = 0x81,
    .delay = 0x00,
    .data_mask = 0x00
}};

#endif
