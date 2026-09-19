/*
INTEGRANTES DO GRUPO:
Aluno: Tiago Silveira Lopes, RA: 10417600
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif
#include <pthread.h>
#include "protocolo.h"

#define MAX_CLIENTES 2

typedef struct {
    int socket;
    char nome[50];
    int conectado;
} Jogador;

Jogador jogadores[MAX_CLIENTES];

pthread_mutex_t mutex_jogadores = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_jogadores = PTHREAD_COND_INITIALIZER;


int jogadores_conectados = 0;
int servidor_socket = -1;
int executando = 1;

/*
 * Envia uma mensagem completa para um cliente.
 */
int enviar_mensagem(int socket, const char *mensagem)
{
    size_t tamanho = strlen(mensagem);

    if (send(socket, mensagem, tamanho, 0) < 0) {
        perror("send");
        return -1;
    }

    return 0;
}

/*
 * Envia uma mensagem para os dois jogadores.
 */
void enviar_para_jogadores(const char *mensagem)
{
    pthread_mutex_lock(&mutex_jogadores);

    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (jogadores[i].conectado) {
            enviar_mensagem(jogadores[i].socket, mensagem);
        }
    }

    pthread_mutex_unlock(&mutex_jogadores);
}

int criar_servidor(int porta)
{
    int socket_servidor;

    // Cria um socket para comunicação.
    // AF_INET: utiliza endereços IPv4.
    // SOCK_STREAM: utiliza TCP, que é orientado à conexão.
    // 0: deixa o sistema escolher automaticamente o protocolo
    // adequado para o tipo de socket (TCP).
    socket_servidor = socket(AF_INET,SOCK_STREAM,0);

    if (socket_servidor < 0) {
        perror("socket");
        return -1;
    }

    int opcao = 1;

    // Configura uma opção do socket.
    // SOL_SOCKET: a opção pertence ao próprio socket.
    // SO_REUSEADDR: permite reutilizar o endereço/porta
    // mesmo que ela tenha sido usada recentemente por outro socket.
    // &opcao: endereço do valor da opção (1 = habilitado).
    // sizeof(opcao): tamanho do valor enviado.
    if (setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEADDR,&opcao,sizeof(opcao)) < 0) {
        perror("setsockopt");
        close(socket_servidor);
        return -1;
    }

    // Estrutura utilizada para armazenar o endereço do servidor.
    struct sockaddr_in endereco;

    // Inicializa toda a estrutura com zero.
    // Isso evita que campos não utilizados contenham lixo de memória.
    memset(&endereco, 0, sizeof(endereco));

    // Define que o endereço utiliza IPv4.
    endereco.sin_family = AF_INET;
    // Permite que o servidor aceite conexões através de qualquer
    // endereço de rede disponível na máquina.
    endereco.sin_addr.s_addr = INADDR_ANY;
    // Define a porta utilizada pelo servidor.
    // htons() converte o número da porta para o formato
    // de bytes utilizado pela rede (network byte order).
    endereco.sin_port = htons(porta);

    // Associa o socket ao endereço e à porta configurados.
    // bind() faz o socket "pertencer" àquela porta/endereço.
    if (bind(socket_servidor,(struct sockaddr *)&endereco,sizeof(endereco)) < 0) {
        perror("bind");
        close(socket_servidor);
        return -1;
    }

    // Coloca o socket em modo de escuta.
    // A partir daqui, o servidor fica aguardando conexões
    // de clientes.
    // MAX_CLIENTES define quantas conexões podem ficar
    // aguardando na fila de conexões.
    if (listen(socket_servidor, MAX_CLIENTES) < 0) {
        perror("listen");
        close(socket_servidor);
        return -1;
    }
    return socket_servidor;
}

void tratar_interrupcao(int sinal)
{
    (void)sinal;

    executando = 0;

    if (servidor_socket != -1) {
        close(servidor_socket);
    }
}

/*
 * Thread responsável por receber o nome do jogador.
 */
