/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file client.h
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

#ifndef CLIENT_H
#define CLIENT_H

#include "../utils/general.h"
#include "../utils/storage/bookstorage.h"
#include "../utils/storage/sllist.h"

typedef struct client client_t;

struct client{
    mpiproc_t node;

    bookstorage_t books;    // Holds all books the client has under posession/borrowed

    sllist_t neigbors;  // All books that were in 'self' at some point
};

/**
 * Initializes the client with id 'id', with a total of N processes in the system
 */
client_t client_init(mpipid_t id, int N);

/**
 * Listens for events from the coordinator and performs necessary actions
 */
void client_run(client_t self);

#endif