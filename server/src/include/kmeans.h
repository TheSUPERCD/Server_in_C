#pragma once

#ifndef KMEANS_H
#define KMEANS_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>
#include <pthread.h>

#define MAX_POINTS 4096
#define MAX_CLUSTERS 32
#define NUM_THREADS 2

// point construct
typedef struct point{
    float x; // The x-coordinate of the point
    float y; // The y-coordinate of the point
    int cluster; // The cluster that the point belongs to
} point;

// construct for holding the required parameters we need to pass into a function running using pthreads-based multithreading
typedef struct cluster_data {
    point *datapoints;
    point *centroids;
    int num_threads;
    int num_datapoints;
    int num_centroids;
    int tid;    // simple way to define and use own block of data -- use own thread-id to figure it out
    bool *converged;    // has the k-means algorithm converged?
} cluster_t;


// read the cluster-data from a given file. 
void read_data(point *datapoints, point *centroids, int num_datapoints, int num_centroids, char *filename);

// runs the k-means algorithm
void kmeans_parallel(point *datapoints, point *centroids, int num_datapoints, int num_centroids);

// writes the results to a given file
void write_results(point *datapoints, point *centroids, int num_datapoints, int num_centroids, char *filename);

// run the kmeans clustering algorithm and save the output in the given file-location. If num_datapoints = -1, then num_datapoints defaults to 1797
void run_kmeans(int num_datapoints, int num_centroids, char *in_filename, char *out_filename);

#endif