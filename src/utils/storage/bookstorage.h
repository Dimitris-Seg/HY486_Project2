/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file bookstorage.h
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

#ifndef STORAGE_H
#define STORAGE_H

#include "../book.h"
#include "../general.h"
//

typedef struct bookstorage*         bookstorage_t;
typedef struct bookstorage_node*    bookstorage_node_t;
typedef struct book_lookup          book_lookup_t;

// A hashmap that stores book entries
struct bookstorage{
    bookstorage_node_t* cells;
    size_t size; // The total available cells
};

struct bookstorage_node{
    book_t book;
    size_t count;
    bookstorage_node_t next;
};

struct book_lookup{
    book_t book;
    bool successful;
};

/**
 * Creates a new book storage hashtable stucture
 */
bookstorage_t bookstorage_create(size_t size);

/**
 * Finds a book with a certain bookid in the storage and returns a pointer to its node, or NULL if not found
 */
bookstorage_node_t bookstorage_find(bookstorage_t storage, bookid_t id);

/**
 * Stores the book in storage
 */
void bookstorage_store(bookstorage_t storage, book_t book);

/**
 * Stores 'count' copies of the book in storage
 */
void bookstorage_mass_store(bookstorage_t storage, book_t book, size_t count);

/**
 * Removes a book with the specified id from the storage and returns it if available
 */
book_lookup_t bookstorage_take(bookstorage_t storage, bookid_t id);

/**
 * Applies the map function on all elements of the structure
 */
void bookstorage_map(bookstorage_t storage, void (*map)(bookstorage_node_t, void *), void *args);
#endif