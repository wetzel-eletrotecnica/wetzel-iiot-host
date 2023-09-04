# Memory
### Owner: Pedro Melo 

## Detalhes

Para salvar um dado na memória primeiro é necessário fazer uma conversão desse para um ponteiro `void*` e obter o espaço requerido para salvá-lo. Além disso, é preciso especificar a região na tabela de partição que essa informação será salva e o seu namespace (espeficado pelo usuário).

Para ler o dado salvo no NVS também é necessário obter o espaço requirido por ele, especificar a região na tabela de partição que esse dado foi salvo e o namespace usado na gravação. O retorno é necessário passar como parâmetro um ponteiro alocado com a memória requirida pelo dado. 

A classe permite apagar a memória NVS usada pelo software e consultar o número de espaços ou entradas usados.

## Notas do editor

Essa função não é thread safe e a implementação deve levar em conta competições por recurso. Além disso, a implementação foi feita em C++ e recomenda-se usar ferramentas dessa linguagem.

## Exemplo de uso da Memory

Nesse exemplo são mostradas as duas funções básicas de gravar e ler dados. Fica claro que o usuário deve ser resposável por converter os dados que serão salvo para uma forma genérica dada por `void*`.

```
#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include "Memory.h"

extern "C" void app_main(void) {

  Memory* memory_ptr = Memory::GetPtrMemory();

  memory_ptr->Begin();

  //Escrita
  std::string nome_str = "Hello Machine";
  size_t required_size = nome_str.length() + 1;
  void* data_write_void_ptr = static_cast<void*>(const_cast<char*>(nome_str.c_str()));
  memory_ptr->RecordOnMemory("nvs", "storage_machine", &data_write_void_ptr, required_size);

  // Leitura
  std::unique_ptr<char[]> data_read_char_ptr(new char[required_size]);
  void* data_read_void_ptr = static_cast<void*>(data_read_char_ptr.get());
  err = memory_ptr->GetDataOnMemory("nvs", "storage_machine", &data_read_void_ptr, required_size);
  std::string nome_str_read = data_read_char_ptr.get();

  std::cout << "Data read: " << nome_str_read << std::endl;

  size_t used_entries;

  err = memory_ptr->GetUsedEntryCount("nvs", "storage_machine", used_entries);

  std::cout << "Num entries 1: " << used_entries << std::endl;
}

```