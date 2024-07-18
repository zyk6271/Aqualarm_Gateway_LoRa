/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-27     Rick       the first version
 */
#include "rtthread.h"
#define DBG_TAG "factory"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

uint8_t rf_factoryflag;

void factory_detect(void)
{
    if(factory_button_detect())
    {
        beep_led_test();
        factory_button_init();
        radio_init();
        WiFi_FactoryInit();
        while(1)
        {
            rt_thread_mdelay(1000);
        }
    }
}

void rf_clear(void)
{
    rf_factoryflag = 1;
}

void rf_refresh(void)
{
    rf_factoryflag = 0;
}

void factory_refresh(void)
{
    if(rf_factoryflag == 1)
    {
        rf_led_factory(0);
    }
    radio_mainunit_factory_send();
    rf_factoryflag = 1;
    mcu_start_wifitest();
}
