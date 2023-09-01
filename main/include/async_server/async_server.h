#ifndef ASYNC_SERVER_H_
#define ASYNC_SERVER_H_

#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>

#include "configuration.h"

#define DIRECT_MSG_FINAL_RESPONSE_OK "009,OK,"
#define DIRECT_MSG_FINAL_RESPONSE_NOK "009,NOK,"
#define DIRECT_MSG_MULTI_RESPONSE_OK "007,OK,"
#define END_OF_MESSAGE_IDENTIFIER ";"

namespace Wetzel {

enum Wifi_Status : uint8_t {
    WIFI_NOT_CONNECTED = 1,
    WIFI_CONNECTED,
};

class AsyncServer {
   public:
    AsyncServer() {}
    ~AsyncServer() {}
    static void begin();
    static void begin_task();
    static bool _att_wifi_ssid(bool send_data);
    static SemaphoreHandle_t _uart_mutex;
    static void check_mesh_connection_task(void* arg);
    static void read_report_in_serial_task(void* arg);
    static char http_packet_buffer[ASYNC_HTTP_PACKET_BUFFER_SIZE];
    static uint32_t http_requests_received;

   private:
    static void _send_ans_to_app(void* param);
    static void _send_data_to_sensor(const char* msg);
    static void _update_ssid(char* msg);
    static uint16_t _wait_for_ok(char* response = NULL);
    static esp_err_t echo_post_handler(httpd_req_t* req);
    static esp_err_t out_get_handler(httpd_req_t* req);
    static esp_err_t ans_get_handler(httpd_req_t* req);
    static esp_err_t upgrade_post_handler(httpd_req_t* req);
    static esp_err_t direct_msg_handler(httpd_req_t* req);
    static esp_err_t report_msg_handler(httpd_req_t* req);
    static httpd_handle_t start_webserver(void);
    static void stop_webserver(httpd_handle_t server);
    static void disconnect_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);
    static void connect_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
    static void wifi_any_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);
    static void ip_any_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
    static void _device_response_string(char* response);
    static void _device_response_from_uart_task(httpd_req_t* req = nullptr);
    static void _uart_receive_response_task(void* arg);

    static QueueHandle_t _queue_uart_msg;
    static bool _msg_received;
    static bool _client_is_connected;
    static char _msg[ASYNC_SRV_MSG_LENGTH];
    static bool _http_request;
    static bool _server2_begin;
    static bool _read_report_task_block;
    static uint8_t _reset_counter;
    static char _last_ssid[33];
    static Wifi_Status _last_status;
    static char _last_password[65];
    static bool _waiting_for_ans;
    static bool _uart_response_is_complete;
    static bool _uart_response_is_timeout;
    static const httpd_uri_t echo;
    static const httpd_uri_t out;
    static const httpd_uri_t ans;
    static const httpd_uri_t upgrade;
    static const httpd_uri_t direct;
    static const httpd_uri_t report_dados;
    // static WiFiServer* server2;
};
}  // namespace Wetzel
#endif