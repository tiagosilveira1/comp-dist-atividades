/*
INTEGRANTES DO GRUPO:
Aluno: Tiago Silveira Lopes, RA: 10417600
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#endif
#ifdef _WIN32
#define FECHAR_SOCKET closesocket
#else
#define FECHAR_SOCKET close
#endif

#include "protocolo.h"


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

int receber_mensagem(int socket, char *mensagem, int tamanho)
{
    int posicao = 0;
    char caractere;
    int resultado;

    if (tamanho <= 1) {
        return -1;
    }

    while (posicao < tamanho - 1) {

        resultado = recv(socket,&caractere,1,0);

        if (resultado == 0) {
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
 * Cria a conexão com o servidor.
 */
int conectar_servidor(const char *ip, int porta)
{
    #ifdef _WIN32
    SOCKET socket_cliente;
    #else
    int socket_cliente;
    #endif

    socket_cliente = socket(AF_INET,SOCK_STREAM,0);

    #ifdef _WIN32 
    if (socket_cliente == INVALID_SOCKET) { 
        fprintf( stderr, "Erro em socket(): %d\n", WSAGetLastError() ); 
        return -1; 
    } 
    #else 
    if (socket_cliente < 0) 
    { 
        perror("socket"); 
        return -1; 
    } 
    #endif

    struct sockaddr_in endereco;

    memset(&endereco, 0, sizeof(endereco));

    endereco.sin_family = AF_INET;
    endereco.sin_port = htons(porta);

    if (inet_pton(AF_INET,ip,&endereco.sin_addr) <= 0) {

        fprintf(stderr, "Endereço IP inválido.\n");
        FECHAR_SOCKET(socket_cliente);
        return -1;
    }

    if (connect(socket_cliente,(struct sockaddr *)&endereco,sizeof(endereco)) < 0) {

        #ifdef _WIN32 
        fprintf( stderr, "Erro em connect(): %d\n", WSAGetLastError() ); 
        #else 
        perror("connect"); 
        #endif
        FECHAR_SOCKET(socket_cliente);
        return -1;
    }

    return socket_cliente;
}


/*
 * Lê uma linha do teclado utilizando select().
 *
 * Retorna:
 *  1  -> usuário digitou algo
 *  0  -> tempo esgotado
 * -1  -> erro
 */
