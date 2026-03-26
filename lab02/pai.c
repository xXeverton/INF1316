#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

#define SEQ_INITIAL_FLAG 0
#define TRUE 1

typedef struct {
    int num;
    int seq;
} Dado;

Dado dado = {0, SEQ_INITIAL_FLAG};

int main(void) {
    int id_m1 = shmget(IPC_PRIVATE, sizeof(Dado), IPC_CREAT | 0666);
    int id_m2 = shmget(IPC_PRIVATE, sizeof(Dado), IPC_CREAT | 0666);
    Dado *pagina1 = (Dado*) shmat(id_m1, NULL, 0);
    if (pagina1 == (void*) -1) {
        perror("pai.c: shmat para pagina1 falhou");
        exit(1);
    }
    Dado *pagina2 = (Dado*) shmat(id_m2, NULL, 0);
    if (pagina2 == (void*) -1) {
        perror("pai.c: shmat para pagina2 falhou");
        exit(1);
    }
    *pagina1 = dado;
    *pagina2 = dado;

    for (int i = 0; i < 2; i++) {
        int pid = fork();

        if (pid < 0) {
            exit(1);
        }
        else if (pid == 0) {
            char buffer_string[20]; 
            char numero_filho_str[5];

            sprintf(numero_filho_str, "%d", i);
            if (i == 0) {
                // filho 1
                sprintf(buffer_string, "%d", id_m1);
                execlp("./filho", "filho", buffer_string, numero_filho_str, (char*) NULL);
            }
            else {
                // filho 2
                sprintf(buffer_string, "%d", id_m2);
                execlp("./filho", "filho", buffer_string, numero_filho_str, (char*) NULL);
            }
        }
    }

    int produto = 1;
    int ultimo_seq1_lido = SEQ_INITIAL_FLAG;
    int ultimo_seq2_lido = SEQ_INITIAL_FLAG;

while (TRUE) 
    {
        if (pagina1->seq > ultimo_seq1_lido && pagina2->seq > ultimo_seq2_lido)
        {
            produto = pagina1->num * pagina2->num;
            printf("produto = %d\n", produto);
            
            ultimo_seq1_lido = pagina1->seq;
            ultimo_seq2_lido = pagina2->seq;
            break;
        }
    }

    shmdt(pagina1);
    shmdt(pagina2);
    shmctl(id_m1, IPC_RMID, 0);
    shmctl(id_m2, IPC_RMID, 0);
    wait(NULL);
    wait(NULL);
    return 0;
}