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

#define DBG_TAG "RADIO_PROTOCOL_MAINUNIT"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

void device_warning_status_parse(uint32_t source_addr,uint32_t value)
{
    switch(value)
    {
    case ValveClose:
    case ValveOpen:
        wifi_mainunit_clear_warning(source_addr);
        break;
    case MasterSensorLost:
        wifi_mainunit_upload_warning(source_addr,2,1);
        break;
    case MasterSensorLeak:
        wifi_mainunit_upload_warning(source_addr,1,1);
        break;
    case MasterSensorAbnormal:
        wifi_mainunit_upload_warning(source_addr,1,0);
        break;
    case MasterLowTemp:
        wifi_mainunit_upload_warning(source_addr,3,1);
        break;
    case InternalValveFail:
    case ExtendValveFail:
        wifi_mainunit_upload_warning(source_addr,0,1);
        break;
    default:
        break;
    }
}
static void radio_frame_mainunit_parse_heart(rx_format *rx_frame)
{
    uint8_t sub_rssi = 0;
    uint8_t sub_bat = 0;
    uint8_t sub_command = 0;
    uint32_t slaver_addr = 0;
    tx_format tx_frame = {0};

    if((rx_frame->dest_addr != get_local_address()) || (aq_device_find(rx_frame->source_addr) == RT_NULL))
    {
        return;
    }

    if(rx_frame->msg_type == MSG_CONFIRMED_UPLINK)
    {
        tx_frame.msg_ack = RT_TRUE;
        tx_frame.msg_type = MSG_UNCONFIRMED_DOWNLINK;
        tx_frame.dest_addr = rx_frame->source_addr;
        tx_frame.source_addr = get_local_address();
        tx_frame.command = CONFIRM_ACK_CMD;
        radio_mainunit_command_send(&tx_frame);
    }

    sub_command = rx_frame->rx_data[2];
    switch(sub_command)
    {
    case 0://power_on heart
        wifi_mainunit_warning_reset(rx_frame->source_addr);
        device_warning_status_parse(rx_frame->source_addr,rx_frame->rx_data[3]);
        wifi_mainunit_valve_upload(rx_frame->source_addr,rx_frame->rx_data[4]);//主控开关阀
        aq_device_version_set(rx_frame->source_addr,rx_frame->rx_data[5],rx_frame->rx_data[6]);
        radio_mainunit_request_sync(rx_frame->source_addr);
        break;
    case 1://master heart
        device_warning_status_parse(rx_frame->source_addr,rx_frame->rx_data[3]);
        wifi_mainunit_valve_upload(rx_frame->source_addr,rx_frame->rx_data[4]);//主控开关阀
        aq_device_version_set(rx_frame->source_addr,rx_frame->rx_data[5],rx_frame->rx_data[6]);
        radio_mainunit_response_heart(rx_frame->source_addr);
        break;
    case 2://door heart
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_rssi = rx_frame->rx_data[7];
        sub_bat = rx_frame->rx_data[8];
        wifi_doorunit_rssi_upload(slaver_addr,sub_rssi);
        wifi_doorunit_bat_upload(slaver_addr,sub_bat);
        wifi_device_heart_upload(slaver_addr,1);
        break;
    case 3://motion sensor heart
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_rssi = rx_frame->rx_data[7];
        wifi_device_heart_upload(slaver_addr,1);
        wifi_motion_sensor_rssi_upload(slaver_addr,sub_rssi);
        wifi_motion_sensor_config_upload(slaver_addr,rx_frame->rx_data[8],rx_frame->rx_data[9],rx_frame->rx_data[10]);
        break;
    default:
        break;
    }
}

