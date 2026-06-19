#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include "common.h"


// --- NOVIDADES DO T2: FILAS DE REQUISIÇÃO ---
// Guardam as respostas (REPs) que chegam da rede até o Hardware liberar (IRQ1 ou IRQ2)
SFP_Message file_request_queue[9];
int tamanho_file_queue = 0;

SFP_Message dir_request_queue[9];
int tamanho_dir_queue = 0;

// --- NOVIDADES DO T2: REDE E MEMÓRIA COMPARTILHADA ---
int sockfd;
struct sockaddr_in serveraddr;
char *shm_ptrs[5]; // Ponteiros para a shmem de cada aplicação (A1 a A5)

// Variáveis de Escalonamento (Mantidas do T1)
int processo_atual = -1;

// Função do T1 para ler do Pipe (mantida intacta)
int ler_mensagem_pipe(int fd, char *buffer) {
    int i = 0; char c;
    while (read(fd, &c, 1) > 0) {
        buffer[i++] = c;
        if (c == '\0') return i;
    }
    return 0; 
}

// ----- INICIALIZAÇÃO DO KERNELSIM -----
void run_kernel(int read_fd, int write_fd) {
    fcntl(read_fd, F_SETFL, O_NONBLOCK);
    signal(SIGTSTP, handle_sigtstp);

    // 1. CONEXÃO COM A MEMÓRIA COMPARTILHADA (shmem) DAS APLICAÇÕES
    for (int i = 0; i < 5; i++) {
        key_t shm_key = 8000 + (i + 1); // Chaves 8001 a 8005
        int shm_id = shmget(shm_key, sizeof(SFP_Message), IPC_CREAT | 0666);
        shm_ptrs[i] = (char *) shmat(shm_id, NULL, 0);
    }

    // 2. CONFIGURAÇÃO DO SOCKET UDP CLIENTE
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("ERRO abrindo socket UDP no Kernel");
        exit(1);
    }
    
    struct hostent *server = gethostbyname("127.0.0.1"); // localhost
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(8080); // Porta padrão do nosso SFSS

    printf(">>> KernelSim iniciado como Micro-Kernel Cliente UDP na porta 8080 <<<\n");

    // =========================================================================
    // 3. CRIAÇÃO DOS PROCESSOS DE APLICAÇÃO (A1 a A5)
    // =========================================================================
    char write_fd_str[16];
    sprintf(write_fd_str, "%d", write_fd); // Converte o Pipe FD para string para o execl

    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Código do filho: substitui a memória pelo binário 'app'
            execl("./app", "app", nomes[i], write_fd_str, NULL);
            perror("Erro no execl do aplicativo");
            exit(1);
        } else if (pid > 0) {
            // Código do Kernel (Pai): registra o PID na nossa tabela virtual
            processos[i] = pid;
            estado_processos[i] = 0; // PRONTO
        } else {
            perror("Erro no fork");
            exit(1);
        }
    }
    // =========================================================================

    char buffer[1024];

    // ----- LOOP INFINITO DO KERNEL -----
    while (1) {
        // A. ESCUTAR A REDE DE FORMA NÃO-BLOQUEANTE (O segredo do T2!)
        SFP_Message msg_rx;
        socklen_t serverlen = sizeof(serveraddr);
        // O MSG_DONTWAIT faz o Kernel perguntar à rede e seguir em frente se não houver nada, sem travar!
        int n = recvfrom(sockfd, &msg_rx, sizeof(SFP_Message), MSG_DONTWAIT, (struct sockaddr *) &serveraddr, &serverlen);
        
        if (n > 0) {
            if (strncmp(msg_rx.op_type, "RD-REP", 6) == 0 || strncmp(msg_rx.op_type, "WR-REP", 6) == 0) {
                if (tamanho_file_queue < 9) {
                    file_request_queue[tamanho_file_queue++] = msg_rx;
                } else {
                    printf("AVISO: File-Queue cheia! Resposta descartada.\n");
                }
            } else {
                if (tamanho_dir_queue < 9) {
                    dir_request_queue[tamanho_dir_queue++] = msg_rx;
                } else {
                    printf("AVISO: Dir-Queue cheia! Resposta descartada.\n");
                }
            }
        }

        // B. ESCUTAR O PIPE (Comunicação com Hardware e Apps)
        if (ler_mensagem_pipe(read_fd, buffer) > 0) {
            
            // --- IRQ0: Escalonamento Round-Robin (TimeSlice) ---
            if (strcmp(buffer, "IRQ0") == 0) {
                if (processo_atual != -1 && estado_processos[processo_atual] == 1) {
                    kill(processos[processo_atual], SIGSTOP);
                    estado_processos[processo_atual] = 0; 
                }
                for (int i = 1; i <= 5; i++) {
                    int next = (processo_atual + i) % 5;
                    if (estado_processos[next] == 0) {
                        processo_atual = next;
                        estado_processos[processo_atual] = 1;
                        kill(processos[processo_atual], SIGCONT);
                        break;
                    }
                }
            }

            // --- IRQ1: Resposta de Hardware para ARQUIVOS (Desbloqueio) ---
            else if (strcmp(buffer, "IRQ1") == 0) {
                if (tamanho_file_queue > 0) {
                    SFP_Message rep = file_request_queue[0]; // Pega a mais antiga (FIFO)
                    int owner_id = rep.owner;

                    memcpy(shm_ptrs[owner_id], &rep, sizeof(SFP_Message));

                    // Acorda a aplicação
                    estado_processos[owner_id] = 0; // Vai para PRONTO
                    printf(">>> Kernel (IRQ1): Arquivo concluído! Dados na ShMem de %s. Processo PRONTO.\n", nomes[owner_id]);

                    // Anda a fila FIFO
                    for (int k = 0; k < tamanho_file_queue - 1; k++) file_request_queue[k] = file_request_queue[k + 1];
                    tamanho_file_queue--;
                }
            }

            // --- IRQ2: Resposta de Hardware para DIRETÓRIOS (Desbloqueio) ---
            else if (strcmp(buffer, "IRQ2") == 0) {
                if (tamanho_dir_queue > 0) {
                    SFP_Message rep = dir_request_queue[0]; // Pega a mais antiga (FIFO)
                    int owner_id = rep.owner;

                    memcpy(shm_ptrs[owner_id], &rep, sizeof(SFP_Message));

                    estado_processos[owner_id] = 0; 
                    printf(">>> Kernel (IRQ2): Diretório concluído! Dados na ShMem de %s. Processo PRONTO.\n", nomes[owner_id]);

                    for (int k = 0; k < tamanho_dir_queue - 1; k++) dir_request_queue[k] = dir_request_queue[k + 1];
                    tamanho_dir_queue--;
                }
            }

            // --- SYSCALL: Envio do pedido pela REDE ---
            else if (strncmp(buffer, "SYSCALL", 7) == 0) {
                char app[3], op[10], path[128], extra1[64], extra2[MAX_PAYLOAD];
                int arg_count = sscanf(buffer, "SYSCALL %s %s %s %s %s", app, op, path, extra1, extra2);
                
                int app_id = app[1] - '1'; // "A1" vira 0

                // 1. Montamos o Datagrama SFP
                SFP_Message req;
                memset(&req, 0, sizeof(SFP_Message));
                strcpy(req.op_type, op);
                req.owner = app_id;
                strcpy(req.path, path);
                req.path_len = strlen(path);

                // Preenche os campos específicos dependendo da operação
                if (strncmp(op, "RD-REQ", 6) == 0) {
                    req.offset = atoi(extra1);
                } else if (strncmp(op, "WR-REQ", 6) == 0) {
                    req.offset = atoi(extra1);
                    if (arg_count == 5) strcpy(req.payload, extra2);
                } else if (strncmp(op, "DC-REQ", 6) == 0 || strncmp(op, "DR-REQ", 6) == 0) {
                    strcpy(req.dirname, extra1);
                    req.dir_len = strlen(extra1);
                }

                // 2. Dispara o pacote UDP pela rede para o SFSS
                sendto(sockfd, &req, sizeof(SFP_Message), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
                printf(">>> Kernel: Mensagem %s de %s enviada p/ SFSS via UDP.\n", op, app);

                // 3. Encara (bloqueia) a aplicação que pediu
                kill(processos[app_id], SIGSTOP);
                estado_processos[app_id] = 2; // BLOQUEADO
                
                // Grava o motivo no PCB para o relatório do Ctrl+Z
                disp_bloqueado[app_id] = (strncmp(op, "R", 1) == 0 || strncmp(op, "W", 1) == 0) ? '1' : '2'; // 1=Arq, 2=Dir
                oper_bloqueado[app_id] = op[0]; // Pega a primeira letra (R, W, D)

                // Força o escalonamento imediatamente
                if (processo_atual == app_id) {
                    processo_atual = -1; // Libera a roleta
                }
            }

            // --- UPDATE: Atualiza PCB ---
            else if (strncmp(buffer, "UPDATE", 6) == 0) {
                char app[3]; int pc, mem;
                sscanf(buffer, "UPDATE %s %d %d", app, &pc, &mem);
                int idx = app[1] - '1';
                pc_processos[idx] = pc;
                mem_processos[idx] = mem;
            }
        }

        // C. VERIFICAR SE TODOS OS PROCESSOS TERMINARAM
        int finalizados = 0;
        for (int i = 0; i < 5; i++) {
            if (estado_processos[i] == 3) {
                finalizados++;
            } else if (waitpid(processos[i], NULL, WNOHANG) > 0) {
                // WNOHANG = verifica sem bloquear
                estado_processos[i] = 3; // TERMINADO
                finalizados++;
                printf(">>> Kernel: Processo %s terminou.\n", nomes[i]);
            }
        }

        if (finalizados == 5) {
            printf(">>> Kernel: Todos os processos finalizaram!\n");
            break; // Sai do while(1)
        }
        // Evita consumo abusivo de CPU no loop infinito
        usleep(1000); 
    }
}

