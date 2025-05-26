/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file main.c
 *
 * Computer Science Department, University Of Crete
 *
 *           (   (      
 *       (   )\ ))\ )   
 *       )\ (()/(()/(   
 *     (((_) /(_))(_))  
 *     )\___(_))(_))_   
 *    ((/ __/ __||   \  
 *     | (__\__ \| |) | 
 *      \___|___/|___/  
 */

#include "utils/logging.h"
#include "library/library.h"
#include "client/client.h"
#include "utils/storage/bookstorage.h"
//
#include <stdlib.h>
#include <time.h>
#include <mpi/mpi.h>
//

void print_books(bookstorage_node_t node, void *args){
    (void)args;

    INFO("Found book %d for %d$ (x%lu)", node->book.id, node->book.cost, node->count);
}

// Test main
int main(int argc, char *argv[]){
    srand(time(NULL)); // Seed randomizer
    
    MPI_Init(&argc, &argv); // Setup mpi 

    //-- MPI is ready now --//

    int N = -1;

    if(argc > 1){
        N = atoi(argv[1]);
    }
    
    if(N<=0){
        ERROR("Invalid usage, expected positive int N (size of library grid dimension) as argument");
        exit(EXIT_FAILURE);
    }

    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size); // The total number of processes running in this mpi session
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // The current process's rank

    if(size != N*N+(N*N*N/2)+1){
        ERROR("The provided N and np values (N=%d, np=%d) don't match", N, size);
        exit(EXIT_FAILURE);
    }

    if(rank == 0){
        // Process 0 is the coordinator

        enable_debugger(); // Enables the debugger for the coordinator
        PRINT_WATERMARK();
        INFO("Process %d says: I'm a coordinator!", rank);
    }else if(rank<=N*N){
        // Process simulates a library
        // N^2 total libraries

        library_t lib = library_init(rank, N);
        INFO("Process %d says: I'm a library!", rank);
    }else if(rank <= N*N+(N*N*N)/2) {
        // Process simulates a borrower/client
        // floor(N^3)/2. total clients

        client_t client = client_init(rank, N);
        INFO("Process %d says: I'm a client!", rank);
    }

    MPI_Finalize(); // Cleanup mpi environment
}