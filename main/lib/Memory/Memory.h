#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

class Memory{
  public:

    /**
     * @brief Função que salva um dado na memória NVS
     * @param flag nome da partição que será lido o dado
     * @param name_space nome do espaço de memória que será lido o dado
     * @param data ponteiro para o dado que será lido
     * @param size tamanho do dado que será lido 
     * @return ESP_OK se o dado foi salvo com sucesso
    */
    esp_err_t RecordOnMemory(const char* name_space, void **data, size_t size, const char* flag = "nvs");
    
    /**
     * @brief Função que lê um dado na memória NVS
     * @param flag nome da partição que será lido o dado
     * @param name_space nome do espaço de memória que será lido o dado
     * @param data ponteiro para o dado que será lido
     * @param size tamanho do dado que será lido
     * @return ESP_OK se o dado foi lido com sucesso
    */
    esp_err_t GetDataOnMemory(const char* name_space, void **data, size_t size, const char* flag = "nvs");
    
    /**
     * @brief Função que apaga a memória NVS
     * @return ESP_OK se a memória foi apagada com sucesso
    */
    esp_err_t ForceEraseMemory();

    /**
     * @brief Função que inicia a memória NVS
     * @return ESP_OK se a memória foi iniciada com sucesso
    */
    esp_err_t Begin();
    
    /**
     * @brief Função que retorna a quantidade de entradas (espaços de memória mínimos) usadas na memória NVS
     * @param flag nome da partição que será lido o dado
     * @param name_space nome do espaço de memória que será lido o dado
     * @param used_entries Alias para a quantidade de entradas usadas na memória NVS
     * @return ESP_OK se a quantidade de entradas foi lida com sucesso
    */
    esp_err_t GetUsedEntryCount(const char* name_space, size_t& used_entries, const char* flag = "nvs");
    
    
    esp_err_t GetRequiredSize(const char* name_space, size_t& required_size, const char* flag = "nvs");
    
    /**
     * @brief Função que retorna o ponteiro estático da classe Memory
     * @return Ponteiro da classe Memory
    */
    static Memory* GetObjectMemory();
  private:
    static Memory* ptr_memory;

    /**
     * @brief Construtor padrão da classe Memory
    */	
    Memory();

    /**
     * @brief Destrutor padrão da classe Memory
    */
    ~Memory();
};

#endif // _MEMORY_H_