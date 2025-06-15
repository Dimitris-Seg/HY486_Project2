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

int Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){
    int res = MPI_Send(buf, count, datatype, dest, tag, comm);
    return res;
}

int Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status){
    int res = MPI_Recv(buf, count, datatype, source, tag, comm,  status);
    return res;
}

void cleanup_mpi_in_buffers(){
    MPI_Status status;
    int flag;
    do {
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            message_t discarded;
            MPI_Datatype message_datatype;
            mpi_create_message_struct(
                &message_datatype
            );
            Recv(&discarded, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        }
    } while (flag);
}

void mpi_create_message_struct(MPI_Datatype *newtype){
    message_t message;

    int block_lengths[] = {1, 4};
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
        "MT_ACK_LE_LIBR",
        "LEADER",
        "ALREADY",
        "PARENT",
        "START_LE_LOANERS",
        "LE_LOANERS",
        "MT_ACK_LE_LOANERS",
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
        "TERMINATE",
    };

    return names[type];
}
