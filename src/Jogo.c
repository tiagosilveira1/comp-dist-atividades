/*
INTEGRANTES DO GRUPO:
Aluno: Tiago Silveira Lopes, RA: 10417600
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include "jogo.h"


/*
 * Aguarda as respostas dos dois jogadores simultaneamente.
 *
 * Os dois jogadores possuem o mesmo limite de tempo.
 *
 * O select() é utilizado antes de cada leitura para garantir
 * que o recv() somente seja chamado quando houver dados
 * disponíveis no socket.
 *
 * Isso evita que receber_mensagem() fique bloqueada esperando
 * o restante de uma mensagem TCP que ainda não chegou.
 *
 * Retorna em resultado1 e resultado2:
 *  1 -> jogador enviou uma mensagem completa
 *  0 -> tempo esgotado
 * -1 -> erro ou desconexão
 */
void receber_respostas_rodada(char *resposta1,int tamanho1,char *resposta2,int tamanho2,int *resultado1,int *resultado2)
{
    fd_set conjunto;
    struct timeval tempo;

    time_t inicio;
    time_t atual;

    int segundos_passados;
    int segundos_restantes;

    int recebeu1 = 0;
    int recebeu2 = 0;

    int posicao1 = 0;
    int posicao2 = 0;

    /*
     * Inicialmente nenhum jogador possui
     * uma resposta completa.
     */
    *resultado1 = 0;
    *resultado2 = 0;

    inicio = time(NULL);

    /*
     * Continua enquanto pelo menos um jogador
     * ainda não terminou de enviar sua resposta.
     */
    while (!recebeu1 || !recebeu2) {

        /*
         * Calcula quanto tempo passou desde
         * o início da rodada.
         */
        atual = time(NULL);

        segundos_passados = (int)(atual - inicio);

        segundos_restantes = TEMPO_RODADA - segundos_passados;

        /*
         * O tempo da rodada terminou.
         */
        if (segundos_restantes <= 0) {
            break;
        }

        FD_ZERO(&conjunto);

        if (!recebeu1) {
            FD_SET(jogadores[0].socket, &conjunto);
        }

        if (!recebeu2) {
            FD_SET(jogadores[1].socket, &conjunto);
        }

        /*
         * Tempo máximo de espera
         */
        tempo.tv_sec = segundos_restantes;
        tempo.tv_usec = 0;

        int maior_socket = jogadores[0].socket;

        if (jogadores[1].socket > maior_socket) {
            maior_socket = jogadores[1].socket;
        }

        /*
         * Espera até:
         *
         * - jogador 1 enviar dados;
         * - jogador 2 enviar dados;
         * - ou o tempo acabar.
         */
        int resultado = select(maior_socket + 1,&conjunto,NULL,NULL,&tempo);

        /*
         * Erro no select().
         */
        if (resultado < 0) {
            *resultado1 = -1;
            *resultado2 = -1;
            return;
        }

        /*
         * Nenhum socket ficou disponível
         * antes do fim do tempo.
         */
        if (resultado == 0) {
            break;
        }

        /*
         * =====================================================
         * JOGADOR 1
         * =====================================================
         */

        if (!recebeu1 && FD_ISSET(jogadores[0].socket, &conjunto)) {

            char caractere;
            int recebido = recv(jogadores[0].socket,&caractere,1,0);

            /*
             * Jogador desconectou.
             */
            if (recebido == 0) {
                *resultado1 = -1;
                recebeu1 = 1;
            }

            /*
             * Erro de comunicação.
             */
            else if (recebido < 0) {
                *resultado1 = -1;
                recebeu1 = 1;
            }

            /*
             * Recebeu um byte.
             */
            else {

                /*
                 * Verifica se é o final da mensagem.
                 */
                if (caractere == '\n') {
                    resposta1[posicao1] = '\0';
                    *resultado1 = 1;
                    recebeu1 = 1;
                }

                /*
                 * Ainda não chegou ao final
                 * da mensagem.
                 */
                else if (posicao1 < tamanho1 - 1) {
                    resposta1[posicao1] = caractere;
                    posicao1++;
                }

                /*
                 * Mensagem maior que o buffer.
                 */
                else {
                    resposta1[tamanho1 - 1] = '\0';
                    *resultado1 = -1;
                    recebeu1 = 1;
                }
            }
        }

        /*
         * =====================================================
         * JOGADOR 2
         * =====================================================
         */

        if (!recebeu2 && FD_ISSET(jogadores[1].socket, &conjunto)) {

            char caractere;

            /*
             * Lê somente um byte.
             */
            int recebido = recv(jogadores[1].socket,&caractere,1,0);

            /*
             * Jogador desconectou.
             */
            if (recebido == 0) {
                *resultado2 = -1;
                recebeu2 = 1;
            }

            /*
             * Erro de comunicação.
             */
            else if (recebido < 0) {
                *resultado2 = -1;
                recebeu2 = 1;
            }

            /*
             * Recebeu um byte.
             */
            else {

                /*
                 * Final da mensagem.
                 */
                if (caractere == '\n') {
                    resposta2[posicao2] = '\0';
                    *resultado2 = 1;
                    recebeu2 = 1;
                }

                /*
                 * Ainda está recebendo a mensagem.
                 */
                else if (posicao2 < tamanho2 - 1) {
                    resposta2[posicao2] = caractere;
                    posicao2++;
                }

                /*
                 * Mensagem maior que o buffer.
                 */
                else {
                    resposta2[tamanho2 - 1] = '\0';
                    *resultado2 = -1;
                    recebeu2 = 1;
                }
            }
        }
    }
}

