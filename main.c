#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include "time.h"
#include "sobel.h"
#include "pnm.h"

typedef double resType;
typedef unsigned char bufType;

int WIDTH;
int HEIGHT;

int numThreads;

int INPUT_FD = -1;
int OUTPUT_FD = -1;

pthread_t *THREADS = NULL;
resType **RES = NULL;
bufType **BUF = NULL;
int **ARGS = NULL;

const int NS_PER_SECOND = 1000000000;

void diffTime(struct timespec t1, struct timespec t2, struct timespec *td);

int readAndWriteNumber(int *number);

void mallocThreads();

void clear(int isError);

void imageToGrayScale();

void fillRes();

void writeResult(double max);

void printError();

void printMessageError(const char *messageIfErrorNotExists);

char *readNumber(int *number);

void mallocRes();

void mallocBuf();

void mallocArgs();

void *lineConvolution(void *arg);

int *laneDistribution(int countParts, int length);

int fillArgs(int **args);


int main(int argc, char *argv[]) {

//    argc = 4;
////    argv = (char **) malloc(4 * sizeof(char *));
////    for (int i = 0; i < 4; i++) {
////        argv[i] = (char *) malloc(10 * sizeof(char));
////    }
//
//    argv[1] = "../image2.pnm";
//    argv[2] = "../output3.pnm";
//    argv[3] = "5";


    if (argc != 4) {
        printf("Incorrect files...\n");
        printf("First argument input pnm image!\n");
        printf("Second output file!\n");
        printf("Numbers a threads\n");
        return 1;
    }

    char *end;
    numThreads = strtol(argv[3], &end, 10);
    if (numThreads == 0) {
        printMessageError("Problems with numbers threads...\n");
        return 1;
    }

    if (numThreads < 0 || numThreads > 100) {
        printMessageError("Too many or too little amount threads...\n");
        return 1;
    }

    INPUT_FD = open(argv[1], O_RDONLY);
    if (INPUT_FD == -1) {
        printMessageError("Problems with input file...\n");
        clear(1);
    }

    OUTPUT_FD = open(argv[2], O_CREAT | O_WRONLY, 0666);
    if (OUTPUT_FD == -1) {
        printMessageError("Problems with output file...\n");
        clear(1);
    }

    if (assertFormatImage(INPUT_FD) == -1) {
        printMessageError("Incorrect format image...\n");
       // clear(1);
    }

    if (skipComments(INPUT_FD) == -1) {
        printMessageError("Problems with skipping comments...\n");
        clear(1);
    }

    if (write(OUTPUT_FD, FORMAT_VERSION, strlen(FORMAT_VERSION)) == -1) {
        printMessageError("Problems with writing format...\n");
        clear(1);
    }

    if (readAndWriteNumber(&WIDTH) == -1) {
        printMessageError("Problems with reading width...\n");
        clear(1);
    }

    if (write(OUTPUT_FD, " ", 1) == -1) {
        printMessageError("Problems with writing sizes...\n");
        clear(1);
    }

    if (readAndWriteNumber(&HEIGHT) == -1) {
        printMessageError("Problems with reading height...\n");
        clear(1);
    }

    if (assertMaxValueColor(INPUT_FD) == -1) {
        printMessageError("Incorrect max value color...\n");
        //clear(1);
    }

    if (write(OUTPUT_FD, "\n", 1) == -1) {
        printMessageError("Problems with writing...\n");
        clear(1);
    }

    if (write(OUTPUT_FD, MAX_VALUE_COLOR, strlen(MAX_VALUE_COLOR)) == -1) {
        printMessageError("Problems with writing...\n");
        clear(1);
    }

    mallocRes();
    if (RES == NULL) {
        printMessageError("Problems with allocation memory...\n");
        clear(1);
    }

    mallocBuf();
    if (BUF == NULL) {
        printMessageError("Problems with allocation memory...\n");
        clear(1);
    }

    mallocArgs();
    if (ARGS == NULL) {
        printMessageError("Problems with allocating memory...\n");
        clear(1);
    }

    mallocThreads();
    if (THREADS == NULL) {
        printMessageError("Problems with allocating memory...\n");
        clear(1);
    }

    imageToGrayScale();

    fillRes();

    printf("num %d\n", numThreads);
    printf("height %d\n", HEIGHT);

    if (numThreads > HEIGHT) {
        numThreads = HEIGHT / 2;
        numThreads++;
        printf("Amount threads decrease to %d\n", numThreads);
    }

    if (fillArgs(ARGS) == -1) {
        printMessageError("Problems with filling arguments...\n");
        clear(1);
    }

    struct timespec start, finish, delta;
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
        printMessageError("Problems with clock!\n");
        clear(1);
    }


    for (int i = 0; i < numThreads; ++i) {
        if (pthread_create(&THREADS[i], NULL, &lineConvolution, ARGS[i]) != 0) {
            printMessageError("Problems with creating THREADS...\n");
            for (int j = 0; j < i; ++j) {
                pthread_cancel(THREADS[j]);
            }
            clear(1);
        }
    }

    double *maxFromThread;
    double max;
    for (int i = 0; i < numThreads; ++i) {
        if (pthread_join(THREADS[i], (void **) &maxFromThread) != 0) {
            printMessageError("Problems with joining THREADS...\n");
            free(maxFromThread);
            for (int j = i; j < numThreads; ++j) {
                pthread_cancel(THREADS[j]);
            }
            clear(1);
        }
        if (maxFromThread == NULL) {
            printMessageError("Problems with allocating...\n");
            free(maxFromThread);
            for (int j = i; j < numThreads; ++j) {
                pthread_cancel(THREADS[j]);
            }
            clear(1);
        }
        if (*maxFromThread > max) {
            max = *maxFromThread;
        }
        free(maxFromThread);
    }

    if (clock_gettime(CLOCK_REALTIME, &finish) == -1) {
        printMessageError("Problems clock!\n");
        clear(1);
    }

    diffTime(start, finish, &delta);
    printf("Passed time: %d.%.9ld sec\n", (int) delta.tv_sec, delta.tv_nsec);

    writeResult(max);

