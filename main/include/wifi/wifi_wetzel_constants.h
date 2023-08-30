#ifndef WIFI_WETZEL_CONSTANTS_H_
#define WIFI_WETZEL_CONSTANTS_H_

namespace Wetzel {

//General
#define IPV4_MAX_SIZE                               16
#define MAX_NUMBER_OF_STA_CONNECTION_ATTEMPTS       5
#define PERIOD_OF_STA_CONNECTION_ATTEMPTS_MS        10000
#define WIFI_DEFAULT_MODE                           wifi_mode_t::WIFI_MODE_AP
#define WIFI_NETWORK_MAX_SSID_LEN                   32
#define WIFI_NETWORK_MAX_PASSWORD_LEN               64

// SoftAP
#define DEFAULT_SOFT_AP_SSID                        "Wetzel Lighting Control"
#define DEFAULT_SOFT_AP_PASSWORD                    "123456789"
#define DEFAULT_SOFT_AP_SSID_LENGHT                 sizeof(DEFAULT_SOFT_AP_SSID)
#define DEFAULT_SOFT_AP_CHANNEL                     3
#define DEFAULT_SOFT_AP_AUTH_MODE                   wifi_auth_mode_t::WIFI_AUTH_WPA_WPA2_PSK
#define DEFAULT_SOFT_AP_SSID_HIDDEN                 0
#define DEFAULT_SOFT_AP_MAX_CONNECTIONS             1
#define DEFAULT_SOFT_AP_BEACON_INTERVAL             100

// STA
#define DEFAULT_STA_AUTH_MODE                       wifi_auth_mode_t::WIFI_AUTH_WPA2_PSK
#define DEFAULT_STA_SSID                            "External WiFi"
#define DEFAULT_STA_PASSWORD                        "dummy_password"

// NVS
#define NVS_KEY_WIFI_MODE                           "wifi_mode"
#define NVS_KEY_SOFT_AP_CONFIG                      "softap_config"
#define NVS_KEY_STA_CONFIG                          "sta_config"
#define NVS_KEY_STA_IP_CONFIG                       "ipv4_config"
}  // namespace Wetzel
#endif