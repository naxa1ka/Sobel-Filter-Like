all:
	gcc  main.c pnm.c sobel.c -o sobel -pthread  -lm