//    clear(0);
    return EXIT_SUCCESS;
}



void writeResult(double max) {
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            for (int k = 0; k < 3; k++) {
                unsigned char normal = (unsigned char) ((RES[i][j] / max) * 255);
//                unsigned char normal = BUF[i][j];
                if (write(OUTPUT_FD, &normal, 1) == 0) {
                    printMessageError("Problems with writing...\n");
                    clear(1);
                }
            }
        }
    }
}

void fillRes() {
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            RES[i][j] = 0;
        }
    }
}

void imageToGrayScale() {
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            BUF[i][j] = lum(readPixel(INPUT_FD));
        }
    }
}

void mallocThreads() { THREADS = malloc(numThreads * sizeof(pthread_t)); }

int readAndWriteNumber(int *number) {
    char *charNumber = readNumber(number);
    if (charNumber == NULL) {
        free(charNumber);
        return -1;
    }
    if (write(OUTPUT_FD, charNumber, strlen(charNumber)) == -1) {
        free(charNumber);
        return -1;
    }
    free(charNumber);
    return 0;
}


void clear(int isError) {
    if (INPUT_FD != -1) {
        close(INPUT_FD);
    }

    if (OUTPUT_FD != -1) {
        close(OUTPUT_FD);
    }

    if (RES != NULL) {
        for (int i = 0; i < HEIGHT; i++) {
            free(RES[i]);
        }
        free(RES);
        RES = NULL;
    }

    if (BUF != NULL) {
        for (int i = 0; i < HEIGHT; i++) {
            free(BUF[i]);
        }
        free(BUF);
        BUF = NULL;
    }

    if (ARGS != NULL) {
        for (int i = 0; i < HEIGHT; i++) {
            free(ARGS[i]);
        }
        free(ARGS);
        ARGS = NULL;
    }

    if (THREADS != NULL) {
        free(THREADS);
        THREADS = NULL;
    }

    if (isError == 1)
        exit(EXIT_FAILURE);
}


void printError() {
    perror(strerror(errno));
}

void printMessageError(const char *messageIfErrorNotExists) {
        printf("%s", messageIfErrorNotExists);
}

