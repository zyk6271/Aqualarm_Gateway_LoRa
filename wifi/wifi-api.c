/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-17     Rick       the first version
 */
#include "wifi.h"
#include "wifi-api.h"
#include "flashwork.h"
#include "rtdevice.h"
#include "wifi-service.h"
#include "radio_encoder.h"
#include "radio_protocol.h"

#define DBG_TAG "WIFI-API"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static char MAINUNIT_PRODUCT_PID[] = {"cbzww5ywtyczyaau"};
static char ENDUNIT_PRODUCT_PID[] = {"lnbkva6cip8dw7vy"};
static char DOOR_PRODUCT_PID[] = {"emnzq3qxwfplx7db"};

#define REMOTE_MAX_DEVICE   32

uint8_t remote_sync_ready = 0;
uint32_t remote_device_list[REMOTE_MAX_DEVICE] = {0};

void wifi_gateway_addr_upload(uint32_t gateway_id)
{
    uint8_t addr_buf[]={"0000"};
    mcu_dp_value_update(GATEWAY_DPID_SELF_ID,gateway_id,addr_buf,my_strlen(addr_buf));
}

void wifi_mainunit_clear_warning(uint32_t device_id)
{
    unsigned char *from_id_buf = rt_malloc(16);
    rt_sprintf(from_id_buf,"%ld",device_id);
    mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_ALARM,0,from_id_buf,my_strlen(from_id_buf)); //BOOL型数据上报;
    rt_free(from_id_buf);
}