void *atender_jogador(void *arg)
{
    int indice = *(int *)arg;

    free(arg);

    char buffer[256];

    int bytes = recv(
        jogadores[indice].socket,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (bytes <= 0) {
        printf("[-] Jogador desconectou.\n");

        close(jogadores[indice].socket);

        pthread_mutex_lock(&mutex_jogadores);

        jogadores[indice].conectado = 0;
        jogadores_conectados--;

        pthread_mutex_unlock(&mutex_jogadores);

        return NULL;
    }

    buffer[bytes] = '\0';

    /*
     * Esperamos:
     *
     * NOME|Alice
     */
    if (strncmp(buffer, MSG_NOME "|", strlen(MSG_NOME) + 1) == 0) {

        char *nome = buffer + strlen(MSG_NOME) + 1;

        /*
         * Remove '\n', caso exista.
         */
        nome[strcspn(nome, "\r\n")] = '\0';

        strncpy(jogadores[indice].nome,nome,sizeof(jogadores[indice].nome) - 1);

        jogadores[indice].nome[sizeof(jogadores[indice].nome) - 1] = '\0';

        printf("[+] Jogador %d: %s\n",indice + 1,jogadores[indice].nome);

        char resposta[256];

        snprintf(resposta,sizeof(resposta),MSG_BEM_VINDO "|%s\n",jogadores[indice].nome);

        enviar_mensagem(jogadores[indice].socket,resposta);
    }

    return NULL;
}

/*
 * Inicia a partida.
 *
 * Nesta primeira versão apenas sincronizamos
 * os jogadores. A lógica das cinco rodadas
 * pode ficar em jogo.c.
 */
void iniciar_partida(void)
{
    char mensagem[256];

    pthread_mutex_lock(&mutex_jogadores);

    snprintf(mensagem,sizeof(mensagem),"%s|%s vs %s\n",MSG_RODADA,jogadores[0].nome,jogadores[1].nome);

    pthread_mutex_unlock(&mutex_jogadores);

    printf("[Partida] %s vs %s\n",jogadores[0].nome,jogadores[1].nome);

    enviar_para_jogadores(mensagem);
}

int main(int argc, char *argv[])
{
    //-----------------------------------------------------------------------
    //Variáveis locais ------------------------------------------------------
    int porta = PORTA; //7070 padrão
    pthread_t thread;

    //-----------------------------------------------------------------------
    //Argumentos ------------------------------------------------------------

    //Permitir ./servidor <porta>
    if (argc >= 2) {
        porta = atoi(argv[1]);

        if (porta <= 0 || porta > 65535) {
            fprintf(stderr, "Porta inválida.\n");
            return EXIT_FAILURE;
        }
    }

    //-----------------------------------------------------------------------
    //Sinal de interrupção --------------------------------------------------
    signal(SIGINT, tratar_interrupcao);
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    //-----------------------------------------------------------------------
    //Menu ------------------------------------------------------------------
    printf("========================================\n");
    printf("     BATALHA DE PALAVRAS - SERVIDOR\n");
    printf("========================================\n");
    printf("Porta: %d\n", porta);
    printf("Aguardando %d jogadores...\n\n", MAX_CLIENTES);

    //-----------------------------------------------------------------------
    //Criando servidor ------------------------------------------------------
    servidor_socket = criar_servidor(porta);

    if (servidor_socket < 0) {
        return EXIT_FAILURE;
    }

    //-----------------------------------------------------------------------
    //inicialização estrutura do jogador ------------------------------------
    for (int i = 0; i < MAX_CLIENTES; i++) {
        jogadores[i].socket = -1;
        jogadores[i].nome[0] = '\0';
        jogadores[i].conectado = 0;
    }

    while (executando && jogadores_conectados < MAX_CLIENTES) {
        struct sockaddr_in endereco_cliente;
#ifdef _WIN32
        int tamanho = sizeof(endereco_cliente);
#else
        socklen_t tamanho = sizeof(endereco_cliente);
#endif

        int socket_cliente = accept(servidor_socket,(struct sockaddr *)&endereco_cliente,&tamanho);

        if (socket_cliente < 0) {

            if (!executando) {
                break;
            }

            perror("accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN];

        inet_ntop(AF_INET,&endereco_cliente.sin_addr,ip,sizeof(ip));

        printf("[+] Jogador conectou: %s:%d\n",ip,ntohs(endereco_cliente.sin_port));

        pthread_mutex_lock(&mutex_jogadores);

        int indice = -1;

        for (int i = 0; i < MAX_CLIENTES; i++) {
            if (!jogadores[i].conectado) {
                indice = i;
                break;
            }
        }

        if (indice == -1) {
            pthread_mutex_unlock(&mutex_jogadores);

            enviar_mensagem(socket_cliente,MSG_SERVIDOR_CHEIO "\n");

            close(socket_cliente);
            continue;
        }

        jogadores[indice].socket = socket_cliente;
        jogadores[indice].conectado = 1;

        jogadores_conectados++;

        /*
         * Avisa o cliente para enviar o nome.
         */
        enviar_mensagem(socket_cliente,MSG_NOME "|\n");

        pthread_cond_broadcast(&cond_jogadores);

        pthread_mutex_unlock(&mutex_jogadores);

        /*
         * Cria uma thread para esse cliente.
         */

        int *arg = malloc(sizeof(int));

        if (arg == NULL) {
            perror("malloc");

            close(socket_cliente);

            pthread_mutex_lock(&mutex_jogadores);

            jogadores[indice].conectado = 0;
            jogadores_conectados--;

            pthread_mutex_unlock(&mutex_jogadores);

            continue;
        }

        *arg = indice;

        //-----------------------------------------------------------------------
        //Criar thread para um jogador ------------------------------------------
        if (pthread_create(&thread,NULL,atender_jogador,arg) != 0) {

            perror("pthread_create");
            free(arg);
            close(socket_cliente);
            pthread_mutex_lock(&mutex_jogadores);
            jogadores[indice].conectado = 0;
            jogadores_conectados--;
            pthread_mutex_unlock(&mutex_jogadores);

            continue;
        }

        pthread_detach(thread);
    }

    //-----------------------------------------------------------------------
    //Esperar pelos jogadores -----------------------------------------------
    if (executando &&jogadores_conectados == MAX_CLIENTES) {
        /*
         * Pequena espera para as threads receberem
         * os nomes dos jogadores.
         */
        sleep(1);
        iniciar_partida();
    }

    printf("\n[*] Servidor encerrado.\n");

    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (jogadores[i].socket != -1) {
            close(jogadores[i].socket);
        }
    }

    if (servidor_socket != -1) {
        close(servidor_socket);
    }

    pthread_mutex_destroy(&mutex_jogadores);
    pthread_cond_destroy(&cond_jogadores);

    return EXIT_SUCCESS;
}
