/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  The WebSocket Handbook  (PDF)
*   FileName             :  websocketclient.h
*   File Description     :  Khai bao ham su dung websocket trong esp32
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

#ifndef __WEBSOCKET_CLIENT_H
#define __WEBSOCKET_CLIENT_H 

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_transport.h>
#include <esp_transport_tcp.h>
#include <esp_transport_ssl.h>
#include <map>
#include <string>
#include <math.h>
/*==================================================================================================
*                                        FILE VERSION 
==================================================================================================*/

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/
 
/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
// The WebSocket Handbook  (PDF) TRANG 29 - OPCODES - Dinh nghia kieu cua khung du lieu truyen
#define WS_OP_CONT  (0)            // Khung du lieu tiep tuc
#define WS_OP_TXT   (1)            // Khung du lieu van ban (UTF-8)
#define WS_OP_BIN   (2)            // Khung du lieu nhi phan
#define WS_OP_CLOSE (8)            // Khung du lieu dong ket noi
#define WS_OP_PING  (0x9)          // Khung du lieu PING
#define WS_OP_PONG  (0xA)          // Khung du lieu PONG
/*==================================================================================================
*                                             ENUMS
==================================================================================================*/

/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
class WebSocketClient;
typedef std::function<void(WebSocketClient* c, char* msg, int len, int type)> Func_WebSocketCB; // Con tro ham khai bao ham goi khi nhan goi tin websocket
typedef std::function<void(WebSocketClient* c, bool connected)> Func_WebSocketConnectedCB;      // Con tro ham khai bao ham goi khi ket noi thanh cong websocket
typedef std::function<void(WebSocketClient* c, char* msg, int len)> Func_WebSocketON;           // Con tro ham khai bao ham goi khi nhan event websocket
/*==================================================================================================
*                                             CLASS
==================================================================================================*/
class WebSocketClient {
public:
    /*!
     * \brief Ham tao cua lop WebSocketClient
     * \param cptr_url - WebSocket url
     * \param cptr_token - Mã ủy quyền (Chu ky) The WebSocket Handbook  (PDF) TRANG 77 - RFC7519
     * \param i16_pingInterval_ms - Thoi gian timeout ping den may chu cai dat theo ms
     * \param i16_maxBufSize - Do lon bo dem truyen/nhan tinh theo byte
     * \param u8_pr - Do uu tien khi su dung task trong freertos
     * \param bu8_coreID - Core cua esp32 su dung de chay cho task nay . tskNO_AFFINITY nghia la chay tren ca 2 task
     */
    WebSocketClient(const char* cptr_url, const char* cptr_token = NULL, int i16_pingInterval_ms = 10000, int i16_maxBufSize = 1024, uint8_t u8_pr = 5, BaseType_t bu8_coreID = 0);
    ~WebSocketClient();

    /*!
     * \brief Truyen du lieu websocket
     * \param cptr_msg - Con tro goi tin truyen di
     * \param u32_size - Do dai goi tin truyen di tinh theo byte
     * \param i16_type - Dinh dang khung du lieu . Mac dinh la truyen text
     */
    int send(const char* cptr_msg, uint32_t u32_size, int i16_type = WS_OP_TXT);

    /*!
     * \brief Truyen du lieu websocket 2 goi du lieu
     * \param cptr_msg0 - Con tro du lieu 0
     * \param u32_size0 - Do dai du lieu 0
     * \param cptr_msg1 - Con tro du lieu 1
     * \param u32_size1 - Do dai du lieu 1
     * \param i16_type - Dinh dang khung du lieu . Mac dinh la truyen text
     */
    int send2(const char* cptr_msg0, uint32_t u32_size0, const char* cptr_msg1, uint32_t u32_size1, int i16_type = WS_OP_TXT);


    /*!
     * \brief Cai dat ham phan hoi khi khoi tao websocket
     */
    void setCB(Func_WebSocketCB cb) { m_cb = cb; }

    /*!
     * \brief Cai dat ham phan hoi khi ket noi hoac ngat ket noi websocket
     */
    void setConnectCB(Func_WebSocketConnectedCB ccb) { m_ccb = ccb; }

    /*!
     * \brief Ham tao task de chay websocket
     */
    void start();

    /*!
     * \brief Ham ket thuc task chay websocket
     */
    void stop();

    /*!
     * \brief Ham chay trong task
     */
    void run();

