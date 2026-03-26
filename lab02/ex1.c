#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define LINHA 3
#define COLUNA 3

void imprime_matrix(int *matriz)
{
    printf("\nMatriz Resultado (C):\n");
    for (int i = 0; i < LINHA; i++)
    {
        for (int j = 0; j < COLUNA; j++)
        {
            int index = (i * COLUNA) + j;
            printf("%d ", matriz[index]);
        }
        printf("\n");
    }
}

int main(void)
{
    int pid, mypid;
    int tamanho_matriz = LINHA * COLUNA * sizeof(int);
    int id_a, id_b, id_c;
    int *matriz_a;
    int *matriz_b;
    int *matriz_c;

    // IPC_PRIVATE: gera uma chave nova
    // S_IRUSR | S_IWUSR : garante permissão de leitura e escrita pro usuário
    id_a = shmget(IPC_PRIVATE, tamanho_matriz, IPC_CREAT | S_IRUSR | S_IWUSR);
    id_b = shmget(IPC_PRIVATE, tamanho_matriz, IPC_CREAT | S_IRUSR | S_IWUSR);
    id_c = shmget(IPC_PRIVATE, tamanho_matriz, IPC_CREAT | S_IRUSR | S_IWUSR);
    matriz_a = (int *)shmat(id_a, 0, 0);
    matriz_b = (int *)shmat(id_b, 0, 0);
    matriz_c = (int *)shmat(id_c, 0, 0);

    if (matriz_a == (int *)-1 || matriz_b == (int *)-1 || matriz_c == (int *)-1)
    {
        printf("Erro ao anexar meomória\n");
        exit(1);
    }

    for (int i = 0; i < LINHA; i++)
    {
        for (int j = 0; j < COLUNA; j++)
        {
            int index = (i * COLUNA) + j;
            matriz_a[index] = 1;
            matriz_b[index] = 1;
            matriz_c[index] = 0;
        }
    }

    for (int i = 0; i < LINHA; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            mypid = getpid();
            printf("PID Filho: %d - PROCESSO: %d\n", mypid, i);
            
            for (int j = 0; j < COLUNA; j++)
            {
                int index = (i * COLUNA) + j;
                matriz_c[index] = matriz_a[index] + matriz_b[index];
            }
            
            shmdt(matriz_a);
            shmdt(matriz_b);
            shmdt(matriz_c);
            exit(0);
        }
    }

    for (int i = 0; i < LINHA; i++) wait(NULL);

    imprime_matrix(matriz_c);

    shmdt(matriz_a);
    shmdt(matriz_b);
    shmdt(matriz_c);
    

    shmctl(id_a, IPC_RMID, 0);
    shmctl(id_b, IPC_RMID, 0);
    shmctl(id_c, IPC_RMID, 0);
    return 0;
}