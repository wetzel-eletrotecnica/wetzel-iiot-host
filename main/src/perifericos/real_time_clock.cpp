#include "real_time_clock.h"

#include <esp_system.h>
#include <sys/time.h>

#include <Wire.h>
#include <driver/gpio.h>
#include <limits.h>
#include <cstring>
#include <sstream>
#include <freertos/timers.h>

#include <time.h>
#include <sys/time.h>

#include "TimeLib.h"
#include "configuration.h"
#include "debug.h"

namespace Wetzel {

bool RealTimeClock::__isModuleStart = 0;
tmElements_t RealTimeClock::obj_time;
RealTimeClock *RealTimeClock::_instance = nullptr;

RealTimeClock::RealTimeClock() {}

/**
 * @brief Inicializa a estrutura para monitorar o tempo em unix
 * @return esp_err_t 
*/
esp_err_t RealTimeClock::begin() 
{
    // Cleaning the variables
    std::memset(&obj_time, 0, sizeof(obj_time));
    // Forçar 0 para inciar em 1970
    configureRtc(0);
    __isModuleStart = 1;
    // Se quiser ver o horário (APENAS PARA DEBUG)
    TimerHandle_t t_d = xTimerCreate("PrintDataHourRTC", 1000, true, nullptr, PrintDataHourRTC);
    xTimerStart(t_d, 0);

    return ESP_OK;
}

/**
 * @brief Destrutor
*/
RealTimeClock::~RealTimeClock() {
    delete _instance;
}

/**
 * @brief Substitui o unix tempo do device pelo parametro
 * @param new_unix_seconds novo tempo poxis
 * @return esp_err_t
*/
esp_err_t RealTimeClock::configureRtc(uint32_t new_unix_seconds) 
{
    // Estrutura temporária para a funcao abaixo 
    struct timeval tv; 

    if (new_unix_seconds > 0)
    {
        tv.tv_sec = new_unix_seconds - 1020 ;// - 10800 - 1080;
    }
    else
    {
        /* escrever zero dá problema, então começa 1 segundo depois de 1970 */
        tv.tv_sec = 1;
    }

    // Configura o RTC com a hora atual 
    settimeofday(&tv, 0);

    return ESP_OK;
}

/**
 * @brief Retorna o tempo unix atual em segundos
 * @return uint32_t
*/
uint32_t RealTimeClock::unixSeconds() const 
{
    time_t unix_sec_time = time(NULL);
    return second(unix_sec_time);
}

/**
 * @brief Retorna o unix time do dia atual
 * @return uint32_t
*/
uint32_t RealTimeClock::unixDay() const 
{
    time_t unix_day_time = time(NULL);
    return day(unix_day_time);
}

/**
 * @brief Retorna uma cópia do objeto de tempo do sistema
 * @return tmElements_t
*/
tmElements_t RealTimeClock::dateTime() const
{
    tmElements_t buffer;
    std::memcpy(&buffer, &obj_time, sizeof(tmElements_t));
    return buffer;
}

/**
 * @brief Retorna o Objeto de classe
 * @return RealTimeClock
*/
RealTimeClock* RealTimeClock::getInstance() {
    if (_instance == nullptr) {
        _instance = new RealTimeClock();
    }
    return _instance;
}

void RealTimeClock::PrintDataHourRTC(void * args)
{
    std::stringstream buffer;
    
    // Pega horario do RTC  
    time_t time_in_seconds = time(NULL);
    
    buffer << hour(time_in_seconds)   << ":" 
           << minute(time_in_seconds) << ":" 
           << second(time_in_seconds) << "_"
           << day(time_in_seconds)    << "/"
           << month(time_in_seconds)  << "/"
           << year(time_in_seconds);

    ESP_LOGI("--RTC--", "%s", buffer.str().c_str());
}


}  // namespace Wetzel