int ler_com_timeout(char *buffer, int tamanho, int segundos)
{

#ifdef _WIN32

    int posicao = 0;

    DWORD inicio;
    DWORD agora;
    DWORD tempo_limite;

    inicio = GetTickCount();

    tempo_limite = (DWORD)segundos * 1000;

    while (1) {

        agora = GetTickCount();

        if (agora - inicio >= tempo_limite) {

            buffer[posicao] = '\0';

            return 0;
        }

        if (_kbhit()) {

            int caractere = _getch();

            /*
             * ENTER
             */
            if (caractere == '\r' || caractere == '\n') {

                buffer[posicao] = '\0';

                printf("\n");

                if (posicao == 0) {
                    return 0;
                }

                return 1;
            }

            /*
             * BACKSPACE
             */
            if (caractere == '\b') {

                if (posicao > 0) {

                    posicao--;

                    printf("\b \b");
                }

                continue;
            }

            /*
             * Caracteres normais.
             */
            if (isprint((unsigned char)caractere)) {

                if (posicao < tamanho - 1) {

                    buffer[posicao] = (char)caractere;

                    posicao++;

                    putchar(caractere);

                    fflush(stdout);
                }
            }
        }

        /*
         * Evita consumir 100% da CPU.
         */
        Sleep(10);
    }

#else

    fd_set conjunto;
    struct timeval tempo;

    FD_ZERO(&conjunto);

    FD_SET(STDIN_FILENO, &conjunto);

    tempo.tv_sec = segundos;
    tempo.tv_usec = 0;

    int resultado = select(
        STDIN_FILENO + 1,
        &conjunto,
        NULL,
        NULL,
        &tempo
    );

    if (resultado < 0) {

        perror("select");

        return -1;
    }

    if (resultado == 0) {
        return 0;
    }

    if (fgets(buffer, tamanho, stdin) == NULL) {
        return -1;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';

    return 1;

#endif
}


/*
 * Exibe uma mensagem recebida do servidor.
 */
void processar_mensagem(int socket,char *mensagem)
{
    /*
     * MSG|texto
     */
    if (strncmp(mensagem, MSG "|", strlen(MSG) + 1) == 0) {

        char *texto = mensagem + strlen(MSG) + 1;

        printf("\n%s\n", texto);
    }

    /*
     * NOME|
     *
     * O servidor está solicitando o nome.
     */
    else if (
        strncmp(mensagem,NOME "|",strlen(NOME) + 1) == 0) {

        char nome[TAM_NOME];

        printf("\nDigite seu nome: ");

        if (fgets(nome, sizeof(nome), stdin) == NULL) {
            return;
        }

        nome[strcspn(nome, "\r\n")] = '\0';

        char resposta[TAM_MENSAGEM];

        snprintf(resposta,sizeof(resposta),NOME "|%s\n",nome);

        enviar_mensagem(socket, resposta);
    }

    /*
     * AGUARDE|texto
     */
    else if (strncmp(mensagem,AGUARDE "|",strlen(AGUARDE) + 1) == 0) {

        char *texto = mensagem + strlen(AGUARDE) + 1;

        printf("\n%s\n", texto);
    }

    /*
     * RODADA|numero|letra|tempo
     */
    else if (strncmp(mensagem,RODADA "|",strlen(RODADA) + 1) == 0) {

        int numero;
        char letra;
        int tempo;

        sscanf(mensagem,RODADA "|%d|%c|%d",&numero,&letra,&tempo);

        printf("\n");
        printf("========================================\n");
        printf("           RODADA %d de %d\n",numero,NUM_RODADAS);
        printf("========================================\n");
        printf("Letra: [%c]\n", letra);
        printf("Tempo: %d segundos\n", tempo);
        printf("Mínimo: %d caracteres\n", MIN_CARACTERES);
        printf("Sua palavra: ");

        char palavra[TAM_PALAVRA];

        int resultado = ler_com_timeout(palavra,sizeof(palavra),tempo);

        if (resultado == 1) {

            char resposta[TAM_MENSAGEM];

            snprintf(resposta,sizeof(resposta),PALAVRA "|%s\n",palavra);

            enviar_mensagem(socket,resposta);

            printf("Enviado: \"%s\"\n",palavra);
        }
        else if (resultado == 0) {

            printf("\nTempo esgotado!\n");

            enviar_mensagem(socket,TIMEOUT "|\n");
        }
        else {
            printf("\nErro ao ler palavra.\n");
        }
    }

    /*
     * RESULTADO|texto
     */
    else if (strncmp(mensagem,RESULTADO "|",strlen(RESULTADO) + 1) == 0) {

        char *texto = mensagem + strlen(RESULTADO) + 1;

        printf("\nResultado: %s\n", texto);
    }

    /*
     * PLACAR|nome1|pts1|nome2|pts2
     */
    else if (strncmp(mensagem,PLACAR "|",strlen(PLACAR) + 1) == 0) {

        char nome1[TAM_NOME];
        char nome2[TAM_NOME];

        int pontos1;
        int pontos2;

        if (sscanf(mensagem,PLACAR "|%49[^|]|%d|%49[^|]|%d",nome1,&pontos1,nome2,&pontos2) != 4) {
            printf("\nMensagem de placar inválida.\n");
            return;
        }

        printf("\n");
        printf("----------------------------------------\n");
        printf("PLACAR: %s %d x %d %s\n",nome1,pontos1,pontos2,nome2);
        printf("----------------------------------------\n");
    }

    /*
     * FIM|texto
     */
    else if (strncmp(mensagem,FIM "|",strlen(FIM) + 1) == 0) {
        char *texto =
            mensagem + strlen(FIM) + 1;

        printf("\n========================================\n");
        printf("              FIM DO JOGO\n");
        printf("========================================\n");
        printf("%s\n", texto);
        printf("========================================\n");
    }

    else if (strcmp(mensagem, SERVIDOR_CHEIO) == 0) {
    printf("\nServidor cheio.\n");
}

    /*
     * Mensagem desconhecida.
     */
    else {
        printf("\nMensagem desconhecida: %s\n", mensagem);
    }
}


int main(int argc, char *argv[])
{

    #ifdef _WIN32
    WSADATA dados_winsock;

    if (WSAStartup(MAKEWORD(2, 2), &dados_winsock) != 0) {
        fprintf(stderr, "Erro ao inicializar Winsock.\n");
        return EXIT_FAILURE;
    }
    #endif
    /*
     * Valores padrão:
     *
     * IP   = 127.0.0.1
     * porta = 7070
     */
    const char *ip = "127.0.0.1";
    int porta = PORTA;

    /*
     * ./cliente
     */
    if (argc == 1) {
        /* usa valores padrão */
    }

    /*
     * ./cliente 127.0.0.1 9000
     */
    else if (argc == 3) {

        ip = argv[1];
        porta = atoi(argv[2]);

        if (porta <= 0 || porta > 65535) {
            fprintf(stderr, "Porta inválida.\n");
            #ifdef _WIN32
            WSACleanup();
            #endif
            return EXIT_FAILURE;
        }
    }

    else {
        fprintf(stderr,"Uso: %s [IP PORTA]\n",argv[0]);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return EXIT_FAILURE;
    }


    printf("========================================\n");
    printf("       BATALHA DE PALAVRAS - CLIENTE\n");
    printf("========================================\n");

    printf("Conectando a %s:%d...\n",ip,porta);


    /*
     * Conecta ao servidor.
     */
    #ifdef _WIN32
    SOCKET socket_cliente = conectar_servidor(ip,porta);
    #else
    int socket_cliente = conectar_servidor(ip,porta);
    #endif

    #ifdef _WIN32
    if (socket_cliente == INVALID_SOCKET) {  
        WSACleanup();
        return EXIT_FAILURE;
    }
    #else
    if (socket_cliente < 0) {
        return EXIT_FAILURE;
    }
    #endif

    printf("Conectado!\n");


    /*
     * Loop principal:
     *
     * fica esperando mensagens do servidor.
     */
    while (1) {

        char mensagem[TAM_MENSAGEM];

        int resultado = receber_mensagem(socket_cliente,mensagem,sizeof(mensagem));


        if (resultado == 0) {
            printf("\nServidor encerrou a conexão.\n");
            break;
        }

        if (resultado < 0) {
            perror("recv");
            break;
        }

        /*
         * Processa a mensagem recebida.
         */
        processar_mensagem(socket_cliente,mensagem);

        /*
         * Se recebeu FIM, encerra.
         */
        if (strncmp(mensagem,FIM "|",strlen(FIM) + 1) == 0) {break;}
    }


    FECHAR_SOCKET(socket_cliente);

    printf("\nCliente encerrado.\n");
    #ifdef _WIN32
    WSACleanup();
    #endif
    return EXIT_SUCCESS;
}
