/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file coordinator.c
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

#include "coordinator.h"
#include "../utils/storage/sllist.h"
#include "../utils/message.h"
#include "../utils/book.h"
//
#include <mpi/mpi.h>
//

coordinator_t coordinator_init(mpipid_t id, int N, string instr_file){
    coordinator_t coordinator = {
        .instructions = read_instructions(instr_file),
        .node = {.id = id, .N = N},
        .lib_leader = {.id = -1, .N = N},
    };

    return coordinator;
}

static void coordinator_connect(coordinator_t* self, int src, int target){
    (void)self; // Unused

    message_t message = {
        .type = MT_CONNECT,
        .args = {src, target, -1},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    MPI_Send(&message, 1, message_datatype, src, 0, MPI_COMM_WORLD);
    DEBUG("Sent '%s' to %d", message_type_to_str(MT_CONNECT), src);

    message_t response;

    MPI_Recv(&response, 1, message_datatype, src, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if(response.type != MT_ACK){
        ERROR("Expected '%s' from node '%d', got '%s'", message_type_to_str(MT_ACK), target, message_type_to_str(response.type));
        exit(EXIT_FAILURE);
    }

    DEBUG("Received '%s' from %d", message_type_to_str(MT_ACK), src);
}

static void coordinator_le_libr(coordinator_t* self){
    message_t elect_msg = {
        .type = MT_START_LEADER_ELECTION,
        .args = {-1, -1, -1},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    // Broadcast election start to all libraries
    for(int id=1; id<=self->node.N*self->node.N; ++id){ // All library ids
        MPI_Send(&elect_msg, 1, message_datatype, id, 0, MPI_COMM_WORLD);
        DEBUG("Sent '%s' to %d", message_type_to_str(elect_msg.type), id);
    }

    // Receive election done from elected process (leader)
    message_t response;
    MPI_Status status;
    MPI_Recv(&response, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //
    if(response.type != MT_LE_LIBR_DONE){ // Enforce strict check if we didn't expect this
        ERROR("Received rogue '%s' from process '%d'", message_type_to_str(response.type), status.MPI_SOURCE);
        exit(EXIT_FAILURE);
    }

    int leader_id = status.MPI_SOURCE;

    DEBUG("Received '%s' from %d", message_type_to_str(response.type), status.MPI_SOURCE);
    DEBUG("Elected library leader is '%d'", leader_id);

    self->lib_leader.id = leader_id;
}

static void coordinator_le_loaners(coordinator_t* self){
    message_t elect_msg = {
        .type = MT_START_LE_LOANERS,
        .args = {-1, -1, -1},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    // Broadcast election start to all loaners
    int start_index = self->node.N*self->node.N+1;
    for(int id=start_index; id<start_index+(self->node.N*self->node.N*self->node.N)/2; ++id){ // All loaner ids
        MPI_Send(&elect_msg, 1, message_datatype, id, 0, MPI_COMM_WORLD);
        DEBUG("Sent '%s' to %d", message_type_to_str(elect_msg.type), id);
    }

    // Receive election done from elected process (leader)
    message_t response;
    MPI_Status status;
    MPI_Recv(&response, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    //
    if(response.type != MT_LE_LOANERS_DONE){ // Enforce strict check if we didn't expect this
        ERROR("Received rogue '%s' from process '%d'", message_type_to_str(response.type), status.MPI_SOURCE);
        exit(EXIT_FAILURE);
    }

    int leader_id = status.MPI_SOURCE;

    DEBUG("Received '%s' from %d", message_type_to_str(response.type), status.MPI_SOURCE);
    DEBUG("Elected loaner leader is '%d'", leader_id);

    self->loaner_leader.id = leader_id;
}

static void coordinator_take_book(coordinator_t* self, int c_id, int b_id){
    (void)self; // Unused

    message_t take_msg = {
        .type = MT_TAKE_BOOK,
        .args = {b_id, -1, -1},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    MPI_Send(&take_msg, 1, message_datatype, c_id, 0, MPI_COMM_WORLD);
    DEBUG("Sent '%s' to %d", message_type_to_str(take_msg.type), c_id);

    message_t response;
    MPI_Status status;
    MPI_Recv(&response, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    if(response.type != MT_DONE_FIND_BOOK){
        ERROR("Received rogue '%s' from process '%d'", message_type_to_str(response.type), status.MPI_SOURCE);
        exit(EXIT_FAILURE);
    }
}

static void coordinator_donate_book(coordinator_t* self, int c_id, int b_id, int count){
    (void)self; // Unused

    message_t donate_msg = {
        .type = MT_NOTIFY_DONATE_BOOK,
        .args = {c_id, b_id, count},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    MPI_Send(&donate_msg, 1, message_datatype, c_id, 0, MPI_COMM_WORLD);
    DEBUG("Sent '%s' to %d", message_type_to_str(donate_msg.type), c_id);

    DEBUG("RECEIVING RESPONSE NOTIFY! (%d)", c_id);

    message_t response;
    MPI_Recv(&response, 1, message_datatype, c_id, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    DEBUG("RECEIVED RESPONSE NOTIFY!");

    if(response.type != MT_NOTIFY_DONATE_DONE){
        ERROR("Received rogue '%s' from process '%d'", message_type_to_str(response.type), c_id);
        exit(EXIT_FAILURE);
    }
}

static void coordinator_get_most_popular_book(coordinator_t* self){
    message_t get_most_popular_msg = {
        .type = MT_GET_MOST_POPULAR_BOOK,
        .args = {-1, -1, -1},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );
    
    if(self->loaner_leader.id==-1){
        ERROR("Loaner leader election required before most popular book can be acquired");
        exit(EXIT_FAILURE);
    }

    MPI_Send(&get_most_popular_msg, 1, message_datatype, self->loaner_leader.id, 0, MPI_COMM_WORLD);

    message_t response;
    MPI_Recv(&response, 1, message_datatype, self->loaner_leader.id, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if(response.type != MT_GET_MOST_POPULAR_BOOK_DONE){
        ERROR("Received rogue '%s' from process '%d'", message_type_to_str(response.type), self->loaner_leader.id);
        exit(EXIT_FAILURE);
    }   
}

static void coordinator_check_num_books_loaned(coordinator_t* self){
    // Implementation pending...
}

void coordinator_run(coordinator_t* self){
    size_t coordinator_index = 0;

    static const string tokennames[] = {
        "CONNECT", 
        "START_LE_LIBR", 
        "START_LE_LOANERS", 
        "TAKE_BOOK", 
        "DONATE_BOOK", 
        "GET_MOST_POPULAR_BOOK", 
        "CHECK_NUM_BOOKS_LOANED",
        "INT",
    };

    while(coordinator_index < self->instructions->len){
        token_t token = self->instructions->tokens[coordinator_index];

        switch (token.type){
        case TT_CONNECT:{                // 2 int args
            token_t node_src = self->instructions->tokens[++coordinator_index];
            token_t node_tgt = self->instructions->tokens[++coordinator_index];

            ENFORCE_ARG_TYPE(node_src.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_tgt.type, TT_INT, token.type);

            INFO("CONNECT(%d, %d)", node_src.val+1, node_tgt.val+1);
            coordinator_connect(self, node_src.val+1, node_tgt.val+1);
            break;
        }
        case TT_START_LE_LIBR:{          // no args
            INFO("TT_START_LE_LIBR()");
            coordinator_le_libr(self);
            break;
        }
        case TT_START_LE_LOANERS:{       // no args
            INFO("TT_START_LE_LOANERS()");
            coordinator_le_loaners(self);
            break;
        }
        case TT_TAKE_BOOK:{              // 2 int args
            token_t node_who = self->instructions->tokens[++coordinator_index];
            token_t node_which = self->instructions->tokens[++coordinator_index];

            ENFORCE_ARG_TYPE(node_who.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_which.type, TT_INT, token.type);

            INFO("TT_TAKE_BOOK(%d, %d)", node_who.val+1, node_which.val);
            coordinator_take_book(self, node_who.val+1, node_which.val);
            break;
        }
        case TT_DONATE_BOOK:{            // 3 int args
            token_t node_who = self->instructions->tokens[++coordinator_index];
            token_t node_which = self->instructions->tokens[++coordinator_index];
            token_t node_count = self->instructions->tokens[++coordinator_index];

            ENFORCE_ARG_TYPE(node_who.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_which.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_count.type, TT_INT, token.type);

            INFO("TT_DONATE_BOOK(%d, %d, %d)", node_who.val+1, node_which.val, node_count.val);
            coordinator_donate_book(self, node_who.val+1, node_which.val, node_count.val);
            break;
        }
        case TT_GET_MOST_POPULAR_BOOK:{  // no args
            INFO("TT_GET_MOST_POPULAR_BOOK()");
            coordinator_get_most_popular_book(self);
            break;
        }
        case TT_CHECK_NUM_BOOKS_LOANED:{ // no args
            INFO("TT_CHECK_NUM_BOOKS_LOANED()");
            coordinator_check_num_books_loaned(self);
            break;
        }
        default:{
            // Invalid program format
            ERROR("Invalid instruction syntax for coordinator, unexpected token '%s' (token index %lu)", tokennames[token.type], coordinator_index);
            exit(EXIT_FAILURE);
        }

        }

        ++coordinator_index; // Next instruction
    }
}