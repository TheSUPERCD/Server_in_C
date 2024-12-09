#include"include/kmeans.h"

void read_data(point *datapoints, point *centroids, int num_datapoints, int num_centroids, char *filename){
    FILE *fp = fopen(filename, "r");
    if(fp==NULL){
        perror("Cannot open the file");
        exit(EXIT_FAILURE);
    }
    
    for(int i=0; i<num_datapoints; i++){
        fscanf(fp, "%f %f", &datapoints[i].x, &datapoints[i].y);
        datapoints[i].cluster = -1;
    }
    fclose(fp);
    printf("Read the problem data!\n");

    srand(0);
    for(int i=0; i<num_centroids; i++){
        int r = rand() % num_datapoints;
        centroids[i].x = datapoints[r].x;
        centroids[i].y = datapoints[r].y;
    }
    return;
}

// used to get a sense of the distance between points -- sqrt not used because it's not needed to 'just get a sense' of the distance
float dist_metric(point p1, point p2){
    return (p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y);
}

// assign cluster centroids 
void* assign_centroids_to_points(void* arg){
    cluster_t *cluster = (cluster_t *)arg;
    int start = cluster->tid * (cluster->num_datapoints / NUM_THREADS);
    int end = (cluster->tid == NUM_THREADS - 1) ? cluster->num_datapoints : (cluster->tid + 1) * (cluster->num_datapoints / NUM_THREADS);

    for(int i=start; i<end; i++){
        float min_distance = dist_metric(cluster->datapoints[i], cluster->centroids[0]);
        int old_cluster = 0;

        for(int j=1; j<cluster->num_centroids; j++){
            float dist = dist_metric(cluster->datapoints[i], cluster->centroids[j]);
            if(dist<min_distance){
                min_distance = dist;
                old_cluster = j;
            }
        }

        // update the cluster assignment here
        if(cluster->datapoints[i].cluster != old_cluster){
            cluster->datapoints[i].cluster = old_cluster;
            *(cluster->converged) = false;
        }
    }
    pthread_exit(NULL);
}

// calculate and update new centroid
void* update_centroids(void* arg){
    cluster_t* cluster = (cluster_t *)arg;
    int start = cluster->tid * (cluster->num_centroids / NUM_THREADS);
    int end = (cluster->tid == NUM_THREADS - 1) ? cluster->num_centroids : (cluster->tid + 1) * (cluster->num_centroids / NUM_THREADS);

    for(int i=start; i<end; i++){
        float temp_x = 0, temp_y = 0;
        int count = 0;

        for(int j=0; j<cluster->num_datapoints; j++){
            if(cluster->datapoints[j].cluster == i){
                temp_x += cluster->datapoints[j].x;
                temp_y += cluster->datapoints[j].y;
                count++;
            }
        }

        if(count > 0){
            cluster->centroids[i].x = temp_x / count;
            cluster->centroids[i].y = temp_y / count;
        }
    }
    pthread_exit(NULL);
}

// kmeans using pthreads
void kmeans_parallel(point *datapoints, point *centroids, int num_datapoints, int num_centroids){
    pthread_t threads[NUM_THREADS];
    cluster_t divided_clusters[NUM_THREADS];
    int iter = 0;
    bool converged;
    
    for(int i=0; i<NUM_THREADS; i++){
        divided_clusters[i].tid = i;
        divided_clusters[i].num_threads = NUM_THREADS;
        divided_clusters[i].datapoints = datapoints;
        divided_clusters[i].centroids = centroids;
        divided_clusters[i].num_datapoints = num_datapoints;
        divided_clusters[i].num_centroids = num_centroids;
        divided_clusters[i].converged = &converged;
    }

    while(1){
        iter++;
        converged = true;
        for(int i=0; i<NUM_THREADS; i++)
            pthread_create(&threads[i], NULL, assign_centroids_to_points, (void*)&divided_clusters[i]);
        
        for(int i=0; i<NUM_THREADS; i++)
            pthread_join(threads[i], NULL);

        for(int i=0; i<NUM_THREADS; i++)
            pthread_create(&threads[i], NULL, update_centroids, (void*)&divided_clusters[i]);

        for(int i=0; i<NUM_THREADS; i++)
            pthread_join(threads[i], NULL);

        if(converged)
            break;
    }
    printf("Number of iterations taken = %d\n", iter);
    printf("Computed cluster numbers successfully!\n");
    return;
}


void write_results(point *datapoints, point *centroids, int num_datapoints, int num_centroids, char *filename){
    FILE* fp = fopen(filename, "w");
    if(fp==NULL){
        perror("Cannot open the file");
        exit(EXIT_FAILURE);
    }
    
    for(int i=0; i<num_datapoints; i++){
        fprintf(fp, "%.2f %.2f %d\n", datapoints[i].x, datapoints[i].y, datapoints[i].cluster);
    }
    fclose(fp);
    printf("Wrote the results to a file!\n");
}


void run_kmeans(int num_datapoints, int num_centroids, char *in_filename, char *out_filename){
    if(num_datapoints == -1){
        num_datapoints = 1797;
    }
    point *datapoints = (point *)malloc(num_datapoints*sizeof(point));
    point *centroids = (point *)malloc(num_centroids*sizeof(point));
    
    read_data(datapoints, centroids, num_datapoints, num_centroids, in_filename);
    kmeans_parallel(datapoints, centroids, num_datapoints, num_centroids);
    write_results(datapoints, centroids, num_datapoints, num_centroids, out_filename);

    free(datapoints);
    free(centroids);

    return;
}







