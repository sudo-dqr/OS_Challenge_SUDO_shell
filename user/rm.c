#include <lib.h>

#define N 0
#define R 1
#define RF 2

void rm(char *path, int flag) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (flag != RF) {
            printf("rm: cannot remove '%s': No such file or directory\n", path);
        }
    } else {
        close(fd);
        /*为了避免传参问题 将模式拼接在路径末尾\0之后*/
        char path1[MAXPATHLEN+1];
        strcpy(path1, path);
        int len = strlen(path1);
        path1[++len] = flag + '0';
        /********************/
        fd = remove(path1);
        if (fd < 0) {
            printf("rm: cannot remove '%s': Is a directory\n", path);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("rm: missing operand\n");
    }
    if (strcmp(argv[1], "-r") == 0) {
        rm(argv[2], R);
    } else if (strcmp(argv[1], "-rf") == 0) {
        rm(argv[2], RF);
    } else {
        rm(argv[1], N);
    }
    return 0;
}