static void radio_frame_mainunit_parse_sync(rx_format *rx_frame)
{
    uint8_t sub_rssi = 0;
    uint8_t sub_type = 0;
    uint8_t sub_command = 0;
    uint8_t sub_alive = 0;
    uint8_t sub_bat = 0;
    uint32_t slaver_addr = 0;
    tx_format tx_frame = {0};
    aqualarm_device_t *device = RT_NULL;

    if((rx_frame->dest_addr != get_local_address()) || (aq_device_find(rx_frame->source_addr) == RT_NULL))
    {
        return;
    }

    if(rx_frame->msg_type == MSG_CONFIRMED_UPLINK)
    {
        tx_frame.msg_ack = RT_TRUE;
        tx_frame.msg_type = MSG_UNCONFIRMED_DOWNLINK;
        tx_frame.dest_addr = rx_frame->source_addr;
        tx_frame.source_addr = get_local_address();
        tx_frame.command = CONFIRM_ACK_CMD;
        radio_mainunit_command_send(&tx_frame);
    }

    sub_command = rx_frame->rx_data[2];
    switch(sub_command)
    {
    case 0://add
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_rssi = rx_frame->rx_data[7];
        sub_type = rx_frame->rx_data[8];
        device = aq_device_create(0,0,sub_rssi,0,sub_type,slaver_addr,rx_frame->source_addr);
        if(device != RT_NULL)
        {
            wifi_device_add(slaver_addr);
            wifi_device_heart_upload(slaver_addr,1);
            wifi_device_rssi_upload(slaver_addr,sub_rssi);
        }
        break;
    case 1://del
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        wifi_device_delete(slaver_addr);
        aq_device_delete(slaver_addr);
        break;
    case 2://reset
        aq_device_print();
        aq_bind_delete(rx_frame->source_addr);
        aq_device_delete(rx_frame->source_addr);
        break;
    case 3://sync   length + ID_0-3 + type + alive + rssi + bat
        wifi_device_add(rx_frame->source_addr);
        device_warning_status_parse(rx_frame->source_addr,rx_frame->rx_data[3]);
        wifi_mainunit_valve_upload(rx_frame->source_addr,rx_frame->rx_data[4]);//主控开关阀
        for(uint8_t i = 0; i < rx_frame->rx_data[5]; i++)
        {
            slaver_addr = rx_frame->rx_data[(i * 8) + 6]<<24 | rx_frame->rx_data[(i * 8) + 7]<<16 \
                    | rx_frame->rx_data[(i * 8) + 8]<<8 | rx_frame->rx_data[(i * 8) + 9];
            sub_type = rx_frame->rx_data[(i * 8) + 10];
            sub_alive = rx_frame->rx_data[(i * 8) + 11];
            sub_rssi = rx_frame->rx_data[(i * 8) + 12];
            sub_bat = rx_frame->rx_data[(i * 8) + 13];

            device = aq_device_create(0,0,sub_rssi,sub_bat,sub_type,slaver_addr,rx_frame->source_addr);
            if(device != RT_NULL)
            {
                wifi_device_add(slaver_addr);
                wifi_device_heart_upload(slaver_addr,sub_alive);
                wifi_device_rssi_upload(slaver_addr,sub_rssi);
                wifi_device_bat_upload(slaver_addr,sub_bat);
            }
        }
        break;
    default:
        break;
    }
}
static void radio_frame_mainunit_parse_warn(rx_format *rx_frame)
{
    uint8_t sub_value = 0;
    uint8_t sub_status = 0;
    uint8_t sub_rssi = 0;
    uint8_t sub_command = 0;
    uint32_t slaver_addr = 0;
    tx_format tx_frame = {0};

    if((rx_frame->dest_addr != get_local_address()) || (aq_device_find(rx_frame->source_addr) == RT_NULL))
    {
        return;
    }

    if(rx_frame->msg_type == MSG_CONFIRMED_UPLINK)
    {
        tx_frame.msg_ack = RT_TRUE;
        tx_frame.msg_type = MSG_UNCONFIRMED_DOWNLINK;
        tx_frame.dest_addr = rx_frame->source_addr;
        tx_frame.source_addr = get_local_address();
        tx_frame.command = CONFIRM_ACK_CMD;
        radio_mainunit_command_send(&tx_frame);
    }

    sub_command = rx_frame->rx_data[2];
    switch(sub_command)
    {
    case 0:
        break;
    case 1://master leak
        sub_value = rx_frame->rx_data[3];
        wifi_mainunit_valve_upload(rx_frame->source_addr,0);//主控开关阀
        wifi_mainunit_upload_warning(rx_frame->source_addr,1,sub_value);
        break;
    case 2://master lost
        sub_value = rx_frame->rx_data[3];
        wifi_mainunit_upload_warning(rx_frame->source_addr,2,sub_value);
        break;
    case 3://master valve fail
        sub_value = rx_frame->rx_data[3];
        wifi_mainunit_upload_warning(rx_frame->source_addr,0,sub_value);
        break;
    case 4://master low temp
        sub_value = rx_frame->rx_data[3];
        wifi_mainunit_upload_warning(rx_frame->source_addr,3,sub_value);
        break;
    case 5://slaver offline
        for(uint8_t i = 0; i < rx_frame->rx_data[3]; i++)
        {
            slaver_addr = rx_frame->rx_data[(i * 4) + 4] << 24 | rx_frame->rx_data[(i * 4) + 5] << 16 \
                    | rx_frame->rx_data[(i * 4) + 6] << 8 | rx_frame->rx_data[(i * 4) + 7];
            wifi_device_heart_upload(slaver_addr,0);
        }
        break;
    case 6://slaver heart
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_value = rx_frame->rx_data[7];
        sub_status = rx_frame->rx_data[8];
        sub_rssi = rx_frame->rx_data[9];
        wifi_device_heart_upload(slaver_addr,1);
        wifi_device_rssi_upload(slaver_addr,sub_rssi);
        wifi_endunit_upload_warning(slaver_addr,2,sub_value);
        if(sub_status == 0)
        {
            wifi_endunit_warning_reset(slaver_addr);
        }
        break;
    case 7://slaver leak
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_value = rx_frame->rx_data[7];
        sub_rssi = rx_frame->rx_data[8];
        wifi_mainunit_valve_upload(rx_frame->source_addr,0);//主控开关阀
        wifi_device_heart_upload(slaver_addr,1);
        wifi_device_rssi_upload(slaver_addr,sub_rssi);
        wifi_endunit_upload_warning(slaver_addr,1,sub_value);
        break;
    case 8://slaver lost
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_value = rx_frame->rx_data[7];
        sub_rssi = rx_frame->rx_data[8];
        wifi_device_heart_upload(slaver_addr,1);
        wifi_device_rssi_upload(slaver_addr,sub_rssi);
        wifi_endunit_upload_warning(slaver_addr,3,sub_value);
        break;
    default:
        break;
    }
}

