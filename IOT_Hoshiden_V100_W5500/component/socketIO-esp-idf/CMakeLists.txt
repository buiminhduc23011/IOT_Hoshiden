idf_component_register(
    SRCS "src/eeprom.cpp"
    SRCS "src/ethernetclient.cpp"
    SRCS "src/socketioclient.cpp"
    SRCS "src/websocketclient.cpp"
    SRCS "src/wificlient.c"  # Chỉ định tệp nguồn
    INCLUDE_DIRS "include"          # Chỉ định thư mục chứa tệp header
    REQUIRES nvs_flash esp_websocket_client esp_http_client json esp_event esp_http_client
)