char *readNumber(int *number) {
    int size = 3;
    int actualSize = 0;

    char *buf = malloc(sizeof(char) * size);
    if (buf == NULL) {
        return NULL;
    }

    char tempChar = '-';

    while (tempChar < '0' || tempChar > '9') { //skip not digits
        if (read(INPUT_FD, &tempChar, sizeof(char)) == -1) {
            return NULL;
        }
    }

    while (tempChar != ' ' && tempChar != '\n') {
        if (actualSize + 1 > size) { //if need resize
            size++;
            buf = realloc(buf, sizeof(char) * size);
            if (buf == NULL) {
                return NULL;
            }
        }
        buf[actualSize++] = tempChar;
        if (read(INPUT_FD, &tempChar, sizeof(char)) == -1) {
            return NULL;
        }
    }

    char *end;
    *number = strtol(buf, &end, 10);

    if (*number == 0) {
        return NULL;
    }

    return buf;
}

void mallocRes() {
    RES = (resType **) malloc(HEIGHT * sizeof(resType *));
    if (RES == NULL) {
        free(RES);
        RES = NULL;
        return;
    }

    for (int i = 0; i < HEIGHT; i++) {
        RES[i] = (resType *) malloc(WIDTH * sizeof(resType));

        if (RES[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                free(RES[i]);
            }
            free(RES);
            RES = NULL;
            return;
        }
    }

}

void mallocBuf() {
    BUF = (bufType **) malloc(HEIGHT * sizeof(bufType *));
    if (BUF == NULL) {
        free(BUF);
        BUF = NULL;
        return;
    }

    for (int i = 0; i < HEIGHT; i++) {
        BUF[i] = (bufType *) malloc(WIDTH * sizeof(bufType));

        if (BUF[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                free(BUF[i]);
            }
            free(BUF);
            BUF = NULL;
            return;
        }
    }

}

void mallocArgs() {
    ARGS = malloc(numThreads * sizeof(int *));
    if (ARGS == NULL) {
        free(ARGS);
        ARGS = NULL;
        return;
    }

    for (int i = 0; i < numThreads; ++i) {
        ARGS[i] = malloc(2 * sizeof(int));

        if (ARGS[i] == NULL) {
            for (int j = 0; j < i; ++j) {
                free(ARGS[i]);
            }
            free(ARGS);
            ARGS = NULL;
            return;
        }
    }
}

void *lineConvolution(void *arg) {
    int start = *(((int *) arg));
    int end = *(((int *) arg) + 1);

    printf("start end %d %d\n", start, end);
    double max = 0;

    for (int i = start; i < end; ++i) {
        for (int j = 1; j < WIDTH - 1; ++j) { //skip corners
            double sobel = convolution(BUF, i, j);
            if (sobel > max) {
                max = sobel;    //normalize
            }
            RES[i][j] = sobel;
        }
    }

    double *result = malloc(sizeof(double));
    if (result == NULL) {
        return NULL;
    }
    *result = max;

    return (void *) result;
}

int *laneDistribution(int countParts, int length) {
    int part = length / countParts; //максимальная одна из его частей
    int remainder = length - part * countParts; //нераспределенный остаток

    int *parts = malloc(countParts * sizeof(unsigned int));
    if (parts == NULL) {
        return NULL;
    }

    for (int i = 0; i < countParts; ++i) {
        parts[i] = part;
    }

    while (remainder != 0) {
        for (int i = 0; i < countParts; ++i) {
            parts[i]++;
            remainder--;
            if (remainder == 0) break;
        }
    }

    return parts;
}

int fillArgs(int **args) {
    int *parts = laneDistribution(numThreads, HEIGHT - 2);
    if (parts == NULL) {
        return -1;
    }

    int sum = 1; //skip corners
    for (int i = 0; i < numThreads; ++i) {
        args[i][0] = sum; //start
        args[i][1] = parts[i] + sum; //end
        sum += parts[i];
    }

    free(parts);
    return 0;
}

void diffTime(struct timespec t1, struct timespec t2, struct timespec *td) {
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0) {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    } else if (td->tv_sec < 0 && td->tv_nsec > 0) {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}