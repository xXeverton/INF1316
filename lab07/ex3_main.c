#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define DB_SIZE 1000
#define CACHE_SIZE 100
#define CACHE_HIT 1
#define CACHE_MISS 0
#define DB_SUCCESS 1
#define DB_ERROR 0
#define DELAY 1

typedef struct
{
    int key;
    int value;
    time_t timestamp;
} CacheEntry;

CacheEntry cache[CACHE_SIZE];
int banco_de_dados[DB_SIZE];
int is_valid[CACHE_SIZE] = {0};

// Função que trata o sinal Ctrl-Z (SIGTSTP)
void handle_sigtstp(int sig)
{
    printf("\n[SINAL] Cntl-Z interceptado! Limpando todo o cache...\n");
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        is_valid[i] = 0;
    }
}

// Simula leitura lenta no DB
int db_read(int key, int *out_value)
{
    if (key < 0 || key >= DB_SIZE)
        return DB_ERROR;
    *out_value = banco_de_dados[key];
    printf("[DB-READ] Lendo do BD -> Chave: %d, Valor: %d\n", key, *out_value);
    sleep(DELAY);
    return DB_SUCCESS;
}

// Simula escrita lenta no DB
int db_write(int key, int new_value)
{
    if (key < 0 || key >= DB_SIZE)
        return DB_ERROR;
    banco_de_dados[key] = new_value;
    printf("[DB-WRITE] Gravando no BD -> Chave: %d, Novo Valor: %d\n", key, new_value);
    sleep(DELAY);
    return DB_SUCCESS;
}

int main(void)
{

    for (int i = 0; i < DB_SIZE; i++)
    {
        banco_de_dados[i] = i;
    }

    signal(SIGTSTP, handle_sigtstp);

    FILE *file = fopen("log.txt", "r");
    if (!file)
    {
        printf("Erro ao abrir log.txt\n");
        return 1;
    }

    while (1)
    {
        char op;
        int key;
        int value;

        if ((fscanf(file, " %c", &op)) == EOF)
        {
            rewind(file);
            printf("\n REINICIANDO A LEITURA DO ARQUIVO LOG \n\n");
            continue;
        }

        // BLOCO ÚNICO PARA READ
        if (op == 'R')
        {
            fscanf(file, ", %d", &key);

            // Processa a lógica do Cache
            int index = key % CACHE_SIZE;

            if (is_valid[index] == 1 && cache[index].key == key)
            {
                printf("[APP] CACHE HIT (Chave %d): Retornou %d instantaneamente.\n", key, cache[index].value);
            }
            else
            {
                printf("[APP] CACHE MISS (Chave %d). ", key);

                int temp_value;
                if (db_read(key, &temp_value) == DB_SUCCESS)
                {
                    cache[index].key = key;
                    cache[index].value = temp_value;
                    cache[index].timestamp = time(NULL);
                    is_valid[index] = 1;
                }
            }
        }

        else if (op == 'W')
        {
            fscanf(file, ", %d, %d", &value, &key);
            printf("[APP] REQUISIÇÃO DE ESCRITA (Chave %d, Novo Valor %d). ", key, value);

            if (db_write(key, value) == DB_SUCCESS)
            {
                int index = key % CACHE_SIZE;

                cache[index].key = key;
                cache[index].value = value;          // Guardamos o novo valor diretamente no cache
                cache[index].timestamp = time(NULL); // Atualizamos o relógio
                is_valid[index] = 1;                 // Garantimos que a gaveta está marcada como válida
                printf("[APP] WRITE-THROUGH: Cache atualizado simultaneamente!\\n");
            }
        }
    }
    usleep(500000);
    fclose(file);
    return 0;
}