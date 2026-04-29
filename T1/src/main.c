#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "common.h"

// -- Variáveis Globais para o Relatório ---
pid_t processos[5];
char disp_bloqueado[5]; // Guarda '1' ou '2'
char oper_bloqueado[5]; // Guarda 'R', 'W', ou 'X'
char *nomes[] = {"A1", "A2", "A3", "A4", "A5"};
int estado_processos[5] = {0, 0, 0, 0, 0};

int pc_processos[5] = {0, 0, 0, 0, 0};  // Guarda o PC atual
int mem_processos[5] = {0, 0, 0, 0, 0}; // Guarda a Memória atual
int io_counts_d1[5] = {0, 0, 0, 0, 0};
int io_counts_d2[5] = {0, 0, 0, 0, 0};

// ---------------------------------------------------------
// Ponto de Partida
// ---------------------------------------------------------
int main()
{
    int fd[2]; // fd[0] é a boca de leitura, fd[1] é a boca de escrita
    pid_t pid_controller;

    // 1. Criar o pipe ANTES do fork (Crucial!)
    if (pipe(fd) == -1)
    {
        perror("Falha ao criar o pipe");
        exit(1);
    }

    printf("A iniciar o Simulador de SO...\n");

    // 2. O grande momento: dividir o processo em dois
    pid_controller = fork();

    if (pid_controller < 0)
    {
        perror("Falha no fork");
        exit(1);
    }

    if (pid_controller == 0)
    {
        // CÓDIGO DO FILHO: Vai atuar como InterController Sim
        close(fd[0]); // O filho não vai ler do pipe, por isso fechamos a leitura
        run_controller(fd[1]);
        exit(0);
    }
    else {
        // CÓDIGO DO PAI: Vai atuar como KernelSim
        run_kernel(fd[0], fd[1]);

        // Quando o run_kernel der o 'break' e terminar, o código continua aqui:
        kill(pid_controller, SIGKILL); // Mata o Fofoqueiro (Controlador)
        printf("Simulador encerrado com sucesso. Tchau!\n");
        exit(0);
    }

    return 0;
}