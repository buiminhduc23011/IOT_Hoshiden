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
    Bùi Văn Đức              	09/01/2024     Tao file
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
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"
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
#define OTA_URL_SIZE 256
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
char *urlFimware;
#define ota_debug(fmt, args...) ESP_LOGI(TAG, fmt, ##args);
#define ota_info(fmt, args...) ESP_LOGI(TAG, fmt, ##args);
#define ota_error(fmt, args...) ESP_LOGE(TAG, fmt, ##args);

void OTASetUrl(char *url)
{
    ota_info("VL");
    int leng = strlen(url);
    urlFimware = (char *)malloc((leng + 1) * sizeof(char));
    strcpy(urlFimware, url);
}
//---------------------------------------------------------------------------------------------------

esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}
//---------------------------------------------------------------------------------------------------
void ota_task(void *pvParameter)
{
    ota_info("Starting OTA");
    esp_http_client_config_t http_config = {
        .url = urlFimware,
        .event_handler = http_event_handler,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    ESP_LOGI(TAG, "Download firmware from: %s", urlFimware);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK)
    {
        ota_info("Firmware upgrade done. ESP32 is about to reboot...............................");
        esp_restart();
    }
    else
    {
        ota_error("Firmware upgrade failed");
        vTaskSuspend(OTATaskHandle);
    }
    while (1)
    {
        ota_info("Task OTA run!!!!")
            vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
//---------------------------------------------------------------------------------------------------
void OTARun(void)
{
    ESP_LOGI(TAG, "Starting OTA");
    ESP_LOGI(TAG, "Free heap size: %d bytes\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    xTaskCreate(&ota_task, "OTATask", 4096, NULL, 1, &OTATaskHandle);
}

//======================================END FILE===================================================/
