#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "common.h"

/*
 * KERNELSIM - T2
 *
 * Estados dos processos:
 *   0 = PRONTO      1 = EXECUTANDO
 *   2 = BLOQUEADO   3 = TERMINADO
 *
 * Novidades do T2:
 *   - Trata mensagem ACCESS (mem, op): MMU simulada + Page Fault
 *   - Algoritmos global_substitute() e local_substitute()
 *   - Fila de swap com contador irq3_pendentes para "dirty eviction"
 *   - modifyBit atualizado a cada ACCESS de escrita
 *   - campo when atualizado a cada page hit ou page fault resolvido
 */

// ── variáveis do kernel (extern no common.h) ──────────────────────────────

// ── variáveis locais ao kernel ────────────────────────────────────────────
static int fila_d1[5], tamanho_d1 = 0;
static int fila_d2[5], tamanho_d2 = 0;
static int quadros_ocupados = 0; // quantos dos 32 frames estão em uso
static int processo_atual = 0;

// ═══════════════════════════════════════════════════════════════════════════
// ALGORITMOS DE SUBSTITUIÇÃO DE PÁGINA
// ═══════════════════════════════════════════════════════════════════════════

/*
 * global_substitute()
 * Escolhe a vítima entre TODOS os 32 quadros da RAM.
 * Política: NRU simplificado — prefere páginas limpas (modifyBit=0)
 *           sobre páginas sujas (modifyBit=1).
 *           Dentro do mesmo grupo, escolhe a de menor "when" (LRU-like).
 * Retorna o índice do quadro vítima (0-31).
 */
int global_substitute(void)
{
    int vitima = -1;
    int menor_when = __INT_MAX__;

    // Primeira passagem: procura página limpa (modifyBit == 0)
    for (int f = 0; f < 32; f++)
    {
        int p = memoria_ram[f].id_processo;
        int pg = memoria_ram[f].pagina_logica;
        if (tabelas_paginas[p][pg].modifyBit == 0)
        {
            if (tabelas_paginas[p][pg].when < menor_when)
            {
                menor_when = tabelas_paginas[p][pg].when;
                vitima = f;
            }
        }
    }

    // Segunda passagem: se todas sujas, escolhe a menos recentemente usada
    if (vitima == -1)
    {
        menor_when = __INT_MAX__;
        for (int f = 0; f < 32; f++)
        {
            int p = memoria_ram[f].id_processo;
            int pg = memoria_ram[f].pagina_logica;
            if (tabelas_paginas[p][pg].when < menor_when)
            {
                menor_when = tabelas_paginas[p][pg].when;
                vitima = f;
            }
        }
    }

    return vitima;
}

/*
 * local_substitute(id_processo)
 * Escolhe a vítima SOMENTE entre as páginas do próprio processo Ax.
 * Mesma política NRU: limpa primeiro, depois LRU.
 * Se o processo ainda não tiver nenhuma página na RAM, cai no global.
 */
int local_substitute(int id_processo)
{
    int vitima = -1;
    int menor_when = __INT_MAX__;

    // Primeira passagem: página limpa do próprio processo
    for (int f = 0; f < 32; f++)
    {
        if (memoria_ram[f].id_processo == id_processo)
        {
            int pg = memoria_ram[f].pagina_logica;
            if (tabelas_paginas[id_processo][pg].modifyBit == 0 &&
                tabelas_paginas[id_processo][pg].when < menor_when)
            {
                menor_when = tabelas_paginas[id_processo][pg].when;
                vitima = f;
            }
        }
    }

    // Segunda passagem: página suja do próprio processo
    if (vitima == -1)
    {
        menor_when = __INT_MAX__;
        for (int f = 0; f < 32; f++)
        {
            if (memoria_ram[f].id_processo == id_processo)
            {
                int pg = memoria_ram[f].pagina_logica;
                if (tabelas_paginas[id_processo][pg].when < menor_when)
                {
                    menor_when = tabelas_paginas[id_processo][pg].when;
                    vitima = f;
                }
            }
        }
    }

    // Se processo não tem páginas na RAM ainda → usa global
    if (vitima == -1)
        vitima = global_substitute();

    return vitima;
}

