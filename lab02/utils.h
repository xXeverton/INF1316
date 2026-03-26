#include <stdio.h>

void errorShMat(void* sh)
{
    if(sh == (int*)-1){
        printf("Erro no attach\n");
        exit(1);
    }
}

void errorShGet(int shId)
{
    if(shId < 0){
        printf("Erro no shmget\n");
        exit(1);
    }
}
