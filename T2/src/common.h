#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

/*
 * VARIÁVEIS GLOBAIS COMPARTILHADAS (BASE DO T1 MANTIDA)
 */
extern pid_t processos[1];              // PIDs dos 5 processos de aplicação
extern char disp_bloqueado[1];          // Dispositivo que cada processo está esperando
extern char oper_bloqueado[1];          // Operação (R/W/X) que cada processo solicitou
extern char *nomes[];                   // Nomes dos processos (A1-A5)
extern int estado_processos[1];         // Estado de cada processo (PRONTO/EXECUTANDO/BLOQUEADO/TERMINADO)

extern int pc_processos[1];             // Program Counter de cada processo
extern int mem_processos[1];            // Último endereço de memória acessado

// Mantivemos os nomes para não quebrar o seu código existente, 
// mas o significado deles no T2 mudou:
extern int io_counts_d1[1];             // T2: Contador de acessos a ARQUIVOS
extern int io_counts_d2[1];             // T2: Contador de acessos a DIRETÓRIOS

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
    char op_type[2];   // Ex: "RD-REQ", "WR-REP", "DC-REQ", "DL-REP" [3-5]
    int owner;          // 0 a 4 (Representando os processos A1 a A5) [3]
    
    char path[6];     // Caminho alvo. Ex: "/A1/MyDir/Myfile" [3]
    int path_len;       // Tamanho da string path (strlen) [3]
    
    char payload[MAX_PAYLOAD]; // Buffer p/ leitura/escrita (16 bytes) ou a allfilenames [3, 7]
    int offset;         // Posição no arquivo (0, 16, 32...) ou Código de Erro negativo [3, 8]
    
    char dirname[6];  // Usado apenas nas chamadas add e rem (DC-REQ e DR-REQ) [5]
    int dir_len;        // Tamanho da string dirname (strlen2) [5]
    
    // Exclusivos para a resposta do listdir (DL-REP) [7]
    int fstlstpositions[9][10]; // [índice do nome] -> {inicio, fim, tipo_arquivo}
    int nrnames;                // Quantidade de arquivos/pastas lidos (ou erro negativo)
} SFP_Message;

#endif