    /*!
     * \brief Ham cai dat timeout ping
     */
    void setPingInterval(int ms) { m_ping_interval = ms; }
    /*!
     * \brief Ham kiem tra thoi gian timeout khi ping
     */
    int  getPingInterval() const { return m_ping_interval; }

    /*!
     * \brief Ham cai dat timeout ket noi lai
     */
    void setReconnectInterval(int ms) { m_reconnectInterval = ms; }

    /*!
     * \brief Ham kiem tra timeout ket noi lai
     */
    int  getReconnectInterval() const { return m_reconnectInterval; }
    
    /*!
     * \brief Ham cai dat timeout ket noi
     */
    void setConnectTimeout(int ms) { m_connectTimeout = ms; }

    /*!
     * \brief Ham kiem tra thoi gian timeout khi ket noi
     */
    int  getConnectTimeout() const { return m_connectTimeout; }
    
    /*!
     * \brief Ham cai dat timeout truyen goi tin
     */
    void setWriteTimeout(int ms) { m_writeTimeout = ms; }

    /*!
     * \brief Ham kiem tra timeout truyen goi tin
     */
    int  getWriteTimeout() const { return m_writeTimeout; }
    
    /*!
     * \brief Ham cai dat timeout nhan goi tin
     */
    void setReadTimeout(int ms) { m_readTimeout = ms; }

    /*!
     * \brief Ham kiem tra timeout nhan goi tin
     */
    int  getReadTimeout() const { return m_readTimeout; }
    
    /*!
     * \brief Ham kiem tra ket noi
     */
    bool isConnected() const { return m_connected; }

    /*!
     * \brief Ham kiem tra su kien duoc gui den 
     */
    void on(std::string what, Func_WebSocketON cb) {
        m_on.insert({ what, cb });
    }

    /*!
     * \brief Ham xoa su kien duoc gui den 
     */
    void off(std::string what) {
        m_on.erase(what);
    }

private:
    int wsConnect(int timeout_ms = 10000);    // Ket noi websocket
    void wsParseURL();                        // Phan tach URL
    int wsFeedFrame();                      // Tao khung truyen
    int wsOnFrame();                        // Tao khung su kien
    int wsSendPing();                         // Tao khung truyn ping
    int CountPong;
    uint32_t ToInt32(char* _string);
public:
    esp_transport_handle_t m_tr;
    // --------------------------------------- May chu
    char* m_url;
    char* m_token;
    int               m_port;
    bool              m_ssl;
    bool              m_sio;
    int               m_sio_v;
    const char* m_path;
    const char* m_host;
    // --------------------------------------- Thoi gian timne out
    int               m_reconnectInterval;
    int               m_connectTimeout;
    int               m_writeTimeout;
    int               m_readTimeout;
    // --------------------------------------- Bo dem du lieu truyen/nhan
    int               m_maxBuf;
    int               m_maxBufC;
    char             *rx_buf;
    int               line_begin;           
    int               line_pos;            
    int               line_end;             
    char             *tx_buf;
    // --------------------------------------- Cac truong trong khung truyen websocket  - The WebSocket Handbook  (PDF) TRANG 27 - Message frames
    uint8_t           ws_frame_type;        /*!< OPCODE        */
    uint8_t           ws_is_fin;            /*!< FIN           */
    uint8_t           ws_is_mask;           /*!< MASK          */
    uint8_t           ws_is_ctrl;           /*!< CTRL          */
    int               ws_header_size;       /*!< Do dai header */
    int               ws_frame_size;        /*!< Do dai khung  */
    char             *ws_msg;               /*!< Con tro du lieu */
    /* Ping/Pong */
    int               m_ping_interval;
    int               ws_ping_cnt;          /*!< So lan ping              */
    int               ws_pong_cnt;          /*!< So lan pong             */
    //----------------------------------------
    bool              m_connected;
    uint32_t timeoutPingInterval;
    Func_WebSocketCB     m_cb;
    Func_WebSocketConnectedCB m_ccb;
    std::map<std::string, Func_WebSocketON> m_on;
    SemaphoreHandle_t m_lock;
    //----------------------------------------- FreeRtos
    xTaskHandle       m_handle;
    uint16_t          m_stackSize;
    uint8_t           m_priority;
    BaseType_t        m_coreId;
};
#endif /*<!__WEBSOCKET_CLIENT_H>*/
//------------------------------------------END FILE----------------------------------------------//
