#include<lib.h>

int current_offset = 0;
int furthest_offset = 0;

void init_history(){
    int fd = open(".mosh_history", O_CREAT);
    if(fd < 0){
        printf("Error creating history file\n");
        exit();
    }
    close(fd);
}


int writeEntry(int fd, int offset, const char *buf) {
    int length = strlen(buf);
    seek(fd, offset);
    write(fd, &length, 4);

}

int downEntry() {

}

int upEntry() {

}

int history_write() {

}

int history_get_all(char *history[]) {


}

int next_history() {

}

