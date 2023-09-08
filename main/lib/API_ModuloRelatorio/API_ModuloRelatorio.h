#ifndef API_ModuloRelatorio_H_
#define API_ModuloRelatorio_H_

#include "local_storage.h"
#include "Memory.h"

#include <stdint.h>
#include <string>

namespace Wetzel {
class API_ModuloRelatorio
{
public:
    typedef enum { LUM_17K = 0, LUM_23K, LUM_32K } luminaria_type_t;
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
     * @note #define RETURN_PONTOS_EM_HORA   "GET|DD:XX_MM_YYYY"
     * @note #define RETURN_PONTOS_EM_DIAS   "GET|MM:00_XX_YYYY"
     * @note #define RETURN_PONTOS_EM_MES    "GET|YY:_00_00_0000"
     * @note #define SET_QTD_LUMINARIA       "SET|qtd_luminaria:XX|id:XXX"
     * @note #define SET_TYPE_LUMINARIA      "SET|type_luminaria:XX|id:XXX"
     * @param search_data Parametros da url que serão usados para buscar informação
     * @param search_size Tamanaho da search_data
     * @param return_data Resposta do pedido da search_data
     * @return 1 - Sucesso , 0 - Falhou (Deve descartar o resultado)
    */
    bool DeliveryDataByDataRequest(char * search_data, size_t serach_size, std::string & return_data);

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
    static API_ModuloRelatorio * GetObjs();

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
     * @brief Retorna o index do vetor _valid_return_api de acordo com o tipo da msg
     * @param search_data Mensagem para classificar
     * @return Index do Vetor _valid_return_api
    */
    uint8_t ClassifyMessageReturnIndexVectorAPI(const char* search_data);

    /* Estrutra que armazena as informações na rede */
    typedef struct {
        uint8_t mac[6];
        uint8_t qtd_luminarias;
        luminaria_type_t modelo_luminarias;
    } device_report_info_t;

    typedef struct {
        device_report_info_t * _ptr_devices_info;
        uint8_t _qnt_devices = 0;
    } device_ptr_t;

    /* Variaveis */
    device_ptr_t _devices_info;
    uint16_t total_lumina_on_mesh;
    luminaria_type_t last_tipo_luminaria;

    /* Defines do filtro */
    #define RETURN_PONTOS_EM_HORA   "GET|DD:XX_MM_YYYY"
    #define RETURN_PONTOS_EM_DIAS   "GET|MM:00_XX_YYYY"
    #define RETURN_PONTOS_EM_MES    "GET|YY:_00_00_0000"
    #define SET_QTD_LUMINARIA       "SET|qtd_luminaria:XX|id:XXX"
    #define SET_TYPE_LUMINARIA      "SET|type_luminaria:XX|id:XXX"

    static const char * _valid_return_api[];
    static const uint16_t _watts_luminaria[];
    static API_ModuloRelatorio * _ptr_obj;
};

}

#endif