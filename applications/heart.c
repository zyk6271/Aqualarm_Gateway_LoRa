#include "rtthread.h"
#include "heart.h"
#include "flashwork.h"
#include "radio_encoder.h"
#include "wifi-api.h"
#include "radio_protocol.h"

#define DBG_TAG "heart"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

rt_slist_t *sync_node;
rt_timer_t sync_timer = RT_NULL;

rt_thread_t heart_t = RT_NULL;

extern rt_slist_t _device_list;

void aqualarm_device_heart_check(void)
{
    rt_slist_t *node;
    aqualarm_device_t *device = RT_NULL;
    rt_slist_for_each(node, &_device_list)
    {
        device = rt_slist_entry(node, aqualarm_device_t, slist);
        if(device->type == DEVICE_TYPE_MAINUNIT || device->type == DEVICE_TYPE_ALLINONE)
        {
            if(device->heart)
            {
                device->heart = 0;
            }
            else
            {
                aq_device_online_set(device->device_id,0);
                wifi_device_heart_upload(device->device_id,0);
                LOG_E("aq_device_offline_set %d\r\n",device->device_id);
            }
        }
    }
}

void device_sync_start(void)
{
    sync_node = rt_slist_first(&_device_list);
    if(sync_node != RT_NULL)
    {
        rt_timer_start(sync_timer);
    }
}

void sync_timer_callback(void)
{
    aqualarm_device_t *device = RT_NULL;
    while (sync_node != RT_NULL)
    {
        device = rt_slist_entry(sync_node, aqualarm_device_t, slist);
        if (device->type == DEVICE_TYPE_MAINUNIT || device->type == DEVICE_TYPE_ALLINONE)
        {
            radio_mainunit_request_sync(device->device_id);
            LOG_I("radio_mainunit_request_sync %d\r\n", device->device_id);
            sync_node = rt_slist_next(sync_node);
            break; // Found a matching device, no need to continue the loop
        }
        sync_node = rt_slist_next(sync_node);
    }

    if (sync_node != RT_NULL)
    {
        rt_timer_start(sync_timer); // Restart timer if we found a device
    }
}

void heart_init(void)
{
    sync_timer = rt_timer_create("sync_timer", sync_timer_callback, RT_NULL, 1000, RT_TIMER_FLAG_SOFT_TIMER | RT_TIMER_FLAG_ONE_SHOT);
}
