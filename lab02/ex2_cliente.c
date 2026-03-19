#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    int segmento;
    char *p;

    segmento = shmget(8752, 1024, 0666); 
    if (segmento == -1) {
        printf("Erro ao localizar o segmento.");
        exit(1);
    }

    p = (char *) shmat(segmento, 0, 0);
    if (p == (char *) -1) {
        printf("Erro no shmat");
        exit(1);
    }

    printf("Mensagem do Dia recebida: %s\n", p);

    shmdt(p);

    return 0;
}