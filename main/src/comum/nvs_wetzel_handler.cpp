#include "nvs_wetzel_handler.h"

#include <nvs.h>
#include <nvs_handle.hpp>
#include <nvs_flash.h>

#include "debug.h"

static const char* TAG = __FILE__;

namespace Wetzel {

void save_in_nvs(const char* key, void* blob, size_t len) {
    esp_err_t err;

    std::unique_ptr<nvs::NVSHandle> handle =
        nvs::open_nvs_handle("nvs", NVS_READWRITE, &err);

    if (err != ESP_OK) {
        print_nvs_error_in_log(err, "Inicializacao");
        return;
    }

    err = handle->set_blob(key, blob, len);
    print_nvs_error_in_log(err, key);
    handle->commit();
}

void print_nvs_error_in_log(esp_err_t err, const char* identifier) {
    switch (err) {
        case ESP_OK:
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            MY_LOGI("NVS %s não encontrado.", identifier);
            break;
        default:
            MY_LOGW("Error (%s) reading!\n", esp_err_to_name(err));
    }
}

}  // namespace Wetzel