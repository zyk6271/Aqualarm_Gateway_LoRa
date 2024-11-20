/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-22     Rick       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include "radio_encoder.h"
#include "protocol.h"
#include "radio_app.h"

#define DBG_TAG "RADIO_ENCODER"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static rt_mq_t rf_en_mq;
static rt_thread_t rf_encode_t = RT_NULL;
static struct rt_completion rf_txdone_sem;

uint32_t local_address = 0;

#define MAX_LBT_RETRY_TIMES     3

uint32_t get_local_address(void)
{
    return local_address;
}

void lora_tx_enqueue(char *data,uint8_t length,uint8_t parameter)
{
    struct send_msg msg_ptr = {0};
    rt_memcpy(&msg_ptr.data_ptr,data,length < 253 ? length : 253);
    msg_ptr.data_size = length; /* 数据块的长度 */
    msg_ptr.parameter = parameter; /* 数据块的长度 */
    rt_mq_send(rf_en_mq, (void*)&msg_ptr, sizeof(struct send_msg));
}

void rf_txdone_callback(void)
{
    rt_completion_done(&rf_txdone_sem);
}

rt_err_t rf_send_with_lbt(uint8_t freq_type,char* data_ptr,uint8_t data_size)
{
    uint8_t retry_times = 0;
    uint32_t send_freq = RF_TX_FREQUENCY;
    for(retry_times = 0; retry_times < MAX_LBT_RETRY_TIMES; retry_times++)
    {
        if(csma_check_start(send_freq) == RT_EOK)
        {
            break;
        }
        else
        {
            LOG_E("CSMA check retry at start %d",retry_times);
            rt_thread_mdelay(random_second_get(50,80) * 10);//500-800 ms
        }
    }

    if (retry_times >= MAX_LBT_RETRY_TIMES)
    {
       LOG_E("send fail because channel busy\n");
       radio_recv_start();
       return RT_ERROR;
    }
    else
    {
        rt_completion_init(&rf_txdone_sem);
        RF_Send(data_ptr, data_size);
        if(rt_completion_wait(&rf_txdone_sem, 1000) == RT_EOK)
        {
            LOG_D("rf_send_with_lbt send packet success");
            return RT_EOK;
        }
        else
        {
            LOG_E("rf_send_with_lbt send packet failed");
            return RT_ERROR;
        }
    }
}

void rf_encode_entry(void *paramaeter)
{
    struct send_msg msg_ptr; /* 用于放置消息的局部变量 */
    while (1)
    {
        rt_mq_recv(rf_en_mq,(void*)&msg_ptr, sizeof(struct send_msg), RT_WAITING_FOREVER);
        rf_send_with_lbt(msg_ptr.parameter, msg_ptr.data_ptr, msg_ptr.data_size);
    }
}

void RadioQueue_Init(void)
{
    int *p;
    p=(int *)(0x0800BFF0);//这就是已知的地址，要强制类型转换
    local_address = *p;//从Flash加载ID
    if (local_address == 0xFFFFFFFF || local_address == 0)
    {
        local_address = 40000000;
    }
    LOG_I("System Version:%s,local_address:%ld\r\n",MCU_VER,local_address);

    rf_en_mq = rt_mq_create("rf_en_mq", sizeof(struct send_msg), 5, RT_IPC_FLAG_PRIO);
    rf_encode_t = rt_thread_create("radio_send", rf_encode_entry, RT_NULL, 1024, 9, 10);
    rt_thread_startup(rf_encode_t);
}

