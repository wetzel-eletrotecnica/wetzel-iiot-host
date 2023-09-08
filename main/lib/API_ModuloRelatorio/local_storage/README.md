# local_storage

### Owner: Lucas Dallamico

# Descrição

Implementa uma biblioteca para armazenar as informações de consumo bem como forma de tratar a entregar informações para o APP

## Armazenamento dos dados

As informações são armazenadas em uma struct no seguinte formato:

YEAR
  |
  |__ MES
       |
       |__ Dia
            |
            |__ Hora

Os dados são armazenados em potência da última configuração de luminaria.