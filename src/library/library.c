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
#include <assert.h>
//

mpiproc_t get_neighbor(gridpos_t coords, mpiproc_t proc, int nid){
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

mpipid_t next_route_in_grid(library_t self){
    // If y%2 is even, go right, else left
    // If x/(N-1) is 1, go up

    int nextI = self.coords.i - 2*(self.coords.j%2)+1;
    if(nextI>self.node.N-1)nextI = self.node.N-1;
    if(nextI<0)nextI = 0;

    // When even check if i is N-1, else if it's 0
    int nextJ = self.coords.j + (self.coords.i == ((1 - (self.coords.j%2)) * (self.node.N - 1))); 

    if(nextJ >= self.node.N){
        return -1; // Last one, no further routing in grid
    }

    return nextI + nextJ * self.node.N + 1;
}

// Random value in range [MIN_BOOK_PRICE, MAX_BOOK_PRICE]
static bookcost_t random_book_cost(){
    return MIN_BOOK_PRICE + (rand() % (MAX_BOOK_PRICE - MIN_BOOK_PRICE + 1));
}

library_t library_init(mpipid_t id, int N){
    library_t lib = {
        .node = {.id = id, .N = N},
        .leader = -1,
        .coords = (gridpos_t){.i = (id - 1) % N, .j = (id - 1) / N},
        .books = bookstorage_create(N * N),     // Use N*N buckets
        .donated = bookstorage_create(N * N),   //
        .children = sllist_create(),
        .book_locations = NULL,
        .donated_locations = NULL,
        .books_loaned = 0,
    };

    for (int nid = 0; nid < 4; ++nid){
        lib.neighbors[nid] = get_neighbor(lib.coords, lib.node, nid);
    }

    bookid_t b_id = (lib.node.id-1) * lib.node.N + 1;

    for (int i = 0; i < N; ++i){
        // Mass store N books with id b_id
        book_t book = (book_t){.id = b_id, .cost = random_book_cost()};
        DEBUG("Book '%d' cost is '%d'", book.id, book.cost);
        bookstorage_mass_store(lib.books, book, N);
        ++b_id;
    }

    return lib;
}

void all_neighbors_unexplored(library_t self, sllist_t unexplored){
    for (int i = 0; i < 4; ++i)
    {
        if (self.neighbors[i].id != -1)
        { // If neighbor exists
            sllist_add(unexplored, self.neighbors[i].id);
        }
    }
}

void print_book(bookstorage_node_t node, void *ignore){
    assert(node);
    (void)ignore;

    DEBUG("Book: %d (x%lu)", node->book.id, node->count);
}

void print_book_location(libdict_node_t node, void *ignore){
    assert(node);
    (void)ignore;
    
    DEBUG("Location: %d at %d (x%lu)", node->book.id, node->lib, node->count);
}

// TODO: Find how to fix books not found after donate, probably snatched from some other process? IDK,
//  maybe they need to retry on fail, or be able to query the remaining books or something

// Finds a library location, but also removes a donated entry if found
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

    bool le_started = FALSE; // For the leader to avoid leftover garbage

    int total_loaned_books = 0;
    sllist_t book_loaned_acks = sllist_create();

    sllist_t unexplored = sllist_create();
    all_neighbors_unexplored(self, unexplored);

    while (TRUE){
        bool explore = FALSE;

        message_t message;
        MPI_Status status;

        Recv(&message, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        int source = status.MPI_SOURCE;

        DEBUG("Received %s %d %d %d %d", message_type_to_str(message.type), message.args[0], message.args[1], message.args[2], message.args[3]);

        switch (message.type){
        case MT_START_LEADER_ELECTION:
        {
            le_started = TRUE;
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
                    .type = MT_ALREADY,
                    .args = {leader, -1, -1},
                };

                Send(&already_msg, 1, message_datatype, source, 0, MPI_COMM_WORLD);
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
            le_started = FALSE;

            self.leader = message.args[0]; // Leader id

            message_t ack = {
                .type = MT_ACK_LE_LIBR,
                .args = {-1, -1, -1},
            };
        
            cleanup_mpi_in_buffers(); // Clean up input buffer to avoid garbage

            DEBUG("Sending '%s' to '%d'", message_type_to_str(ack.type), source);
            Send(&ack, 1, message_datatype, source, 0, MPI_COMM_WORLD);
            break;
        }
        case MT_LEND_BOOK:
        {
            bool is_donated = FALSE;

            bookid_t b_id = message.args[0];

            book_lookup_t res = bookstorage_take(self.books, b_id);

            if(!res.successful){
                res = bookstorage_take(self.donated, b_id);
                is_donated = TRUE;
            }

            if (res.successful)
            {
                // I already have the book

                ++self.books_loaned; // Update loaned book tracking

                // If the book is donated we should update the leader with the new count
                if(is_donated){
                    if(self.leader==-1){
                        ERROR("Leader election required before donation");
                        exit(EXIT_FAILURE);
                    }
                    bookstorage_node_t entry = bookstorage_find(self.donated, b_id);
                    if(!entry){
                        ERROR("I never had any book with id '%d'", b_id);
                        exit(EXIT_FAILURE);
                    }

                    if(self.node.id != self.leader){
                        message_t upd_donated = {
                            .type = MT_UPD_DONATED,
                            .args = {b_id, entry->count, self.node.id},
                        };

                        Send(&upd_donated, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);

                        message_t ack;
                        Recv(&ack, 1, message_datatype, self.leader, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        if(ack.type != MT_ACK_UD){
                            ERROR("Received rogue '%s' from '%d'", message_type_to_str(ack.type), self.leader);
                            exit(EXIT_FAILURE);
                        }
                    }else{
                        // Update my own count since I'm the leader
                        libdict_update_count(self.donated_locations, (book_t){.id = b_id}, self.node.id, entry->count);
                    }
                }

                message_t get = {
                    .type = MT_GET_BOOK,
                    .args = {b_id, TRUE, res.book.cost},
                };

                Send(&get, 1, message_datatype, source, 0, MPI_COMM_WORLD);
            }else{
                // Book not found, ask for it

                mpipid_t target_l_id;

                if (self.leader == -1){ // Just in case
                    ERROR("Library leader not elected, make sure to run a leader election before making queries to the library db");
                    exit(EXIT_FAILURE);
                }

                if(self.leader == self.node.id){ // If I'm the leader already
                    target_l_id = get_library_location(self, b_id); 
                }else{
                    message_t find = {
                        .type = MT_FIND_BOOK,
                        .args = {b_id, -1, -1},
                    };

                    Send(&find, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);

                    message_t response;
                    Recv(&response, 1, message_datatype, self.leader, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if (response.type != MT_BOOK_FOUND){
                        ERROR("Rogue '%s' received from '%d'", message_type_to_str(message.type), self.leader);
                        exit(EXIT_FAILURE);
                    }

                    target_l_id = response.args[0];
                }

                if(target_l_id == self.node.id){
                    // If the target is me, then the book doesn't exist (or I would've had it)

                    ERROR("I don't have any remaining books with id '%d'", b_id);

                    message_t ack = {
                        .type = MT_ACK_TB,
                        .args = {b_id, -1, source, BOOK_NOT_FOUND},
                    };
        
                    Send(&ack, 1, message_datatype, source, 0, MPI_COMM_WORLD);
                }else{
                    message_t request = {
                        .type = MT_BOOK_REQUEST,
                        .args = {b_id, source, self.node.id},
                    };

                    Send(&request, 1, message_datatype, target_l_id, 0, MPI_COMM_WORLD);
                }
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

            libdict_update_count(self.donated_locations, (book_t){.id = b_id}, l_id, b_count);

            DEBUG("Leader updated their books, we have:");

            //libdict_map(self.donated_locations, print_book_location, NULL); // TODO: REMOVE TEST ONLY

            message_t ack = {
                .type = MT_ACK_UD,
                .args = {-1, -1, -1},
            };

            Send(&ack, 1, message_datatype, source, 0, MPI_COMM_WORLD);
            break;
        }
        case MT_ACK_TB: {
            bookid_t b_id     = message.args[0];
            bookcost_t b_cost = message.args[1];
            mpipid_t c_id     = message.args[2];
            mpipid_t b_count  = message.args[3];

            if(b_count != -1){
                if(self.node.id == self.leader){
                    libdict_node_t entry = libdict_find_strict(self.donated_locations, b_id, source);
                    if(entry){
                        entry->count = b_count; // Keep leader's count updated too
                    }
                }
                // else{
                //     message_t upd = {
                //         .type = MT_UPD_DONATED,
                //         .args = {b_id, b_count, source},
                //     };

                //     Send(&upd, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);

                //     message_t ack;
                //     Recv(&ack, 1, message_datatype, self.leader, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //     if(ack.type != MT_ACK_UD){
                //         ERROR("Rogue '%s' received from leader, expected donation update ack", message_type_to_str(ack.type));
                //         exit(EXIT_FAILURE);
                //     }
                // }
            }

            message_t ack = {
                .type = MT_ACK_TB,
                .args = {b_id, b_count, b_cost, -1},
            };

            Send(&ack, 1, message_datatype, c_id, 0, MPI_COMM_WORLD);
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

            Send(&found, 1, message_datatype, source, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_BOOK_REQUEST: {
            bookid_t b_id = message.args[0];
            bookid_t c_id = message.args[1];

            bookcost_t b_cost = -1;
            
            bookstorage_t storage = self.books;
            bookstorage_node_t entry = bookstorage_find(storage, b_id);

            // If not in original books look at donated
            if(!entry){
                storage = self.donated;
                entry = bookstorage_find(storage, b_id);
            }

            if(!entry){
                ERROR("I never had any book with id '%d'", b_id);
                exit(EXIT_FAILURE);
            }

            book_lookup_t res = bookstorage_take(storage, b_id);

            if(res.successful){
                b_cost = res.book.cost;
                ++self.books_loaned; // Update loaned book tracking
            }else{
                ERROR("I don't have any remaining books with id '%d'", b_id);
            }

            
            if(self.node.id == self.leader){
                // Update my own count since I'm the leader
                libdict_update_count(self.donated_locations, (book_t){.id = b_id}, self.node.id, entry->count);
            }else if(res.successful && storage == self.donated){
                // If the book is donated we should update the leader with the new count

                if(self.leader==-1){
                    ERROR("Leader election required before book retrieval");
                    exit(EXIT_FAILURE);
                }

                message_t upd_donated = {
                    .type = MT_UPD_DONATED,
                    .args = {b_id, entry->count, self.node.id},
                };

                Send(&upd_donated, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);

                message_t ack;
                Recv(&ack, 1, message_datatype, self.leader, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if(ack.type != MT_ACK_UD){
                    ERROR("Received rogue '%s' from '%d'", message_type_to_str(ack.type), self.leader);
                    exit(EXIT_FAILURE);
                }
            }

            int total = res.successful?(int)entry->count:BOOK_NOT_FOUND;

            message_t ack = {
                .type = MT_ACK_TB,
                .args = {b_id, b_cost, c_id, total},
            };

            Send(&ack, 1, message_datatype, source, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_STORE_BOOK: {
            bookid_t b_id = message.args[0];

            bookstorage_store(self.donated, (book_t){.id = b_id});

            bookstorage_node_t entry = bookstorage_find(self.donated, b_id);
            if(!entry){
                ERROR("Fatal error, no element in bookstorage for added entry");
                exit(EXIT_FAILURE);
            }

            if(self.node.id != self.leader){
                size_t count = entry->count;
                message_t update = {
                    .type = MT_UPD_DONATED,
                    .args = {b_id, count, self.node.id},
                };
                Send(&update, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);

                message_t ack_upd;
                Recv(&ack_upd, 1, message_datatype, self.leader, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if(ack_upd.type != MT_ACK_UD){
                    ERROR("Rogue '%s' received from leader, expected donation update ack", message_type_to_str(ack_upd.type));
                    exit(EXIT_FAILURE);
                }
            }else{
                // Update my own count since I'm the leader
                libdict_update_count(self.donated_locations, (book_t){.id = b_id}, self.node.id, entry->count);
            }

            message_t ack = {
                .type = MT_ACK_DB,
                .args = {-1, -1, -1},
            };
            
            Send(&ack, 1, message_datatype, source, 0, MPI_COMM_WORLD);

            break;
        }
        case MT_CHECK_NUM_BOOKS_LOAN: {
            if(self.leader==-1){
                ERROR("Leader election required before loaned books sanity check");
                exit(EXIT_FAILURE);
            }

            // If received from the coordinator, I (should be leader) will send it to the first library to start
            if(source == COORDINATOR_INDEX){
                if(self.node.id != self.leader){
                    ERROR("Unexpected '%s' received from coordinator (destined for leader)", message_type_to_str(message.type));
                    exit(EXIT_FAILURE);
                }

                total_loaned_books = 0;             // Reset state
                sllist_removeAll(book_loaned_acks); //

                // If the first library is not the leader we tell it to check num books
                if(self.node.id != FIRST_LIBRARY){
                    message_t start = {
                        .type = MT_CHECK_NUM_BOOKS_LOAN,
                        .args = {-1, -1, -1},
                    };
                    
                    
                    DEBUG("Sending '%s' to library '%d'", message_type_to_str(start.type), FIRST_LIBRARY);
                    Send(&start, 1, message_datatype, FIRST_LIBRARY, 0, MPI_COMM_WORLD);
                    break;
                }
            }
            // Forward to next in grid if such a node exists
            mpipid_t next_route = next_route_in_grid(self);
            if(next_route!=-1){
                message_t forward = {
                    .type = MT_CHECK_NUM_BOOKS_LOAN,
                    .args = {-1, -1, -1},
                };

                DEBUG("Sending '%s' to library '%d'", message_type_to_str(forward.type), next_route);
                Send(&forward, 1, message_datatype, next_route, 0, MPI_COMM_WORLD);
            }

            if(self.node.id != self.leader){
                // Respond to leader
                message_t books_loaned_msg = {
                    .type = MT_NUM_BOOKS_LOANED,
                    .args = {self.books_loaned, -1, -1},
                };

                DEBUG("Sending '%s' to library leader '%d'", message_type_to_str(books_loaned_msg.type), self.leader);
                Send(&books_loaned_msg, 1, message_datatype, self.leader, 0, MPI_COMM_WORLD);
                break;
            }else{
                // I'm the leader, fall back to num books loaned right away
                message.type = MT_DONATE_BOOK;
                message.args[0] = self.books_loaned;
                source = self.node.id;

                FALLTHROUGH;
            }
        }
        case MT_NUM_BOOKS_LOANED: {
            if(self.leader==-1){
                ERROR("Leader election required before loaned books sanity check");
                exit(EXIT_FAILURE);
            }

            if(self.node.id != self.leader){
                ERROR("Only leader is expected to receive '%s'", message_type_to_str(message.type));
                exit(EXIT_FAILURE);
            }

            if(sllist_search(book_loaned_acks, source)){
                ERROR("Duplicate '%s' received from '%d'", message_type_to_str(message.type), source);
                exit(EXIT_FAILURE);
            }

            sllist_add(book_loaned_acks, source);

            int new_loans = message.args[0];

            if(new_loans < 0){
                ERROR("Invalid loaned books received from '%d'", source);
                exit(EXIT_FAILURE);
            }

            total_loaned_books += new_loans;

            // If I've received a message from all libraries
            if(sllist_length(book_loaned_acks) == SQUARE(self.node.N)){
                INFO("Library Books: %d", total_loaned_books);

                message_t done = {
                    .type = MT_CHECK_NUM_BOOKS_LOAN_DONE,
                    .args = {total_loaned_books, -1, -1},
                };

                DEBUG("Sending '%s' to coordinator", message_type_to_str(done.type));
                Send(&done, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);
            }

            break;
        }
        case MT_TERMINATE: {
            DEBUG("Terminating...");
            return;
        }
        default:
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
                Send(&explore_msg, 1, message_datatype, proc_id, 0, MPI_COMM_WORLD);
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
                    Send(&parent_msg, 1, message_datatype, parent, 0, MPI_COMM_WORLD);
                }
                else
                {
                    // I'm root so send elect done to coordinator

                    if(!le_started)break; // Ignore (garbage because of out-of-suync parent or leader messages resulting 'explore')

                    size_t book_locations_len = CUBE(self.node.N); // N^3 instead of N^4

                    // Create book locations array if it doesn't exist (this should always hold unless misused)
                    if (!self.book_locations)
                    {
                        self.book_locations = safe_malloc(book_locations_len * sizeof(mpipid_t));

                        // Initialize entries
                        for (size_t b_id = 0; b_id < book_locations_len; ++b_id)
                        {
                            self.book_locations[b_id] = b_id / self.node.N + 1;
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

                    sllist_t pending_le_acks = sllist_create();

                    // Broadcast election result to all libraries except self
                    for (int id = 1; id <= self.node.N * self.node.N; ++id)
                    { // All library ids
                        if (id == self.node.id)
                            continue;

                        sllist_add(pending_le_acks, id);

                        Send(&elect_result, 1, message_datatype, id, 0, MPI_COMM_WORLD);
                        DEBUG("Sent '%s' to %d", message_type_to_str(elect_result.type), id);
                    }

                    le_started = FALSE;

                    // Ensure everyone knows the elected leader before finishing
                    while(!sllist_empty(pending_le_acks)){
                        message_t ack;
                        MPI_Status status;

                        Recv(&ack, 1, message_datatype, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                        int sender = status.MPI_SOURCE;

                        if(ack.type != MT_ACK_LE_LIBR)continue; // Not what I'm looking for

                        if(!sllist_search(pending_le_acks, sender)){
                            ERROR("Duplicate '%s' from '%d'", message_type_to_str(ack.type), sender);
                            exit(EXIT_FAILURE);
                        }

                        sllist_remove(pending_le_acks, sender);
                    }

                    cleanup_mpi_in_buffers(); // Clean up input buffer to avoid garbage

                    sllist_destroy(pending_le_acks); // Not needed anymore

                    message_t elect_done_msg = {
                        .type = MT_LE_LIBR_DONE,
                        .args = {self.node.id, -1, -1},
                    };

                    
                    Send(&elect_done_msg, 1, message_datatype, COORDINATOR_INDEX, 0, MPI_COMM_WORLD);
                }
            }
        }
    }

    sllist_destroy(unexplored); // Not needed anymore
    sllist_destroy(book_loaned_acks);
}
