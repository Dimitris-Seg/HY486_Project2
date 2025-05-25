/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file sllist.h
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

#ifndef SLLIST_H
#define SLLIST_H

#include "../general.h"
//

/**
 * This module aims to store neighboring processes (for client/borrower processes)
 */

typedef int                 sllist_id_t;   // The chosen id type for elements in the list
typedef mpipid_t            sllist_elem_t; // The element type stored in sllist
typedef struct sllist*      sllist_t;
typedef struct sllist_node* sllist_node_t;

// Creates (allocates) a new sllist
sllist_t sllist_create();

// Destroys (deallocates) an sllist
void sllist_destroy(sllist_t list);

// Peforms a lookup of the list and returns the element with id 'id', or NULL if not found
sllist_node_t sllist_search(sllist_t list, sllist_id_t id);

// Adds the new element 'elem' to the (start of the) list
void sllist_add(sllist_t list, sllist_elem_t elem, sllist_id_t i);

struct sllist_node{
    sllist_id_t   id;
    sllist_elem_t elem;
    sllist_node_t next;
};

/**
 * Simple singly-linked list
 */
struct sllist{
    sllist_node_t head;
};


#endif