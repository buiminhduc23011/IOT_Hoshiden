/*==================================================================================================
*   Project              :  IOT NIDEC
*   Document             :  framework ESS32-IDF
*   FileName             :  ethernetclient.cpp
*   File Description     :  Định nghĩa hàm sử dụng Ethernet trong ESP32 với chip W5500
==================================================================================================*/
/*==================================================================================================
Revision History:
Modification
    Author                   Date D/M/Y     Description of Changes
----------------------------	----------     ------------------------------------------
    Do Xuan An               08/03/2024     Tạo file
==================================================================================================*/
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <string.h>
#include "ethernetclient.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/spi_master.h"
#include "esp_eth_mac.h"
#include "driver/gpio.h"
#include "esp_eth.h"

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
#define CONFIG_EXAMPLE_ETH_SPI_MISO_GPIO GPIO_NUM_19 // Chân MISO của SPI
#define CONFIG_EXAMPLE_ETH_SPI_MOSI_GPIO GPIO_NUM_23 // Chân MOSI của SPI
#define CONFIG_EXAMPLE_ETH_SPI_SCLK_GPIO GPIO_NUM_18 // Chân SCLK của SPI
#define CONFIG_EXAMPLE_ETH_SPI_CS GPIO_NUM_5         // Chân CS của SPI
#define CONFIG_EXAMPLE_ETH_SPI_RST GPIO_NUM_4        // Chân ngắt của SPI
#define CONFIG_EXAMPLE_ETH_SPI_HOST HSPI_HOST
#define CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ 12 // Tốc độ SPI (10 MHz)
#define CONFIG_EXAMPLE_SPI_ETHERNETS_NUM 1

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
static const char TAG[] = "EthernetClient";
Func_EthernetConnected f_ccb;
static esp_netif_t *s_esp_netif = NULL;

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

static void on_ethernet_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Ethernet disconnected, trying to reconnect...");

    // Nếu cần, có thể thử kết nối lại Ethernet
    if (s_esp_netif)  // Kiểm tra xem esp_netif có hợp lệ không
    {
        esp_netif_action_stop(s_esp_netif, NULL, 0, NULL);  // Dừng kết nối hiện tại
        ESP_LOGI(TAG, "Ethernet stopped. Retrying connection...");
        
        // Tùy chọn: Chờ một khoảng thời gian trước khi thử kết nối lại
        vTaskDelay(pdMS_TO_TICKS(5000));  // Chờ 5 giây trước khi thử lại
        
        // Thử kết nối lại
        esp_netif_action_start(s_esp_netif, NULL, 0, NULL);
        ESP_LOGI(TAG, "Retrying Ethernet connection...");
    }

    if (f_ccb)
    {
        f_ccb(false);  // Gọi callback để thông báo trạng thái ngắt kết nối
    }
}


static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    if (f_ccb)
    {
        f_ccb(true);
    }
}

static esp_netif_t *ethernet_start(void)
{
    // Cấu hình esp_netif cho Ethernet
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    esp_netif_config_t cfg_spi = {
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
    esp_netif_t *eth_netif_spi = esp_netif_new(&cfg_spi);

    // Install GPIO ISR handler to be able to service SPI Eth modules interrupts
    gpio_install_isr_service(0);

    // Khởi tạo SPI bus
    spi_device_handle_t spi_handle = NULL;
    spi_bus_config_t buscfg = {
        .mosi_io_num = CONFIG_EXAMPLE_ETH_SPI_MOSI_GPIO,
        .miso_io_num = CONFIG_EXAMPLE_ETH_SPI_MISO_GPIO,
        .sclk_io_num = CONFIG_EXAMPLE_ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_EXAMPLE_ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Cấu hình SPI device cho W5500
    spi_device_interface_config_t devcfg = {
        .command_bits = 16,                                               // Đặt số bit của phase lệnh
        .address_bits = 8,                                                // Đặt số bit của phase địa chỉ
        .mode = 0,                                                        // SPI mode 0
        .clock_speed_hz = CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ * 1000 * 1000, // Tốc độ SPI
        .spics_io_num = CONFIG_EXAMPLE_ETH_SPI_CS,                        // Chân Chip Select (CS)
        .queue_size = 20,                                                 // Kích thước hàng đợi cho các giao dịch SPI
    };
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_EXAMPLE_ETH_SPI_HOST, &devcfg, &spi_handle));

    // Khởi tạo cấu hình MAC và PHY
    eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();
    phy_config_spi.reset_gpio_num = CONFIG_EXAMPLE_ETH_SPI_RST;

    // Tạo MAC và PHY cho W5500
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle); // Khai báo w5500_config
    esp_eth_mac_t *mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
    esp_eth_phy_t *phy_spi = esp_eth_phy_new_w5500(&phy_config_spi);

    // Cấu hình và khởi động Ethernet driver
    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi, phy_spi);
    esp_eth_handle_t eth_handle_spi = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi));

    // Lấy địa chỉ MAC từ W5500
    uint8_t mac_addr[6] = {0};
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi, ETH_CMD_G_MAC_ADDR, mac_addr));

    // In ra địa chỉ MAC đã lấy được từ W5500
    ESP_LOGI(TAG, "W5500 MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", 
             mac_addr[0], mac_addr[1], mac_addr[2], 
             mac_addr[3], mac_addr[4], mac_addr[5]);

    // attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi)));

    // Đăng ký sự kiện cho Ethernet
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &on_ethernet_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_got_ip, NULL));

    ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi));

    ESP_LOGI(TAG, "Ethernet initialized");
    return eth_netif_spi;
}

static void ethernet_stop(void)
{
    esp_netif_t *ethernet_netif = esp_netif_next(NULL);

    // Hủy đăng ký sự kiện
    ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &on_ethernet_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, &on_got_ip));

    if (ethernet_netif != NULL)
    {
        esp_netif_destroy(ethernet_netif);
    }
}

/*!
 * \brief Kết nối với Ethernet
 */
void Ethernet_Connect(void)
{
    // Khởi tạo Ethernet và esp_netif
    s_esp_netif = ethernet_start();
}




/*!
 * \brief Ngắt kết nối Ethernet
 */
void Ethernet_Disconnect(void)
{
    ethernet_stop();
}

/*!
 * \brief Cài hàm gọi khi kết nối/ngắt kết nối Ethernet
 */
void Ethernet_SetConnectCB(Func_EthernetConnected ccb)
{
    f_ccb = ccb;
}

/*!
 * \brief Lấy địa chỉ IP hiện tại
 */
void GetIP(char **ipPtr)
{
    if (s_esp_netif)
    {
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(s_esp_netif, &ip_info);
        *ipPtr = strdup(ip4addr_ntoa((const ip4_addr_t *)&ip_info.ip));
    }
    else
    {
        *ipPtr = NULL;
    }
}
