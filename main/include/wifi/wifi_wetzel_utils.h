#ifndef WIFI_WETZEL_UTILS_H_
#define WIFI_WETZEL_UTILS_H_

#include <esp_err.h>
#include <string>

namespace Wetzel {

esp_err_t ipv4_uint8_array_to_str(uint8_t* ipv4_array, char* str);
esp_err_t ipv4_str_to_uint8_array(char* str, uint8_t* ipv4_array);
void get_mac_str_to_int8(const std::string & s, uint8_t ret[6]);
bool is_mac_equal(const uint8_t first[6], const uint8_t second[6]);
bool is_mac_broadcast(const uint8_t addr[6]);
uint8_t char_to_hex(const char& c);
}  // namespace Wetzel

#endif