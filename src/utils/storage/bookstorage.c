/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file bookstorage.c
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

#include "bookstorage.h"
#include "../logging.h"
//
#include <assert.h>
//

static size_t hash_bookid(bookid_t id, size_t n){
    return (id%n);
}

bookstorage_t bookstorage_create(size_t size){
    if(size==0){
        ERROR("Storage container size must be >0");
        exit(EXIT_FAILURE);
    }

    bookstorage_t storage = (bookstorage_t)safe_malloc(sizeof(struct bookstorage));
    storage->cells = (bookstorage_node_t*)safe_calloc(size, sizeof(bookstorage_node_t));
    storage->size = size;
    return storage;
}

void bookstorage_destroy(bookstorage_t storage){
    assert(storage);

    for(size_t cell=0;cell<storage->size;++cell){
        bookstorage_node_t it = storage->cells[cell];

        while(it){
            bookstorage_node_t next = it->next;
            free(it);
            it = next;
        }
    }

    free(storage->cells);
    free(storage);
}

bookstorage_node_t bookstorage_find(bookstorage_t storage, bookid_t id){
    assert(storage);

    bookstorage_node_t it = storage->cells[hash_bookid(id, storage->size)];

    while(it){
        if(it->book.id == id){
            return it;
        }
        it = it->next;
    }

    return NULL;
}

void bookstorage_store(bookstorage_t storage, book_t book){
    bookstorage_mass_store(storage, book, 1);
}

void bookstorage_mass_store(bookstorage_t storage, book_t book, size_t count){
    assert(storage);
    
    bookstorage_node_t it = bookstorage_find(storage, book.id); // Check if it's already there

    if(it){
        // If it's already contained simply increase its counter
        it->count+=count; 

        // Update price if old one didn't have any
        if(it->book.cost <= 0){
            it->book.cost = book.cost;
        }
    }else{
        // If it's a new book we must insert it in the structure anew
        bookstorage_node_t node = safe_malloc(sizeof(struct bookstorage_node));
        node->book = book;
        node->count = count;
        node->next = storage->cells[hash_bookid(book.id, storage->size)];
        storage->cells[hash_bookid(book.id, storage->size)] = node;
    }
}

book_lookup_t bookstorage_take(bookstorage_t storage, bookid_t id){
    assert(storage);

    bookstorage_node_t it = bookstorage_find(storage, id); // Check if it exists

    if(it){
        if(it->count>0){
            --it->count;
            return (book_lookup_t){.book = it->book, .successful = TRUE};
        }

        // // Otherwise, we should remove the node
        // book_lookup_t res = {.book = it->book, .successful = TRUE};
        // storage->cells[hash_bookid(id, storage->size)] = it->next;
        // free(it);
        // return res;
    }
    return (book_lookup_t){.book = (book_t){}, .successful = FALSE};
}

void bookstorage_map(bookstorage_t storage, void (*map)(bookstorage_node_t, void *), void *args){
    assert(storage);

    for(size_t cell=0;cell<storage->size;++cell){
        bookstorage_node_t it = storage->cells[cell];

        while(it){
            map(it, args);
            it = it->next;
        }
    }
}