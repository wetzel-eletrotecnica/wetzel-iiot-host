#include "wifi_wetzel_esp32.h"

#include <IPAddress.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <inttypes.h>
#include <lwip/err.h>
#include <lwip/ip_addr.h>
#include <lwip/ip4_addr.h>
#include <lwip/sys.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <nvs_handle.hpp>

#include <string>

#include "configuration.h"
#include "debug.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_wetzel_handler.h"
#include "wifi_wetzel_utils.h"

static const char* TAG = __FILE__;

namespace Wetzel {

static EventGroupHandle_t s_wifi_event_group;

/**
 * @brief instanciações de variáveis static
 * 
 */
WiFi* WiFi::_instance = nullptr;
uint8_t WiFi::number_of_connection_attempts = 0;
wifi_mode_t WiFi::_current_mode;
SemaphoreHandle_t WiFi::_wifi_mutex;

/**
 * @brief Criação da task de tentativa de reconexão no modo Station
 * 
 */
TimerHandle_t WiFi::xReconnectSTATask =
    xTimerCreate("Tentativa de reconexão do STA",
                 PERIOD_OF_STA_CONNECTION_ATTEMPTS_MS / portTICK_RATE_MS, true,
                 NULL, reconnect_task_handler);

/**
 * @brief Handler chamado para todo evento do ESP. Possui casos para eventos especificos de WIFI e IP.
 * 
 * @param arg 
 * @param event_base 
 * @param event_id 
 * @param event_data 
 */
void WiFi::any_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
    if (xSemaphoreTake(_wifi_mutex, 50 / portTICK_RATE_MS) != pdTRUE) {
        return;
    }
    if (_current_mode == WIFI_MODE_APSTA || _current_mode == WIFI_MODE_STA) {
        if (event_base == WIFI_EVENT &&
            event_id == WIFI_EVENT_STA_DISCONNECTED) {
            sta_disconnected_handler();
        } else if (event_base == WIFI_EVENT &&
                   event_id == WIFI_EVENT_STA_CONNECTED) {
            sta_connected_handler();
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            sta_got_ip_handler();
        }
    }
    if (_current_mode == WIFI_MODE_APSTA || _current_mode == WIFI_MODE_AP) {}
    xSemaphoreGive(_wifi_mutex);
}

void WiFi::reconnect_task_handler(void* timer) {
    esp_err_t err = esp_wifi_connect();
    // MY_LOGD("Tentando reconexão com STA. Resultado: %x", err);
}

/**
 * @brief Evento de desconexão do STA, chamado quando STA mode está ativo e STA é desconectado.
 * Tenta reconectar até o limite de conexões, e depois ativa a task de reconexão periódica.
 * 
 */
void WiFi::sta_disconnected_handler() {
    if (number_of_connection_attempts < MAX_NUMBER_OF_STA_CONNECTION_ATTEMPTS) {
        number_of_connection_attempts++;
        esp_wifi_connect();
    } else if (xTimerIsTimerActive(xReconnectSTATask) == pdFALSE) {
        MY_LOGI("Inicializando task periódica de conexao [t=%ds]",
                (uint8_t)(PERIOD_OF_STA_CONNECTION_ATTEMPTS_MS / 1000))
        xTimerStart(xReconnectSTATask, 0);
    }
}

/**
 * @brief Evento de conexão do STA, chamado quando STA mode está ativo e STA é conectado.
 * Para a task de reconexão, e zera a contagem de tentativas de conexão.
 * 
 */
void WiFi::sta_connected_handler() {
    ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    if (xTimerIsTimerActive(xReconnectSTATask) != pdFALSE) {
        MY_LOGI("Parando task periódica de conexao [t=%ds]",
                (uint8_t)(PERIOD_OF_STA_CONNECTION_ATTEMPTS_MS / 1000))
        xTimerStop(xReconnectSTATask, 0);
    }
    number_of_connection_attempts = 0;
}

