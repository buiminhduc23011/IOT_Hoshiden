/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  framework ESS32-IDF 
*   FileName             :  wificlient.cpp
*   File Description     :  Dinh nghia ham su dung wifi trong esp32
*
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification     
    Author                  	Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    Do Xuan An              	08/03/2024     Tao file
----------------------------	----------     ------------------------------------------
==================================================================================================*/
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
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
static uint16_t FlagSource;
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
void FLAG_Init(void)
{
    FlagSource = FLAG_NONE;
}

void FLAG_SetFlag(enum FLAG_t flag)
{
    FlagSource |= flag;
}

void FLAG_ClearFlag(enum FLAG_t flag)
{
    FlagSource &= (~flag);
}

bool FLAG_GetFlag(enum FLAG_t flag)
{
    return (FlagSource&flag)?true:false;
}

//======================================END FILE===================================================/
