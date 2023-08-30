#ifndef REPORT_DIRECT_MSG_HANDLER_LIGHTING_CONTROL_H_
#define REPORT_DIRECT_MSG_HANDLER_LIGHTING_CONTROL_H_

#include <esp_http_server.h>
#include <string>

namespace Wetzel {

const uint8_t RTC_UPDATE_CODE = 7;
const uint8_t REPORT_CONFIG_CODE = 8;

esp_err_t msg_handler_rtc_update(char* msg);
esp_err_t msg_handler_report_config(char* msg);
}  // namespace Wetzel

#endif