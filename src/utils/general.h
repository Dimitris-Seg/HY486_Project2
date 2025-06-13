/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file general.h
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

#ifndef GENERAL_H
#define GENERAL_H

#include <stdlib.h>
//

#define NULL_MPI_PROC (mpiproc_t){.id = -1}   // Macro NULL process initializer (dummy)
#define MPI_PROC_NULL(proc) ((proc).id == -1) // Macro to find when a process is set to NULL (doesn't exist)

#define MAX_BOOK_PRICE 100 // The upper bound for the randomized book prices
#define MIN_BOOK_PRICE 5   // The lower bound -//-

#define BOOK_NOT_FOUND -2   // Error response code for unsuccessful lookup (insufficient books available)
#define COORDINATOR_INDEX 0 // Do not change, only for readability

typedef int                 mpipid_t;
typedef struct mpiproc      mpiproc_t;
typedef struct gridpos      gridpos_t;
typedef char*               string;
typedef enum{FALSE, TRUE}   bool;

struct mpiproc{
    mpipid_t id; // The unique id given to the process through mpi, -1 is nullval
    int N;       // The size of one library grid dimension
};

struct gridpos{ // The i j coordinates of a libary in the grid
    int i,j;
};

/**
 * Custom version of malloc that catches allocation errors
 */
void *safe_malloc(size_t size); 

/**
 * Custom version of calloc that catches allocation errors
 */
void *safe_calloc(size_t nmemb, size_t size);

/**
 * Custom version of realloc that catches allocation errors
 */
void *safe_realloc(void *mem, size_t size);

#endif