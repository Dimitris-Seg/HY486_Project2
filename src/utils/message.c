/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file message.c
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

#include "message.h"
//

void mpi_create_message_struct(MPI_Datatype *newtype){
    message_t message;

    int block_lengths[] = {1, 3};
    MPI_Aint displacements[2];
    MPI_Datatype types[2] = {MPI_INT, MPI_INT};

    MPI_Aint base;
    MPI_Get_address(&message, &base);
    MPI_Get_address(&message.type, &displacements[0]);
    MPI_Get_address(&message.args, &displacements[1]);

    // Make field addresses relative
    displacements[0] -= base;
    displacements[1] -= base;

    MPI_Type_create_struct(2, block_lengths, displacements, types, newtype);
    MPI_Type_commit(newtype);
}

// Type must be a valid message type
string message_type_to_str(message_type_t type){
    static const string names[] = {
        "CONNECT",
        "NEIGHBOR",
        "ACK",
        "START_LEADER_ELECTION",
        "LE_LIBR_DONE",
        "MT_LE_LIBR",
        "LEADER",
        "ALREADY",
        "PARENT",
        "START_LE_LOANERS",
        "LE_LOANERS",
        "LE_LOANERS_DONE",
        "ELECT",
        "TAKE_BOOK",
        "BOOK_REQUEST",
        "BOOK_FOUND",
        "MT_UPD_DONATED",
        "MT_ACK_UD",
        "ACK_TB",
        "ACK_DB",
        "ACK_BK_INFO",
        "ACK_NBL",
        "LEND_BOOK",
        "GET_BOOK",
        "FIND_BOOK",
        "DONE_FIND_BOOK",
        "DONATE_BOOK",
        "NOTIFY_DONATE_BOOK",
        "STORE_BOOK",
        "DONATE_DONE",
        "NOTIFY_DONATE_DONE",
        "GET_MOST_POPULAR_BOOK",
        "GET_POPULAR_BK_INFO",
        "GET_MOST_POPULAR_BOOK_DONE",
        "CHECK_NUM_BOOKS_LOAN",
        "NUM_BOOKS_LOANED",
        "CHECK_NUM_BOOKS_LOAN_DONE",
    };

    return names[type];
}
