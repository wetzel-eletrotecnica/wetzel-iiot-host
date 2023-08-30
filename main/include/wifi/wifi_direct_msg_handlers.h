#ifndef DIRECT_MSG_HANDLER_LIGHTING_CONTROL_H_
#define DIRECT_MSG_HANDLER_LIGHTING_CONTROL_H_

#include <esp_http_server.h>
#include <string>

namespace Wetzel {

const uint8_t WIFI_MODE_CHANGE_CODE = 1;
const uint8_t WIFI_AP_CONFIG_CHANGE_CODE = 2;
const uint8_t WIFI_STA_CONFIG_CHANGE_CODE = 3;
const uint8_t WIFI_GET_SSID_LIST_CODE = 4;
const uint8_t WIFI_AP_VALIDATE_CREDENTIALS_CODE = 5;
const uint8_t INTERFACE_INFO_REQUEST_CODE = 6;

esp_err_t msg_handler_wifi_mode_change(char* msg);
esp_err_t msg_handler_wifi_ap_config_change(char* msg);
esp_err_t msg_handler_wifi_sta_config_change(char* msg);
esp_err_t msg_handler_get_ssid_list(char* msg);
esp_err_t msg_handler_interface_info_requested(char* response);
esp_err_t msg_handler_validate_ap_credentials(char* msg);

}  // namespace Wetzel

#endif