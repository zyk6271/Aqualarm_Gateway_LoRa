#include <agile_led.h>
#include <stdlib.h>
#include "led.h"
#include "pin_config.h"
#include <agile_led.h>

static agile_led_t *led_obj_rf_green = RT_NULL;
static agile_led_t *led_obj_rf_red = RT_NULL;
static agile_led_t *led_obj_wifi_red = RT_NULL;
static agile_led_t *led_obj_wifi_blue = RT_NULL;
static agile_led_t *led_obj_beep = RT_NULL;

#define DBG_TAG "LED"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

void led_init(void)
{
    led_obj_rf_green = agile_led_create(LED1_PIN, PIN_LOW, "200,200", -1);
    led_obj_rf_red = agile_led_create(LED2_PIN, PIN_LOW, "200,200", -1);
    led_obj_wifi_red = agile_led_create(LED3_PIN, PIN_LOW, "200,200", -1);
    led_obj_wifi_blue = agile_led_create(LED4_PIN, PIN_LOW, "200,200", -1);
    led_obj_beep = agile_led_create(BUZZER_PIN, PIN_HIGH, "200,200", -1);

    agile_led_set_light_mode(led_obj_beep, "500,1", 1);
    agile_led_start(led_obj_beep);
}

void led_beep_start(uint8_t count)
{
    agile_led_stop(led_obj_beep);
    agile_led_set_light_mode(led_obj_beep, "200,200", count);
    agile_led_start(led_obj_beep);
}

void led_beep_power(uint8_t count)
{
    agile_led_stop(led_obj_beep);
    agile_led_set_light_mode(led_obj_beep, "400,400", count);
    agile_led_start(led_obj_beep);
}

void wifi_led(uint8_t type)
{
    switch(type)
    {
    case 0://启动失败
        agile_led_stop(led_obj_wifi_red);
        agile_led_stop(led_obj_wifi_blue);
        break;
    case 1://AP慢闪
        agile_led_stop(led_obj_wifi_blue);
        agile_led_set_light_mode(led_obj_wifi_red, "1000,1000", -1);
        agile_led_start(led_obj_wifi_red);
        break;
    case 2://已配置未连接路由器
        agile_led_stop(led_obj_wifi_blue);
        agile_led_set_light_mode(led_obj_wifi_red, "150,150", -1);
        agile_led_start(led_obj_wifi_red);
        break;
    case 3://已连接路由器未连接互联网
        agile_led_stop(led_obj_wifi_red);
        agile_led_set_light_mode(led_obj_wifi_blue, "150,150", -1);
        agile_led_start(led_obj_wifi_blue);
        break;
    case 4://已连接互联网
        agile_led_stop(led_obj_wifi_red);
        agile_led_set_light_mode(led_obj_wifi_blue, "200,0", -1);
        agile_led_start(led_obj_wifi_blue);
        break;
    default:
        break;
    }
}

void wifi_led_factory(uint8_t type)
{
    switch(type)
    {
    case 0://关闭全部
        agile_led_stop(led_obj_wifi_red);
        agile_led_stop(led_obj_wifi_blue);
        break;
    case 1://wifi搜索失败
        agile_led_stop(led_obj_wifi_blue);
        agile_led_set_light_mode(led_obj_wifi_red, "200,0", -1);
        agile_led_start(led_obj_wifi_red);
        break;
    case 2://RSSI异常
        agile_led_set_light_mode(led_obj_wifi_blue, "200,200,0,200", -1);
        agile_led_start(led_obj_wifi_blue);
        agile_led_set_light_mode(led_obj_wifi_red, "0,200,200,200", -1);
        agile_led_start(led_obj_wifi_red);
        break;
    case 3://RSSI正常
        agile_led_stop(led_obj_wifi_red);
        agile_led_set_light_mode(led_obj_wifi_blue, "200,0", -1);
        agile_led_start(led_obj_wifi_blue);
        break;
    default:
        break;
    }
}

void rf_led_resume(agile_led_t *led)
{
    agile_led_set_compelete_callback(led_obj_rf_green,RT_NULL);
    agile_led_set_light_mode(led_obj_rf_green, "200,0", -1);
    agile_led_start(led_obj_rf_green);
}

void rf_led(uint8_t type)
{
    switch(type)
    {
    case 0://RF初始化失败
        agile_led_stop(led_obj_rf_green);
        agile_led_set_light_mode(led_obj_rf_red, "200,0", -1);
        agile_led_start(led_obj_rf_red);
        break;
    case 1://RF初始化成功
        agile_led_stop(led_obj_rf_red);
        agile_led_set_light_mode(led_obj_rf_green, "200,0", -1);
        agile_led_start(led_obj_rf_green);
        break;
    case 2://RF发送
        agile_led_stop(led_obj_rf_red);
        agile_led_set_light_mode(led_obj_rf_green, "10,100",1);
        agile_led_set_compelete_callback(led_obj_rf_green,rf_led_resume);
        agile_led_start(led_obj_rf_green);
        break;
    case 3://RF接收
        agile_led_stop(led_obj_rf_red);
        agile_led_set_light_mode(led_obj_rf_green, "10,100",1);
        agile_led_set_compelete_callback(led_obj_rf_green,rf_led_resume);
        agile_led_start(led_obj_rf_green);
        break;
    }
}

void rf_led_factory(uint8_t type)
{
    switch(type)
    {
    case 0://产测无应答
        agile_led_stop(led_obj_rf_green);
        agile_led_set_light_mode(led_obj_rf_red, "200,0", -1);
        agile_led_start(led_obj_rf_red);
        break;
    case 1://RSSI过小
        agile_led_stop(led_obj_rf_red);
        agile_led_set_light_mode(led_obj_rf_green, "200,0", -1);
        agile_led_start(led_obj_rf_green);
        break;
    case 2://RSSI正常
        agile_led_stop(led_obj_rf_red);
        agile_led_set_light_mode(led_obj_rf_green, "10,100",1);
        agile_led_set_compelete_callback(led_obj_rf_green,rf_led_resume);
        agile_led_start(led_obj_rf_green);
        break;
    }
}

void beep_wifi_reset_success(void)
{
    agile_led_stop(led_obj_beep);
    agile_led_set_light_mode(led_obj_beep, "200,200", 3);
    agile_led_start(led_obj_beep);
}

void beep_wifi_reset_fail(void)
{
    agile_led_set_light_mode(led_obj_beep, "50,50,300,300,", 3);
    agile_led_start(led_obj_beep);
}

void beep_learn_success(void)
{
    agile_led_set_light_mode(led_obj_beep, "200,200", 5);
    agile_led_start(led_obj_beep);
}

void beep_learn_fail(void)
{
    agile_led_set_light_mode(led_obj_beep, "50,50,200,200", 3);
    agile_led_start(led_obj_beep);
}

void beep_led_test(void)
{
    agile_led_set_light_mode(led_obj_rf_red, "0,600,1200,600", 1);
    agile_led_set_light_mode(led_obj_rf_green, "600,600,600,600", 1);
    agile_led_set_light_mode(led_obj_wifi_red, "0,600,1200,600", 1);
    agile_led_set_light_mode(led_obj_wifi_blue, "600,600,600,600", 1);
    agile_led_start(led_obj_rf_red);
    agile_led_start(led_obj_rf_green);
    agile_led_start(led_obj_wifi_red);
    agile_led_start(led_obj_wifi_blue);
}
