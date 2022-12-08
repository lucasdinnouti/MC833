### Instruções de execução

1. Build do servidor: `gcc -Wall servidor.c -o servidor`
2. Build do cliente: `gcc -Wall cliente.c -o cliente`
3. Executar o servidor: `./servidor <PORTA>`
4. Executar um cliente: `./cliente 127.0.0.1 <PORTA>`

Respostas padrões: 
* Do you want to talk to someone? `y`
* Which client do you want to talk to? `<NUMERO_CLIENTE>` (um dos númeres listados)
* ME: `<MENSAGEM>`