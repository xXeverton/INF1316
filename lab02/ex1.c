#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINHA 3
#define COLUNA 3

int main(void)
{
    int tamanho_matriz = LINHA * COLUNA * sizeof(int);
    int id_a, id_b, id_c;
    int* matriz_a, matriz_b, matriz_c;

    // IPC_PRIVATE: gera uma chave nova
    // S_IRUSR | S_IWUSR : garante permissão de leitura e escrita pro usuário
    id_a = shmget(IPC_PRIVATE, tamanho_matriz, IPC_CREAT | S_IRUSR | S_IWUSR);
    matriz_a = (int*)shmat(id_a, 0, 0);
    matriz_b = (int*)shmat(id_b, 0, 0);
    matriz_c = (int*)shmat(id_c, 0, 0);

    for (int i = 0; i < LINHA; i++)
    {
        // TODO: Endenter como inicializar uma matriz na parte de processos
        // for (int j = 0; j < COLUNA; j++)
        // {
        //     matriz_a[i][j] = 0;
        //     matriz_c[i][j] = 1;
        // }
    }



    return 0;
}