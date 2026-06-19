#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "common.h"

#define MAX 2000

int main(int argc, char *argv[]) {
    // Recebemos o nome do processo (A1-A5) e o fd do pipe para o Kernel
    char *nome_app = (argc > 1) ? argv[1] : "A1";
    int write_fd = (argc > 2) ? atoi(argv[2]) : -1;
    
    int pc = 0;
    int mem;
    // CORREÇÃO: Buffer grande o suficiente para as mensagens SFP compridas
    char msg_syscall[512]; 

    // CORREÇÃO: Faltava calcular a chave única da memória (Ex: "A1" vira 8001)
    int app_id = nome_app[1] - '0'; 
    key_t shm_key = 8000 + app_id; 

    int shm_id = shmget(shm_key, sizeof(SFP_Message), IPC_CREAT | 0666);
    char *shm_ptr = (char *) shmat(shm_id, NULL, 0);
    memset(shm_ptr, 0, sizeof(SFP_Message));

    // Inicializa o gerador de números aleatórios com base no PID
    srand(time(NULL) ^ getpid());

    // ----- LOOP PRINCIPAL DO APLICATIVO -----
    for (pc = 1; pc <= MAX; pc++) {
        
        SFP_Message *resp = (SFP_Message *)shm_ptr;
        if (resp->op_type[0] != '\0') {
            printf("\n<<< App [%s] ACORDOU! Resposta recebida: %s >>>\n", nome_app, resp->op_type);
            if (strncmp(resp->op_type, "DL-REP", 6) == 0) {
                printf("    Listagem de %d itens:\n", resp->nrnames);
                for (int i = 0; i < resp->nrnames; i++) {
                    int inicio = resp->fstlstpositions[i][0];
                    int fim    = resp->fstlstpositions[i][1];
                    int tipo   = resp->fstlstpositions[i][2];
                    printf("      [%d] %.*s (%s)\n", 
                        i, fim - inicio + 1, &resp->payload[inicio],
                        tipo == 1 ? "DIR" : "ARQ");
                }
            } else if (strncmp(resp->op_type, "RD-REP", 6) == 0) {
                printf("    Leitura (offset %d): %.16s\n", resp->offset, resp->payload);
            } else if (strncmp(resp->op_type, "WR-REP", 6) == 0) {
                printf("    Escrita confirmada (offset %d)\n", resp->offset);
            } else {
                printf("    Diretorio atualizado: %s\n", resp->path);
            }
            memset(shm_ptr, 0, sizeof(SFP_Message));
            printf("\n");
        }

        // 2. USO DA CPU (Acesso virtual)
        mem = rand() % 16; 
        printf("App [%s] rodando... PC: %d, Memoria acessada: m%02d\n", nome_app, pc, mem);

        // 3. ATUALIZAÇÃO DO PCB NO KERNEL (Via Pipe)
        if (write_fd != -1) {
            // CORREÇÃO: Tamanho seguro
            char msg_update[64]; 
            sprintf(msg_update, "UPDATE %s %d %d", nome_app, pc, mem);
            write(write_fd, msg_update, strlen(msg_update) + 1);
        }
        
        // 4. CHAMADAS DE SISTEMA (10% de probabilidade no T2)
        if (rand() % 100 < 10 && write_fd != -1) {
            int d = rand() % 100;
            
            // Sorteia a zona de acesso: 50% Privado (/Ax), 50% Compartilhado (/A0)
            // CORREÇÃO: Tamanho seguro para caminho longo
            char path[128]; 
            if (rand() % 2 == 0) sprintf(path, "/A0/dir_teste");
            else sprintf(path, "/%s/dir_teste", nome_app);
            
            // Regra do enunciado: ÍMPAR = Operações de Arquivo
            if (d % 2 != 0) { 
                // Os blocos do T2 são restritos a 16 bytes. Sorteando o Offset:
                int offsets[] = {0, 16, 32, 48, 64, 80, 96};
                int off = offsets[rand() % 7];
                
                // CORREÇÃO: Tamanho seguro
                char file_path[256]; 
                sprintf(file_path, "%s/arquivo.txt", path);
                
                if (d % 3 == 0 || d % 3 == 2) { 
                    // Requisita Leitura (RD-REQ)
                    sprintf(msg_syscall, "SYSCALL %s RD-REQ %s %d", nome_app, file_path, off);
                } else { 
                    // Requisita Escrita (WR-REQ) empacotando os 16 bytes fictícios
                    sprintf(msg_syscall, "SYSCALL %s WR-REQ %s %d DADOS_TESTE_16B.", nome_app, file_path, off);
                }
            } 
            // Regra do enunciado: PAR = Operações de Diretório
            else { 
                if (d % 3 == 0) {
                    // Adiciona diretório (DC-REQ)
                    sprintf(msg_syscall, "SYSCALL %s DC-REQ %s NovaPasta", nome_app, path);
                } else if (d % 3 == 1) {
                    // Remove diretório/arquivo (DR-REQ)
                    sprintf(msg_syscall, "SYSCALL %s DR-REQ %s VelhaPasta", nome_app, path);
                } else {
                    // Lista o diretório (DL-REQ)
                    sprintf(msg_syscall, "SYSCALL %s DL-REQ %s", nome_app, path);
                }
            }

            // Atira a requisição formatada pelo Pipe e se prepara para o SIGSTOP
            write(write_fd, msg_syscall, strlen(msg_syscall) + 1);
            printf(">>> App [%s] pediu SYSCALL DE REDE: %s\n", nome_app, msg_syscall);
        }

        // Simula uma unidade de tempo de processamento
        sleep(1); 
    }
    // Desconecta-se graciosamente da memória compartilhada ao morrer
    shmdt(shm_ptr);
    return 0;
}