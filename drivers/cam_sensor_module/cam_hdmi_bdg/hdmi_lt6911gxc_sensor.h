/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef _HDMI_LT6911GXC_SENSOR_H_
#define _HDMI_LT6911GXC_SENSOR_H_

#include <cam_sensor_cmn_header.h>

struct cam_sensor_i2c_reg_array  gxc_write_en_regs [6] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0xE1,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x03,
    .reg_data = 0x2E,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x03,
    .reg_data = 0xEE,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0xFF,
    .reg_data = 0xE0,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x04,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_write_config_regs [4] = {
{
    .reg_addr = 0x5E,
    .reg_data = 0xDF,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x20,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x21,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_write_addr_set_regs [5] = {
{
    .reg_addr = 0x5B,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5C,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5D,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x10,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_write_over_regs [2] = {
{
    .reg_addr = 0x5A,
    .reg_data = 0x08,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_block_erase_regs [9] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0xe0,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0xEE,
    .reg_data = 0x01,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x04,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5B,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5C,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5D,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x01,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_block_erase_ext_regs [9] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0xe0,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0xEE,
    .reg_data = 0x01,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x04,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5B,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5C,
    .reg_data = 0x80,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5D,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x01,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_config_regs [8] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0xe0,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0xEE,
    .reg_data = 0x01,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5E,
    .reg_data = 0xc1,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x59,
    .reg_data = 0x50,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x10,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x21,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_read_addr_regs [9] = {
{
    .reg_addr = 0x5E,
    .reg_data = 0x5F,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x20,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5B,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5C,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5D,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x10,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x5A,
    .reg_data = 0x00,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0x58,
    .reg_data = 0x21,
    .delay = 0x01,
    .data_mask = 0x00
}};

struct cam_sensor_i2c_reg_array  gxc_get_fw_regs [3] = {
{
    .reg_addr = 0xFF,
    .reg_data = 0xE0,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0xEE,
    .reg_data = 0x01,
    .delay = 0x01,
    .data_mask = 0x00
},
{
    .reg_addr = 0xFF,
    .reg_data = 0xE0,
    .delay = 0x01,
    .data_mask = 0x00
}};

#endif