/*
 * Compara duas strings ignorando maiúsculas e minúsculas.
 *
 * Retorna:
 *  1 -> strings iguais
 *  0 -> strings diferentes
 *
 * Substitui strcasecmp(), que não é padrão C.
 */
int strings_iguais_ignore_case(const char *texto1, const char *texto2)
{
    int i = 0;

    if (texto1 == NULL || texto2 == NULL) {
        return 0;
    }

    while (texto1[i] != '\0' && texto2[i] != '\0') {

        if (tolower((unsigned char)texto1[i]) != tolower((unsigned char)texto2[i])) {
            return 0;
        }
        i++;
    }

    return texto1[i] == '\0' && texto2[i] == '\0';
}

/*
 * Inicia a partida:
 *
 * Envia a mensagem de início para os jogadores e executa as rodadas.
 */
void iniciar_partida(void)
{
    char mensagem[TAM_MENSAGEM];

    snprintf(mensagem,sizeof(mensagem),MSG "|Partida iniciada: %s vs %s\n",jogadores[0].nome,jogadores[1].nome);

    enviar_para_jogadores(mensagem);

    for (int rodada = 1; rodada <= NUM_RODADAS; rodada++) {
        executar_rodada(rodada);
    }


    /*
     * Todas as rodadas terminaram.
     * Informa aos jogadores que a partida acabou.
     */
    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (jogadores[i].conectado) {
            enviar_fim(jogadores[i].socket,"Fim de jogo.");
        }
    }
}

