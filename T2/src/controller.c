#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "common.h"

/*
 * INTERCONTROLLER SIM - Simulador de Controlador de Interrupções (T2)
 * 
 * Este processo emula o hardware responsavel por gerar interrupções:
 * 
 * IRQ0 (TimeSlice): Gerado periodicamente a cada 500ms
 *       Informa ao Kernel que a fatia de tempo de um processo terminou,
 *       forçando uma troca de contexto (preemption).
 * 
 * IRQ1 (Arquivos): 10% de probablidade a cada 500ms
 *       Emula a conclusão de uma operação sobre Arquivos (read, write) no servidor remoto SFSS.
 * 
 * IRQ2 (Diretórios): 2% de probabilidade a cada 500ms
 *       Emula a conclusão de uma operação sobre Diretórios (add, rem, listdir) no servidor remoto SFSS.
 */

void run_controller(int write_fd)
{
    // Ignora Ctrl+Z para evitar duplicar relatórios
    signal(SIGTSTP, SIG_IGN);

    srand(time(NULL));
    char msg[3];

    while (1)
    {
        // Aguarda 500ms antes de gerar novas interrupções
        usleep(500000);
        
        // IRQ0: TimeSlice
        // Sempre gerado, sincroniza o escalonamento
        strcpy(msg, "IRQ0");
        write(write_fd, msg, strlen(msg) + 1);

        // Sorteia interrupções de hardware/rede de forma aleatória (0 a 99)
        int probabilidade = rand() % 100;

        // IRQ1: 10% de probabilidade (Conclusão de operação em Arquivo)
        if (probabilidade < 10)
        {
            strcpy(msg, "IRQ1");
            write(write_fd, msg, strlen(msg) + 1);
        }
        // IRQ2: 2% de probabilidade (Conclusão de operação em Diretório)
        // Se não caiu nos primeiros 10% (0 a 9), testamos se caiu nos próximos 2% (10 ou 11)
        else if (probabilidade < 12)
        {
            strcpy(msg, "IRQ2");
            write(write_fd, msg, strlen(msg) + 1);
        }
    }
}


