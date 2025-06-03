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
#include "../utils/storage/libdict.h"
//
#include <stdlib.h>
#include <mpi/mpi.h>
//

mpiproc_t get_neighbor(gridpos_t coords, mpiproc_t proc, int nid)
{
    int N = proc.N;

    switch (nid)
    {
    case 0: // LEFT neighbor
        return (mpiproc_t){.id = (coords.i % N) > 0 ? proc.id - 1 : -1};
    case 1: // TOP neighbor
        return (mpiproc_t){.id = (coords.j) < N - 1 ? proc.id + N : -1};
    case 2: // RIGHT neighbor
        return (mpiproc_t){.id = (coords.i % N) < N - 1 ? proc.id + 1 : -1};
    case 3: // BOTTOM neighbor
        return (mpiproc_t){.id = (coords.j) > 0 ? proc.id - N : -1};
    default:
        // Should never happen unless misused
        exit(EXIT_FAILURE);
    }
}

// Random value in range [MIN_BOOK_PRICE, MAX_BOOK_PRICE]
static bookcost_t random_book_cost()
{
    return MIN_BOOK_PRICE + (rand() % (MAX_BOOK_PRICE - MIN_BOOK_PRICE + 1));
}

library_t library_init(mpipid_t id, int N)
{
    library_t lib = {
        .node = {.id = id, .N = N},
        .leader = -1,
        .coords = (gridpos_t){.i = (id - 1) % N, .j = (id - 1) / N},
        .books = bookstorage_create(N * N), // Use N*N buckets
        .children = sllist_create(),
        .book_locations = NULL,
        .donated_locations = NULL,
    };

    for (int nid = 0; nid < 4; ++nid)
    {
        lib.neighbors[nid] = get_neighbor(lib.coords, lib.node, nid);
    }

    bookid_t b_id = (lib.node.id-1) * lib.node.N + 1;

    for (int i = 0; i < N; ++i)
    {
        // Mass store N books with id b_id
        bookstorage_mass_store(lib.books, (book_t){.id = b_id, .cost = random_book_cost()}, N);
        ++b_id;
    }

    return lib;
}

void all_neighbors_unexplored(library_t self, sllist_t unexplored)
{
    for (int i = 0; i < 4; ++i)
    {
        if (self.neighbors[i].id != -1)
        { // If neighbor exists
            sllist_add(unexplored, self.neighbors[i].id);
        }
    }
}

void print_book(bookstorage_node_t node, void *ignore){
    (void)ignore;
    DEBUG("Book: %d (x%d)", node->book.id, node->count);
}


static mpipid_t get_library_location(library_t self, bookid_t b_id){

    // First, check the donated books
    mpipid_t lib = libdict_find(self.donated_locations, b_id);

    if(lib != -1){
        return lib; // Found in donated books
    }else if (b_id <= self.node.N * self.node.N * self.node.N){ // N^3
        return self.book_locations[b_id-1]; // Refer to original book
    }else{
        // Book is not in donated or original
        ERROR("Book with id '%d' not found", b_id);
        exit(EXIT_FAILURE);
    }
}

