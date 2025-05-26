/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file tokenlist.h
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

#ifndef TOKENLIST_H
#define TOKENLIST_H

#include "../general.h"
#include "token.h"
//
#include <assert.h>
//

#define TOKENLIST_INIT_SIZE   32  
#define TOKENLIST_GROWTH_RATE 32

typedef struct tokenlist*   tokenlist_t;

struct tokenlist {
    token_t* tokens;
    size_t size;
    size_t len;
};

// Creates a new tokenlist object and allocates a default buffer
tokenlist_t tokenlist_create();

// Deallocates the tokenlist object and the internal buffer
void tokenlist_destroy(tokenlist_t tokenlist);

// Appends the token to the end of the buffer, ensuring the buffer is large enough
void tokenlist_append(tokenlist_t tokenlist, token_t token);

#endif