/* Autores: Leonardo Krambeck e Pietro Polinari Cavassin
   Baseado no exemplo tempo.c do Prof. Elias P. Duarte Jr.
   TP1 — Algoritmo 1: Eleição de Líder Chang-Roberts no anel (VRing)
   Sistemas Distribuídos UFPR 2026/1

   Versão SEM FALHAS. N processos (ids 0..N-1) em anel unidirecional.
   Alguns processos são candidatos a líder; o critério de eleição é o
   MAIOR identificador. Cada candidato envia uma mensagem que circula o
   anel; quando um candidato recebe de volta a sua própria mensagem, sabe
   que é o líder (a mensagem visitou todos os processos do anel).

   Uso: algoritmo1 <N> <cenario>
        cenario 1 = um único candidato
        cenario 2 = candidatos sorteados aleatoriamente
        cenario 3 = todos os N processos são candidatos                 */

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

// Eventos da simulação
#define INICIA 1   // um candidato inicia a eleição enviando sua própria mensagem
#define MENSAGEM 2 // chegada de uma mensagem de eleição a um processo

#define TEMPO_MSG 1.0 // tempo de transmissão de uma mensagem entre dois processos vizinhos

// Descritor do processo
typedef struct {
	int id;        // identificador de facility do SMPL (mantido da estrutura base; sem uso na versão sem falhas)
	int Lider;     // maior id de candidato que este processo conhece (-1 = nenhum líder conhecido)
	int candidato; // 1 se o processo é candidato a líder, 0 caso contrário
} TipoProcesso;

TipoProcesso *processo;

int main(int argc, char *argv[]) {

	static int N, // número de processos
	    token,    // valor carregado pelo evento corrente (ver codificação abaixo)
	    event, i, modo,
	    dest,               // destino de uma mensagem (decodificado do token)
	    idCand,             // id do candidato anunciado por uma mensagem (decodificado do token)
	    proximo,            // próximo processo no anel
	    liderAntigo,        // Lider antes de uma atualização (apenas para o log)
	    numCandidatos = 0,  // quantidade de candidatos no cenário
	    totalMensagens = 0, // métrica: total de mensagens enviadas
	    liderEleito = -1;   // id do líder eleito (-1 enquanto a eleição não termina)

	static char fa_name[5];

	if (argc != 3) {
		puts("Uso correto: algoritmo1 <numero de processos> <cenario: 1=unico 2=aleatorio 3=todos>");
		exit(1);
	}

	N = atoi(argv[1]);
	modo = atoi(argv[2]);
	if (N < 1 || modo < 1 || modo > 3) {
		puts("Parametros invalidos: N>=1 e cenario em {1,2,3}.");
		exit(1);
	}

	smpl(0, "TP1 Algoritmo 1 - Chang-Roberts");
	reset();
	stream(1);

	// inicializar processos
	processo = (TipoProcesso *)malloc(sizeof(TipoProcesso) * N);

	for (i = 0; i < N; i++) {
		memset(fa_name, '\0', 5);
		sprintf(fa_name, "%d", i);
		processo[i].id = facility(fa_name, 1);

		// define se o processo i é candidato, conforme o cenário escolhido
		if (modo == 1) {
			if (i == 0)
				processo[i].candidato = 1; // cenário 1: apenas o processo 0 é candidato
			else
				processo[i].candidato = 0;
		} else if (modo == 3)
			processo[i].candidato = 1; // cenário 3: todos são candidatos
		else
			processo[i].candidato = (ranf() < 0.5); // cenário 2: sorteio aleatório (50% de chance)

		// inicialização do Lider segundo o algoritmo Chang-Roberts
		if (processo[i].candidato) {
			processo[i].Lider = i; // candidato assume a si mesmo como líder inicial
			numCandidatos++;
		} else {
			processo[i].Lider = -1; // não-candidato ainda não conhece nenhum líder
		}
	}

	// no cenário aleatório pode acontecer de ninguém ser sorteado; garante ao menos um candidato
	if (modo == 2 && numCandidatos == 0) {
		processo[N - 1].candidato = 1;
		processo[N - 1].Lider = N - 1;
		numCandidatos = 1;
	}

	// escalonamento inicial: cada candidato lança sua candidatura no tempo 0.
	// Como todos iniciam ao mesmo tempo, o tempo total da eleição é exatamente
	// o de uma volta completa da mensagem vencedora no anel = N unidades.
	for (i = 0; i < N; i++)
		if (processo[i].candidato)
			schedule(INICIA, 0.0, i);

	// cabeçalho do log
	puts("===============================================================");
	puts("           Sistemas Distribuidos Prof. Elias");
	puts("     LOG do Trabalho Pratico 1, Algoritmo 1: Chang-Roberts");
	printf("   N=%d processos | cenario=%d | %d candidato(s)\n", N, modo, numCandidatos);
	printf("   Candidatos: ");
	for (i = 0; i < N; i++)
		if (processo[i].candidato)
			printf("%d ", i);
	printf("\n");
	puts("===============================================================");

	// loop principal: roda até que um líder seja eleito
	// (no Chang-Roberts sem falhas, a mensagem do maior id sempre retorna ao remetente)
	while (liderEleito == -1) {
		cause(&event, &token);
		switch (event) {
			case INICIA:
				// token = índice do processo candidato que inicia a eleição
				proximo = (token + 1) % N;
				printf("[Tempo %5.1f] Processo %d é candidato! Enviou mensagem com (id=%d, cand) ao proximo nodo id=%d\n",
				       time(), token, token, proximo);
				// envia a mensagem ao proximo;
				// o token codifica destino e id anunciado: destino*N + id
				schedule(MENSAGEM, TEMPO_MSG, proximo * N + token);
				totalMensagens++;
				break;

			case MENSAGEM:
				// decodifica do token o destino e o id de candidato anunciado pela mensagem
				dest = token / N;
				idCand = token % N;

				if (idCand == dest) {
					// a mensagem percorreu o anel inteiro e voltou ao seu remetente => ele é o líder
					liderEleito = dest;
					printf("[Tempo %5.1f] Processo %d: recebeu de volta a sua propria mensagem (id=%d): foi ELEITO LIDER!\n",
					       time(), dest, idCand);
				} else if (idCand > processo[dest].Lider) {
					// id recebido é maior que o líder conhecido: atualiza e repassa adiante
					liderAntigo = processo[dest].Lider;
					processo[dest].Lider = idCand;
					proximo = (dest + 1) % N;
					printf("[Tempo %5.1f] Processo %d: recebeu mensagem do candidato %d (id=%d > Lider=%d): atualizou Lider=%d e repassou ao proximo nodo id=%d\n",
					       time(), dest, idCand, idCand, liderAntigo, idCand, proximo);
					schedule(MENSAGEM, TEMPO_MSG, proximo * N + idCand);
					totalMensagens++;
				} else {
					// id recebido <= líder conhecido: a mensagem é descartada (não circula mais)
					printf("[Tempo %5.1f] Processo %d: recebeu mensagem do candidato %d (id=%d < Lider=%d): descartou a mensagem\n",
					       time(), dest, idCand, idCand, processo[dest].Lider);
				}
				break;
		} // switch
	} // while

	// métricas exigidas pelo enunciado
	puts("===============================================================");
	puts("                     RESULTADO DA ELEICAO");
	printf("   Lider eleito.......: processo %d\n", liderEleito);
	printf("   Total de mensagens.: %d\n", totalMensagens);
	printf("   Tempo de simulacao.: %.f unidades de tempo\n", time());
	puts("===============================================================");

	return 0;
} // algoritmo1.c
