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

static void coordinator_connect(coordinator_t self, int src, int target){
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

static void coordinator_le_libr(coordinator_t self){
    message_t elect_msg = {
        .type = MT_START_LEADER_ELECTION,
        .args = {-1, -1, -1},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    // Broadcast election start to all libraries
    for(int id=1; id<=self.node.N*self.node.N; ++id){ // All library ids
        MPI_Send(&elect_msg, 1, message_datatype, id, 0, MPI_COMM_WORLD);
        DEBUG("Sent '%s' to %d", message_type_to_str(MT_START_LEADER_ELECTION), id);
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

    self.lib_leader.id = leader_id;
}

static void coordinator_le_loaners(coordinator_t self){
    message_t elect_msg = {
        .type = MT_START_LE_LOANERS,
        .args = {-1, -1, -1},
    };

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    // Broadcast election start to all libraries
    int start_index = self.node.N*self.node.N+1;
    for(int id=start_index; id<start_index+(self.node.N*self.node.N*self.node.N)/2; ++id){ // All library ids
        MPI_Send(&elect_msg, 1, message_datatype, id, 0, MPI_COMM_WORLD);
        DEBUG("Sent '%s' to %d", message_type_to_str(MT_START_LEADER_ELECTION), id);
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

    self.loaner_leader.id = leader_id;
}

static void coordinator_take_book(coordinator_t self, int c_id, int b_id){
    // Implementation pending...
}

static void coordinator_donate_book(coordinator_t self, int c_id, int b_id, int count){
    // Implementation pending...
}

static void coordinator_get_most_popular_book(coordinator_t self){
    // Implementation pending...
}

static void coordinator_check_num_books_loaned(coordinator_t self){
    // Implementation pending...
}

void coordinator_run(coordinator_t self){
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

    while(coordinator_index < self.instructions->len){
        token_t token = self.instructions->tokens[coordinator_index];

        switch (token.type){
        case TT_CONNECT:{                // 2 int args
            token_t node_src = self.instructions->tokens[++coordinator_index];
            token_t node_tgt = self.instructions->tokens[++coordinator_index];

            ENFORCE_ARG_TYPE(node_src.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_tgt.type, TT_INT, token.type);

            DEBUG("CONNECT(%d, %d)", node_src.val, node_tgt.val);
            coordinator_connect(self, node_src.val, node_tgt.val);
            break;
        }
        case TT_START_LE_LIBR:{          // no args
            DEBUG("TT_START_LE_LIBR()");
            coordinator_le_libr(self);
            break;
        }
        case TT_START_LE_LOANERS:{       // no args
            DEBUG("TT_START_LE_LOANERS()");
            coordinator_le_loaners(self);
            break;
        }
        case TT_TAKE_BOOK:{              // 2 int args
            token_t node_who = self.instructions->tokens[++coordinator_index];
            token_t node_which = self.instructions->tokens[++coordinator_index];

            ENFORCE_ARG_TYPE(node_who.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_which.type, TT_INT, token.type);

            DEBUG("TT_TAKE_BOOK(%d, %d)", node_who.val, node_which.val);
            coordinator_take_book(self, node_who.val, node_which.val);
            break;
        }
        case TT_DONATE_BOOK:{            // 3 int args
            token_t node_who = self.instructions->tokens[++coordinator_index];
            token_t node_which = self.instructions->tokens[++coordinator_index];
            token_t node_count = self.instructions->tokens[++coordinator_index];

            ENFORCE_ARG_TYPE(node_who.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_which.type, TT_INT, token.type);
            ENFORCE_ARG_TYPE(node_count.type, TT_INT, token.type);

            DEBUG("TT_DONATE_BOOK(%d, %d, %d)", node_who.val, node_which.val, node_count.val);
            coordinator_donate_book(self, node_who.val, node_which.val, node_count.val);
            break;
        }
        case TT_GET_MOST_POPULAR_BOOK:{  // no args
            DEBUG("TT_GET_MOST_POPULAR_BOOK()");
            coordinator_get_most_popular_book(self);
            break;
        }
        case TT_CHECK_NUM_BOOKS_LOANED:{ // no args
            DEBUG("TT_CHECK_NUM_BOOKS_LOANED()");
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