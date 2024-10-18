/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-25     Rick       the first version
 */
#include <rtthread.h>
#include <rthw.h>
#include "pin_config.h"
#include "key.h"
#include "led.h"
#include "Radio_encoder.h"
#include "flashwork.h"
#include "wifi-service.h"
#include "agile_button.h"

#define DBG_TAG "key"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

agile_btn_t *key_on_btn = RT_NULL;
agile_btn_t *key_off_btn = RT_NULL;

static uint32_t key_on_count,key_off_count,key_on_off_flag = 0;

void key_off_long_hold_handle(void)
{
    if(key_off_count++ < 4)
    {
        LOG_D("key_off_long_hold_handle %d\r\n",key_off_count);
    }
    else
    {
        if(key_on_count)
        {
            key_on_off_long_click_handle();
        }
        else
        {
            if(key_off_count == 5)
            {
                if(get_mainunit_learn_valid())
                {
                    radio_mainunit_request_learn();
                }
                else
                {
                    beep_learn_fail();
                }
            }
        }
    }
}

void key_off_long_free_handle(void)
{
    key_on_off_flag = 0;
    key_off_count = 0;
}

void key_on_long_hold_handle(void)
{
    if(key_on_count++ < 4)
    {
        LOG_D("key_on_long_hold_handle %d\r\n",key_on_count);
    }
    else
    {
        if(key_off_count)
        {
            key_on_off_long_click_handle();
        }
        else
        {
            if(key_on_count == 5)
            {
                wifi_network_reset();
            }
        }
    }
}

void key_on_long_free_handle(void)
{
    key_on_off_flag = 0;
    key_on_count = 0;
}

void key_on_off_long_click_handle(void)
{
    if(key_on_count > 3 && key_off_count > 3)
    {
        if(key_on_off_flag == 0)
        {
            LOG_D("key_on_off_long_click_handle\r\n");
            key_on_off_flag = 1;
            led_beep_start(5);
            reset_factory_setting();
            wifi_factory_reset_timer_start();
        }
    }
    else
    {
        LOG_D("key_on_off_long_click_handle %d %d\r\n",key_off_count,key_on_count);
    }
}

uint8_t factory_button_detect(void)
{
    rt_pin_mode(KEY_ON_PIN, PIN_MODE_INPUT);
    rt_pin_mode(KEY_OFF_PIN, PIN_MODE_INPUT);
    if(rt_pin_read(KEY_ON_PIN) == 0 && rt_pin_read(KEY_OFF_PIN) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void factory_key_callback(void *parameter)
{
    led_beep_kick();
}

void factory_button_init(void)
{
    key_on_btn = agile_btn_create(KEY_ON_PIN, PIN_LOW, PIN_MODE_INPUT);
    key_off_btn = agile_btn_create(KEY_OFF_PIN, PIN_LOW, PIN_MODE_INPUT);
    agile_btn_set_event_cb(key_on_btn, BTN_PRESS_DOWN_EVENT, factory_key_callback);
    agile_btn_set_event_cb(key_off_btn, BTN_PRESS_DOWN_EVENT, factory_key_callback);
    agile_btn_start(key_on_btn);
    agile_btn_start(key_off_btn);
}

void button_init(void)
{
    key_on_btn = agile_btn_create(KEY_ON_PIN, PIN_LOW, PIN_MODE_INPUT);
    key_off_btn = agile_btn_create(KEY_OFF_PIN, PIN_LOW, PIN_MODE_INPUT);

    agile_btn_set_event_cb(key_on_btn, BTN_HOLD_EVENT, key_on_long_hold_handle);
    agile_btn_set_event_cb(key_on_btn, BTN_HOLD_FREE_EVENT, key_on_long_free_handle);
    agile_btn_set_event_cb(key_off_btn, BTN_HOLD_EVENT, key_off_long_hold_handle);
    agile_btn_set_event_cb(key_off_btn, BTN_HOLD_FREE_EVENT, key_off_long_free_handle);

    agile_btn_start(key_on_btn);
    agile_btn_start(key_off_btn);
}
