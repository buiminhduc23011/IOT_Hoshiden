/*==================================================================================================
*   Project              :  IOT NIDEC
*   Doccument            :  framework ESP32-IDF
*   FileName             :  TCP.c
*   File Description     :  TCP server/client loopback over W5500 using SPI
*
==================================================================================================*/
/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "Application/loopback/loopback.h"
#include "TCP.h"
#include "flag.h"
#include "IO.h"

/*==================================================================================================
*                                   FUNCTION PROTOTYPES
==================================================================================================*/
spi_device_handle_t spi;

#define SOCKET_LOOPBACK 0
#define PORT_LOOPBACK 5000
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)
#define _LOOPBACK_DEBUG1_
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDE, 0x12, 0x34, 0x57}, // MAC address
        .ip = {192, 168, 0, 10},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 0, 1},                      // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_DHCP                         // DHCP enable/disable
};
static wiz_NetInfo temp_g_net_info;
static uint8_t g_loopback_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};

void wizchip_initialize(void);
void network_initialize(wiz_NetInfo net_info1);
void print_network_information(wiz_NetInfo net_info);
// void print_network_information();
int32_t loopback_tcps1(uint8_t sn, uint8_t *buf, uint16_t port);
/*
 This code demonstrates how to use the SPI master half duplex mode to read/write a AT932C46D EEPROM (8-bit mode).
*/
#define WIZNET_HOST VSPI_HOST // HSPI_HOST//VSPI_HOST
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5
// #define PIN_NUM_CS 14
#define PIN_NUM_INT 4
#define PIN_NUM_RESET 16
int32_t loopback_ret = 0;

bool IsConnectCam = false;

bool ConnectCam(void)
{
    return IsConnectCam;
}
void wiznet_spi_pre_transfer_callback(spi_transaction_t *t)
{
    // int dc=(int)t->user;
    // gpio_set_level(PIN_NUM_DC, dc);
}

static char TAG[] = "TCP";
char *QR_Code;
char *GetQRcode(void)
{
    if (QR_Code != NULL)
    {
        return QR_Code; // Trả về chuỗi QR_Code nếu không NULL
    }
    else
    {
        return NULL; // Trả về NULL nếu QR_Code chưa có dữ liệu
    }
}
void reset_QRcode(void)
{
    QR_Code = NULL;
}
TaskHandle_t TCPTaskHandle;
void Wizchip_write_buf(uint8_t *AddrSel, uint8_t *value, uint16_t len)
{
    esp_err_t ret = ESP_OK;
    spi_transaction_t trans = {
        //.flags = len <= 4 ? SPI_TRANS_USE_TXDATA : 0,
        .cmd = (AddrSel[0] << 8) | (AddrSel[1] & 0x00FF), // 0xf0,//(address >> W5500_ADDR_OFFSET),
        .addr = AddrSel[2],                               //(address & 0xFFFF),
        .length = 8 * len,
        .tx_buffer = value};

    if (spi_device_polling_transmit(spi, &trans) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
        ret = ESP_FAIL;
    }
}

void Wizchip_read_buf(uint8_t *AddrSel, uint8_t *value, uint16_t len)
{
    esp_err_t ret = ESP_OK;
    spi_transaction_t trans = {
        .flags = len <= 4 ? SPI_TRANS_USE_RXDATA : 0,     // use direct reads for registers to prevent overwrites by 4-byte boundary writes
        .cmd = (AddrSel[0] << 8) | (AddrSel[1] & 0x00FF), // 0xf0,//(address >> W5500_ADDR_OFFSET),
        .addr = AddrSel[2],                               //(address & 0xFFFF),
        .length = 8 * len,
        .rx_buffer = value};

    if (spi_device_polling_transmit(spi, &trans) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s(%d): spi transmit failed", __FUNCTION__, __LINE__);
        ret = ESP_FAIL;
    }

    if ((trans.flags & SPI_TRANS_USE_RXDATA) && len <= 4)
    {
        memcpy(value, trans.rx_data, len); // copy register values to output
    }
    // spi_device_release_bus(spi);
    // return ret;
}

void reset_pin_set(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = 1 << PIN_NUM_RESET; // GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 1;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
}

