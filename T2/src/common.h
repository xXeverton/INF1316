#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

/*
 * VARIÁVEIS GLOBAIS COMPARTILHADAS
 * 
 * Essas variáveis são compartilhadas entre o main.c (inicialização),
 * kernel.c (gerenciamento) e o handler de sinais. Permitem que o relatório
 * do Ctrl+Z (handle_sigtstp) acesse o estado completo de todos os processos.
 */

extern pid_t processos[5];              // PIDs dos 5 processos de aplicação
extern char disp_bloqueado[5];          // Dispositivo que cada processo está esperando
extern char oper_bloqueado[5];          // Operação (R/W/X) que cada processo solicitou
extern char *nomes[];                   // Nomes dos processos (A1-A5)
extern int estado_processos[5];         // Estado de cada processo (PRONTO/EXECUTANDO/BLOQUEADO/TERMINADO)

extern int pc_processos[5];             // Program Counter de cada processo
extern int mem_processos[5];            // Último endereço de memória acessado
extern int io_counts_d1[5];             // Contador de acessos ao dispositivo D1
extern int io_counts_d2[5];             // Contador de acessos ao dispositivo D2

// Protótipos das funções principais
void run_controller(int write_fd);      // InterController Sim: gera interrupções
void run_kernel(int read_fd, int write_fd);  // KernelSim: gerencia os processos
void handle_sigtstp(int sig);           // Handler de Ctrl+Z: exibe relatório
int ler_mensagem_pipe(int fd, char *buffer);  // Lê mensagens do pipe

// ESTRUTURAS DO TRABALHO 2: MEMÓRIA VIRTUAL E PAGINAÇÃO
// Estrutura de uma Entrada da Tabela de Páginas (PTE - Page Table Entry)
typedef struct {
    int valid;      // 1 = Presente na RAM, 0 = Não está na RAM (Page Fault)
    int frame;      // Se valid==1, indica em qual quadro da RAM (0 a 31) a página está
    int modifyBit;  // 1 = Página foi modificada (Dirty), 0 = Página limpa
    int when;
} PageTableEntry;

// A Tabela de Páginas Global para os 5 Processos
// Como temos 5 processos (A1 a A5) e cada um tem 16 páginas lógicas (0 a 15),
// criamos uma Matriz (Array de Arrays) de structs.
// Ex: tabelas_paginas[0][5] acessa a página lógica 5 do processo A1.
extern PageTableEntry tabelas_paginas[5][16];

// A Memória RAM Física e o Controle de Espaço Livre
// O professor pediu um array RAM[32] e um RAM_free[32].
// Vamos guardar qual Processo (PID simulado ou índice 0-4) e qual Página (0-15) 
// estão ocupando cada quadro da RAM para facilitar o algoritmo de substituição depois.
typedef struct {
    int id_processo; // Qual processo é dono deste quadro (0 a 4)
    int pagina_logica; // Qual página lógica está aqui dentro (0 a 15)
} QuadroRAM;

extern QuadroRAM memoria_ram[32]; // A memória física
extern int ram_free[32];          // 1 = Quadro Livre, 0 = Quadro Ocupado

// 4. Estatísticas para o Relatório do Ctrl+Z
extern int page_faults[5];        // Conta quantos Page Faults cada processo sofreu


#endif