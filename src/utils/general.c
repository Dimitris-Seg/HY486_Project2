/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file general.c
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

#include "general.h"
#include "logging.h"
//

void *safe_malloc(size_t size){
    void *alloc = malloc(size);

    if(!alloc){
        ERROR("Failed to allocate %lu bytes", size);
        exit(EXIT_FAILURE);
    }

    return alloc;
}

void *safe_calloc(size_t nmemb, size_t size){
    void *alloc = calloc(nmemb, size);

    if(!alloc){
        ERROR("Failed to allocate %lu bytes", size);
        exit(EXIT_FAILURE);
    }

    return alloc;
}

void *safe_realloc(void *mem, size_t size){
    void *alloc = realloc(mem, size);

    if(!alloc){
        ERROR("Failed to allocate %lu bytes", size);
        exit(EXIT_FAILURE);
    }

    return alloc;
}