/**
 * @brief Evento de aquisição de IP do STA, chamado quando STA mode está ativo e STA recebe um IP de um Access Point. //TODO usar isso para conexao com servidor
 * 
 */
void WiFi::sta_got_ip_handler() {
    ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
    // xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
}

/**
 * @brief Returna instância de Singleton WiFi
 * 
 * @return WiFi* Instância única da classe WiFi.
 */
WiFi* WiFi::getInstance() {
    if (_instance == nullptr) {
        _instance = new WiFi();
    }
    return _instance;
}

WiFi::WiFi() {
    sprintf(_network_ssid, DEFAULT_SOFT_AP_SSID);
    sprintf(_network_password, DEFAULT_SOFT_AP_PASSWORD);
}

WiFi::~WiFi() {
    delete _instance;
}

/**
 * @brief Inicia e configura o WiFi com suas configurações padrão.
 * Necessário sua chamada antes deutilizar das funções da classe
 * 
 */
void WiFi::begin() {
    if (is_initialized) {
        MY_LOGE("WiFi já inicializado");
        return;
    }

    // MY_LOGD("Iniciando o wifi");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    err = esp_netif_dhcpc_stop(sta_netif);
    MY_LOGD("DHCP INIT: %s",esp_err_to_name(err));

    _wifi_mutex = xSemaphoreCreateMutex();

    is_initialized = true;

    _ap_config = default_ap_config();
    _sta_config = default_sta_config();
    _current_mode = WIFI_DEFAULT_MODE;

    std::unique_ptr<nvs::NVSHandle> handle =
        nvs::open_nvs_handle("nvs", NVS_READWRITE, &err);
    if (err != ESP_OK) {
        MY_LOGE("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        err = handle->get_blob(NVS_KEY_SOFT_AP_CONFIG, (void*)&_ap_config.ap,
                               sizeof(wifi_ap_config_t));
        print_nvs_error_in_log(err, NVS_KEY_SOFT_AP_CONFIG);

        err = handle->get_blob(NVS_KEY_STA_CONFIG, (void*)&_sta_config.sta,
                               sizeof(wifi_sta_config_t));
        print_nvs_error_in_log(err, NVS_KEY_STA_CONFIG);

        err = handle->get_blob(NVS_KEY_WIFI_MODE, (void*)&_current_mode,
                               sizeof(wifi_mode_t));
        print_nvs_error_in_log(err, NVS_KEY_WIFI_MODE);

        err = handle->get_blob(NVS_KEY_STA_IP_CONFIG, (void*)&_ipv4_info,
                               sizeof(ipv4_information_t));
        print_nvs_error_in_log(err, NVS_KEY_WIFI_MODE);

        //sta_ip_configuration(_ipv4_info);

        handle->commit();
    }
}

/**
 * @brief Ativa WiFi no modo especificado. Configura o mesmo automaticamente,
 * através das configurações passadas pelos métodos ap_configuration() e config_sta().
 * 
 * @param mode suporta três modos (AP, STA, APSTA)
 * @param timeout_ms timeout de tentativas de conexão, utilizado apenas nos modos STA e APSTA;
 * @return verdadeiro caso consiga inicializar, falso do contrário.
 */
esp_err_t WiFi::start(wifi_mode_t mode, uint16_t timeout_ms) {
    if (mode != WIFI_MODE_AP && mode != WIFI_MODE_APSTA &&
        mode != WIFI_MODE_STA) {
        MY_LOGW("Modo de WiFi não suportado");
        return ESP_FAIL;
    }

    if (is_running) {
        MY_LOGE("WiFi já está iniciado");
        MY_LOGI("Abortando inicialização de WIFI");
        return ESP_FAIL;
    }

    if (!is_restarting) {
        if (xSemaphoreTake(_wifi_mutex, 10000 / portTICK_RATE_MS) != pdTRUE) {
            return ESP_FAIL;
        }
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            IP_EVENT, ESP_EVENT_ANY_ID, &any_event_handler, NULL, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &any_event_handler, NULL, NULL));
    }

    if (_current_mode != mode) {
        _current_mode = mode;
        save_in_nvs(NVS_KEY_WIFI_MODE, (void*)&_current_mode,
                    sizeof(wifi_mode_t));
    }

    if (!is_restarting) {
        xSemaphoreGive(_wifi_mutex);
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &_ap_config));
    }
    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &_sta_config));
    }

    ESP_ERROR_CHECK(esp_wifi_start());

    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    }

    is_running = true;
    return ESP_OK;
}

