#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "common.h"

/*
 * SIMULADOR DE KERNEL PREEMPTIVO (INF1316 - T1)
 * 
 * Este é o ponto de entrada do sistema. O programa cria dois processos principais:
 * 1. InterController Sim - Simula interrupções de hardware (IRQ0, IRQ1, IRQ2)
 * 2. KernelSim - Gerencia 5 processos de aplicação usando Round-Robin preemptivo
 * 
 * Alterações de contexto ocorrem quando:
 * - IRQ0 (TimeSlice a cada 500ms): o Kernel escolhe outro processo para executar
 * - IRQ1/IRQ2: processos bloqueados em I/O são desbloqueados
 * 
 * O usuario pode pausar a simulação com Ctrl+Z para ver o estado de todos
 * os processos (PC, memória, estado, acessos a dispositivos, etc.)
 */

// Variáveis globais que armazenam o estado dos processos
// Compartilhadas entre Kernel e o handler de sinais
pid_t processos[5];
char disp_bloqueado[5];
char oper_bloqueado[5];
char *nomes[] = {"A1", "A2", "A3", "A4", "A5"};
int estado_processos[5] = {0, 0, 0, 0, 0};

int pc_processos[5] = {0, 0, 0, 0, 0};
int mem_processos[5] = {0, 0, 0, 0, 0};
int io_counts_d1[5] = {0, 0, 0, 0, 0};
int io_counts_d2[5] = {0, 0, 0, 0, 0};

int main()
{
    int fd[2];
    pid_t pid_controller;

    // Cria um pipe antes de fazer o fork
    // Isso permite comunicacao entre pai (Kernel) e filho (InterController)
    if (pipe(fd) == -1)
    {
        perror("Falha ao criar o pipe");
        exit(1);
    }

    printf("A iniciar o Simulador de SO...\n");

    // Faz o fork para criar o processo filhão que será o InterController
    // O pai continuará como KernelSim
    pid_controller = fork();

    if (pid_controller < 0)
    {
        perror("Falha no fork");
        exit(1);
    }

    if (pid_controller == 0)
    {
        // CÓDIGO DO FILHO: InterController Sim
        // O filho não lê do pipe, então fecha o fd de leitura
        close(fd[0]);
        run_controller(fd[1]);
        exit(0);
    }
    else 
    {
        // CÓDIGO DO PAI: KernelSim
        // O pai gerencia a execução e ler as interrupções do filho
        run_kernel(fd[0], fd[1]);

        // Quando o Kernel termina (todos os processos finalizaram),
        // mata o InterController e encerra a simulação
        kill(pid_controller, SIGKILL);
        printf("Simulador encerrado com sucesso. Tchau!\n");
        exit(0);
    }

    return 0;
}