#include "Memory.h"

namespace Wetzel {

Memory* Memory::ptr_memory = nullptr;

/**
 * @brief Construtor padrão da classe Memory
*/
Memory::Memory() {}

/**
 * @brief Destrutor padrão da classe Memory
*/
Memory::~Memory() {
  //! Tirar Deinit e desalocação de memória daqui
  nvs_flash_deinit();
  if(ptr_memory != nullptr) {
    delete ptr_memory;
    ptr_memory = nullptr;
  }
}

/**
 * @brief Função que inicia a memória NVS
 * @return ESP_OK se a memória foi iniciada com sucesso
*/
esp_err_t Memory::Begin(){
  esp_err_t err = nvs_flash_init();
  if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
    // NVS está corrompida e precisa ser apagada 
    // Tentar novamente iniciar o NVS 
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  return err;
}


/**
 * @brief Função que apaga a memória NVS
 * @return ESP_OK se a memória foi apagada com sucesso
*/
esp_err_t Memory::ForceEraseMemory(){
  esp_err_t err;
  err = nvs_flash_erase();
  if(err != ESP_OK){
    return err;
  } 
  return ESP_OK;
}

/**
 * @brief Função que retorna o ponteiro estático da classe Memory
 * @return Ponteiro da classe Memory
*/
Memory* Memory::GetObjectMemory(){
  if(ptr_memory == nullptr) {
    ptr_memory = new Memory();
    if (ptr_memory == nullptr) {
      //ESP_LOGE("[Sem memória]", "Sem memoria para alocar um objeto da classe Memory");
      return nullptr;
    }
  }
  return ptr_memory;
}

/**
 * @brief Função que salva um dado na memória NVS
 * @param flag nome da partição que será lido o dado
 * @param name_space nome do espaço de memória que será lido o dado
 * @param data ponteiro para o dado que será lido
 * @param size tamanho do dado que será lido 
 * @return ESP_OK se o dado foi salvo com sucesso
*/
esp_err_t Memory::RecordOnMemory(const char* name_space, void **data, size_t size, const char* flag){
  esp_err_t err;


  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle_from_partition(flag, name_space, NVS_READWRITE, &err);
  if(err != ESP_OK){
    return err;
  } 

  if(handle->set_blob(name_space, *data, size) != ESP_OK){
    return err;
  } 

  if(handle->commit() != ESP_OK){
    return err;
  } 

  return ESP_OK;
}

/**
 * @brief Função que lê um dado na memória NVS
 * @param flag nome da partição que será lido o dado
 * @param name_space nome do espaço de memória que será lido o dado
 * @param data ponteiro para o dado que será lido
 * @param size tamanho do dado que será lido
 * @return ESP_OK se o dado foi lido com sucesso
*/
esp_err_t Memory::GetDataOnMemory(const char* name_space, void **data, size_t size, const char* flag){
  esp_err_t err;

  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle_from_partition(flag, name_space, NVS_READWRITE, &err);
  if(err != ESP_OK){
    return err;
  } 

  if (handle->get_blob(name_space, *data, size) != ESP_OK) {
    return err;
  }
  return ESP_OK;
}

/**
 * @brief Função que retorna a quantidade de entradas (espaços de memória mínimos) usadas na memória NVS
 * @param flag nome da partição que será lido o dado
 * @param name_space nome do espaço de memória que será lido o dado
 * @param used_entries Alias para a quantidade de entradas usadas na memória NVS
 * @return ESP_OK se a quantidade de entradas foi lida com sucesso
*/
esp_err_t Memory::GetUsedEntryCount(const char* name_space, size_t& used_entries, const char* flag){
  esp_err_t err;

  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle_from_partition(flag, name_space, NVS_READWRITE, &err);
  if(err != ESP_OK){
    return err;
  } 

  if(handle->get_used_entry_count(used_entries) != ESP_OK){
    return err;
  }

  return ESP_OK;
}

esp_err_t Memory::GetRequiredSize(const char* name_space, size_t& required_size, const char* flag){
  esp_err_t err;

  std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle_from_partition(flag, name_space, NVS_READWRITE, &err);
  if (err != ESP_OK) {
      return err;
  }
  
  err = handle->get_item_size(nvs::ItemType::BLOB, name_space, required_size);
  if(err != ESP_OK){
    printf((err != ESP_OK) ? "Erro (%s) ao checar espaço!\n" : "OK\n", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

} // namespace Wetzel 