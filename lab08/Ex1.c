#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

int file_select(const struct dirent *entry)
{
    if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        return (FALSE);
    else
        return (TRUE);
}

int main()
{
    char pathname[MAXPATHLEN];
    struct dirent **files;
    int count, i;
    struct stat info;
    time_t agora = time(NULL);

    if (getcwd(pathname, sizeof(pathname)) == NULL) {
        printf("Error getting path\n");
        exit(0);
    }

    printf("Current Working Directory = %s\n", pathname);

    count = scandir(pathname, &files, file_select, alphasort);

    if (count <= 0) {
        printf("No files in this directory\n");
        exit(0);
    }

    printf("Number of files = %d\n", count);

    for (i = 0; i < count; ++i) {
        char fullpath[MAXPATHLEN + 1 + 256];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", pathname, files[i]->d_name);

        if (stat(fullpath, &info) == 0) {
            double idade = difftime(agora, info.st_mtime) / (60 * 60 * 24);
            printf("%-15s inode %lu  size: %ld  age: %.0f days\n",
                   files[i]->d_name,
                   (unsigned long)info.st_ino,
                   (long)info.st_size,
                   idade);
        }

        free(files[i]);
    }

    free(files);

    return 0;
}