#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>


int main() {
    int segmento;
    char *p; //char para guardar a msg

    segmento = shmget(8752, 1024, IPC_CREAT | 0666);
    if (segmento == -1) {
        printf("Erro ao criar segmento");
        exit(1);
    }

    p = (char *) shmat(segmento, 0, 0); //linka a memória compartilhada com o ponteiro
    if (p == (char *) -1) {
        printf("Erro no shmat");
        exit(1);
    }

    printf("Digite a mensagem do dia: ");
    fgets(p, 1024, stdin); 

    shmdt(p);

    return 0;
}