/*
INTEGRANTES DO GRUPO:
Aluno: Tiago Silveira Lopes, RA: 10417600
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif
#include <pthread.h>
#include "protocolo.h"
#ifdef _WIN32
#define FECHAR_SOCKET closesocket
#else
#define FECHAR_SOCKET close
#endif
#include "jogo.h"

Jogador jogadores[MAX_CLIENTES];

pthread_mutex_t mutex_jogadores = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nomes = PTHREAD_COND_INITIALIZER;


int jogadores_conectados = 0;
int servidor_socket = -1;
int executando = 1;
int nomes_recebidos = 0;


/*
 * Envia todos os bytes da mensagem.
 *
 */
int enviar_mensagem(int socket, const char *mensagem)
{
    int tamanho;
    int enviados = 0;
    int resultado;

    tamanho = strlen(mensagem);

    while (enviados < tamanho) {

        resultado = send(socket,mensagem + enviados,tamanho - enviados,0);

        if (resultado <= 0) {
            return -1;
        }

        enviados += resultado;
    }

    return 0;
}


/*
 * Recebe uma mensagem do socket.
 *
 */
int receber_mensagem(int socket, char *mensagem, int tamanho)
{
    int posicao = 0;
    char caractere;
    int resultado;

    if (tamanho <= 1) {
        return -1;
    }

    while (posicao < tamanho - 1) {

        resultado = recv(
            socket,
            &caractere,
            1,
            0
        );

        if (resultado == 0) {
            /*
             * O outro lado fechou a conexão.
             */
            return 0;
        }

        if (resultado < 0) {
            return -1;
        }

        if (caractere == '\n') {
            break;
        }

        mensagem[posicao] = caractere;
        posicao++;
    }

    mensagem[posicao] = '\0';

    return 1;
}

/*
 * Remove '\n' e '\r' do final da string.
 *
 * Útil para mensagens recebidas através
 * do protocolo.
 */
void remover_quebra_linha(char *texto)
{
    int tamanho;

    if (texto == NULL) {
        return;
    }

    tamanho = strlen(texto);

    while (tamanho > 0 &&(texto[tamanho - 1] == '\n' || texto[tamanho - 1] == '\r')) {
        texto[tamanho - 1] = '\0';
        tamanho--;
    }
}

/*
 * Envia:
 *
 * MSG|texto\n
 */
int enviar_msg(int socket, const char *texto)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),MSG "|%s\n",texto);

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * NOME|\n
 */
int enviar_solicitacao_nome(int socket)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),NOME "|\n");

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * AGUARDE|texto\n
 */
int enviar_aguarde(int socket, const char *texto)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),AGUARDE "|%s\n",texto);

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * RODADA|numero|letra|tempo\n
 */
int enviar_rodada(int socket,int numero,char letra,int tempo)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),RODADA "|%d|%c|%d\n",numero,letra,tempo);

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * RESULTADO|texto\n
 */
int enviar_resultado(int socket, const char *texto)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),RESULTADO "|%s\n",texto);

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * PLACAR|nome1|pontos1|nome2|pontos2\n
 */
int enviar_placar(int socket,const char *nome1,int pontos1,const char *nome2,int pontos2)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),PLACAR "|%s|%d|%s|%d\n",nome1,pontos1,nome2,pontos2);

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * FIM|texto\n
 */
