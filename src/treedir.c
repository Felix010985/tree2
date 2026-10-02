#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LANGS 128
#define TYPE_DIR  0
#define TYPE_FILE 1
#define TYPE_ERR  2

typedef struct {
    char ext[16];
    char name[32];
    int count;
} LangStat;

LangStat stats[MAX_LANGS] = {
    {".c", "C", 0},
    {".h", "C Header", 0},
    {".cpp", "C++", 0},
    {".hpp", "C++ Header", 0},
    {".sh", "Shell", 0},
    {".json", "JSON", 0},
    {".css", "CSS", 0},
    {".zlt", "Zlata Index", 0},
    {".md", "Markdown", 0}
};
int totalFiles = 0;

int isDirectory(const char *dir_path) {
    DIR *dir = opendir(dir_path);

    if (dir) {
        return 0;
    }

    else {
        return 1;
    }
}

int getFileType(const char *path) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return TYPE_ERR;
    }

    if (S_ISDIR(st.st_mode)) {
        return TYPE_DIR;
    }

    if (S_ISREG(st.st_mode)) {
        return TYPE_FILE;
    }

    return TYPE_ERR;
}

void countFileLang(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return;

    for (int i = 0; i < MAX_LANGS; i++) {
        if (stats[i].ext[0] == '\0') break;
        if (strcmp(dot, stats[i].ext) == 0) {
            stats[i].count++;
            totalFiles++;
            break;
        }
    }
}

void newLine() { write(1, "\r\n", 2);}

void printTree(const char *dir_path, int level, unsigned long long prefixMask) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    struct dirent **namelist = NULL;
    int n = scandir(dir_path, &namelist, NULL, alphasort);

    if (n < 0) {
        closedir(dir);
        return;
    }

    int totalEntries = 0;
    for (int i = 0; i < n; i++) {
        if (strcmp(namelist[i]->d_name, ".") != 0 && strcmp(namelist[i]->d_name, "..") != 0) {
            totalEntries++;
        }
    }

    int currentEntry = 0;
    for (int i = 0; i < n; i++) {
        char *name = namelist[i]->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            free(namelist[i]);
            continue;
        }

        currentEntry++;
        int is_last = (currentEntry == totalEntries);

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dir_path, name);

        struct stat st;
        int is_dir = 0;
        int is_reg = 0;
        int is_exe = 0;

        if (stat(fullPath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                is_dir = 1;
            } else if (S_ISREG(st.st_mode)) {
                is_reg = 1;
                if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                    is_exe = 1;
                }
            }
        }

        for (int l = 0; l < level; l++) {
            if (prefixMask & (1ULL << l)) {
                printf("\033[37m│   \033[0m");
            } else {
                printf("    ");
            }
        }

        if (is_last) {
            printf("\033[37m└── \033[0m");
        } else {
            printf("\033[37m├── \033[0m");
        }

        if (is_dir) {
            printf("\033[1;34m%s\033[0m\n", name);
        } else if (is_exe) {
            printf("\033[1;32m%s\033[0m\n", name);
        } else {
            printf("\033[0m%s\n", name);
        }

        if (is_dir) {
            unsigned long long nextMask = prefixMask;
            if (!is_last) {
                nextMask |= (1ULL << level);
            } else {
                nextMask &= ~(1ULL << level);
            }
            printTree(fullPath, level + 1, nextMask);
        } else if (is_reg) {
            countFileLang(name);
        }

        free(namelist[i]);
    }
    free(namelist);
    closedir(dir);
}

int main(int argc, char *argv[]) {
    char *targetDir = ".";
    if (argc > 1) {
        targetDir = argv[1];
    }

    if (isDirectory(targetDir)) {
        printf("Not a directory oops.\n");
        return 1;
    }

    printf("\033[1;34m%s\033[0m\n", targetDir);
    printTree(targetDir, 0, 0);
    newLine();

    if (totalFiles == 0) {
        printf("Nothing found according to the table.\n");
    } else {
        printf("\033[1;97mLanguages:");
        printf("\033[0m");
        fflush(stdout);
        newLine();
        for (int i = 0; i < MAX_LANGS; i++) {

            if (stats[i].ext[0] == '\0') break;
            if (stats[i].count > 0) {
                float percentage = ((float)stats[i].count / totalFiles) * 100.0;

                printf("\033[1m%-12s\033[0m: %d files (%.1f%%)\n", stats[i].name, stats[i].count, percentage);
            }
        }
    }
    return 0;
}
