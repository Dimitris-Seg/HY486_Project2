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

extern bool debugger;       // Regulates which processes are allowed to print (excluded: print and println)
extern string process_name; // The current process name for debugging purposes
extern int process_id;      // The current process id for debugging purposes

#define ALL_DEBUG       1 // If true all processes can print debug logs
#define FAIL_ON_ERROR   0 // The process will automatically fail on the first error if true
#define FAIL_ON_WARN    0 // The process will automatically fail on the first warning if true
#define DEBUGGING_ON    1 // Enables debug logs (DEBUG & ERROR)
#define WATERMARK_ON    1 // CSD watermark at the start
#define FLUSH_OUTPUT    0 // If true all debug (DEBUT & ERROR) output will be flushed
#define COLORS          1 // Use terminal colors

#if ALL_DEBUG
    #define ENABLE_DEBUGGER() ((void)0)
#else
    #define ENABLE_DEBUGGER() (debugger = TRUE)
#endif

#if COLORS
#define TINT_RED        "\033[0;31m"    
#define TINT_GREEN      "\033[0;32m"  
#define TINT_YELLOW     "\033[0;33m"    
#define TINT_BLUE       "\033[0;34m"  
#define TINT_MAG        "\033[0;35m"    
#define TINT_CYAN       "\033[0;36m"  
#define TINT_NONE       "\033[0m"  
#define COLOR_RED       printf(TINT_RED)
#define COLOR_GREEN     printf(TINT_GREEN)
#define COLOR_YELLOW    printf(TINT_YELLOW)
#define COLOR_BLUE      printf(TINT_BLUE)
#define COLOR_MAG       printf(TINT_MAG)
#define COLOR_CYAN      printf(TINT_CYAN)
#define COLOR_RESET     printf(TINT_NONE)    
#else
#define TINT_RED        ""    
#define TINT_GREEN      ""  
#define TINT_YELLOW     ""  
#define TINT_BLUE       ""
#define TINT_MAG        ""  
#define TINT_CYAN       ""  
#define TINT_NONE       ""  
#define COLOR_RED       (void)(0)
#define COLOR_YELLOW    (void)(0)
#define COLOR_GREEN     (void)(0)
#define COLOR_BLUE      (void)(0)
#define COLOR_MAG       (void)(0)
#define COLOR_CYAN      (void)(0)
#define COLOR_RESET     (void)(0)
#endif

#define SET_PROCESS_NAME(name) process_name = (name)
#define SET_PROCESS_ID(id) process_id = (id)

/**
 * These functions return a static buffer containing the
 * date and time respectively. This choice was made 
 * purely for the sake of simplicity and performance,
 * rendering them non thread-safe.
 */
string get_current_date();
string get_current_time();

#define CUSTOM_LOG(tag, msg, ...) (debugger?printf("[%s]\t%s[%s %s] %s[%-12s:%2d]:%s " msg "%s\n", tag, TINT_MAG, get_current_date(), get_current_time(), TINT_BLUE, process_name, process_id, TINT_CYAN, ##__VA_ARGS__, TINT_NONE):(void)(0))

#if FAIL_ON_ERROR
    #define ERROR(msg, ...) COLOR_RED, CUSTOM_LOG("ERROR", msg, ##__VA_ARGS__), COLOR_RESET, exit(EXIT_FAILURE)
#elif FLUSH_OUTPUT
    #define ERROR(msg, ...) COLOR_RED, CUSTOM_LOG("ERROR", msg, ##__VA_ARGS__), COLOR_RESET, fflush(stdout)
#else
    #define ERROR(msg, ...) COLOR_RED, CUSTOM_LOG("ERROR", msg, ##__VA_ARGS__), COLOR_RESET
#endif

#if DEBUGGING_ON
#if FLUSH_OUTPUT
    #define DEBUG(msg, ...) CUSTOM_LOG(TINT_YELLOW"DEBUG"TINT_NONE, msg, ##__VA_ARGS__), fflush(stdout)
#else
#define DEBUG(msg, ...) CUSTOM_LOG(TINT_YELLOW"DEBUG"TINT_NONE, msg, ##__VA_ARGS__)
#endif
#else
    #define DEBUG(MSG, ...)
#endif

#define INFO(msg, ...) CUSTOM_LOG(TINT_GREEN"INFO"TINT_NONE, msg, ##__VA_ARGS__)


#if FAIL_ON_WARN
    #define WARN(msg, ...) COLOR_RED, (debugger?PRINTLN("[WARN] "msg, ##__VA_ARGS__):(void)(0)), COLOR_RESET, exit(EXIT_FAILURE)
#else
    #define WARN(msg, ...) COLOR_RED, (debugger?PRINTLN("[WARN] "msg, ##__VA_ARGS__):(void)(0)), COLOR_RESET
#endif

#define PRINT(msg, ...) printf(msg, ##__VA_ARGS__)
#define PRINTLN(msg, ...) PRINT(msg"\n", ##__VA_ARGS__)

#if WATERMARK_ON

#define PRINT_WATERMARK() if(debugger)printf( \
"\n"\
"    _____  _____ _____         \n"\
"   / ____|/ ____|  __ \\       \n"\
"  | |    | (___ | |  | |      \n"\
"  | |     \\___ \\| |  | |    \n"\
"  | |____ ____) | |__| |      \n"\
"   \\_____|_____/|_____/       \n"\
"\n" \
    "(2025) Computer Science Department, University Of Crete \n\n" \
    "Project 2, HY486\n" \
    "Build [%s %s] \n"                                           \
    "========================================================\n\n" \
, __DATE__, __TIME__)
#else
#define PRINT_WATERMARK()
#endif

#define PRINT_LOG_HELPER() if(debugger)\
    printf(TINT_GREEN"[TYPE] "TINT_MAG" [       DATE        ] "TINT_BLUE"[ PROCESS | ID ] "TINT_CYAN" [ MESSAGE ]\n"TINT_NONE),\
    printf("────────────────────────────────────────────────────────────\n")

#endif