static void radio_frame_mainunit_parse_control(rx_format *rx_frame)
{
    uint8_t sub_value = 0;
    uint8_t sub_rssi = 0;
    uint8_t sub_command = 0;
    uint32_t slaver_addr = 0;
    tx_format tx_frame = {0};

    if((rx_frame->dest_addr != get_local_address()) || (aq_device_find(rx_frame->source_addr) == RT_NULL))
    {
        return;
    }

    if(rx_frame->msg_type == MSG_CONFIRMED_UPLINK)
    {
        tx_frame.msg_ack = RT_TRUE;
        tx_frame.msg_type = MSG_UNCONFIRMED_DOWNLINK;
        tx_frame.dest_addr = rx_frame->source_addr;
        tx_frame.source_addr = get_local_address();
        tx_frame.command = CONFIRM_ACK_CMD;
        radio_mainunit_command_send(&tx_frame);
    }

    sub_command = rx_frame->rx_data[2];
    switch(sub_command)
    {
    case 0://master control
        sub_value = rx_frame->rx_data[3];
        wifi_mainunit_valve_upload(rx_frame->source_addr,sub_value);//主控开关阀
        break;
    case 1://slaver control
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_value = rx_frame->rx_data[7];
        sub_rssi = rx_frame->rx_data[8];
        wifi_device_heart_upload(slaver_addr,1);
        wifi_device_rssi_upload(slaver_addr,sub_rssi);
        wifi_endunit_control_upload(slaver_addr,sub_value);
        wifi_mainunit_valve_upload(rx_frame->source_addr,sub_value);//主控开关阀
        break;
    case 2://door control
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_value = rx_frame->rx_data[7];
        sub_rssi = rx_frame->rx_data[8];
        wifi_device_heart_upload(slaver_addr,1);
        wifi_device_rssi_upload(slaver_addr,sub_rssi);
        wifi_doorunit_control_upload(slaver_addr,sub_value);
        wifi_mainunit_valve_upload(rx_frame->source_addr,sub_value);//主控开关阀
        break;
    case 3://door delay control
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_value = rx_frame->rx_data[7];
        sub_rssi = rx_frame->rx_data[8];
        wifi_device_heart_upload(slaver_addr,1);
        wifi_device_rssi_upload(slaver_addr,sub_rssi);
        wifi_doorunit_delay_upload(slaver_addr,sub_value);
        break;
    case 4://motion sensor control
        slaver_addr = rx_frame->rx_data[3]<<24 | rx_frame->rx_data[4]<<16 | rx_frame->rx_data[5]<<8 | rx_frame->rx_data[6];
        sub_value = rx_frame->rx_data[7];
        sub_rssi = rx_frame->rx_data[8];
        wifi_device_heart_upload(slaver_addr,1);
        wifi_device_rssi_upload(slaver_addr,sub_rssi);
        wifi_motion_sensor_control_upload(slaver_addr,sub_value);
        wifi_mainunit_valve_upload(rx_frame->source_addr,sub_value);//主控开关阀
        wifi_motion_sensor_config_upload(slaver_addr,rx_frame->rx_data[9],rx_frame->rx_data[10],rx_frame->rx_data[11]);
        break;
    default:
        break;
    }
}

