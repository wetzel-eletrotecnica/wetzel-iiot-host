#ifndef _WIFI_WETZELESP_32_H_
#define _WIFI_WETZEL_ESP32_H_

#include <esp_wifi.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <string>

#include "configuration.h"
#include "wifi_wetzel_constants.h"

namespace Wetzel {

typedef struct {
    uint8_t ip[4] = {0, 0, 0, 0};
    uint8_t mask[4] = {0, 0, 0, 0};
    uint8_t gateway[4] = {0, 0, 0, 0};
    uint8_t primary_dns[4] = {0, 0, 0, 0};
    uint8_t secondary_dns[4] = {0, 0, 0, 0};
} ipv4_information_t;

class WiFi {
    typedef std::string String;

   public:
    void operator=(WiFi const&) = delete;
    ~WiFi();

    static WiFi* getInstance();

    static SemaphoreHandle_t _wifi_mutex;
    static SemaphoreHandle_t _current_mode_mutex;

    void begin();
    esp_err_t start(wifi_mode_t mode, uint16_t timeout_ms = 4000);
    esp_err_t restart(wifi_mode_t mode, uint16_t timeout_ms = 4000);
    esp_err_t stop();

    const char* ap_mode_ssid() const;
    bool ap_mode_ssid(const char* new_ssid);
    const char* sta_mode_ssid() const;
    bool sta_mode_ssid(const char* new_ssid);

    void ap_configuration(wifi_config_t& new_ap_config);
    void sta_configuration(wifi_config_t& new_sta_config);
    wifi_config_t ap_configuration();
    wifi_config_t sta_configuration();
    wifi_config_t default_ap_config();
    wifi_config_t default_sta_config();

    esp_err_t sta_ip_configuration(char* ip, char* mask, char* gateway);
    esp_err_t sta_ip_configuration(uint8_t* ip, uint8_t* mask,
                                   uint8_t* gateway);
    esp_err_t sta_ip_configuration(ipv4_information_t ipv4_info);
    esp_netif_ip_info_t sta_ip_configuration();

    ipv4_information_t ipv4_info();

    wifi_mode_t current_mode();

    uint8_t mac[6];
    char _network_ssid[WIFI_NETWORK_MAX_SSID_LEN];

   private:
    WiFi();

    char _network_password[WIFI_NETWORK_MAX_PASSWORD_LEN];

    wifi_config_t _ap_config;
    wifi_config_t _sta_config;

    esp_netif_t* ap_netif;
    esp_netif_t* sta_netif;
    esp_netif_ip_info_t _netif_ip_info;
    ipv4_information_t _ipv4_info;

    bool wifi_ap_is_configured = false;
    bool wifi_sta_is_configured = false;
    bool is_running = false;
    bool is_restarting = false;
    bool is_initialized = false;

    esp_err_t sta_ip_configuration(esp_netif_ip_info_t& ip_info);

    static wifi_mode_t _current_mode;
    static uint8_t number_of_connection_attempts;
    static WiFi* _instance;
    static TimerHandle_t xReconnectSTATask;
    static void any_event_handler(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data);
    static void sta_disconnected_handler();
    static void sta_connected_handler();
    static void sta_got_ip_handler();
    static void reconnect_task_handler(void* timer);
};

}  // namespace Wetzel

#endif
