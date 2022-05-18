cmake_minimum_required(VERSION 3.19)
project(sobel C)

set(CMAKE_C_STANDARD 99)
add_executable(sobel main.c pnm.h pnm.c sobel.h sobel.c)
