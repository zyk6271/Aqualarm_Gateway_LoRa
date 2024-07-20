/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-12-30     Rick       the first version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "wifi-uart.h"
#include "wifi.h"
#include "wifi-service.h"
#include "led.h"

#define DBG_TAG "wifi_service"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

rt_thread_t wifi_service_t = RT_NULL;

rt_timer_t wifi_test_timer = RT_NULL;
rt_timer_t wifi_network_rst_timer = RT_NULL;
rt_timer_t wifi_factory_rst_timer = RT_NULL;

uint8_t wifi_network_rst_cnt = 0;
uint8_t wifi_factory_rst_cnt = 0;

void wifi_network_reset(void)
{
    mcu_reset_wifi();
    wifi_network_rst_cnt = 0;
    rt_timer_start(wifi_network_rst_timer);
}

void wifi_network_rst_timer_callback(void *parameter)
{
    if(wifi_network_rst_cnt < 3 && mcu_get_reset_wifi_flag() != 1)
    {
        wifi_network_rst_cnt++;
        mcu_reset_wifi();
        LOG_D("wifi reset retry %d\r\n",wifi_network_rst_cnt);
    }
    else if(wifi_network_rst_cnt < 3 && mcu_get_reset_wifi_flag() == 1)
    {
        wifi_network_rst_cnt++;
        LOG_I("wifi reset success\r\n");
        rt_timer_stop(wifi_network_rst_timer);
        beep_wifi_reset_success();
    }
    else if(wifi_network_rst_cnt >= 3)
    {
        LOG_E("wifi reset failed\r\n");
        rt_timer_stop(wifi_network_rst_timer);
        beep_wifi_reset_fail();
    }
}

void wifi_factory_reset_timer_start(void)
{
    wifi_factory_rst_cnt = 0;
    rt_timer_start(wifi_factory_rst_timer);
}
void wifi_factory_rst_timer_callback(void *parameter)
{
    if(wifi_factory_rst_cnt < 3)
    {
        wifi_factory_rst_cnt ++;
        reset_factory_setting();
    }
    else
    {
        rt_hw_cpu_reset();
    }
}

void service_callback(void *parameter)
{
    rt_thread_mdelay(200);
    wifi_power_on();
    while(1)
    {
        wifi_uart_service();
        rt_thread_mdelay(5);
    }
}

void wifi_test(void)
{
    rt_timer_start(wifi_test_timer);
    LOG_D("wifi_test start\r\n");
}

void wifi_test_callback(void *parameter)
{
    factory_refresh();
}

void wifi_service_init(void)
{
    lora_ota_timer_init();
    wifi_factory_rst_timer = rt_timer_create("wifi_factory_rst_timer", wifi_factory_rst_timer_callback, RT_NULL, 3000, RT_TIMER_FLAG_SOFT_TIMER|RT_TIMER_FLAG_PERIODIC);
    wifi_network_rst_timer = rt_timer_create("wifi_network_rst_timer", wifi_network_rst_timer_callback, RT_NULL, 2000, RT_TIMER_FLAG_SOFT_TIMER|RT_TIMER_FLAG_PERIODIC);
    wifi_test_timer = rt_timer_create("wifi_test_timer", wifi_test_callback, RT_NULL, 5000, RT_TIMER_FLAG_SOFT_TIMER|RT_TIMER_FLAG_PERIODIC);
    wifi_service_t = rt_thread_create("wifi-service", service_callback, RT_NULL, 2048, 8, 10);
    rt_thread_startup(wifi_service_t);
}
