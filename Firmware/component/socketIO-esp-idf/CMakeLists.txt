idf_component_register(
    SRC_DIRS src "src" "src" 
    INCLUDE_DIRS include "include" "include"
    REQUIRES nvs_flash esp_websocket_client esp_http_client json esp_event esp_http_client esp_netif esp_wifi driver
)