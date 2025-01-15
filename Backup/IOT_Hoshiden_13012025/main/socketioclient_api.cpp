/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  socket.io
*   FileName             :  socketioclient.cpp
*   File Description     :  Khai bao ham su dung SOCKETIO trong esp32
*
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification
    Author                  	Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    Do Xuan An              	06/03/2024     Tao file
----------------------------	----------     ------------------------------------------
==================================================================================================*/
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "socketioclient_api.h"
#include <esp_log.h>
#include <string.h>
#include "cJSON.h"
#include "ota.h"
#include "IO.h"
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
static char TAG[] = "SIOC_API";
IOT_Data_t *SocketIoClientAPI::iot_Data = nullptr;
static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;
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

#define api_sio_debug(fmt, args...) // ESP_LOGI(TAG,fmt, ## args);
#define api_sio_info(fmt, args...) ESP_LOGI(TAG, fmt, ##args);
#define api_sio_error(fmt, args...) ESP_LOGE(TAG, fmt, ##args);

//---------------------------------------------------------------------------------------------------
void SocketIoClientAPI::initCbFunc()
{
    sio->setConnectCB([](SocketIoClient *ws, bool b)
                      {
        api_sio_info("Connected <%d>", b?1:0);
        if(b)
        {
            iot_Data->ServerStatus = true;
        }
        else 
        {
            iot_Data->ServerStatus = false;
        } });

    // sio->seertEventFuncCallBack([](SocketIoClient *ws, uint32_t id)
    //                             { FLAG_SetFlag(FLAG_SIO_CALLBACK_UPDATE_COUNT); });

    sio->setCB([](SocketIoClient *c, const char *msg, int len, int type)
               {
                   // api_sio_info("Co goi tin moi");
                   // api_sio_info("%s",msg);
               });

    sio->on("update-firmware", [](SocketIoClient *c, char *msg)
            {
        cJSON *data_receive = cJSON_Parse(msg);
        api_sio_info("Event Receive: update-firmware");
        if (cJSON_GetObjectItem(data_receive, "url"))
        {
            OTASetUrl(cJSON_GetObjectItem(data_receive, "url")->valuestring);
            OTARun();
        }
        cJSON_Delete(data_receive);
        FLAG_SetFlag(FLAG_SIO_EVENT_UPDATE_FIMWARE); });

    sio->on("error-pcb", [](SocketIoClient *c, char *msg)
            {
                api_sio_info("Event Receive: error-pcb");
                cJSON *data_receive = cJSON_Parse(msg);
                
                // Initialize variables with default values
                uint16_t _errorcode = 0; // Default to 0
                bool _flicker = false;   // Default to false

                if (cJSON_GetObjectItem(data_receive, "errorCode"))
                {
                    _errorcode = cJSON_GetObjectItem(data_receive, "errorCode")->valueint;
                }
                if (cJSON_GetObjectItem(data_receive, "flicker"))
                {
                    _flicker = cJSON_GetObjectItem(data_receive, "flicker")->valueint;
                }
                SetError(_errorcode, _flicker); });

    sio->on("off-buzz", [](SocketIoClient *c, char *msg)
            {
                api_sio_info("Event Receive: off-buzz");
                cJSON *data_receive = cJSON_Parse(msg);
                if (cJSON_GetObjectItem(data_receive, "status"))
                {
                    bool status = cJSON_GetObjectItem(data_receive, "status")->valueint;
                    ESP_LOGI(TAG, "Status off-buzz: %d", status );
                   SetBuzz(status);
                } });
    sio->on("success-pcb", [](SocketIoClient *c, char *msg)
            {
                api_sio_info("Event Receive: success-pcb");
                cJSON *data_receive = cJSON_Parse(msg);
                if (cJSON_GetObjectItem(data_receive, "status"))
                {
                    bool status = cJSON_GetObjectItem(data_receive, "status")->valueint;
                    if(status)
                    {
                         FLAG_SetFlag(FLAG_SIO_EVENT_UPDATE_STATUS_PCB);
                    }
                } });
    sio->on("open-pcb", [](SocketIoClient *c, char *msg)
            {
                api_sio_info("Event Receive: open-pcb");
                cJSON *data_receive = cJSON_Parse(msg);
                if (cJSON_GetObjectItem(data_receive, "status"))
                {
                    bool status = cJSON_GetObjectItem(data_receive, "status")->valueint;
                    if(status)
                    {
                         FLAG_SetFlag(FLAG_SIO_EVENT_UPDATE_STATUS_PCB);
                    }
                } });

    sio->on("config-iot", [](SocketIoClient *c, char *msg)
            {
        cJSON *data_receive = cJSON_Parse(msg);
        api_sio_info("Event Receive: config-iot");
        if (cJSON_GetObjectItem(data_receive, "SSID"))
        {
            iot_Data->Ssid = (char*)malloc(50*sizeof(char));
            strcpy(iot_Data->Ssid,cJSON_GetObjectItem(data_receive, "SSID")->valuestring);
        }
        if (cJSON_GetObjectItem(data_receive, "PASS"))
        {
            iot_Data->Pass = (char*)malloc(50*sizeof(char));
            strcpy(iot_Data->Pass,cJSON_GetObjectItem(data_receive, "PASS")->valuestring);
        }
        if (cJSON_GetObjectItem(data_receive, "HOST"))
        {
            iot_Data->IpSev = (char*)malloc(50*sizeof(char));
            strcpy(iot_Data->IpSev,cJSON_GetObjectItem(data_receive, "HOST")->valuestring);
        }
         if (cJSON_GetObjectItem(data_receive, "HOSTNAME"))
        {
            iot_Data->HostName = (char*)malloc(50*sizeof(char));
            strcpy(iot_Data->HostName,cJSON_GetObjectItem(data_receive, "HOSTNAME")->valuestring);
        }
        if (cJSON_GetObjectItem(data_receive, "PORT"))
        {
            uint16_t _port = cJSON_GetObjectItem(data_receive, "PORT")->valueint;
            iot_Data->port = _port;
        }
       
        cJSON_Delete(data_receive);
        FLAG_SetFlag(FLAG_SIO_EVENT_CONFIG); });
}

