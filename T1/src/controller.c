#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "common.h" // Importa as globais e assinaturas


// ---------------------------------------------------------
// Função do Fofoqueiro: InterController Sim
// ---------------------------------------------------------
void run_controller(int write_fd)
{
    // O Controlador ignora o Ctrl+Z para não duplicar o relatório
    signal(SIGTSTP, SIG_IGN);

    srand(time(NULL)); // Inicializa a semente para os números aleatórios
    char msg[50];

    while (1)
    {
        // Gera o TimeSlice (IRQ0) a cada 500ms (500.000 microssegundos)
        usleep(500000);
        strcpy(msg, "IRQ0");
        write(write_fd, msg, strlen(msg) + 1);

        // Sorteia interrupções de I/O (D1 e D2)
        int probabilidade = rand() % 100; // Gera um número de 0 a 99

        if (probabilidade < 10)
        {
            // 10% de probabilidade (P1 = 0.1)
            strcpy(msg, "IRQ1");
            write(write_fd, msg, strlen(msg) + 1);
        }
        else if (probabilidade >= 10 && probabilidade < 15)
        {
            // 5% de probabilidade (P2 = 0.05) -> do 10 ao 14 são 5 números
            strcpy(msg, "IRQ2");
            write(write_fd, msg, strlen(msg) + 1);
        }
    }
}


