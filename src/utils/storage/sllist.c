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

#include "sllist.h"
#include "../general.h"
//
#include <assert.h>
//

sllist_t sllist_create(){
    sllist_t list = safe_malloc(sizeof(struct sllist));
    list->head = NULL;

    return list;
}

void sllist_destroy(sllist_t list){
    assert(list);

    sllist_node_t it = list->head;

    // Free all nodes first
    while(it!=NULL){
        sllist_node_t next = it->next;
        free(it);
        it = next;
    }

    // Free actual list
    free(list);
}

sllist_node_t sllist_search(sllist_t list, sllist_id_t id){
    assert(list);
    
    sllist_node_t it = list->head;

    while(it!=NULL && it->id != id){
        if(it->id == id){
            return it;
        }
        it = it->next;
    }

    return NULL;
}

void sllist_add(sllist_t list, sllist_elem_t elem, sllist_id_t id){
    assert(list);

    sllist_node_t node = safe_malloc(sizeof(struct sllist_node));
    node->elem = elem;
    node->id   = id;
    node->next = list->head;
    list->head = node;
}