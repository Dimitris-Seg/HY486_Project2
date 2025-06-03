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
        .node           = {.id = id, .N = N},
        .leader         = -1,
        .donate_next    = 0, // First library in order (only used by the elected leader)
        .neigbors       = sllist_create(),
        .books          = bookstorage_create(N*N), // Use N*N buckets
    };

    return client;
}

void print_neighbor(sllist_node_t node, void *ignored){
    (void)ignored; // Ignore waring

    DEBUG("'%d' is a neighbor", node->elem);
}

// Dynamically compute the client's corresponding library
static mpipid_t mylib(client_t self){
    
    mpipid_t nsq = self.node.N*self.node.N;
    return 2*(self.node.id-nsq)/((nsq*self.node.N)/(double)nsq)+1;
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
    sllist_t pending_takes      = sllist_create(); // Contains all pending take book requests
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
        case MT_ELECT: {
            if(sllist_search(elects_received, source)){
                ERROR("Received duplicate '%s' from process '%d'", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            sllist_add(elects_received, source);

            size_t pending_neighbors = sllist_length(self.neigbors) - sllist_length(elects_received);

            if(pending_neighbors == 1){
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
            }else{
                // Case 1: 'Elect' messages sent by both processes to eachother, resolve winner on rank
                // Case 2: I'm the last process to receive all elects, I'm definitely the winner
                if((last_elect_target == source && self.node.id > source)||(pending_neighbors==0 && last_elect_target == -1)){ 
                    // I'm the winner

                    message_t elect_done = {
                        .type = MT_LE_LOANERS,
                        .args = {self.node.id, -1, -1},
                    };

                    message_t elect_res = {
                        .type = MT_LE_LOANERS_DONE,
                        .args = {-1, -1, -1},
                    };

                    self.leader = self.node.id; // Update local leader field

                    INFO("Sending '%s' to all neighbors", message_type_to_str(elect_done.type));

                    // Broadcast election start to all other loaners
                    int start_index = self.node.N*self.node.N+1;
                    for(int id=start_index; id<start_index+(self.node.N*self.node.N*self.node.N)/2; ++id){ // All loaner ids
                        if(id!=self.node.id){
                            MPI_Send(&elect_done, 1, message_datatype, id, 0, MPI_COMM_WORLD);
                            DEBUG("Sent '%s' to %d", message_type_to_str(elect_done.type), id);
                        }
                    }

                    INFO("Sending '%s' to coordinator", message_type_to_str(elect_res.type));
                    MPI_Send(&elect_res, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);
                }
            }

            break;
        }
        case MT_LE_LOANERS: {
            // Reset state
            sllist_removeAll(elects_received);
            last_elect_target = -1;
            self.leader = source; // Update local leader field
            break;
        }
        case MT_TAKE_BOOK: {

            bookid_t b_id = message.args[0];
            mpipid_t l_id = mylib(self);

            message_t lend = {
                .type = MT_LEND_BOOK,
                .args = {b_id, self.node.id, -1},
            };

            sllist_add(pending_takes, l_id);
        
            DEBUG("Sending '%s' to '%d'", message_type_to_str(lend.type), l_id);
            MPI_Send(&lend, 1, message_datatype, l_id, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_ACK_TB  : // Cascade
        case MT_GET_BOOK: {

            bookid_t b_id   = message.args[0];

            if(!sllist_search(pending_takes, source)){
                ERROR("Received rogue '%s' from '%d'", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            sllist_remove(pending_takes, source); // Consume pending take

            bookstorage_store(self.books, (book_t){.id = b_id}); // We don't care about the cost (for now)

            message_t done = {
                .type = MT_DONE_FIND_BOOK,
                .args = {-1, -1, -1},
            };
        
            DEBUG("Sending '%s' to '%d'", message_type_to_str(done.type), COORDINATOR_INDEX);
            MPI_Send(&done, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);
            break;
        }
        case MT_NOTIFY_DONATE_BOOK: {
            if(self.leader == -1){
                ERROR("Unspecified Client/Borrower leader, ensure leader election before any donate events");
                exit(EXIT_FAILURE);
            }
            
            bookid_t b_id = message.args[1];
            size_t copies = message.args[2];

            message_t donate = {
                .type = MT_DONATE_BOOK,
                .args = {b_id, copies, -1},
            };
        
            DEBUG("Sending '%s' to '%d'", message_type_to_str(donate.type), self.leader);
            MPI_Send(&donate, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);
            break;
        }
        case MT_DONATE_BOOK: {
            if(self.node.id != self.leader){
                ERROR("Non-leader received leader-only event '%s' from '%d'", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            bookid_t b_id = message.args[0];
            size_t copies = message.args[1];

            message_t donate = {
                .type = MT_STORE_BOOK,
                .args = {b_id, -1, -1},
            };

            for(size_t copy = 0; copy < copies; ++copy){
                mpipid_t lib = (self.donate_next++)%(self.node.N*self.node.N) + 1; // donate_next/(N^2)
                DEBUG("Sending '%s' to '%d'", message_type_to_str(donate.type), lib);
                MPI_Send(&donate, 1, message_datatype, lib, 0, MPI_COMM_WORLD);

                message_t ack;
                MPI_Recv(&ack, 1, message_datatype, lib, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if(ack.type != MT_ACK_DB){
                    ERROR("Rogue '%s' from '%d'",message_type_to_str(ack.type), lib);
                    exit(EXIT_FAILURE);
                }
            }

            message_t done;
            mpipid_t target;
            
            // If I'm the leader already send this back to the coordinator right away
            if(source == self.node.id){
                done = (message_t){
                    .type = MT_NOTIFY_DONATE_DONE,
                    .args = {-1, -1, -1},
                };

                target = COORDINATOR_INDEX;
            }else{
                done = (message_t){
                    .type = MT_DONATE_DONE,
                    .args = {-1, -1, -1},
                };

                target = source;
            }

            MPI_Send(&done, 1, message_datatype, target, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_DONATE_DONE: {
            message_t done = {
                .type = MT_NOTIFY_DONATE_DONE,
                .args = {-1, -1, -1},
            };

            MPI_Send(&done, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);

            break;
        }
        default:
            ERROR("Unexpected message '%s' received from %d", message_type_to_str(message.type), source);
            exit(EXIT_FAILURE);
        }
    }
}