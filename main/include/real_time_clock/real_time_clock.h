#ifndef REAL_TIME_CLOCK_H_
#define REAL_TIME_CLOCK_H_

//#include <RTClib.h>
//#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include "TimeLib.h"
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace Wetzel {
class RealTimeClock {

private:
    /**
     * @brief Inicializa a estrutura para monitorar o tempo em unix
     * @return esp_err_t 
    */
    RealTimeClock(/* args */);

    /**
     * @brief Destrutor
    */
    static void PrintDataHourRTC(void * args);
    static RealTimeClock * _instance;
    static tmElements_t obj_time;
    static bool __isModuleStart;

public:
    void operator=(RealTimeClock const&) = delete;
    virtual ~RealTimeClock();

    /**
     * @brief Inicializa a estrutura RTC
     * @return esp_err_t
    */
    esp_err_t begin();

    /**
     * @brief Substitui o unix tempo do device pelo parametro
     * @param new_unix_seconds novo tempo poxis
     * @return esp_err_t
    */
    esp_err_t configureRtc(uint32_t new_unix_seconds);

    /**
     * @brief Retorna o tempo unix atual em segundos
     * @return uint32_t
    */
    uint32_t unixSeconds() const;

    /**
     * @brief Retorna o unix time do dia atual
     * @return uint32_t
    */
    uint32_t unixDay() const;

    /**
     * @brief Retorna uma cópia do objeto de tempo do sistema
     * @return tmElements_t
    */
    tmElements_t dateTime() const;

    /**
     * @brief Retorna o Objeto de classe
     * @return RealTimeClock
    */
    static RealTimeClock* getInstance();
};
}  // namespace Wetzel

#endif