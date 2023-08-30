#include "real_time_clock.h"

#include <Wire.h>
#include <driver/gpio.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <limits.h>

#include "configuration.h"
#include "debug.h"

namespace Wetzel {

static const char* TAG = __FILE__;

RTC_DS3231 RealTimeClock::_rtc;
RealTimeClock* RealTimeClock::_instance = nullptr;
uint32_t RealTimeClock::_unixSeconds;
uint16_t RealTimeClock::_unixDay;
DateTime RealTimeClock::_dateTime;
SemaphoreHandle_t RealTimeClock::_rtc_mutex;
TaskHandle_t RealTimeClock::update_local_clock_1s_handle = NULL;

bool RealTimeClock::lost_track = false;

RealTimeClock::RealTimeClock() {
    _unixSeconds = _dateTime.unixtime();
}

void RealTimeClock::update_local_clock_1s_interrupt(void* task_handle) {
    TaskHandle_t* handle = (TaskHandle_t*)task_handle;
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

    xTaskNotifyFromISR(*handle, 0, eNoAction, &pxHigherPriorityTaskWoken);
}

void RealTimeClock::update_local_clock_1s_handler(void* arg) {
    uint32_t ulInterruptStatus;

    while (1) {
        xTaskNotifyWait(0, ULONG_MAX, &ulInterruptStatus, portMAX_DELAY);

        if (xSemaphoreTake(_rtc_mutex, 1000 / portTICK_RATE_MS) !=
            pdTRUE) { 
            lost_track = true;
            return;
        }
        if (lost_track) {
            _dateTime = _rtc.now();
            _unixSeconds = _dateTime.unixtime();
            lost_track = false;
        } else {
            _unixSeconds++;
            _dateTime = DateTime(_unixSeconds);
        }
        xSemaphoreGive(_rtc_mutex);
    }
}

RealTimeClock::~RealTimeClock() {
    delete _instance;
}

esp_err_t RealTimeClock::begin(TwoWire* wireInstance) {

    if (rtc_initialized) {
        MY_LOGE("RTC já inicializado");
        return ESP_FAIL;
    }

    if (_rtc.begin(wireInstance) == false) {
        return ESP_FAIL;
    }

    _rtc.writeSqwPinMode(Ds3231SqwPinMode::DS3231_SquareWave1Hz);

    _rtc_mutex = xSemaphoreCreateMutex();

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << SQW_GPIO;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3));
    BaseType_t xReturned = pdFAIL;
    update_local_clock_1s_handle = NULL;
    xReturned = xTaskCreate(
        update_local_clock_1s_handler, /* Function that implements the task. */
        "Update_Local_Clock_Task",     /* Text name for the task. */
        2056,                          /* Stack size in words, not bytes. */
        NULL,                          /* Parameter passed into the task. */
        UPDATE_LOCAL_CLOCK_TASK_PRIORITY, /* Priority at which the task is created. */
        &update_local_clock_1s_handle); /* Used to pass out the created task's handle. */
    if (xReturned != pdPASS) {
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(update_local_clock_with_rtc());

    ESP_ERROR_CHECK(gpio_isr_handler_add(SQW_GPIO,
                                         update_local_clock_1s_interrupt,
                                         &update_local_clock_1s_handle));

    rtc_initialized = true;

    return ESP_OK;
}

esp_err_t RealTimeClock::update_local_clock_with_rtc() {
    if (xSemaphoreTake(_rtc_mutex, 1000 / portTICK_RATE_MS) !=
        pdTRUE) {  // TODO colocar método de segurança caso falhe
        return ESP_FAIL;
    }
    _dateTime = _rtc.now();
    _unixSeconds = _dateTime.unixtime();
    _unixDay = _unixSeconds / (60 * 60 * 24);
    xSemaphoreGive(_rtc_mutex);
    return ESP_OK;
}

esp_err_t RealTimeClock::configureRtc(uint32_t new_unix_seconds) {
    esp_err_t err;
    DateTime new_date_time(new_unix_seconds);
    _rtc.adjust(new_date_time);
    err = update_local_clock_with_rtc();
    return err;
}

RTC_DS3231* RealTimeClock::rtc() {
    return &_rtc;
}

uint32_t RealTimeClock::unixSeconds() const {
    return _unixSeconds;
}

uint16_t RealTimeClock::unixDay() const {
    return _unixDay;
}

DateTime RealTimeClock::dateTime() const {
    return _dateTime;
}

RealTimeClock* RealTimeClock::getInstance() {
    if (_instance == nullptr) {
        _instance = new RealTimeClock();
    }
    return _instance;
}
}  // namespace Wetzel