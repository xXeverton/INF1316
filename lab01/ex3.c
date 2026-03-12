#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    
    int pid, status, i, j, temp;
    int vetor[10] = {7, 9, 2, 3, 1, 8, 4, 6, 5};

    printf("Vetor (antes do fork):\n");
    for(int i = 0; i < 10; i++){
        printf("%d ", vetor[i]);
    }
    printf("\n");

    pid = fork();

    if (pid != 0){

        waitpid(pid, &status, 0);

        printf("Pai: vetor depois do wait:\n");
        for(i = 0; i < 10; i++){
            printf("%d ", vetor[i]);
        }
        printf("\n");
        
        if (!WIFEXITED(status)) printf("O processo nao terminou corretamente\n");
    }

    else {

        for (i = 0; i < 9; i++) {
            for (j = 0; j < 9 - i; j++) {
                if (vetor[j] > vetor[j + 1]) {
                    temp = vetor[j];
                    vetor[j] = vetor[j + 1];
                    vetor[j + 1] = temp;
                }
            }
        }
        
        printf("Filho: vetor ordenado: ");
        for(i = 0; i < 10; i++){
            printf("%d ", vetor[i]);
        }
        printf("\n");

        exit(0);
    }

    return 0;
}