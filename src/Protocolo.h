#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#define PORTA 7070
#define MAX_JOGADORES 2
#define NUM_RODADAS 5
#define TEMPO_RODADA 10

#define TAM_MENSAGEM 256

#define MSG_MSG           "MSG"
#define MSG_AGUARDE       "AGUARDE"
#define MSG_NOME          "NOME"
#define MSG_RODADA        "RODADA"
#define MSG_RESULTADO     "RESULT"
#define MSG_PLACAR        "PLACAR"
#define MSG_FIM           "FIM"
#define MSG_SAIR          "QUIT"

#define MSG_CS_NOME       "NOME"
#define MSG_CS_PALAVRA    "PALAVRA"
#define MSG_CS_TIMEOUT    "TIMEOUT"

#define SEPARADOR "|"

#endif