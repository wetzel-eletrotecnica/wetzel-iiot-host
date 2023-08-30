#include "report_direct_msg_handlers.h"

#include <esp_err.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "configuration.h"
#include "debug.h"
#include "real_time_clock.h"
#include "report_handler.h"

static const char* TAG = __FILE__;

namespace Wetzel {


esp_err_t msg_handler_rtc_update(char* msg) {
    esp_err_t err;
    char* next_char;
    char* updated_unix_time_char = strtok_r(msg, ",", &next_char);

    uint32_t updated_unix_time = strtoul(updated_unix_time_char, &next_char, 10);

    MY_LOGD("unixtime_char: %s", updated_unix_time_char);
    MY_LOGD("unixtime: %lu", strtoul(updated_unix_time_char, &next_char, 10));
    RealTimeClock* rtc = RealTimeClock::getInstance();
    err = rtc->configureRtc(updated_unix_time);

    return err;
}

// TODO completar metodo de input em mada de report_device_info
esp_err_t msg_handler_report_config(char* msg) {
    esp_err_t err;
    char* next_char;
    device_mac_t mac;
    ReportHandler* report = ReportHandler::getInstance();

    char* report_device_mac_Str = strtok_r(msg, ",", &next_char);
    uint8_t qtd_luminarias = atoi(strtok_r(NULL, ",", &next_char));
    uint8_t modelo_luminarias = atoi(strtok_r(NULL, ",", &next_char));

    MY_LOGD("Mac: %s    |   qtd_Lum: %d     |   modelo_lum: %d", report_device_mac_Str,
            qtd_luminarias, modelo_luminarias);

    sscanf(report_device_mac_Str, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx", &mac[0], &mac[1], &mac[2],
           &mac[3], &mac[4], &mac[5]);

    err = report->add_device_to_report_info_map(mac, qtd_luminarias, modelo_luminarias);

    return err;
}
}  // namespace Wetzel