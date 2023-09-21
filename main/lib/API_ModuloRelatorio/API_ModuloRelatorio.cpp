#include "API_ModuloRelatorio.h"
#include "debug.h"

#include <cstring>
#include <map>

namespace Wetzel
{

    /*
     * =========================================================
     *                   Static Variables
     * =========================================================
     */
    static const char *TAG = __FILE__;
    const uint16_t API_ModuloRelatorio::_watts_luminaria[] = {120, 160, 220};
    API_ModuloRelatorio *API_ModuloRelatorio::_ptr_obj = nullptr;
    /*
     * =========================================================
     *                   Public FUNCTION
     * =========================================================
     */

    API_ModuloRelatorio::~API_ModuloRelatorio() {}

    esp_err_t API_ModuloRelatorio::Begin()
    {
        // Limpa os dados
        _devices_info._ptr_devices_info = nullptr;
        _devices_info._qnt_devices = 0;
        total_lumina_on_mesh = 0;
        last_tipo_luminaria = LUM_17K;
        // Carrega os dados se tiver
        if (LoadDataFromNVS())
        {
            // Tenho que descobrir o total de luminarias na rede para realizar as contas
            UpdateTotalLumiOnMesh();
        }
        return ESP_OK;
    }

    bool API_ModuloRelatorio::DeliveryDataByDataRequest(const char *search_data, size_t serach_size, std::string &return_data)
    {
        MY_LOGI("Entrou em DeliveryDataByDataRequest");
        // Blindagens
        if (search_data == nullptr || serach_size == 0)
        {
            return 0;
        }

        bool isOK = false;
        uint8_t day = 0, month = 0, year = 0;

        type_request_http_t index_API = ClassifyMessageReturn(search_data, day, month, year);

        MY_LOGI("Obtem data object");

        local_storage *m_data = local_storage::GetObjs();

        MY_LOGI("Executa o switch case");

        switch (index_API)
        {
        case kReturnHours /* RETURN_PONTOS_EM_HORA */:
        {
            return_data = m_data->ReturnDayPointByDayAndMonthIndex( // FIXME: Aqui deu segfault
                (local_storage::month_type_t)month,
                day,
                _watts_luminaria[(int)last_tipo_luminaria],
                total_lumina_on_mesh);
            if (return_data.size() > 0)
            {
                isOK = true;
            }
            break;
        }
        case kReturnDay /* RETURN_PONTOS_EM_DIAS */:
        {
            return_data = m_data->ReturnMonthPointByMonthIndex((local_storage::month_type_t)month, _watts_luminaria[(int)last_tipo_luminaria], total_lumina_on_mesh);
            if (return_data.size() > 0)
            {
                isOK = true;
            }
            break;
        }
        case kReturnMonth /* RETURN_PONTOS_EM_MES */:
        {
            return_data = m_data->ReturnAllMonthPoints(_watts_luminaria[(int)last_tipo_luminaria], total_lumina_on_mesh);
            if (return_data.size() > 0)
            {
                isOK = true;
            }
            break;
        }

        default:
            break;
        }

        // Devolve os recursos alocados
        m_data = m_data->GiveObjs();
        return isOK;
    }

    bool API_ModuloRelatorio::SetNumberLuminarias(uint8_t id, uint8_t qtd_luminaria)
    {
        // Aqui eu vou tentar navagar por meio do id, mas ai eu preciso dar uns checks
        // para comparar com a struct
        bool isOK = false;

        if (id < _devices_info._qnt_devices)
        {
            _devices_info._ptr_devices_info[id].qtd_luminarias = qtd_luminaria;
            isOK = true;
            RecordDataOnNVS();
        }
        return isOK;
    }

    bool API_ModuloRelatorio::SetTypeLuminarias(uint8_t id, luminaria_type_t type_luminaria)
    {
        // Aqui eu vou tentar navagar por meio do id, mas ai eu preciso dar uns checks
        // para comparar com a struct
        bool isOK = false;

        if (id < _devices_info._qnt_devices)
        {
            _devices_info._ptr_devices_info[id].modelo_luminarias = type_luminaria;
            isOK = true;
            RecordDataOnNVS();
        }

        return isOK;
    }

    API_ModuloRelatorio *API_ModuloRelatorio::GetObjs()
    {
        if (_ptr_obj == nullptr)
        {
            _ptr_obj = new API_ModuloRelatorio;
            if (_ptr_obj == nullptr)
            {
                MY_LOGE("Erro ao criar o API_ModuloRelatorio objeto");
            }
        }
        return _ptr_obj;
    }

    /*
     * =========================================================
     *                   PRIVATE FUNCTION
     * =========================================================
     */

    API_ModuloRelatorio::API_ModuloRelatorio() {}

    bool API_ModuloRelatorio::LoadDataFromNVS()
    {
        esp_err_t err = ESP_FAIL;

        Memory *local_m_memo = Memory::GetObjectMemory();
        err = local_m_memo->Begin();

        size_t max_size_struct_data = sizeof(_devices_info);
        void *data_read_void_ptr = static_cast<void *>(&_devices_info);

        err = local_m_memo->GetDataOnMemory("reportlu", &data_read_void_ptr, max_size_struct_data, "storage");

        if (err == ESP_OK)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool API_ModuloRelatorio::RecordDataOnNVS()
    {
        esp_err_t err = ESP_FAIL;

        Memory *m_memo = Memory::GetObjectMemory();
        m_memo->Memory::Begin();

        size_t max_size_struct_data = sizeof(_devices_info);
        void *data_read_void_ptr = static_cast<void *>(&_devices_info);

        err = m_memo->RecordOnMemory("reportlu", &data_read_void_ptr, max_size_struct_data, "storage");

        if (err == ESP_OK)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    uint16_t API_ModuloRelatorio::UpdateTotalLumiOnMesh()
    {
        uint16_t i;
        uint16_t qnt_lu = 0;
        for (i = 0; i < _devices_info._qnt_devices; i++)
        {
            qnt_lu += _devices_info._ptr_devices_info[i].qtd_luminarias;
        }
        return qnt_lu;
    }

    API_ModuloRelatorio::type_request_http_t API_ModuloRelatorio::ClassifyMessageReturn(const char *search_data, uint8_t &day, uint8_t &month, uint8_t &year)
    {
        MY_LOGI("Entrou em ClassifyMessageReturn");

        type_request_http_t result = kReturnErro;
        std::string data = search_data; // Converter o char* para uma string C++
        size_t day_pos = data.find("day=");
        size_t month_pos = data.find("month=");
        size_t year_pos = data.find("year=");

        if (day_pos != std::string::npos && month_pos != std::string::npos && year_pos != std::string::npos)
        {
            // Encontrou todas as três informações na string
            day = static_cast<uint8_t>(stoi(data.substr(day_pos + 4, 2)));
            month = static_cast<uint8_t>(stoi(data.substr(month_pos + 6, 2)));
            year = static_cast<uint8_t>(stoi(data.substr(year_pos + 5, 4)));

            /// Agora eu preciso classificar a resposta
            if (day == 0)
            {
                if (month == 0)
                {
                    result = kReturnMonth;
                }
                else
                {
                    result = kReturnDay;
                }
            }
            else
            {
                result = kReturnHours;
            }
        }
        MY_LOGI("Resultado do result = %d", (int)result);
        return result;
    }

} // namespace Wetzel