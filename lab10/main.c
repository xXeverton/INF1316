#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define DB_SIZE 1000
#define CACHE_SIZE 100
#define CACHE_HIT 1
#define CACHE_MISS 0
#define DB_SUCCESS 1
#define DB_ERROR 0
#define DELAY 1 // Mude para 10 se quiser o tempo exato do slide

// Estrutura do Cache conforme os slides
typedef struct {
    int key;
    int value;
    time_t timestamp;
} CacheEntry;

// Arrays globais para o BD, Cache e Validação
CacheEntry cache[CACHE_SIZE];
int is_valid[CACHE_SIZE] = {0};
int banco_de_dados[DB_SIZE];

// Função que trata o sinal Ctrl-Z (SIGTSTP)
void handle_sigtstp(int sig) {
    printf("\n[SINAL] Cntl-Z interceptado! Limpando todo o cache...\n");
    for(int i = 0; i < CACHE_SIZE; i++) {
        is_valid[i] = 0; // Invalida todas as posições
    }
}

// Verifica se o dado está no cache
int get_cache(int key, int* out_value) {
    int index = key % CACHE_SIZE; // Mapeamento direto
    if (is_valid[index] && cache[index].key == key) {
        *out_value = cache[index].value;
        cache[index].timestamp = time(NULL);
        return CACHE_HIT;
    }
    return CACHE_MISS;
}

// Salva um dado no cache
void set_cache(int key, int value) {
    int index = key % CACHE_SIZE;
    cache[index].key = key;
    cache[index].value = value;
    cache[index].timestamp = time(NULL);
    is_valid[index] = 1;
}

// Invalida uma entrada específica do cache (usado após escritas no DB)
void invalidate_cache(int key) {
    int index = key % CACHE_SIZE;
    if (is_valid[index] && cache[index].key == key) {
        is_valid[index] = 0;
    }
}

// Simula leitura lenta no DB
int db_read(int key, int* out_value) {
    if (key < 0 || key >= DB_SIZE) return DB_ERROR;
    *out_value = banco_de_dados[key];
    printf("[DB-READ] Lendo do BD -> Chave: %d, Valor: %d\n", key, *out_value);
    sleep(DELAY);
    return DB_SUCCESS;
}

// Simula escrita lenta no DB
int db_write(int key, int new_value) {
    if (key < 0 || key >= DB_SIZE) return DB_ERROR;
    banco_de_dados[key] = new_value;
    printf("[DB-WRITE] Gravando no BD -> Chave: %d, Novo Valor: %d\n", key, new_value);
    sleep(DELAY);
    return DB_SUCCESS;
}

int main() {
    // 1. Inicializa o banco de dados com valores {1..1000}
    for(int i = 0; i < DB_SIZE; i++) {
        banco_de_dados[i] = i + 1;
    }

    // 2. Intercepta o Ctrl-Z para a função de limpeza
    signal(SIGTSTP, handle_sigtstp);

    // 3. Abre o arquivo de log gerado no passo anterior
    FILE *file = fopen("log.txt", "r");
    if (!file) {
        printf("Erro ao abrir log.txt. Execute log_gen primeiro!\n");
        return 1;
    }

    char line[256];
    
    // 4. Loop infinito processando os logs ciclicamente
    while (1) {
        // Se chegar ao final do arquivo, volta ao começo
        if (fgets(line, sizeof(line), file) == NULL) {
            rewind(file);
            printf("\n--- REINICIANDO A LEITURA DO ARQUIVO LOG ---\n\n");
            continue;
        }

        int key, value;
        
        // Verifica qual o tipo de operação com base na string
        if (sscanf(line, "R, %d", &key) == 1) {
            int read_val;
            // Implementação Cache-aside para Leitura
            if (get_cache(key, &read_val) == CACHE_HIT) {
                printf("[APP] CACHE HIT (Chave %d): Retornou %d instantaneamente.\n", key, read_val);
            } else {
                printf("[APP] CACHE MISS (Chave %d). ", key);
                if (db_read(key, &read_val) == DB_SUCCESS) {
                    set_cache(key, read_val);
                }
            }
        } else if (sscanf(line, "W, %d, %d", &value, &key) == 2) {
            // Implementação Cache-aside para Escrita (Atualiza DB e Invalida Cache)
            printf("[APP] REQUISIÇÃO DE ESCRITA (Chave %d, Novo Valor %d). ", key, value);
            if (db_write(key, value) == DB_SUCCESS) {
                invalidate_cache(key);
            }
        }
    }

    fclose(file);
    return 0;
}