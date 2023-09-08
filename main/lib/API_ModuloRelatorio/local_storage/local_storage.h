#ifndef local_storage_H_
#define local_storage_H_

#include <cstring>
#include <string>
#include <stdint.h>
#include <esp_system.h>
#include <esp_err.h>
#include "debug.h"

namespace Wetzel {
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
    static local_storage * GetObjs();

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
     * @brief Retorna uma string contendo o consumo por hora de um dia especifico
     * @note "255,255, ..." -> 24 pontos
     * @param month Mês
     * @param day dia
     * @param watts_luminaria potencia da luminaria acoplanada no modulo controlador
     * @param qnt_luminarias quantidade de luminarias conectadas ao modulo controlador
     * @return std::string com a string pronta
    */
    std::string ReturnDayPointByDayAndMonthIndex(month_type_t month, uint8_t day, uint16_t watts_luminaria, uint8_t qnt_luminarias);

    /**
     * @brief Retorna uma string com o consumo do mes
     * @note "255,255, ..." -> 31 pontos 
     * @param month Mês
     * @param watts_luminaria potencia da luminaria acoplanada no modulo controlador
     * @param qnt_luminarias quantidade de luminarias conectadas ao modulo controlador
     * @return std::string com a string pronta 
    */
    std::string ReturnMonthPointByMonthIndex(month_type_t month, uint16_t watts_luminaria, uint8_t qnt_luminarias);

    /**
     * @brief Retorna uma string com o consumo de todos os meses do histório
     * @note "255,255, ..." -> 12 pontos 
     * @param watts_luminaria potencia da luminaria acoplanada no modulo controlador
     * @param qnt_luminarias quantidade de luminarias conectadas ao modulo controlador
     * @return std::string com a string pronta 
    */
    std::string ReturnAllMonthPoints(uint16_t watts_luminaria, uint8_t qnt_luminarias);

    /**
     * @brief Salva o buffer temporario na estrutra de dados
     * @return 1 - Sucesso, 0 - Falhou
    */
    bool UpdateValueonStruct();

private:
    /**
     * @brief Retorna a posição do mês no vetor
    */
    uint8_t ReturnVectorIndexByMonth(month_type_t month);

    /**
     * @brief Construtor privado
    */
    local_storage(/* args */);

    /**
     * @brief Carrega as informações da NVS se não estiver corrupta
     * @return esp_err_t
    */
    esp_err_t LoadDataFromNVS();

    /**
     * @brief Pega o dia que finalizou e armazena na NVS
    */
    void RecordOnNVStheFinishDay();

    /**
     * @brief Pega o valor do buffer temporario e armazena na estrutura
     * @note TASK TIMER do FreeRTOS
    */
    static void UpdateStructWhich10min(void * args);

    /**
     * @brief Retorna o consumo total do mês
     * @param month Mês de referência
    */
    uint32_t ReturnTotalInWattsSpendOnMonthByIndex(month_type_t month, uint16_t watts_luminaria, uint8_t qnt_luminarias);

    /* Organização dos dados que serão armazenados */
    typedef struct {
        uint16_t _hora[24];
    } dia_type_t;

    typedef struct {
        dia_type_t _dia[31];
        month_type_t _mes;
    } mes_type_t;

    mes_type_t _mes_storage[8];
    uint8_t _buffer_storage;
    static local_storage * _ptr_local_storage;
};
} // namespace Wetzel
#endif