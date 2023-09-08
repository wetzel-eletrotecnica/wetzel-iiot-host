#include "local_storage.h"
#include "real_time_clock.h"
#include "Memory.h"
#include <memory>
#include <freertos/timers.h>

namespace Wetzel {
/**
 * ====================================================
 *                     Debug msg
 * ====================================================
*/
static const char* TAG = __FILE__;
#define MEMORY_POTENCIA_STORAGE  "rela_pote"
#define MEMORY_LUMINARI_STORAGE  "rela_lumi"
/**
 * ====================================================
 *                    Static Const
 * ====================================================
*/
local_storage * local_storage::_ptr_local_storage = nullptr;
/**
 * ====================================================
 *                   Public Function 
 * ====================================================
*/
local_storage::local_storage(/* args */) {}

local_storage::~local_storage(){}

esp_err_t local_storage::Begin()
{
    // Verifica na memoria se exite informações armazenadas
    if (LoadDataFromNVS() != ESP_OK)
    {
        // Limpa a estrutura
        std::memset(&_mes_storage, 0, sizeof(_mes_storage));
    }
    // Inicia o timer da rotina que pega do buffer e popula a estrutra
    TimerHandle_t timer = xTimerCreate("UpdateStructWhich10min", 10000, pdTRUE, 0, UpdateStructWhich10min);
    if (timer == nullptr)
    {
        MY_LOGE("Em begin local_storage, erro ao criar o UpdateStructWhich10min");
    }
    xTimerStart(timer, 0);
 
    return ESP_OK;
}

local_storage * local_storage::GetObjs()
{
    if (_ptr_local_storage == nullptr)
    {
        _ptr_local_storage = new local_storage;
        if (_ptr_local_storage == nullptr)
        {
            MY_LOGE("Erro ao criar o local_storage objeto");
        }
    }
    return _ptr_local_storage;
}

bool local_storage::RecordPWMonStorage(uint8_t value)
{
    // Faz uma média para ir acumulando a mudança
    _buffer_storage = (_buffer_storage + value) / 2;
    return true;
}
 
local_storage::month_type_t local_storage::ConvertMonthStringInEnum(char * month, size_t size)
{
    if (month == nullptr) {return kJAN;}
    const char string_mes[12][4] = {"Jan", "Fev", "Mar", "Apr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dec"};

    uint8_t i;
    for (i = 0; i < 12; i++)
    {
        if (std::strcmp(string_mes[i], month) == 0)
        {
            return (month_type_t)i;
        }
    }
    return kJAN;
}

std::string local_storage::ReturnDayPointByDayAndMonthIndex(month_type_t month, uint8_t day, uint16_t watts_luminaria, uint8_t qnt_luminarias)
{
    std::string str_return = "";

    if ( day < 32 )
    {
        // Vou navegar no vetor para encontrar o mes de referencia
        uint8_t mes_index = ReturnVectorIndexByMonth(month);

        // Agora vou pegar todos os pontos do dia em questão
        uint8_t j;
        for (j = 0; j < 24 /* horas */ ; j++)
        {
            if (_mes_storage[mes_index]._dia[day]._hora[j] > 0)
            {
                double potencia = _mes_storage[mes_index]._dia[day]._hora[j] * watts_luminaria * qnt_luminarias;
                str_return += std::to_string(j) + ":" + std::to_string(potencia);
                if ( j < 23)
                {
                    str_return += ";";
                }
            }
        }
    }
    return str_return;
}

std::string local_storage::ReturnMonthPointByMonthIndex(month_type_t month, uint16_t watts_luminaria, uint8_t qnt_luminarias)
{
    std::string str_return = std::to_string(month) + ":" + std::to_string(ReturnTotalInWattsSpendOnMonthByIndex(month, watts_luminaria, qnt_luminarias));
    return str_return;
}


std::string local_storage::ReturnAllMonthPoints(uint16_t watts_luminaria, uint8_t qnt_luminarias)
{
    std::string str_return = "";

    uint8_t i;
    for (i = 0; i < 12; i++)
    {
        str_return += ReturnMonthPointByMonthIndex((month_type_t)i, watts_luminaria, qnt_luminarias);
        if (i < 12 - 1)
        {
            str_return += ",";
        }
    }
    return str_return;
}

/**
 * ====================================================
 *                  Private Function 
 * ====================================================
*/

esp_err_t local_storage::LoadDataFromNVS()
{
    esp_err_t err = ESP_FAIL;

    Memory * local_m_memo = Memory::GetObjectMemory();
    err = local_m_memo->Begin();

    size_t max_size_struct_data = sizeof(_mes_storage);

    void* data_read_void_ptr = static_cast<void*>(&_mes_storage);

    err = local_m_memo->GetDataOnMemory(MEMORY_POTENCIA_STORAGE, &data_read_void_ptr, max_size_struct_data, "storage");
    return err;
}


void local_storage::RecordOnNVStheFinishDay()
{
    // FIXME: Aqui deveria retorna o erro da operação
    Memory * m_memo = Memory::GetObjectMemory();
    m_memo->Memory::Begin();
    
    size_t max_size_struct_data = sizeof(_mes_storage);

    void* data_read_void_ptr = static_cast<void*>(&_mes_storage);

    m_memo->RecordOnMemory(MEMORY_POTENCIA_STORAGE, &data_read_void_ptr, max_size_struct_data, "storage");
}

void local_storage::UpdateStructWhich10min(void * args)
{
    local_storage * _ptr_local = local_storage::GetObjs();
    _ptr_local->UpdateValueonStruct();
}

bool local_storage::UpdateValueonStruct()
{
    RealTimeClock * m_rtc = RealTimeClock::getInstance();
    tmElements_t current_time = m_rtc->RealTimeClock::dateTime();

    // Pega o valor do buffer e armazena na hora em que foi coletado

    // FIXME: Aqui é operação sensivel, previsa de semaphoro
    _mes_storage[ReturnVectorIndexByMonth((month_type_t)current_time.Month)]._dia[current_time.Day]._hora[current_time.Hour] += _buffer_storage;
    _buffer_storage = 0;

    // Verifica se virou o dia
    if (current_time.Hour == 0 && current_time.Year > 20 /* faltou um parametro para evitar duplicada no mesmo horaio */)
    {
        RecordOnNVStheFinishDay();
    }
    return true;
}


uint32_t local_storage::ReturnTotalInWattsSpendOnMonthByIndex(month_type_t month, uint16_t watts_luminaria, uint8_t qnt_luminarias)
{
    uint32_t sun = 0;
    // Vou navegar no vetor para encontrar o mes de referencia
    uint8_t j, mes_index = ReturnVectorIndexByMonth(month);

    if (mes_index != 255)
    {
        for (j = 0; j < 31 /* dias */ ; j++)
        {
            // Agora vou varrer todas as horas do dia
            uint8_t k;
            for (k = 0; k < 24 /* horas */; k++)
            {
                sun += _mes_storage[mes_index]._dia[j]._hora[k] * watts_luminaria * qnt_luminarias;
            }
        }
    }
    return sun;
}


uint8_t local_storage::ReturnVectorIndexByMonth(month_type_t month)
{
    uint8_t i;
    for (i = 0; i < sizeof(_mes_storage) /* mes */ ; i++)
    {
        if (_mes_storage[i]._mes == month)
        {
            return i;
        }
    }
    return 255;
}

} // namespace Wetzel 