//---------------------------------------------------------------------------------------------------
void SocketIoClientAPI::start()
{
    sio->start();
}

//---------------------------------------------------------------------------------------------------
void SocketIoClientAPI::SIOClientSendEvent(const char *event, const char *data)
{
    if (iot_Data->ServerStatus)
    {
        if (FlagSio == false)
        {
            FlagSio = true;
            sio->emit(event, data);
            FlagSio = false;
        }
    }
}

//---------------------------------------------------------------------------------------------------
void SocketIoClientAPI::SIOClientSendEvent(const char *event, const char *data, uint32_t id)
{
    if (iot_Data->ServerStatus)
    {
        if (FlagSio == false)
        {
            FlagSio = true;
            sio->emit(event, data, id);
            FlagSio = false;
        }
    }
}
//---------------------------------------------------------------------------------------------------
void SocketIoClientAPI::SendEventMachineStatus(bool status)
{
    cJSON *machine_status = cJSON_CreateObject();
    cJSON_AddBoolToObject(machine_status, "isRunning", status);
    char *data_machine = cJSON_Print(machine_status);
    SIOClientSendEvent("iot-status", data_machine);
    cJSON_Delete(machine_status);
    cJSON_free(data_machine);
}

//---------------------------------------------------------------------------------------------------
void SocketIoClientAPI::IsReceivedStatus(bool _isReceived)
{
    cJSON *isReceived = cJSON_CreateObject();
    cJSON_AddBoolToObject(isReceived, "isReceived", _isReceived);
    char *data_send = cJSON_Print(isReceived);
    SIOClientSendEvent("update-status", data_send);
    cJSON_Delete(isReceived);
    cJSON_free(data_send);
}
void SocketIoClientAPI::SendPCB(const char *QR)
{
    cJSON *qr_code = cJSON_CreateObject();
    cJSON_AddStringToObject(qr_code, "data", QR);
    char *data = cJSON_Print(qr_code);
    SIOClientSendEvent("declare-Pcb-IOT", data, 1);
    ESP_LOGI(TAG, "QR_Code: %s", QR);
    cJSON_Delete(qr_code);
    cJSON_free(data);
}
//======================================END FILE===================================================/
