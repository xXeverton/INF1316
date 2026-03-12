#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    
    int mypid, pid, status; //cria as variáveis básicas
    int numero = 1;

    printf("Numero = %d (antes do processo pai)\n", numero);

    pid = fork(); //chama a função fork e cria um novo processo

    if (pid != 0) { //se o processo for o pai:
    
        mypid = getpid(); //pai recebe o seu pid
        printf("PID Pai: %d\n", mypid);

        waitpid(-1, &status, 0); //pausa o processo pai e pula para o filho
        printf("Numero = %d (apos processo filho)\n", numero);
        
        if (!WIFEXITED(status)) printf("O processo nao terminou corretamente\n");
    }

    else {

        numero = 5;

        mypid = getpid(); //filho recebe pid
        printf("Filho PID: %d\n", mypid);
        
        printf("Numero = %d (durante processo filho)\n", numero);
        exit(0); //acaba processo do filho
    }

    return 0;
}