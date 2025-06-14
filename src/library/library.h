/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file library.h
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

#ifndef LIBRARY_H
#define LIBRARY_H

#include "../utils/general.h"
#include "../utils/storage/bookstorage.h"
#include "../utils/storage/sllist.h"
#include "../utils/storage/libdict.h"
#include "../utils/logging.h"
//

typedef struct library library_t;

struct library{
    mpiproc_t node;

    mpipid_t  leader;               // The elected library leader

    gridpos_t coords;               // The coordinates of self in the library grid

    mpiproc_t neighbors[4];         // All neighboring mpi processes in the grid (left, top, right, bottom)

    sllist_t children;              // Children in the DFS tree determined by leader election

    bookstorage_t books;            // Holds all original books in the library with their counts

    bookstorage_t donated;          // Holds all donated books in the library with their counts

    size_t books_loaned;            // Tracks the number of books that have ever been loaned (for sanity check)

    mpipid_t *book_locations;       // Holds the library ids for every fixed book id (at index id)

    libdict_t donated_locations;   // Holds the library locations of donated books
    
};

/**
 * Retrieves the neighbor with index 'nid' of the proccess/library positioned
 * at 'coords', with a process id 'sid'
 */
mpiproc_t get_neighbor(gridpos_t coords, mpiproc_t sid, int nid);

/**
 * Initializes the library with id 'id', with a total of N processes in the system
 */
library_t library_init(mpipid_t id, int N);

/**
 * Listens for events from the coordinator and performs necessary actions
 */
void library_run(library_t self);
#endif