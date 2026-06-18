#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

long total_size = 0;  // soma total dos tamanhos dos arquivos

// percorre recursivamente e mostra diretórios e arquivos com indentação
void browse_dir(const char *current_dir, int level) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;
    char full_path[1024];

    // tenta abrir o diretório
    if (!(dir = opendir(current_dir))) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // ignora "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // monta o caminho completo
        snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

        // obtém informações
        if (stat(full_path, &file_info) == -1)
            continue;

        // imprime nome com indentação proporcional ao nível
        printf("%*s[%s]\n", level * 2, "", entry->d_name);

        if (S_ISDIR(file_info.st_mode)) {
            // chamada recursiva para subdiretórios
            browse_dir(full_path, level + 1);
        } else if (S_ISREG(file_info.st_mode)) {
            total_size += file_info.st_size;  // soma tamanho de arquivos regulares
        }
    }

    closedir(dir);
}

int main(void) {
    char current_path[1024];

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("Error getting current directory");
        return 1;
    }

    printf("Conteúdos da árvore de dir:\n");
    browse_dir(current_path, 0);
    printf("\n>>> tamanho total dos arquivos encontrados  = %ld bytes\n", total_size);

    return 0;
}