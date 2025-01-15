#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_chip_info.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <map>
#include <set>
#include <string>
#include <math.h>
#include <esp_log.h>
#include <string>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "socketioclient_api.h"
#include "wificlient.h"
#include "datatype.h"
#include "flag.h"
#include "main.h"
#include "eeprom.h"
#include "IO.h"
#include "TCP.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static const char *TAG = "MAIN";
TaskHandle_t MainTaskHandle;
SocketIoClientAPI sioapi;
IOT_Data_t iot_Data;
Eeprom envs;
char *text_rev;

#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM (3) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
QueueHandle_t uart0_queue;
char **buff_uart;
uint16_t T_Retry_Wifi;
uint16_t T_Dis_Server;
uint16_t P_Connect;
char **
split(char *str, char delimiter, int *count)
{
    int i, start = 0, part_count = 0;
    int len = strlen(str);
    char **parts = NULL;
    for (i = 0; i <= len; i++)
    {
        if (str[i] == delimiter || str[i] == '\0')
        {
            int part_len = i - start;
            char *part = (char *)malloc(part_len + 1);
            memcpy(part, &str[start], part_len);
            part[part_len] = '\0';
            parts = (char **)realloc(parts, (part_count + 1) * sizeof(char *));
            parts[part_count] = part;
            part_count++;
            start = i + 1;
        }
    }
    *count = part_count;
    return parts;
}

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    int count = 0;
    uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);
    for (;;)
    {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, portMAX_DELAY))
        {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch (event.type)
            {
            case UART_DATA:
                uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                buff_uart = split((char *)dtmp, ';', &count);
                ESP_LOGI(TAG, "uart count : %d", count);
                FLAG_SetFlag(FLAG_UART_EVENT_REV_DATA);
                break;
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            case UART_PATTERN_DET:
                ESP_LOGI(TAG, "[UART PATTERN DETECTED]");
                break;
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void WWifi_ConnectedCB(bool b)
{
    if (b)
    {
        GetIP(&iot_Data.Ip);
        ESP_LOGI(TAG, "ip : %s", iot_Data.Ip);
        iot_Data.WifiStatus = true;
    }
    else
    {
        iot_Data.WifiStatus = false;
    }
}

//---------------------------------------------------------------------------------
void ReadConfig(IOT_Data_t *_iotConfig)
{
    _iotConfig->Ssid = (char *)malloc(50 * sizeof(char));
    if (envs.readString(NVS_KEY_WIFI_SSID, _iotConfig->Ssid) == 0)
    {
        _iotConfig->Ssid = "STI_VietNam_No8";
    }
    _iotConfig->Pass = (char *)malloc(50 * sizeof(char));
    if (envs.readString(NVS_KEY_WIFI_PASS, _iotConfig->Pass) == 0)
    {
        _iotConfig->Pass = "66668888";
    }
    _iotConfig->IpSev = (char *)malloc(50 * sizeof(char));
    if (envs.readString(NVS_KEY_IP_SERVER, _iotConfig->IpSev) == 0)
    {
        _iotConfig->IpSev = "192.168.1.19";
    }
    if (envs.readUint16(NVS_KEY_PORT_SERVER, &_iotConfig->port) == 0)
    {
        _iotConfig->port = 3000;
    }
    _iotConfig->HostName = (char *)malloc(50 * sizeof(char));
    if (envs.readString(NVS_KEY_HOST_NAME, _iotConfig->HostName) == 0)
    {
        _iotConfig->HostName = "STI-IOT";
    }
}

uint16_t ConvertToU16(char *buff)
{
    uint16_t port = 0;
    int length = strlen(buff);
    for (int i = 0; i < length; i++)
    {
        if ((buff[i] >= '0') && (buff[i] <= '9'))
        {
            port += (buff[i] - 0x30) * pow(10, (length - 1) - i);
        }
    }
    return port;
}

void SetConfig()
{
    envs.writeString(NVS_KEY_WIFI_SSID, buff_uart[1]);
    envs.writeString(NVS_KEY_WIFI_PASS, buff_uart[2]);
    envs.writeString(NVS_KEY_IP_SERVER, buff_uart[3]);
    envs.writeUint16(NVS_KEY_PORT_SERVER, ConvertToU16(buff_uart[4]));
    envs.writeString(NVS_KEY_HOST_NAME, buff_uart[5]);
}
void SaveState()
{
    if (FLAG_GetFlag(FLAG_SIO_EVENT_UPDATE_ERROR_PCB))
    {
        envs.writeString(NVS_KEY_QRCODE, "disconnect");
        envs.writeUint16(NVS_KEY_STATE_UPDATE_PCB, 1);
    }
    else
    {
        if (FLAG_GetFlag(FLAG_SIO_EVENT_UPDATE_PCB))
        {
            envs.writeUint16(NVS_KEY_STATE_UPDATE_PCB, 1);
        }
        else
        {
            envs.writeUint16(NVS_KEY_STATE_UPDATE_PCB, 0);
        }
        char *qrcode = GetQRcode();
        if (qrcode != NULL)
        {
            char *qrcode_copy = strdup(qrcode);
            envs.writeString(NVS_KEY_QRCODE, qrcode_copy);
            free(qrcode_copy);
        }
        else
        {
            envs.writeString(NVS_KEY_QRCODE, "null");
        }
        free(qrcode);
    }
    envs.writeUint16(NVS_KEY_STATE_XL, GetStateXL());
}
void ClearState()
{
    envs.writeUint16(NVS_KEY_STATE_UPDATE_PCB, 0);
    envs.writeString(NVS_KEY_QRCODE, "null");
    envs.writeUint16(NVS_KEY_STATE_XL, 0);
    ESP_LOGI(TAG, "Free State");
}
void GetState()
{
    char _Qrcode[50];
    if (envs.readString(NVS_KEY_QRCODE, _Qrcode) != 0)
    {
        SetQRCode(_Qrcode);
    }
    uint16_t StateUpdatePCB;
    if (envs.readUint16(NVS_KEY_STATE_UPDATE_PCB, &StateUpdatePCB) != 0)
    {
        if (StateUpdatePCB == 1)
        {
            FLAG_SetFlag(FLAG_SIO_EVENT_UPDATE_PCB);
        }
    }
    uint16_t State_XL;
    if (envs.readUint16(NVS_KEY_STATE_XL, &State_XL) != 0)
    {
        SetStateXL(State_XL);
    }
}
//---------------------------------------------------------------------------------
extern "C" void app_main()
{
    //----------------------------------------------------------------------------------
    esp_chip_info_t chip_info;
    uint8_t MacBase[6];
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "Initialising WiFi Connection...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    FLAG_Init();
    IO_Init();

    envs.begin(200);
    ReadConfig(&iot_Data);
    GetState();
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    esp_log_level_set(TAG, ESP_LOG_INFO);
    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_enable_pattern_det_baud_intr(EX_UART_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    uart_pattern_queue_reset(EX_UART_NUM, 20);
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);

    iot_Data.WifiStatus = false;
    iot_Data.ServerStatus = false;
    iot_Data.Mac = (char *)malloc(20 * sizeof(char));
    iot_Data.FimwareVer = (char *)malloc(50 * sizeof(char));
    sprintf(iot_Data.FimwareVer, TOSTRING(PROJECT_NAME));
    //
    esp_err_t err = esp_read_mac(MacBase, ESP_MAC_WIFI_STA);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get base MAC address from efuse. (%s)", esp_err_to_name(err));
    }
    else
    {
        snprintf(iot_Data.Mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X", MacBase[0], MacBase[1], MacBase[2], MacBase[3], MacBase[4], MacBase[5]);
    }

    TCP_Init();

    vTaskDelay(2000);
    Wifi_SetConnectCB(&WWifi_ConnectedCB);
    Wifi_Connect(iot_Data.Ssid, iot_Data.Pass, iot_Data.HostName);
    while (!iot_Data.WifiStatus)
    {
        if (FLAG_GetFlag(FLAG_UART_EVENT_REV_DATA))
        {
            FLAG_ClearFlag(FLAG_UART_EVENT_REV_DATA);
            if (strcmp(buff_uart[0], "Config") == 0)
            {
                SetConfig();
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
            }
            if (strcmp(buff_uart[0], "get") == 0)
            {
                ESP_LOGI("GET_CONFIG", "SSID : (%s)", iot_Data.Ssid);
                ESP_LOGI("GET_CONFIG", "PASS : (%s)", iot_Data.Pass);
                ESP_LOGI("GET_CONFIG", "IP SV: (%s)", iot_Data.IpSev);
                ESP_LOGI("GET_CONFIG", "PORT : (%d)", iot_Data.port);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    sioapi.begin(&iot_Data);
    sioapi.initCbFunc();
    sioapi.start();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}
void main_task(void)
{
   // ESP_LOGI(TAG, "Current RSSI: %d dBm - Status Wifi: %d - Status Server: %d", GetRssi(), iot_Data.WifiStatus, iot_Data.ServerStatus);
    if (FLAG_GetFlag(FLAG_UART_EVENT_REV_DATA))
    {
        FLAG_ClearFlag(FLAG_UART_EVENT_REV_DATA);
        if (strcmp(buff_uart[0], "Config") == 0)
        {
            SetConfig();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
        }
        if (strcmp(buff_uart[0], "get") == 0)
        {
            ESP_LOGI("GET_CONFIG", "SSID : (%s)", iot_Data.Ssid);
            ESP_LOGI("GET_CONFIG", "PASS : (%s)", iot_Data.Pass);
            ESP_LOGI("GET_CONFIG", "IP SV: (%s)", iot_Data.IpSev);
            ESP_LOGI("GET_CONFIG", "PORT : (%d)", iot_Data.port);
            ESP_LOGI("GET_CONFIG", "HOSTNAME : (%s)", iot_Data.HostName);
        }
    }
    if (FLAG_GetFlag(FLAG_SIO_EVENT_CONFIG))
    {
        FLAG_ClearFlag(FLAG_SIO_EVENT_CONFIG);
        envs.writeString(NVS_KEY_WIFI_SSID, iot_Data.Ssid);
        envs.writeString(NVS_KEY_WIFI_PASS, iot_Data.Pass);
        envs.writeString(NVS_KEY_IP_SERVER, iot_Data.IpSev);
        envs.writeUint16(NVS_KEY_PORT_SERVER, iot_Data.port);
        envs.writeString(NVS_KEY_HOST_NAME, iot_Data.HostName);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    if (iot_Data.ServerStatus)
    {
        if (P_Connect < 1000)
        {
            P_Connect++;
        }
        if (P_Connect == 1)
        {
            ClearState();
        }
        if (FLAG_GetFlag(FLAG_SIO_EVENT_UPDATE_PCB))
        {
            FLAG_ClearFlag(FLAG_SIO_EVENT_UPDATE_PCB);
            sioapi.SendPCB(GetQRcode());
        }
        if (FLAG_GetFlag(FLAG_SIO_EVENT_UPDATE_ERROR_PCB))
        {
            FLAG_ClearFlag(FLAG_SIO_EVENT_UPDATE_ERROR_PCB);
            sioapi.SendPCB("disconnect");
        }
        T_Dis_Server = 0;
        if (GetError() == 20)
            SetError(0, true);
    }
    else
    {
        P_Connect = 0;
        if (T_Dis_Server < 1000)
        {
            T_Dis_Server++;
        }
        if (T_Dis_Server == 15)
        {
            SetError(20, true);
        }
    }
    if (iot_Data.ServerStatus == true && iot_Data.WifiStatus == true && ConnectCam() == true)
    {
        SetWarring(0);
    }
    else if (iot_Data.WifiStatus == false)
    {
        SetWarring(10);
    }
    else if (iot_Data.ServerStatus == false)
    {
        SetWarring(5);
    }
    else if (ConnectCam() == false)
    {
        SetWarring(2);
    }
}