esp_err_t WiFi::restart(wifi_mode_t mode, uint16_t timeout_ms) {
    if (xSemaphoreTake(_wifi_mutex, 10000 / portTICK_RATE_MS) != pdTRUE) {
        return ESP_FAIL;
    }

    if (!is_running) {
        MY_LOGE("WiFi ainda não iniciado");
        MY_LOGI("Abortando inicialização de WIFI");
        xSemaphoreGive(_wifi_mutex);
        return ESP_FAIL;
    }

    is_restarting = true;
    esp_wifi_stop();
    esp_wifi_restore();
    xTimerStop(xReconnectSTATask, 0);

    is_running = false;

    esp_err_t err = start(mode, timeout_ms);
    is_restarting = false;

    xSemaphoreGive(_wifi_mutex);
    return err;
}

// esp_err_t WiFi::stop() {
//     const char* err = esp_err_to_name(esp_wifi_stop());
//     remove_all_handlers();
//     if (err == ESP_OK) {
//         return true;
//     }
//     return false;
// }

void WiFi::ap_configuration(wifi_config_t& new_ap_config) {
    _ap_config = new_ap_config;
    save_in_nvs(NVS_KEY_SOFT_AP_CONFIG, (void*)&_ap_config.ap,
                sizeof(wifi_ap_config_t));
}

void WiFi::sta_configuration(wifi_config_t& new_sta_config) {
    _sta_config = new_sta_config;
    save_in_nvs(NVS_KEY_STA_CONFIG, (void*)&_sta_config.sta,
                sizeof(wifi_sta_config_t));
}

wifi_config_t WiFi::ap_configuration() {
    return _ap_config;
}

wifi_config_t WiFi::sta_configuration() {
    return _sta_config;
}

wifi_config_t WiFi::default_ap_config() {
    wifi_config_t _default_ap_config = {
        .ap = {
            .ssid_len = DEFAULT_SOFT_AP_SSID_LENGHT,
            .channel = DEFAULT_SOFT_AP_CHANNEL,
            .authmode = DEFAULT_SOFT_AP_AUTH_MODE,
            .ssid_hidden = DEFAULT_SOFT_AP_SSID_HIDDEN,
            .max_connection = DEFAULT_SOFT_AP_MAX_CONNECTIONS,
            .beacon_interval = DEFAULT_SOFT_AP_BEACON_INTERVAL,
        }};

    strcpy((char*)_default_ap_config.ap.ssid, DEFAULT_SOFT_AP_SSID);
    strcpy((char*)_default_ap_config.ap.password, DEFAULT_SOFT_AP_PASSWORD);

    return _default_ap_config;
}

wifi_config_t WiFi::default_sta_config() {
    wifi_config_t _default_sta_config = {
        .sta = {.threshold = {
                    .authmode = DEFAULT_STA_AUTH_MODE,
                }}};

    strcpy((char*)_default_sta_config.sta.ssid, DEFAULT_STA_SSID);
    strcpy((char*)_default_sta_config.sta.password, DEFAULT_STA_PASSWORD);

    return _default_sta_config;
}

ipv4_information_t WiFi::ipv4_info() {
    return _ipv4_info;
}

