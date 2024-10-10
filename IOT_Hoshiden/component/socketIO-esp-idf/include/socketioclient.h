/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  socket.io
*   FileName             :  socketioclient.h
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

#ifndef __SOCKETIO_CLIENT_H
#define __SOCKETIO_CLIENT_H 

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include "websocketclient.h"
#include <map>
#include <string>
#include <list>
/*==================================================================================================
*                                        FILE VERSION 
==================================================================================================*/

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/
 
/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
class SocketIoClient;

#define SIO_IO_OPEN         '0'    
#define SIO_IO_CLOSE        '1'    
#define SIO_IO_PING         '2'    
#define SIO_IO_PONG         '3'    
#define SIO_IO_MESSAGE      '4'    
#define SIO_IO_UPGRADE      '5'   
#define SIO_IO_NOOP         '6'    

#define SIO_MSG_CONNECT     '0'
#define SIO_MSG_DISCONNECT  '1'
#define SIO_MSG_EVENT       '2'
#define SIO_MSG_ACK         '3'
#define SIO_MSG_ERROR       '4'
#define SIO_MSG_BINARY_EV   '5'
#define SIO_MSG_BINARY_ACK  '6'
/*==================================================================================================
*                                             ENUMS
==================================================================================================*/

/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef std::function<void(SocketIoClient* c, const char* msg, int len, int type)> Func_SIOCB;       // Con tro ham khai bao ham goi khi nhan goi tin socketio
typedef std::function<void(SocketIoClient* c, bool connected)> Func_SIOConnectedCB;                  // Con tro ham khai bao ham goi khi ket noi thanh cong socketio
typedef std::function<void(SocketIoClient* c, char* msg)> Func_SIOON;   
typedef std::function<void(SocketIoClient* c, uint32_t id)> Func_CallBackEvent;                                // Con tro ham khai bao ham goi khi nhan event socketio

template<typename T, typename... U>
size_t getAddress(std::function<T(U...)> f) {
    typedef T(fnType)(U...);
    fnType** fnPointer = f.template target<fnType*>();
    return (size_t)*fnPointer;
}
/*==================================================================================================
*                                             CLASS
==================================================================================================*/
class SocketIoClient {
public:
    /*!
     * \brief Ham kho tao lop SocketIoClient
     * \param cptr_url - URL may chu . Vi du : http://192.168.1.86:6868
     * \param cptr_token - Chu ky
     * \param i16_pingInterval_ms - Khoang thoi gian cho ket noi theo ms mac dinh 10000
     * \param i16_maxBufSize - Cau hinh do lon khung truyen . Mac dinh 1024
     * \param u8_pr - Do uu tien trong task FreeRtos
     * \param bu8_coreID - Core cua esp32 su dung de chay task SocketIO
     */
    SocketIoClient(const char* cptr_url, const char* cptr_token = NULL, int i16_pingInterval_ms = 10000, int i16_maxBufSize = 1024, uint8_t u8_pr = 5, BaseType_t bu8_coreID = 0);
    ~SocketIoClient();

    /*!
     * \brief Gui khung khung truyen SocketIO
     * \param c_type - Kieu khung
     * \param cptr_payload - Con tro du lieu
     * \param u32_length - Do dai du lieu
     */
    int send(char c_type, const char* cptr_payload, uint32_t u32_length);

    /*!
     * \brief Gui du lieu theo ten event
     * \param cptr_key - Ten event
     * \param cptr_val - Du lieu truyen
     */
    int emit(const char* cptr_key, const char* cptr_val);

    /*!
     * \brief Gui du lieu theo ten event
     * \param cptr_key - Ten event
     * \param cptr_val - Du lieu truyen
     * \param id       - Id cua ham call back
     */
    int emit(const char* cptr_key, const char* cptr_val, uint32_t id);
    /*!
     * \brief Cai ham goi khi khoi tao SocketIO
     */
    void setCB(Func_SIOCB cb) { m_cb = cb; }

    /*!
     * \brief Cai ham goi khi ket noi SocketIO
     */
    void setConnectCB(Func_SIOConnectedCB ccb) { m_ccb = ccb; }
    
    void seertEventFuncCallBack(Func_CallBackEvent fccb) {m_callbackevent = fccb;}
    /*!
     * \brief Ham tao task chay websocket
     */
    void start() { m_ws->start(); }
    
    /*!
     * \brief Ham nhan su kien va goi ham xu ly khi nhan dung ten su kien truyen vao
     */
    void on(std::string what, Func_SIOON cb) {
        m_on.insert({ what, cb });
    }

    /*!
     * \brief Ham xoa su kien
     */
    void off(std::string what) {
        std::multimap<std::string, Func_SIOON>::iterator itr;
        std::list<std::multimap<std::string, Func_SIOON>::iterator> l;
        for (itr = m_on.find(what); itr != m_on.end(); itr++) {
            l.push_back(itr);
        }
        for (auto i = l.begin(); i != l.end(); i++) {
            m_on.erase(*i);
        }
    }   
private:
    
public:
    WebSocketClient* m_ws;
    Func_SIOCB                        m_cb;
    Func_SIOConnectedCB               m_ccb;
    Func_CallBackEvent                m_callbackevent;
    std::multimap<std::string, Func_SIOON> m_on;
};
#endif /*<!__SOCKETIO_CLIENT_H>*/
//------------------------------------------END FILE----------------------------------------------//
