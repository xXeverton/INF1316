#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "common.h"

/*
 * INTERCONTROLLER SIM - Simulador de Controlador de Interrupções
 * 
 * Este processo emula o hardware responsavel por gerar interrupções:
 * 
 * IRQ0 (TimeSlice): Gerado periodicamente a cada 500ms
 *       Informa ao Kernel que a fatia de tempo de um processo terminou,
 *       forçando uma troca de contexto (preemption).
 * 
 * IRQ1 (Dispositivo D1): 10% de probablidade a cada 500ms
 *       Indica que uma operação de E/S no dispositivo D1 terminou.
 *       Como D1 é rápido, tem maior probabilidade que D2.
 * 
 * IRQ2 (Dispositivo D2): 5% de probabilidade a cada 500ms
 *       Indica que uma operação de E/S no dispositivo D2 terminou.
 *       D2 é 20x mais lento que D1.
 */

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
    }
}


