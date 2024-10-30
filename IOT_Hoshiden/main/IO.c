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
#include "main.h"
#include "TCP.h"

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
uint8_t State_Trigger = 0;
bool OFF_BUZZ = false;
bool Flicker = false;
bool OpenXL = false;
uint16_t Flag_SS_DETECT_ON = 0;
uint16_t Flag_SS_DETECT_OFF = 0;
uint16_t T_OFF_Trigger = 0;
uint16_t TIMEOUT_TRIG = 0;
uint16_t Flag_Logi = 0;
uint16_t Time_W_E = 0;
uint16_t Time_W_W = 0;
bool CheckCam = false;
uint16_t TimeoutCamera = 0;
uint16_t State_XL = 0;

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
void SetError(uint16_t _E, bool _flicker)
{
    E = _E;
    Flicker = _flicker;
}
void SetWarring(uint16_t _W)
{
    W = _W;
}
void SetBuzz(bool status)
{
    OFF_BUZZ = status;
}
int16_t GetError(void)
{
    return E;
}
uint16_t GetStateXL(void)
{
    return State_XL;
}
void SetStateXL(uint state)
{
    State_XL = state;
}
void E_interval()
{
    if (E > 0)
    {
        if (OFF_BUZZ)
            gpio_set_level(BUZZ, LOW);
        else
            gpio_set_level(BUZZ, HIGH);
        if (Flicker)
        {
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
                Time_W_E = 0;
            }
            if (Flag_E >= E)
            {
                Flag_E = 0;
                RED_PIN = 0;
                gpio_set_level(RED, RED_PIN);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        }
        else
        {
            RED_PIN = 1;
            gpio_set_level(RED, RED_PIN);
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
            Time_W_W = 0;
        }
        if (Flag_W >= W)
        {
            Flag_W = 0;
            YEL_PIN = 0;
            gpio_set_level(YEL, YEL_PIN);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
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
        if (!gpio_get_level(SS_DETECT))
        {
            State_Trigger = 1;
            T_OFF_Trigger = 0;
            if (State_Trigger == 1 && TIMEOUT_TRIG < 1000)
            {
                TIMEOUT_TRIG++;
            }
            if (TIMEOUT_TRIG > 200)
            {
                State_Trigger = 0;
            }
            if (TIMEOUT_TRIG < 2)
            {
                reset_QRcode();
            }
        }
        else
        {
            TIMEOUT_TRIG = 0;
            if (T_OFF_Trigger < 1000)
            {
                T_OFF_Trigger++;
            }
            if (T_OFF_Trigger > 200) // Timeout Trigger, 200 là giữ trigger trong 2s kể từ khi mất cảm biến
            {
                State_Trigger = 0;
            }
        }
        if (!FLAG_GetFlag(FLAG_SIO_EVENT_UPDATE_PCB))
        {
            gpio_set_level(TRIGGER_CAMERA, State_Trigger);
        }
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
            OpenXL = true;
            FLAG_ClearFlag(FLAG_SIO_EVENT_UPDATE_STATUS_PCB);
        }
        if (OpenXL)
        {
            State_XL = 0;
            CheckCam = false;
            if (Flag_SS_DETECT_OFF < 500)
            {
                if (gpio_get_level(SS_DETECT))
                {
                    Flag_SS_DETECT_OFF++;
                }
            }
            if (Flag_SS_DETECT_OFF > 200)
            {
                OpenXL = false;
            }
            Flag_SS_DETECT_ON = 0;
        }
        if (!gpio_get_level(SS_DETECT) && OpenXL == false)
        {
            if (Flag_SS_DETECT_ON < 50)
                Flag_SS_DETECT_ON++;
            if (Flag_SS_DETECT_ON > 30)
            {
                State_XL = 1; // Đóng
                if (!FLAG_GetFlag(FLAG_SIO_EVENT_UPDATE_PCB))
                {
                    CheckCam = true;
                }
                Flag_SS_DETECT_OFF = 0;
            }
        }
        gpio_set_level(XL01, State_XL);
        // Check Timeout Camera
        if (CheckCam)
        {
            if (TimeoutCamera < 400)
            {
                TimeoutCamera++;
            }
            if (TimeoutCamera == 300)
            {
                char *qr_code = GetQRcode();
                if (qr_code == NULL)
                {
                    ESP_LOGI(TAG, "GetQRcode() returned NULL, triggering error flag");
                    FLAG_SetFlag(FLAG_SIO_EVENT_UPDATE_ERROR_PCB);
                    FLAG_SetFlag(FLAG_SIO_EVENT_RETRY_CONNECTCAMERA);
                }
                else
                {
                    ESP_LOGI(TAG, "GetQRcode() returned valid data: %s", qr_code);
                }
            }
        }
        else
        {
            TimeoutCamera = 0;
        }

        Flag_Logi++;
        if (Flag_Logi > 100)
        {
            // if (OpenXL)
            // {
            //     ESP_LOGI(TAG, "OpenXL True ;Flag_SS_DETECT_OFF: %d", Flag_SS_DETECT_OFF);
            // }
            // else
            // {
            //     ESP_LOGI(TAG, "OpenXL False ;Flag_SS_DETECT_OFF: %d", Flag_SS_DETECT_OFF);
            // }
            // if (OFF_BUZZ)
            //     ESP_LOGI(TAG, "OFF Buzz");
            // else
            //     ESP_LOGI(TAG, "ON Buzz");
            // ESP_LOGI(TAG, "E: %d, W: %d", E, W);
            main_task();

            Flag_Logi = 0;
        }
        vTaskDelay(time_sample / portTICK_PERIOD_MS);
    }
}

//---------------------------------------------------------------------------------------------------
void IORun(void)
{
    ESP_LOGI(TAG, "Starting");
    xTaskCreate(&io_task, "IOTask", 4096, NULL, 5, &IOTaskHandle);
}

//======================================END FILE===================================================/
