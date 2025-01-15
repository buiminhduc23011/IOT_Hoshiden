#ifndef __MAIN__H
#define __MAIN__H


#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "stdbool.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// NVS
#define NVS_NAMESPACE_CONFIG "CONFIG"
#define NVS_KEY_WIFI_SSID "WSSID"
#define NVS_KEY_WIFI_PASS "WPASS"
#define NVS_KEY_IP_SERVER "IP"
#define NVS_KEY_PORT_SERVER "PORT"
#define NVS_KEY_HOST_NAME "HOST"

#define NVS_KEY_QRCODE "QrCode"
#define NVS_KEY_STATE_XL "StateXL"
#define NVS_KEY_STATE_UPDATE_PCB "StateUpdatePcb"

// Wifi
#define WIFI_NAME_DEFAULT "STI_VietNam_No8"
#define WIFI_PASS_DEFAULT "66668888"

// TCP
#define TCP_SERVER_ADDRESS "192.168.1.58"
#define TCP_SERVER_PORT 3000

void main_task(void);

#ifdef __cplusplus
}
#endif

#endif