#include<lib.h>

void touch(char *path) {
    int fd = open(path, O_RDONLY);
    // 首先检查文件是否存在
    if (fd > 0) {
        close(fd);
        return;
    } else {
        // 文件不存在，创建文件
        fd = open(path, O_CREAT);
        if (fd < 0) {
            printf("touch: cannot touch '%s': No such file or directory\n", path);
            return;
        }
        close(fd);
    
    }
}

int main(int argc, char **argv) {
    int i;
    if (argc < 2) {
        printf("touch: missing file operand\n");
        return 0;
    }
    for (i = 1; i < argc; i++) {
        touch(argv[i]);
    }
}




