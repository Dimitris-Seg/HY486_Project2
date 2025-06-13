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
#include "../utils/message.h"
//
#include <mpi/mpi.h>
//

typedef struct client           client_t;
typedef struct mapped_message   mapped_message_t;

struct client{
    mpiproc_t node;

    mpipid_t leader;        // The elected leader process (among the clients/borrowers)

    mpipid_t donate_next;   // Next library to receive a donated book (works round-robin)

    bookstorage_t books;    // Holds all books the client has ever had under posession/borrowed (never removed)

    sllist_t neigbors;      // All books that were in 'self' at some point
};

struct mapped_message{
    MPI_Datatype*   datatype;
    message_t*      message;
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