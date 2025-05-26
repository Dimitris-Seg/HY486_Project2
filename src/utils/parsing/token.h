/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file token.h
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

#ifndef TOKEN_H
#define TOKEN_H

typedef enum token_type     token_type_t;
typedef struct token        token_t;

enum token_type {
    TT_CONNECT, 
    TT_START_LE_LIBR, 
    TT_START_LE_LOANERS, 
    TT_TAKE_BOOK, 
    TT_DONATE_BOOK, 
    TT_GET_MOST_POPULAR_BOOK, 
    TT_CHECK_NUM_BOOKS_LOANED,
    TT_INT,
};

/**
 * val only makes sense for the TT_INT type
 */
struct token {
    token_type_t type;
    int val;
};

#endif