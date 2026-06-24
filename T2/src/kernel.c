/*
 * KERNELSIM - Simulador de Kernel Preemptivo
 *
 * Gerencia 5 processos de aplicação usando escalonamento Round-Robin com preemption.
 * Responde a três tipos de interrupções:
 * - IRQ0: TimeSlice (força troca de contexto)
 * - IRQ1: Hardware D1 completa uma operação (desbloqueia processos)
 * - IRQ2: Hardware D2 completa uma operação (desbloqueia processos)
 *
 * Estados dos processos:
 * 0 = PRONTO (aguardando CPU)
 * 1 = EXECUTANDO (em posse da CPU)
 * 2 = BLOQUEADO (aguardando E/S)
 * 3 = TERMINADO (finalizou seus MAX ciclos)
 */

/*
 * Lê uma mensagem completa do pipe terminada em \0
 * As mensagens vêm do InterController (interrupções) ou dos processos (UPDATE/SYSCALL)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "common.h"

// Globais no kernel.c
int fila_swap[5];
int contador_swap[5];
int tamanho_swap = 0;
int duplo_page_faults[5] = {0, 0, 0, 0, 0};
int ponteiro_global = 0;
int ponteiro_local[5] = {0, 0, 0, 0, 0};


int ler_mensagem_pipe(int fd, char *buffer)
{
    int i = 0;
    char c;
    // Lê 1 único caractere por vez
    while (read(fd, &c, 1) > 0)
    {
        buffer[i++] = c;
        if (c == '\0')
        {
            return i; // Mensagem completa encontrada e separada!
        }
    }
    return 0; // Pipe vazio ou fechado
}

// Algoritmo Global: Prefere roubar quadros limpos, não importa de quem seja
int global_substitute(QuadroRAM RAM[]) {
    // 1ª Passagem: Procura um quadro com modifyBit == 0 (página limpa)
    for (int i = 0; i < 32; i++) {
        int indice = (ponteiro_global + i) % 32;
        int id_dono = RAM[indice].id_processo;
        int pag_dono = RAM[indice].pagina_logica;
        
        // Se a página está limpa, escolhe ela para não tomar penalidade de 2 IRQ3
        if (tabelas_paginas[id_dono][pag_dono].modifyBit == 0) {
            ponteiro_global = (indice + 1) % 32;
            return indice; 
        }
    }
    
    // 2ª Passagem: Se TODAS as páginas da RAM estão sujas, pega a primeira apontada pelo relógio
    int quadro_vitima = ponteiro_global;
    ponteiro_global = (ponteiro_global + 1) % 32;
    return quadro_vitima;
}
// Algoritmo Local: Tenta substituir apenas as páginas do PRÓPRIO processo
int local_substitute(QuadroRAM RAM[], int id_processo) {
    // 1ª Passagem: Procura quadro do próprio processo com modifyBit == 0
    for (int i = 0; i < 32; i++) {
        int indice = (ponteiro_local[id_processo] + i) % 32;
        
        if (RAM[indice].id_processo == id_processo) {
            int pag_dono = RAM[indice].pagina_logica;
            if (tabelas_paginas[id_processo][pag_dono].modifyBit == 0) {
                ponteiro_local[id_processo] = (indice + 1) % 32;
                return indice;
            }
        }
    }
    
    // 2ª Passagem: Se não achou limpa, pega a primeira suja que seja do próprio processo
    for (int i = 0; i < 32; i++) {
        int indice = (ponteiro_local[id_processo] + i) % 32;
        
        if (RAM[indice].id_processo == id_processo) {
            ponteiro_local[id_processo] = (indice + 1) % 32;
            return indice;
        }
    }
    
    // Fallback: Se o processo não tem NENHUM quadro na RAM, recai no global.
    return global_substitute(RAM);
}

// ----- INICIALIZAÇÃO DO KERNELSIM E CRIAÇÃO DOS PROCESSOS -----

void run_kernel(int read_fd, int write_fd)
{
    signal(SIGTSTP, handle_sigtstp);

    char buffer[50];
    printf("KernelSim iniciado e a aguardar interrupções...\n");

    char fd_str[10];
    sprintf(fd_str, "%d", write_fd);

    // Cria os 5 processos de aplicação
    for (int i = 0; i < 5; i++)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            printf("Erro ao criar processo filho\n");
            exit(1);
        }

        if (pid == 0)
        {
            // Filho executa o programa da aplicação
            execl("./bin/app", "./bin/app", nomes[i], fd_str, NULL);
            perror("Falha no execl\n");
            exit(1);
        }
        else
        {
            // Pai armazena o PID e pausa imediatamente
            processos[i] = pid;
            usleep(10000);
            kill(pid, SIGSTOP);
            printf("Kernel: Processo %s (PID %d) criado e pausado\n", nomes[i], pid);
        }
    }

    // Filas para processos bloqueados aguardando I/O
    int fila_d1[5], tamanho_d1 = 0;
    int fila_d2[5], tamanho_d2 = 0;

    // Escalonador Round-Robin: começa pelo primeiro processo
    int processo_atual = 0;
    kill(processos[processo_atual], SIGCONT);
    estado_processos[processo_atual] = 1;
    printf("Kernel: Iniciando a execução com %s (PID %d)\n", nomes[processo_atual], processos[processo_atual]);

    // Contador para saber se quadro preencheu
    int quadros_ocupados = 0;                   // vai de 0 a 32

    // ----- LOOP PRINCIPAL: PROCESSA INTERRUPÇÕES E GERENCIA PROCESSOS -----

    while (1)
    {
        int status;

        // Verifica se algum processo finalizou sua execução
        for (int i = 0; i < 5; i++)
        {
            if (estado_processos[i] != 3)
            {
                if (waitpid(processos[i], &status, WNOHANG) > 0)
                {
                    estado_processos[i] = 3;
                    printf("\n>>> Kernel: Processo %s finalizou sua execucao! <<<\n\n", nomes[i]);
                }

                // Verifica se todos os processos terminaram
                int processos_terminados = 0;
                for (int i = 0; i < 5; i++)
                {
                    if (estado_processos[i] == 3)
                        processos_terminados++;
                }

                // Se todos terminaram, encerra o simulador
                if (processos_terminados == 5)
                {
                    printf("\n=======================================================\n");
                    printf(">>> TODOS OS PROCESSOS APLICAÇÃO TERMINARAM <<<\n");
                    printf(">>> INICIANDO DESLIGAMENTO DO KERNEL...     <<<\n");
                    printf("=======================================================\n");
                    kill(0, SIGKILL);
                }
            }
        }

        // Aguarda uma mensagem do pipe
        if (ler_mensagem_pipe(read_fd, buffer) > 0)
        {

            // IRQ0: TimeSlice terminou - força troca de contexto (preemption)
            if (strcmp(buffer, "IRQ0") == 0)
            {
                // Pausa o processo atual se estiver executando
                if (estado_processos[processo_atual] == 1)
                {
                    kill(processos[processo_atual], SIGSTOP);
                    estado_processos[processo_atual] = 0;
                }

                // Encontra o próximo processo PRONTO (em round-robin)
                for (int j = 0; j < 5; j++)
                {
                    processo_atual = (processo_atual + 1) % 5;
                    if (estado_processos[processo_atual] == 0)
                    {
                        estado_processos[processo_atual] = 1;
                        kill(processos[processo_atual], SIGCONT);
                        printf("--- Troca de Contexto por fim do TimeSlice (IRQ0): Agora rodando %s ---\n", nomes[processo_atual]);
                        break;
                    }
                }
            }

            // UPDATE: Recebe informação de estado de um processo (PC, memória e Operação R/W)
            else if (strncmp(buffer, "UPDATE", 6) == 0)
            {
                char pid_str[3];
                int num_pc, num_mem;
                char op_memoria;
                
                // Agora o sscanf pega o %c no final da string!
                sscanf(buffer, "UPDATE %s %d %d %c", pid_str, &num_pc, &num_mem, &op_memoria);
                int id_processo = pid_str[1] - '1';

                pc_processos[id_processo] = num_pc;
                mem_processos[id_processo] = num_mem;

                // ====================================================================
                // T2: MMU SIMULADA E TRATAMENTO DE PAGE FAULT
                // Só testamos se o processo estiver ativamente a correr e se de fato houve acesso (diferente de N)
                // ====================================================================
                if (estado_processos[id_processo] == 1 && op_memoria != 'N')
                {
                    // Verifica na Tabela de Páginas se a página Lógica (num_mem) está na RAM

                    kill(processos[id_processo], SIGSTOP);
                    
                    if (tabelas_paginas[id_processo][num_mem].valid == 0)
                    {
                        // PAGE FAULT! A página não está na RAM.
                        printf(">>> [PAGE FAULT] Processo %s (pag logica %d) nao esta na RAM!\n", nomes[id_processo], num_mem);
                        page_faults[id_processo]++;
                        int quadro_escolhido = -1;
                        int precisa_dois_irq3 = 0;
                        int ram_estava_cheia = 0; // Nova variável para controlar o fluxo!
                        // --- PASSO 4.a: Procura espaço vazio ---
                        if (quadros_ocupados < 32) {
                            quadro_escolhido = quadros_ocupados;
                            ram_free[quadro_escolhido] = 0;
                            quadros_ocupados++;
                        } 
                        // --- PASSO 4.b: RAM cheia, usar algoritmo de substituição ---
                        else {
                            ram_estava_cheia = 1; // Marcamos que houve substituição
                            
                            quadro_escolhido = global_substitute(memoria_ram);
                            // quadro_escolhido = local_substitute(memoria_ram, id_processo);

                            int id_vitima = memoria_ram[quadro_escolhido].id_processo;
                            int pag_vitima = memoria_ram[quadro_escolhido].pagina_logica;
                            // Verifica se a vítima estava modificada ANTES de invalidar
                            if (tabelas_paginas[id_vitima][pag_vitima].modifyBit == 1) {
                                precisa_dois_irq3 = 1; // Vítima suja! Vai precisar de 2 IRQ3
                                duplo_page_faults[id_processo]++;
                            }
                            // Invalida a página da vítima
                            tabelas_paginas[id_vitima][pag_vitima].valid = 0;
                            tabelas_paginas[id_vitima][pag_vitima].frame = -1;
                        }
                        // Atualiza as tabelas com o novo quadro (Comum a ambos os casos)
                        tabelas_paginas[id_processo][num_mem].valid = 1;
                        tabelas_paginas[id_processo][num_mem].frame = quadro_escolhido;
                        if (op_memoria == 'W') {
                            tabelas_paginas[id_processo][num_mem].modifyBit = 1;
                        } else {
                            tabelas_paginas[id_processo][num_mem].modifyBit = 0;
                        }
                        tabelas_paginas[id_processo][num_mem].when = num_pc; 
                        
                        memoria_ram[quadro_escolhido].id_processo = id_processo;
                        memoria_ram[quadro_escolhido].pagina_logica = num_mem;
                        // --- PASSO 4.c: Ações dependentes de como o quadro foi obtido ---
                        if (ram_estava_cheia) {
                            // Se ocorreu substituição, o processo VAI PARA O SWAP e é BLOQUEADO
                            fila_swap[tamanho_swap] = id_processo;
                            contador_swap[tamanho_swap] = precisa_dois_irq3;
                            tamanho_swap++;
                            
                            // (Correção 1: Bloqueio e Troca de contexto realocada para cá!)
                            estado_processos[id_processo] = 2;
                            if (processo_atual == id_processo) {
                                for (int j = 0; j < 5; j++) {
                                    processo_atual = (processo_atual + 1) % 5;
                                    if (estado_processos[processo_atual] == 0) {
                                        estado_processos[processo_atual] = 1;
                                        kill(processos[processo_atual], SIGCONT);
                                        printf("--- Troca de Contexto por Swap: Agora rodando %s ---\n", nomes[processo_atual]);
                                        break;
                                    }
                                }
                            }
                        } else {
                            // PASSO 4.a: Apenas achou quadro vazio, atende imediatamente
                            // O enunciado pede expressamente para dar SIGCONT aqui.
                            kill(processos[id_processo], SIGCONT);
                            printf(">>> Kernel: Quadro vazio preenchido (Sem ir pro Swap!). Processo %s continua executando.\n", nomes[id_processo]);
                        }
                    }

                    else
                    {
                        // PAGE HIT! A página já está num quadro da RAM. Tudo OK.
                        
                        // Se o processo acessou a página para ESCREVER, agora sim ela fica suja!
                        if (op_memoria == 'W') {
                            tabelas_paginas[id_processo][num_mem].modifyBit = 1;
                        }
                        
                        // Atualiza o PC (when) para registrar a última vez que ela foi acessada
                        tabelas_paginas[id_processo][num_mem].when = num_pc;

                        kill(processos[id_processo], SIGCONT);
                    }
                }
                // ====================================================================
            }

                        // SYSCALL: Um processo solicita uma operação de E/S e é bloqueado
            else if (strncmp(buffer, "SYSCALL", 7) == 0)
            {
                // Formato: "SYSCALL A1 D1 R"
                int id_bloqueado = buffer[9] - '1';

                // Se o processo não estiver EXECUTANDO (status 1), 
                // significa que ele já tomou um Page Fault e foi bloqueado
                // pela MMU instantes antes dessa mensagem de Syscall chegar.
                if (estado_processos[id_bloqueado] != 1) {
                    // Nós simplesmente ignoramos esse "Syscall fantasma" 
                    // e pulamos para ler a próxima mensagem do pipe.
                    continue; 
                }
                // ==========================================================

                printf("Kernel recebeu: %s \n", buffer);

                disp_bloqueado[id_bloqueado] = buffer[12];
                oper_bloqueado[id_bloqueado] = buffer[14];

                // Conta os acessos a cada dispositivo (para o relatório)
                if (buffer[12] == '1')
                    io_counts_d1[id_bloqueado]++;
                else if (buffer[12] == '2')
                    io_counts_d2[id_bloqueado]++;

                // Pausa o processo imediatamente
                kill(processos[id_bloqueado], SIGSTOP);
                estado_processos[id_bloqueado] = 2;

                // Coloca na fila de espera do dispositivo apropriado
                if (buffer[12] == '1')
                {
                    fila_d1[tamanho_d1++] = id_bloqueado;
                    printf("Kernel: Processo %s foi bloqueado e colocado na Fila D1.\n", nomes[id_bloqueado]);
                }
                else if (buffer[12] == '2')
                {
                    fila_d2[tamanho_d2++] = id_bloqueado;
                    printf("Kernel: Processo %s foi bloqueado e colocado na Fila D2.\n", nomes[id_bloqueado]);
                }

                // Se o processo que pediu I/O era o que estava executando,
                // precisa escolher outro para rodar na CPU
                if (processo_atual == id_bloqueado)
                {
                    for (int j = 0; j < 5; j++)
                    {
                        processo_atual = (processo_atual + 1) % 5;
                        if (estado_processos[processo_atual] == 0)
                        {
                            estado_processos[processo_atual] = 1;
                            kill(processos[processo_atual], SIGCONT);
                            printf("--- Troca de Contexto FORCADA por Syscall: Agora rodando %s ---\n", nomes[processo_atual]);
                            break;
                        }
                    }
                }
            }

            // IRQ1: Dispositivo D1 completou uma operação de E/S
            else if (strcmp(buffer, "IRQ1") == 0)
            {
                if (tamanho_d1 > 0)
                {
                    // Desbloqueia o primeiro processo que está aguardando D1
                    int id_acordado = fila_d1[0];
                    estado_processos[id_acordado] = 0;
                    printf(
                        ">>> Kernel: Hardware D1 terminou! Processo %s foi DESBLOQUEADO e agora esta PRONTO.\n",
                        nomes[id_acordado]);

                    // Remove o processo da fila (FIFO)
                    for (int k = 0; k < tamanho_d1 - 1; k++)
                    {
                        fila_d1[k] = fila_d1[k + 1];
                    }
                    tamanho_d1--;
                }
            }

            // IRQ2: Dispositivo D2 completou uma operação de E/S
            else if (strcmp(buffer, "IRQ2") == 0)
            {
                if (tamanho_d2 > 0)
                {
                    // Desbloqueia o primeiro processo que está aguardando D2
                    int id_acordado = fila_d2[0];
                    estado_processos[id_acordado] = 0;
                    printf(
                        ">>> Kernel: Hardware D2 terminou! Processo %s foi DESBLOQUEADO e agora esta PRONTO.\n",
                        nomes[id_acordado]);

                    // Remove o processo da fila (FIFO)
                    for (int k = 0; k < tamanho_d2 - 1; k++)
                    {
                        fila_d2[k] = fila_d2[k + 1];
                    }
                    tamanho_d2--;
                }
            }

            // IRQ3: Disco termina de buscar/salvar a página
            else if (strcmp(buffer, "IRQ3") == 0)
            {
                if (tamanho_swap > 0)
                {
                    int id_primeiro = fila_swap[0];
                    
                    if (contador_swap[0] > 0) {
                        // Recebeu o primeiro IRQ3, mas precisava salvar a página suja antes!
                        printf(">>> Kernel: Swap (IRQ3) salvou pagina suja. Processo %s tem que aguardar e volta pro final da fila.\n", nomes[id_primeiro]);
                        
                        // Decrementa o contador (agora é 0, no próximo IRQ3 ele vai passar)
                        contador_swap[0]--;
                        
                        // Move o processo para o final da fila (Rotaciona a fila)
                        int temp_id = fila_swap[0];
                        int temp_cont = contador_swap[0];
                        
                        for (int k = 0; k < tamanho_swap - 1; k++) {
                            fila_swap[k] = fila_swap[k + 1];
                            contador_swap[k] = contador_swap[k + 1];
                        }
                        
                        fila_swap[tamanho_swap - 1] = temp_id;
                        contador_swap[tamanho_swap - 1] = temp_cont;
                    }
                    else {
                        // Contador é 0! A paginação terminou de fato. Desbloqueia o processo.
                        estado_processos[id_primeiro] = 0;
                        printf(">>> Kernel: Disco de Swap concluiu (IRQ3)! Processo %s foi DESBLOQUEADO e agora esta PRONTO.\n", nomes[id_primeiro]);

                        // Remove da fila de swap
                        for (int k = 0; k < tamanho_swap - 1; k++) {
                            fila_swap[k] = fila_swap[k + 1];
                            contador_swap[k] = contador_swap[k + 1];
                        }
                        tamanho_swap--;
                    }
                }
            }
        }
    }
}

// ----- RELATÓRIO DE ESTADO (EXIBIDO AO APERTAR CTRL+Z) -----

void handle_sigtstp(int sig)
{
    printf("\n\n=======================================================\n");
    printf("     SIMULAÇÃO PAUSADA (Ctrl+Z) - STATUS DO SISTEMA    \n");
    printf("=======================================================\n");

    for (int i = 0; i < 5; i++)
    {

        printf("Processo [%s]:\n", nomes[i]);
        printf("  - PC Atual      : %d\n", pc_processos[i]);
        printf("  - Memoria       : m%02d\n", mem_processos[i]);
        printf("  - Acessos a D1  : %d vezes\n", io_counts_d1[i]);
        printf("  - Acessos a D2  : %d vezes\n", io_counts_d2[i]);

        int esperando_swap = 0;
        for (int k = 0; k < tamanho_swap; k++)
        {
            if (fila_swap[k] == i)
            {
                esperando_swap = 1;
            }
        }

        printf("  - Estado        : ");
        if (estado_processos[i] == 0)
            printf("PRONTO\n");
        else if (estado_processos[i] == 1)
            printf("EXECUTANDO (CPU)\n");

        else if (estado_processos[i] == 2)
        {
            if (esperando_swap == 1)
            {
                printf("BLOQUEADO (Aguardando Disco de Swap)\n");
            }
            else
            {
                printf("BLOQUEADO (Aguardando D%c, operacao %c)\n", disp_bloqueado[i], oper_bloqueado[i]);
            }
        }

        else if (estado_processos[i] == 3)
            printf("TERMINADO\n");

       // ==========================================
        // ESTATÍSTICAS DO TRABALHO 2
        // ==========================================
        printf("  - Page Faults   : %d totais\n", page_faults[i]);
        printf("  - Duplos P.F.   : %d (custaram 2 IRQ3)\n", duplo_page_faults[i]);

        printf("-------------------------------------------------------\n");
    }

    printf("\nPressione [ENTER] para retomar a simulação...\n");
    getchar();

    printf("Retomando simulação...\n");
    printf("=======================================================\n\n");
}