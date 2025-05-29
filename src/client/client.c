/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file client.c
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

#include "client.h"
#include "../utils/general.h"
#include "../utils/message.h"
#include "../utils/logging.h"
//
#include <mpi/mpi.h>
//

client_t client_init(mpipid_t id, int N){
    client_t client = {
        .node     = {.id = id, .N = N},
        .neigbors = sllist_create(),
        .books    = bookstorage_create(N*N), // Use N*N buckets
    };

    return client;
}

void print_neighbor(sllist_node_t node, void *ignored){
    (void)ignored; // Ignore waring

    DEBUG("'%d' is a neighbor", node->elem);
}

void client_run(client_t self){
    DEBUG("Starting client '%d' execution", self.node.id);

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    sllist_t pending_acks = sllist_create(); // Contains all pending acks

    // Until termination signal
    while(TRUE){
        message_t message;

        MPI_Status status;

        MPI_Recv(&message, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        int source = status.MPI_SOURCE;

        DEBUG("Received %s %d %d %d", message_type_to_str(message.type), message.args[0], message.args[1], message.args[2]);

        switch(message.type){
        case MT_CONNECT:{
            if(source != COORDINATOR_INDEX){ // Non-coordinator sent connect, invalid behaviour
                ERROR("Received '%s' from process '%d' (only expected from coordinator)", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            mpipid_t neighbor_id = message.args[1];

            sllist_add_unique(self.neigbors, neighbor_id); // Record neighbor
            sllist_add_unique(pending_acks, neighbor_id);  // Add neighbor to pending acks

            //sllist_map(self.neigbors, print_neighbor, NULL); // Prints all neighbors of process

            message_t neighbor = {
                .type = MT_NEIGHBOR,
                .args = {message.args[0], message.args[1], -1},
            };

            MPI_Send(&neighbor, 1, message_datatype, neighbor_id, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_NEIGHBOR: {
            if(source == COORDINATOR_INDEX){ // The coordinator sent a neighbor request, invalid behaviour
                ERROR("Received '%s' from coordinator (invalid behaviour)", message_type_to_str(message.type));
                exit(EXIT_FAILURE);
            }

            mpipid_t neighbor_id = message.args[0];

            sllist_add_unique(self.neigbors, neighbor_id); // Record neighbor

            //sllist_map(self.neigbors, print_neighbor, NULL); // Prints all neighbors of process

            message_t ack = {
                .type = MT_ACK,
                .args = {-1, -1, -1},
            };

            DEBUG("Sending '%s' to '%d'", message_type_to_str(ack.type), neighbor_id);
            MPI_Send(&ack, 1, message_datatype, neighbor_id, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_ACK: {
            if(source == COORDINATOR_INDEX){ // The coordinator sent an ack, invalid behaviour
                ERROR("Received '%s' from coordinator (invalid behaviour)", message_type_to_str(message.type));
                exit(EXIT_FAILURE);
            }

            if(!sllist_search(pending_acks, source)){
                ERROR("Received rogue '%s' from process '%d'", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            sllist_remove(pending_acks, source); // Not pending anymore

            message_t ack = {
                .type = MT_ACK,
                .args = {0, 0, 0},
            };

            MPI_Send(&ack, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);

            break;
        }
        default:
            ERROR("Unexpected message '%s' received from %d", message_type_to_str(message.type), source);
            exit(EXIT_FAILURE);
        }
    }
}