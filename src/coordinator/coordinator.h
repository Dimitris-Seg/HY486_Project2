/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file coordinator.h
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

#ifndef COORDINATOR_H
#define COORDINATOR_H

#include "../utils/parsing/tokenlist.h"
#include "../utils/parsing/parser.h"
//

// of is the input argument type, type is the target argument type, instr is the current instruction type
#define ENFORCE_ARG_TYPE(of, type, instr) \
    if((of) != (type))\
        ERROR("Unexpected argument to '%s', expected type '%s', got '%s'", tokennames[(instr)], tokennames[(type)], tokennames[(of)]),\
        exit(EXIT_FAILURE)

typedef struct coordinator coordinator_t;

struct coordinator {
    mpiproc_t node;
    mpiproc_t lib_leader;
    mpiproc_t loaner_leader;
    tokenlist_t instructions;
};

/**
 * Initializes the coordinator instance and loads the program from instr_file
 */
coordinator_t coordinator_init(mpipid_t id, int N, string instr_file);

/**
 * Executes the program (instructions) loaded in the coordinator
 */
void coordinator_run(coordinator_t* self);

#endif
