/*==================================================================================================
*   Project              :  IOT GUNZE
*   Doccument            :  ESP32 OTA
*   FileName             :  ota.c
*   File Description     :  Khai bao cac ham nap code online
*
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification     
    Author                  	Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    Do Xuan An              	17/04/2024     Tao file
----------------------------	----------     ------------------------------------------
==================================================================================================*/
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <string.h>
#include "ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "cJSON.h"
/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/
/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
static char TAG[] = "OTA";
TaskHandle_t OTATaskHandle;
/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/
/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/
/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL FUNCTIONS
==================================================================================================*/
char* urlFimware;
#define ota_debug(fmt, args...)  ESP_LOGI(TAG,fmt, ## args);
#define ota_info(fmt, args...)   ESP_LOGI(TAG,fmt, ## args);
#define ota_error(fmt, args...)  ESP_LOGE(TAG,fmt, ## args);

void OTASetUrl(char* url)
{
    ota_info("VL");
    int leng = strlen(url);
    urlFimware = (char*)malloc((leng+1) * sizeof(char));
    strcpy(urlFimware,url);
}
//---------------------------------------------------------------------------------------------------
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ota_debug("HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ota_debug("HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ota_debug("HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ota_debug("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ota_debug("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ota_debug("HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ota_debug("HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}
//---------------------------------------------------------------------------------------------------
void ota_task(void *pvParameter)
{
    ota_info("Starting OTA");
    esp_http_client_config_t config = {
        .url = urlFimware,
        .timeout_ms = 15000,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,

    };
    ota_info("ota url : %s",config.url);
    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        ota_info("Firmware upgrade done");
        esp_restart();
    } else {
        ota_error("Firmware upgrade failed");
        vTaskSuspend(OTATaskHandle);
    }
    while (1) {
        ota_info("Task OTA run!!!!")
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//---------------------------------------------------------------------------------------------------
void OTARun(void)
{
    ESP_LOGI(TAG, "Starting OTA");
    xTaskCreate(&ota_task, "OTATask", 4096, NULL, 1, &OTATaskHandle);
}

//======================================END FILE===================================================/
