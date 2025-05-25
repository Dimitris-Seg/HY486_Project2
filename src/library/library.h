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
#include "../utils/logging.h"
//

typedef struct library library_t;

struct library{
    mpiproc_t self;

    gridpos_t coords;

    mpiproc_t neighbors[4]; // All neighboring mpi processes in the grid (left, top, right, bottom)

    bookstorage_t books;    // Holds all books in the library with their counts
};

/**
 * Retrieves the neighbor with index 'nid' of the proccess/library positioned
 * at 'coords', with a process id 'sid'
 */
mpiproc_t get_neighbor(gridpos_t coords, mpiproc_t sid, int nid);

/**
 * Initializes the library with id 'id'
 */
library_t library_init(mpipid_t id);
#endif