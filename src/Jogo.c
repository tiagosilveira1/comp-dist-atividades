#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#endif

#include "jogo.h"


/*
 * Envia todos os bytes da mensagem.
 *
 * A função send() pode enviar apenas uma parte
 * da mensagem. Por isso, continuamos enviando
 * até que todos os bytes sejam enviados.
 */
int enviar_mensagem(int socket, const char *mensagem)
{
    int tamanho;
    int enviados = 0;
    int resultado;

    tamanho = strlen(mensagem);

    while (enviados < tamanho) {

        resultado = send(
            socket,
            mensagem + enviados,
            tamanho - enviados,
            0
        );

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
 * O protocolo utiliza '\n' como final da mensagem.
 *
 * Portanto, recebemos caractere por caractere
 * até encontrar '\n'.
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
 * Recebe uma mensagem com limite de tempo.
 *
 * select() verifica se existe alguma informação
 * disponível no socket antes de chamar recv().
 */
int receber_com_timeout(
    int socket,
    char *mensagem,
    int tamanho,
    int segundos
)
{
    fd_set conjunto;
    struct timeval tempo;
    int resultado;

    FD_ZERO(&conjunto);
    FD_SET(socket, &conjunto);

    tempo.tv_sec = segundos;
    tempo.tv_usec = 0;

    resultado = select(
        socket + 1,
        &conjunto,
        NULL,
        NULL,
        &tempo
    );

    if (resultado == 0) {
        /*
         * Tempo acabou.
         */
        return 0;
    }

    if (resultado < 0) {
        return -1;
    }

    /*
     * Há dados disponíveis no socket.
     */
    return receber_mensagem(
        socket,
        mensagem,
        tamanho
    );
}


/*
 * Valida uma palavra de acordo com as regras
 * da atividade.
 */
int validar_palavra(const char *palavra, char letra)
{
    int tamanho;
    int i;

    if (palavra == NULL) {
        return 0;
    }

    tamanho = strlen(palavra);

    /*
     * A palavra precisa ter pelo menos
     * MIN_CARACTERES caracteres.
     */
    if (tamanho < MIN_CARACTERES) {
        return 0;
    }

    /*
     * A primeira letra deve ser a letra
     * sorteada para a rodada.
     *
     * Usamos toupper() para ignorar maiúsculas
     * e minúsculas.
     */
    if (toupper((unsigned char)palavra[0]) !=
        toupper((unsigned char)letra)) {

        return 0;
    }

    /*
     * Todos os caracteres precisam ser letras.
     */
    for (i = 0; i < tamanho; i++) {

        if (!isalpha((unsigned char)palavra[i])) {
            return 0;
        }
    }

    return 1;
}


/*
 * Gera uma letra aleatória entre A e Z.
 */
char gerar_letra(void)
{
    return 'A' + rand() % 26;
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

    while (tamanho > 0 &&
           (texto[tamanho - 1] == '\n' ||
            texto[tamanho - 1] == '\r')) {

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

    snprintf(
        mensagem,
        sizeof(mensagem),
        MSG "|%s\n",
        texto
    );

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

    snprintf(
        mensagem,
        sizeof(mensagem),
        NOME "|\n"
    );

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

    snprintf(
        mensagem,
        sizeof(mensagem),
        AGUARDE "|%s\n",
        texto
    );

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * RODADA|numero|letra|tempo\n
 */
int enviar_rodada(
    int socket,
    int numero,
    char letra,
    int tempo
)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(
        mensagem,
        sizeof(mensagem),
        RODADA "|%d|%c|%d\n",
        numero,
        letra,
        tempo
    );

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

    snprintf(
        mensagem,
        sizeof(mensagem),
        RESULTADO "|%s\n",
        texto
    );

    return enviar_mensagem(socket, mensagem);
}


/*
 * Envia:
 *
 * PLACAR|nome1|pontos1|nome2|pontos2\n
 */
int enviar_placar(
    int socket,
    const char *nome1,
    int pontos1,
    const char *nome2,
    int pontos2
)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(
        mensagem,
        sizeof(mensagem),
        PLACAR "|%s|%d|%s|%d\n",
        nome1,
        pontos1,
        nome2,
        pontos2
    );

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

    snprintf(
        mensagem,
        sizeof(mensagem),
        FIM "|%s\n",
        texto
    );

    return enviar_mensagem(socket, mensagem);
}