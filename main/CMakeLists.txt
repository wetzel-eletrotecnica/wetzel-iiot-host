idf_component_register(
    SRCS 
        "src/esp-interface.cpp"
        "src/async_server/async_server.cpp"
        "src/comum/nvs_wetzel_handler.cpp"
        "src/server_html_utils.cpp"
        "src/perifericos/sd_card_handler.cpp"  
        "src/real_time_clock/real_time_clock.cpp"
        "src/relatorio/report_direct_msg_handlers.cpp"
        "src/relatorio/report_handler.cpp"
        "src/wifi/wifi_direct_msg_handlers.cpp"
        "src/wifi/wifi_event_listener.cpp"
        "src/wifi/wifi_wetzel_esp32.cpp"
        "src/wifi/wifi_wetzel_utils.cpp"
        "src/perifericos/Time.cpp"
        "lib/Memory/Memory.cpp"
        # Implementacao modulo relatorio
        "lib/API_ModuloRelatorio/local_storage/local_storage.cpp"
        "lib/API_ModuloRelatorio/API_ModuloRelatorio.cpp"
    INCLUDE_DIRS 
        "."
        "include"
        "include/comum"
        "include/perifericos"
        "include/relatorio"
        "include/wifi"
        "include/async_server"
        "include/real_time_clock"
        "lib/Memory"
        # Implementacao modulo relatorio
        "lib/API_ModuloRelatorio/local_storage"
        "lib/API_ModuloRelatorio"
    REQUIRES
        arduino esp_http_server sdmmc fatfs
)

# defini a versão do c++ usado no projeto
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)