#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX 2000

/*
 * PROCESSO DE APLICAÇÃO (A1, A2, A3, A4, A5)
 * 
 * Este é um dos 5 processos que simulam aplicações do usuário.
 * Cada um executa um loop de MAX iterações, acessando endereços de memória
 * virtual aleatórios e ocasionalmente requisitando operações de E/S.
 * 
 * O processo comunica com o Kernel via pipe para:
 * - Informar seu estado atual (PC e memória em uso)
 * - Solicitar operações de E/S (syscall) quando necessário
 * 
 * O Kernel é responsável por pausar/retomar este processo usando
 * sinais SIGSTOP/SIGCONT.
 */

int main(int argc, char *argv[]) {
    // Recebemos o nome do processo (A1-A5) e o fd do pipe para comunicar com o Kernel
    char *nome_app = (argc > 1) ? argv[1] : "Desconhecido";
    int write_fd = (argc > 2) ? atoi(argv[2]) : -1;
    
    int pc = 0;
    int mem;
    char msg_syscall[50];
    
    // Inicializa o gerador de números aleatórios com base no PID
    // Assim cada processo tem sua própria sequência "aleatória"
    srand(time(NULL) ^ getpid());

    mem = rand() % 16;

    for (pc = 1; pc <= MAX; pc++) {
        // O enunciado pede para acessar uma nova página lógica apenas com certa probabilidade (ex: 40%)
        if (rand() % 100 < 40) {
            // Simula acesso a um endereço de memória virtual aleatória (m00 a m15)
            mem = rand() % 16; 
            
            // Sorteio de Leitura (R) ou Escrita (W) baseado no enunciado
            char op_memoria = (mem % 2 != 0) ? 'R' : 'W';
            
            printf("App [%s] rodando... PC: %d, Memoria acessada: m%02d (%c)\n", nome_app, pc, mem, op_memoria);
            // Envia o UPDATE para o Kernel agora contendo a operação (R ou W) no final
            if (write_fd != -1) {
                char msg_update[50];
                sprintf(msg_update, "UPDATE %s %d %d %c", nome_app, pc, mem, op_memoria);
                write(write_fd, msg_update, strlen(msg_update) + 1);
            }
            
        } else {
            // Se não acessou memória nova, ainda avisamos o Kernel para atualizar o PC da tela de status (Ctrl+Z)
            // Enviamos 'N' (None) no lugar da operação
            if (write_fd != -1) {
                char msg_update[50];
                sprintf(msg_update, "UPDATE %s %d %d N", nome_app, pc, mem);
                write(write_fd, msg_update, strlen(msg_update) + 1);
            }
        }
        // Com pequena probabilidade (5%), o processo solicita uma operação de E/S
        // Isso pode bloquear o processo até o hardware (simulado) responder
        if (rand() % 100 < 5 && write_fd != -1) {
            int d = rand() % 100;
            char dis = (d % 2 == 0) ? '1' : '2';  // Escolhe entre D1 ou D2
            char op;                               // Tipo de operação
            if (d % 3 == 0) op = 'R';
            else if (d % 3 == 1) op = 'W';
            else op = 'X';

            // Envia a requisição de syscall para o Kernel
            sprintf(msg_syscall, "SYSCALL %s D%c %c", nome_app, dis, op);
            write(write_fd, msg_syscall, strlen(msg_syscall) + 1);
            printf(">>> App [%s] disparou chamada de sistema: %s\n", nome_app, msg_syscall);
        }

        // Simula uma unidade de tempo de processamento
        sleep(1); 
    }
    
    return 0;
}