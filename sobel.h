
#include "math.h"
#include <unistd.h>

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} pixel;

double convolution(unsigned char **array, int x, int y);
unsigned char lum(pixel pixel);
pixel readPixel(int fd);