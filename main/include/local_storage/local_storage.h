#ifndef local_storage_H_
#define local_storage_H_

#include <cstring>
#include <stdint.h>
#include <map>
#include <list>
#include <esp_system.h>
#include <esp_err.h>
#include "debug.h"

class local_storage
{
public:
    typedef enum {
        kJAN = 0,
        KFEV,
        kMAR,
        kABR,
        kMAY,
        kJUN,
        kJUL,
        kAGO,
        kSEP,
        kOCT,
        kNOV,
        kDEC,
    } month_type_t;

public:

    /**
     * @brief Destrutor
    */
    ~local_storage();

    /**
     * @brief Inicializa o variaveis e funções auxiliares bem como carrega dados da NVS
     * @return esp_err_t
    */
    esp_err_t Begin();

    /**
     * @brief Retorna o objeto da classe
     * @return Ponteiro da classe local_storage
    */
    local_storage * GetObjs();

    /**
     * @brief Salva na estrutura o valor do KWh
     * @param value Valor em reais flutuante do kilowatt
     * @return 1 - Sucesso, 0 - Falhou
    */
    bool RecordKWHvalue(float value);

    /**
     * @brief Grava no buffer o valor pwm entregue pela rede
     * @param value PWM retornado pela rede
     * @return 1 - Sucesso, 0 - Falhou
    */
    bool RecordPWMonStorage(uint8_t value);

    /**
     * @brief Converte o mês em string para o tipo month_type_t
     * @param month Mês
     * @param size Tamanho da string
     * @return local_storage::month_type_t
    */
    static month_type_t ConvertMonthStringInEnum(char * month, size_t size);

    /**
     * @brief Retorna uma string com todas as informações
     * @note "255,255, ..." -> 24 pontos
     * @param month Mês
     * @param day dia
     * @return std::string com a string pronta
    */
    std::string ReturnDayPointByDayAndMonthIndex(month_type_t month, uint8_t day);

    /**
     * @brief Retorna uma string com a média do mês
     * @note "255,255, ..." -> 31 pontos 
     * @param month Mês
     * @return std::string com a string pronta 
    */
    std::string ReturnMonthPointByMonthIndex(month_type_t month);

    /**
     * @brief Defini a o mês e dia atual na classe
     * @param month Mês
     * @param day Dia
     * @return 1 - Sucesso, 0 - Falhou 
    */
    bool SetCurrentMonthAndDay(char * month, char *  day);

private:

    /**
     * @brief Construtor privado
    */
    local_storage(/* args */);

    /* Organização dos dados que serão armazenados */
    static std::map< month_type_t, std::list<uint8_t>> _pwm_month;
    static std::list<uint8_t> _pwm_dia;

    typedef struct {
        uint8_t _hora[24];
    } dia_type_t;

    typedef struct {
        dia_type_t _dia[31];
        uint8_t _mes;
    } mes_type_t;

    mes_type_t _mes_storage[8];
    uint8_t buffer_storage;

    /**
     * @brief Carrega as informações da NVS se não estiver corrupta
     * @return esp_err_t
    */
    esp_err_t LoadDataFromNVS();

    /**
     * @brief Pega o dia que finalizou e armazena na NVS
    */
    void NextDay();

    /**
     * @brief Pega o valor do buffer temporario e armazena na estrutura
     * @note TASK TIMER do FreeRTOS
    */
    static void UpdateStructWhich10min(void * args);

    /* storage the current location on struct */
    static uint8_t _current_day;
    static month_type_t _current_month;
    float _kwh_price = 0;
    static local_storage * _ptr_local_storage;
};

#endif