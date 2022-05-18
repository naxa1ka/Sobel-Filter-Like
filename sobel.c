#include "sobel.h"

const float GRAYSCALE[] = {0.3f, 0.59f, 0.11f}; //sum must be 1

double convolution(unsigned char **array, int x, int y)    //Свертка изображения
{
    double xSum = 0;
    double ySum = 0;

    ySum += -1 * array[x - 1][y - 1];
    ySum += -2 * array[x - 1][y];
    ySum += -1 * array[x - 1][y + 1];

    ySum += 1 * array[x + 1][y - 1];
    ySum += 2 * array[x + 1][y];
    ySum += 1 * array[x + 1][y + 1];

    /***************************************************/

    xSum += -1 * array[x - 1][y - 1];
    xSum += -2 * array[x][y - 1];
    xSum += -1 * array[x + 1][y - 1];

    xSum += 1 * array[x - 1][y + 1];
    xSum += 2 * array[x][y + 1];
    xSum += 1 * array[x + 1][y + 1];

    return sqrt(xSum * xSum + ySum * ySum);
}

pixel readPixel(int fd) {
    pixel pixel;
    read(fd, &pixel.r, 1);
    read(fd, &pixel.g, 1);
    read(fd, &pixel.b, 1);
    return pixel;
}

unsigned char lum(pixel pixel) {
    return pixel.r * GRAYSCALE[0] + pixel.g * GRAYSCALE[1] + pixel.b * GRAYSCALE[2];
}