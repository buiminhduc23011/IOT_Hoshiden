/*==================================================================================================
*   Project              :  IOT NIDEC
*   Document             :  framework ESS32-IDF 
*   FileName             :  ethernetclient.h
*   File Description     :  Khai báo hàm sử dụng Ethernet trong ESP32 với chip W5500
*
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification     
    Author                  	Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    Do Xuan An              	08/03/2024     Tạo file
    Do Xuan An              	06/10/2024     Cập nhật các hàm và bổ sung tính năng
----------------------------	----------     ------------------------------------------
==================================================================================================*/

#ifndef __ETHERNET_CLIENT_H
#define __ETHERNET_CLIENT_H 

#ifdef __cplusplus
extern "C"
{
#endif
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "esp_err.h"
#include "esp_netif.h"
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
typedef void (*Func_EthernetConnected)(bool connected); // Con trỏ hàm khai báo hàm gọi khi kết nối/ngắt kết nối

/*==================================================================================================
*                                             CLASS
==================================================================================================*/
/*!
 * \brief Kết nối với Ethernet
 * \param[in] ip       Địa chỉ IP của thiết bị
 * \param[in] netmask  Subnet mask
 * \param[in] gateway  Địa chỉ Gateway
 */
void Ethernet_Connect(void);

/*!
 * \brief Ngắt kết nối Ethernet
 */
void Ethernet_Disconnect(void);

/*!
 * \brief Cài hàm gọi khi kết nối/ngắt kết nối Ethernet
 * \param[in] ccb   Hàm callback khi kết nối hoặc ngắt kết nối Ethernet
 */
void Ethernet_SetConnectCB(Func_EthernetConnected ccb);

/*!
 * \brief Hàm lấy địa chỉ IP hiện tại của thiết bị Ethernet
 * \param[out] ipPtr  Trỏ đến chuỗi chứa địa chỉ IP, cần phải giải phóng sau khi sử dụng.
 */
void GetEthernetIP(char** ipPtr);

/*!
 * \brief Hàm lấy địa chỉ MAC của thiết bị Ethernet
 * \param[out] macPtr  Trỏ đến chuỗi chứa địa chỉ MAC, cần phải giải phóng sau khi sử dụng.
 */
void GetEthernetMac(char** macPtr);

#ifdef __cplusplus
}
#endif

#endif /* __ETHERNET_CLIENT_H */
//------------------------------------------END FILE----------------------------------------------//
