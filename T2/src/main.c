#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "common.h"

// ── Variáveis globais definidas aqui, declaradas extern no common.h ──
pid_t  processos[5];
char   disp_bloqueado[5];
char   oper_bloqueado[5];
char  *nomes[] = {"A1", "A2", "A3", "A4", "A5"};
int    estado_processos[5] = {0, 0, 0, 0, 0};

int pc_processos[5]  = {0, 0, 0, 0, 0};
int mem_processos[5] = {0, 0, 0, 0, 0};
int io_counts_d1[5]  = {0, 0, 0, 0, 0};
int io_counts_d2[5]  = {0, 0, 0, 0, 0};

// T2
int            page_faults[5]       = {0, 0, 0, 0, 0};
int            duplo_page_faults[5] = {0, 0, 0, 0, 0};
PageTableEntry tabelas_paginas[5][16];
QuadroRAM      memoria_ram[32];
int            ram_free[32];
EntradaSwap    fila_swap[20];
int            tamanho_swap = 0;

int main(void)
{
    int   fd[2];
    pid_t pid_controller;

    if (pipe(fd) == -1) {
        perror("Falha ao criar o pipe");
        exit(1);
    }

    // Inicializa RAM: todos os quadros livres
    for (int i = 0; i < 32; i++) {
        ram_free[i] = 1;
        memoria_ram[i].id_processo   = -1;
        memoria_ram[i].pagina_logica = -1;
    }

    // Inicializa Tabelas de Páginas: todas as entradas inválidas
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 16; j++) {
            tabelas_paginas[i][j].valid     = 0;
            tabelas_paginas[i][j].frame     = -1;
            tabelas_paginas[i][j].modifyBit = 0;
            tabelas_paginas[i][j].when      = 0;
        }
    }

    printf("Iniciando Simulador de SO (T2)...\n");

    pid_controller = fork();
    if (pid_controller < 0) {
        perror("Falha no fork");
        exit(1);
    }

    if (pid_controller == 0) {
        // FILHO: InterController Sim
        close(fd[0]);
        run_controller(fd[1]);
        exit(0);
    } else {
        // PAI: KernelSim
        run_kernel(fd[0], fd[1]);
        kill(pid_controller, SIGKILL);
        printf("Simulador encerrado. Tchau!\n");
        exit(0);
    }

    return 0;
}