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
    BÙI VĂN ĐỨC              	01/10/2024     Tao file
----------------------------	----------     ------------------------------------------
==================================================================================================*/
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <string.h>
#include "IO.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "cJSON.h"
#include "flag.h"
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
static char TAG[] = "IO";
TaskHandle_t IOTaskHandle;
uint16_t E = 0;
uint16_t W = 0;
uint16_t Flag_E = 0;
uint16_t Flag_W = 0;
uint16_t Time_E = 0;
uint16_t Time_W = 0;
uint16_t time_sample = 10;
uint8_t YEL_PIN = 0;
uint8_t RED_PIN = 0;

uint8_t Flag_SS_DETECT = 0;

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
//---------------------------------------------------------------------------------------------------
void IO_Init()
{
    gpio_pad_select_gpio(SS_DETECT);
    gpio_pad_select_gpio(INPUT_SPARE_1);
    gpio_pad_select_gpio(INPUT_SPARE_2);
    gpio_pad_select_gpio(INPUT_SPARE_3);
    gpio_set_direction(SS_DETECT, GPIO_MODE_INPUT);
    gpio_set_direction(INPUT_SPARE_1, GPIO_MODE_INPUT);
    gpio_set_direction(INPUT_SPARE_2, GPIO_MODE_INPUT);
    gpio_set_direction(INPUT_SPARE_3, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(XL01);
    gpio_pad_select_gpio(RED);
    gpio_pad_select_gpio(YEL);
    gpio_pad_select_gpio(GRE);
    gpio_pad_select_gpio(BUZZ);
    gpio_pad_select_gpio(TRIGGER_CAMERA);

    gpio_set_direction(XL01, GPIO_MODE_OUTPUT);
    gpio_set_direction(RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(YEL, GPIO_MODE_OUTPUT);
    gpio_set_direction(GRE, GPIO_MODE_OUTPUT);
    gpio_set_direction(BUZZ, GPIO_MODE_OUTPUT);
    gpio_set_direction(TRIGGER_CAMERA, GPIO_MODE_OUTPUT);

    gpio_set_level(XL01, LOW);
    gpio_set_level(RED, LOW);
    gpio_set_level(YEL, LOW);
    gpio_set_level(GRE, LOW);
    gpio_set_level(BUZZ, LOW);
    gpio_set_level(TRIGGER_CAMERA, LOW);
    IORun();
}

void SetError(uint16_t _E)
{
    E = _E;
}
void SetWarring(uint16_t _W)
{
    W = _W;
}
void E_interval()
{
    if (E > 0)
    {
        gpio_set_level(BUZZ, HIGH);
        Time_E++;
        uint16_t Frequency = 1000 / time_sample / E;
        if (Time_E >= Frequency)
        {
            if (RED_PIN == 1)
            {
                Flag_E++;
            }
            RED_PIN = !RED_PIN;
            gpio_set_level(RED, RED_PIN);
            Time_E = 0;
        }
        if (Flag_E >= E)
        {

            vTaskDelay(1000 / portTICK_PERIOD_MS);
            Flag_E = 0;
        }
    }
    else
    {
        RED_PIN = 0;
        gpio_set_level(BUZZ, LOW);
        Time_E = 0;
        gpio_set_level(RED, RED_PIN);
    }
}
void W_interval()
{
    if (W > 0)
    {
        Time_W++;
        uint16_t Frequency = 1000 / time_sample / W;
        if (Time_W >= Frequency)
        {
            if (YEL_PIN == 1)
            {
                Flag_W++;
            }
            YEL_PIN = !YEL_PIN;
            gpio_set_level(YEL, YEL_PIN);
            Time_W = 0;
        }
        if (Flag_W >= W)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            Flag_W = 0;
        }
    }
    else
    {
        YEL_PIN = 0;
        Time_W = 0;
        gpio_set_level(YEL, YEL_PIN);
    }
}
void io_task(void *pvParameter)
{
    while (1)
    {
        // Trigger
        gpio_set_level(TRIGGER_CAMERA, !gpio_get_level(SS_DETECT));
        // Tower
        if (E == 0 && W == 0)
        {
            gpio_set_level(RED, LOW);
            gpio_set_level(YEL, LOW);
            gpio_set_level(GRE, HIGH);
            gpio_set_level(BUZZ, LOW);
        }
        else
        {
            E_interval();
            W_interval();
            gpio_set_level(GRE, LOW);
        }
        // Cyl
        if (FLAG_GetFlag(FLAG_SIO_EVENT_UPDATE_STATUS_PCB))
        {
            gpio_set_level(XL01, HIGH); // Đóng
            FLAG_ClearFlag(FLAG_SIO_EVENT_UPDATE_STATUS_PCB);
            Flag_SS_DETECT = 0;
        }
        if(!gpio_get_level(SS_DETECT))
        {
            Flag_SS_DETECT ++;
            if(Flag_SS_DETECT >10) gpio_set_level(XL01, LOW);// Mỏ
        }
        vTaskDelay(time_sample / portTICK_PERIOD_MS);
    }
}
//---------------------------------------------------------------------------------------------------
void IORun(void)
{
    ESP_LOGI(TAG, "Starting");
    xTaskCreate(&io_task, "IOTask", 4096, NULL, 2, &IOTaskHandle);
}

//======================================END FILE===================================================/
