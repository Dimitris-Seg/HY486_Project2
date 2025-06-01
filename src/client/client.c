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
#include <assert.h>
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

void send_to_neighbor(sllist_node_t node, void *datatype){
    assert(node && datatype);
    
    mapped_message_t data = *((mapped_message_t*)datatype);
    MPI_Datatype message_datatype = *(data.datatype);
    message_t message = *(data.message);

    MPI_Send(&message, 1, message_datatype, node->elem, 0, MPI_COMM_WORLD);
}

void client_run(client_t self){
    DEBUG("Starting client '%d' execution", self.node.id);

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    sllist_t pending_acks       = sllist_create(); // Contains all pending acks
    sllist_t elects_received    = sllist_create(); // Contains all received 'elect' messages from neighbors
    mpipid_t last_elect_target  = -1;              // Contains the recipient of the last sent 'elect'

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
                .args = {-1, -1, -1},
            };

            MPI_Send(&ack, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_START_LE_LOANERS: {
            if(source != COORDINATOR_INDEX){ // Non-coordinator sent LE start, invalid behaviour
                ERROR("Received '%s' from process '%d' (only expected from coordinator)", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            if(sllist_length(self.neigbors) == 1){
                // Leaf node, start convergecast

                message_t elect = {
                    .type = MT_ELECT,
                    .args = {-1, -1, -1},
                };
                sllist_map(
                    self.neigbors, 
                    send_to_neighbor, 
                    &(mapped_message_t){
                        .datatype = &message_datatype, 
                        .message  = &elect,
                    }
                );
            }
            
            break;
        }
        case MT_ELECT: { // TODO: Pick up where you left off, second case in the algorithm termination, this is not done yet
            if(sllist_search(elects_received, source)){
                ERROR("Received duplicate '%s' from process '%d'", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            sllist_add(elects_received, source);

            if(sllist_length(self.neigbors) - sllist_length(elects_received) == 1){
                // Send elect to only non-elected neighbor
            
                // Here we find the only pending neighbor
                sllist_node_t it = self.neigbors->head;
                while(it){
                    if(!sllist_search(elects_received, it->elem)){
                        // This is the only pending neighbor
                        message_t elect = {
                            .type = MT_ELECT,
                            .args = {-1, -1, -1},
                        };
                    
                        DEBUG("Sending '%s' to '%d'", message_type_to_str(elect.type), it->elem);
                        MPI_Send(&elect, 1, message_datatype, it->elem, 0, MPI_COMM_WORLD);

                        last_elect_target = it->elem;

                        break;
                    }
                    it = it->next;
                }
            }else if(last_elect_target == source){
                // 'Elect' messages sent by both processes to eachother, resolve winner on rank

                if(self.node.id > source){ // I'm the winner
                    message_t elect_done = {
                        .type = MT_LE_LOANERS,
                        .args = {self.node.id, -1, -1},
                    };

                    message_t elect_res = {
                        .type = MT_LE_LOANERS_DONE,
                        .args = {self.node.id, -1, -1},
                    };
                
                    INFO("Sending '%s' to all neighbors", message_type_to_str(elect_done.type));

                    sllist_map(
                        self.neigbors, 
                        send_to_neighbor, 
                        &(mapped_message_t){
                            .datatype = &message_datatype, 
                            .message  = &elect_done,
                        }
                    );

                    INFO("Sending '%s' to coordinator", message_type_to_str(elect_res.type));
                    MPI_Send(&elect_res, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);
                }
            }

            break;
        }
        case MT_LE_LOANERS: {
            if(!sllist_empty(elects_received)){ // First LE_LOANERS
                message_t elect_done = {
                    .type = MT_LE_LOANERS,
                    .args = {message.args[0], -1, -1},
                };
                
                sllist_remove(elects_received, source);

                // Send to all but the one who sent it to me
                sllist_map(
                    elects_received, 
                    send_to_neighbor, 
                    &(mapped_message_t){
                        .datatype = &message_datatype, 
                        .message  = &elect_done,
                    }
                );

                // Reset state
                sllist_removeAll(elects_received);
                last_elect_target = -1;
            }
            break;
        }
        default:
            ERROR("Unexpected message '%s' received from %d", message_type_to_str(message.type), source);
            exit(EXIT_FAILURE);
        }
    }
}