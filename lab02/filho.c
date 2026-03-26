#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h> // Para rand() e srand()
#include <sys/wait.h>
#include <time.h>

#define NUM_FILHO_1 3
#define NUM_FILHO_2 8
#define SEQ_INITIAL_FLAG 0

typedef struct
{
    int num;
    int seq;
} Dado;

int main(int argc, char *argv[])
{
    int id = atoi(argv[1]);
    int num_filho = atoi(argv[2]);
    Dado *pagina = (Dado *)shmat(id, NULL, 0);
    srand(time(NULL) * getpid());
    int tempo_em_microssegundos = (rand() % 1000000) + 500000;
    usleep(tempo_em_microssegundos);

    Dado dado;
    if (num_filho == 0)
    {
        Dado dado;
        dado.num = rand() % 10; // Gera um número aleatório (ex: de 0 a 9)
        dado.seq = SEQ_INITIAL_FLAG + 1;
        *pagina = dado;

        printf("Filho %d gerou o numero: %d\n", num_filho, dado.num);
    }
    else
    {
        dado.num = NUM_FILHO_2;
        dado.seq = SEQ_INITIAL_FLAG + 1;
        *pagina = dado;
    }

    shmdt(pagina);
    return 0;
}