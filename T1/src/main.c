#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

// ---------------------------------------------------------
// Função do Fofoqueiro: InterController Sim
// ---------------------------------------------------------
void run_controller(int write_fd) {
    srand(time(NULL)); // Inicializa a semente para os números aleatórios
    char msg[10];

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
void run_kernel(int read_fd) {
    char buffer[10];
    printf("KernelSim iniciado e a aguardar interrupções...\n");

    while(1) {
        // Lê do pipe bloqueando até chegar uma nova mensagem
        if (read(read_fd, buffer, sizeof(buffer)) > 0) {
            printf("Kernel recebeu: %s\n", buffer);
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
        close(fd[1]); // O pai não vai escrever no pipe, por isso fechamos a escrita
        run_kernel(fd[0]);

        // Como é um loop infinito, nunca chegará a este wait na prática
        wait(NULL);
    }

    return 0;
}