void executar_rodada(int numero)
{
    char letra;
    char resposta1[TAM_MENSAGEM];
    char resposta2[TAM_MENSAGEM];

    char palavra1[TAM_PALAVRA];
    char palavra2[TAM_PALAVRA];

    int resultado1;
    int resultado2;

    int valida1 = 0;
    int valida2 = 0;

    /*
     * Sorteia a letra da rodada.
     *
     */
    letra = gerar_letra();

    printf("\n[Rodada %d] Letra: %c\n",numero,letra);

    /*
     * Envia a rodada para os dois jogadores.
     *
     * enviar_rodada() está implementada em jogo.c.
     */
    for (int i = 0; i < MAX_CLIENTES; i++) {
        if (jogadores[i].conectado) {
            enviar_rodada(jogadores[i].socket,numero,letra,TEMPO_RODADA);
        }
    }

    /*
 * Aguarda as respostas dos dois jogadores
 * durante o mesmo período de tempo.
 */
    receber_respostas_rodada(resposta1,sizeof(resposta1),resposta2,sizeof(resposta2),&resultado1,&resultado2);

    /*
     * Analisa a resposta do jogador 1.
     */
    if (resultado1 == 1) {
        /*
         * Esperamos:
         *
         * PALAVRA|banana
         */
        if (strncmp(resposta1,PALAVRA "|",strlen(PALAVRA) + 1) == 0) {

            char *palavra =
                resposta1 + strlen(PALAVRA) + 1;

            strncpy(palavra1,palavra,sizeof(palavra1) - 1);

            palavra1[sizeof(palavra1) - 1] = '\0';

            remover_quebra_linha(palavra1);

            /*
             * Verifica se a palavra é válida.
             */
            valida1 = validar_palavra(palavra1,letra);
        }
    }


    /*
     * Analisa a resposta do jogador 2.
     */
    if (resultado2 == 1) {
        /*
         * Esperamos:
         *
         * PALAVRA|banana
         */
        if (strncmp(resposta2,PALAVRA "|",strlen(PALAVRA) + 1) == 0) {
            char *palavra = resposta2 + strlen(PALAVRA) + 1;
            strncpy(palavra2,palavra,sizeof(palavra2) - 1);
            palavra2[sizeof(palavra2) - 1] = '\0';
            remover_quebra_linha(palavra2);
            /*
             * Verifica se a palavra é válida.
             */
            valida2 = validar_palavra(palavra2,letra);
        }
    }


    /*
     * Jogadores enviaram as mesmas palavras
     */
    int palavras_iguais = 0;

    if (valida1 && valida2) {
        if (strings_iguais_ignore_case(palavra1, palavra2)) {
            palavras_iguais = 1;
        }
    }


    /*
     * Atualiza a pontuação.
     */
    if (!palavras_iguais) {
        if (valida1) {
            jogadores[0].pontuacao++;
        }

        if (valida2) {
            jogadores[1].pontuacao++;
        }
    }


    /*
     * Envia o resultado para cada jogador.
     */
    char mensagem_resultado[TAM_MENSAGEM];
    /*
     * Resultado do jogador 1.
     */
    if (resultado1 == -1) {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Jogador desconectado.");
}   else if (resultado1 == 0) {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Tempo esgotado.");
    } else if (!valida1) {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Palavra invalida.");

    } else if (palavras_iguais) {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Palavras iguais. Nenhum jogador ganhou ponto.");

    } else {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Palavra valida! +1 ponto.");
    }

    enviar_resultado(jogadores[0].socket,mensagem_resultado);


    /*
     * Resultado do jogador 2.
     */
    if (resultado2 == -1) {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Jogador desconectado.");
    } else if (resultado2 == 0) {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Tempo esgotado.");

    } else if (!valida2) {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Palavra invalida.");

    } else if (palavras_iguais) {

        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Palavras iguais. Nenhum jogador ganhou ponto.");

    } else {
        snprintf(mensagem_resultado,sizeof(mensagem_resultado),"Palavra valida! +1 ponto.");
    }

    enviar_resultado(jogadores[1].socket,mensagem_resultado);


    /*
     * Mostra o placar no servidor.
     */
    printf("Placar: %s = %d | %s = %d\n",jogadores[0].nome,jogadores[0].pontuacao,jogadores[1].nome,jogadores[1].pontuacao);

    /*
     * Envia o placar atualizado para os dois jogadores.
     */
    for (int i = 0; i < MAX_CLIENTES; i++) {

        if (jogadores[i].conectado) {
            enviar_placar(jogadores[i].socket,jogadores[0].nome,jogadores[0].pontuacao,jogadores[1].nome,jogadores[1].pontuacao);
        }
    }
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
