/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file parser.c
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

#include "parser.h"

static token_t to_token(string buff){
    static const string matches[] = {
        "CONNECT", 
        "START_LE_LIBR", 
        "START_LE_LOANERS", 
        "TAKE_BOOK", 
        "DONATE_BOOK", 
        "GET_MOST_POPULAR_BOOK", 
        "CHECK_NUM_BOOKS_LOANED",
    };

    static const int matches_len = sizeof(matches)/sizeof(string);

    token_t token = {.val = -1};

    bool matched = FALSE;
    for(int i = 0;i < matches_len; ++i){
        if(!strcmp(buff, matches[i])){
            matched = TRUE;
            token.type = (token_type_t)i;
            break;
        }
    }
    
    if(!matched){
        int int_trial = atoi(buff);

        if(int_trial<=0){
            ERROR("Unexpected token '%s' in instruction file", buff);
            exit(EXIT_FAILURE);
        }

        token.type = TT_INT;
        token.val = int_trial + 1;
    }

    return token;
}

tokenlist_t read_instructions(string filename){
    char buff[MAX_INSTR_LEN+1];
    size_t buff_index = 0;

    FILE *file = fopen(filename, "r");

    if(!file){
        ERROR("Could not open file %s for reading: %s", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    tokenlist_t tokenlist = tokenlist_create();
    
    int c;

    while((c = fgetc(file))!=EOF){
        if(c==SEPARATOR || c==LINEBREAK){
            if(buff_index){
                buff[buff_index] = '\0';
                tokenlist_append(tokenlist, to_token(buff));
            }

            buff_index = 0;
            continue;
        }

        if(buff_index == MAX_INSTR_LEN){
            ERROR("Unexpectedly long instruction in instruction file");
            exit(EXIT_FAILURE);
        }

        buff[buff_index++] = c;
    }

    // Flush remaining characters (if any)
    if (buff_index > 0) {
        buff[buff_index] = '\0';
        tokenlist_append(tokenlist, to_token(buff));
    }

    fclose(file);

    return tokenlist;
}