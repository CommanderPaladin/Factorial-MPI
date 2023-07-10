#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <mpi.h>
#include <sys/time.h>

#define DEBUG 0

void factorial(mpz_t result, unsigned int start, unsigned int end) {
    mpz_set_ui(result, 1);
    for (unsigned int i = start; i <= end; ++i) {
        mpz_mul_ui(result, result, i);
    }
}

int main(int argc, char** argv) {
    unsigned int number;
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        printf("Enter a positive integer: ");
        scanf("%u", &number);
    }

    MPI_Barrier(MPI_COMM_WORLD);  // Wait for rank 0 to read the input

    struct timeval start_time, end_time;

    if (rank == 0) {
        gettimeofday(&start_time, NULL);
    }

    MPI_Bcast(&number, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);  // Broadcast the number to all processes

    unsigned int chunk_size = number / size;
    unsigned int start = rank * chunk_size + 1;
    unsigned int end = (rank == size - 1) ? number : (rank + 1) * chunk_size;

    mpz_t partial_result;
    mpz_init(partial_result);

    factorial(partial_result, start, end);

    if (rank != 0) {
        size_t count;
        unsigned char* buffer;
        mpz_export(NULL, &count, 1, sizeof(unsigned char), 0, 0, partial_result);
        buffer = (unsigned char*)malloc(count * sizeof(unsigned char));
        mpz_export(buffer, &count, 1, sizeof(unsigned char), 0, 0, partial_result);
        MPI_Send(buffer, count, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
        free(buffer);
    } else {
        mpz_t final_result;
        mpz_init(final_result);
        mpz_set(final_result, partial_result);

        for (int i = 1; i < size; i++) {
            MPI_Status status;
            size_t count;
            unsigned char* buffer;
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_UNSIGNED_CHAR, &count);
            buffer = (unsigned char*)malloc(count * sizeof(unsigned char));
            MPI_Recv(buffer, count, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            mpz_import(partial_result, count, 1, sizeof(unsigned char), 0, 0, buffer);
            mpz_mul(final_result, final_result, partial_result);
            free(buffer);
        }

        char* result_str = mpz_get_str(NULL, 10, final_result);
        printf("Factorial of %u = %s\n", number, result_str);

        // Save result to file
        FILE* file = fopen("factorial_result.txt", "w");
        if (file != NULL) {
            fprintf(file, "Factorial of %u = %s\n", number, result_str);
            fclose(file);
        } else {
            printf("Failed to save result to file.\n");
        }

        mpz_clear(final_result);
        free(result_str);
    }

    mpz_clear(partial_result);

    MPI_Barrier(MPI_COMM_WORLD);  // Wait for all processes to finish

    if (rank == 0) {
        gettimeofday(&end_time, NULL);
        double execution_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1e6;

        if (DEBUG) {
            // Display results from each worker
            for (int i = 0; i < size; i++) {
                mpz_t worker_result;
                mpz_init(worker_result);
                factorial(worker_result, i * chunk_size + 1, (i == size - 1) ? number : (i + 1) * chunk_size);
                char* result_str = mpz_get_str(NULL, 10, worker_result);
                printf("Worker %d: Factorial of %u = %s\n", i, number, result_str);
                mpz_clear(worker_result);
                free(result_str);
            }
        }

        printf("Execution time: %.6f seconds\n", execution_time);
    }

    MPI_Finalize();

    return 0;
}

