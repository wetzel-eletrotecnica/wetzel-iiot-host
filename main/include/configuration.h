#ifndef WETZEL_IIOT_INTERFACE_CONFIGURATION_H_
#define WETZEL_IIOT_INTERFACE_CONFIGURATION_H_
/**
 * =========================================================
 *                          VERSAO
 * =========================================================
*/
#define FIRMWARE_VERSION                                    "1.2.0"
/**
 * =========================================================
 *                       TIPO DEVICE
 * =========================================================
*/
#define RELATORIO_NONE                                      0x00
#define RELATORIO_LOCAL                                     0x01
#define RELATORIO_CLOUD                                     0x02
// Defini o tipo do dispositivo
#define TIPO_INTERFACE                                      RELATORIO_LOCAL
/**
 * =========================================================
 *                          PINOUT
 * =========================================================
*/
//RTC
#define SQW_GPIO                                            GPIO_NUM_23
//SPISD
#define SPI_CS_GPIO                                         GPIO_NUM_13
#define SPI_SCLK_GPIO                                       GPIO_NUM_14
#define SPI_MOSI_GPIO                                       GPIO_NUM_15
#define SPI_MISO_GPIO                                       GPIO_NUM_4
// Feedback conecao celular
#define LED_PIN                                             GPIO_NUM_25
/**
 * =========================================================
 *                COMUNICACAO ENTRE ESPs (UART2)
 * =========================================================
*/
#define UART_QUEUE_LENGTH                                   40
#define UART_QUEUE_MSG_LENGTH                               500
#define UART_BAUD_RATE                                      230400
#define UART_RX_BUFFER                                      1024
/**
 * =========================================================
 *                         FREERTOS
 * =========================================================
*/
#define UPDATE_LOCAL_CLOCK_TASK_PRIORITY                    5
#define TASK_STACK_REF_SIZE                                 1024
#define CONFIG_APP_TASK_DEFAULT_PRIORITY                    6

#define DEVICE_XQUEUE_SEND_WAIT_MS                          100 / portTICK_RATE_MS
//my_async_server
#define ASYNC_SRV_MSG_LENGTH                                500
#define ASYNC_SRV_SERVER_PORT                               80
#define ASYNC_SRV_CHECK_MESH_TASK_STACK_SIZE                8 * TASK_STACK_REF_SIZE
#define ASYNC_SRV_READ_REPORT_TASK_STACK_SIZE               8 * TASK_STACK_REF_SIZE
#define ASYNC_SRV_CHECK_MESH_TASK_PRIORITY                  CONFIG_APP_TASK_DEFAULT_PRIORITY - 3
#define ASYNC_SRV_REPORT_READ_TASK_PRIORITY                 CONFIG_APP_TASK_DEFAULT_PRIORITY - 2

#define ASYNC_SRV_WB_TASK_STACK_SIZE                        8 * TASK_STACK_REF_SIZE
// #define ASYNC_SRV_WB_TASK_PRIORITY                          CONFIG_APP_TASK_DEFAULT_PRIORITY - 1
#define ASYNC_SRV_WB_TASK_FIRST_DELAY_MS                    3000
#define ASYNC_SRV_WB_TASK_HTTP_REQUEST_TIMEOUT_MS           4000
#define ASYNC_SRV_WAIT_OK_TIMEOUT_MS                        4000
#define ASYNC_SRV_D_RESPONSE_REQUEST_TIMEOUT_MS             10000
#define ASYNC_SRV_CLIENT_RESPONSE_REQUEST_TIMEOUT_MS        10000
#define ASYNC_SRV_CHECK_CONNECTION_TASK_DELAY_MS            10000
#define ASYNC_HTTP_PACKET_BUFFER_SIZE                       4095 //tamanho exato do bloco do SPIFFS
// HTTP
#define HANDLE_HTTP_RESPONSE_CODE_OK                        200
#define HANDLE_HTTP_RESPONSE_CODE_NOT_OK                    500
#define HANDLE_HTTP_RESPONSE_CODE_TIEMOUT                   504
/**
 * =========================================================
 *                           RSSI
 * =========================================================
*/
#define MAIN_RSSI_CHECK_TIMER_INTERVAL                      3000
#define MAIN_RSSI_CHECK_TIMER_RESTART_DELAY_MS              5000
#define MAIN_RSSI_CHECK_TIMER_MIN_RSSI                      -75
/**
 * =========================================================
 *                         LOG
 * =========================================================
*/
#ifndef LOG_COLOR_WHITE
#define LOG_COLOR_WHITE
#endif

#define LOG_OVERRIDE_COLORS                                 FALSE
#define LOG_COLOR_ERROR                                     LOG_COLOR_RED
#define LOG_COLOR_WARN                                      LOG_COLOR_BROWN
#define LOG_COLOR_INFO                                      LOG_COLOR_GREEN
#define LOG_COLOR_DEBUG                                     LOG_COLOR_BLUE
#define LOG_COLOR_VERBOSE                                   LOG_COLOR_BROWN
#define LOG_COLOR_ATTENTION                                 LOG_COLOR_CYAN
#define LOG_COLOR_TIMER                                     LOG_COLOR_BLUE

#endif