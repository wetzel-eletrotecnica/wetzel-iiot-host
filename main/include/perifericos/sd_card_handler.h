#ifndef _SD_CARD_HANDLER_H_
#define _SD_CARD_HANDLER_H_

#include <driver/sdmmc_host.h>
#include <stdio.h>

namespace Wetzel {

#define MOUNT_POINT "/sdcard"
#define EXAMPLE_MAX_CHAR_SIZE 64  // Variável de configuração de exemplo (temporaria)
#define MAX_FILES_OPENED 2        // 1 -> Read | 1 -> Write
#define ALLOCATION_UNIT_SIZE 1024 * 16
#define MAX_FREQ_KHZ 20000 / 2
#define FORMAT_IF_MOUNT_FAILED true
#define SPI_MAX_TRANSFER_SIZE 4000
#define MAX_CARD_MOUNT_ATTEMPTS 5

/**
 * @brief Definições virtuais de FILEs. Apenas uma de cada tipo disponível.
 * 
 */
typedef enum { WRITING_FILE, READING_FILE } file_type_t;

class CartaoSD {
   private:
    CartaoSD();

    /* data */
    static CartaoSD* _instance;

    FILE* _writing_file = NULL;
    FILE* _reading_file = NULL;

    bool _writing_file_is_open = false;
    bool _reading_file_is_open = false;

    // Variáveis necessárias de configuração da SDMMC Lib
    sdmmc_host_t _host;
    sdmmc_card_t* _card = NULL;

    char writing_file_path[25];
    char reading_file_path[25];

   public:
    ~CartaoSD();

    static CartaoSD* getInstance();

    /**
     * @brief Inicializa módulo do cartão SD. Configura pinos, partição, filas e SPI. 
     * 
     * @note Pinos:
     * @param mosi_port  
     * @param miso_port 
     * @param sclk_port 
     * @param cs_port
     *  
     * @return esp_err_t 
     */
    esp_err_t begin(gpio_num_t mosi_port, gpio_num_t miso_port, gpio_num_t sclk_port,
                    gpio_num_t cs_port);

    /**
     * @brief Abre FILE especificada e retorna seu ponteiro.
     * @note Apenas uma FILE por tipo pode estar aberta ao mesmo tempo (Read / Write)
     * 
     * @param file_path 
     * @param type 
     * @return FILE* 
     */
    FILE* openFile(const char* file_path, file_type_t type);

    /**
     * @brief Fecha FILE relacionada ao tipo especificado (Read / Write)
     * 
     * @param file 
     * @return esp_err_t 
     */
    esp_err_t closeFile(file_type_t file);

    // Retornam FILE*
    FILE* writingFile();
    FILE* readingFile();

    // Retornam FILE path
    char* writingFilePath();
    char* readingFilePath();
};

}  // namespace Wetzel
#endif