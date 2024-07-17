/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-24     Rick       the first version
 */
#include <rtthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <finsh.h>
#include <fal.h>

#include "radio_protocol.h"
#include "radio_protocol_mainunit.h"

#define DBG_TAG "lora_ota"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

uint32_t total_size = 0;
void lora_ota_begin(rt_uint32_t sub_addr,uint32_t firm_size)
{
    total_size = firm_size;
    LOG_I("lora_ota_begin send to %d,file size %ld",sub_addr,firm_size);
    radio_mainunit_lora_ota_begin(sub_addr,firm_size);
}

void lora_ota_receive(rt_uint32_t sub_addr,rt_uint8_t *buf, rt_size_t offset, rt_size_t len)
{
    LOG_I("lora_ota_receive send to %d,file offset %d,total %d,len %d",sub_addr,offset,total_size,len);
    radio_mainunit_lora_ota_receive(sub_addr, buf, offset, len);
}

void lora_ota_end(rt_uint32_t sub_addr)
{
    LOG_I("lora_ota_end send to %d",sub_addr);
    radio_mainunit_lora_ota_end(sub_addr);
}

void radio_mainunit_lora_ota_begin(uint32_t device_addr,uint32_t firm_size)
{
    tx_format tx_frame = {0};
    uint8_t send_buf[8] = {0};

    send_buf[0] = 1;
    send_buf[1] = (firm_size >> 24) & 0xFF;
    send_buf[2] = (firm_size >> 16) & 0xFF;
    send_buf[3] = (firm_size >> 8) & 0xFF;
    send_buf[4] = firm_size & 0xFF;

    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = device_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = FIRMWARE_UPDATE_CMD;
    tx_frame.tx_data = send_buf;
    tx_frame.tx_len = 5;

    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_lora_ota_receive(rt_uint32_t device_addr,rt_uint8_t *buf, rt_size_t offset, rt_size_t len)
{
    tx_format tx_frame = {0};
    uint8_t send_buf[140] = {0};

    send_buf[0] = 2;
    send_buf[1] = (offset >> 24) & 0xFF;
    send_buf[2] = (offset >> 16) & 0xFF;
    send_buf[3] = (offset >> 8) & 0xFF;
    send_buf[4] = offset & 0xFF;
    send_buf[5] = len;
    rt_memcpy(&send_buf[6],buf,len);

    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = device_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = FIRMWARE_UPDATE_CMD;
    tx_frame.tx_data = send_buf;
    tx_frame.tx_len = 6 + len;

    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_lora_ota_end(uint32_t device_addr)
{
    tx_format tx_frame = {0};
    uint8_t send_buf[8] = {0};

    send_buf[0] = 3;

    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = device_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = FIRMWARE_UPDATE_CMD;
    tx_frame.tx_data = send_buf;
    tx_frame.tx_len = 1;

    radio_mainunit_command_send(&tx_frame);
}
