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
 * 1. Controller Sim (controller) - Simula interrupções de hardware (IRQ0, IRQ1, IRQ2)
 * 2. KernelSim (kernal) - Gerencia 5 processos de aplicação usando Round-Robin preemptivo
 * 
 * Alterações de contexto ocorrem quando:
 * - IRQ0 (TimeSlice a cada 500ms): o Kernel escolhe outro processo para executar
 * - IRQ1/IRQ2: processos bloqueados em I/O são desbloqueados (simulam que querem fazer ação de entrada e saída)
 * - IR3:
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

// Adicionei para o trabalho 2
int page_faults[5] = {0, 0, 0, 0, 0};
PageTableEntry tabelas_paginas[5][16];
QuadroRAM memoria_ram[32];      // Qual processo é dono deste quadro (0 a 4)
int ram_free[32];        // 1 = Quadro livre, 0 = Quadro Ocupado


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

    // inicializa a RAM dizendo que todos os quadros estão livres
    for (int i = 0; i < 32; i++) ram_free[i] = 1;

    // inicializa a tabela de páginas dizendo que nenhum processo está na RAM (valid = 0)
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            tabelas_paginas[i][j].valid = 0;
            tabelas_paginas[i][j].frame = -1;
            tabelas_paginas[i][j].modifyBit =0;
        }
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
        // O filho não lê do pipe, então fecha o fd de leitura
        close(fd[0]);

        run_controller(fd[1]); // chama a função do controller, então o filho = controller

        exit(0);
    }

    else 
    {
        // O pai gerencia a execução e ler as interrupções do filho
        run_kernel(fd[0], fd[1]); // chama a função do kernel, então o pai = kernel

        // Quando o Kernel termina (todos os processos finalizaram), mata o controller e encerra a simulação
        kill(pid_controller, SIGKILL);
        printf("Simulador encerrado com sucesso. Tchau!\n");
        
        exit(0);
    }

    return 0;
}