void library_run(library_t self)
{
    DEBUG("Starting client '%d' execution", self.node.id);

    MPI_Datatype message_datatype;
    mpi_create_message_struct(
        &message_datatype);

    int leader = self.node.id;
    int parent = self.node.id;
    sllist_t unexplored = sllist_create();
    all_neighbors_unexplored(self, unexplored);

    while (TRUE)
    {
        bool explore = FALSE;

        message_t message;
        MPI_Status status;

        MPI_Recv(&message, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        int source = status.MPI_SOURCE;

        DEBUG("Received %s %d %d %d", message_type_to_str(message.type), message.args[0], message.args[1], message.args[2]);

        switch (message.type)
        {
        case MT_START_LEADER_ELECTION:
        {
            explore = TRUE;
            break;
        }
        case MT_LEADER:
        {
            if (leader < message.args[0])
            {
                // Incoming message with higher id, switch to new tree
                leader = message.args[0];
                parent = source;
                sllist_removeAll(self.children);
                all_neighbors_unexplored(self, unexplored);
                explore = TRUE;
            }
            else if (leader == message.args[0])
            {
                // Already in the same tree
                message_t already_msg = {
                    .type = MT_PARENT,
                    .args = {leader, -1, -1},
                };

                MPI_Send(&already_msg, 1, message_datatype, source, 0, MPI_COMM_WORLD);
            } // else the message is ignored (stalled) because it has a smaller id
            break;
        }
        case MT_ALREADY:
        {
            if (leader == message.args[0])
            {
                explore = TRUE;
            }
            break;
        }
        case MT_PARENT:
        {
            if (leader == message.args[0])
            {
                sllist_add(self.children, source);
                explore = TRUE;
            }
            break;
        }
        case MT_LE_LIBR:
        {
            self.leader = message.args[0]; // Leader id
            break;
        }
        case MT_LEND_BOOK:
        {
            bookid_t b_id = message.args[0];

            book_lookup_t res = bookstorage_take(self.books, b_id);

            if (res.successful)
            {
                // I already have the book

                message_t get = {
                    .type = MT_GET_BOOK,
                    .args = {b_id, -1, -1},
                };

                MPI_Send(&get, 1, message_datatype, source, 0, MPI_COMM_WORLD);
            }else{
                // Book not found, ask for it

                mpipid_t target_l_id;

                if (self.leader == -1){ // Just in case
                    ERROR("Library leader not elected, make sure to run a leader election before making queries to the library db");
                    exit(EXIT_FAILURE);
                }

                if(self.leader == self.node.id){ // If I'm the leader already
                    DEBUG("I'm already a leader!");
                    target_l_id = get_library_location(self, b_id); 
                    DEBUG("target is: %d", target_l_id);
                }else{

                    DEBUG("My leader is '%d'", self.leader);

                    message_t find = {
                        .type = MT_FIND_BOOK,
                        .args = {b_id, -1, -1},
                    };

                    MPI_Send(&find, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);

                    message_t response;
                    MPI_Recv(&response, 1, message_datatype, self.leader, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if (response.type != MT_BOOK_FOUND){
                        ERROR("Rogue '%s' received from '%d'", message_type_to_str(message.type), self.leader);
                        exit(EXIT_FAILURE);
                    }

                    target_l_id = response.args[0];

                }

                message_t request = {
                    .type = MT_BOOK_REQUEST,
                    .args = {b_id, source, self.node.id},
                };

                MPI_Send(&request, 1, message_datatype, target_l_id, 0, MPI_COMM_WORLD);
            }

            break;
        }
        case MT_UPD_DONATED: {
            if(self.node.id != self.leader){
                ERROR("Non-leader received '%s' from '%d'", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            bookid_t b_id     = message.args[0];
            bookid_t b_count  = message.args[1];
            mpipid_t l_id     = message.args[2];

            libdict_node_t entry = libdict_find_strict(self.donated_locations, b_id, l_id);
            if(entry){
                entry->count = b_count;
            }

            message_t ack = {
                .type = MT_ACK_UD,
                .args = {-1, -1, -1},
            };

            MPI_Send(&ack, 1, message_datatype, source, 0, MPI_COMM_WORLD);
            break;
        }
        case MT_ACK_TB: {
            bookid_t b_id    = message.args[0];
            mpipid_t c_id    = message.args[1];
            mpipid_t b_count = message.args[2];

            if(b_count != -1){
                if(self.node.id == self.leader){
                    libdict_node_t entry = libdict_find_strict(self.donated_locations, b_id, source);
                    if(entry){
                        entry->count = b_count; // Keep leader's count updated too
                    }
                }else{
                    message_t upd = {
                        .type = MT_UPD_DONATED,
                        .args = {b_id, b_count, source},
                    };

                    MPI_Send(&upd, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);

                    message_t ack;
                    MPI_Recv(&ack, 1, message_datatype, self.leader, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if(ack.type != MT_ACK_UD){
                        ERROR("Rogue '%s' received from leader, expected donation update ack");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            message_t ack = {
                .type = MT_ACK_TB,
                .args = {b_id, -1, -1},
            };

            MPI_Send(&ack, 1, message_datatype, c_id, 0, MPI_COMM_WORLD);
            break;
        }
        case MT_FIND_BOOK:
        {
            if (self.node.id != self.leader)
            {
                ERROR("Only the library leader (%d) can receive '%s'", self.leader, message_type_to_str(message.type));
                exit(EXIT_FAILURE);
            }

            mpipid_t b_id = message.args[0];

            mpipid_t target_l_id = get_library_location(self, b_id);

            message_t found = {
                .type = MT_BOOK_FOUND,
                .args = {target_l_id, -1, -1},
            };

            MPI_Send(&found, 1, message_datatype, source, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_BOOK_REQUEST: {
            bookid_t b_id = message.args[0];
            bookid_t c_id = message.args[1];
            
            bookstorage_node_t entry = bookstorage_find(self.books, b_id);
            if(!entry){
                ERROR("I don't have any book with id '%d'", b_id);
                exit(EXIT_FAILURE);
            }

            book_lookup_t res = bookstorage_take(self.books, b_id);

            if(!res.successful){
                ERROR("I don't have any remaining books with id '%d'", b_id);
                exit(EXIT_FAILURE);
            }

            message_t ack = {
                .type = MT_ACK_TB,
                .args = {b_id, c_id, entry->count},
            };

            MPI_Send(&ack, 1, message_datatype, source, 0, MPI_COMM_WORLD);

            break;
        }
        default:
            sllist_destroy(unexplored); // Not needed anymore
            ERROR("Unexpected message '%s' received from %d", message_type_to_str(message.type), source);
            exit(EXIT_FAILURE);
        }

        if (explore)
        {
            if (!sllist_empty(unexplored))
            {
                mpipid_t proc_id = sllist_removeOne(unexplored);
                message_t explore_msg = {
                    .type = MT_LEADER,
                    .args = {leader, -1, -1},
                };

                DEBUG("Sending '%s' to '%d'", message_type_to_str(explore_msg.type), proc_id);
                MPI_Send(&explore_msg, 1, message_datatype, proc_id, 0, MPI_COMM_WORLD);
            }
            else
            {
                if (parent != self.node.id)
                {
                    message_t parent_msg = {
                        .type = MT_PARENT,
                        .args = {leader, -1, -1},
                    };

                    DEBUG("Sending '%s' to '%d'", message_type_to_str(parent_msg.type), parent);
                    MPI_Send(&parent_msg, 1, message_datatype, parent, 0, MPI_COMM_WORLD);
                }
                else
                {
                    // I'm root so send elect done to coordinator

                    size_t book_locations_len = self.node.N * self.node.N * self.node.N; // N^3 instead of N^4

                    // Create book locations array if it doesn't exist (this should always hold unless misused)
                    if (!self.book_locations)
                    {
                        self.book_locations = safe_malloc(book_locations_len * sizeof(mpipid_t));

                        // Initialize entries
                        for (size_t b_id = 0; b_id < book_locations_len; ++b_id)
                        {
                            self.book_locations[b_id] = b_id / self.node.N + 1; // N^2
                        }
                    }

                    // Create donated locations hashtable if it doesn't exist (this should always hold unless misused)
                    if (!self.donated_locations)
                    {
                        self.donated_locations = libdict_create(self.node.N);
                    }

                    message_t elect_result = {
                        .type = MT_LE_LIBR,
                        .args = {self.node.id, -1, -1},
                    };

                    self.leader = self.node.id;

                    // Broadcast election result to all libraries except self
                    for (int id = 1; id <= self.node.N * self.node.N; ++id)
                    { // All library ids
                        if (id == self.node.id)
                            continue;

                        MPI_Send(&elect_result, 1, message_datatype, id, 0, MPI_COMM_WORLD);
                        DEBUG("Sent '%s' to %d", message_type_to_str(elect_result.type), id);
                    }

                    message_t elect_done_msg = {
                        .type = MT_LE_LIBR_DONE,
                        .args = {self.node.id, -1, -1},
                    };

                    MPI_Send(&elect_done_msg, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);
                }
            }
        }
    }

    sllist_destroy(unexplored); // Not needed anymore
}
