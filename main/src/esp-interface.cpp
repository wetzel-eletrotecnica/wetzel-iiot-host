#ifndef ESP32
#define ESP32
#endif

#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_http_server.h>
#include <esp_intr_alloc.h>
#include <esp_wifi.h>
#include <macros.h>
#include <HardwareSerial.h>

#include "async_server.h"
#include "configuration.h"
#include "debug.h"
#include "real_time_clock.h"
#include "report_handler.h"
#include "sd_card_handler.h"
#include "wifi_wetzel_esp32.h"

static const char* TAG = __FILE__;

void uart2_input_task(void* param);
static void print_system_info_timercb(void* timer);
static void rssi_check(void* timer);

extern "C" {
void app_main();
}  // 134217756U

void app_main() {
    uart_config_t uart_config = {
        .baud_rate = 115200, //230400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL,
                                        intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));

    // estabiliando uart apos troca de baudrate
    vTaskDelay(100 / portTICK_PERIOD_MS);
    MY_LOGI("FIRMWARE: WETZEL-IIOT-INTERFACE VERSION %s", FIRMWARE_VERSION);

    Serial2.setRxBufferSize(UART_RX_BUFFER);
    Serial2.begin(UART_BAUD_RATE);
    pinMode(LED_PIN, OUTPUT);

    Wetzel::WiFi* wifi = Wetzel::WiFi::getInstance();
    wifi->begin();
    wifi->start(wifi->current_mode());

    // IF REPORT
    Wetzel::CartaoSD* card = Wetzel::CartaoSD::getInstance();
    card->begin(SPI_MOSI_GPIO, SPI_MISO_GPIO, SPI_SCLK_GPIO, SPI_CS_GPIO);

    /* Implementação do RTC do esp sem módulo */
    Wetzel::RealTimeClock* rtc = Wetzel::RealTimeClock::getInstance();
    rtc->begin();
    Wetzel::ReportHandler* report_handler =
        Wetzel::ReportHandler::getInstance();
    report_handler->begin();
    // ENDIF

    Wetzel::AsyncServer::begin();
    vTaskDelay(1500 / portTICK_PERIOD_MS);

    TimerHandle_t timer =
        xTimerCreate("print_system_info", 5000 / portTICK_RATE_MS, true, NULL,
                     print_system_info_timercb);
    xTimerStart(timer, 0);

    TimerHandle_t timer2 = xTimerCreate(
        "RSSI_check", MAIN_RSSI_CHECK_TIMER_INTERVAL / portTICK_RATE_MS, true,
        NULL, rssi_check);
    xTimerStart(timer2, 500);

    // TEST
    const int max_n_entry_day = 172800;
    const int days_a_year = 365;

    Wetzel::report_entry_t entry = {
        .device_info =
            {
                .id = 12,
                .qtd_luminarias = 16,
                .modelo_luminarias = Wetzel::luminaria_type_catalog_t::LUM_17K,
            },
        .pwm_value = 211,
        .unix_seconds = 17280880,
    };

    // for (int i = 0; i < days_a_year; i++) {
    //     char path[20];
    //     sprintf(path, "/%02d%02d%02d.txt",23,i/30+1,i%30);
    //     FILE* file = card->openFile(path, Wetzel::WRITING_FILE);
    //     MY_LOGI("Recording File Day: %s",path);
    //     vTaskDelay(pdTICKS_TO_MS(1000));
    //     for (int j = 0; j < max_n_entry_day; j++) {
    //         fwrite(&id, sizeof(id), 1, file);
    //         fwrite(&n_lum, sizeof(n_lum), 1, file);
    //         fwrite(&dim_level, sizeof(dim_level), 1, file);
    //         fwrite(&dateTime, sizeof(dateTime), 1, file);
    //     }
    //     card->closeFile(Wetzel::WRITING_FILE);
    // }

    // while (1) {
    //     ;
    // }

    // END OF TEST

    vTaskDelete(NULL);
}

static void print_system_info_timercb(void* timer) {

    MY_LOGT("INICIALIZANDO PRINT CALLBACK");
    MY_LOGT("System information, free heap: %u", esp_get_free_heap_size());

    if (!heap_caps_check_integrity_all(true)) {
        MY_LOGE("At least one heap is corrupt");
    }
}

char* buffer;

static void rssi_check(void* timer) {
    if (xSemaphoreTake(Wetzel::WiFi::_wifi_mutex, 10000 / portTICK_RATE_MS) !=
        pdTRUE) {
        // MY_LOGI("Nao foi possivel pegar o mutex!");
        return;
    }
    // MY_LOGT("Verificando o RSSI");
    wifi_sta_list_t sta;
    esp_err_t err = esp_wifi_ap_get_sta_list(&sta);
    if (err != ESP_OK) {
        MY_LOGE("esp_wifi_ap_get_sta_list(&sta) error=%d", err);
        return;
    }
    // MY_LOGT("Numero de conexoes: %d", sta.num);
    // MY_LOGT("RSSI[0] do celular: %d", sta.sta[0].rssi);
    // MY_LOGT("Numero de mensagens recebidas: %d",
    // Wetzel::AsyncServer::http_requests_received);
    digitalWrite(LED_PIN, sta.num > 0);

    if (sta.num > 0 && sta.sta[0].rssi < MAIN_RSSI_CHECK_TIMER_MIN_RSSI) {
        // MY_LOGW("O sinal esta muito fraco");
        esp_wifi_stop();
        vTaskDelay(MAIN_RSSI_CHECK_TIMER_RESTART_DELAY_MS / portTICK_RATE_MS);
        esp_wifi_start();
    }
    xSemaphoreGive(Wetzel::WiFi::_wifi_mutex);

    return;
}
