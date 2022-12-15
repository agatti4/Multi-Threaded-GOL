# Multi-Threaded GOL

Here is a project I made for Systems and Multiprogramming. 
This version of Conway's Game of Life is multi-threaded and partitions the threads into rows or columns of the board.

To compile: Use terminal command gcc -pthread -g -o main main.c pthread_barrier.c
To run: Use terminal command ./main fileName numberOfThreads row/col wrap/nowrap hide/show slow/med/fast(if show)
