/*==================================================================================================
*   Project              :  IOT GUNZE
*   Doccument            :  ESP32 Ethernet
*   FileName             :  Ethernet.c
*   File Description     :  Khai bao cac ham nap MB Master
*
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification
    Author                  	Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    BÙI VĂN ĐỨC              	25/11/2024     Tao file
----------------------------	----------     ------------------------------------------
==================================================================================================*/

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <Ethernet.h>
#include <stdbool.h>
#include "esp_err.h"
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
#define ESP_GOTO_ON_FALSE(a, err_code, label, tag, message) \
    do                                                      \
    {                                                       \
        if (!(a))                                           \
        {                                                   \
            ESP_LOGE(tag, "%s", message);                   \
            ret = err_code;                                 \
            goto label;                                     \
        }                                                   \
    } while (0)

#define ESP_GOTO_ON_ERROR(a, label, tag, message)         \
    do                                                    \
    {                                                     \
        esp_err_t err_rc_ = (a);                          \
        if (err_rc_ != ESP_OK)                            \
        {                                                 \
            ESP_LOGE(tag, "%s (0x%x)", message, err_rc_); \
            ret = err_rc_;                                \
            goto label;                                   \
        }                                                 \
    } while (0)

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/
/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
static char TAG[] = "ETHERNET-TASK";

static bool is_connected = false;
static char ip_address[16] = "0.0.0.0";
static char mac_address[18] = "00:00:00:00:00:00";

//---------------------------------------------------------------------------------------------------

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
/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        snprintf(mac_address, sizeof(mac_address), "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        is_connected = true;
        ESP_LOGI(TAG, "Ethernet Connected");
        ESP_LOGI(TAG, "Ethernet HW Addr %s", mac_address);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        is_connected = false;
        ESP_LOGI(TAG, "Ethernet Disconnected");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        ESP_LOGI(TAG, "Waiting for IP Address Allocation..............");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    snprintf(ip_address, sizeof(ip_address), IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "-----------------------");
    ESP_LOGI(TAG, "IP: %s", ip_address);
    ESP_LOGI(TAG, "NETMASK: " IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "GATEWAY: " IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "-----------------------");
}

/** Function to get Ethernet connection status */
bool Eth_getConnectionStatus(void)
{
    return is_connected;
}

/** Function to get Ethernet IP address */
char *Eth_getIPAddress(void)
{
    return ip_address;
}

/** Function to get Ethernet MAC address */
char *Eth_getMACAddress(void)
{
    return mac_address;
}
esp_err_t ETH_W5500_Set_Static_IP(esp_eth_handle_t eth_handle, int eth_port_index)
{
    esp_err_t ret = ESP_OK; // Biến lưu mã lỗi trả về

    ESP_LOGI(TAG, "Creating esp_netif...");
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_inherent_config_t base_cfg = *(cfg.base); // Tạo bản sao cấu hình mặc định

    // Gán if_key duy nhất
    char if_key_str[16];
    snprintf(if_key_str, sizeof(if_key_str), "ETH_%d", eth_port_index);
    base_cfg.if_key = if_key_str; // Sửa if_key trong bản sao

    cfg.base = &base_cfg; // Gắn lại cấu hình cơ bản vào cfg

    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    ESP_GOTO_ON_FALSE(eth_netif != NULL, ESP_FAIL, err, TAG, "Failed to create netif");
    ESP_LOGI(TAG, "esp_netif created successfully with key: %s", if_key_str);

    // Gắn driver Ethernet vào esp_netif
    ESP_LOGI(TAG, "Attaching driver to esp_netif...");
    ESP_GOTO_ON_ERROR(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)), err, TAG, "Failed to attach netif");
    ESP_LOGI(TAG, "Driver attached successfully");

    // Cấu hình IP tĩnh
    ESP_LOGI(TAG, "Configuring static IP...");
    esp_netif_ip_info_t ip_info = {
        .ip = {.addr = ESP_IP4TOADDR(192, 168, 0, 10 + eth_port_index)}, // Thay đổi IP dựa trên eth_port_index
        .netmask = {.addr = ESP_IP4TOADDR(255, 255, 255, 0)},             // Subnet mask
        .gw = {.addr = ESP_IP4TOADDR(0, 0, 0, 0)}                         // Gateway không cần thiết
    };
    ESP_GOTO_ON_ERROR(esp_netif_dhcpc_stop(eth_netif), err, TAG, "Failed to stop DHCP client"); // Tắt DHCP
    ESP_LOGI(TAG, "DHCP client stopped successfully");
    ESP_GOTO_ON_ERROR(esp_netif_set_ip_info(eth_netif, &ip_info), err, TAG, "Failed to set IP info");
    ESP_LOGI(TAG, "Static IP configured: IP: " IPSTR ", GW: " IPSTR ", NM: " IPSTR,
             IP2STR(&ip_info.ip), IP2STR(&ip_info.gw), IP2STR(&ip_info.netmask));

    return ESP_OK;

err:
    if (eth_netif)
    {
        esp_netif_destroy(eth_netif);
    }
    return ret;
}

void Eth_Start(void)
{
    // Initialize Ethernet driver
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(ETH_W5500_Init(&eth_handles, &eth_port_cnt));

    esp_netif_t *eth_netifs[eth_port_cnt];
    esp_eth_netif_glue_handle_t eth_netif_glues[eth_port_cnt];

    // Create instance(s) of esp-netif for Ethernet(s)
    if (eth_port_cnt == 1)
    {
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netifs[0] = esp_netif_new(&cfg);
        eth_netif_glues[0] = esp_eth_new_netif_glue(eth_handles[0]);
        ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[0], eth_netif_glues[0]));
        // Set static IP for each interface
        esp_err_t ret = ETH_W5500_Set_Static_IP(eth_handles[0], 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set static IP: %d", ret);
        }
    }
    else
    {
        esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        esp_netif_config_t cfg_spi = {
            .base = &esp_netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
        char if_key_str[10];
        char if_desc_str[10];
        char num_str[3];
        for (int i = 0; i < eth_port_cnt; i++)
        {
            itoa(i, num_str, 10);
            strcat(strcpy(if_key_str, "ETH_"), num_str);
            strcat(strcpy(if_desc_str, "eth"), num_str);
            esp_netif_config.if_key = if_key_str;
            esp_netif_config.if_desc = if_desc_str;
            esp_netif_config.route_prio -= i * 5;
            eth_netifs[i] = esp_netif_new(&cfg_spi);
            eth_netif_glues[i] = esp_eth_new_netif_glue(eth_handles[0]);
            ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[i], eth_netif_glues[i]));
            // Set static IP for each interface
            esp_err_t ret = ETH_W5500_Set_Static_IP(eth_handles[i],i);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set static IP: %d", ret);
            }
        }
    }
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    for (int i = 0; i < eth_port_cnt; i++)
    {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }
}

//======================================END FILE===================================================
