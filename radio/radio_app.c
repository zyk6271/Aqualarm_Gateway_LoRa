/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-13     Rick       the first version
 */
#include "rtthread.h"
#include "main.h"
#include "radio.h"
#include "radio_app.h"
#include "radio_encoder.h"

#define DBG_TAG "RADIO_APP"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

RadioEvents_t RadioEvents;

static void OnTxDone(void)
{
    LOG_D("TxDone\r\n");

    Radio.SetChannel( RF_RX_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON_DISABLE,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON_DISABLE, true );
    Radio.SetMaxPayloadLength(MODEM_LORA, 255);
    Radio.Rx(0);

    rf_txdone_callback();
}

void RF_Send(char *payload,int size)
{
//    LOG_HEX("gw-tx", 16, payload, size);
    Radio.Send(payload, size);
}

static void OnRxDone(uint8_t *src_payload, uint16_t size, int16_t rssi, int8_t snr)
{
//    LOG_D("OnRxDone,rssi %d,snr %d,data ",rssi,snr);
//    for(uint8_t i = 0;i < size; i++)
//    {
//        LOG_D("%02X ",src_payload[i]);
//    }
//    LOG_HEX("gw-rx", 16, src_payload, size);

    radio_protocol_parse(rssi,snr,src_payload,size);
}

static void OnTxTimeout(void)
{
    LOG_W("OnTxTimeout\r\n");

    Radio.SetChannel( RF_RX_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON_DISABLE,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON_DISABLE, true );
    Radio.SetMaxPayloadLength(MODEM_LORA, 255);
    Radio.Rx(0);
}

static void OnRxTimeout(void)
{
    LOG_W("OnRxTimeout\r\n");

    Radio.SetChannel( RF_RX_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON_DISABLE,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON_DISABLE, true );
    Radio.SetMaxPayloadLength(MODEM_LORA, 255);
    Radio.Rx(0);
}

static void OnRxError(void)
{
    LOG_W("OnRxError\r\n");

    Radio.SetChannel( RF_RX_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON_DISABLE,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON_DISABLE, true );
    Radio.SetMaxPayloadLength(MODEM_LORA, 255);
    Radio.Rx(0);
}

static void OnCadDone(bool channelActivityDetected)
{
    LOG_D("OnCadDone channelActivityDetected (%d)\r\n",channelActivityDetected);
    radio_cad_detected(channelActivityDetected);
}

void radio_init(void)
{
    RadioCRC_Init();
    RadioQueue_Init();

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;
    RadioEvents.CadDone = OnCadDone;

    Radio.Init(&RadioEvents);

    Radio.SetChannel( RF_RX_FREQUENCY );

    Radio.SetTxConfig( MODEM_LORA, LORA_TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON_DISABLE,
                                   true, 0, 0, LORA_IQ_INVERSION_ON_DISABLE, 5000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON_DISABLE,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON_DISABLE, true );

    Radio.SetMaxPayloadLength(MODEM_LORA, 255);
    Radio.Rx(0);

    rf_led(1);
}
