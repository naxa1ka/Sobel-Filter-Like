#include <unistd.h>
#include "pnm.h"

char FORMAT_VERSION[4] = "P6\n\0";
char MAX_VALUE_COLOR[5] = "255\n\0";

int assertString(int fd, const char *source) {
    int length = (int) strlen(source);
    char buf[length];
    if (read(fd, buf, length) == -1) {
        return -1;
    }
    if (strcmp(source, buf) != 0) {
        return -1;
    }
    return 0;
}

int assertMaxValueColor(int fd) {
    return assertString(fd, MAX_VALUE_COLOR);
}

int assertFormatImage(int fd) {
    return assertString(fd, FORMAT_VERSION);
}

int skipComments(int fd) {
    char buf;
    while (1) {
        if (read(fd, &buf, sizeof(char)) == -1) {
            return -1;
        }
        if (buf != '#') {
            if (lseek(fd, (long) (-sizeof(char)), SEEK_CUR) == (off_t) -1) { //return pos
                return -1;
            }
            return 0;
        }
        while (buf != '\n') {
            if (read(fd, &buf, sizeof(char)) == -1) {
                return -1;
            }
        }
    }
}