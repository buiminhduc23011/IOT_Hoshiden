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
#include "socketioclient.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <sstream>
#include <string.h>
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
static char TAG[] = "SIOC";
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


#ifdef DEBUG
#define cl_sio_debug(fmt, args...)  ESP_LOGI(TAG, fmt, ## args);
#define cl_sio_info(fmt, args...)   ESP_LOGI(TAG, fmt, ## args);
#define cl_sio_error(fmt, args...)  ESP_LOGE(TAG, fmt, ## args);
#else
#define cl_sio_debug(fmt, args...)
#define cl_sio_info(fmt, args...)   ESP_LOGI(TAG, fmt, ## args);
#define cl_sio_error(fmt, args...)  ESP_LOGE(TAG, fmt, ## args);
#endif

SocketIoClient::SocketIoClient(const char* cptr_url, const char* cptr_token, int i16_pingInterval_ms, int i16_maxBufSize, uint8_t u8_pr, BaseType_t bu8_coreID)
{
    ESP_LOGI(TAG,"Token : %s",cptr_token );
    m_ws = new WebSocketClient(cptr_url, cptr_token, i16_pingInterval_ms, i16_maxBufSize, u8_pr, bu8_coreID);

    m_ws->setConnectCB([this](WebSocketClient* ws, bool b) {
        if (!b) {
            if (this->m_ccb) this->m_ccb(this, false);
        } else {
            cl_sio_debug("send introduce");
            /* Send introduce */
            ws->send("2probe", 6);
            ws->send("5", 1);
        }
    });

    m_ws->setCB([this](WebSocketClient* c, char* payload, int length, int type) {
        char eType;

        if (length < 1) return;
        eType = (char)payload[0];

        switch (eType) {
            case SIO_IO_PING:
                payload[0] = SIO_IO_PONG;
                cl_sio_info("PING");
                c->send(payload, length, WS_OP_TXT);
                break;

            case SIO_IO_PONG:
                cl_sio_info("PONG");
                if ((length == 6) && (!strncmp(payload,"3probe", 6))) {
                    cl_sio_info("Da ket noi Websocket :-)");
                    this->send(SIO_MSG_CONNECT, "/", 1);
                }
                break;
            case SIO_IO_MESSAGE: {
                if (length < 2) return;
                char ioType = (char)payload[1];
                char* data = &payload[2];
                int lData = length - 2;
                switch (ioType) {
                    case SIO_MSG_EVENT:
                        cl_sio_debug("Su kien nhan duoc (%d)", lData);
                        if (this->m_cb) this->m_cb(this, data, lData, ioType);
                        if (m_on.size()) {
                            char* k = data, * x;
                            bool lev = false;
                            int len = lData;
                            while ((len > 0) && ((*k == '[') || (*k == '"') || (*k == ' '))) { if (*k == '"') lev = true; k++;len--;}
                            x = k + 1;len--;
                            while ((len > 0) && (*x != '"') && (*x != ',') && ((lev) || (*x != ' '))) { x++; len--; }
                            std::string key(k, (int)(x - k));
                            cl_sio_debug("key (%s)", key.c_str());
                            auto itr = m_on.find(key);
                            if ((len > 1) && (itr != m_on.end())) {
                                x++;len--;
                                x[len - 1] = '\0'; len--;
                                while ((len > 0) && (*x == ',') && (*x != ' ')) { x++; len--; }
                                while ((len > 0) && ((x[len - 1] == ']') || (x[len - 1] == ' '))) { x[len - 1] = '\0'; len--; }
                                if (len > 0) {
                                    //for (; itr != m_on.end(); itr++) {
                                        if(itr != m_on.end()){
                                        itr->second(this, x);
                                    }
                                    //}
                                }
                            }
                        }
                        break;
                    case SIO_MSG_CONNECT:
                        cl_sio_debug("Vao (%d)", lData);
                        if (this->m_ccb) this->m_ccb(this, true);
                        return;
                    case SIO_MSG_DISCONNECT:
                        ESP_LOGI("AnDX","Disconnect Socket IO");
                        break;
                    case SIO_MSG_ACK:
                    {
                        uint32_t id = 0;
                        if (data) {
					        while (*data != '[') {
						        id = id*10 + (*data - 48);
						        data++;
					        }
                            if (this->m_callbackevent) this->m_callbackevent(this, id);
				        }
                        break;
                    }
                        
                    case SIO_MSG_ERROR:
                    case SIO_MSG_BINARY_EV:
                    case SIO_MSG_BINARY_ACK:
                        ESP_LOGI("AnDX","hihi");
                        break;
                    default:
                        ESP_LOGI("ANDX","Kieu %c (%02X) khong duoc trien khai", ioType, ioType);
                        break;
                }
            }
        }
    });
}

//--------------------------------------------------------------------------------------------------------------------------------------------------
SocketIoClient::~SocketIoClient()
{
    if (m_ws) delete m_ws;
}


//--------------------------------------------------------------------------------------------------------------------------------------------------
int SocketIoClient::send(char c_type, const char* cptr_payload, uint32_t u32_length)
{
    uint8_t buf[2] = { SIO_IO_MESSAGE, c_type };
    if (u32_length == 0) {
        u32_length = strlen((const char*)cptr_payload);
    }
    return m_ws->send2((const char*)buf, 2, cptr_payload, u32_length, WS_OP_TXT);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------
int SocketIoClient::emit(const char* cptr_key, const char* cptr_val)
{
    std::string frame = "[\"" + std::string(cptr_key) + "\"," + std::string(cptr_val) + "]";
    std::string payloadHeader = "";
    std::ostringstream ss;
    ss<<SIO_IO_MESSAGE;
    ss<<SIO_MSG_EVENT;
    payloadHeader.append(ss.str());
    return m_ws->send2((const char*)payloadHeader.c_str(), payloadHeader.length(), frame.c_str(), frame.length(), WS_OP_TXT);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------
int SocketIoClient::emit(const char* cptr_key, const char* cptr_val, uint32_t id)
{
    std::string frame = "[\"" + std::string(cptr_key) + "\"," + std::string(cptr_val) + "]";
    std::string payloadHeader = "";
    std::ostringstream ss;
    ss<<SIO_IO_MESSAGE;
    ss<<SIO_MSG_EVENT;
    ss<<id;
    payloadHeader.append(ss.str());
    return m_ws->send2((const char*)payloadHeader.c_str(), payloadHeader.length(), frame.c_str(), frame.length(), WS_OP_TXT);
}
//======================================END FILE===================================================/
