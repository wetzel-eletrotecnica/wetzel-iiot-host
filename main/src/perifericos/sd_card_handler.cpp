#include "sd_card_handler.h"

#include <driver/sdspi_host.h>
#include <driver/spi_common.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "debug.h"

static const char* TAG = __FILE__;

namespace Wetzel {

CartaoSD* CartaoSD::_instance = nullptr;
// THIS IS A TEST
static esp_err_t s_example_write_file(const char* path, char* data) {
    // MY_LOGI("Opening file %s", path);
    // FILE* f = fopen(path, "ab");
    // if (f == NULL) {
    //     MY_LOGE("Failed to open file for writing");
    //     return ESP_FAIL;
    // }
    // uint8_t id = 12;
    // uint8_t n_lum = 16;
    // uint8_t dim_level = 223;
    // uint32_t dateTime = 1677595987;
    // fwrite(&id, sizeof(id), 1, f);
    // fwrite(&n_lum, sizeof(n_lum), 1, f);
    // fwrite(&dim_level, sizeof(dim_level), 1, f);
    // fwrite(&dateTime, sizeof(dateTime), 1, f);
    // fclose(f);
    // MY_LOGI("File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char* path) {
    // ESP_LOGI(TAG, "Reading file %s", path);
    // FILE* f = fopen(path, "rb");
    // if (f == NULL) {
    //     ESP_LOGE(TAG, "Failed to open file for reading");
    //     return ESP_FAIL;
    // }
    // char line[EXAMPLE_MAX_CHAR_SIZE];
    // while (fgets(line, sizeof(line), f) != NULL) {
    //     ESP_LOGI(TAG, "Read from file: '%s'", line);
    // }
    // fclose(f);

    return ESP_OK;
}
// END OF TEST
CartaoSD::CartaoSD() = default;

CartaoSD::~CartaoSD() {
    delete _instance;
}

CartaoSD* CartaoSD::getInstance() {
    if (_instance == nullptr) {
        _instance = new CartaoSD();
    }
    return _instance;
}
esp_err_t CartaoSD::begin(gpio_num_t mosi_port, gpio_num_t miso_port,

                          gpio_num_t sclk_port, gpio_num_t cs_port) {
    esp_err_t err;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = FORMAT_IF_MOUNT_FAILED,
        .max_files = MAX_FILES_OPENED,
        .allocation_unit_size = ALLOCATION_UNIT_SIZE};

    const char mount_point[] = MOUNT_POINT;
    MY_LOGI("Initializing SD card");
    MY_LOGI("Using SPI peripheral");

    // sdmmc_host_t _host = SDSPI_HOST_DEFAULT();
    sdmmc_host_t aux = SDSPI_HOST_DEFAULT();
    _host = aux;
    _host.max_freq_khz = MAX_FREQ_KHZ;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = mosi_port,
        .miso_io_num = miso_port,
        .sclk_io_num = sclk_port,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
    };
    err = spi_bus_initialize((spi_host_device_t)_host.slot, &bus_cfg,
                             SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        MY_LOGE("Failed to initialize bus.");
        return err;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = cs_port;
    slot_config.host_id = (spi_host_device_t)_host.slot;

    MY_LOGI("%d", _host.slot);
    MY_LOGI("Mounting filesystem");

    uint8_t counter = 0;
    do {
        counter++;
        err = esp_vfs_fat_sdspi_mount(mount_point, &_host, &slot_config,
                                      &mount_config, &_card);
        vTaskDelay(pdMS_TO_TICKS(1));
    } while (err != ESP_OK && counter < MAX_CARD_MOUNT_ATTEMPTS);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            MY_LOGE(

                "Failed to mount filesystem. "
                "If you want the card to be formatted, set the "
                "CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED "
                "menuconfig option.");
        } else {
            MY_LOGE(
                "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors "
                "in place.",
                esp_err_to_name(err));
        }
        return err;
    }
    MY_LOGI("Filesystem mounted");
    sdmmc_card_print_info(stdout, _card);

    memset(writing_file_path,0,sizeof(writing_file_path));
    memset(reading_file_path,0,sizeof(reading_file_path));
    
    return ESP_OK;
}

FILE* CartaoSD::openFile(const char* file_path, file_type_t type) {
    MY_LOGD("Opening file %s", file_path);
    FILE* file = NULL;

    if (file_path == NULL) {
        MY_LOGE("File path NULL ptr");
        return NULL;
    }

    char complete_file_path[25] = "\0";
    strcat(complete_file_path, MOUNT_POINT);
    strcat(complete_file_path, file_path);

    switch (type) {
    case WRITING_FILE:
        if (_writing_file_is_open) {
            MY_LOGD("Writing file already in use. Closing it...");
            closeFile(WRITING_FILE);
        }
        file = fopen(complete_file_path, "ab");
        if (file == NULL) {
            MY_LOGE("Failed to open file for writing");
            return NULL;
        }
        _writing_file = file;
        _writing_file_is_open = true;
        strncpy(writing_file_path, complete_file_path, 25);
        break;
    case READING_FILE:
        if (_reading_file_is_open) {
            MY_LOGD("Reading file already in use. Closing it...");
            closeFile(READING_FILE);
        }
        file = fopen(complete_file_path, "rb");
        if (file == NULL) {
            MY_LOGE("Failed to open file for reading");
            return NULL;
        }
        _reading_file = file;
        _reading_file_is_open = true;
        strncpy(reading_file_path, complete_file_path, 25);
    }
    return file;
}

esp_err_t CartaoSD::closeFile(file_type_t file) {
    enum {
        SUCCESS = 0,
        FILE_IS_NOT_OPENED,
    };

    int result = FILE_IS_NOT_OPENED;
    switch (file) {

    case WRITING_FILE:
        if (_writing_file_is_open) {
            result = fclose(_writing_file);
            _writing_file_is_open = false;
            _writing_file = NULL;
        }
        break;

    case READING_FILE:
        if (_reading_file_is_open) {
            result = fclose(_reading_file);
            _reading_file_is_open = false;
            _reading_file = NULL;
        }
    }

    switch (result) {
    case SUCCESS:
        return ESP_OK;
    case FILE_IS_NOT_OPENED:
        return ESP_ERR_NOT_FOUND;
    default:
        return ESP_FAIL;
    }
}

FILE* CartaoSD::writingFile() {
    return _writing_file;
}

FILE* CartaoSD::readingFile() {
    return _reading_file;
}

char* CartaoSD::writingFilePath() {
    return writing_file_path;
}

char* CartaoSD::readingFilePath() {
    return reading_file_path;
}

// void CartaoSD::test_func() {
//     const char* file_hello = MOUNT_POINT "/hello.txt";
//     char data[EXAMPLE_MAX_CHAR_SIZE];
//     snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello",
//              sd_card->cid.name);
//     ret = s_example_write_file(file_hello, data);
//     if (ret != ESP_OK) {
//         return;
//     }

//     ret = s_example_read_file(file_hello);
//     if (ret != ESP_OK) {
//         return;
//     }

//     esp_vfs_fat_sdcard_unmount(mount_point, sd_card);
//     ESP_LOGI(TAG, "Card unmounted");
// }

}  // namespace Wetzel