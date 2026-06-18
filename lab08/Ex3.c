#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

long total_size = 0;  // soma total dos tamanhos dos arquivos

// percorre recursivamente o diretório
void browsedir(const char *current_dir) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    char full_path[1024];

    // tenta abrir o diretório
    if (!(dir = opendir(current_dir))) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // ignora as entradas "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // monta o caminho completo do arquivo
        snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

        // obtém informações do arquivo/dir
        if (stat(full_path, &file_info) == -1)
            continue; 

        // se for diretório, chama a função recursivamente
        if (S_ISDIR(file_info.st_mode)) {
            browsedir(full_path);
        }
        // se for arquivo regular, soma seu tamanho
        else if (S_ISREG(file_info.st_mode)) {
            total_size += file_info.st_size;
        }
    }

    closedir(dir);
}

int main(void) {
    char current_path[1024];

    // obtém o diretório corrente
    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        puts(">>Error ao obter o dir atual");
        return 1;
    }

    browsedir(current_path);
    printf(">>> tamanho total dos arquivos encontrados = %ld bytes\n", total_size);

    return 0;
}