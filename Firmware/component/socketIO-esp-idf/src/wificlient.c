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
#include <string.h>
#include "wificlient.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
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
static char TAG[] = "WifiClient";
const char *cptr_ssid = NULL;
const char *cptr_pass = NULL;
const char *cptr_host = NULL;
char *ptr_Ip = NULL;
char *ptr_Mac = NULL;
Func_WifiConnected f_ccb;
static esp_netif_t *s_esp_netif = NULL;
uint8_t conf_bssid[6];
#define DEFAULT_SCAN_LIST_SIZE 10
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

static bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED)
    {
        return;
    }
    ESP_ERROR_CHECK(err);
    f_ccb(false);
}

static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!is_our_netif(TAG, event->esp_netif))
    {
        ESP_LOGW(TAG, "Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    sprintf(ptr_Ip, IPSTR, IP2STR(&event->ip_info.ip));
    f_ccb(true);
}

esp_netif_t *get_example_netif_from_desc(const char *desc)
{
    esp_netif_t *netif = NULL;
    char *expected_desc;
    asprintf(&expected_desc, "%s: %s", TAG, desc);
    while ((netif = esp_netif_next(netif)) != NULL)
    {
        if (strcmp(esp_netif_get_desc(netif), expected_desc) == 0)
        {
            free(expected_desc);
            return netif;
        }
    }
    free(expected_desc);
    return netif;
}

void wifi_scan(char *ssid)
{
    int8_t rssi_val = -100;
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGI(TAG, "Tong so AP tim thay  = %u ", ap_count);

    for (int i = 0; i < number; i++)
    {
        if (strcmp(ssid, (char *)ap_info[i].ssid) == 0)
        {
            if (ap_info[i].rssi > rssi_val)
            {
                rssi_val = ap_info[i].rssi;
                for (int j = 0; j < 6; j++)
                {
                    conf_bssid[j] = ap_info[i].bssid[j];
                }
            }
        }
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
}

static esp_netif_t *wifi_start(void)
{
    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_scan(cptr_ssid);
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;
    esp_netif_t *netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    if (cptr_ssid)
    {
        strncpy((char *)wifi_config.sta.ssid, cptr_ssid, strlen(cptr_ssid));
    }
    if (cptr_pass)
    {
        strncpy((char *)wifi_config.sta.password, cptr_pass, strlen(cptr_pass));
    }
    for (int i = 0; i < 6; i++)
    {
        wifi_config.sta.bssid[i] = conf_bssid[i];
    }
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
    return netif;
}

static void wifi_stop(void)
{
    esp_netif_t *wifi_netif = get_example_netif_from_desc("sta");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT)
    {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);
}

static void stop(void)
{
    wifi_stop();
}

/*!
 * \brief Ket noi voi wifi
 */
void Wifi_Connect(const char *ssid, const char *pass, const char *host)
{
    uint8_t MacBase[6];
    cptr_ssid = strdup(ssid);
    cptr_pass = strdup(pass);
    cptr_host = strdup(host);
    ptr_Ip = (char *)malloc(20 * sizeof(char));
    ptr_Mac = (char *)malloc(18 * sizeof(char));
    if (ssid)
    {
        strncpy(cptr_ssid, ssid, strlen(ssid));
    }
    if (pass)
    {
        strncpy(cptr_pass, pass, strlen(pass));
    }
    s_esp_netif = wifi_start();
    ESP_ERROR_CHECK(esp_netif_set_hostname(s_esp_netif, cptr_host));
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&stop));
    esp_err_t err = esp_read_mac(MacBase, ESP_MAC_WIFI_STA);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get base MAC address from efuse. (%s)", esp_err_to_name(err));
    }
    else
    {
        snprintf(ptr_Mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X", MacBase[0], MacBase[1], MacBase[2], MacBase[3], MacBase[4], MacBase[5]);
    }
    ESP_LOGI(TAG, "Wifi Connected");
}

/*!
 * \brief Ngat ket noi wifi
 */
void Wifi_Disconnect(void)
{
    if (s_esp_netif != NULL)
    {
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
        esp_err_t err = esp_wifi_stop();
        ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&stop));
        if (err == ESP_ERR_WIFI_NOT_INIT)
        {
            return;
        }
        ESP_ERROR_CHECK(err);
        ESP_ERROR_CHECK(esp_wifi_deinit());
        ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(s_esp_netif));
        esp_netif_destroy(s_esp_netif);
        s_esp_netif = NULL;
        ESP_LOGI(TAG, "Disconnected from %s", cptr_ssid);
    }
}

/*!
 * \brief Cai ham goi khi ket noi/ ngat ket noi wifi
 */
void Wifi_SetConnectCB(Func_WifiConnected ccb)
{
    f_ccb = ccb;
}

//---------------------------------------------------------------------------------------------------------------
void GetIP(char **ipPtr)
{
    if (ptr_Ip)
        *ipPtr = ptr_Ip;
    else
        return;
}
//---------------------------------------------------------------------------------------------------------------
void GetMac(char **macPtr)
{
    if (ptr_Mac)
        *macPtr = ptr_Mac;
    else
        return;
}
int16_t GetRssi(void)
{
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_OK)
    {
        return ap_info.rssi;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get RSSI. Error: %s", esp_err_to_name(err));
        return 0;
    }
}
//---------------------------------------------------------------------------------------------------------------

//======================================END FILE===================================================/
