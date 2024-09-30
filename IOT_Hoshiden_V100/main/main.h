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

// Wifi
#define WIFI_NAME_DEFAULT "STI_VietNam_No8"
#define WIFI_PASS_DEFAULT "66668888"

// TCP
#define TCP_SERVER_ADDRESS "192.168.1.58"
#define TCP_SERVER_PORT 3000

//INPUT
#define SS_DETECT GPIO_NUM_36    // I1
#define INPUT_SPARE_1 GPIO_NUM_39  // I2
#define INPUT_SPARE_2 GPIO_NUM_34  // I3
#define INPUT_SPARE_3 GPIO_NUM_35 // I4
//OUTPUT
#define XL01 GPIO_NUM_25 //O1
#define RED GPIO_NUM_26 //O2
#define YEL GPIO_NUM_27 //O3
#define GRE GPIO_NUM_14 //O4
#define BUZZ GPIO_NUM_15 //O5
#define TRIGGER_CAMERA GPIO_NUM_2 //O6

//W5500
#define ETH_CS GPIO_NUM_5
#define ETH_SCLK GPIO_NUM_18
#define ETH_MISO GPIO_NUM_19
#define ETH_MOSI GPIO_NUM_23

//LOGIC
#define HIGH (1)
#define LOW (0)


#ifdef __cplusplus
}
#endif

#endif