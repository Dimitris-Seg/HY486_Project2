/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file parser.h
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

#ifndef PARSER_H
#define PARSER_H

#include "tokenlist.h"
#include "../general.h"
#include "../logging.h"
//
#include <stdio.h>
#include <errno.h>
#include <string.h>
//

#define MAX_INSTR_LEN 64

#define SEPARATOR ' '
#define LINEBREAK '\n'

/**
 * Parses all instructions in the file and inserts them as tokens in the returned list
 */
tokenlist_t read_instructions(string filename);

#endif