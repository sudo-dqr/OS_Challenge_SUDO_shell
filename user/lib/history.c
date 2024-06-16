#include<lib.h>

#define MAX_HISTORY 20

void init_history(){
    int fd = open(".mosh_history", O_CREAT);
    if(fd < 0){
        printf("Error creating history file\n");
        exit();
    }
    close(fd);
}