// ----- RELATÓRIO DE DIAGNÓSTICO (CTRL+Z) -----
void handle_sigtstp(int sig) {
    printf("\n\n=======================================================\n");
    printf("     SIMULAÇÃO PAUSADA (Ctrl+Z) - MICRO-KERNEL T2      \n");
    printf("=======================================================\n");

    for (int i = 0; i < 5; i++) {
        printf("Processo [%s]:\n", nomes[i]);
        printf("  - PC Atual      : %d\n", pc_processos[i]);
        printf("  - Memoria       : m%02d\n", mem_processos[i]);

        printf("  - Estado        : ");
        if (estado_processos[i] == 0) printf("PRONTO\n");
        else if (estado_processos[i] == 1) printf("EXECUTANDO (CPU)\n");
        else if (estado_processos[i] == 2) {
            printf("BLOQUEADO ");
            if (disp_bloqueado[i] == '1') printf("(Aguardando resposta de ARQUIVO no IRQ1)\n");
            else printf("(Aguardando resposta de DIRETORIO no IRQ2)\n");
        }
        else if (estado_processos[i] == 3) printf("TERMINADO\n");
        printf("-------------------------------------------------------\n");
    }

    printf("\n--- FILAS DE REDE (SFSS) ---\n");
    printf("File-Request-Queue : %d pacote(s) aguardando o próximo IRQ1\n", tamanho_file_queue);
    printf("Dir-Request-Queue  : %d pacote(s) aguardando o próximo IRQ2\n", tamanho_dir_queue);

    printf("\nPressione [ENTER] para retomar a simulação...\n");
    getchar();

    printf("Retomando simulação...\n");
    printf("=======================================================\n\n");
}