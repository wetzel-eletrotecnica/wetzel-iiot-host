#include "local_storage.h"
#include "real_time_clock.h"
#include "Memory.h"
#include <memory>

static const char* TAG = __FILE__;

#define MEMORY_FLAG_STORAGE  "relatorio"

std::map< local_storage::month_type_t, std::list<uint8_t>> local_storage::_pwm_month;
std::list<uint8_t> local_storage::_pwm_dia;
uint8_t local_storage::_current_day = 0;
local_storage::month_type_t local_storage::_current_month = kJAN;
local_storage * local_storage::_ptr_local_storage = nullptr;

local_storage::local_storage(/* args */) {}

local_storage::~local_storage(){}

esp_err_t local_storage::Begin()
{
    // Inicializa as variaveis

    // Obtem o dia atual
    RealTimeClock * m_rtc = RealTimeClock::getInstance();
    tmElements_t current_time = dateTime();
    SetCurrentMonthAndDay(monthShortStr(current_time.Month), dayStr(current_time.Day));

    // Verifica na memoria se exite informações armazenadas
    Memory * m_memo = Memory::GetObjectMemory();
    m_memo->Memory::Begin();

    size_t max_size_struct_data = sizeof(_mes_storage);

    std::unique_ptr<mes_type_t[]> data_read_char_ptr(new mes_type_t[8]);
    void* data_read_void_ptr = static_cast<void*>(data_read_char_ptr.get());

    esp_err_t err = ESP_FAIL;
    err = m_memo->GetDataOnMemory("nvs", MEMORY_FLAG_STORAGE, &data_read_void_ptr, max_size_struct_data);
    if (err == ESP_OK)
    {
        std::memcpy(&_mes_storage, &data_read_void_ptr, max_size_struct_data);
    }
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

bool local_storage::RecordKWHvalue(float value)
{
    _kwh_price = value;
    return true;
}

bool local_storage::RecordPWMonStorage(uint8_t value)
{
    // Faz uma média para ir acumulando a mudança
    buffer_storage = (buffer_storage + value) / 2;
    return true;
}

//TODO: PArei aqui 
local_storage::month_type_t local_storage::ConvertMonthStringInEnum(char * month, size_t size)
{
    return kJAN;
}

std::string local_storage::ReturnDayPointByDayAndMonthIndex(month_type_t month, uint8_t day)
{
    return "";
}
std::string local_storage::ReturnMonthPointByMonthIndex(month_type_t month)
{
    return "";
}

bool local_storage::SetCurrentMonthAndDay(char * month, char *  day) { return true;}

esp_err_t  local_storage::LoadDataFromNVS()
{
    return ESP_OK;
}
void local_storage::NextDay(){}
void local_storage::UpdateStructWhich10min(void * args)
{

}