void wifi_mainunit_reset_warning(uint32_t device_id)
{
    unsigned char *from_id_buf = rt_malloc(16);
    rt_sprintf(from_id_buf,"%ld",device_id);
    mcu_dp_bool_update(MAINUNIT_DPID_SELF_ID,device_id,from_id_buf,my_strlen(from_id_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_ALARM,0,from_id_buf,my_strlen(from_id_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_VALVE1_CHECK_FAIL,0,from_id_buf,my_strlen(from_id_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_VALVE2_CHECK_FAIL,0,from_id_buf,my_strlen(from_id_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_TEMP_STATE,0,from_id_buf,my_strlen(from_id_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_LINE_STATE,0,from_id_buf,my_strlen(from_id_buf)); //BOOL型数据上报;
    rt_free(from_id_buf);
}

void wifi_mainunit_upload_warning(uint32_t device_id,uint8_t type,uint8_t value)
{
    unsigned char *device_addr_buf = rt_malloc(16);
    rt_sprintf(device_addr_buf,"%ld",device_id);
    switch(type)
    {
        case 0://自检
            if(value == 1)//内部阀门自检正常
            {
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE1_CHECK_FAIL,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE1_CHECK_SUCCESS,1,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
            }
            else if(value == 2)//外部阀门自检正常
            {
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE2_CHECK_FAIL,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE2_CHECK_SUCCESS,1,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
            }
            else if(value == 3)//内部阀门自检失败
            {
                mcu_dp_bool_update(MAINUNIT_DPID_TEMP_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_LINE_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE1_CHECK_FAIL,1,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
            }
            else if(value == 4)//外部阀门自检失败
            {
                mcu_dp_bool_update(MAINUNIT_DPID_TEMP_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_LINE_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE2_CHECK_FAIL,1,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
            }
           break;
        case 1://漏水
            if(value)
            {
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE1_CHECK_FAIL,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_VALVE2_CHECK_FAIL,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_TEMP_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_LINE_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
                mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_STATE,0,device_addr_buf,my_strlen(device_addr_buf));
                mcu_dp_bool_update(MAINUNIT_DPID_DELAY_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //VALUE型数据上报;
            }
            mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_ALARM,value,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
           break;
        case 2://掉落
            mcu_dp_bool_update(MAINUNIT_DPID_LINE_STATE,value,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
           break;
        case 3://NTC
            if(value)
            {
                mcu_dp_bool_update(MAINUNIT_DPID_LINE_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
            }
            mcu_dp_bool_update(MAINUNIT_DPID_TEMP_STATE,value,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
           break;
        default:
            break;
    }
    rt_free(device_addr_buf);
}

void wifi_mainunit_delete_device(uint32_t device_id)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    unsigned char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device->bind_id);
    radio_mainunit_remote_delete(device->device_id);
    if(device->type == DEVICE_TYPE_DOORUNIT)
    {
        mcu_dp_value_update(MAINUNIT_DPID_DOOR_ID,0,addr_buf,my_strlen(addr_buf));
    }
    aq_device_delete(device->device_id);
    rt_free(addr_buf);
}

void wifi_mainunit_valve_upload(uint32_t device_id,uint8_t state)
{
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    if(state == 0)
    {
        mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_ALARM,0,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
        mcu_dp_bool_update(MAINUNIT_DPID_DELAY_STATE,0,addr_buf,my_strlen(addr_buf)); //VALUE型数据上报;
    }
    mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_STATE,state,addr_buf,my_strlen(addr_buf));
    aq_device_valve_set(device_id,state);
    rt_free(addr_buf);
}

void wifi_mainunit_add_device(uint32_t device_id,char *ver_str)
{
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    local_add_subdev_limit(1,0,0x01);
    gateway_subdevice_add(ver_str,MAINUNIT_PRODUCT_PID,0,addr_buf,10,1);
    rt_free(addr_buf);
}

void wifi_mainunit_info_upload(uint32_t device_id)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_value_update(MAINUNIT_DPID_SELF_ID,device_id,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_STATE,device->valve,addr_buf,my_strlen(addr_buf)); //VALUE型数据上报;
    mcu_dp_enum_update(MAINUNIT_DPID_SIGN_STATE,device->rssi,addr_buf,my_strlen(addr_buf)); //VALUE型数据上报;
    mcu_dp_value_update(MAINUNIT_DPID_DOOR_ID,aq_device_doorunit_find(device_id),addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    rt_free(addr_buf);
}

void wifi_mainunit_warning_reset(uint32_t device_id)
{
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_bool_update(MAINUNIT_DPID_VALVE1_CHECK_FAIL,0,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_VALVE2_CHECK_FAIL,0,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_DEVICE_ALARM,0,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_LINE_STATE,0,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(MAINUNIT_DPID_TEMP_STATE,0,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    rt_free(addr_buf);
}

void wifi_mainunit_rssi_upload(uint32_t device_id,int rssi)
{
    uint8_t level = 0;
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    if(rssi < -100)
    {
        level = 0;
    }
    else if(rssi >= -100 && rssi < -90)
    {
        level = 1;
    }
    else if(rssi >= -90)
    {
        level = 2;
    }
    mcu_dp_enum_update(MAINUNIT_DPID_SIGN_STATE,level,addr_buf,my_strlen(addr_buf));
    rt_free(addr_buf);
}

void wifi_endunit_add_device(uint32_t device_id)
{
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    local_add_subdev_limit(1,0,0x01);
    gateway_subdevice_add("1.0",ENDUNIT_PRODUCT_PID,0,addr_buf,10,0);
    rt_free(addr_buf);
}

void wifi_endunit_info_upload(uint32_t device_id,uint32_t from_id)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_value_update(ENDUNIT_DPID_BIND_ID,from_id,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    mcu_dp_enum_update(ENDUNIT_DPID_RSSI,device->rssi,addr_buf,my_strlen(addr_buf)); //VALUE型数据上报;
    rt_free(addr_buf);
}

void wifi_endunit_warning_reset(uint32_t device_id)
{
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_enum_update(ENDUNIT_DPID_WATERSENSOR_STATE,0,addr_buf,my_strlen(addr_buf)); //BOOL型数据上报;
    mcu_dp_bool_update(ENDUNIT_DPID_LINE_STATE,0,addr_buf,my_strlen(addr_buf)); //VALUE型数据上报;
    rt_free(addr_buf);
}

void wifi_endunit_upload_warning(uint32_t device_id,uint8_t type,uint8_t value)
{
    unsigned char *device_addr_buf = rt_malloc(16);
    rt_sprintf(device_addr_buf,"%ld",device_id);
    switch(type)
    {
        case 0://掉线
           mcu_dp_bool_update(ENDUNIT_DPID_CONTROL_STATE,value,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
           break;
        case 1://漏水
            if(value)
            {
                mcu_dp_bool_update(ENDUNIT_DPID_LINE_STATE,0,device_addr_buf,my_strlen(device_addr_buf)); //VALUE型数据上报;
            }
            mcu_dp_enum_update(ENDUNIT_DPID_WATERSENSOR_STATE,value,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
           break;
        case 2://电量
            mcu_dp_enum_update(ENDUNIT_DPID_BATTERY_STATES,value,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
           break;
        case 3://掉落
            mcu_dp_bool_update(ENDUNIT_DPID_LINE_STATE,value,device_addr_buf,my_strlen(device_addr_buf)); //VALUE型数据上报;
           break;
        default:
            break;
    }
    rt_free(device_addr_buf);
}

void wifi_endunit_close_warning(uint32_t device_id)
{
    unsigned char *from_id_buf = rt_malloc(16);
    rt_sprintf(from_id_buf,"%ld",device_id);
    mcu_dp_enum_update(1,0,from_id_buf,my_strlen(from_id_buf));
    rt_free(from_id_buf);
}

void wifi_endunit_control_upload(uint32_t device_id,uint8_t state)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_bool_update(ENDUNIT_DPID_CONTROL_STATE,state,addr_buf,my_strlen(addr_buf));
    rt_free(addr_buf);
}

void wifi_endunit_rssi_upload(uint32_t device_id,uint8_t rssi)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    device->rssi = rssi;
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_enum_update(ENDUNIT_DPID_RSSI,rssi,addr_buf,my_strlen(addr_buf));
    rt_free(addr_buf);
}

void wifi_endunit_bat_upload(uint32_t device_id,uint8_t bat_level)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    device->battery = bat_level;
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_enum_update(ENDUNIT_DPID_BATTERY_STATES,bat_level,addr_buf,my_strlen(addr_buf));
    rt_free(addr_buf);
}

void wifi_doorunit_add_device(uint32_t device_id)
{
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    local_add_subdev_limit(1,0,0x01);
    gateway_subdevice_add("1.0",DOOR_PRODUCT_PID,0,addr_buf,10,0);
    rt_free(addr_buf);
}

void wifi_doorunit_info_upload(uint32_t device_id,uint32_t bind_id)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    char *bind_addr_buf = rt_malloc(16);
    char *device_addr_buf = rt_malloc(16);
    rt_sprintf(bind_addr_buf,"%ld",bind_id);
    rt_sprintf(device_addr_buf,"%ld",device_id);
    if(device->type == DEVICE_TYPE_DOORUNIT)
    {
        mcu_dp_value_update(DOORUNIT_DPID_BIND_ID,bind_id,device_addr_buf,my_strlen(device_addr_buf)); //BOOL型数据上报;
        mcu_dp_enum_update(DOORUNIT_DPID_RSSI,device->rssi,device_addr_buf,my_strlen(device_addr_buf)); //VALUE型数据上报;
        mcu_dp_value_update(MAINUNIT_DPID_DOOR_ID,device_id,bind_addr_buf,my_strlen(bind_addr_buf)); //BOOL型数据上报;
    }
    rt_free(bind_addr_buf);
    rt_free(device_addr_buf);
}

void wifi_doorunit_delay_upload(uint32_t device_id,uint8_t state)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    char *bind_addr_buf = rt_malloc(16);
    char *device_addr_buf = rt_malloc(16);
    rt_sprintf(bind_addr_buf,"%ld",device->bind_id);
    rt_sprintf(device_addr_buf,"%ld",device_id);
    if(device->type == DEVICE_TYPE_DOORUNIT)
    {
        mcu_dp_bool_update(DOORUNIT_DPID_DELAY_STATE,state,device_addr_buf,my_strlen(device_addr_buf)); //VALUE型数据上报;
        mcu_dp_bool_update(MAINUNIT_DPID_DELAY_STATE,state,bind_addr_buf,my_strlen(bind_addr_buf)); //VALUE型数据上报;
    }
    rt_free(device_addr_buf);
    rt_free(bind_addr_buf);
}

void wifi_doorunit_control_upload(uint32_t device_id,uint8_t state)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    if(device->type == DEVICE_TYPE_DOORUNIT)
    {
        mcu_dp_bool_update(DOORUNIT_DPID_CONTROL_STATE,state,addr_buf,my_strlen(addr_buf));
    }
    rt_free(addr_buf);
}

void wifi_doorunit_rssi_upload(uint32_t device_id,uint8_t rssi)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }
    device->rssi = rssi;
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_enum_update(DOORUNIT_DPID_RSSI,rssi,addr_buf,my_strlen(addr_buf));
    rt_free(addr_buf);
}

void wifi_doorunit_bat_upload(uint32_t device_id,uint8_t bat_level)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    device->battery = bat_level;
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    mcu_dp_enum_update(DOORUNIT_DPID_BATTERY,bat_level,addr_buf,my_strlen(addr_buf));
    rt_free(addr_buf);
}

void wifi_device_rssi_upload(uint32_t device_id,uint8_t rssi)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    switch(device->type)
    {
    case DEVICE_TYPE_MAINUNIT:
    case DEVICE_TYPE_ALLINONE:
        wifi_mainunit_rssi_upload(device_id,rssi);
        break;
    case DEVICE_TYPE_ENDUNIT:
        wifi_endunit_rssi_upload(device_id,rssi);
        break;
    case DEVICE_TYPE_DOORUNIT:
        wifi_doorunit_rssi_upload(device_id,rssi);
        break;
    default:
        break;
    }
}

void wifi_device_bat_upload(uint32_t device_id,uint8_t bat_level)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    switch(device->type)
    {
    case DEVICE_TYPE_MAINUNIT:
    case DEVICE_TYPE_ALLINONE:
        break;
    case DEVICE_TYPE_ENDUNIT:
        wifi_endunit_bat_upload(device_id,bat_level);
        break;
    case DEVICE_TYPE_DOORUNIT:
        wifi_doorunit_bat_upload(device_id,bat_level);
        break;
    default:
        break;
    }
}

uint8_t remote_device_find(uint32_t addr)
{
    for(uint8_t i = 0;i < REMOTE_MAX_DEVICE;i++)
    {
        if(remote_device_list[i] == addr)
        {
            return 1;
        }
    }

    return 0;
}

void remote_device_del(uint32_t addr)
{
    for(uint8_t i = 0;i < REMOTE_MAX_DEVICE;i++)
    {
        if(remote_device_list[i] == addr)
        {
            remote_device_list[i] = 0;
            return;
        }
    }
}

void remote_device_add(uint32_t addr)
{
    if(remote_device_find(addr))
    {
        return;
    }

    for(uint8_t i = 0;i < REMOTE_MAX_DEVICE;i++)
    {
        if(remote_device_list[i] == 0)
        {
            remote_device_list[i] = addr;
            return;
        }
    }
}

void remote_device_sync_finish(void)
{
    for(uint8_t i = 0;i < REMOTE_MAX_DEVICE;i++)
    {
        if(remote_device_list[i] != 0)
        {
            LOG_D("remote_device_list[%d] is %d\r\n",i,remote_device_list[i]);
        }
    }
    device_sync_start();//start local device sync
    remote_sync_ready = 1;
}

void wifi_device_add(uint32_t device_id)
{
    uint8_t ver_str[16] = {0};

    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    if(remote_sync_ready == 0 || remote_device_find(device_id) == 1)
    {
        return;
    }

    remote_device_add(device_id);
    aq_device_upload_set(device_id,0);

    switch(device->type)
    {
    case DEVICE_TYPE_MAINUNIT:
    case DEVICE_TYPE_ALLINONE:
        rt_sprintf(ver_str, "1.%d.%d",device->main_ver,device->sub_ver);
        wifi_mainunit_add_device(device_id,ver_str);
        break;
    case DEVICE_TYPE_ENDUNIT:
        wifi_endunit_add_device(device_id);
        break;
    case DEVICE_TYPE_DOORUNIT:
        wifi_doorunit_add_device(device_id);
        break;
    default:
        break;
    }
}

void wifi_device_delete(uint32_t device_id)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    char *bind_addr_buf = rt_malloc(16);
    char *device_addr_buf = rt_malloc(16);
    rt_sprintf(device_addr_buf,"%ld",device_id);
    remote_device_del(device_id);
    local_subdev_del_cmd(device_addr_buf);
    if(device->type == DEVICE_TYPE_DOORUNIT)
    {
        rt_sprintf(bind_addr_buf,"%ld",device->bind_id);
        mcu_dp_value_update(MAINUNIT_DPID_DOOR_ID,0,bind_addr_buf,my_strlen(bind_addr_buf)); //BOOL型数据上报;
    }
    rt_free(bind_addr_buf);
    rt_free(device_addr_buf);
}

