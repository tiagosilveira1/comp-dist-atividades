/*
INTEGRANTES DO GRUPO:
Aluno: Tiago Silveira Lopes, RA: 10417600
*/

/*
Funções compartilhadas entre Servidor.c e Jogo.c.
*/

#ifndef JOGO_H
#define JOGO_H

#include <pthread.h>
#include "protocolo.h"

typedef struct {
    int socket;
    char nome[TAM_NOME];
    int conectado;
    int pontuacao;
} Jogador;

extern Jogador jogadores[MAX_CLIENTES];

extern pthread_mutex_t mutex_jogadores;

/*
 * Aguarda as respostas dos dois jogadores
 * durante o mesmo período de tempo.
 */
void receber_respostas_rodada(char *resposta1,int tamanho1,char *resposta2,int tamanho2,int *resultado1,int *resultado2);

/*
 * Iniciar jogo.
 */
void iniciar_partida(void);

/*
 * Executa rodada do jogo.
 */
void executar_rodada(int numero);

/*
 * Envia uma mensagem completa pelo socket.
 */
int enviar_mensagem(int socket, const char *mensagem);

/*
 * Recebe uma mensagem terminada por '\n'.
 */
int receber_mensagem(int socket, char *mensagem, int tamanho);

/*
 * Verifica se uma palavra é válida para a rodada.
 *
 * Regras:
 * - começa com a letra indicada;
 * - possui pelo menos MIN_CARACTERES;
 * - contém somente letras de A-Z/a-z.
 *
 * Retorna 1 se válida e 0 se inválida.
 */
int validar_palavra(const char *palavra, char letra);

/*
 * Gera uma letra aleatória de A-Z.
 */
char gerar_letra(void);

/*
 * Remove '\n' e '\r' do final de uma string.
 */
void remover_quebra_linha(char *texto);

/*
 * Envia uma mensagem usando o protocolo:
 *
 * MSG|texto
 */
int enviar_msg(int socket, const char *texto);

/*
 * Solicita o nome do jogador:
 *
 * NOME|
 */
int enviar_solicitacao_nome(int socket);

/*
 * Informa ao jogador que ele deve aguardar:
 *
 * AGUARDE|texto
 */
int enviar_aguarde(int socket, const char *texto);

/*
 * Envia as informações de uma rodada:
 *
 * RODADA|numero|letra|tempo
 */
int enviar_rodada(int socket, int numero, char letra, int tempo);

/*
 * Envia o resultado de uma rodada:
 *
 * RESULTADO|texto
 */
int enviar_resultado(int socket, const char *texto);

/*
 * Envia o placar:
 *
 * PLACAR|nome1|pontos1|nome2|pontos2
 */
int enviar_placar(
    int socket,
    const char *nome1,
    int pontos1,
    const char *nome2,
    int pontos2
);

/*
 * Envia uma mensagem para todos os jogadores conectados.
 */
void enviar_para_jogadores(const char *mensagem);

/*
 * Envia o encerramento da partida:
 *
 * FIM|texto
 */
int enviar_fim(int socket, const char *texto);

#endif
