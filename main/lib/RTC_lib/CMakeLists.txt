# Adafruit Bus IO Library
# https://github.com/adafruit/Adafruit_BusIO
# MIT License


idf_component_register(SRCS "src/RTC_DS1307.cpp" "src/RTC_DS3231.cpp" "src/RTC_Micros.cpp" "src/RTC_Millis.cpp" "src/RTC_PCF8523.cpp" "src/RTC_PCF8563.cpp" "src/RTClib.cpp" 
                       INCLUDE_DIRS "." "src"
                       REQUIRES Adafruit_BusIO_lib)