void TCP_Init(void)
{
    esp_err_t ret;
    // spi_device_handle_t spi;
    int i = 0;

    uint8_t temp_recv;
    uint8_t temp_Addr[3];
    uint16_t temp_spi_clk = 20;

    ESP_LOGI(TAG, "Wiznet W5500 TEST \r\n");

    ESP_LOGI(TAG, "Initializing bus SPI%d...", WIZNET_HOST + 1);
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    spi_device_interface_config_t devcfg = {

        .clock_speed_hz = temp_spi_clk * 1000 * 1000, // Clock out at 10 MHz
        .address_bits = 8,
        .command_bits = 16,
        .mode = 0,                                  // SPI mode 0
        .spics_io_num = PIN_NUM_CS,                 // CS pin
        .queue_size = 7,                            // We want to be able to queue 7 transactions at a time
        .pre_cb = wiznet_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
    };
    reset_pin_set();
    gpio_set_level(PIN_NUM_RESET, 0);
    // Initialize the SPI bus
    ret = spi_bus_initialize(WIZNET_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(WIZNET_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    // ESP_LOGI(TAG, "app clk = %d hz", esp_clk_apb_freq());
    // ESP_LOGI(TAG, "SPI CLK = %d Mhz\r\n", temp_spi_clk);
    vTaskDelay(50);
    gpio_set_level(PIN_NUM_RESET, 1);
    vTaskDelay(20);

    temp_Addr[0] = 0x00;
    temp_Addr[1] = 0x39;
    temp_Addr[2] = 0x00;
    Wizchip_read_buf(temp_Addr, &temp_recv, sizeof(temp_recv));
    ESP_LOGI(TAG, "W5500 VER =%02X \r\n", temp_recv);

    wizchip_initialize();
    network_initialize(g_net_info);
    // print_network_information(g_net_info);
    print_network_information(temp_g_net_info);

    // print_network_information();
    ESP_LOGI(TAG, "socket mem size");
    for (i = 0; i < 8; i++)
    {
        ESP_LOGI(TAG, "%d - Rx:%02X Tx:%02X ", i, getSn_TXBUF_SIZE(i), getSn_RXBUF_SIZE(i));
    }
    ESP_LOGI(TAG, "socket 0 status = %02X", getSn_SR(0));

    ESP_LOGI(TAG, "loopback start");
    TCP_Run();
}

void wizchip_initialize(void)
{

    reg_wizchip_spiburst_cbfunc(Wizchip_read_buf, Wizchip_write_buf);

    uint8_t memsize[2][8] = {{2, 2, 2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2, 2, 2}};

    if (ctlwizchip(CW_INIT_WIZCHIP, (void *)memsize) == -1)
    {
        ESP_LOGI(TAG, " W5x00 initialized fail\n");

        return;
    }
}
void network_initialize(wiz_NetInfo net_info1)
{
    ctlnetwork(CN_SET_NETINFO, (void *)&net_info1);
}

void print_network_information(wiz_NetInfo net_info)
// void print_network_information()
{
    // wiz_NetInfo net_info = {0,};
    uint8_t tmp_str[8] = {
        0,
    };

    ctlnetwork(CN_GET_NETINFO, (void *)&net_info);
    ctlwizchip(CW_GET_ID, (void *)tmp_str);

    if (net_info.dhcp == NETINFO_DHCP)
    {
        ESP_LOGI(TAG, "====================================================================================================\n");
        ESP_LOGI(TAG, " %s network configuration : DHCP\n\n", (char *)tmp_str);
    }
    else
    {
        ESP_LOGI(TAG, "====================================================================================================\n");
        ESP_LOGI(TAG, " %s network configuration : static\n\n", (char *)tmp_str);
    }

    ESP_LOGI(TAG, " MAC         : %02X:%02X:%02X:%02X:%02X:%02X\n", net_info.mac[0], net_info.mac[1], net_info.mac[2], net_info.mac[3], net_info.mac[4], net_info.mac[5]);
    ESP_LOGI(TAG, " IP          : %d.%d.%d.%d\n", net_info.ip[0], net_info.ip[1], net_info.ip[2], net_info.ip[3]);
    ESP_LOGI(TAG, " Subnet Mask : %d.%d.%d.%d\n", net_info.sn[0], net_info.sn[1], net_info.sn[2], net_info.sn[3]);
    ESP_LOGI(TAG, " Gateway     : %d.%d.%d.%d\n", net_info.gw[0], net_info.gw[1], net_info.gw[2], net_info.gw[3]);
    ESP_LOGI(TAG, " DNS         : %d.%d.%d.%d\n", net_info.dns[0], net_info.dns[1], net_info.dns[2], net_info.dns[3]);
    ESP_LOGI(TAG, "====================================================================================================\n\n");
}

int32_t loopback_tcps1(uint8_t sn, uint8_t *buf, uint16_t port)
{
    int32_t ret;
    uint16_t size = 0, sentsize = 0;
    uint8_t status = 0;
#ifdef _LOOPBACK_DEBUG1_
    uint8_t destip[4];
    uint16_t destport;
#endif
    status = getSn_SR(sn);
    uint8_t phy_status = getPHYCFGR();
    //  ESP_LOGI(TAG, "status = %02X", status);
    if (FLAG_GetFlag(FLAG_SIO_EVENT_RETRY_CONNECTCAMERA))
    {
        FLAG_ClearFlag(FLAG_SIO_EVENT_RETRY_CONNECTCAMERA);
        if (status == SOCK_ESTABLISHED)
        {
            IsConnectCam = false;
            if ((ret = socket(sn, Sn_MR_TCP, port, 0x00)) != sn)
                return ret;
        }
        ESP_LOGI(TAG, "ReConnect Camera");
    }
    else
    {
        switch (status)
        {
        case SOCK_ESTABLISHED:
            if (getSn_IR(sn) & Sn_IR_CON)
            {
#ifdef _LOOPBACK_DEBUG1_
                getSn_DIPR(sn, destip);
                destport = getSn_DPORT(sn);
                ESP_LOGI(TAG, "%d:Connected - %d.%d.%d.%d : %d\r\n", sn, destip[0], destip[1], destip[2], destip[3], destport);
                IsConnectCam = true;
#endif
                setSn_IR(sn, Sn_IR_CON);
            }
            if ((size = getSn_RX_RSR(sn)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
            {
                if (size > DATA_BUF_SIZE)
                    size = DATA_BUF_SIZE;
                ret = recv(sn, buf, size);

                if (ret <= 0)
                    return ret; // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
                size = (uint16_t)ret;
                sentsize = 0;

                while (size != sentsize)
                {
                    char temp_str[ETHERNET_BUF_MAX_SIZE + 1];
                    memset(temp_str, 0, sizeof(temp_str));
                    for (int i = 0; i < size; i++)
                    {
                        temp_str[i] = buf[i];
                    }
                    QR_Code = temp_str;
                    FLAG_SetFlag(FLAG_SIO_EVENT_UPDATE_PCB);
                    ret = send(sn, buf + sentsize, size - sentsize);

                    if (ret < 0)
                    {
                        close(sn);
                        return ret;
                    }
                    sentsize += ret; // Don't care SOCKERR_BUSY, because it is zero.
                }
            }
            break;
        case SOCK_CLOSE_WAIT:
#ifdef _LOOPBACK_DEBUG1_
            ESP_LOGI(TAG, "%d:CloseWait\r\n", sn);
#endif
            if ((ret = disconnect(sn)) != SOCK_OK)
                return ret;
#ifdef _LOOPBACK_DEBUG1_
            ESP_LOGI(TAG, "%d:Socket Closed\r\n", sn);
#endif
            break;
        case SOCK_INIT:
#ifdef _LOOPBACK_DEBUG1_
            ESP_LOGI(TAG, "%d:Listen, TCP server loopback, port [%d]\r\n", sn, port);
            IsConnectCam = false;
#endif
            if ((ret = listen(sn)) != SOCK_OK)
            {
                return ret;
            }
            break;
        case SOCK_CLOSED:
#ifdef _LOOPBACK_DEBUG1_
            ESP_LOGI(TAG, "%d:TCP server loopback start\r\n", sn);
#endif
            if ((ret = socket(sn, Sn_MR_TCP, port, 0x00)) != sn)
                return ret;
#ifdef _LOOPBACK_DEBUG1_
            ESP_LOGI(TAG, "%d:Socket opened\r\n", sn);
#endif
            break;
        default:
            return status;
            break;
        }
    }

    return 1;
}
// void send_heartbeat(uint8_t sn)
// {
//     const char *heartbeat_msg = "HEARTBEAT"; // Chuỗi heartbeat sẽ được gửi
//     int32_t ret;

//     // Gửi heartbeat qua socket
//     ret = send(sn, (uint8_t *)heartbeat_msg, strlen(heartbeat_msg));

//     // Kiểm tra xem có lỗi trong quá trình gửi không
//     if (ret < 0)
//     {
//         ESP_LOGI(TAG, "Failed to send heartbeat, closing socket");
//         close(sn); // Nếu gửi không thành công, đóng socket
//     }
//     else
//     {
//         ESP_LOGI(TAG, "Heartbeat sent successfully");
//     }
// }

void tcp_task(void *pvParameter)
{
    while (1)
    {
        loopback_ret = loopback_tcps1(0, g_loopback_buf, 5000);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
void TCP_Run(void)
{
    ESP_LOGI(TAG, "Starting TCP");
    xTaskCreate(&tcp_task, "TCPTask", 4096, NULL, 1, &TCPTaskHandle);
}