// ═══════════════════════════════════════════════════════════════════════════
// TRATAMENTO DE ACESSO À MEMÓRIA (MMU SIMULADA)
// ═══════════════════════════════════════════════════════════════════════════

/*
 * trata_acesso_memoria(id_processo, pagina_logica, op, pc_atual)
 *
 * Chamado quando o Kernel recebe ACCESS de um processo.
 * Verifica a tabela de páginas e decide:
 *   - Page Hit  → atualiza modifyBit/when, libera processo (SIGCONT).
 *   - Page Fault → bloqueia processo, escolhe quadro (livre ou substituição),
 *                  enfileira na Sw_Queue com irq3_pendentes correto.
 */
static void trata_acesso_memoria(int id, int pag, char op, int pc_atual)
{
    if (tabelas_paginas[id][pag].valid == 1)
    {
        // ── PAGE HIT ──────────────────────────────────────────────────────
        printf("  [MMU] PAGE HIT: %s pag %d → frame %d\n",
               nomes[id], pag, tabelas_paginas[id][pag].frame);

        // Atualiza modifyBit se for escrita
        if (op == 'W')
            tabelas_paginas[id][pag].modifyBit = 1;

        // Atualiza when (usado pelo algoritmo de substituição)
        tabelas_paginas[id][pag].when = pc_atual;

        // Processo continua normalmente — não bloqueia
        return;
    }

    // ── PAGE FAULT ────────────────────────────────────────────────────────
    page_faults[id]++;
    printf(">>> [PAGE FAULT] %s pag %d — total faults: %d\n",
           nomes[id], pag, page_faults[id]);

    // Bloqueia o processo até o swap resolver
    kill(processos[id], SIGSTOP);
    estado_processos[id] = 2;

    // Monta a entrada da fila de swap
    EntradaSwap entrada;
    entrada.id_processo = id;
    entrada.pagina_logica = pag;

    if (quadros_ocupados < 32)
    {
        // ── Caso A: existe quadro livre ───────────────────────────────────
        // O quadro será atribuído quando IRQ3 chegar (swap lê a página).
        // Precisa de 1 IRQ3.
        entrada.irq3_pendentes = 1;
        printf("  [SWAP] %s pag %d enfileirada (quadro livre disponível, 1 IRQ3)\n",
               nomes[id], pag);
    }
    else
    {
        // ── Caso B: RAM cheia → precisa substituir ────────────────────────
        // Escolhe a vítima (use global ou local conforme desejado)
        int frame_vitima = global_substitute(); // ← troque por local_substitute(id) para testar local
        int id_vitima = memoria_ram[frame_vitima].id_processo;
        int pag_vitima = memoria_ram[frame_vitima].pagina_logica;

        printf("  [SUBST] Vítima: %s pag %d (frame %d, dirty=%d)\n",
               nomes[id_vitima], pag_vitima, frame_vitima,
               tabelas_paginas[id_vitima][pag_vitima].modifyBit);

        // Invalida a vítima na sua tabela de páginas
        tabelas_paginas[id_vitima][pag_vitima].valid = 0;
        tabelas_paginas[id_vitima][pag_vitima].frame = -1;

        // Já reserva o frame para a nova página
        tabelas_paginas[id][pag].frame = frame_vitima;

        // Se a vítima estava suja (dirty), precisa ser salva no swap ANTES
        // de carregar a nova página → dois IRQ3.
        if (tabelas_paginas[id_vitima][pag_vitima].modifyBit == 1)
        {
            entrada.irq3_pendentes = 2;
            duplo_page_faults[id]++;
            printf("  [SWAP] Página vítima está SUJA → 2 IRQ3 necessários\n");
        }
        else
        {
            entrada.irq3_pendentes = 1;
            printf("  [SWAP] Página vítima está LIMPA → 1 IRQ3 necessário\n");
        }

        // Limpa o dirty bit da vítima (será escrita no swap se dirty)
        tabelas_paginas[id_vitima][pag_vitima].modifyBit = 0;
    }

    // Enfileira na Sw_Queue (FCFS)
    fila_swap[tamanho_swap++] = entrada;

    // Escala outro processo enquanto esse espera pelo swap
    for (int j = 0; j < 5; j++)
    {
        processo_atual = (processo_atual + 1) % 5;
        if (estado_processos[processo_atual] == 0)
        {
            estado_processos[processo_atual] = 1;
            kill(processos[processo_atual], SIGCONT);
            printf("  [SCHED] Troca para %s enquanto %s espera swap\n",
                   nomes[processo_atual], nomes[id]);
            break;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// LEITOR DE PIPE
// ═══════════════════════════════════════════════════════════════════════════

int ler_mensagem_pipe(int fd, char *buffer)
{
    int i = 0;
    char c;
    while (read(fd, &c, 1) > 0)
    {
        buffer[i++] = c;
        if (c == '\0')
            return i;
    }
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
// KERNEL PRINCIPAL
// ═══════════════════════════════════════════════════════════════════════════

void run_kernel(int read_fd, int write_fd)
{
    signal(SIGTSTP, handle_sigtstp);

    printf("KernelSim T2 iniciado...\n");

    char fd_str[10];
    sprintf(fd_str, "%d", write_fd);

    // Cria os 5 processos de aplicação
    for (int i = 0; i < 5; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            exit(1);
        }

        if (pid == 0)
        {
            execl("./bin/app", "./bin/app", nomes[i], fd_str, NULL);
            perror("execl");
            exit(1);
        }
        else
        {
            processos[i] = pid;
            usleep(10000);
            kill(pid, SIGSTOP);
            printf("Kernel: %s (PID %d) criado e pausado\n", nomes[i], pid);
        }
    }

    // Inicia com o primeiro processo
    kill(processos[0], SIGCONT);
    estado_processos[0] = 1;
    printf("Kernel: Iniciando com %s\n", nomes[0]);

    char buffer[128];

    // ── LOOP PRINCIPAL ──────────────────────────────────────────────────
    while (1)
    {
        // Verifica processos terminados
        for (int i = 0; i < 5; i++)
        {
            if (estado_processos[i] == 3)
                continue;
            int status;
            if (waitpid(processos[i], &status, WNOHANG) > 0)
            {
                estado_processos[i] = 3;
                printf("\n>>> %s finalizou!\n\n", nomes[i]);
            }
        }

        int terminados = 0;
        for (int i = 0; i < 5; i++)
            if (estado_processos[i] == 3)
                terminados++;
        if (terminados == 5)
        {
            printf("\n=== TODOS OS PROCESSOS TERMINARAM ===\n");
            kill(0, SIGKILL);
        }

        if (ler_mensagem_pipe(read_fd, buffer) <= 0)
            continue;

        // ── IRQ0: TimeSlice ─────────────────────────────────────────────
        if (strcmp(buffer, "IRQ0") == 0)
        {
            if (estado_processos[processo_atual] == 1)
            {
                kill(processos[processo_atual], SIGSTOP);
                estado_processos[processo_atual] = 0;
            }
            for (int j = 0; j < 5; j++)
            {
                processo_atual = (processo_atual + 1) % 5;
                if (estado_processos[processo_atual] == 0)
                {
                    estado_processos[processo_atual] = 1;
                    kill(processos[processo_atual], SIGCONT);
                    printf("--- IRQ0: Rodando %s ---\n", nomes[processo_atual]);
                    break;
                }
            }
        }

        // ── ACCESS: acesso à memória virtual (MMU simulada) ─────────────
        else if (strncmp(buffer, "ACCESS", 6) == 0)
        {
            // Formato: "ACCESS A2 5 W"
            char nome[4];
            int pag_logica;
            char op;
            sscanf(buffer, "ACCESS %s %d %c", nome, &pag_logica, &op);
            int id = nome[1] - '1';

            // Atualiza mem do processo para o relatório
            mem_processos[id] = pag_logica;

            printf("Kernel recebeu: %s\n", buffer);
            trata_acesso_memoria(id, pag_logica, op, pc_processos[id]);
        }

        // ── UPDATE: atualiza PC e mem para o relatório ──────────────────
        else if (strncmp(buffer, "UPDATE", 6) == 0)
        {
            char nome[4];
            int num_pc, num_mem;
            sscanf(buffer, "UPDATE %s %d %d", nome, &num_pc, &num_mem);
            int id = nome[1] - '1';
            pc_processos[id] = num_pc;
            mem_processos[id] = num_mem;
        }

        // ── SYSCALL: I/O para D1 ou D2 ──────────────────────────────────
        else if (strncmp(buffer, "SYSCALL", 7) == 0)
        {
            printf("Kernel recebeu: %s\n", buffer);
            // Formato: "SYSCALL A1 D1 R"
            int id_bloqueado = buffer[9] - '1';
            char disp = buffer[12];
            char op = buffer[14];

            disp_bloqueado[id_bloqueado] = disp;
            oper_bloqueado[id_bloqueado] = op;

            if (disp == '1')
                io_counts_d1[id_bloqueado]++;
            else
                io_counts_d2[id_bloqueado]++;

            kill(processos[id_bloqueado], SIGSTOP);
            estado_processos[id_bloqueado] = 2;

            if (disp == '1')
            {
                fila_d1[tamanho_d1++] = id_bloqueado;
                printf("Kernel: %s → Fila D1\n", nomes[id_bloqueado]);
            }
            else
            {
                fila_d2[tamanho_d2++] = id_bloqueado;
                printf("Kernel: %s → Fila D2\n", nomes[id_bloqueado]);
            }

            if (processo_atual == id_bloqueado)
            {
                for (int j = 0; j < 5; j++)
                {
                    processo_atual = (processo_atual + 1) % 5;
                    if (estado_processos[processo_atual] == 0)
                    {
                        estado_processos[processo_atual] = 1;
                        kill(processos[processo_atual], SIGCONT);
                        printf("--- SYSCALL: Troca para %s ---\n",
                               nomes[processo_atual]);
                        break;
                    }
                }
            }
        }

        // ── IRQ1: D1 terminou ───────────────────────────────────────────
        else if (strcmp(buffer, "IRQ1") == 0)
        {
            if (tamanho_d1 > 0)
            {
                int id = fila_d1[0];
                estado_processos[id] = 0;
                printf(">>> IRQ1: %s DESBLOQUEADO (D1 pronto)\n", nomes[id]);
                for (int k = 0; k < tamanho_d1 - 1; k++)
                    fila_d1[k] = fila_d1[k + 1];
                tamanho_d1--;
            }
        }

        // ── IRQ2: D2 terminou ───────────────────────────────────────────
        else if (strcmp(buffer, "IRQ2") == 0)
        {
            if (tamanho_d2 > 0)
            {
                int id = fila_d2[0];
                estado_processos[id] = 0;
                printf(">>> IRQ2: %s DESBLOQUEADO (D2 pronto)\n", nomes[id]);
                for (int k = 0; k < tamanho_d2 - 1; k++)
                    fila_d2[k] = fila_d2[k + 1];
                tamanho_d2--;
            }
        }

        // ── IRQ3: Swap terminou ─────────────────────────────────────────
        else if (strcmp(buffer, "IRQ3") == 0)
        {
            if (tamanho_swap == 0)
                continue;

            EntradaSwap *frente = &fila_swap[0];

            // Decrementa o contador de IRQ3 pendentes
            frente->irq3_pendentes--;

            if (frente->irq3_pendentes > 0)
            {
                // Ainda precisa de mais IRQ3s (era dirty) → volta para o FINAL da fila
                printf(">>> IRQ3: %s pag %d ainda precisa de %d IRQ3(s) → volta ao fim da fila\n",
                       nomes[frente->id_processo], frente->pagina_logica,
                       frente->irq3_pendentes);
                EntradaSwap tmp = *frente;
                // Remove da frente
                for (int k = 0; k < tamanho_swap - 1; k++)
                    fila_swap[k] = fila_swap[k + 1];
                // Coloca no fim
                fila_swap[tamanho_swap - 1] = tmp;
                // tamanho_swap não muda
            }
            else
            {
                // ── Página finalmente carregada na RAM ───────────────────
                int id = frente->id_processo;
                int pag = frente->pagina_logica;

                // Remove da fila de swap
                for (int k = 0; k < tamanho_swap - 1; k++)
                    fila_swap[k] = fila_swap[k + 1];
                tamanho_swap--;

                // Determina o frame (pode já estar reservado se houve substituição)
                int frame;
                if (tabelas_paginas[id][pag].frame != -1)
                {
                    // Frame já foi reservado durante a substituição
                    frame = tabelas_paginas[id][pag].frame;
                }
                else
                {
                    // Pega o próximo frame livre
                    frame = quadros_ocupados;
                    quadros_ocupados++;
                }

                // Atualiza a tabela de páginas do processo
                tabelas_paginas[id][pag].valid = 1;
                tabelas_paginas[id][pag].frame = frame;
                tabelas_paginas[id][pag].modifyBit = 0; // recém-carregada = limpa
                tabelas_paginas[id][pag].when = pc_processos[id];

                // Atualiza a RAM
                memoria_ram[frame].id_processo = id;
                memoria_ram[frame].pagina_logica = pag;
                ram_free[frame] = 0;

                // Desbloqueia o processo
                estado_processos[id] = 0;
                printf(">>> IRQ3: %s pag %d carregada no frame %d — DESBLOQUEADO\n",
                       nomes[id], pag, frame);
            }
        }
    } // fim while
}

// ═══════════════════════════════════════════════════════════════════════════
// RELATÓRIO CTRL+Z
// ═══════════════════════════════════════════════════════════════════════════

void handle_sigtstp(int sig)
{
    (void)sig;
    printf("\n\n=======================================================\n");
    printf("     SIMULAÇÃO PAUSADA (Ctrl+Z) - STATUS DO SISTEMA    \n");
    printf("=======================================================\n");

    for (int i = 0; i < 5; i++)
    {
        printf("Processo [%s]:\n", nomes[i]);
        printf("  PC           : %d\n", pc_processos[i]);
        printf("  Memoria      : m%02d\n", mem_processos[i]);
        printf("  Acessos D1   : %d\n", io_counts_d1[i]);
        printf("  Acessos D2   : %d\n", io_counts_d2[i]);
        printf("  Page Faults  : %d\n", page_faults[i]);
        printf("  Duplos PF    : %d (custaram 2 IRQ3)\n", duplo_page_faults[i]);

        // Verifica se está na fila de swap
        int na_swap = 0, irq3_rest = 0;
        for (int k = 0; k < tamanho_swap; k++)
        {
            if (fila_swap[k].id_processo == i)
            {
                na_swap = 1;
                irq3_rest = fila_swap[k].irq3_pendentes;
                break;
            }
        }

        printf("  Estado       : ");
        switch (estado_processos[i])
        {
        case 0:
            printf("PRONTO\n");
            break;
        case 1:
            printf("EXECUTANDO\n");
            break;
        case 2:
            if (na_swap)
                printf("BLOQUEADO (Swap, %d IRQ3 restante(s))\n", irq3_rest);
            else
                printf("BLOQUEADO (D%c, op=%c)\n",
                       disp_bloqueado[i], oper_bloqueado[i]);
            break;
        case 3:
            printf("TERMINADO\n");
            break;
        }

        // Páginas na RAM deste processo
        printf("  Páginas na RAM: ");
        int tem = 0;
        for (int j = 0; j < 16; j++)
        {
            if (tabelas_paginas[i][j].valid)
            {
                printf("pag%d→f%d(%s) ", j, tabelas_paginas[i][j].frame,
                       tabelas_paginas[i][j].modifyBit ? "D" : "L");
                tem = 1;
            }
        }
        if (!tem)
            printf("nenhuma");
        printf("\n");

        printf("-------------------------------------------------------\n");
    }

    printf("\nPressione [ENTER] para retomar...\n");

    // Pausa todos os processos de aplicação enquanto relatório é exibido
    for (int i = 0; i < 5; i++)
        if (estado_processos[i] == 1 || estado_processos[i] == 0)
            kill(processos[i], SIGSTOP);

    // Lê um caractere do terminal para esperar o ENTER
    char c;
    read(STDIN_FILENO, &c, 1);

    // Retoma o processo que estava executando
    for (int i = 0; i < 5; i++)
        if (estado_processos[i] == 0)
            kill(processos[i], SIGCONT);
    if (estado_processos[processo_atual] != 2 && estado_processos[processo_atual] != 3)
        kill(processos[processo_atual], SIGCONT);

    printf("Retomando...\n=======================================================\n\n");
}