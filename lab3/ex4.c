#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define TROCA_CONT 10

int main(void) {
    pid_t filho1, filho2;
    int trocas = 0;
    
    // primeiro processo filho
    if ((filho1 = fork()) == 0) {
        printf("filho 1 (pid: %d) iniciado\n", getpid());
        fflush(stdout);

        kill(getpid(), SIGSTOP); // começa parado

        while(1) {
            printf("filho 1 (pid: %d) executando\n", getpid());
            fflush(stdout);
            sleep(1);
        }
    }

    // pausa para garantir que filho1 imprima antes de criar filho2
    sleep(1);
    
    // segundo processo filho
    if ((filho2 = fork()) == 0) {
        printf("filho 2 (pid: %d) iniciado\n", getpid());
        fflush(stdout);

        kill(getpid(), SIGSTOP);

        while(1) {
            printf("filho 2 (pid: %d) executando\n", getpid());
            fflush(stdout);
            sleep(1);
        }
    }
    
    // processo pai
    if (filho1 > 0 && filho2 > 0) {
        printf("pai (pid: %d) criou filhos: %d e %d\n", getpid(), filho1, filho2);
        
        // inicia só o primeiro filho 
        kill(filho1, SIGCONT);
        kill(filho2, SIGSTOP); // garante que filho2 fique parado
        printf("filho 1 iniciado, filho 2 parado\n");
        
        sleep(2);
        
        while (trocas < TROCA_CONT) {
            printf("\n--- troca número %d ---\n", trocas + 1);
            
            if (trocas % 2 == 0) {
                kill(filho1, SIGSTOP);
                kill(filho2, SIGCONT);
                printf("para filho 1 e continua filho 2\n");
            } else {
                kill(filho2, SIGSTOP);
                kill(filho1, SIGCONT);
                printf("para filho 2 e continua filho 1\n");
            }
            
            trocas++;
            sleep(2);
        }
        
        printf("\nfim depois de %d trocas...\n", TROCA_CONT);
        kill(filho1, SIGKILL);
        kill(filho2, SIGKILL);
        
        wait(NULL);
        wait(NULL);
    }
    
    return 0;
}