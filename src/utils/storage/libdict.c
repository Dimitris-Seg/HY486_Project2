/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file libdict.c
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

#include "libdict.h"
#include "../logging.h"
//
#include <assert.h>
//

static size_t hash_bookid(bookid_t id, size_t n){
    return (id%n);
}

libdict_t libdict_create(size_t size){
    if(size==0){
        ERROR("Storage container size must be >0");
        exit(EXIT_FAILURE);
    }

    libdict_t storage = (libdict_t)safe_malloc(sizeof(struct libdict));
    storage->cells = (libdict_node_t*)safe_calloc(size, sizeof(libdict_node_t));
    storage->size = size;
    return storage;
}

void libdict_destroy(libdict_t storage){
    assert(storage);

    for(size_t cell=0;cell<storage->size;++cell){
        libdict_node_t it = storage->cells[cell];

        while(it){
            libdict_node_t next = it->next;
            free(it);
            it = next;
        }
    }

    free(storage->cells);
    free(storage);
}

mpipid_t libdict_find(libdict_t storage, bookid_t id){
    assert(storage);

    libdict_node_t it = storage->cells[hash_bookid(id, storage->size)];

    while(it){
        if(it->book.id == id){
            return it->lib;
        }
        it = it->next;
    }

    return -1;
}

libdict_node_t libdict_find_strict(libdict_t storage, bookid_t id, mpipid_t lib){
    assert(storage);

    libdict_node_t it = storage->cells[hash_bookid(id, storage->size)];

    while(it){
        if(it->book.id == id && it->lib == lib){
            return it;
        }
        it = it->next;
    }

    return NULL;
}

void libdict_store(libdict_t storage, book_t book, mpipid_t lib){
    assert(storage);
    
    libdict_node_t it = libdict_find_strict(storage, book.id, lib); // Check if it's already there

    if(it){
        // If it's already contained simply increase its counter
        ++it->count; 
    }else{
        // If it's a new book we must insert it in the structure anew
        libdict_node_t node = safe_malloc(sizeof(struct libdict));
        node->book = book;
        node->lib  = lib;
        node->count = 1;
        node->next = storage->cells[hash_bookid(book.id, storage->size)];
        storage->cells[hash_bookid(book.id, storage->size)] = node;
    }
}

libdict_lookup_t libdict_take(libdict_t storage, bookid_t id, mpipid_t lib){
    assert(storage);

    libdict_node_t it = libdict_find_strict(storage, id, lib); // Check if it exists

    if(it){
        if(it->count>1){ 
            --it->count;
            return (libdict_lookup_t){.book = it->book, .successful = TRUE};
        }
    }
    return (libdict_lookup_t){.book = (book_t){}, .successful = FALSE};
}

void libdict_map(libdict_t storage, void (*map)(libdict_node_t, void *), void *args){
    assert(storage);

    for(size_t cell=0;cell<storage->size;++cell){
        libdict_node_t it = storage->cells[cell];

        while(it){
            map(it, args);
            it = it->next;
        }
    }
}