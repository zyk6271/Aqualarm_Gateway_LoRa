#ifndef __FLASHWORK_H__
#define __FLASHWORK_H__

#include <stdint.h>
#include <rtdef.h>

typedef struct{
    int rssi;
    uint8_t valve;
    uint8_t battery;
    uint8_t waterleak;
    uint32_t bind_id;
    uint8_t slot;
    uint8_t type;
    uint8_t recv;
    uint8_t online;
    uint8_t upload;
    uint8_t main_ver;
    uint8_t sub_ver;
    uint32_t device_id;
    rt_slist_t slist;
}aqualarm_device_t;

void aq_device_online_set(uint32_t device_id,uint8_t state);

#endif

