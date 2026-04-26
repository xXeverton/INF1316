#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <signal.h>

// ---------------------------------------------------------
// Função do Fofoqueiro: InterController Sim
// ---------------------------------------------------------
void run_controller(int write_fd) {
    srand(time(NULL)); // Inicializa a semente para os números aleatórios
    char msg[50];

    while(1) {
        // Gera o TimeSlice (IRQ0) a cada 500ms (500.000 microssegundos)
        usleep(500000); 
        strcpy(msg, "IRQ0");
        write(write_fd, msg, sizeof(msg));

        // Sorteia interrupções de I/O (D1 e D2)
        int probabilidade = rand() % 100; // Gera um número de 0 a 99
        
        if (probabilidade < 10) { 
            // 10% de probabilidade (P1 = 0.1)
            strcpy(msg, "IRQ1");
            write(write_fd, msg, sizeof(msg));
        } else if (probabilidade >= 10 && probabilidade < 15) { 
            // 5% de probabilidade (P2 = 0.05) -> do 10 ao 14 são 5 números
            strcpy(msg, "IRQ2");
            write(write_fd, msg, sizeof(msg));
        }
    }
}

// ---------------------------------------------------------
// Função do Chefe: KernelSim
// ---------------------------------------------------------
void run_kernel(int read_fd, int write_fd) {
    char buffer[50];
    printf("KernelSim iniciado e a aguardar interrupções...\n");
    
    pid_t processos[5];
    char* nomes[] = {"A1", "A2", "A3", "A4", "A5"};
    
    char fd_str[10];
    sprintf(fd_str, "%d", write_fd);

    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            printf("Erro ao criar processo filho\n");
            exit(1);
        }


        if (pid == 0){
            execl("./bin/app", "./bin/app", nomes[i], fd_str, NULL);

            perror("Falha no execl\n");
            exit(1);

        }
        else {
            // Código do Pai
            processos[i] = pid;

            // Dá um pequeno tempo para garantir que o filho fez o exec antes de pausar
            usleep(10000);
            kill(pid, SIGSTOP);
            printf("Kernel: Processo %s (PID %d) criado e pausado\n", nomes[i], pid);
        }
    }

    
    // Estado possíveis para simplificar: 0 = PRONTO, 1 = EXECUTANDO, 2 = BLOQUEADO
    int estado_processos[5] = {0, 0, 0, 0, 0};
    
    // Filas para D1 e D2
    int fila_d1[5];
    int tamanho_d1 = 0;
    
    int fila_d2[5];
    int tamanho_d2 = 0;
    
    // Escalonador Round-Robin
    int processo_atual = 0;
    kill(processos[processo_atual], SIGCONT);   // Acorda o primeiro processo para o a simulação começar!
    printf("Kernel: Iniciando a execução com %s (PID %d)\n", nomes[processo_atual], processos[processo_atual]);

    while(1) {
        // Lê do pipe bloqueando até chegar uma nova mensagem
        if (read(read_fd, buffer, sizeof(buffer)) > 0) {
            
            // Se for um alarme de tempo (IRQ0), fazemos a troca de contexto!
            if (strcmp(buffer, "IRQ0") == 0) {
                // 1. Pausa o que está rodando agora: 
                // Só pausa o processo atual se ele estava EXECUTANDO (1)
                // Se ele já estava bloqueado (2) não fazemos nada com ele
                if (estado_processos[processo_atual] == 1) {
                    kill(processos[processo_atual], SIGSTOP);
                    estado_processos[processo_atual] = 0; // volta para pronto
                }

                // 2. Vai para o próximo processo PRONTO na fila circular
                for (int j = 0; j < 5; j++) {
                    processo_atual = (processo_atual + 1) % 5;
                    if (estado_processos[processo_atual] == 0) {
                        estado_processos[processo_atual] = 1;
                        // 3. Acorda o novo processo
                        kill(processos[processo_atual], SIGCONT);
                        printf("--- Troca de Contexto por IRQ0: Agora rodando %s ---\n", nomes[processo_atual]);
                        break;
                    }
                }
            }

             
            else if (strncmp(buffer, "SYSCALL", 7) == 0) {
                // A mensagem é do tipo: "SYSCALL A1 D1 R"
                printf("Kernel recebeu: %s \n", buffer);

                // 1. Descobrir quem mandou a SYSCALL (olhando A1, A2, ...)
                int id_bloqueado = buffer[9] - '1'; // Converte char '1' para int 0

                // 2. pausa o processo imediatamente
                kill(processos[id_bloqueado], SIGSTOP);
                estado_processos[id_bloqueado] = 2; // Marca como bloqueado

                // 3. Coloca na fila certa (olhando para D1 e D2)
                if (buffer[12] == '1'){
                    fila_d1[tamanho_d1++] = id_bloqueado;
                    printf("Kernel: Processo %s foi bloqueado e colocado na Fila D1.\n", nomes[id_bloqueado]);
                }

                else if (buffer[12] == '2') {
                    fila_d2[tamanho_d2++] = id_bloqueado;
                    printf("Kernel: Processo %s foi bloqueado e colocado na Fila D2.\n", nomes[id_bloqueado]);
                }

                // 4. Se o cara pediu I/O era o processo atual rodando, temos que 
                // obrigatoriamente achar outro cara PRONTO para dar a CPU para ele
                if (processo_atual == id_bloqueado) {
                    // Loop para encontrar o próximo cara pronto!
                    for (int j = 0; j < 5; j++) {
                        processo_atual = (processo_atual + 1) % 5;
                        if (estado_processos[processo_atual] == 0) {
                            estado_processos[processo_atual] = 1;
                            kill(processos[processo_atual], SIGCONT);
                            printf("--- Troca de Contexto FORCADA por Syscall: Agora rodando %s ---\\n", nomes[processo_atual]);
                            break;
                        }
                    }
                }
            }

            else if (strcmp(buffer, "IRQ1") == 0) {
                if (tamanho_d1 > 0) {
                    // Pega o primeiro da Fila (o mais antigo)
                    int id_acordado = fila_d1[0];

                    // Ele sai da geladeira e volta para a fila da CPU (Roudin-Robin)
                    estado_processos[id_acordado] = 0;
                    printf("Kernel: Hardware D1 terminou! Processo %s agora esta PRONTO.\n", nomes[id_acordado]);

                    // FIFO: desloca todo mundo para a frente 
                    for (int k = 0; k < tamanho_d1 - 1; k++) {
                        fila_d1[k] = fila_d1[k + 1];
                    }

                    tamanho_d1--; // A fila diminui
                }
            }

            else if (strcmp(buffer, "IRQ2") == 0) {
                if (tamanho_d2 > 0) {
                    // Pega o primeiro da Fila (o mais antigo)
                    int id_acordado = fila_d2[0];

                    // Ele sai da geladeira e volta para a fila da CPU (Roudin-Robin)
                    estado_processos[id_acordado] = 0;
                    printf("Kernel: Hardware D2 terminou! Processo %s agora esta PRONTO.\n", nomes[id_acordado]);

                    // FIFO: desloca todo mundo para a frente 
                    for (int k = 0; k < tamanho_d2 - 1; k++) {
                        fila_d2[k] = fila_d2[k + 1];
                    }

                    tamanho_d2--; // A fila diminui
                }
            }

            // (Mais tarde implementaremos o IRQ1 e IRQ2 aqui)
        }
    }
}

// ---------------------------------------------------------
// Ponto de Partida
// ---------------------------------------------------------
int main() {
    int fd[2]; // fd[0] é a boca de leitura, fd[1] é a boca de escrita
    pid_t pid_controller;

    // 1. Criar o pipe ANTES do fork (Crucial!)
    if (pipe(fd) == -1) {
        perror("Falha ao criar o pipe");
        exit(1);
    }

    printf("A iniciar o Simulador de SO...\n");

    // 2. O grande momento: dividir o processo em dois
    pid_controller = fork();

    if (pid_controller < 0) {
        perror("Falha no fork");
        exit(1);
    }

    if (pid_controller == 0) {
        // CÓDIGO DO FILHO: Vai atuar como InterController Sim
        close(fd[0]); // O filho não vai ler do pipe, por isso fechamos a leitura
        run_controller(fd[1]);
        exit(0);
    } else {
        // CÓDIGO DO PAI: Vai atuar como KernelSim
        // close(fd[1]); // O pai não vai escrever no pipe, por isso fechamos a escrita
        run_kernel(fd[0], fd[1]);

        // Como é um loop infinito, nunca chegará a este wait na prática
        wait(NULL);
    }

    return 0;
}