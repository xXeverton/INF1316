#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

/*
 * VARIÁVEIS GLOBAIS COMPARTILHADAS (BASE DO T1 MANTIDA)
 */
extern pid_t processos[5];              
extern char disp_bloqueado[5];          
extern char oper_bloqueado[5];          
extern char *nomes[5];                  
extern int estado_processos[5];         
extern int pc_processos[5];             
extern int mem_processos[5];            
extern int io_counts_d1[5];             
extern int io_counts_d2[5];

// Protótipos das funções principais
void run_controller(int write_fd);      
void run_kernel(int read_fd, int write_fd);  
void handle_sigtstp(int sig);           
int ler_mensagem_pipe(int fd, char *buffer);  

/*
 * =====================================================================
 * ESTRUTURAS DO TRABALHO 2: SIMPLE FILE PROTOCOL (SFP) SOBRE UDP
 * =====================================================================
 * Esta estrutura (struct) agrupa todas as variáveis necessárias para 
 * montar as requisições (REQ) e respostas (REP) definidas no enunciado,
 * facilitando o envio pela rede num único "pacote" UDP.
 */

#define MAX_PAYLOAD 1024 // Tamanho seguro para suportar a string compactada do listdir

typedef struct {
char op_type[10];  // Corrigido: Suporta "RD-REQ", "DL-REP", etc + '\0'
    int owner;         // 0 a 4 (Representando os processos A1 a A5)
    
    char path[128];    // Corrigido: Caminho alvo com folga para strings maiores
    int path_len;      // Tamanho da string path (strlen)
    
    char payload[MAX_PAYLOAD]; // Buffer p/ leitura/escrita ou allfilenames
    int offset;        // Posição no arquivo ou Código de Erro negativo
    
    char dirname[64];  // Corrigido: Usado nas chamadas add e rem
    int dir_len;       // Tamanho da string dirname
    
    int fstlstpositions[40][3]; 
    int nrnames;       // Quantidade de arquivos/pastas lidos (ou erro)
} SFP_Message;

#endif