#include "IEC_Timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static char TAG[] = "IECTimer-TASK";

void TON(TON_TIME *ton_time)
{
    if (ton_time->IN)
    {
        if (!ton_time->flag_start)
        {
            ton_time->start_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            ton_time->flag_start = true;
            ton_time->Q = false;
        }
        else
        {
            if (!ton_time->Q)
            {
                uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
                if (ton_time->ET >= ton_time->PT)
                {
                    ton_time->Q = true;
                    ton_time->ET = ton_time->PT;
                }
                else
                {
                    ton_time->ET = current_time - ton_time->start_time;
                    ton_time->Q = false;
                }
            }
        }
    }
    else
    {
        ton_time->Q = false;
        ton_time->ET = 0;
        ton_time->start_time = 0;
        ton_time->flag_start = false;
    }
}
