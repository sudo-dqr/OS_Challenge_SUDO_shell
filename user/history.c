#include<lib.h>

#define HISTORY_SIZE 20

char *history[HISTORY_SIZE] = {0};

int main(int argc, char **argv) {
    int length = history_get_all(history);
    for (int i = 0; i < length; i++) {
        printf("%s\n", history[i]);
    }
    return 0;
}