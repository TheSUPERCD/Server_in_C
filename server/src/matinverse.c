#include"include/matinverse.h"

pthread_t threads[NUM_THREADS];

void initialize_matrix(double *matrix, int matsize, init_type_t type, double diagonal_offset, double offset, int maxrand){
    if(type==INIT_FAST){
        for(int i=0; i<matsize; i++){
            for(int j=0; j<matsize; j++){
                if(i==j)
                    matrix[i*matsize + j] = diagonal_offset;
                else
                    matrix[i*matsize + j] = offset;
            }
        }
        return;
    }
    for(int i=0; i<matsize; i++){
        for(int j=0; j<matsize; j++){
            if(i==j)
                matrix[i*matsize + j] = (double)(rand() % maxrand + 5);
            else
                matrix[i*matsize + j] = (double)(rand() % maxrand + 1);
        }
    }
    return;
}

void compute_inverse(double *matrix, int matsize, double *inverse_matrix){
    for(int pivot=0; pivot<matsize; pivot++){
        double pivot_val = matrix[pivot*matsize + pivot];
        for(int j=0; j<matsize; j++){
            matrix[pivot*matsize + j] = matrix[pivot*matsize + j] / pivot_val;
            inverse_matrix[pivot*matsize + j] = inverse_matrix[pivot*matsize + j] / pivot_val;
        }
        assert(matrix[pivot*matsize + pivot] == 1.0);
        double multiplier;
        for(int i=0; i<matsize; i++){
            multiplier = matrix[i*matsize + pivot];
            if(i == pivot){
                continue;
            }
            for(int j=0; j<matsize; j++){
                matrix[i*matsize + j] = matrix[i*matsize + j] - matrix[pivot*matsize + j]*multiplier;
                inverse_matrix[i*matsize + j] = inverse_matrix[i*matsize + j] - inverse_matrix[pivot*matsize + j]*multiplier;
            }
            assert(matrix[i*matsize + pivot] == 0.0);
        }
    }
}


void *pivotrow_normalizer(void *args){
    invdat_t *invdata = (invdat_t *)args;
    int start = invdata->tid * (invdata->matsize / NUM_THREADS);
    int end = (invdata->tid == NUM_THREADS-1) ? invdata->matsize : (invdata->tid + 1) * (invdata->matsize / NUM_THREADS);
    for(int j=start; j<end; j++){
        invdata->matrix[invdata->pivot*invdata->matsize + j] /= invdata->pivot_val;
        invdata->inverse_matrix[invdata->pivot*invdata->matsize + j] /= invdata->pivot_val;
    }
    return NULL;
}

void *apply_matrix_transform(void *args){
    invdat_t *invdata = (invdat_t *)args;
    int start = invdata->tid * (invdata->matsize / NUM_THREADS);
    int end = (invdata->tid == NUM_THREADS-1) ? invdata->matsize : (invdata->tid + 1) * (invdata->matsize / NUM_THREADS);
    for(int j=start; j<end; j++){
        invdata->matrix[invdata->i*invdata->matsize + j] = invdata->matrix[invdata->i*invdata->matsize + j] - invdata->matrix[invdata->pivot*invdata->matsize + j]*invdata->multiplier;
        invdata->inverse_matrix[invdata->i*invdata->matsize + j] = invdata->inverse_matrix[invdata->i*invdata->matsize + j] - invdata->inverse_matrix[invdata->pivot*invdata->matsize + j]*invdata->multiplier;
    }
    return NULL;
}

void compute_inverse_parallel(double *matrix, int matsize, double *inverse_matrix){
    // initialize the data-sharing constructs
    invdat_t invdata[NUM_THREADS];
    for(int i=0; i<NUM_THREADS; i++){
        invdata[i].tid = i;
        invdata[i].matrix = matrix;
        invdata[i].inverse_matrix = inverse_matrix;
        invdata[i].matsize = matsize;
    }

    // for each pivot
    for(int pivot=0; pivot<matsize; pivot++){
        // 1st phase
        for(int t=0; t<NUM_THREADS; t++){
            invdata[t].pivot = pivot;
            invdata[t].pivot_val = matrix[pivot*matsize + pivot];
            pthread_create(&threads[t], NULL, pivotrow_normalizer, (void *)&invdata[t]);
        }
        for(int t=0; t<NUM_THREADS; t++)
            pthread_join(threads[t], NULL);
        
        assert(matrix[pivot*matsize + pivot] == 1.0);

        // 2nd phase
        for(int i=0; i<matsize; i++){
            // skip doing anything for the pivot row
            if(i == pivot)
                continue;
            
            // each row is sequentially processed -- columns are done in parallel batches (for each row)
            for(int t=0; t<NUM_THREADS; t++){
                invdata[t].i = i;
                invdata[t].multiplier = matrix[i*matsize + pivot];
                pthread_create(&threads[t], NULL, apply_matrix_transform, (void *)&invdata[t]);
            }
            for(int t=0; t<NUM_THREADS; t++)
                pthread_join(threads[t], NULL);
            
            assert(matrix[i*matsize + pivot] == 0.0);
        }
    }
    return;
}


void printMat(double *matrix, int matsize, FILE *fp){
    for(int i=0; i<matsize; i++){
        for(int j=0; j<matsize; j++){
            fprintf(fp, "%5.2f ", matrix[i*matsize + j]);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

void compare_mat(double *mat_A, double *mat_B, int matsize){
    for(int i=0; i<matsize*matsize; i++){
        if(mat_A[i] == mat_B[i])
            continue;
        else{
            printf("Value not matched at %d || A=%f, B=%f\n", i, mat_A[i], mat_B[i]);
            return;
        }
    }
}


void compute_matinv(int N, int maxnum, char *init_type, char *filepath){
    double *matrix = (double *)malloc(N*N*sizeof(double));
    double *inverse_matrix = (double *)malloc(N*N*sizeof(double));
    initialize_matrix(inverse_matrix, N, INIT_FAST, 1.0, 0.0, 0);
    
    if(strcmp(init_type, "fast")==0){
        initialize_matrix(matrix, N, INIT_FAST, 5.0, 2.0, 0);
    } else{
        initialize_matrix(matrix, N, INIT_RAND, 5.0, 2.0, maxnum);
    }

    compute_inverse_parallel(matrix, N, inverse_matrix);
    
    FILE *fp = fopen(filepath, "w+");
    printMat(inverse_matrix, N, fp);
    fclose(fp);

    free(matrix);
    free(inverse_matrix);
}