void wifi_device_info_upload(uint32_t device_id)
{
    aqualarm_device_t *device = aq_device_find(device_id);
    if(device == RT_NULL)
    {
        return;
    }

    switch(device->type)
    {
    case DEVICE_TYPE_MAINUNIT:
    case DEVICE_TYPE_ALLINONE:
        wifi_mainunit_info_upload(device_id);
        break;
    case DEVICE_TYPE_ENDUNIT:
        wifi_endunit_info_upload(device_id,device->bind_id);
        break;
    case DEVICE_TYPE_DOORUNIT:
        wifi_doorunit_info_upload(device_id,device->bind_id);
        break;
    default:
        break;
    }
}

void wifi_device_heart_upload(uint32_t device_id,uint8_t state)
{
    char *addr_buf = rt_malloc(16);
    rt_sprintf(addr_buf,"%ld",device_id);
    aq_device_online_set(device_id,state);
    if(state)
    {
        heart_beat_report(addr_buf,0);
        user_updata_subden_online_state(0,addr_buf,1,1);
    }
    else
    {
        user_updata_subden_online_state(0,addr_buf,1,0);
    }
    rt_free(addr_buf);
}

void wifi_heart_reponse(char *addr_buf)//called by wifi module
{
    if(addr_buf == RT_NULL)
    {
        return;
    }

    uint32_t device_addr = atol(addr_buf);
    aqualarm_device_t *device = aq_device_find(device_addr);
    if(device == RT_NULL)
    {
        return;
    }

    if(aq_device_upload_get(device_addr) == 0)
    {
        aq_device_upload_set(device_addr,1);
        wifi_device_info_upload(device->device_id);
    }

    if(device->type == DEVICE_TYPE_DOORUNIT || device->type == DEVICE_TYPE_ENDUNIT)
    {
        if(aq_device_online_get(device->bind_id) == 1 && aq_device_online_get(device->device_id) == 1)
        {
            wifi_device_heart_upload(device->device_id,1);
        }
        else
        {
            wifi_device_heart_upload(device->device_id,0);
        }
    }
    else
    {
        if(aq_device_online_get(device->device_id))
        {
            wifi_device_heart_upload(device->device_id,1);
        }
        else
        {
            wifi_device_heart_upload(device->device_id,0);
        }
    }
}
