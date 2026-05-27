#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Necessário para brk() e sbrk()

// Guarda o endereço inicial do nosso pool de memória
void *base_do_heap;

// Marca onde o heap começa antes de fazermos qualquer alocação
void iniciar_pool_memoria(void) {
    base_do_heap = sbrk(0); // sbrk(0) apenas retorna o topo atual do heap
    if (base_do_heap == (void *)-1) {
        perror("Erro ao capturar a base do heap");
        exit(1);
    }
}

// Move o topo do heap para frente, "alocando" n bytes
void *alocar_memoria(int n) {
    void *topo_antigo = sbrk(n); 
    if (topo_antigo == (void *)-1) {
        perror("Erro ao expandir o heap");
        return NULL;
    }
    return topo_antigo; // Retorna onde o novo bloco começa
}

// Libera tudo de uma vez, voltando o topo do heap para a base inicial
void resetar_pool_memoria(void) {
    if (brk(base_do_heap) == -1) { 
        perror("Erro ao restaurar o heap inicial");
    } else {
        printf("\n>>> Memória liberada com sucesso! <<<\n");
    }
}

int main(void) {
    // Desativa o buffer do printf para ele não chamar malloc() internamente
    // e bagunçar o nosso controle manual do heap.
    setvbuf(stdout, NULL, _IONBF, 0);

    iniciar_pool_memoria();
    printf("Base inicial do heap: %p\n\n", base_do_heap);

    printf("--- Iniciando alocações ---\n");
    
    // Alocando espaço para 5 inteiros
    int *numeros = (int *)alocar_memoria(5 * sizeof(int));
    printf("Array de inteiros alocado em: %p\n", numeros);
    printf("Novo topo do heap:            %p\n\n", sbrk(0));

    // Alocando espaço para 50 caracteres
    char *texto = (char *)alocar_memoria(50 * sizeof(char));
    printf("String alocada em:            %p\n", texto);
    printf("Novo topo do heap:            %p\n\n", sbrk(0));

    // Testando a área alocada para garantir que está utilizável
    if (numeros != NULL) {
        numeros[0] = 42;
        numeros[4] = 99;
        printf("Valores gravados com sucesso: nums[0]=%d, nums[4]=%d\n", numeros[0], numeros[4]);
    }

    // Limpeza geral de tudo o que foi alocado acima
    resetar_pool_memoria();
    printf("Topo do heap após o reset:    %p\n", sbrk(0));

    return 0;
}