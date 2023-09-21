#ifndef API_ModuloRelatorio_H_
#define API_ModuloRelatorio_H_

#include "local_storage.h"
#include "Memory.h"

#include <stdint.h>
#include <string>

namespace Wetzel
{
    class API_ModuloRelatorio
    {
    public:
        typedef enum
        {
            LUM_17K,
            LUM_23K,
            LUM_32K,
            LUM_INDEF = 255,
        } luminaria_type_t;

        typedef enum
        {
            kReturnHours = 0,
            kReturnDay,
            kReturnMonth,
            kReturnQntLuminaria,
            kReturnTypeLuminaria,
            kReturnErro = 255
        } type_request_http_t;

    public:
        /**
         * @brief Destrutor
         */
        ~API_ModuloRelatorio();

        /**
         * @brief Inicializa a API_ModuloRelatorio
         * @return esp_err_t
         */
        esp_err_t Begin();

        /**
         * @brief Retorna o pedido feito por parametro search_data em return_data
         * @note APENAS DEVE SER CHAMADO NO ASYNC_SERVER
         * @note A Informação deve serguir uma das seguinte regras:
         * @note parametros recebidos = day=01&month=05&year=2023
         * @param search_data Parametros da url que serão usados para buscar informação
         * @param search_size Tamanaho da search_data
         * @param return_data Resposta do pedido da search_data
         * @return 1 - Sucesso , 0 - Falhou (Deve descartar o resultado)
         */
        bool DeliveryDataByDataRequest(const char *search_data, size_t serach_size, std::string &return_data);

        /**
         * @brief Defini o nome número de luminarias na rede
         * @param id Identificação do controlador
         * @param qtd_luminaria nova quantidade de luminarias
         * @return 1 - Sucesso, 0 - Falhou
         */
        bool SetNumberLuminarias(uint8_t id, uint8_t qtd_luminaria);

        /**
         * @brief Defini o tipo de luminaria presente na rede
         * @param id Identificação do controlador
         * @param type_luminaria tipo da luminaria especificada
         * @return 1 - Sucesso, 0 - Falhou
         */
        bool SetTypeLuminarias(uint8_t id, luminaria_type_t type_luminaria);

        /**
         * @brief Retorna o objeto da classe
         * @return Ponteiro da classe API_ModuloRelatorio
         */
        static API_ModuloRelatorio *GetObjs();

        /**
         * @note Singletons derivado do design pattern
         */
        API_ModuloRelatorio(API_ModuloRelatorio &other) = delete;
        void operator=(const API_ModuloRelatorio &) = delete;

    private:
        /**
         * @brief Construor privado
         */
        API_ModuloRelatorio();

        /**
         * @brief Obtem os dados armazenados na memoria
         * @note As informações são populadas de acordo com o APP
         * @return 1 - Sucesso, 0 - Falhou
         */
        bool LoadDataFromNVS();

        /**
         * @brief Grava as informações de _devices_info na NVS
         * @return 1 - Sucesso, 0 - Falhou
         */
        bool RecordDataOnNVS();

        /**
         * @brief Atualiza a variavel de total de luminarias declaradas na rede
         * @note O valor declarado vem dos registros do aplicativo
         * @return O total de luminarias declaradas na rede
         */
        uint16_t UpdateTotalLumiOnMesh();

        /**
         * @brief Retorna o tipo de requisição que está na mensagem
         * @param search_data Mensagem para classificar
         * @param day
         * @param month
         * @param year
         * @note Caso a requisição não for compativél com os parametros, seta zero
         * @return type_request_device_t
         */
        type_request_http_t ClassifyMessageReturn(const char *search_data, uint8_t &day, uint8_t &month, uint8_t &year);

        /* Estrutra que armazena as informações na rede */
        typedef struct
        {
            uint8_t mac[6];
            uint8_t qtd_luminarias;
            luminaria_type_t modelo_luminarias;
        } device_report_info_t;

        typedef struct
        {
            device_report_info_t *_ptr_devices_info;
            uint8_t _qnt_devices = 0;
        } device_ptr_t;

        /* Variaveis */
        device_ptr_t _devices_info;
        uint16_t total_lumina_on_mesh;
        luminaria_type_t last_tipo_luminaria;

        static const uint16_t _watts_luminaria[];
        static API_ModuloRelatorio *_ptr_obj;
    };

}

#endif