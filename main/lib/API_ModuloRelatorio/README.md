# API Modulo Relatorio

### Owner: Lucas Dallamico

# Anotações

08/09/23
=========================

- Rota para entregar a informação para o aplicativo

=========================

Vou criar um retorno que usar uma resposta padrão 
	- Eu vou retornar a resposta em um ponteiro inteligente 
		- De maneira que n de ruim realocar e destruir
		OOOOOOUUU
		
		- EU meio que preciso que retorna já pronto em char * para fazer uma operação a menos
		
		- std::string . c_str() -> já garanto que consigo fazer a cópia 
		
		- EEEEEEEE consigo pegar o tamnaho de retorno 
			- Pq em teoria depois que eu retornar não será preciso fazer mais alterações
			


${root}/report

- Criar uma função na API_relatorio que wrappeia todo a comunicação para o local_storage
- Adaptar uma implementação para pegar o e atualizar a informação da rede


-----> Estrutura para guardar a informação de setagem externa no APP
- Eu vou criar uma struct que 

meu_MAC_t
{
	uint8_t[6];
	uint8_t qnt_luminarias
	uint8_t tipo_lu ; --: Prevenção para problemas futuroas
}

device_info
{
	meu_mac_t * devices_mac; --> Como eu armazeno essa esturrtura? 
	
	size_t offset_devices_mac = ??;
}

- Tem um jeito facil
	- Para cada offset que eu escrever na memoria eu desloco X sizeof(meu_mac_t) até o size_elementos registrados
	- Se dispositos novos morrem ou serem apagados ??? 
		- O histórico de consumo n muda
			- Só se der um novo set, altera o futuro 