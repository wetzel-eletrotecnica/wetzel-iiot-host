#include "async_server.h"

#include <HardwareSerial.h>
#include <IPAddress.h>
#include <driver/uart.h>
#include <errno.h>
#include <freertos/portmacro.h>
#include <sys\stat.h>

#include <string>

#include "debug.h"
#include "macros.h"
#include "report_direct_msg_handlers.h"
#include "report_handler.h"
#include "server_html_utils.h"
#include "wifi_direct_msg_handlers.h"
#include "wifi_event_listener.h"
#include "wifi_wetzel_esp32.h"

/* Modulo de relatório */
#include "local_storage.h"
#include "API_ModuloRelatorio.h"

namespace Wetzel
{

    static const char *TAG = __FILE__;
    // static const char* _TEXT_PARAM = "text";

    bool AsyncServer::_read_report_task_block = false;
    bool AsyncServer::_msg_received = false;
    char AsyncServer::_msg[ASYNC_SRV_MSG_LENGTH];
    bool AsyncServer::_client_is_connected = false;
    uint8_t AsyncServer::_reset_counter = 0;
    SemaphoreHandle_t AsyncServer::_uart_mutex;
    bool AsyncServer::_http_request = false;
    char AsyncServer::_last_ssid[33] = "";
    Wifi_Status AsyncServer::_last_status = Wifi_Status::WIFI_NOT_CONNECTED;
    char AsyncServer::_last_password[65] = "";
    bool AsyncServer::_waiting_for_ans = false;
    char AsyncServer::http_packet_buffer[ASYNC_HTTP_PACKET_BUFFER_SIZE];
    QueueHandle_t AsyncServer::_queue_uart_msg;
    bool AsyncServer::_uart_response_is_complete = false;
    bool AsyncServer::_uart_response_is_timeout = false;
    uint32_t AsyncServer::http_requests_received = 0;
    const httpd_uri_t AsyncServer::echo = {.uri = "/echo",
                                           .method = HTTP_GET,
                                           .handler = echo_post_handler,
                                           .user_ctx = NULL};
    const httpd_uri_t AsyncServer::ans = {.uri = "/ans",
                                          .method = HTTP_GET,
                                          .handler = ans_get_handler,
                                          .user_ctx = NULL};
    const httpd_uri_t AsyncServer::out = {.uri = "/out",
                                          .method = HTTP_GET,
                                          .handler = out_get_handler,
                                          .user_ctx = NULL};
    const httpd_uri_t AsyncServer::upgrade = {.uri = "/upgrade",
                                              .method = HTTP_POST,
                                              .handler = upgrade_post_handler,
                                              .user_ctx = NULL};
    const httpd_uri_t AsyncServer::direct = {.uri = "/direct",
                                             .method = HTTP_GET,
                                             .handler = direct_msg_handler,
                                             .user_ctx = NULL};

    /**
     * Estrutura para criar uma rota para enviar o report gasto das luminarias
     */
    const httpd_uri_t AsyncServer::report_dados = {.uri = "/report",
                                                   .method = HTTP_GET,
                                                   .handler = report_msg_handler,
                                                   .user_ctx = NULL};

    uint32_t _http_timer_counter = 0;

    void AsyncServer::begin()
    {
        static httpd_handle_t server = NULL;
        BaseType_t xReturned;
        IPAddress IP;
        _queue_uart_msg = xQueueCreate(UART_QUEUE_LENGTH, UART_QUEUE_MSG_LENGTH);
        // ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                                   &ip_any_handler, &server));
        // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   &wifi_any_handler, &server));
        // esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, disconnect_handler, &server);
        server = start_webserver();

        // MY_LOGI("AP IP address: %s", IP.toString().c_str());
        _uart_mutex = xSemaphoreCreateMutex();

        xReturned =
            xTaskCreate(check_mesh_connection_task, "check_mesh_connection_task",
                        ASYNC_SRV_CHECK_MESH_TASK_STACK_SIZE, NULL,
                        ASYNC_SRV_CHECK_MESH_TASK_PRIORITY, NULL);

        if (xReturned != pdPASS)
        {
            MY_LOGI("Nao foi possivel criar a _webserver_task");
            abort();
        }

        xReturned =
            xTaskCreate(read_report_in_serial_task, "read_report_in_serial_task",
                        ASYNC_SRV_CHECK_MESH_TASK_STACK_SIZE, NULL,
                        ASYNC_SRV_REPORT_READ_TASK_PRIORITY, NULL);

        if (xReturned != pdPASS)
        {
            MY_LOGI("Nao foi possivel criar a _serial_report_task");
            abort();
        }
    }

