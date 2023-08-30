#include "wifi_direct_msg_handlers.h"

#include <esp_err.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "configuration.h"
#include "debug.h"
#include "wifi_wetzel_esp32.h"
#include "wifi_wetzel_utils.h"

static const char* TAG = __FILE__;

namespace Wetzel {
esp_err_t msg_handler_wifi_mode_change(char* msg) {

    char* next_char;
    char* token = strtok_r(msg, ",", &next_char);
    wifi_mode_t new_mode = *token == '1'   ? WIFI_MODE_APSTA
                           : *token == '0' ? WIFI_MODE_AP
                                           : WIFI_MODE_NULL;
    if (new_mode == WIFI_MODE_NULL) {
        MY_LOGE("New WiFi mode invalid");
        return ESP_FAIL;
    }
    MY_LOGD("WIFI_MODE %c", *token);

    //Esperado final da mensagem
    if (*(strtok_r(NULL, ",", &next_char)) != ';') {
        MY_LOGE("Mensagem com argumentos a mais");
        return ESP_FAIL;
    }

    WiFi* wifi = WiFi::getInstance();
    esp_err_t err = wifi->restart(new_mode);

    return err;
}

esp_err_t msg_handler_wifi_ap_config_change(char* msg) {
    char* next_char;
    char* new_ssid = strtok_r(msg, ",", &next_char);
    char* new_password = strtok_r(NULL, ",", &next_char);
    if (*(strtok_r(NULL, ",", &next_char)) != ';') {
        MY_LOGE("Mensagem com argumentos a mais");
        return ESP_FAIL;
    }
    MY_LOGD("ESTOU CONFIGURANDO AP");
    WiFi* wifi = WiFi::getInstance();
    wifi_config_t new_config = wifi->ap_configuration();

    strcpy((char*)new_config.ap.ssid, new_ssid);
    strcpy((char*)new_config.ap.password, new_password);
    new_config.ap.ssid_len = strlen(new_ssid);

    wifi->ap_configuration(new_config);
    MY_LOGD("ap comfigurated");
    wifi->restart(wifi->current_mode());
    MY_LOGD("Wifi restarted");

    return ESP_OK;
}

/**
 * msg_component[0] -> ssid
 * msg_component[1] -> password
 * msg_component[2] -> ip
 * msg_component[3] -> submask
 * msg_component[4] -> gateway
 * msg_component[5] -> dns primario
 * msg_component[6] -> dns secundario
 */
esp_err_t msg_handler_wifi_sta_config_change(char* msg) {
    char* next_char;
    esp_err_t err;
    char* msg_components[7];
    msg_components[0] = strtok_r(msg, ",", &next_char);

    if (msg_components[0] == NULL) {
        MY_LOGE("Mensagem sem argumentos");
    }
    for (int i = 1; i < 7; i++) {

        msg_components[i] = strtok_r(NULL, ",", &next_char);
        if (msg_components[i][0] == '#') {
            msg_components[i][0] = '\0';
        } else if (msg_components[i][0] == ';') {
            MY_LOGE("Argumentos insuficientes");
            return ESP_FAIL;
        }
        MY_LOGD("ITERACAO %d    MENSAGEM: %s", i, msg_components[i]);
    }
    if (*(strtok_r(NULL, ",", &next_char)) != ';') {
        MY_LOGE("Argumentos a mais");
        return ESP_FAIL;
    }

    WiFi* wifi = WiFi::getInstance();
    wifi_config_t new_config = wifi->sta_configuration();

    strcpy((char*)new_config.sta.ssid, msg_components[0]);
    strcpy((char*)new_config.sta.password, msg_components[1]);

    wifi->sta_configuration(new_config);

    err = wifi->sta_ip_configuration(msg_components[2], msg_components[3],
                                     msg_components[4]);
    if (err != ESP_OK) {
        MY_LOGW("Falha na configuração do sta | abortada configuração");
    }

    wifi->restart(wifi->current_mode());

    return ESP_OK;
}

// TODO Criar método para retornar redes disponiveis e intensidade de seus sinais
// para realizar conexão STA com servidor
esp_err_t msg_handler_get_ssid_list(char* msg) {
    return ESP_OK;
}

esp_err_t msg_handler_interface_info_requested(char* response) {

    WiFi* wifi = WiFi::getInstance();
    wifi_ap_config_t ap_config = wifi->ap_configuration().ap;
    wifi_sta_config_t sta_config = wifi->sta_configuration().sta;
    ipv4_information_t ip_info = wifi->ipv4_info();

    char ip_str[IPV4_MAX_SIZE];
    char mask_str[IPV4_MAX_SIZE];
    char gateway_str[IPV4_MAX_SIZE];
    char primary_dns_str[IPV4_MAX_SIZE];
    char secondary_dns_str[IPV4_MAX_SIZE];

    ESP_ERROR_CHECK(ipv4_uint8_array_to_str(ip_info.ip, ip_str));
    ESP_ERROR_CHECK(ipv4_uint8_array_to_str(ip_info.mask, mask_str));
    ESP_ERROR_CHECK(ipv4_uint8_array_to_str(ip_info.gateway, gateway_str));
    ESP_ERROR_CHECK(
        ipv4_uint8_array_to_str(ip_info.primary_dns, primary_dns_str));
    ESP_ERROR_CHECK(
        ipv4_uint8_array_to_str(ip_info.secondary_dns, secondary_dns_str));

    // Max char lenght = 204;
    snprintf(response, 1500, "%d,%s,%s,%s,%s,%s,%s,%s,", wifi->current_mode(),
             ap_config.ssid, sta_config.ssid, ip_str, mask_str, gateway_str,
             primary_dns_str, secondary_dns_str);

    return ESP_OK;
}

esp_err_t msg_handler_validate_ap_credentials(char* msg) {
    char* next_char;
    char* ssid = strtok_r(msg, ",", &next_char);
    char* password = strtok_r(NULL, ",", &next_char);
    if (*(strtok_r(NULL, ",", &next_char)) != ';') {
        MY_LOGE("Argumentos a mais");
        return ESP_FAIL;
    }
    WiFi* wifi = WiFi::getInstance();
    if (strcmp(ssid, (char*)wifi->ap_configuration().ap.ssid) != 0) {
        MY_LOGI("SSID incorreto");
        MY_LOGD("Exp: %s  |  Rec: %s", (char*)wifi->ap_configuration().ap.ssid,
                ssid);

        return ESP_FAIL;
    }
    if (strcmp(password, (char*)wifi->ap_configuration().ap.password) != 0) {
        MY_LOGI("Senha incorreta");
        MY_LOGD("Exp: %s  |  Rec: %s",
                (char*)wifi->ap_configuration().ap.password, password);
        return ESP_FAIL;
    }
    return ESP_OK;
}
}  // namespace Wetzel