esp_err_t WiFi::sta_ip_configuration(esp_netif_ip_info_t& ip_info) {
    esp_err_t err = esp_netif_set_ip_info(sta_netif, &ip_info);
    if (err != ESP_OK) {
        MY_LOGE("Erro na configuração dos IPs");
        return err;
    }

    // TEST DNS
    esp_netif_dns_info_t dns;
    dns.ip.u_addr.ip4.addr = ip_info.gw.addr;
    dns.ip.type = IPADDR_TYPE_V4;
    err = esp_netif_set_dns_info(sta_netif, esp_netif_dns_type_t::ESP_NETIF_DNS_MAIN, &dns);
    MY_LOGD("DNS_MAIN: %s",esp_err_to_name(err));
    err = esp_netif_set_dns_info(sta_netif, esp_netif_dns_type_t::ESP_NETIF_DNS_BACKUP, &dns);
    MY_LOGD("DNS_BKUP: %s",esp_err_to_name(err));
    // END OF TEST
    
    _netif_ip_info = ip_info;

    return ESP_OK;
}

esp_err_t WiFi::sta_ip_configuration(char* ip, char* mask, char* gateway) {
    esp_netif_ip_info_t ip_info;
    esp_err_t err;
    char ip_copy[IPV4_MAX_SIZE], mask_copy[IPV4_MAX_SIZE],
        gateway_copy[IPV4_MAX_SIZE];

    strlcpy(ip_copy, ip, IPV4_MAX_SIZE);
    strlcpy(mask_copy, mask, IPV4_MAX_SIZE);
    strlcpy(gateway_copy, gateway, IPV4_MAX_SIZE);

    uint8_t u_ip[4], u_mask[4], u_gateway[4];

    err = ipv4_str_to_uint8_array(ip_copy, u_ip);
    if (err != ESP_OK) {
        return err;
    }
    err = ipv4_str_to_uint8_array(mask_copy, u_mask);
    if (err != ESP_OK) {
        return err;
    }
    err = ipv4_str_to_uint8_array(gateway_copy, u_gateway);
    if (err != ESP_OK) {
        return err;
    }

    err = sta_ip_configuration(u_ip, u_mask, u_gateway);

    return err;
}

esp_err_t WiFi::sta_ip_configuration(uint8_t* ip, uint8_t* mask,
                                     uint8_t* gateway) {

    for (int i = 0; i < 4; i++) {
        if (ip + i == NULL || mask + i == NULL || gateway + i == NULL) {
            MY_LOGE("Configuração de IP com endereços inválidos");
            return ESP_FAIL;
        }
    }

    esp_netif_ip_info_t ip_info;
    memset(&ip_info, 0 , sizeof(esp_netif_ip_info_t));

    IP4_ADDR(&ip_info.ip, ip[0], ip[1], ip[2], ip[3]);
    IP4_ADDR(&ip_info.netmask, mask[0], mask[1], mask[2], mask[3]);
    IP4_ADDR(&ip_info.gw, gateway[0], gateway[1], gateway[2], gateway[3]);

    esp_err_t err = sta_ip_configuration(ip_info);

    // Preenchendo variável de IPs formatados em Strings
    if (err == ESP_OK) {
        for (int i = 0; i < 4; i++) {
            _ipv4_info.ip[i] = ip[i];
            _ipv4_info.mask[i] = mask[i];
            _ipv4_info.gateway[i] = gateway[i];
            _ipv4_info.primary_dns[i] = 0;
            _ipv4_info.secondary_dns[i] = 0;
        }
        save_in_nvs(NVS_KEY_STA_IP_CONFIG, (void*)&_ipv4_info,
                    sizeof(ipv4_information_t));
    } else {
        MY_LOGW("IP Settings error: %s", esp_err_to_name(err));
    }

    return err;
}
esp_err_t WiFi::sta_ip_configuration(ipv4_information_t ipv4_info) {

    return sta_ip_configuration(_ipv4_info.ip, _ipv4_info.mask,
                                _ipv4_info.gateway);
}

esp_netif_ip_info_t WiFi::sta_ip_configuration() {
    return _netif_ip_info;
}

wifi_mode_t WiFi::current_mode() {
    return _current_mode;
}

}  // namespace Wetzel
