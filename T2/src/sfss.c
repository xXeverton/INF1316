#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include "common.h"

#define BUFSIZE 1024
#define ROOT_DIR "./SFSS-root-dir"

void error(char *msg) {
    perror(msg);
    exit(1);
}

// Função auxiliar para preencher arquivo com espaços (0x20) se o offset for maior que o arquivo
void preencher_com_espacos_se_necessario(int fd, int offset) {
    struct stat st;
    fstat(fd, &st);
    if (offset > st.st_size) {
        lseek(fd, st.st_size, SEEK_SET);
        int diff = offset - st.st_size;
        char *espacos = malloc(diff);
        memset(espacos, 0x20, diff); // Preenche com ' ' (0x20)
        write(fd, espacos, diff);
        free(espacos);
    }
}

int main(int argc, char **argv) {
    int sockfd, portno = 8080; // Porta padrão
    socklen_t clientlen; // Corrigido tipo para socklen_t
    struct sockaddr_in serveraddr, clientaddr;
    SFP_Message msg; // A nossa estrutura do T2!

    // Criação do Socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) error("ERRO ao abrir o socket do servidor");

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
        error("ERRO no binding do servidor");

    printf(">>> SFSS (Servidor Stateless UDP) rodando na porta %d...\n", portno);

    mkdir(ROOT_DIR, 0777);
    char subdir[64];
    for (int i = 0; i <= 5; i++) {
        sprintf(subdir, "%s/A%d", ROOT_DIR, i);
        mkdir(subdir, 0777);
        sprintf(subdir, "%s/A%d/dir_teste", ROOT_DIR, i);
        mkdir(subdir, 0777);
    }

    printf(">>> Os arquivos serao salvos no diretorio base: %s\n", ROOT_DIR);

    clientlen = sizeof(clientaddr);
    
    // --- LOOP PRINCIPAL DO SERVIDOR ---
    while (1) {
        // 1. RECEBE O DATAGRAMA DA REDE
        int n = recvfrom(sockfd, &msg, sizeof(SFP_Message), 0, (struct sockaddr *) &clientaddr, &clientlen);
        if (n < 0) error("ERRO no recvfrom");

        // 2. MONTA O CAMINHO REAL (Mapeamento do Servidor)
        char real_path[256];
        sprintf(real_path, "%s%s", ROOT_DIR, msg.path); 
        printf("\nSFSS: Recebido %s do App %d para o caminho: %s\n", msg.op_type, msg.owner, msg.path);

        // 3. EXECUTA AS OPERAÇÕES STATELESS
        if (strncmp(msg.op_type, "RD-REQ", 6) == 0) {
            strcpy(msg.op_type, "RD-REP"); // Prepara a resposta
            
            int fd = open(real_path, O_RDONLY);
            if (fd < 0) {
                msg.offset = -1; // Erro: arquivo não existe
            } else {
                lseek(fd, msg.offset, SEEK_SET);
                memset(msg.payload, 0, MAX_PAYLOAD);
                int lidos = read(fd, msg.payload, 16); // Lê no máximo 16 bytes
                if (lidos < 0) msg.offset = -1;
                close(fd);
            }
        } 
        else if (strncmp(msg.op_type, "WR-REQ", 6) == 0) {
            strcpy(msg.op_type, "WR-REP");

            memset(msg.payload, 0, MAX_PAYLOAD);
            
            // Regra do enunciado: Se offset 0 e payload vazio, remove o arquivo
            if (msg.offset == 0 && strlen(msg.payload) == 0) {
                if (unlink(real_path) < 0) msg.offset = -1;
            } else {
                int fd = open(real_path, O_WRONLY | O_CREAT, 0666);
                if (fd < 0) {
                    msg.offset = -1;
                } else {
                    preencher_com_espacos_se_necessario(fd, msg.offset);
                    lseek(fd, msg.offset, SEEK_SET);
                    write(fd, msg.payload, 16); // Escreve os dados
                    close(fd);
                }
            }
        }
        else if (strncmp(msg.op_type, "DC-REQ", 6) == 0) {
            strcpy(msg.op_type, "DC-REP");
            
            char full_dir[512];
            sprintf(full_dir, "%s/%s", real_path, msg.dirname);
            if (mkdir(full_dir, 0777) == 0) {
                char temp_path[256];
                sprintf(temp_path, "%s/%s", msg.path, msg.dirname); // Atualiza com novo sufixo
                strcpy(msg.path, temp_path);
                msg.path_len = strlen(msg.path);
            } else {
                msg.path_len = -1; // Código de erro
            }
        }
        else if (strncmp(msg.op_type, "DR-REQ", 6) == 0) {
            strcpy(msg.op_type, "DR-REP");
            
            char full_dir[512];
            sprintf(full_dir, "%s/%s", real_path, msg.dirname);
            // Tenta remover como diretorio, se falhar tenta como arquivo (unlink)
            if (rmdir(full_dir) < 0 && unlink(full_dir) < 0) {
                msg.path_len = -1; // Erro
            }
        }
        else if (strncmp(msg.op_type, "DL-REQ", 6) == 0) {
            strcpy(msg.op_type, "DL-REP");
            
            DIR *dir = opendir(real_path);
            msg.nrnames = 0;
            memset(msg.payload, 0, MAX_PAYLOAD);
            int cursor = 0;

            if (dir == NULL) {
                msg.nrnames = -1; // Erro ao abrir pasta
            } else {
                struct dirent *ent;
                // Lê até 40 itens da pasta
                while ((ent = readdir(dir)) != NULL && msg.nrnames < 40) {
                    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
                    
                    int len = strlen(ent->d_name);
                    strcpy(&msg.payload[cursor], ent->d_name);
                    
                    // Salva posição de início, fim e tipo (T2)
                    msg.fstlstpositions[msg.nrnames][0] = cursor;
                    msg.fstlstpositions[msg.nrnames][1] = cursor + len - 1;
                    msg.fstlstpositions[msg.nrnames][2] = (ent->d_type == DT_DIR) ? 1 : 0; // 1=Dir, 0=File
                    
                    cursor += len;
                    msg.nrnames++;
                }
                closedir(dir);
            }
        }

        // 4. DEVOLVE A RESPOSTA PARA O KERNEL
        sendto(sockfd, &msg, sizeof(SFP_Message), 0, (struct sockaddr *) &clientaddr, clientlen);
        printf("SFSS: Resposta %s devolvida para o KernelSim.\n", msg.op_type);
    }

    return 0;
}