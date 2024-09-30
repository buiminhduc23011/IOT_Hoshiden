/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  socket.io
*   FileName             :  socketioclient_api.h
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

#ifndef __SOCKETIOCLIENT_API_H
#define __SOCKETIOCLIENT_API_H 

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "socketioclient.h"
#include <esp_log.h>
#include "datatype.h"
/*==================================================================================================
*                                        FILE VERSION 
==================================================================================================*/

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/
 
/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/

/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
*                                             CLASS
==================================================================================================*/
class SocketIoClientAPI {
    private:
        char sv_url[100];
        SocketIoClient* sio;
        static IOT_Data_t *iot_Data;
        int findFirstSetBit(int number)
        {
            for (int i = 0; i < 16; i++)
            {
                if ((number >> i) & 1)
                {
                    return i + 1;
                }
            }
            return -1;
        }
        void EncodeUrl(const char* IP, uint16_t port)
        {
            sprintf(sv_url,"http://%s:%d",IP,port);
        }
        bool FlagSio;
    public:
        SocketIoClientAPI(){
        }
        ~SocketIoClientAPI(){
            delete(sio);
        }
        /*!
        * \brief Ham cai dat cac ham callback cho socketio
        */
        void initCbFunc(void);
        /*!
        * \brief Ham bat dau tao task hoat dong cho socketio
        */
        void start();
        /*!
        * \brief Ham truyen event
        */
        void SIOClientSendEvent(const char *event, const char *data);
        /*!
        * \brief Ham truyen event
        */
        void SIOClientSendEvent(const char *event, const char *data, uint32_t id);
        /*!
        * \brief Ham bat dau
        */
        void begin(IOT_Data_t *_iot_Data, bool machine_status)
        {
            iot_Data = _iot_Data;
            EncodeUrl(_iot_Data->IpSev, _iot_Data->port);
            char Mac[100];
            if(machine_status)sprintf(Mac,"Mac: %s\r\nIp: %s\r\nFirmware: %s\r\nis-running: true",_iot_Data->Mac,_iot_Data->Ip,_iot_Data->FimwareVer);
            else sprintf(Mac,"Mac: %s\r\nIp: %s\r\nFirmware: %s\r\nis-running: false",_iot_Data->Mac,_iot_Data->Ip,_iot_Data->FimwareVer);
            sio = new SocketIoClient(sv_url,Mac);
            FlagSio = false;
        }
        /*!
        * \brief Ham gui su kien connect IOT
        */
       void SendEventMachineStatus(bool status);
       void SendEventMachineCount(uint32_t _count);
};
#endif /*<!__SOCKETIOCLIENT_API_H>*/
//------------------------------------------END FILE----------------------------------------------//
