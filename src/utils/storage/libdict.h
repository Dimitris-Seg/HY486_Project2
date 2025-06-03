/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file libdict.h
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

#ifndef LIBDICT_H
#define LIBDICT_H

#include "../book.h"
#include "../general.h"
//

typedef struct libdict*         libdict_t;
typedef struct libdict_node*    libdict_node_t;
typedef struct libdict_lookup   libdict_lookup_t;

// A hashmap that stores book entries
struct libdict{
    libdict_node_t* cells;
    size_t size; // The total available cells
};

struct libdict_node{
    book_t          book;
    mpipid_t        lib;
    size_t          count;
    libdict_node_t  next;
};

struct libdict_lookup{
    book_t book;
    bool successful;
};

/**
 * Creates a new library dictionary hashtable stucture
 */
libdict_t libdict_create(size_t size);

/**
 * Finds a book with a certain bookid in the storage and returns a pointer to its node, or NULL if not found
 */
mpipid_t libdict_find(libdict_t storage, bookid_t id);

/**
 * Stores the book-library correspondance in storage
 */
void libdict_store(libdict_t storage, book_t book, mpipid_t lib);

/**
 * Removes a book with the specified id from the storage of a library and returns it if available
 */
libdict_lookup_t libdict_take(libdict_t storage, bookid_t id, mpipid_t lib);

/**
 * Applies the map function on all elements of the structure
 */
void libdict_map(libdict_t storage, void (*map)(libdict_node_t, void *), void *args);
#endif