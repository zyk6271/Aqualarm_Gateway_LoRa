/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-13     Rick       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <spi_flash.h>
#include <drv_spi.h>
#include <string.h>
#include <stdlib.h>
#include "pin_config.h"
#include "fal.h"
#include "easyflash.h"
#include "flashwork.h"
#include "radio_protocol.h"

#define DBG_TAG "FLASH"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

rt_spi_flash_device_t w25q16;

uint32_t total_slot = 0;

rt_align(RT_ALIGN_SIZE)
rt_slist_t _device_list = RT_SLIST_OBJECT_INIT(_device_list);
static struct rt_mutex flash_mutex;

int storage_init(void)
{
    rt_err_t status;
    rt_mutex_init(&flash_mutex, "flash_mutex", RT_IPC_FLAG_FIFO);
    extern rt_spi_flash_device_t rt_sfud_flash_probe(const char *spi_flash_dev_name, const char *spi_dev_name);
    rt_hw_spi_device_attach("spi1", "spi10", GPIOB, GPIO_PIN_6);
    w25q16 = rt_sfud_flash_probe("norflash0", "spi10");
    if (RT_NULL == w25q16)
    {
        LOG_E("sfud fail\r\n");
        return RT_ERROR;
    };
    status = fal_init();
    if (status == 0)
    {
        LOG_E("fal_init fail\r\n");
        return RT_ERROR;
    };
    status = easyflash_init();
    if (status != EF_NO_ERR)
    {
        LOG_E("easyflash_init fail\r\n");
        return RT_ERROR;
    };
    LOG_I("Storage Init Success\r\n");
    read_device_from_flash();
    aq_device_print();
    return RT_EOK;
}

uint32_t flash_get_key(char *key_name)
{
    uint8_t read_len = 0;
    uint32_t read_value = 0;
    char read_value_temp[32] = {0};
    read_len = ef_get_env_blob(key_name, read_value_temp, 32, NULL);
    if(read_len>0)
    {
        read_value = atol(read_value_temp);
    }
    else
    {
        read_value = 0;
    }

    return read_value;
}

void flash_set_key(char *key_name,uint32_t value)
{
    char *value_buf = rt_malloc(64);//申请临时buffer空间
    rt_sprintf(value_buf, "%d", value);
    ef_set_env_blob(key_name, value_buf,rt_strlen(value_buf));
    rt_free(value_buf);
}

void read_device_from_flash(void)
{
    total_slot = flash_get_key("slot");

    uint32_t index = 0;
    aqualarm_device_t *device = RT_NULL;
    while (index < 32)
    {
        if ((total_slot & (1 << index)))
        {
            device = (aqualarm_device_t *)rt_malloc(sizeof(aqualarm_device_t));
            char key[16] = {0};
            rt_sprintf(key, "index:%d", index);
            size_t len = ef_get_env_blob(key, device, sizeof(aqualarm_device_t) , NULL);
            if(len == 0)
            {
                rt_free(device);
            }
            else
            {
                rt_mutex_take(&flash_mutex, RT_WAITING_FOREVER);
                rt_slist_append(&_device_list, &(device->slist));
                rt_mutex_release(&flash_mutex);
            }
        }
        index++;
    }
}

int8_t get_free_device_slot(void)
{
    uint32_t index = 0; // 记录当前位的索引
    while (index < 32)
    {
        if ((total_slot & (1 << index)) == 0)
        {
            total_slot |= 1 << index;
            return index;
        }
        index++;
    }
    return RT_ERROR;
}

void delete_select_device_slot(uint32_t index)
{
    total_slot &= ~(1 << index);
}

aqualarm_device_t *aq_device_find(uint32_t device_id)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            return device;
        }
    }

    return RT_NULL;
}

static void aq_device_save(aqualarm_device_t * device)
{
    rt_mutex_take(&flash_mutex, RT_WAITING_FOREVER);
    char key[16] = {0};
    rt_sprintf(key, "index:%d", device->slot);
    ef_set_env_blob(key,device,sizeof(aqualarm_device_t));
    rt_mutex_release(&flash_mutex);
}

