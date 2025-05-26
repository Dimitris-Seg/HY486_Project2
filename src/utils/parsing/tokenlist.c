/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file tokenlist.c
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

#include "tokenlist.h"

tokenlist_t tokenlist_create(){
    tokenlist_t tokenlist = (tokenlist_t)safe_malloc(sizeof(struct tokenlist));
    tokenlist->size = TOKENLIST_INIT_SIZE;
    tokenlist->len = 0;
    tokenlist->tokens = (token_t*)safe_malloc(TOKENLIST_INIT_SIZE*sizeof(token_t));

    return tokenlist;
}

void tokenlist_destroy(tokenlist_t tokenlist){
    assert(tokenlist && tokenlist->tokens);

    free(tokenlist->tokens);
    free(tokenlist);
}

void tokenlist_append(tokenlist_t tokenlist, token_t token){
    if(tokenlist->len + 1 >= tokenlist->size){
        tokenlist->size += TOKENLIST_GROWTH_RATE;
        tokenlist->tokens = (token_t*)safe_realloc(tokenlist->tokens, tokenlist->size*sizeof(token_t));
    }

    tokenlist->tokens[tokenlist->len++] = token;
}