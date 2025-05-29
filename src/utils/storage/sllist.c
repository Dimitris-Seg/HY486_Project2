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

sllist_node_t sllist_search(sllist_t list, sllist_elem_t elem){
    assert(list);
    
    sllist_node_t it = list->head;

    while(it){
        if(it->elem == elem){
            // Found
            return it;
        }
        it = it->next;
    }

    return NULL;
}

void sllist_add_unique(sllist_t list, sllist_elem_t elem){
    if(!sllist_search(list, elem)){
        sllist_add(list, elem);
    }
}

void sllist_add(sllist_t list, sllist_elem_t elem){
    assert(list);

    sllist_node_t node = safe_malloc(sizeof(struct sllist_node));
    node->elem = elem;
    node->next = list->head;
    list->head = node;
}

void sllist_remove(sllist_t list, sllist_elem_t elem){
    assert(list);
    
    sllist_node_t it = list->head;
    sllist_node_t prev = NULL;

    while(it){
        if(it->elem == elem){
            // Found
            if(prev){
                prev->next = it->next;
                free(it);
            }else{
                list->head = it->next;
            }
        }
        prev = it;
        it = it->next;
    }

    // Element not found, ignore
}

sllist_elem_t sllist_removeOne(sllist_t list){
    sllist_elem_t ret = -1;
    if(list->head){
        ret = list->head->elem;
        list->head = list->head->next;
    }
    return ret;
}

void sllist_removeAll(sllist_t list){
    assert(list);
    
    sllist_node_t it = list->head;
    sllist_node_t prev = NULL;

    list->head = NULL;

    while(it){
        prev = it;
        it = it->next;
        free(prev);
    }
}

void sllist_map(sllist_t list, void(*cb)(sllist_node_t, void *), void *args){
    assert(list);

    sllist_node_t it = list->head;
    
    while(it){
        cb(it, args); // Map
        it = it->next;
    }
}

bool sllist_empty(sllist_t list){
    return (list->head == NULL);
}