int enviar_fim(int socket, const char *texto)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),FIM "|%s\n",texto);

    return enviar_mensagem(socket, mensagem);
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

    //Falhou criação
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
    #ifdef _WIN32
    if (setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEADDR,(const char *)&opcao,sizeof(opcao)) < 0) {
    #else
    if (setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEADDR,&opcao,sizeof(opcao)) < 0) {
    #endif

    perror("setsockopt");
    FECHAR_SOCKET(socket_servidor);
    return -1;
}

    // Estrutura utilizada para armazenar o endereço do servidor.
    struct sockaddr_in endereco;

    // Inicializa toda a estrutura com zero.
    memset(&endereco, 0, sizeof(endereco));

    // Define que o endereço utiliza IPv4.
    endereco.sin_family = AF_INET;
    // Permite que o servidor aceite conexões através de qualquer
    // endereço de rede disponível na máquina.
    endereco.sin_addr.s_addr = INADDR_ANY;
    // Define a porta utilizada pelo servidor.
    // htons() converte o número da porta para o formato
    // de bytes utilizado pela rede.
    endereco.sin_port = htons(porta);

    // Associa o socket ao endereço e à porta configurados.
    // bind() faz o socket "pertencer" àquela porta/endereço.
    if (bind(socket_servidor,(struct sockaddr *)&endereco,sizeof(endereco)) < 0) {
        perror("bind");
        FECHAR_SOCKET(socket_servidor);
        return -1;
    }

    // Coloca o socket em modo de escuta.
    // A partir daqui, o servidor fica aguardando conexões
    // de clientes.
    // MAX_CLIENTES define quantas conexões podem ficar
    // aguardando na fila de conexões (nesse caso duas).
    if (listen(socket_servidor, MAX_CLIENTES) < 0) {
        perror("listen");
        FECHAR_SOCKET(socket_servidor);
        return -1;
    }
    return socket_servidor;
}

/*
 * Trata signal de interrupção (Ctrl+C).
 */
void tratar_interrupcao(int sinal)
{
    (void)sinal;

    executando = 0;

    if (servidor_socket != -1) {
        FECHAR_SOCKET(servidor_socket);
    }
}

/*
 * Thread responsável por receber o nome do jogador.
 */