static void radio_frame_mainunit_parse_learn(rx_format *rx_frame)
{
    uint8_t send_value,main_ver,sub_ver = 0;
    tx_format tx_frame = {0};
    aqualarm_device_t *device = RT_NULL;

    if(rx_frame->dest_addr == get_local_address())
    {
        uint8_t sub_command = rx_frame->rx_data[2];
        switch(sub_command)
        {
        case 0://learn ack
            send_value = 1;
            tx_frame.msg_ack = RT_TRUE;
            tx_frame.msg_type = MSG_CONFIRMED_UPLINK;
            tx_frame.dest_addr = rx_frame->source_addr;
            tx_frame.source_addr = get_local_address();
            tx_frame.command = LEARN_DEVICE_CMD;
            tx_frame.tx_data = &send_value;
            tx_frame.tx_len = 1;
            radio_mainunit_command_send(&tx_frame);

            LOG_I("radio_frame_mainunit_parse_learn request %d\r\n",rx_frame->source_addr);
            break;
        case 1://learn done
            main_ver = rx_frame->rx_data[3];
            sub_ver = rx_frame->rx_data[4];
            device = aq_device_create(main_ver,sub_ver,rx_frame->rssi,0,rx_frame->device_type,\
                                        rx_frame->source_addr,0);
            if(device != RT_NULL)
            {
                wifi_device_add(rx_frame->source_addr);
                beep_learn_success();
                LOG_I("radio_frame_mainunit_parse_learn done %d\r\n",rx_frame->source_addr);
            }
            else
            {
                beep_learn_fail();
                LOG_E("radio_frame_mainunit_parse_learn failed %d\r\n",rx_frame->source_addr);
            }
            break;
        default:
            break;
        }
    }
}

static void radio_frame_mainunit_parse_ota(rx_format *rx_frame)
{
    if((rx_frame->dest_addr != get_local_address()) || (aq_device_find(rx_frame->source_addr) == RT_NULL))
    {
        return;
    }

    unsigned short length = 0;
    uint8_t sub_command = rx_frame->rx_data[2];
    switch(sub_command)
    {
    case 1:
        length = set_wifi_uart_byte(length,SUB_PACKAGE_SIZE);
        wifi_uart_write_frame(SUBDEV_START_UPGRADE_CMD, MCU_TX_VER, length);
        break;
    case 2:
        lora_ota_response();
        break;
    case 3:
        lora_ota_response();
        break;
    }
}

void radio_frame_mainunit_parse(rx_format *rx_frame)
{
    if((rx_frame->rx_data[0] == LEARN_DEVICE_CMD) && (rx_frame->dest_addr == get_local_address()))//learn device ignore source address check
    {
        radio_frame_mainunit_parse_learn(rx_frame);
    }

    if((rx_frame->dest_addr != get_local_address()) || (aq_device_find(rx_frame->source_addr) == RT_NULL))
    {
        return;
    }

    switch(rx_frame->rx_data[0])
    {
    case HEART_UPLOAD_CMD:
        radio_frame_mainunit_parse_heart(rx_frame);
        break;
    case DEVICE_SYNC_CMD:
        radio_frame_mainunit_parse_sync(rx_frame);
        break;
    case WARNING_UPLOAD_CMD:
        radio_frame_mainunit_parse_warn(rx_frame);
        break;
    case CONTROL_VALVE_CMD:
        radio_frame_mainunit_parse_control(rx_frame);
        break;
   case FIRMWARE_UPDATE_CMD:
        radio_frame_mainunit_parse_ota(rx_frame);
        break;
    default:
        break;
    }

    rf_led(3);
    aqualarm_device_heart(rx_frame);
    wifi_mainunit_rssi_upload(rx_frame->source_addr,rx_frame->rssi);
}

