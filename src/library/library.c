/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file library.c
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

#include "library.h"
#include "../utils/message.h"
//
#include <stdlib.h>
#include <mpi/mpi.h>
//

mpiproc_t get_neighbor(gridpos_t coords, mpiproc_t proc, int nid){
    int N = proc.N;

    switch(nid){
    case 0: // LEFT neighbor
        return (mpiproc_t){.id=(coords.i%N)>0?
            proc.id-1:-1 
        };
    case 1: // TOP neighbor
        return (mpiproc_t){.id=(coords.j)<N-1?
            proc.id+N:-1 
        };
    case 2: // RIGHT neighbor
        return (mpiproc_t){.id=(coords.i%N)<N-1?
            proc.id+1:-1 
        };
    case 3: // BOTTOM neighbor
        return (mpiproc_t){.id=(coords.j)>0?
            proc.id-N:-1 
        };
    default:
        // Should never happen unless misused
        exit(EXIT_FAILURE);
    }
}

// Random value in range [MIN_BOOK_PRICE, MAX_BOOK_PRICE]
static bookcost_t random_book_cost(){
    return MIN_BOOK_PRICE+(rand()%(MAX_BOOK_PRICE-MIN_BOOK_PRICE+1));
}

library_t library_init(mpipid_t id, int N){
    library_t lib = {
        .node           = {.id = id, .N = N},
        .coords         = (gridpos_t){.i = (id-1)%N, .j = (id-1)/N},
        .books          = bookstorage_create(N*N), // Use N*N buckets
        .children       = sllist_create(),
    };

    for(int nid = 0;nid < 4; ++nid){
        lib.neighbors[nid] = get_neighbor(lib.coords, lib.node, nid);
    }

    bookid_t b_id = lib.node.id;

    for(int i=0;i<N;++i){
        // Mass store N books with id b_id
        bookstorage_mass_store(lib.books, (book_t){.id = b_id, .cost = random_book_cost()}, N);
        ++b_id;
    }

    return lib;
}

void all_neighbors_unexplored(library_t self, sllist_t unexplored){
    for(int i=0;i<4;++i){
        if(self.neighbors[i].id!=-1){ // If neighbor exists
            sllist_add(unexplored, self.neighbors[i].id);
        }
    }
}

void library_run(library_t self){
    DEBUG("Starting client '%d' execution", self.node.id);

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype
    );

    int leader = self.node.id;
    int parent = self.node.id;
    sllist_t unexplored = sllist_create();
    all_neighbors_unexplored(self, unexplored);

    while(TRUE){
        bool explore = FALSE;

        message_t message;
        MPI_Status status;

        MPI_Recv(&message, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        int source = status.MPI_SOURCE;

        DEBUG("Received %s %d %d %d", message_type_to_str(message.type), message.args[0], message.args[1], message.args[2]);

        switch(message.type){
        case MT_START_LEADER_ELECTION: {
            explore = TRUE;
            break;
        }
        case MT_LEADER: {
            if(leader < message.args[0]){
                // Incoming message with higher id, switch to new tree
                leader = message.args[0];
                parent = source;
                sllist_removeAll(self.children);
                all_neighbors_unexplored(self, unexplored);
                explore = TRUE;
            }else if(leader == message.args[0]){
                // Already in the same tree
                message_t already_msg = {
                    .type = MT_PARENT,
                    .args = {leader, -1, -1},
                };

                MPI_Send(&already_msg, 1, message_datatype, source, 0, MPI_COMM_WORLD);
            } // else the message is ignored (stalled) because it has a smaller id
            break;
        }
        case MT_ALREADY: {
            if(leader == message.args[0]){
                explore = TRUE;
            }
            break;
        }
        case MT_PARENT: {
            if(leader == message.args[0]){
                sllist_add(self.children, source);
                explore = TRUE;
            }
            break;
        }
        default:
            sllist_destroy(unexplored); // Not needed anymore
            ERROR("Unexpected message '%s' received from %d", message_type_to_str(message.type), source);
            exit(EXIT_FAILURE);
        }

        if(explore){
            if(!sllist_empty(unexplored)){
                mpipid_t proc_id = sllist_removeOne(unexplored);
                message_t explore_msg = {
                    .type = MT_LEADER,
                    .args = {leader, -1, -1},
                };
    
                DEBUG("Sending '%s' to '%d'", message_type_to_str(explore_msg.type), proc_id);
                MPI_Send(&explore_msg, 1, message_datatype, proc_id, 0, MPI_COMM_WORLD);
            }else{
                if(parent != self.node.id){
                    message_t parent_msg = {
                        .type = MT_PARENT,
                        .args = {leader, -1, -1},
                    };
        
                    DEBUG("Sending '%s' to '%d'", message_type_to_str(parent_msg.type), parent);
                    MPI_Send(&parent_msg, 1, message_datatype, parent, 0, MPI_COMM_WORLD);
                }else{
                    // I'm root so send elect done to coordinator

                    message_t elect_done_msg = {
                        .type = MT_LE_LIBR_DONE,
                        .args = {leader, -1, -1},
                    };

                    MPI_Send(&elect_done_msg, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);
                }
            }
        }
    }

    sllist_destroy(unexplored); // Not needed anymore
}

