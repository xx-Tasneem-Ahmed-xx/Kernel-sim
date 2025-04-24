#include "../pcb.h"
#include "../headers.h"
// Compare by Remaining Time (for SRTN)
int compare_remaining_time(PCB *a, PCB *b)
{
    // Lower remaining time = higher priority
    if (a->remaining_time == NULL || b->remaining_time == NULL) {
        // Handle null pointers safely
        if (a->remaining_time == NULL && b->remaining_time == NULL)
            return 0;
        if (a->remaining_time == NULL)
            return 1;  // b comes first
        return -1;  // a comes first
    }
    return *(a->remaining_time) - *(b->remaining_time);
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
