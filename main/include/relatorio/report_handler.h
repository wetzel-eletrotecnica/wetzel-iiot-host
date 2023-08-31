#ifndef REPORT_HANDLER_H_
#define REPORT_HANDLER_H_

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <map>
#include <queue>

#include <freertos/portmacro.h>

#include "real_time_clock.h"
#include "sd_card_handler.h"

namespace Wetzel {

typedef enum { LUM_17K, LUM_23K, LUM_32K } luminaria_type_catalog_t;
typedef uint8_t luminaria_type_t;

typedef std::vector<uint8_t> device_mac_t;

// typedef uint8_t device_id_t;

typedef struct {
    char msg[18];
} report_msg_entry_t;

typedef struct {
    uint8_t id;
    uint8_t qtd_luminarias;
    luminaria_type_t modelo_luminarias;
} device_report_info_t;

/**
 * @brief Estrutura auxiliar para fazer amostragem de reports a cada 1minuto
 * 
 */
typedef struct {
    uint8_t last_average_pwm;
    uint8_t average_pwm;
    uint8_t pwm_i;
    uint8_t pwm_i_1;
    uint32_t t_i;
    uint32_t t_i_1;
    uint32_t t_0;
    uint8_t n_lum;
    luminaria_type_t lum_type;
    bool is_new_param;
} report_sampling_param_t;

typedef std::map<device_mac_t, device_report_info_t> device_report_info_map_t;

typedef struct report_entry_t {
    device_report_info_t device_info;
    uint8_t pwm_value;
    uint32_t unix_seconds;
};

uint8_t calculate_average_pwm(report_sampling_param_t param);

class ReportHandler {
   private:
    ReportHandler();

    static ReportHandler* _instance;

    static CartaoSD* _card;
    static RealTimeClock* _rtc;

    static FILE* _writing_file;
    static FILE* _reading_file;

    static std::queue<report_entry_t> _writing_buffer;
    static std::queue<report_entry_t> _reading_buffer;
    static std::queue<report_msg_entry_t> _report_msg_buffer;

    static SemaphoreHandle_t _writing_queue_mutex;
    static SemaphoreHandle_t _reading_queue_mutex;
    static SemaphoreHandle_t _report_msg_queue_mutex;
    static SemaphoreHandle_t _device_info_map_mutex;

    static TaskHandle_t writing_file_handle;

    static int _current_file_unix_day;

    static device_report_info_map_t _mapa_de_info_de_devices;
    static void writing_file_handler(void* arg);
    static void report_entry_handler(void* arg);

    static esp_err_t createFileName(char* file_name, uint16_t year, uint8_t month, uint8_t day,
                                    char* extension);

   public:
    ~ReportHandler();

    static ReportHandler* getInstance();
    esp_err_t begin();

    static esp_err_t add_report_to_writing_buffer(report_entry_t& report);
    static esp_err_t add_entry_to_report_msg_buffer(report_msg_entry_t& report_msg);
    static esp_err_t add_device_to_report_info_map(device_mac_t mac, uint8_t qtd_luminarias,
                                                   luminaria_type_t modelo_luminarias);
};

}  // namespace Wetzel
#endif