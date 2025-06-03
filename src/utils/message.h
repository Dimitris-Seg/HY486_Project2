/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file message.h
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

#ifndef MESSAGE_H
#define MESSAGE_H

#include "general.h"
//
#include <mpi/mpi.h>
//

typedef enum message_type   message_type_t;
typedef struct message      message_t;

enum message_type {
    MT_CONNECT,
    MT_NEIGHBOR,
    MT_ACK,
    MT_START_LEADER_ELECTION,
    MT_LE_LIBR_DONE,
    MT_LE_LIBR,
    MT_LEADER,
    MT_ALREADY,
    MT_PARENT,
    MT_START_LE_LOANERS,
    MT_LE_LOANERS,
    MT_LE_LOANERS_DONE,
    MT_ELECT,
    MT_TAKE_BOOK,
    MT_BOOK_REQUEST,
    MT_BOOK_FOUND,
    MT_UPD_DONATED,
    MT_ACK_UD,
    MT_ACK_TB,
    MT_ACK_DB,
    MT_ACK_BK_INFO,
    MT_ACK_NBL,
    MT_LEND_BOOK,
    MT_GET_BOOK,
    MT_FIND_BOOK,
    MT_DONE_FIND_BOOK,
    MT_DONATE_BOOK,
    MT_NOTIFY_DONATE_BOOK,
    MT_STORE_BOOK,
    MT_DONATE_DONE,
    MT_NOTIFY_DONATE_DONE,
    MT_GET_MOST_POPULAR_BOOK,
    MT_GET_POPULAR_BK_INFO,
    MT_GET_MOST_POPULAR_BOOK_DONE,
    MT_CHECK_NUM_BOOKS_LOAN,
    MT_NUM_BOOKS_LOANED,
    MT_CHECK_NUM_BOOKS_LOAN_DONE,
};

struct message {
    int type;
    int args[3];
};

// Initializes an MPI_Datatype for the message struct
void mpi_create_message_struct(MPI_Datatype *newtype);

// Type must be a valid message type
string message_type_to_str(message_type_t type);

#endif