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

#endif