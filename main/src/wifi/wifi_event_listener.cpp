#include "wifi_event_listener.h"

#include <esp_wifi.h>

#include "debug.h"

static const char* TAG = __FILE__;

namespace Wetzel {

void _print_wifi_event(int32_t event_id) {
    const char* event =
        event_id == WIFI_EVENT_WIFI_READY ? "WIFI_EVENT_WIFI_READY"
                                          : /**< ESP32 WiFi ready */
            event_id == WIFI_EVENT_SCAN_DONE ? "WIFI_EVENT_SCAN_DONE"
                                             : /**< ESP32 finish scanning AP */
            event_id == WIFI_EVENT_STA_START ? "WIFI_EVENT_STA_START"
                                             : /**< ESP32 station start */
            event_id == WIFI_EVENT_STA_STOP ? "WIFI_EVENT_STA_STOP"
                                            : /**< ESP32 station stop */
            event_id == WIFI_EVENT_STA_CONNECTED
            ? "WIFI_EVENT_STA_CONNECTED"
            : /**< ESP32 station connected to AP */
            event_id == WIFI_EVENT_STA_DISCONNECTED
            ? "WIFI_EVENT_STA_DISCONNECTED"
            : /**< ESP32 station disconnected from AP */
            event_id == WIFI_EVENT_STA_AUTHMODE_CHANGE
            ? "WIFI_EVENT_STA_AUTHMODE_CHANGE"
            : /**< the auth mode of AP connected by ESP32 station changed */
            event_id == WIFI_EVENT_STA_WPS_ER_SUCCESS
            ? "WIFI_EVENT_STA_WPS_ER_SUCCESS"
            : /**< ESP32 station wps succeeds in enrollee mode */
            event_id == WIFI_EVENT_STA_WPS_ER_FAILED
            ? "WIFI_EVENT_STA_WPS_ER_FAILED"
            : /**< ESP32 station wps fails in enrollee mode */
            event_id == WIFI_EVENT_STA_WPS_ER_TIMEOUT
            ? "WIFI_EVENT_STA_WPS_ER_TIMEOUT"
            : /**< ESP32 station wps timeout in enrollee mode */
            event_id == WIFI_EVENT_STA_WPS_ER_PIN
            ? "WIFI_EVENT_STA_WPS_ER_PIN"
            : /**< ESP32 station wps pin code in enrollee mode */
            event_id == WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP
            ? "WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP"
            : /**< ESP32 station wps overlap in enrollee mode */
            event_id == WIFI_EVENT_AP_START ? "WIFI_EVENT_AP_START"
                                            : /**< ESP32 soft-AP start */
            event_id == WIFI_EVENT_AP_STOP ? "WIFI_EVENT_AP_STOP"
                                           : /**< ESP32 soft-AP stop */
            event_id == WIFI_EVENT_AP_STACONNECTED
            ? "WIFI_EVENT_AP_STACONNECTED"
            : /**< a station connected to ESP32 soft-AP */
            event_id == WIFI_EVENT_AP_STADISCONNECTED
            ? "WIFI_EVENT_AP_STADISCONNECTED"
            : /**< a station disconnected from ESP32 soft-AP */
            event_id == WIFI_EVENT_AP_PROBEREQRECVED
            ? "WIFI_EVENT_AP_PROBEREQRECVED"
            : /**< Receive probe request packet in soft-AP interface */
            event_id == WIFI_EVENT_FTM_REPORT
            ? "WIFI_EVENT_FTM_REPORT"
            : /**< Receive report of FTM procedure */
            /* Add next events after this only */
            event_id == WIFI_EVENT_STA_BSS_RSSI_LOW
            ? "WIFI_EVENT_STA_BSS_RSSI_LOW"
            : /**< AP's RSSI crossed configured threshold */
            event_id == WIFI_EVENT_ACTION_TX_STATUS
            ? "WIFI_EVENT_ACTION_TX_STATUS"
            : /**< Status indication of Action Tx operation */
            event_id == WIFI_EVENT_ROC_DONE
            ? "WIFI_EVENT_ROC_DONE"
            : /**< Remain-on-Channel operation complete */
            event_id == WIFI_EVENT_STA_BEACON_TIMEOUT
            ? "WIFI_EVENT_STA_BEACON_TIMEOUT"
            : /**< ESP32 station beacon timeout */
            event_id == WIFI_EVENT_MAX ? "WIFI_EVENT_MAX"
                                       : "WIFI_EVENT DESCONHECIDO";
    MY_LOGD("wifi event: %s", event);
}

void _print_ip_event(int32_t event_id) {
    const char* event =
        event_id == IP_EVENT_STA_GOT_IP
            ? "IP_EVENT_STA_GOT_IP"
            : /*!< station got IP from connected AP */
            event_id == IP_EVENT_STA_LOST_IP
            ? "IP_EVENT_STA_LOST_IP"
            : /*!< station lost IP and the IP is reset to 0 */
            event_id == IP_EVENT_AP_STAIPASSIGNED
            ? "IP_EVENT_AP_STAIPASSIGNED"
            : /*!< soft-AP assign an IP to a connected station */
            event_id == IP_EVENT_GOT_IP6
            ? "IP_EVENT_GOT_IP6"
            : /*!< station or ap or ethernet interface v6IP addr is preferred */
            event_id == IP_EVENT_ETH_GOT_IP
            ? "IP_EVENT_ETH_GOT_IP"
            : /*!< ethernet got IP from connected AP */
            event_id == IP_EVENT_PPP_GOT_IP ? "IP_EVENT_PPP_GOT_IP"
                                            : /*!< PPP interface got IP */
            event_id == IP_EVENT_PPP_LOST_IP ? "IP_EVENT_PPP_LOST_IP"
                                             : /*!< PPP interface lost IP */
            "IP_EVENT DESCONHECIDO";
    MY_LOGW("ip event: %s", event);
}
}  // namespace Wetzel