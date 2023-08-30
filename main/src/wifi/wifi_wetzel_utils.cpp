#include "wifi_wetzel_utils.h"

#include <string.h>
#include <string>
#include <stdio.h>

#include "wifi_wetzel_constants.h"
#include "debug.h"

static const char* TAG = __FILE__;

namespace Wetzel {

esp_err_t ipv4_str_to_uint8_array(char* str, uint8_t* int_array) {
    esp_err_t err = ESP_OK;

    if (int_array == NULL) {
        err = ESP_FAIL;
    }

    // Verifica se String é válida
    for (uint8_t i = 0; i < IPV4_MAX_SIZE; i++) {
        if (str + i == NULL) {
            err = ESP_FAIL;
            break;
        }
        if (str[i] == '\0') {
            break;
        }
    }

    if (err == ESP_FAIL) {
        MY_LOGE("Conversão abortada -> Parâmetros inválidos");
        return err;
    }

    char* next_char;
    for (uint8_t i = 0; i < 4; i++) {
        char* token;
        if (i != 0) {
            token = strtok_r(NULL, ".", &next_char);
        } else {
            token = strtok_r(str, ".", &next_char);
        }
        if (token == NULL) {
            MY_LOGE("Formato de IPv4 inválido");
            return ESP_FAIL;
        }
        int_array[i] = (uint8_t)atoi(token);
    }

    return ESP_OK;
}

/**
 * @brief Converte array contendo partes do IPv4 em uma string formatada no formato x.x.x.x
 * string deve possuir memória alocada o suficiente para os inteiros + 4 caracteres (três '.' e um '\0')
 * 
 * @param ipv4_array array de 4 posições com os valores do IPv4
 * @param str String onde o resultado será retornado
 * @return erp_err_t ESP_TRUE if success, ESP_FAIL otherwise
 */
esp_err_t ipv4_uint8_array_to_str(uint8_t* ipv4_array, char* str) {
    if (str == NULL) {
        MY_LOGE("String inválida");
        return ESP_FAIL;
    }
    for (uint8_t i = 0; i < 4; i++) {
        if (ipv4_array + i == NULL) {
            MY_LOGE("IPv4 array inválido");
            return ESP_FAIL;
        }
    }
    snprintf(str, IPV4_MAX_SIZE, "%u.%u.%u.%u", (unsigned int)ipv4_array[0],
             (unsigned int)ipv4_array[1], (unsigned int)ipv4_array[2],
             (unsigned int)ipv4_array[3]);
    
    return ESP_OK;
}

void get_mac_str_to_int8(const std::string & s, uint8_t ret[6]) {
    MY_LOGD("%s", s.c_str());
    for (int i = 0; i < 6; i++) {
        ret[i] = char_to_hex(s[3 * i]) * 16 + char_to_hex(s[3 * i + 1]);
    }
}

bool is_mac_equal(const uint8_t first[6], const uint8_t second[6]) {
    for (uint8_t i = 0; i < 6; i++) {
        if (first[i] != second[i])
            return false;
    }
    return true;
}

bool is_mac_broadcast(const uint8_t addr[6]) {
    for (uint8_t i = 0; i < 6; i++) {
        if (addr[i] != 0xff)
            return false;
    }
    return true;
}

uint8_t char_to_hex(const char& c) {
    return c <= '9' && c >= '0'   ? c - '0'
           : c <= 'Z' && c >= 'A' ? c - 'A' + 10
           : c <= 'z' && c >= 'a' ? c - 'a' + 10
                                  : 255;
}

}  // namespace Wetzel
