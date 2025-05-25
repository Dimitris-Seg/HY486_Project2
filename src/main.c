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
//

void print_books(bookstorage_node_t node, void *args){
    (void)args;

    INFO("Found book %d for %d$ (x%lu)", node->book.id, node->book.cost, node->count);
}

// Test main
int main(){
    srand(time(NULL)); // Seed randomizer

    PRINT_WATERMARK();

    library_t lib = library_init(0);

    INFO(
        "Library %d is at (%d, %d) with neighbors [%d, %d, %d, %d]", 
        lib.self.id, lib.coords.i, lib.coords.j, lib.neighbors[0].id, lib.neighbors[1].id, lib.neighbors[2].id, lib.neighbors[3].id
    );

    bookstorage_map(lib.books, print_books, NULL);

    client_t client = client_init(1);
}