    uint16_t AsyncServer::_wait_for_ok(char *response)
    {
        uint32_t tempo = millis();
        char ent[ASYNC_SRV_MSG_LENGTH + 1];
        bool ok_received = false;
        bool not_ok_received = false;
        // limpa qualquer mensagem de entrada antes de começar a requisicao.
        while (millis() - tempo < ASYNC_SRV_WAIT_OK_TIMEOUT_MS)
        {
            if (Serial2.available())
            {
                memset(ent, 0, ASYNC_SRV_MSG_LENGTH);
                MY_LOGI("Tem dado a ser lido UART2");
                MY_LOGI("Antes do readBytes");
                Serial2.readBytesUntil(';', ent, ASYNC_SRV_MSG_LENGTH);

                if (ent[0] ==
                    '#')
                { // ignora mensagem de relatorio, sinalizada pela inicio '#'
                    MY_LOGD("Mensagem de report ignorada: %s", ent);
                    report_msg_entry_t new_entry;
                    strcpy(new_entry.msg, ent + 4);
                    ReportHandler::add_entry_to_report_msg_buffer(new_entry);
                    continue;
                }

                MY_LOGI("Dado lido: %s", ent);
                strcat(ent, ";");
                MY_LOGI("%s", ent);
                // if(strstr(ent, "ok;") != NULL)
                if (strncmp(ent, "ok,", 3) == 0)
                    ok_received = true;
                else
                    not_ok_received = true;
                break;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        if (response != NULL)
            strcpy(response, ent);

        // se recebeu o ok, cria a task que recebe as mensagens e repassa para o app
        if (ok_received)
        {
            MY_LOGI("Ok recebido");
            return HANDLE_HTTP_RESPONSE_CODE_OK;
            // if(create_task) xTaskCreate(_send_ans_to_app, "send_ans_to_app", 10 * 1024, NULL, 20, NULL);
        }
        else if (not_ok_received)
        {
            MY_LOGI("Not Ok recebido");
            return HANDLE_HTTP_RESPONSE_CODE_NOT_OK;
        }
        else
        {
            MY_LOGI("Timeout!");
            return HANDLE_HTTP_RESPONSE_CODE_TIEMOUT;
        }
    }

    void AsyncServer::_send_data_to_sensor(const char *msg)
    {
        Serial2.flush();
        const uint16_t SEND_LEN = ASYNC_SRV_MSG_LENGTH; // TODO: Aqui antes era uint8_t mas estava errado
        uint16_t len = strlen(msg);
        uint16_t i = 0;
        uint8_t sum;
        char send[SEND_LEN + 1];
        MY_LOGI("Mensagem completa: %s", msg);
        do
        {
            sum = (len - i > SEND_LEN) ? SEND_LEN : len - i + 1;
            memcpy(send, msg + i, sum);
            i += sum;
            MY_LOGI("Enviando %d : %s", sum, send);
            Serial2.print(send);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        } while (i < len);
        MY_LOGI("Fim envio para o sensor");
    }

    bool AsyncServer::_att_wifi_ssid(bool send_data)
    {
        MY_LOGD("inicio semaphore_take");
        if (xSemaphoreTake(_uart_mutex, portMAX_DELAY) != pdTRUE)
        {
            MY_LOGI("Nao foi possivel pegar o mutex!");
            return false;
        }
        MY_LOGD("semaphore_taken!");

        if (send_data)
        {
            char msg[] = "9031,;";
            MY_LOGD("Inicio serial.available()");
            // while (Serial.available()) Serial.read();
            MY_LOGD("inicio _send data to sensor");
            _send_data_to_sensor(msg);
            MY_LOGD("inicio ");
            uint16_t response = _wait_for_ok();
            MY_LOGD("end _wait_for_ok");
            if (response != HANDLE_HTTP_RESPONSE_CODE_OK)
            {
                xSemaphoreGive(_uart_mutex);
                MY_LOGI("Semaphore _uart_mutex given");
                return false;
            }
        }
        // TODO verificar possivel erro onde mensagemd e relatorio é equivocadamente lida por esse trecho do codigo
        char ent[1500];
        memset(ent, 0, 1500);
        MY_LOGD("inicio readBytesUntil");
        uint16_t char_read = Serial2.readBytesUntil(';', ent, ASYNC_SRV_MSG_LENGTH);
        // strncat(ent, ";", 1);
        MY_LOGD("char_read: %u", char_read);
        MY_LOGD(" RESP = %s", ent);
        if (char_read > 4)
            _update_ssid(ent);
        xSemaphoreGive(_uart_mutex);
        return true;
    }

    void AsyncServer::_update_ssid(char *msg)
    {
        MY_LOGI("msg completa: %s", msg);
        WiFi *wifi = WiFi::getInstance();
        bool ssid_change = true;
        bool status_change = true;
        uint8_t status;
        char *next_char;

        char *status_str = strtok_r(msg, ",", &next_char);
        char *ssid = strtok_r(NULL, ",", &next_char);
        char *password = strtok_r(NULL, ",", &next_char);

        if (status_str == NULL)
        {
            MY_LOGW("status_str NULL");
            return;
        }
        if (ssid == NULL)
        {
            MY_LOGW("ssid NULL");
            return;
        }
        if (password == NULL)
        {
            MY_LOGW("password NULL");
            return;
        }
        if (strtok_r(NULL, ",", &next_char) != NULL)
        {
            MY_LOGE("Mais argumentos que o esperado");
            while (strtok_r(NULL, ",", &next_char) != NULL)
                ;
            return;
        }

        status = atoi(status_str);
        MY_LOGD("Valores novos  : %d %s %s", status, ssid, password);
        MY_LOGD("Valores antigos: %d %s %s", _last_status, _last_ssid,
                _last_password);
        if (_last_status == status)
        {
            MY_LOGI("Status nao mudou");
            status_change = false;
        }

        if (strcmp(ssid, _last_ssid) == 0)
        {
            MY_LOGI("SSID nao mudou");
            ssid_change = false;
        }

        if (strcmp(password, _last_password) == 0)
        {
            MY_LOGI("Password nao mudou");
            ssid_change = false;
        }

        if (!(ssid_change || status_change))
        {
            MY_LOGI("SSID e Password não mudaram");
            return;
        }

        if (status_change && !ssid_change)
        {
            MY_LOGI("status changed and ssid not changed");
            _last_status = (Wifi_Status)status;
            switch (status)
            {
            case Wifi_Status::WIFI_NOT_CONNECTED:
                MY_LOGI("mesh nao conectado");
                if (strcmp(ssid, "123456") == 0)
                {
                    MY_LOGI("SSID ja eh o inicial");
                    return;
                }
                MY_LOGI("Mudando para Rede Inicial");
                wifi->restart(WIFI_DEFAULT_MODE);
                break;
            case Wifi_Status::WIFI_CONNECTED:
                MY_LOGI("mesh ja conectado");
                if (strcmp(ssid, "123456") == 0)
                {
                    MY_LOGI("SSID ja eh o inicial");
                    return;
                }
                MY_LOGI("Mudando para Rede Configuracao");
                wifi->restart(WIFI_DEFAULT_MODE);
            }
            return;
        }

        if (!status_change && ssid_change)
        {
            MY_LOGI("status not changed and ssidchanged");
            if (status == Wifi_Status::WIFI_NOT_CONNECTED)
            {
                MY_LOGI("Mudando para Rede Inicial");
                wifi->restart(WIFI_DEFAULT_MODE);
                strcpy(_last_ssid, ssid);
                MY_LOGI("mesh not connected! return");
                return;
            }
            if (strcmp(ssid, "123456") == 0)
            {
                MY_LOGI("Mudando para Rede Inicial");
                wifi->restart(WIFI_DEFAULT_MODE);
                strcpy(_last_ssid, ssid);
            }
            else if (strcmp(ssid, "wetzel") == 0)
            {
                MY_LOGI("Mudando para Rede Configuracao");
                wifi->restart(WIFI_DEFAULT_MODE);
                strcpy(_last_ssid, ssid);
            }
            else
            {
                MY_LOGI("Nenhum valor encontrado");
                wifi->restart(WIFI_DEFAULT_MODE);
                strcpy(_last_ssid, ssid);
            }
            return;
        }

        if (status == Wifi_Status::WIFI_NOT_CONNECTED)
        {
            MY_LOGI("mesh nao conectado");
            if (strcmp(ssid, "123456") == 0)
            {
                MY_LOGI("SSID ja eh o inicial");
                return;
            }
        }

        if (strcmp(ssid, "123456") == 0)
        {
            MY_LOGI("Mudando para Rede Inicial");
            wifi->restart(WIFI_DEFAULT_MODE);
            strcpy(_last_ssid, ssid);
        }
        else if (strcmp(ssid, "wetzel") == 0)
        {
            MY_LOGI("Mudando para Rede Configuracao");
            wifi->restart(WIFI_DEFAULT_MODE);
            strcpy(_last_ssid, ssid);
        }
        else
        {
            MY_LOGI("Nenhum valor encontrado");
            wifi->restart(WIFI_DEFAULT_MODE);
            strcpy(_last_ssid, ssid);
        }
    }

    void AsyncServer::check_mesh_connection_task(void *arg)
    {
        while (1)
        {
            vTaskDelay(ASYNC_SRV_CHECK_CONNECTION_TASK_DELAY_MS /
                       portTICK_PERIOD_MS);
            if (_http_request || _waiting_for_ans)
                continue;
            _att_wifi_ssid(true);
        }
    }

    void AsyncServer::read_report_in_serial_task(void *arg)
    {
        char serial_msg[ASYNC_SRV_MSG_LENGTH + 1];
        ReportHandler *report_handler = ReportHandler::getInstance();

        while (1)
        {
            // MY_LOGD("inicio semaphore_uart_take");
            if (xSemaphoreTake(_uart_mutex, portMAX_DELAY) != pdTRUE)
            {
                MY_LOGW("Nao pude pegar _uart_mutex");
                vTaskDelay(50 / portTICK_PERIOD_MS);
                continue;
            }
            // MY_LOGD("semaphore_uart_taken!");

            if (Serial2.available())
            {

                memset(serial_msg, 0, ASYNC_SRV_MSG_LENGTH);
                uint8_t value;
                MY_LOGI("Tem dado a ser lido UART2");
                value =
                    Serial2.readBytesUntil(';', serial_msg, ASYNC_SRV_MSG_LENGTH);
                MY_LOGI("Dado lido: %s", serial_msg);

                xSemaphoreGive(_uart_mutex);
                // MY_LOGD("semaphore_uart given...");

                if (serial_msg[0] != '#')
                {
                    MY_LOGE("Mensagem não indentificada recebida: %s\nAbortando...", serial_msg);
                    continue;
                }
                char *strtok_ptr;
                report_msg_entry_t new_entry;
                strcpy(new_entry.msg, serial_msg + 4);
                MY_LOGD("report_msg_entry-> %s", new_entry.msg);
                // +4 no endereço para remover código inicial da mensagem de report
                report_handler->add_entry_to_report_msg_buffer(new_entry);
            }
            else
            {
                xSemaphoreGive(_uart_mutex);
            }
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }
    }

    esp_err_t AsyncServer::echo_post_handler(httpd_req_t *req)
    {
        char buf[100];
        int ret;
        size_t remaining = req->content_len;

        while (remaining > 0)
        {
            /* Read the data for the request */
            if ((ret = httpd_req_recv(req, buf,
                                      std::min(remaining, sizeof(buf)))) <= 0)
            {
                if (ret == HTTPD_SOCK_ERR_TIMEOUT)
                {
                    /* Retry receiving if timeout occurred */
                    continue;
                }
                return ESP_FAIL;
            }

            /* Send back the same data */
            httpd_resp_send_chunk(req, buf, ret);
            remaining -= ret;

            /* Log data received */
            ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
            ESP_LOGI(TAG, "%.*s", ret, buf);
            ESP_LOGI(TAG, "====================================");
        }

        // End response
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }

    esp_err_t AsyncServer::out_get_handler(httpd_req_t *req)
    {
        MY_LOGI("Recebido requisicao HTTP GET /out: %s", req->uri);
        char buf[1500];
        char param[1000];
        size_t buf_len;
        BaseType_t xReturned;
        // String     send_buffer((char *)0);
        uint16_t response;
        char status[6];
        char wait_for_ok_response[10] = {0};

        http_requests_received++;

        memset(buf, 0, 1500);

        if (_waiting_for_ans)
        {
            MY_LOGI("/out ignorado, pois ja esta aguardando uma resposta");
            httpd_resp_set_type(req, "text/plain; charset=utf-16");
            httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        _waiting_for_ans = true;
        MY_LOGD("_waiting_for_ans = true");

        buf_len = httpd_req_get_url_query_len(req) + 1;
        if (buf_len > 1)
        {
            if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query => %s", buf);

                /* Get value of expected key from query string */
                if (httpd_query_key_value(buf, "text", param, sizeof(param)) ==
                    ESP_OK)
                {
                    ESP_LOGI(TAG, "Found URL query parameter => text=%s", param);
                }
            }
        }
        convert_html_text_to_ascii(param);

        MY_LOGI("Send buffer: %s", param);
        uart_flush_input(2);
        xQueueReset(_queue_uart_msg);
        _uart_response_is_complete = false;
        _uart_response_is_timeout = false;

        if (xSemaphoreTake(_uart_mutex, 1000 / portTICK_RATE_MS) != pdTRUE)
        {
            MY_LOGI("Nao foi possivel pegar o mutex!");
            _waiting_for_ans = false;
            return false;
        }
        _send_data_to_sensor(param);

        response = _wait_for_ok(wait_for_ok_response);
        const char *answer =
            response == HANDLE_HTTP_RESPONSE_CODE_OK        ? "OK"
            : response == HANDLE_HTTP_RESPONSE_CODE_NOT_OK  ? "Not fine"
            : response == HANDLE_HTTP_RESPONSE_CODE_TIEMOUT ? "Timeout"
                                                            : "Unexpected error";

        sprintf(status, "%d", response);
        if (response != HANDLE_HTTP_RESPONSE_CODE_OK)
        {
            MY_LOGD("_waiting_for_ans = false");
            _waiting_for_ans = false;
        }
        if (response == HANDLE_HTTP_RESPONSE_CODE_OK)
        {
            xReturned = xTaskCreate(_uart_receive_response_task,
                                    "_uart_receive_response_task",
                                    ASYNC_SRV_CHECK_MESH_TASK_STACK_SIZE, NULL,
                                    CONFIG_APP_TASK_DEFAULT_PRIORITY - 1, NULL);

            // TODO: EU TIREI DAQUI
            // xSemaphoreGive(_uart_mutex);
            if (xReturned != pdPASS)
            {
                MY_LOGI("Nao foi possivel criar a _webserver_task");
                abort();
            }
        }
        httpd_resp_set_status(req, status);
        httpd_resp_set_type(req, "text/plain; charset=utf-16");
        if (response != HANDLE_HTTP_RESPONSE_CODE_OK)
        {
            httpd_resp_send(req, answer, HTTPD_RESP_USE_STRLEN);
        }
        else
        {
            httpd_resp_send(req, wait_for_ok_response, HTTPD_RESP_USE_STRLEN);
        }

        /* After sending the HTTP response the old HTTP request
         * headers are lost. Check if HTTP request headers can be read now. */
        if (httpd_req_get_hdr_value_len(req, "Host") == 0)
        {
            ESP_LOGI(TAG, "Request headers lost");
        }
        // TODO: Antes de finalizar sempre devolve o recurso (COLOQUEI AQUI)
        xSemaphoreGive(_uart_mutex);
        return ESP_OK;
    }

    esp_err_t AsyncServer::ans_get_handler(httpd_req_t *req)
    {
        MY_LOGI("Recebido requisicao HTTP GET /ans: %s", req->uri);

        http_requests_received++;

        if (!_waiting_for_ans)
        {
            MY_LOGI("/ans ignorado, pois nao veio um /out antes");
            return ESP_OK;
        }
        _device_response_from_uart_task(req);
        _http_request = false;
        MY_LOGD("_waiting_for_ans = false");
        _waiting_for_ans = false;

        return ESP_OK;
    }

    /**
     * @brief Responde o aplitivo com uma informação do módulo de relatório
     * @note Rota do módulo de relatório retornar informações
     * @param req Pacote com as informações do solicitante
     * @return esp_err_t
     */
    esp_err_t AsyncServer::report_msg_handler(httpd_req_t *req)
    {
        bool isOK = 0;

        std::string msg_return = "";

        MY_LOGI("entrou em report asncy");

        /// Com os parametros da URL, vou passar para o buffer
        /// Mas antes eu preciso saber qual o tamanho para alocar recurso
        /// se precisar
        size_t buffer_size = httpd_req_get_url_query_len(req);

        MY_LOGI("buffer size = %d", (int)buffer_size);
        char *buffer_url = (char *)malloc(sizeof(char) * buffer_size);

        // Verifica se a mensagem é valida
        if (buffer_size > 0 && buffer_url)
        {
            MY_LOGI("Entrou para tratar o report resposta");

            /// Aqui vou obter a url com os parametros
            esp_err_t err = httpd_req_get_url_query_str(req, buffer_url, buffer_size);
            MY_LOGI("parametros recebidos = %s", buffer_url);

            // Trata a mensagem
            API_ModuloRelatorio *m_relatorio = API_ModuloRelatorio::GetObjs();
            isOK = m_relatorio->DeliveryDataByDataRequest(buffer_url, buffer_size, msg_return);

            if (isOK)
            {
                // Monta a mensagem para retornar para o servidor
                httpd_resp_send(req, msg_return.c_str(), msg_return.size());
            }
            else
            {
                httpd_resp_send_408(req);
            }
        }
        else
        {
            MY_LOGI("desconhecido saida 404");
            // Se deu qualquer coisa errada na obteção da mensagem
            // retorna um erro
            httpd_resp_send_404(req);
        }
        /// Libera os recursos alocados
        free(buffer_url);
        return ESP_OK;
    }

    esp_err_t AsyncServer::direct_msg_handler(httpd_req_t *req)
    {
        MY_LOGI("Recebido requisicao HTTP GET /direct: %s", req->uri);
        char buffer[1500];
        size_t buffer_max_size = sizeof(buffer);
        char param[1000];
        char status[6];
        size_t buf_len;
        bool responses_with_content = false;
        bool multiple_responses = false;
        char *next_char;

        http_requests_received++;

        memset(buffer, 0, buffer_max_size);

        buf_len = httpd_req_get_url_query_len(req) + 1;

        if (buf_len > 1)
        {
            if (httpd_req_get_url_query_str(req, buffer, buf_len) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query => %s", buffer);

                /* Get value of expected key from query string */
                if (httpd_query_key_value(buffer, "text", param, sizeof(param)) ==
                    ESP_OK)
                {
                    ESP_LOGI(TAG, "Found URL query parameter => text=%s", param);
                }
            }
        }
        convert_html_text_to_ascii(param);

        MY_LOGD("Decoded parameter => %s", param);
        char *msg_code = strtok_r(param, ",", &next_char);
        if (msg_code == NULL)
        {
            MY_LOGE("Código inválido de mensagem");
            return ESP_FAIL;
        }

        char *msg_content;

        uint8_t msg_code_uint8 = strtol(msg_code, &msg_content, 10);
        // Para pular '/0' gerado por strtok_r
        msg_content++;

        MY_LOGD("Msg code test %u | msg test %s", msg_code_uint8, msg_content);

        esp_err_t err = ESP_FAIL;
        // memset(buf, 0, 1500);

        // char* msg_response_content = buf;
        switch (msg_code_uint8)
        {
        case WIFI_MODE_CHANGE_CODE:
            err = msg_handler_wifi_mode_change(msg_content);
            // strcpy(msg_response_content, ";\0");
            break;
        case WIFI_AP_CONFIG_CHANGE_CODE:
            err = msg_handler_wifi_ap_config_change(msg_content);
            // strcpy(msg_response_content, ";\0");
            break;
        case WIFI_STA_CONFIG_CHANGE_CODE:
            err = msg_handler_wifi_sta_config_change(msg_content);
            // strcpy(msg_response_content, ";\0");
            break;
        case WIFI_GET_SSID_LIST_CODE:
            err = msg_handler_get_ssid_list(msg_content);
            break;
        case WIFI_AP_VALIDATE_CREDENTIALS_CODE:
            err = msg_handler_validate_ap_credentials(msg_content);
            break;
        case INTERFACE_INFO_REQUEST_CODE:
            memset(buffer, 0, buffer_max_size);
            err = msg_handler_interface_info_requested(buffer);
            responses_with_content = true;
            break;
        case RTC_UPDATE_CODE:
            err = msg_handler_rtc_update(msg_content);
            break;
        case REPORT_CONFIG_CODE:
            // err = msg_handler_report_config(msg_content); // TODO: Comentei aqui
            break;
        default:
            MY_LOGE("Código de mensagem inválido");
            return ESP_FAIL;
        }

        httpd_resp_set_type(req, "text/plain; charset=utf-16");

        char handler_result_msg[1600];
        size_t size_of_handler_result_msg = sizeof(handler_result_msg);

        if (err == ESP_OK)
        {
            snprintf(status, 6, "%d", HANDLE_HTTP_RESPONSE_CODE_OK);
            httpd_resp_set_status(req, status);

            strlcpy(handler_result_msg,
                    multiple_responses ? DIRECT_MSG_MULTI_RESPONSE_OK
                                       : DIRECT_MSG_FINAL_RESPONSE_OK,
                    size_of_handler_result_msg);
            if (responses_with_content)
            {
                strlcat(handler_result_msg, buffer, size_of_handler_result_msg);
            }
            strlcat(handler_result_msg, END_OF_MESSAGE_IDENTIFIER,
                    size_of_handler_result_msg);
        }
        else
        {
            sprintf(status, "%d", HANDLE_HTTP_RESPONSE_CODE_NOT_OK);
            httpd_resp_set_status(req, status);
            strlcpy(handler_result_msg, DIRECT_MSG_FINAL_RESPONSE_NOK,
                    size_of_handler_result_msg);
            strlcat(handler_result_msg, END_OF_MESSAGE_IDENTIFIER,
                    size_of_handler_result_msg);
        }

        httpd_resp_send(req, handler_result_msg, HTTPD_RESP_USE_STRLEN);

        return err;
    }

    esp_err_t clear_file(const char *filename, bool is_binary = true)
    {
        MY_LOGD("Clearing file");
        const char *open_mode = is_binary ? "wb" : "w";
        FILE *f = fopen(filename, open_mode);
        if (f == NULL)
        {
            MY_LOGE("Failed to open file for writing");
            return ESP_FAIL;
        }
        fclose(f);
        return ESP_OK;
    }

    esp_err_t AsyncServer::upgrade_post_handler(httpd_req_t *req)
    {
        // WIP
        return ESP_OK;
    }

    void AsyncServer::_device_response_string(char *response)
    {
        // AsyncWebServerRequest *request = (AsyncWebServerRequest*) param;
        char ent[1500];
        bool request_is_complete = false;
        uint32_t request_timer,
            REQUEST_TIMEOUT = ASYNC_SRV_D_RESPONSE_REQUEST_TIMEOUT_MS;
        uint32_t client_wait_timer,
            CLIENT_WAIT_TIMEOUT = ASYNC_SRV_CLIENT_RESPONSE_REQUEST_TIMEOUT_MS;
        bool request_timeout = false;
        request_timer = millis();
        // recebe as mensagens via UART e repassa para o aplicativo ate receber
        // a mensagem final. Se nao receber a mensagem a tempo retorna uma mensagem de erro
        while (!request_is_complete || !request_timeout)
        {

            if (Serial2.available())
            {
                memset(ent, 0, 1499);
                Serial2.readBytesUntil(';', ent, ASYNC_SRV_MSG_LENGTH);
                MY_LOGD("Dado lido2: %s", ent);
                // se for uma das mensagens de fim, finaliza a requisicao

                // if(strstr(ent, "009") != NULL || strstr(ent, "007") != NULL)
                if (strncmp(ent, "009", 3) == 0 || strncmp(ent, "007", 3) == 0)
                {
                    MY_LOGI("REQUEST IS COMPLETE");
                    request_is_complete = true;
                }
                if (!request_is_complete && strncmp(ent, "008", 3) != 0)
                {
                    MY_LOGE("A mensagem veio com erro!");
                    continue;
                }
                strcat(ent, ";");
                strncpy(_msg, ent, ASYNC_SRV_MSG_LENGTH);
                MY_LOGI("Tudo certo, enviando a mensagem %s", _msg);
                strcat(response, ent);
            }
            // se o tempo de espera da requisicao passar de TIMEOUT
            if (millis() - request_timer >
                ASYNC_SRV_D_RESPONSE_REQUEST_TIMEOUT_MS)
            {
                MY_LOGI("REQUEST TIMEOUT");
                request_timeout = true;
                break;
            }
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }

    void AsyncServer::_device_response_from_uart_task(httpd_req_t *req)
    {
        uint16_t queue_lenght = 65535;
        char uart_queue_msg[UART_QUEUE_MSG_LENGTH];
        MY_LOGD("Iniciando _device_response_from_uart_task");
        // recebe as mensagens via UART e repassa para o aplicativo ate receber
        // a mensagem final. Se nao receber a mensagem a tempo retorna uma mensagem de erro

        while ((!_uart_response_is_complete && !_uart_response_is_timeout) ||
               uxQueueMessagesWaiting(_queue_uart_msg) > 0)
        {
            queue_lenght = uxQueueMessagesWaiting(_queue_uart_msg);
            while (queue_lenght > 0)
            {
                if (xQueueReceive(_queue_uart_msg, uart_queue_msg, 10) == pdTRUE)
                {
                    MY_LOGD("Adicionando na response: %s", uart_queue_msg);
                    if (req != nullptr)
                    {
                        MY_LOGI("Req is not null, sending chunk");
                        httpd_resp_send_chunk(req, uart_queue_msg,
                                              strlen(uart_queue_msg));
                    }
                }
                queue_lenght = uxQueueMessagesWaiting(_queue_uart_msg);
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        MY_LOGD("Ending _device_response_from_uart_task");
        if (req != nullptr)
        {
            MY_LOGI("Req is not null, sending final chunk");
            httpd_resp_send_chunk(req, NULL, 0);
        }
    }

    httpd_handle_t AsyncServer::start_webserver(void)
    {
        httpd_handle_t server = NULL;
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.lru_purge_enable = true;
        config.max_open_sockets = 2;
        config.stack_size = 1024 * 24;
        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
        if (httpd_start(&server, &config) == ESP_OK)
        {
            ESP_LOGI(TAG, "Registering URI handlers");
            httpd_register_uri_handler(server, &echo);
            httpd_register_uri_handler(server, &ans);
            httpd_register_uri_handler(server, &out);
            httpd_register_uri_handler(server, &direct);
            // Inserir o report do relatorio
            httpd_register_uri_handler(server, &report_dados);

            // httpd_register_uri_handler(server, &upgrade);
            return server;
        }

        ESP_LOGI(TAG, "Error starting server!");
        abort();
        return NULL;
    }

    void AsyncServer::stop_webserver(httpd_handle_t server)
    {
        httpd_stop(server);
    }

    void AsyncServer::disconnect_handler(void *arg, esp_event_base_t event_base,
                                         int32_t event_id, void *event_data)
    {

        digitalWrite(LED_PIN, HIGH);
        httpd_handle_t *server = (httpd_handle_t *)arg;
        if (*server)
        {
            ESP_LOGI(TAG, "Stopping webserver");
            stop_webserver(*server);
            *server = NULL;
        }
    }

    void AsyncServer::connect_handler(void *arg, esp_event_base_t event_base,
                                      int32_t event_id, void *event_data)
    {
        digitalWrite(LED_PIN, HIGH);
        httpd_handle_t *server = (httpd_handle_t *)arg;
        if (*server == NULL)
        {
            ESP_LOGI(TAG, "Starting webserver");
            *server = start_webserver();
        }
    }

    void AsyncServer::wifi_any_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
    {
        _print_wifi_event(event_id);
        switch (event_id)
        {
        case WIFI_EVENT_AP_STADISCONNECTED:
            disconnect_handler(arg, event_base, event_id, event_data);
        }
    }

    void AsyncServer::ip_any_handler(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data)
    {
        _print_ip_event(event_id);
        switch (event_id)
        {
        case IP_EVENT_AP_STAIPASSIGNED:
            connect_handler(arg, event_base, event_id, event_data);
        }
    }

    void AsyncServer::_uart_receive_response_task(void *arg)
    {
        MY_LOGI("Iniciando _uart_receive_response_task")
        char ent[1500];
        bool request_is_complete = false;
        uint32_t request_timer;
        bool request_timeout = false;
        request_timer = millis();

        if (xSemaphoreTake(_uart_mutex, portMAX_DELAY) != pdTRUE)
        {
            return;
        }

        // recebe as mensagens via UART e repassa para o aplicativo ate receber
        // a mensagem final. Se nao receber a mensagem a tempo retorna uma mensagem de erro
        while (!_uart_response_is_complete && !_uart_response_is_timeout)
        {
            if (Serial2.available())
            {
                uint8_t value;
                memset(ent, 0, 1499);
                value = Serial2.readBytesUntil(';', ent, ASYNC_SRV_MSG_LENGTH);

                if (ent[0] ==
                    '#')
                { // ignora mensagem de relatorio, sinalizada pela inicio '#'
                    MY_LOGD("Mensagem de report ignorada: %s", ent);
                    report_msg_entry_t new_entry;
                    strcpy(new_entry.msg, ent + 4);
                    ReportHandler::add_entry_to_report_msg_buffer(new_entry);
                    continue;
                }

                MY_LOGD("Dado lido2: %s", ent);
                // se for uma das mensagens de fim, finaliza a requisicao
                if (strncmp(ent, "009", 3) == 0 || strncmp(ent, "007", 3) == 0)
                {
                    MY_LOGI("REQUEST IS COMPLETE");
                    request_is_complete = true;
                }
                if (!request_is_complete && strncmp(ent, "008", 3) != 0)
                {
                    MY_LOGE("A mensagem veio com erro!");
                    continue;
                }
                strcat(ent, ";");
                strncpy(_msg, ent, ASYNC_SRV_MSG_LENGTH);
                MY_LOGD("Tudo certo, adicionando na queue a mensagem %s", _msg);
                uint8_t ret =
                    xQueueSend(_queue_uart_msg, ent, DEVICE_XQUEUE_SEND_WAIT_MS);
                if (ret != pdPASS)
                {
                    MY_LOGE("Error: queue _to_send is full!");
                }
                if (request_is_complete)
                    _uart_response_is_complete = true;
            }
            // se o tempo de espera da requisicao passar de TIMEOUT
            if (millis() - request_timer >
                ASYNC_SRV_D_RESPONSE_REQUEST_TIMEOUT_MS)
            {
                MY_LOGI("REQUEST TIMEOUT");
                _uart_response_is_timeout = true;
                break;
            }
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        xSemaphoreGive(_uart_mutex);
        MY_LOGD("Finalizando a tarefa _uart_receive_response_task");
        vTaskDelete(NULL);
    }
} // namespace Wetzel