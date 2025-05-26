/**
 * Implementation of a distributed library-borrower/client system using mpi
 *
 *@author Dimitris Segkesser, A.M.: 5006
 *@date 25/05/2025
 *
 *@file logging.h
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

#ifndef LOGGING_H
#define LOGGING_H

#include "general.h"
//
#include <stdio.h>
#include <time.h>
//

extern bool debugger; // Regulates which processes are allowed to print (excluded: print and println)

#define ALL_DEBUG       0 // If true all processes will print debug logs
#define FAIL_ON_ERROR   0 // The process will automatically fail on the first error if true
#define DEBUGGING_ON    0 // Enables debug logs (DEBUG & ERROR)
#define WATERMARK_ON    1 // CSD watermark at the start
#define FLUSH_OUTPUT    0 // If true all debug (DEBUT & ERROR) output will be flushed

#ifdef ALL_DEBUG
    #define enable_debugger() (debugger = TRUE)
#else
    #define enable_debugger() ((void)0)
#endif

/**
 * These functions return a static buffer containing the
 * date and time respectively. This choice was made 
 * purely for the sake of simplicity and performance,
 * rendering them non thread-safe.
 */
string get_current_date();
string get_current_time();

#define CUSTOM_LOG(tag, msg, ...) if(debugger)printf("[%s]\t[%s %s] " msg "\n", tag, get_current_date(), get_current_time(), ##__VA_ARGS__)

#if FAIL_ON_ERROR
    #define ERROR(msg, ...) CUSTOM_LOG("ERROR", msg, ##__VA_ARGS__),exit(EXIT_FAILURE)
#elif FLUSH_OUTPUT
    #define ERROR(msg, ...) CUSTOM_LOG("ERROR", msg, ##__VA_ARGS__), fflush(stdout)
#else
    #define ERROR(msg, ...) CUSTOM_LOG("ERROR", msg, ##__VA_ARGS__)
#endif

#if DEBUGGING_ON
#if FLUSH_OUTPUT
    #define DEBUG(msg, ...) CUSTOM_LOG("DEBUG", msg, ##__VA_ARGS__), fflush(stdout)
#else
#define DEBUG(msg, ...) CUSTOM_LOG("DEBUG", msg, ##__VA_ARGS__)
#endif
#else
    #define DEBUG(MSG, ...)
#endif

#define INFO(msg, ...) CUSTOM_LOG("INFO", msg, ##__VA_ARGS__)

#define PRINT(msg, ...) printf(msg, ##__VA_ARGS__)
#define PRINTLN(msg, ...) PRINT(msg"\n", ##__VA_ARGS__)

#if WATERMARK_ON
#define PRINT_WATERMARK() if(debugger)printf( \
    "\n" \
    "     ===   =====   ===              \n" \
    "   //      ||      ||  \\\\         \n" \
    "   ||      =====   ||  ||           \n" \
    "   \\\\         ||   ||  //         \n" \
    "     ===   =====   ===              \n" \
    "\n" \
    "(2025) Computer Science Department, University Of Crete \n\n" \
    "Project 2, HY486\n" \
    "Build [%s %s] \n"                                           \
    "========================================================\n\n" \
, __DATE__, __TIME__)
#else
#define PRINT_WATERMARK()
#endif

#endif