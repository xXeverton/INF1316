#define MAX_KEY 999
#define MIN_KEY 0

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum
{
    READ,
    WRITE
} OpType;


// Função que gera e exibe N operações aleatórias
void gerar_operacoes(int n)
{
    // Inicializa o gerador de números aleatórios com o tempo atual
    srand(time(NULL));
    for (int i = 0; i < n; i++)
    {
        // Escolhe aleatoriamente entre READ (0) e WRITE (1)
        OpType op = rand() % 2;
        // Gera key aleatória no intervalo de 0 a 999
        int key = (rand() % (MAX_KEY - MIN_KEY + 1)) + MIN_KEY;
        if (op == READ)
        {
            // Formato pedido: R, key
            printf("R, %d\n", key);
        }
        else
        {
            // Gera um valor inteiro aleatório qualquer (ex: entre 10 e 5000)
            int value = (rand() % 4991) + 10;
            // Formato pedido: W, value, key
            printf("W, %d, %d\n", value, key);
        }
    }
}


int main() {
    // Redireciona a saída padrão (printf) para o arquivo "log.txt" no modo escrita ("w")
    freopen("log.txt", "w", stdout);
    
    // Chama a função para gerar as 10.000 operações solicitadas no exercício
    gerar_operacoes(10000);
    
    // Opcional: fechar o redirecionamento
    fclose(stdout);
    
    return 0;
}