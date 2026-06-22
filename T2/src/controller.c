#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "common.h"

/*
 * INTERCONTROLLER SIM
 *
 * A cada 500ms gera interrupções com as probabilidades do enunciado T2:
 *   IRQ0 → sempre        (TimeSlice)
 *   IRQ1 → P = 10%       (D1 terminou)
 *   IRQ2 → P =  5%       (D2 terminou)
 *   IRQ3 → P = 50%       (Swap terminou) — frequente pois swap é rápido
 *
 * IRQ1, IRQ2 e IRQ3 são mutuamente exclusivos entre si dentro da mesma
 * rodada de sorteio, mas todos podem coexistir com IRQ0.
 */

void run_controller(int write_fd)
{
    signal(SIGTSTP, SIG_IGN); // Ignora Ctrl+Z no filho

    srand(time(NULL) ^ getpid());

    while (1)
    {
        usleep(500000); // 500ms

        // IRQ0: sempre (TimeSlice)
        char msg[] = "IRQ0";
        write(write_fd, msg, sizeof(msg));

        // Sorteio para IRQ1, IRQ2, IRQ3 (mutuamente exclusivos)
        int r = rand() % 100;

        if (r < 10)
        {
            // IRQ1: 10% — D1 terminou
            char irq1[] = "IRQ1";
            write(write_fd, irq1, sizeof(irq1));
        }
        else if (r < 15)
        {
            // IRQ2: 5% — D2 terminou
            char irq2[] = "IRQ2";
            write(write_fd, irq2, sizeof(irq2));
        }
        else if (r < 65)
        {
            // IRQ3: 50% — Swap terminou (r de 15 a 64 → 50 valores)
            char irq3[] = "IRQ3";
            write(write_fd, irq3, sizeof(irq3));
        }
        // else: os outros 35% → nenhuma interrupção extra neste ciclo
    }
}