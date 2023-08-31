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
    RealTimeClock(/* args */);
    static void PrintDataHourRTC(void * args);
    static RealTimeClock* _instance;
    static tmElements_t obj_time;
    static bool __isModuleStart;

    public:
        void operator=(RealTimeClock const&) = delete;
        virtual ~RealTimeClock();
        esp_err_t begin();
        esp_err_t configureRtc(uint32_t new_unix_seconds);
        uint32_t unixSeconds() const;
        uint32_t unixDay() const;
        tmElements_t dateTime() const;
        static RealTimeClock* getInstance();
};
}  // namespace Wetzel

#endif