#include "esp_log.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "esp_eth.h"
#include <stdlib.h>
#include "flag.h"
#include "Ethernet.h"

#define TAG "TCP_SERVER"
#define TCP_PORT 5000
#define LISTEN_QUEUE_SIZE 5
#define BUFFER_SIZE 1024

static char *QR_Code = NULL;
static bool IsConnectCam = false; // Trạng thái kết nối camera
TaskHandle_t TCPTaskHandle;

esp_eth_handle_t *eth_handles = NULL;
uint8_t eth_count = 0;

// Hàm kiểm tra trạng thái kết nối camera
bool ConnectCam(void)
{
    return IsConnectCam;
}

// Hàm lấy dữ liệu QR_Code
char *GetQRcode(void)
{
    return QR_Code != NULL ? QR_Code : NULL;
}

// Hàm đặt giá trị QR_Code
void SetQRCode(char *value)
{
    if (QR_Code)
    {
        free(QR_Code); // Giải phóng bộ nhớ trước đó nếu cần
    }
    QR_Code = strdup(value); // Sao chép giá trị mới
}

// Hàm reset QR_Code
void reset_QRcode(void)
{
    if (QR_Code)
    {
        free(QR_Code);
        QR_Code = NULL;
    }
}

void tcp_server_task(void *pvParameter)
{
    char addr_str[128];
    int listen_sock, sock;
    struct sockaddr_in dest_addr, source_addr;
    socklen_t addr_len = sizeof(source_addr);
    char rx_buffer[BUFFER_SIZE];

    // Tạo socket
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
    }

    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TCP_PORT);

    // Bind socket
    if (bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }

    // Lắng nghe trên socket
    if (listen(listen_sock, LISTEN_QUEUE_SIZE) != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Socket listening on port %d", TCP_PORT);

    while (1)
    {
        ESP_LOGI(TAG, "Waiting for connection...");
        sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI(TAG, "Socket accepted, ip address: %s", addr_str);
        IsConnectCam = true; // Camera kết nối thành công

        while (1)
        {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if (len < 0)
            {
                ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
                IsConnectCam = false; // Mất kết nối
                break;
            }
            else if (len == 0)
            {
                ESP_LOGI(TAG, "Connection closed");
                IsConnectCam = false; // Kết nối đã đóng
                break;
            }
            else
            {
                rx_buffer[len] = '\0';
                ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

                // Lưu QR_Code
                SetQRCode(rx_buffer);
                FLAG_SetFlag(FLAG_SIO_EVENT_UPDATE_PCB);
                // Echo lại dữ liệu
                int to_write = len;
                while (to_write > 0)
                {
                    int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                    if (written < 0)
                    {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                    to_write -= written;
                }
            }
        }

        close(sock);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}

void TCP_Init(void)
{
    Eth_Start();
    ESP_LOGI(TAG, "Starting TCP Server");
    xTaskCreate(tcp_server_task, "tcp_server_task", 8192, NULL, 5, &TCPTaskHandle);
}
