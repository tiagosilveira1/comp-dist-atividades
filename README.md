# Atividade de sockets - Batalha de palavras

## 1. Nome e RA Integrantes do grupo
**Tiago Silveira Lopes, RA: 10417600**

## 2. Link Repositório

**Link:** https://github.com/tiagosilveira1/comp-dist-atividades

## 3. Descrição

Batalha de palavras feito com sockets, onde um servidor aceita conexões de, no máximo, dois clientes (quantidade necessária para iniciar a partida).

O servidor aguarda a conexão de dois jogadores.

Quando os dois jogadores estiverem conectados:

cada jogador informa seu nome;
o servidor inicia a partida;
uma letra é sorteada;
os dois jogadores recebem a letra;
os jogadores possuem 10 segundos para responder;
o servidor valida as respostas;
o placar é atualizado;
o processo é repetido por 5 rodadas;
o resultado final é enviado aos jogadores.

**Obs: programa executado e compilado com Windows, com utilização de ifdef _WIN32 e else para suporte da plataforma.**

## 4. Processo de compilação

A versão do GCC utilizada foi 14.2.0

A compilação pode ser realizada utilizando o Makefile:

make

Para remover os arquivos gerados:

make clean

**Compilação Manual**

Cliente.c:
``
gcc -Wall -Wextra -pedantic -std=c11 -o cliente cliente.c -lws2_32
``

Servidor.c:
``
gcc -Wall -Wextra -pedantic -std=c11 -o servidor servidor.c jogo.c -lpthread
``

## 5. Execução

1. Iniciar o servidor

Utilizando a porta padrão:

``
./servidor
``

Ou especificando outra porta:

``
./servidor 9000
``

O servidor ficará aguardando conexões de jogadores.

**2. Iniciar os clientes**

Em dois terminais diferentes:

``
./cliente
``

Por padrão, o cliente conecta em:

127.0.0.1:7070

Também é possível informar o IP e a porta:

``
./cliente 127.0.0.1 9000
``

Para utilizar outro computador da mesma rede:

``
./cliente 192.168.1.10 7070
``

## 6. Estrutura do projeto

Seguiu conforme README.md dos requisitos do projeto no [github do professor](https://github.com/traue/26.2-comp-dist-06D/tree/main/01_sockets/atividade):

| **Arquivo** | **Responsabilidade** |
| protocolo.h | Define constantes, limites, porta padrão e tipos de mensagens utilizados na comunicação. |
| jogo.h | Declara as funções relacionadas à lógica do jogo. |
| jogo.c | Implementa a validação das palavras, comunicação formatada pelo protocolo, controle de tempo e funções auxiliares. |
| servidor.c | Implementa o servidor TCP, gerenciamento dos jogadores, criação das threads e controle das partidas. |
| cliente.c | Implementa o cliente TCP, interface textual, recebimento das mensagens e envio das respostas. |
| Makefile | Automatizador do processo de compilação. |
| README.md | Documentação do projeto. |


## x. Tecnologias Utilizadas
Linguagem: C
Comunicação: Sockets TCP
Concorrência: POSIX Threads (pthread)
Controle de tempo: select()
Sistema operacional: Windows 10
Compilação: GCC
Automação: Makefile
