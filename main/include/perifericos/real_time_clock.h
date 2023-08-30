#ifndef REAL_TIME_CLOCK_H_
#define REAL_TIME_CLOCK_H_

#include <RTClib.h>
#include <Wire.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace Wetzel {
class RealTimeClock {

   private:
    RealTimeClock(/* args */);

    /**
     * @brief Interrupção ativada pelo módulo RTC a cada 1 segundo.
     * Libera a task do RTC para fazer update das variáveis de tempo.
     * 
     * @param task_handle Handle da task, para utilização de Notify
     */
    static void update_local_clock_1s_interrupt(void* task_handle);

    /**
     * @brief Handler da Task de update de variáveis de tempo do RTC.
     * 
     * @param arg 
     */
    static void update_local_clock_1s_handler(void* arg);

    /**
     * @brief Atualiza variáveis locais com os valroes do módulo físico do RTC.
     * 
     * @return esp_err_t 
     */
    esp_err_t update_local_clock_with_rtc();
    /* data */

    static RealTimeClock* _instance;
    static RTC_DS3231 _rtc;
    bool rtc_initialized = false;

    static TaskHandle_t update_local_clock_1s_handle;

    static uint32_t _unixSeconds;
    static DateTime _dateTime;
    static uint16_t _unixDay;

    static bool lost_track;

   public:
    void operator=(RealTimeClock const&) = delete;
    static RealTimeClock* getInstance();
    virtual ~RealTimeClock();

    /**
     * @brief Inicializa módulo RTC, adquire data e hora atual e inicializa tasks e interrupções.
     * 
     * @param wireInstance Classe da biblioteca Arduino para comunicações I2C
     * @return esp_err_t 
     */
    esp_err_t begin(TwoWire* wireInstance = &Wire);

    uint32_t unixSeconds() const;
    uint16_t unixDay() const;
    DateTime dateTime() const;

    RTC_DS3231* rtc();

    /**
     * @brief Método para configurar data e hora do módulo físico RTC.
     * 
     * @param new_unix_seconds Tempo em segundos UNIX atual.
     * @return esp_err_t 
     */
    esp_err_t configureRtc(uint32_t new_unix_seconds);

    static SemaphoreHandle_t _rtc_mutex;
};
}  // namespace Wetzel

#endif