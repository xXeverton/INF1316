#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

// Variáveis globais (O 'extern' significa "confie em mim, isso está em outro arquivo")
extern pid_t processos[5];
extern char disp_bloqueado[5];
extern char oper_bloqueado[5];
extern char *nomes[];
extern int estado_processos[5];
extern int pc_processos[5];
extern int mem_processos[5];
extern int io_counts_d1[5];
extern int io_counts_d2[5];

// Protótipos das funções
void run_controller(int write_fd);
void run_kernel(int read_fd, int write_fd);
void handle_sigtstp(int sig);
int ler_mensagem_pipe(int fd, char *buffer);

#endif