/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  ESP32 platform
*   FileName             :  eeprom.h
*   File Description     :  Khai bao ham su dung nvs flash trong esp32
*
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification     
    Author                  	Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    Do Xuan An              	13/03/2024     Tao file
----------------------------	----------     ------------------------------------------
==================================================================================================*/

#ifndef __EEPROM_H
#define __EEPROM_H 

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
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
class Eeprom {
    private:
        const char* _name;
        nvs_handle_t my_handle;
    public:
        Eeprom(const char* name = "storage") : _name(name), my_handle(0) {};
        ~Eeprom() {end();}
        /*!
        * \brief Ham bat dau
        */
        void begin(size_t size);

        /*!
        * \brief Ham dua giu lieu vao bo nho
        */
        void commit(void);

        /*!
        * \brief Ham ket thuc viec ghi du lieu
        */
        void end(void);
        /*!
        * \brief Ghi voi du lieu bat ky . Chi dung cho các kieu giu lieu khong chua con trỏ và kiểu dữ liệu phức tạp
        */
        //template <class T> T writeBase (int address, const T &);
        /*!
        * \brief Ghi voi du lieu bat ky . Chi dung cho các kieu giu lieu khong chua con trỏ và các kiểu dữ liệu phức tạp
        */
        //template <class T> T readBase(int address, T &);
        /*!
        * \brief Ghi voi du lieu string
        */
        int writeString(char* namefield, char* value);
        /*!
        * \brief Doc du lieu string
        */
        int readString(char* namefield, char* value);
        /*!
        * \brief Ghi du lieu uint16_t
        */
        bool writeUint16 (char* namefield, uint16_t value);
        /*!
        * \brief Đoc du lieu uint16_t
        */
        bool readUint16 (char* namefield, uint16_t* value);
        
};
#endif /*<!__EEPROM_H>*/
//------------------------------------------END FILE----------------------------------------------//
