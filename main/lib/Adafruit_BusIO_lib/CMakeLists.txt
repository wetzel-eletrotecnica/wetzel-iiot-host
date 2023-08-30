# Adafruit Bus IO Library
# https://github.com/adafruit/Adafruit_BusIO
# MIT License


idf_component_register(SRCS "Adafruit_I2CDevice.cpp" "Adafruit_BusIO_Register.cpp" "Adafruit_SPIDevice.cpp" 
                       INCLUDE_DIRS "."
                       REQUIRES arduino)

