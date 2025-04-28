#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdarg.h>  // Added for va_start and va_end

// data structure files
#include "DS/priorityQueue.h"
#include "DS/minHeap.h"
#include "DS/Queue.h"
#include "DS/comparators.h"
#include "semaphores.h"  // Fixed: changed from semaphore.h to semaphores.h

// header files
#include "clk.h"
#include "pcb.h"
#include "process.h"

typedef enum { HPF, SRTN, RR } SchedulingAlgorithm;

typedef struct {
    long mtype;
    PCB pcb;
} MsgBuffer;

// ANSI color codes
#define COLOR_RESET   "\x1B[0m"
#define COLOR_RED     "\x1B[31m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_BLUE    "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN    "\x1B[36m"
#define COLOR_WHITE   "\x1B[37m"
#define BOLD          "\x1B[1m"

typedef enum {
    LOG_DEBUG,   // Detailed debug info
    LOG_INFO,    // General information
    LOG_PROCESS, // Process-related events
    LOG_SYSTEM,  // System events
    LOG_STAT,    // Statistics
    LOG_ERROR    // Error messages
} LogLevel;


// Only show messages at or above this level
extern LogLevel current_log_level;

// Logging function with level and colors
void log_message(LogLevel level, const char* format, ...);

// Helper function for process state changes
void log_process_state(PCB* process, const char* state);

// Print a divider line for sections
void print_divider(const char* title);

// Print a simple progress bar
void print_progress_bar(int completed, int total, int width);

#endif  // HEADERS_H