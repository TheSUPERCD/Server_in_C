#pragma once

#ifndef MATINV_H
#define MATINV_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>

#define MAX_ELEMENTS 4096
#define MAX_ROWS 64
#define NUM_THREADS 2

// structure to share data with threads created using pthreads
typedef struct inversion_data {
    double *matrix;
    double *inverse_matrix;
    int matsize;
    int pivot;
    double pivot_val;
    double multiplier;
    int i;
    int tid;
} invdat_t;

// type for better differentiating between "random" and "fast" initializations
typedef enum {INIT_RAND, INIT_FAST} init_type_t;

// initializes the matrix with given parameters
void initialize_matrix(double *matrix, int matsize, init_type_t type, double diagonal_offset, double offset, int maxrand);

// computes the inverse-matrix using Gauss-Jordan transformation method
void compute_inverse(double *matrix, int matsize, double *inverse_matrix);

// computes the inverse-matrix using Gauss-Jordan transformation method -- in parallel
void compute_inverse_parallel(double *matrix, int matsize, double *inverse_matrix);

// print the matrix -- in a file
void printMat(double *matrix, int matsize, FILE *fp);

// compare the matrix with another matrix, return error immediately upon noticing a difference -- used for debugging
void compare_mat(double *mat_A, double *mat_B, int matsize);

// compute the matrix-inverse and save it into a file
void compute_matinv(int N, int maxnum, char *init_type, char *filepath);




#endif