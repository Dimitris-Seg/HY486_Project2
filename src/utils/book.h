/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file book.h
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

#ifndef BOOK_H
#define BOOK_H

typedef int             bookid_t;
typedef int             bookcost_t;
typedef struct book     book_t;

struct book{
    bookid_t    id;   // A unique identifier for each book
    bookcost_t  cost; // The value of the book
};

#endif