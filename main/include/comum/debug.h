#ifndef _DEBUG_CLASS_
#define _DEBUG_CLASS_

#include <esp_log.h>

#include "configuration.h"

#define TRACE()                        \
  Serial.print(__FILE__);              \
  Serial.print(':');                   \
  Serial.print(__LINE__);              \
  Serial.print(": ");                  \
  Serial.println(__PRETTY_FUNCTION__); \

#ifdef LOG_COLOR_BLACK
#undef LOG_COLOR_BLACK
#endif
#define LOG_COLOR_BLACK "30"
#ifdef LOG_COLOR_RED
#undef LOG_COLOR_RED
#endif
#define LOG_COLOR_RED "31"
#ifdef LOG_COLOR_GREEN
#undef LOG_COLOR_GREEN
#endif
#define LOG_COLOR_GREEN "32"
#ifdef LOG_COLOR_BROWN
#undef LOG_COLOR_BROWN
#endif
#define LOG_COLOR_BROWN "33"
#ifdef LOG_COLOR_BLUE
#undef LOG_COLOR_BLUE
#endif
#define LOG_COLOR_BLUE "34"
#ifdef LOG_COLOR_PURPLE
#undef LOG_COLOR_PURPLE
#endif
#define LOG_COLOR_PURPLE "35"
#ifdef LOG_COLOR_CYAN
#undef LOG_COLOR_CYAN
#endif
#define LOG_COLOR_CYAN "36"
#ifdef LOG_COLOR_WHITE
#undef LOG_COLOR_WHITE
#endif
#define LOG_COLOR_WHITE "37"

#define LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR) "\033[1;" COLOR "m"

#if LOG_OVERRIDE_COLORS
#ifdef LOG_COLOR_E
#undef LOG_COLOR_E
#endif
#define LOG_COLOR_E LOG_COLOR(LOG_COLOR_ERROR)
#ifdef LOG_COLOR_W
#undef LOG_COLOR_W
#endif
#define LOG_COLOR_W LOG_COLOR(LOG_COLOR_WARN)
#ifdef LOG_COLOR_I
#undef LOG_COLOR_I
#endif
#define LOG_COLOR_I LOG_COLOR(LOG_COLOR_INFO)
#ifdef LOG_COLOR_D
#undef LOG_COLOR_D
#endif
#define LOG_COLOR_D LOG_COLOR(LOG_COLOR_DEBUG)
#ifdef LOG_COLOR_V
#undef LOG_COLOR_V
#endif
#define LOG_COLOR_V LOG_COLOR(LOG_COLOR_VERBOSE)
#endif

#define LOG_COLOR_A LOG_COLOR(LOG_COLOR_ATTENTION)
#define LOG_COLOR_T LOG_COLOR(LOG_COLOR_TIMER)

#define MY_LOG_FORMAT(letter, format) LOG_COLOR_##letter #letter " (%u) [%s, %s:%d] -> " format LOG_RESET_COLOR "\n"

#define MY_LOGE(format, ...) esp_log_write(ESP_LOG_ERROR, TAG, MY_LOG_FORMAT(E, format), esp_log_timestamp(), TAG, __func__, __LINE__, ##__VA_ARGS__);
#define MY_LOGW(format, ...) esp_log_write(ESP_LOG_WARN, TAG, MY_LOG_FORMAT(W, format), esp_log_timestamp(), TAG, __func__, __LINE__, ##__VA_ARGS__);
#define MY_LOGI(format, ...) esp_log_write(ESP_LOG_INFO, TAG, MY_LOG_FORMAT(I, format), esp_log_timestamp(), TAG, __func__, __LINE__, ##__VA_ARGS__);
#define MY_LOGD(format, ...) esp_log_write(ESP_LOG_DEBUG, TAG, MY_LOG_FORMAT(D, format), esp_log_timestamp(), TAG, __func__, __LINE__, ##__VA_ARGS__);
#define MY_LOGV(format, ...) esp_log_write(ESP_LOG_VERBOSE, TAG, MY_LOG_FORMAT(V, format), esp_log_timestamp(), TAG, __func__, __LINE__, ##__VA_ARGS__);
#define MY_LOGA(format, ...) esp_log_write(ESP_LOG_INFO, TAG, MY_LOG_FORMAT(A, format), esp_log_timestamp(), TAG, __func__, __LINE__, ##__VA_ARGS__);
#define MY_LOGT(format, ...) esp_log_write(ESP_LOG_INFO, TAG, MY_LOG_FORMAT(T, format), esp_log_timestamp(), TAG, __func__, __LINE__, ##__VA_ARGS__);

#endif
