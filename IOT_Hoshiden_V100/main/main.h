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

// Nidec
// #define INPUT_DO GPIO_NUM_32    // I1
// #define INPUT_VANG GPIO_NUM_33  // I2
// #define INPUT_XANH GPIO_NUM_26  // I3
// #define INPUT_COUNT GPIO_NUM_25 // I4

//Gunze
#define INPUT_DO GPIO_NUM_34    // I1
#define INPUT_VANG GPIO_NUM_35  // I2
#define INPUT_XANH GPIO_NUM_36  // I3
#define INPUT_COUNT GPIO_NUM_39 // I4

#define OUTPUT1 GPIO_NUM_27
#define OUTPUT2 GPIO_NUM_18
#define OUTPUT3 GPIO_NUM_19
#define OUTPUT4 GPIO_NUM_23

//#define LED GPIO_NUM_23

// Modbus
#define MB_PORT_NUM UART_NUM_1
#define MB_SLAVE_ADDR 10
#define MB_DEV_SPEED 115200
#define MB_RX_PIN GPIO_NUM_12
#define MB_TX_PIN GPIO_NUM_27
#define MB_RTS_PIN GPIO_NUM_14

// I2C
// #define I2C_SDA                     GPIO_NUM_21
// #define I2C_SCL                     GPIO_NUM_22

#define HIGH (1)
#define LOW (0)


#ifdef __cplusplus
}
#endif

#endif