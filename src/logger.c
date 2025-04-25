#include "headers.h"

// Define the global variable
LogLevel current_log_level = LOG_INFO;

// Logging function with level and colors
void log_message(LogLevel level, const char* format, ...) {
    // Skip logging if level is below current_log_level
    if (level < current_log_level) return;
    
    va_list args;
    va_start(args, format);
    
    // Select color based on log level
    switch(level) {
        case LOG_DEBUG:
            printf("%s[DEBUG]%s ", COLOR_WHITE, COLOR_RESET);
            break;
        case LOG_INFO:
            printf("%s[INFO]%s ", COLOR_BLUE, COLOR_RESET);
            break;
        case LOG_PROCESS:
            printf("%s[PROCESS]%s ", COLOR_GREEN, COLOR_RESET);
            break;
        case LOG_SYSTEM:
            printf("%s[SYSTEM]%s ", COLOR_MAGENTA, COLOR_RESET);
            break;
        case LOG_STAT:
            printf("%s[STATS]%s ", COLOR_CYAN, COLOR_RESET);
            break;
        case LOG_ERROR:
            printf("%s[ERROR]%s ", COLOR_RED, COLOR_RESET);
            break;
    }
    
    // Print the actual message
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
    fflush(stdout);
}

// Helper function for process state changes
void log_process_state(PCB* process, const char* state) {
    if (process == NULL) return;
    
    log_message(LOG_PROCESS, "Process %d %s%s%s [Priority: %d, Remaining: %d, Wait: %d]",
               process->id_from_file, 
               BOLD, state, COLOR_RESET,
               process->priority,
               process->remaining_time,
               process->waiting_time);
}

// Print a divider line for sections
void print_divider(const char* title) {
    printf("\n%s%s====== %s ======%s\n", BOLD, COLOR_YELLOW, title, COLOR_RESET);
}

// Print a simple progress bar
void print_progress_bar(int completed, int total, int width) {
    int filled = (completed * width) / total;
    printf("[");
    for (int i = 0; i < width; i++) {
        if (i < filled) printf("■");
        else printf("□");
    }
    printf("] %d/%d\n", completed, total);
}
