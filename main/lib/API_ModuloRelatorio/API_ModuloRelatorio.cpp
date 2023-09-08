#include "API_ModuloRelatorio.h"
#include "debug.h"

#include <cstring>

namespace Wetzel {

/*
 * =========================================================
 *                   Static Variables
 * =========================================================
*/
static const char* TAG = __FILE__;
const char * API_ModuloRelatorio::_valid_return_api[] = {RETURN_PONTOS_EM_HORA,RETURN_PONTOS_EM_DIAS,RETURN_PONTOS_EM_MES,SET_QTD_LUMINARIA,SET_TYPE_LUMINARIA};
const uint16_t API_ModuloRelatorio::_watts_luminaria[] = {120, 160, 220};
API_ModuloRelatorio * API_ModuloRelatorio::_ptr_obj = nullptr;
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
    // Carrega os dados se tiver
    if (LoadDataFromNVS())
    {
        // Tenho que descobrir o total de luminarias na rede para realizar as contas
        UpdateTotalLumiOnMesh();
    }
    return ESP_OK;
}

bool API_ModuloRelatorio::DeliveryDataByDataRequest(char * search_data, size_t serach_size, std::string & return_data)
{
    // Blindagens
    if (search_data == nullptr || serach_size == 0) {return 0;}

    bool isOK = false;

    uint8_t index_API = ClassifyMessageReturnIndexVectorAPI(search_data);

    local_storage * m_data = local_storage::GetObjs();

    switch (index_API)
    {
        case 0 /* RETURN_PONTOS_EM_HORA */ :
        {
            // Encontre a posição do primeiro caractere após "DD:"
            const char* dia_start = std::strstr(search_data, "DD:") + 3;

            // Extrair o dia como uma sequência de caracteres
            char dia_str[3];
            std::strncpy(dia_str, dia_start, 2);
            dia_str[2] = '\0';

            // Converter a sequência de caracteres em uint8_t
            uint8_t m_dia = std::atoi(dia_str);

            // Encontre a posição do primeiro caractere após o dia
            const char* mes_start = dia_start + 3;

            // Extrair o mês como uma sequência de caracteres
            char mes_str[3];
            std::strncpy(mes_str, mes_start, 2);
            mes_str[2] = '\0';

            // Converter a sequência de caracteres em uint8_t
            local_storage::month_type_t m_mes = (local_storage::month_type_t) std::atoi(mes_str);

            return_data = m_data->ReturnDayPointByDayAndMonthIndex(m_mes, m_dia, _watts_luminaria[(int)last_tipo_luminaria], total_lumina_on_mesh);
            if (return_data.size() > 0) {isOK = true;}
            break;
        }
        case 1 /* RETURN_PONTOS_EM_DIAS */ :
        {
            // Encontre a posição de "MM:" na mensagem
            const char* mes_start = std::strstr(search_data, "MM:");
            if (mes_start != nullptr) 
            {
                // Avance para o primeiro caractere após "MM:"
                mes_start += 3;

                // Extrair o mês como uma sequência de caracteres
                char mes_str[3];
                std::strncpy(mes_str, mes_start, 2);
                mes_str[2] = '\0';

                // Converter a sequência de caracteres em uint8_t
                local_storage::month_type_t m_mes = (local_storage::month_type_t) std::atoi(mes_str);

                return_data = m_data->ReturnMonthPointByMonthIndex(m_mes, _watts_luminaria[(int)last_tipo_luminaria], total_lumina_on_mesh);
                if (return_data.size() > 0) {isOK = true;}
            }
            break;
        }
        case 2 /* RETURN_PONTOS_EM_MES */:
        {
            return_data = m_data->ReturnAllMonthPoints(_watts_luminaria[(int)last_tipo_luminaria], total_lumina_on_mesh);
            if (return_data.size() > 0) {isOK = true;}
            break;
        }
        default:
        {
            if (index_API == 3 || index_API == 4)
            {
                // Encontre a posição de "qtd_luminaria:" na mensagem
                char* qtd_or_type = nullptr;
                if (index_API == 3)
                {
                    qtd_or_type = std::strstr(search_data, "qtd_luminaria:") + 14;
                }
                else
                {
                    qtd_or_type = std::strstr(search_data, "type_luminaria:") + 15;
                }

                // Extrair a quantidade de luminárias como uma sequência de caracteres
                char buffer[3];
                std::strncpy(buffer, qtd_or_type, 2);
                buffer[2] = '\0';

                // Converter a sequência de caracteres em uint8_t
                uint8_t num = std::atoi(buffer);

                // Encontre a posição de "id:" na mensagem
                const char* id_start = std::strstr(search_data, "id:") + 3;

                // Extrair o ID como uma sequência de caracteres
                char id_str[2];
                std::strncpy(id_str, id_start, 1);
                id_str[1] = '\0';

                // Converter a sequência de caracteres em uint8_t
                uint8_t m_id = std::atoi(id_str);

                if (index_API == 3)
                {
                    isOK = SetNumberLuminarias(m_id, num);
                }
                else
                {
                    isOK = SetTypeLuminarias(m_id, (luminaria_type_t)num);
                }
                // Salva as alterações na NVS do dispostivo
                RecordDataOnNVS();
            }
            break;
        }
    }
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
    }

    return isOK;
}

API_ModuloRelatorio * API_ModuloRelatorio::GetObjs()
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

API_ModuloRelatorio::API_ModuloRelatorio(){}

bool API_ModuloRelatorio::LoadDataFromNVS()
{
    esp_err_t err = ESP_FAIL;

    Memory * local_m_memo = Memory::GetObjectMemory();
    err = local_m_memo->Begin();

    size_t max_size_struct_data = sizeof(_devices_info);
    void* data_read_void_ptr = static_cast<void*>(&_devices_info);

    err = local_m_memo->GetDataOnMemory("reportlu", &data_read_void_ptr, max_size_struct_data, "storage");
    
    if (err == ESP_OK) {return true;}
    else {return false;}
}

bool API_ModuloRelatorio::RecordDataOnNVS()
{
    esp_err_t err = ESP_FAIL;

    Memory * m_memo = Memory::GetObjectMemory();
    m_memo->Memory::Begin();
    
    size_t max_size_struct_data = sizeof(_devices_info);
    void* data_read_void_ptr = static_cast<void*>(&_devices_info);

    err = m_memo->RecordOnMemory("reportlu", &data_read_void_ptr, max_size_struct_data, "storage");
    
    if (err == ESP_OK) {return true;}
    else {return false;}
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

uint8_t API_ModuloRelatorio::ClassifyMessageReturnIndexVectorAPI(const char* search_data) 
{
    const uint8_t posi_ref = 6; // Até onde é padrão a URL para comparar
    size_t search_data_length = std::strlen(search_data);

    if (search_data_length < posi_ref + 1) {
        return 255;
    }

    for (uint8_t i = 0; i < 5 /* len _valid_return_api */; i++) {
        if (std::strncmp(search_data + posi_ref, _valid_return_api[i] + posi_ref, search_data_length - posi_ref) == 0) {
            return i;
        }
    }

    return 255; // Se a mensagem não corresponder a nenhuma definição conhecida.
}

} // namespace Wetzel