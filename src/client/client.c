/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file client.c
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

#include "client.h"

/**
 * Initializes the client/borrower with id 'id'
 */
client_t client_init(mpipid_t id){
    client_t client = {
        .self     = {.id = id},
        .neigbors = sllist_create(),
        .books    = bookstorage_create(N*N), // Use N*N buckets
    };

    return client;
}