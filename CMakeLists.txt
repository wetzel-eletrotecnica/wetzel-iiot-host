# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(ECP $ENV{ESP_COMPONENTS_PATH})
set(EXTRA_COMPONENT_DIRS "${EXTRA_COMPONENT_DIRS} ${ECP} main/lib/RTC_lib main/lib/Adafruit_BusIO_lib")


project(main)
