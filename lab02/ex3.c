#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#define TAM 20
#define NPROC 4

int main() {
    int shmid, i, j, chave = 7;
    int *vetor;

    // cria memória compartilhada
    shmid = shmget(1234, TAM * sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        printf("Erro shmget");
        exit(1);
    }

    vetor = (int *) shmat(shmid, NULL, 0);

    // preenche o vetor desordenado
    int dados[TAM] = {5, 3, 9, 1, 7, 8, 2, 4, 6, 0, 11, 15, 7, 13, 2, 7, 19, 18, 17, 16};

    for (i = 0; i < TAM; i++) {
        vetor[i] = dados[i];
    }

    printf("Buscando chave: %d\n", chave);

    int tamanho_bloco = TAM / NPROC;

    // cria processos
    for (i = 0; i < NPROC; i++) {

        if (fork() == 0) {

            int inicio = i * tamanho_bloco;
            int fim = inicio + tamanho_bloco;

            for (j = inicio; j < fim; j++) {
                
                if (vetor[j] == chave) {
                    printf("Processo %d encontrou na posicao %d\n", getpid(), j);
                }
            }

            exit(0);
        }
    }

    // pai espera todos
    for (i = 0; i < NPROC; i++) {
        wait(NULL);
    }

    // libera memória
    shmdt(vetor);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}