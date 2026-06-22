#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

/*
 * VARIÁVEIS GLOBAIS COMPARTILHADAS
 */

extern pid_t processos[5];
extern char disp_bloqueado[5];
extern char oper_bloqueado[5];
extern char *nomes[];
extern int estado_processos[5];

extern int pc_processos[5];
extern int mem_processos[5];
extern int io_counts_d1[5];
extern int io_counts_d2[5];

// Protótipos
void run_controller(int write_fd);
void run_kernel(int read_fd, int write_fd);
void handle_sigtstp(int sig);
int ler_mensagem_pipe(int fd, char *buffer);

/* ---------------------------------------------------------------
 * ESTRUTURAS DE MEMÓRIA VIRTUAL (T2)
 * --------------------------------------------------------------- */

// Entrada da Tabela de Páginas (Page Table Entry)
typedef struct
{
    int valid;     // 1 = na RAM, 0 = ausente (Page Fault)
    int frame;     // índice do quadro em memoria_ram[] (-1 se inválido)
    int modifyBit; // 1 = página foi escrita (dirty), 0 = limpa
    int when;      // valor do PC do processo no momento do último acesso
} PageTableEntry;

extern PageTableEntry tabelas_paginas[5][16]; // TPx[processo][pagina_logica]

// Quadro da memória física
typedef struct
{
    int id_processo;   // índice do processo dono (0-4)
    int pagina_logica; // qual página lógica está aqui (0-15)
} QuadroRAM;

extern QuadroRAM memoria_ram[32];
extern int ram_free[32]; // 1 = livre, 0 = ocupado

// ---------------------------------------------------------------
// Entrada da fila de swap: guarda processo E página solicitada,
// além de um contador para o caso de "duplo IRQ3" (dirty eviction).
// ---------------------------------------------------------------
typedef struct
{
    int id_processo;    // quem está esperando
    int pagina_logica;  // qual página está sendo carregada do swap
    int irq3_pendentes; // 1 = ainda precisa de mais 1 IRQ3 (dirty), 0 = pronto
} EntradaSwap;

extern EntradaSwap fila_swap[20]; // fila de espera pelo disco de swap (FCFS)
extern int tamanho_swap;

// Estatísticas
extern int page_faults[5];
extern int duplo_page_faults[5];

#endif