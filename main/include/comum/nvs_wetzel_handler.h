#ifndef _WETZEL_NVS_MANAGER_H_
#define _WETZEL_NVS_MANAGER_H_

#include <esp_err.h>
#include <stdint.h>

namespace Wetzel {

void save_in_nvs(const char* key, void* blob, size_t len);
void print_nvs_error_in_log(esp_err_t err, const char* identifier);

}  // namespace Wetzel
#endif