void radio_mainunit_request_learn(void)
{
    uint32_t dest_addr = 0xFFFFFFFF;
    tx_format tx_frame = {0};

    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = dest_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = LEARN_DEVICE_CMD;
    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_request_sync(uint32_t device_addr)
{
    uint8_t send_buf[4] = {0};
    send_buf[0] = 0;//sync request
    send_buf[1] = aq_device_bind_count(device_addr);

    tx_format tx_frame = {0};
    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = device_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = DEVICE_SYNC_CMD;
    tx_frame.tx_data = send_buf;
    tx_frame.tx_len = 2;
    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_response_heart(uint32_t device_addr)
{
    uint32_t send_value = 0;
    tx_format tx_frame = {0};
    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = device_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = HEART_UPLOAD_CMD;
    tx_frame.tx_data = &send_value;
    tx_frame.tx_len = 1;
    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_remote_delete(uint32_t device_addr)
{
    aqualarm_device_t *device = aq_device_find(device_addr);
    if(device == RT_NULL)
    {
        return;
    }

    tx_format tx_frame = {0};
    uint8_t send_buf[8] = {0};

    send_buf[0] = 1;//delete
    send_buf[1] = (device_addr>>24) & 0xFF;
    send_buf[2] = (device_addr>>16) & 0xFF;
    send_buf[3] = (device_addr>>8) & 0xFF;
    send_buf[4] = device_addr & 0xFF;

    tx_frame.msg_ack = RT_FALSE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    if(device->bind_id == 0)
    {
        tx_frame.dest_addr = device_addr;
    }
    else
    {
        tx_frame.dest_addr = device->bind_id;
    }
    tx_frame.source_addr = get_local_address();
    tx_frame.command = DEVICE_SYNC_CMD;
    tx_frame.tx_data = send_buf;
    tx_frame.tx_len = 5;
    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_remote_control(uint32_t device_addr,uint8_t value)
{
    tx_format tx_frame = {0};
    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = device_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = CONTROL_VALVE_CMD;
    tx_frame.tx_data = &value;
    tx_frame.tx_len = 1;
    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_ota_send(uint32_t device_addr,uint8_t *firmware,uint8_t length)
{
    tx_format tx_frame = {0};
    tx_frame.msg_ack = RT_TRUE;
    tx_frame.msg_type = MSG_CONFIRMED_DOWNLINK;
    tx_frame.dest_addr = device_addr;
    tx_frame.source_addr = get_local_address();
    tx_frame.command = FIRMWARE_UPDATE_CMD;
    tx_frame.tx_data = firmware;
    tx_frame.tx_len = length;
    radio_mainunit_command_send(&tx_frame);
}

void radio_mainunit_command_send(tx_format *tx_frame)
{
    unsigned short send_len = 0;

    send_len = set_lora_tx_byte(send_len,0xEF);
    send_len = set_lora_tx_byte(send_len,(NETID_TEST_ENV << 4) | NETWORK_VERSION);
    send_len = set_lora_tx_byte(send_len,(tx_frame->msg_ack << 7) | (DEVICE_TYPE_GATEWAY << 3) | tx_frame->msg_type);
    send_len = set_lora_tx_word(send_len,tx_frame->dest_addr);
    send_len = set_lora_tx_word(send_len,tx_frame->source_addr);
    send_len = set_lora_tx_byte(send_len,tx_frame->command);
    send_len = set_lora_tx_byte(send_len,tx_frame->tx_len);
    send_len = set_lora_tx_buffer(send_len,tx_frame->tx_data,tx_frame->tx_len);
    send_len = set_lora_tx_crc(send_len);
    lora_tx_enqueue(get_lora_tx_buf(),send_len,tx_frame->parameter);
}