void *atender_jogador(void *arg)
{
    int indice = *(int *)arg;

    free(arg);

    char buffer[TAM_MENSAGEM];

    int resultado = receber_mensagem(jogadores[indice].socket,buffer,sizeof(buffer));

    // Desconexão ou erro de comunicação
    if (resultado <= 0) {

        printf("[-] Jogador desconectou.\n");

        FECHAR_SOCKET(jogadores[indice].socket);

        pthread_mutex_lock(&mutex_jogadores);

        jogadores[indice].conectado = 0;
        jogadores_conectados--;

        pthread_mutex_unlock(&mutex_jogadores);

        return NULL;
    }

    /*
     * Formato:
     *
     * NOME|João
     */
    if (strncmp(buffer,NOME "|",strlen(NOME) + 1) == 0) {

        char *nome = buffer + strlen(NOME) + 1;

        remover_quebra_linha(nome);

        /*
         * O acesso a nomes_recebidos precisa
         * ser protegido pelo mutex.
         */
        pthread_mutex_lock(&mutex_jogadores);

        strncpy(jogadores[indice].nome,nome,sizeof(jogadores[indice].nome) - 1);

        jogadores[indice].nome[sizeof(jogadores[indice].nome) - 1] = '\0';


        nomes_recebidos++;

        /*
         * Avisa a thread que estiver esperando
         * que um novo nome foi recebido.
         */
        pthread_cond_broadcast(&cond_nomes);

        pthread_mutex_unlock(&mutex_jogadores);

        printf("[+] Jogador %d: %s\n",indice + 1,jogadores[indice].nome);

        enviar_msg(jogadores[indice].socket,"Bem-vindo ao jogo!");

        /*
         * Se ainda não temos os dois jogadores,
         * informa que este jogador deve aguardar.
         */
        
         int aguardar = 0;


         pthread_mutex_lock(&mutex_jogadores);

         if(jogadores_conectados < MAX_CLIENTES) {
            aguardar = 1;
        }

        pthread_mutex_unlock(&mutex_jogadores);

        if (aguardar) {
            enviar_aguarde(jogadores[indice].socket,"Aguardando outro jogador...");
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    //-----------------------------------------------------------------------
    //Variáveis locais ------------------------------------------------------
    int porta = PORTA; //7070 padrão
    pthread_t thread;
    srand((unsigned int)time(NULL));

    //-----------------------------------------------------------------------
    //Argumentos ------------------------------------------------------------

    #ifdef _WIN32
    WSADATA dados_winsock;

    if (WSAStartup(MAKEWORD(2, 2), &dados_winsock) != 0) {
        fprintf(stderr, "Erro ao inicializar Winsock.\n");
        return EXIT_FAILURE;
    }
    #endif

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
    //Menu inicial-----------------------------------------------------------
    printf("========================================\n");
    printf("     BATALHA DE PALAVRAS - SERVIDOR\n");
    printf("========================================\n");
    printf("Porta: %d\n", porta);
    printf("Aguardando %d jogadores...\n\n", MAX_CLIENTES);

    //-----------------------------------------------------------------------
    //Criando servidor ------------------------------------------------------
    servidor_socket = criar_servidor(porta);

    if (servidor_socket < 0) {
        #ifdef _WIN32
        WSACleanup();
        #endif
        return EXIT_FAILURE;
    }

    //-----------------------------------------------------------------------
    //inicialização estrutura do jogador ------------------------------------
    for (int i = 0; i < MAX_CLIENTES; i++) {
    jogadores[i].socket = -1;
    jogadores[i].nome[0] = '\0';
    jogadores[i].conectado = 0;
    jogadores[i].pontuacao = 0;
}

    while (executando && jogadores_conectados < MAX_CLIENTES) {
        struct sockaddr_in endereco_cliente;
        #ifdef _WIN32
        int tamanho = sizeof(endereco_cliente);
        #else
        socklen_t tamanho = sizeof(endereco_cliente);
        #endif
        // Aceitar conexões de clientes.
        int socket_cliente = accept(servidor_socket,(struct sockaddr *)&endereco_cliente,&tamanho);

        if (socket_cliente < 0) {

            if (!executando) {
                break;
            }

            perror("accept");
            continue;
        }

        // Cria um vetor de caracteres para armazenar o endereço IP 
        char ip[INET_ADDRSTRLEN];

        /*Converte o endereço IP armazenado em endereco_cliente.sin_addr
         * de formato binário para uma string.
         *
         *AF_INET:
         *Indica IPv4.
         *
         *&endereco_cliente.sin_addr:
         *Endereço IPv4 do cliente.
         *
         *ip:
         *Vetor onde o endereço convertido será armazenado.
         *
         *sizeof(ip):
         *Tamanho máximo disponível no vetor ip.
         */
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

            enviar_mensagem(socket_cliente,SERVIDOR_CHEIO "\n");

            FECHAR_SOCKET(socket_cliente);
            continue;
        }

        jogadores[indice].socket = socket_cliente;
        jogadores[indice].conectado = 1;

        jogadores_conectados++;

        /*
         * Avisa o cliente para enviar o nome.
         */
        enviar_solicitacao_nome(socket_cliente);

        pthread_mutex_unlock(&mutex_jogadores);

        /*
         * Cria uma thread para atender esse cliente.
         */

        int *arg = malloc(sizeof(int));

        if (arg == NULL) {
            perror("malloc");

            FECHAR_SOCKET(socket_cliente);

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
            FECHAR_SOCKET(socket_cliente);
            pthread_mutex_lock(&mutex_jogadores);
            jogadores[indice].conectado = 0;
            jogadores_conectados--;
            pthread_mutex_unlock(&mutex_jogadores);

            continue;
        }

        pthread_detach(thread);
    }

    //-----------------------------------------------------------------------
    //Esperar pelos jogadores e começar partida -----------------------------
    if (executando && jogadores_conectados == MAX_CLIENTES) {

        pthread_mutex_lock(&mutex_jogadores);

        while (nomes_recebidos < MAX_CLIENTES && executando) {
            pthread_cond_wait(&cond_nomes,&mutex_jogadores);
        }

        pthread_mutex_unlock(&mutex_jogadores);

        if (executando) {
            iniciar_partida();
        }
    }

    //-----------------------------------------------------------------------
    //Fechar servidor -------------------------------------------------------
    printf("\n[*] Servidor encerrado.\n");

    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (jogadores[i].socket != -1) {
            FECHAR_SOCKET(jogadores[i].socket);
        }
    }

    if (servidor_socket != -1) {
        FECHAR_SOCKET(servidor_socket);
    }

    pthread_mutex_destroy(&mutex_jogadores);

    #ifdef _WIN32
    WSACleanup();
    #endif

    return EXIT_SUCCESS;
}
