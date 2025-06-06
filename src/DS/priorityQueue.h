#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stddef.h>
// todo move to headers
struct process {
    int id;
    int arrival_time;
    int execution_time;
    int priority;
    int memory_size;
    // RR-specific fields
    int os_pid;           // PID after fork
    int remaining_time;   // Remaining runtime
    int started;          // Flag: 0 = not started, 1 = resumed
    int start_time;       // Time when process first started
};
typedef struct process Process;

typedef enum {
    SORT_BY_ARRIVAL_TIME = 0,             // Option 1: Sort only by arrival time
    SORT_BY_ARRIVAL_THEN_PRIORITY = 1     // Option 2: Sort by arrival time, then by priority
} SortMode;

typedef struct {
    Process* data;
    size_t size;
    size_t capacity;
    SortMode sort_mode;  
} PriorityQueue;

void pq_init(PriorityQueue* pq, size_t capacity, SortMode mode);
void pq_free(PriorityQueue* pq);
int pq_empty(PriorityQueue* pq);
void pq_push(PriorityQueue* pq, Process value);
Process pq_top(PriorityQueue* pq);
void pq_pop(PriorityQueue* pq);
void pq_resize(PriorityQueue* pq, size_t new_capacity); 
void pq_print(PriorityQueue* pq);

#endif