aqualarm_device_t *aq_device_create(uint8_t main_ver,uint8_t sub_ver,int rssi,uint8_t bat,uint8_t type,uint32_t device_id,uint32_t bind_id)
{
    aqualarm_device_t *check_device = aq_device_find(device_id);
    if(check_device != RT_NULL)
    {
        check_device->rssi = rssi;
        return check_device;
    }

    int8_t slot = get_free_device_slot();
    if(slot == RT_ERROR)
    {
        return RT_NULL;
    }

    aqualarm_device_t *device = (aqualarm_device_t *)rt_malloc(sizeof(aqualarm_device_t));
    if (device == RT_NULL)
    {
        return RT_NULL;
    }

    rt_memset(device, 0, sizeof(aqualarm_device_t));
    device->main_ver = main_ver;
    device->sub_ver = sub_ver;
    device->slot = slot;
    device->battery = bat;
    device->type = type;
    device->rssi = rssi;
    device->device_id = device_id;
    device->bind_id = bind_id;
    device->online = 1;
    rt_slist_init(&(device->slist));

    rt_mutex_take(&flash_mutex, RT_WAITING_FOREVER);
    rt_slist_append(&_device_list, &(device->slist));
    flash_set_key("slot",total_slot);
    aq_device_save(device);
    rt_mutex_release(&flash_mutex);

    return device;
}

uint8_t aq_device_delete(uint32_t device_id)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            wifi_device_delete(device->device_id);
            rt_mutex_take(&flash_mutex, RT_WAITING_FOREVER);
            delete_select_device_slot(device->slot);
            flash_set_key("slot",total_slot);
            rt_slist_remove(&_device_list, &(device->slist));
            rt_mutex_release(&flash_mutex);
            rt_free(device);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

void aq_bind_delete(uint32_t bind_id)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->bind_id == bind_id)
        {
            aq_device_delete(device->device_id);
        }
    }
}

void aq_device_print(void)
{
    rt_slist_t *node;
    rt_slist_for_each(node, &_device_list)
    {
        aqualarm_device_t *device = rt_slist_entry(node, aqualarm_device_t, slist);
        LOG_I("device info:addr %d,bind %d,slot %d,ver v1.%d.%d,rssi %d,bat %d\r\n",device->device_id,\
                device->bind_id,device->slot,device->main_ver,device->sub_ver,device->rssi,device->battery);
        LOG_I("device status:battery %d,waterleak %d,ack %d,online %d,upload %d\r\n",device->battery,\
                device->waterleak,device->heart,device->online,device->upload);
    }
}
MSH_CMD_EXPORT(aq_device_print,aq_device_print);

void aqualarm_device_heart(rx_format *rx_frame)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == rx_frame->source_addr)
        {
            device->heart = 1;
            device->rssi = rx_frame->rssi;
            aq_device_online_set(rx_frame->source_addr,1);
            wifi_device_heart_upload(rx_frame->source_addr,1);
        }
    }
}

void aqualarm_slaver_heart(uint32_t slaver_addr)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == slaver_addr)
        {
            aq_device_online_set(slaver_addr,1);
            wifi_device_heart_upload(slaver_addr,1);
        }
    }
}

void aq_device_valve_set(uint32_t device_id,uint8_t state)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            device->valve = state;
        }
    }
}

void aq_device_version_set(uint32_t device_id,uint8_t main_ver,uint8_t sub_ver)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            if((device->main_ver != main_ver) || (device->sub_ver != sub_ver))
            {
                device->main_ver = main_ver;
                device->sub_ver = sub_ver;
                aq_device_save(device);
            }
        }
    }
}

void aq_device_online_set(uint32_t device_id,uint8_t state)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            if(device->online != state)
            {
                device->online = state;
                aq_device_save(device);
            }
        }
    }
}

uint8_t aq_device_upload_get(uint32_t device_id)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            return device->upload;
        }
    }

    return 0;
}

void aq_device_upload_set(uint32_t device_id,uint8_t state)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            if(device->upload != state)
            {
                device->upload = state;
                aq_device_save(device);
            }
        }
    }
}

uint8_t aq_device_online_get(uint32_t device_id)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->device_id == device_id)
        {
            return device->online;
        }
    }

    return 0;
}

uint32_t aq_device_doorunit_find(uint32_t device_id)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->bind_id == device_id && device->type == DEVICE_TYPE_DOORUNIT)
        {
            return device->device_id;
        }
    }

    return 0;
}
