#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX 2000 // Valor de teste pedido no enunciado

int main(int argc, char *argv[]) {
    // Vamos passar o nome/ID da app por argumento (ex: A1, A2...)
    char *nome_app = (argc > 1) ? argv[1] : "Desconhecido";
    
    int pc = 0;
    int mem;
    
    srand(time(NULL) ^ getpid()); // Inicializa a seed aleatória única por processo

    printf("App [%s] iniciada com PID %d\n", nome_app, getpid());

    for (pc = 1; pc <= MAX; pc++) {
        // Simula a escolha de uma página de memória aleatória (m00-m15)
        mem = rand() % 16; 

        printf("App [%s] rodando... PC: %d, Memoria acessada: m%02d\n", nome_app, pc, mem);
        
        // Simula o processamento que leva 1 segundo
        sleep(1); 
    }

    printf("App [%s] terminou sua execução.\n", nome_app);
    return 0;
}