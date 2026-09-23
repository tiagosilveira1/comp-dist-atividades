/*
INTEGRANTES DO GRUPO:
Aluno: Tiago Silveira Lopes, RA: 10417600
*/

#ifndef PROTOCOLO_H
#define PROTOCOLO_H

/*
 * Configurações do servidor.
 */
#define PORTA 7070
#define MAX_CLIENTES 2

/*
 * Configurações do jogo.
 */
#define NUM_RODADAS 5
#define TEMPO_RODADA 10
#define MIN_CARACTERES 5

/*
 * Tamanhos dos dados.
 */
#define TAM_NOME 50
#define TAM_PALAVRA 100
#define TAM_MENSAGEM 256

/*
 * Mensagens do protocolo.
 *
 */
#define MSG "MSG"
#define NOME "NOME"
#define AGUARDE "AGUARDE"
#define RODADA "RODADA"
#define PALAVRA "PALAVRA"
#define TIMEOUT "TIMEOUT"
#define RESULTADO "RESULTADO"
#define PLACAR "PLACAR"
#define FIM "FIM"
#define SERVIDOR_CHEIO "FULL"

#endif
