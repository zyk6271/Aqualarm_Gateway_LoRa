/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-08     RT-Thread    first version
 */

#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void)
{
    storage_init();
    led_init();
    factory_detect();
    button_init();
    radio_init();
    heart_init();
    wifi_init();
    rtc_init();
    while (1)
    {
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
