#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "common.h"

void run_controller(int write_fd)
{
    // Ignora Ctrl+Z para evitar duplicar relatórios
    signal(SIGTSTP, SIG_IGN);

    srand(time(NULL));
    char msg[50];

    while (1)
    {
        // Aguarda 500ms antes de gerar novas interrupções
        usleep(500000);
        
        // IRQ0: TimeSlice
        // Sempre gerado, sincroniza o escalonamento
        strcpy(msg, "IRQ0");
        write(write_fd, msg, strlen(msg) + 1);

        // Sorteia interrupções de E/S de forma aleatória
        int probabilidade = rand() % 100;

        // IRQ1: 10% de probabilidade (D1 é rápido)
        if (probabilidade < 10)
        {
            strcpy(msg, "IRQ1");
            write(write_fd, msg, strlen(msg) + 1);
        }
        // IRQ2: 5% de probabilidade (D2 é lento)
        else if (probabilidade < 15)
        {
            strcpy(msg, "IRQ2");
            write(write_fd, msg, strlen(msg) + 1);
        }
        // IRQ3: 50% de probabilidade 
        else if (probabilidade < 65)
        {
            strcpy(msg, "IRQ3");
            write(write_fd, msg, strlen(msg) + 1);
        }
    }
}


