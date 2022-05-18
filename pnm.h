
#ifndef SOBEL_PNM_H

#define SOBEL_PNM_H
#endif //SOBEL_PNM_H

extern char FORMAT_VERSION[4];
extern char MAX_VALUE_COLOR[5];

#include <stdio.h>
#include <fcntl.h>
#include "string.h"



int assertString(int fd, const char *source);
int assertMaxValueColor(int fd);
int assertFormatImage(int fd);
int skipComments(int fd);