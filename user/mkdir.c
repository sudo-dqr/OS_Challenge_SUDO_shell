#include<lib.h>

void mkdir(char *path, int flag) {
    int fd = open(path, O_RDONLY); // 首先检查文件是否存在
    if (fd > 0) {
        close(fd);
        if (!flag) { 
            printf("mkdir: cannot create directory '%s': File exists\n", path);
        }
        return;
    } else { // 文件不存在，创建文件
        fd = open(path, O_MKDIR);
        if (fd < 0) {
            if (!flag) {
                printf("mkdir: cannot create directory '%s': No such file or directory\n", path);
                return;
            } else { // 递归创建目录,例如 "/nonexist/dqr/123"
                char *p = path;
                while (*p == '/') {
                    p++;
                }
                while (*p != '\0') {
                    while (*p != '/' && *p != '\0') {
                        p++;
                    }
                    char tmp = *p;
                    *p = '\0';
                    mkdir(path, 1);
                    *p = tmp;
                }
            }
        }
        close(fd);
    }
}

int main(int argc, char **argvs) {
    if (argc < 2) {
        printf("mkdir: missing operand\n");
        return 0;
    }
    // 需要考虑参数p忽略错误
    if (strcmp(argvs[1], "-p") == 0) {
        mkdir(argvs[2], 1);
    } else {
        mkdir(argvs[1], 0);
    }
    return 0;
}