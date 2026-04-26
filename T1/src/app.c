#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX 2000

int main(int argc, char *argv[]) {
    char *nome_app = (argc > 1) ? argv[1] : "Desconhecido";
    int write_fd = (argc > 2) ? atoi(argv[2]) : -1; // Pega o número do pipe!
    
    int pc = 0;
    int mem;
    char msg_syscall[50];
    
    srand(time(NULL) ^ getpid());

    for (pc = 1; pc <= MAX; pc++) {
        mem = rand() % 16; 
        printf("App [%s] rodando... PC: %d, Memoria acessada: m%02d\n", nome_app, pc, mem);
        
        // PROBABILIDADE DE SYSCALL (ex: 5% de chance de pedir I/O a cada ciclo)
        if (rand() % 100 < 5 && write_fd != -1) {
            int d = rand() % 100;
            char dis = (d % 2 == 0) ? '1' : '2'; // Dispositivo D1 ou D2
            char op;
            if (d % 3 == 0) op = 'R';
            else if (d % 3 == 1) op = 'W';
            else op = 'X';

            // Monta a mensagem: "SYSCALL A1 D1 R"
            sprintf(msg_syscall, "SYSCALL %s D%c %c", nome_app, dis, op);
            
            // Manda pelo pipe para o Kernel e avisa na tela
            write(write_fd, msg_syscall, strlen(msg_syscall) + 1);
            printf(">>> App [%s] disparou chamada de sistema: %s\n", nome_app, msg_syscall);
            
            // Ao pedir I/O, o processo precisa parar e esperar a resposta do Kernel
            // Como o Kernel vai dar o SIGSTOP quase imediatamente ao ler o pipe,
            // podemos apenas deixar o fluxo seguir, o Kernel cuida de parar ele.
        }

        sleep(1); 
    }
    return 0;
}