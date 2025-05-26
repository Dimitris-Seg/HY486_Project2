/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file library.c
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

#include "library.h"
//
#include <stdlib.h>
//

mpiproc_t get_neighbor(gridpos_t coords, mpiproc_t proc, int nid){
    int N = proc.N;

    switch(nid){
    case 0: // LEFT neighbor
        return (mpiproc_t){.id=(coords.i%N)>0?
            proc.id-1:-1 
        };
    case 1: // TOP neighbor
        return (mpiproc_t){.id=((coords.j+1)/N)<N-1?
            proc.id+N:-1 
        };
    case 2: // RIGHT neighbor
        return (mpiproc_t){.id=(coords.i%N)<N-1?
            proc.id+1:-1 
        };
    case 3: // BOTTOM neighbor
        return (mpiproc_t){.id=(coords.j)>0?
            proc.id-N:-1 
        };
    default:
        // Should never happen unless misused
        ERROR("Invalid neighbor id '%d'", nid);
        exit(EXIT_FAILURE);
    }
}

// Random value in range [MIN_BOOK_PRICE, MAX_BOOK_PRICE]
static bookcost_t random_book_cost(){
    return MIN_BOOK_PRICE+(rand()%(MAX_BOOK_PRICE-MIN_BOOK_PRICE+1));
}

library_t library_init(mpipid_t id, int N){
    library_t lib = {
        .node           = {.id = id, .N = N},
        .coords         = (gridpos_t){.i = id%N, .j = id/N},
        .books          = bookstorage_create(N*N), // Use N*N buckets
    };

    for(int nid = 0;nid < 4; ++nid){
        lib.neighbors[nid] = get_neighbor(lib.coords, lib.node, nid);
    }

    bookid_t b_id = lib.node.id;

    for(int i=0;i<N;++i){
        // Mass store N books with id b_id
        bookstorage_mass_store(lib.books, (book_t){.id = b_id, .cost = random_book_cost()}, N);
        ++b_id;
    }

    return lib;
}

