#include "report_handler.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <map>
#include <unordered_map>

#include "debug.h"
#include "real_time_clock.h"
#include "sd_card_handler.h"

static const char* TAG = __FILE__;

namespace Wetzel {

#define REPORT_HANDLER_DEFAULT_TASK_PRIORITY CONFIG_APP_TASK_DEFAULT_PRIORITY - 2

#define MS_PERIOD_TO_CHECK 1000
#define MS_PERIOD_TO_WRITE_FILE 60000
/**
 * @brief instanciações de variáveis static
 * 
 */
ReportHandler* ReportHandler::_instance = NULL;
TaskHandle_t ReportHandler::writing_file_handle = NULL;
SemaphoreHandle_t ReportHandler::_writing_queue_mutex;
SemaphoreHandle_t ReportHandler::_reading_queue_mutex;
SemaphoreHandle_t ReportHandler::_report_msg_queue_mutex;
SemaphoreHandle_t ReportHandler::_device_info_map_mutex;

std::queue<report_entry_t> ReportHandler::_writing_buffer;
std::queue<report_entry_t> ReportHandler::_reading_buffer;
std::queue<report_msg_entry_t> ReportHandler::_report_msg_buffer;
CartaoSD* ReportHandler::_card = NULL;
RealTimeClock* ReportHandler::_rtc = NULL;
FILE* ReportHandler::_writing_file = NULL;
FILE* ReportHandler::_reading_file = NULL;
int ReportHandler::_current_file_unix_day = 0;
device_report_info_map_t ReportHandler::_mapa_de_info_de_devices;

ReportHandler::ReportHandler() = default;

void ReportHandler::report_entry_handler(void* arg) {
    char msg_holder[20];
    char* strtok_ptr;
    RealTimeClock* rtc = RealTimeClock::getInstance();

    while (1) {
        if (xSemaphoreTake(_report_msg_queue_mutex, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        while (!_report_msg_buffer.empty()) {
            strcpy(msg_holder, _report_msg_buffer.front().msg);

            device_mac_t mac(6);
            uint8_t pwm_value;

            sscanf(strtok_r(msg_holder, ",", &strtok_ptr), "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

            pwm_value = atoi(strtok_r(NULL, ",", &strtok_ptr));

            _report_msg_buffer.pop();
            xSemaphoreGive(_report_msg_queue_mutex);

            auto map_iterator = _mapa_de_info_de_devices.find(mac);

            if (map_iterator == _mapa_de_info_de_devices.end()) {
                MY_LOGE("Dispositivo não configurado para relatório...");
                continue;
            }

            report_entry_t new_report_entry = {
                .device_info =
                    {
                        .id = map_iterator->second.id,
                        .qtd_luminarias = map_iterator->second.qtd_luminarias,
                        .modelo_luminarias = map_iterator->second.modelo_luminarias,
                    },
                .pwm_value = pwm_value,
                .unix_seconds = rtc->unixSeconds(),
            };

            add_report_to_writing_buffer(new_report_entry);
        }
        vTaskDelay(pdMS_TO_TICKS(MS_PERIOD_TO_CHECK));
    }
}

void ReportHandler::writing_file_handler(void* arg) {
    uint32_t ulInterruptStatus;
    TickType_t last_execution_time;
    const TickType_t frequency = pdMS_TO_TICKS(MS_PERIOD_TO_WRITE_FILE);
    last_execution_time = xTaskGetTickCount();
    RealTimeClock* rtc = RealTimeClock::getInstance();
    // first:id     |   second:report_informations
    std::unordered_map<uint8_t, report_sampling_param_t> entradas;

    while (1) {
        vTaskDelayUntil(&last_execution_time, frequency);

        bool new_entry_added = false;
        char file_name[20];
        createFileName(file_name, _rtc->dateTime().year(), _rtc->dateTime().month(),
                       _rtc->dateTime().day(), "txt");

        if (xSemaphoreTake(_writing_queue_mutex, portTICK_RATE_MS) != pdTRUE) {
            continue;
        }
        while (!_writing_buffer.empty()) {
            new_entry_added = true;
            report_entry_t report_entry = _writing_buffer.front();
            _writing_buffer.pop();
            MY_LOGD("Processing new report_entry");
            auto iterator = entradas.find(report_entry.device_info.id);

            // É nova entrada no mapa (ID nao reconhecido)
            if (iterator == entradas.end()) {
                MY_LOGD("New ID found => ID: %d", (iterator->first));
                report_sampling_param_t param = {
                    .last_average_pwm = report_entry.pwm_value,
                    .average_pwm = report_entry.pwm_value,
                    .pwm_i = report_entry.pwm_value,
                    .pwm_i_1 = report_entry.pwm_value,
                    .t_i = report_entry.unix_seconds,
                    .t_i_1 = report_entry.unix_seconds,
                    .t_0 = report_entry.unix_seconds,
                    .n_lum = report_entry.device_info.qtd_luminarias,
                    .lum_type = report_entry.device_info.modelo_luminarias,
                    .is_new_param = true,
                };
                entradas[report_entry.device_info.id] = param;

            } else {
                auto param = &iterator->second;
                MY_LOGD("New entry from ID: %d", (iterator->first));

                // Se for novo ciclo de amostragem
                if (iterator->second.is_new_param == false) {
                    MY_LOGD("First entry from cycle from ID: %d", (iterator->first));

                    param->n_lum = report_entry.device_info.qtd_luminarias;
                    param->lum_type = report_entry.device_info.modelo_luminarias;
                    param->t_0 = param->t_i;
                    param->last_average_pwm = param->pwm_i;
                } else {
                    param->last_average_pwm = param->average_pwm;
                }

                param->pwm_i_1 = param->pwm_i;
                param->t_i_1 = param->t_i;

                param->pwm_i = report_entry.pwm_value;
                param->t_i = report_entry.unix_seconds;
                param->is_new_param = true;

                param->average_pwm = calculate_average_pwm(*param);
                MY_LOGD("AVERAGE_PWM: %d", param->average_pwm);
            }
        }
        xSemaphoreGive(_writing_queue_mutex);

        if (new_entry_added == true) {
            _card->openFile(file_name, WRITING_FILE);
            MY_LOGD("WRITING FILE %s", file_name);
            for (auto iterator = entradas.begin(); iterator != entradas.end(); iterator++) {
                if (iterator->second.is_new_param) {
                    report_entry_t new_entry = {
                        .device_info =
                            {
                                .id = iterator->first,
                                .qtd_luminarias = iterator->second.n_lum,
                                .modelo_luminarias = iterator->second.lum_type,
                            },
                        .pwm_value = iterator->second.average_pwm,
                        .unix_seconds = iterator->second.t_0,
                    };
                    fwrite(&new_entry.device_info.id, sizeof(new_entry.device_info.id), 1,
                           _writing_file);
                    fwrite(&new_entry.device_info.qtd_luminarias,
                           sizeof(new_entry.device_info.qtd_luminarias), 1, _writing_file);
                    fwrite(&new_entry.device_info.modelo_luminarias,
                           sizeof(new_entry.device_info.modelo_luminarias), 1, _writing_file);
                    fwrite(&new_entry.pwm_value, sizeof(new_entry.pwm_value), 1, _writing_file);
                    fwrite(&new_entry.unix_seconds, sizeof(new_entry.unix_seconds), 1,
                           _writing_file);

                    iterator->second.is_new_param = false;
                }
            }
            _card->closeFile(WRITING_FILE);
            MY_LOGD("FILE %s CLOSED", file_name);
        }
    }
}

esp_err_t ReportHandler::createFileName(char* file_name, uint16_t year, uint8_t month, uint8_t day,
                                        char* extension) {
    if (file_name == NULL) {
        return ESP_FAIL;
    }
    if (extension == NULL) {
        sprintf(file_name, "/%02u%02u%02u", year % 100, month, day);
    } else {
        sprintf(file_name, "/%02u%02u%02u.%s", year % 100, month, day, extension);
    }
    return ESP_OK;
}
ReportHandler::~ReportHandler() {
    delete _instance;
}

ReportHandler* ReportHandler::getInstance() {
    if (_instance == nullptr) {
        _instance = new ReportHandler();
    }
    return _instance;
}

esp_err_t ReportHandler::begin() {
    _rtc = RealTimeClock::getInstance();
    _card = CartaoSD::getInstance();

    _reading_queue_mutex = xSemaphoreCreateMutex();
    _writing_queue_mutex = xSemaphoreCreateMutex();
    _report_msg_queue_mutex = xSemaphoreCreateMutex();
    _device_info_map_mutex = xSemaphoreCreateMutex();

    BaseType_t xReturned = pdFAIL;
    writing_file_handle = NULL;
    xReturned = xTaskCreate(
        writing_file_handler,                 /* Function that implements the task. */
        "writing_file_task",                  /* Text name for the task. */
        4 * TASK_STACK_REF_SIZE,              /* Stack size in words, not bytes. */
        NULL,                                 /* Parameter passed into the task. */
        REPORT_HANDLER_DEFAULT_TASK_PRIORITY, /* Priority at which the task is created. */
        &writing_file_handle);                /* Used to pass out the created task's handle. */
    if (xReturned != pdPASS) {
        MY_LOGE("writing_file_task creation failed");
        return ESP_FAIL;
    }

    xReturned = xTaskCreate(
        report_entry_handler,                 /* Function that implements the task. */
        "report_entry_task",                  /* Text name for the task. */
        4 * TASK_STACK_REF_SIZE,              /* Stack size in words, not bytes. */
        NULL,                                 /* Parameter passed into the task. */
        REPORT_HANDLER_DEFAULT_TASK_PRIORITY, /* Priority at which the task is created. */
        NULL);                                /* Used to pass out the created task's handle. */
    if (xReturned != pdPASS) {
        MY_LOGE("report_entry_task creation failed");
        vTaskSuspend(writing_file_handle);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ReportHandler::add_entry_to_report_msg_buffer(report_msg_entry_t& report_msg) {
    esp_err_t err;

    if (xSemaphoreTake(_report_msg_queue_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_FAIL;
    }

    if (_report_msg_buffer.size() > 5) {
        MY_LOGE("Test max size of queue...");
        xSemaphoreGive(_report_msg_queue_mutex);
        return ESP_FAIL;
    }
    _report_msg_buffer.push(report_msg);
    xSemaphoreGive(_report_msg_queue_mutex);

    MY_LOGD("Report_msg added to report_msg_queue: %s", report_msg.msg);

    return ESP_OK;
}

esp_err_t ReportHandler::add_device_to_report_info_map(device_mac_t mac, uint8_t qtd_luminarias,
                                                       luminaria_type_t modelo_luminarias) {

    if (xSemaphoreTake(_device_info_map_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    auto iterator = _mapa_de_info_de_devices.find(mac);
    if (iterator == _mapa_de_info_de_devices.end()) {
        device_report_info_t new_device_info = {
            id: (uint8_t)_mapa_de_info_de_devices.size(),
            qtd_luminarias: qtd_luminarias,
            modelo_luminarias: modelo_luminarias,
        };
        _mapa_de_info_de_devices[mac] = new_device_info;
        MY_LOGD("Novo dispositivo adicionado a mapa de report: " MACSTR
                "   | qtd_lum: %d   | n_lum: %d",
                MAC2STR(mac), qtd_luminarias, modelo_luminarias);
        xSemaphoreGive(_device_info_map_mutex);
        return ESP_OK;
    }
    xSemaphoreGive(_device_info_map_mutex);
    return ESP_FAIL;
}

esp_err_t ReportHandler::add_report_to_writing_buffer(report_entry_t& report) {
    esp_err_t err;

    if (xSemaphoreTake(_writing_queue_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_FAIL;
    }
    _writing_buffer.push(report);
    xTaskNotify(writing_file_handle, 0, eNoAction);
    xSemaphoreGive(_writing_queue_mutex);

    MY_LOGD("Report_entry added to writing file queue");

    return ESP_OK;
}

uint8_t calculate_average_pwm(report_sampling_param_t param) {
    const auto t0 = param.t_0;
    const auto ti = param.t_i;
    const auto ti_1 = param.t_i_1;
    const auto avg_pwm_i_1 = param.last_average_pwm;
    const auto pwm_i = param.pwm_i;
    const auto pwm_i_1 = param.pwm_i_1;

    if (t0 == ti) {
        MY_LOGE("to == ti");
        return 0;
    }

    return (((avg_pwm_i_1 * (ti_1 - t0)) + ((3 * pwm_i_1) - pwm_i) / 2 * (ti_1 - ti)) / (ti - t0));
}
}  // namespace Wetzel