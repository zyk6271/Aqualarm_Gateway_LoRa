/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-03-20     Rick       the first version
 */
#include "rtthread.h"
#include "flashwork.h"
#include "wifi.h"
#include "radio_protocol.h"
#include "radio_protocol_mainunit.h"

#define DBG_TAG "RADIO_PROTOCOL_FACTORY"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static void radio_frame_factory_parse_heart(rx_format *rx_frame)
{
    rf_refresh();
    if(rx_frame->rssi > -70)
    {
        rf_led_factory(2);
    }
    else
    {
        rf_led_factory(1);
    }
    LOG_I("factory test frame received,rssi %d\r\n",rx_frame->rssi);
}

void radio_frame_factory_parse(rx_format *rx_frame)
{
    if((rx_frame->dest_addr != get_local_address()) || (rx_frame->source_addr != 0xFFFFFFFF))
    {
        return;
    }

    switch(rx_frame->rx_data[0])
    {
    case HEART_UPLOAD_CMD:
        radio_frame_factory_parse_heart(rx_frame);
        break;
    default:
        break;
    }
}

void radio_factory_command_send(tx_format *tx_frame)
{
    unsigned short send_len = 0;

    send_len = set_lora_tx_byte(send_len,0xEF);
    send_len = set_lora_tx_byte(send_len,(NET_REGION_SELECT << 4) | NETWORK_VERSION);
    send_len = set_lora_tx_byte(send_len,(tx_frame->msg_ack << 7) | (DEVICE_TYPE_GATEWAY << 3) | tx_frame->msg_type);
    send_len = set_lora_tx_word(send_len,tx_frame->dest_addr);
    send_len = set_lora_tx_word(send_len,tx_frame->source_addr);
    send_len = set_lora_tx_byte(send_len,tx_frame->command);
    send_len = set_lora_tx_byte(send_len,tx_frame->tx_len);
    send_len = set_lora_tx_buffer(send_len,tx_frame->tx_data,tx_frame->tx_len);
    send_len = set_lora_tx_crc(send_len);
    lora_tx_enqueue(get_lora_tx_buf(),send_len,tx_frame->parameter);
}

void radio_mainunit_factory_send()
{
    tx_format tx_frame = {0};
    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = 0xFFFFFFFF;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = HEART_UPLOAD_CMD;
    radio_factory_command_send(&tx_frame);
}
