/* Autor: Elias P. Duarte Jr.
   Data da Última Modificação: 11/abril/2024
   Descrição: Nosso primeiro programa de simulação da disciplina Sistemas Distribuídos.
     Vamos simular N processos, cada um conta o “tempo” independentemente
    Um exemplo simples e significativo para captar o “espírito” da simulação */

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

// Vamos definir os EVENTOS
#define test 1
#define fault 2
#define recovery 3

// Vamos definir o descritor do processo

typedef struct
{
   int id; // identificador de facility do SMPL
   int *state;
} TipoProcesso;

void printStateVector(TipoProcesso *p, int N)
{
   printf("[ ");
   fflush(stdout);
   for (int i = 0; i < N - 1; ++i)
   {
      printf("%2d, ", p->state[i]);
   }

   if (N > 0)
   {
      printf("%2d ", p->state[N - 1]);
   }
   printf("]");
}

TipoProcesso *processo;

int main(int argc, char *argv[])
{

   static int N, // número de processos
       token,    // indica o processo que está executando
       event, r, i,
       MaxTempoSimulac = 150;

   static char fa_name[5];

   if (argc != 2)
   {
      puts("Uso correto: tempo <número de processos>");
      exit(1);
   }

   N = atoi(argv[1]);

   smpl(0, "Um Exemplo de Simulação");
   reset();
   stream(1);

   // inicializar processos

   processo = (TipoProcesso *)malloc(sizeof(TipoProcesso) * N);

   for (i = 0; i < N; i++)
   {
      memset(fa_name, '\0', 5);
      sprintf(fa_name, "%d", i);
      processo[i].id = facility(fa_name, 1);

      processo[i].state = (int *)malloc(sizeof(int) * N);
      memset(processo[i].state, -1, sizeof(int) * N);
      processo[i].state[i] = 0;
   }
   // vamos fazer o escalonamento inicial de eventos

   // nossos processos vão executar testes em intervalos de testes
   // o intervalo de testes vai ser de 30 unidades de tempo
   // a simulação começa no tempo 0 (zero) e vamos escalonar o primeiro teste de todos os
   //       processos para o tempo 30.0

   for (i = 0; i < N; i++)
   {
      schedule(test, 30.0, i);
   }
   schedule(fault, 31.0, 1);
   schedule(fault, 31.0, 2);
   schedule(recovery, 61.0, 1);
   schedule(recovery, 91.0, 2);

   // agora vem o loop principal do simulador

   puts("===============================================================");
   puts("           Sistemas Distribuídos Prof. Elias");
   puts("          LOG do Trabalho Prático 0, Tarefa 0");
   puts("      Digitar, compilar e executar o programa tempo.c");
   printf("   Este programa foi executado para: N=%d processos.\n", N);
   printf("           Tempo Total de Simulação = %d\n", MaxTempoSimulac);
   puts("===============================================================");

   cause(&event, &token);
   while (time() < 150.0)
   {
      switch (event)
      {
      case test:
         if (status(processo[token].id) != 0)
            break; // se o processo está falho, não testa!

         // Testa processo (Token + 1) % N e retorna status
         int foundCorrect = 0;
         int next = (token + 1) % N;
         while (next != token && !foundCorrect)
         {
            if (status(processo[next].id) != 0)
            {
               printf("[tempo %5.1f] Processo %d testou processo %d falho\n", time(), token, next);
               processo[token].state[next] = 1;
               next = (next + 1) % N;
            }
            else
            {
               printf("[tempo %5.1f] Processo %d testou processo %d correto ", time(), token, next);
               processo[token].state[next] = 0;
               int tokenIt = (next + 1) % N;

               if (tokenIt != token)
               {
                  printf("e obteve informações sobre o(s) processo(s) ");
               }
               else
               {
                  printf("e não precisou obter informação sobre nenhum processo");
               }
               while (tokenIt != token)
               {
                  printf("%d", tokenIt);
                  if (processo[next].state[tokenIt] != -1)
                     processo[token].state[tokenIt] = processo[next].state[tokenIt];
                  tokenIt = (tokenIt + 1) % N;
                  if (tokenIt != token)
                     printf(", ");
               }
               printf("\n");
               foundCorrect = 1;
            }
         };

         if (!foundCorrect)
         {
            printf("[tempo %5.1f] Processo %d é o único correto\n", time(), token);
         }

         printf("[tempo %5.1f] Vetor state do processo %d: ", time(), token);
         printStateVector(&(processo[token]), N);
         printf("\n\n");

         schedule(test, 30.0, token);
         break;
      case fault:
         r = request(processo[token].id, token, 0);
         printf("[tempo %5.1f] Socooorro!!! Sou o processo %d e estou falhando\n\n", time(), token);
         break;
      case recovery:
         release(processo[token].id, token);
         printf("[tempo %5.1f] Viva!!! Sou o processo %d e acabo de recuperar\n\n", time(), token);
         schedule(test, 1.0, token);
         break;
      } // switch
      cause(&event, &token);
   } // while
} // tempo.c
