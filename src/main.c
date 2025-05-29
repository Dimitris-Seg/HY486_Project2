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
#include "coordinator/coordinator.h"
#include "library/library.h"
#include "client/client.h"
#include "utils/storage/bookstorage.h"
//
#include <stdlib.h>
#include <time.h>
#include <mpi/mpi.h>
#include <math.h>
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

    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size); // The total number of processes running in this mpi session
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // The current process's rank
    SET_PROCESS_ID(rank); // For debugging

    if(rank == COORDINATOR_INDEX){ // Early init. for coordinator debugging
        ENABLE_DEBUGGER();
        SET_PROCESS_NAME("Coordinator"); // For debugging
        PRINT_WATERMARK();
    }

    int NUM_LIBS = -1;
    string instr_file = NULL;

    if(argc > 2){
        instr_file = argv[1];
        NUM_LIBS = atoi(argv[2]);

        if(argc>3){
            WARN("Too many arguments (%d), expected <instruction filename> <NUM_LIBS>. Discarding extras.\n", argc-1);
        }
    }else{
        ERROR("Invalid usage, expected arguments <instruction filename> <NUM_LIBS>, NUM_LIBS>0, valid squared value");
        exit(EXIT_FAILURE);
    }

    if(NUM_LIBS<=0){
        ERROR("Invalid usage, expected positive int NUM_LIBS (number of libraries) as argument 2");
        exit(EXIT_FAILURE);
    }

    int N = sqrt(NUM_LIBS);
    if(N*N != NUM_LIBS){
        ERROR("<NUM_LIBS> is not a proper squared value");
        exit(EXIT_FAILURE);
    }

    if(size != N*N+(N*N*N/2)+1){
        ERROR("The provided N and np values (N=%d, np=%d) don't match", N, size);
        exit(EXIT_FAILURE);
    }

    if(rank == COORDINATOR_INDEX){
        // Process 0 is the coordinator

        PRINT_LOG_HELPER();

        INFO("Process %d says: I'm a coordinator!", rank);

        coordinator_t coord = coordinator_init(rank, N, instr_file);
        coordinator_run(coord);
    }else if(rank<=N*N){
        // Process simulates a library
        // N^2 total libraries

        SET_PROCESS_NAME("Library"); // For debugging

        INFO("Process %d says: I'm a library!", rank);
        library_t lib = library_init(rank, N);
        library_run(lib);
    }else if(rank <= N*N+(N*N*N)/2+1) {
        // Process simulates a borrower/client
        // floor(N^3)/2. total clients

        SET_PROCESS_NAME("Loaner"); // For debugging

        INFO("Process %d says: I'm a client!", rank);

        client_t client = client_init(rank, N);
        client_run(client);
    }

    MPI_Finalize(); // Cleanup mpi environment
}