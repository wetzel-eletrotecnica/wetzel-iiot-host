#ifndef EVENT_LISTENER_LIGHTING_CONTROL_H_
#define EVENT_LISTENER_LIGHTING_CONTROL_H_

#include <stdint.h>

namespace Wetzel{

/**
 * @brief Imprime no LOG de Debug o nome do evento de WiFi relacionado a event_id
 * 
 * @param event_id 
 */
void _print_wifi_event(int32_t event_id);

/**
 * @brief Imprime no LOG de Debug o nome do evento de IP relacionado a event_id
 * 
 * @param event_id 
 */
void _print_ip_event(int32_t event_id);
}

#endif