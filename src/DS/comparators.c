#include "../pcb.h"
#include "../headers.h"
// Compare by Remaining Time (for SRTN)
int compare_remaining_time(PCB *a, PCB *b)
{
    // Lower remaining time = higher priority
    // Direct comparison since remaining_time is an integer, not a pointer
    return a->remaining_time - b->remaining_time;
}

// Compare by Priority (lower value = higher priority)
int compare_priority(PCB *a, PCB *b) {
    // Lower priority value -> higher actual priority
    return a->priority - b->priority;
}

// Compare by Arrival Time (for FCFS)
int compare_arrival_time(PCB *a, PCB *b) {
    return a->arrival_time - b->arrival_time;
}