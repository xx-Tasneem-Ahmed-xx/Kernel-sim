#include "../pcb.h"

// Compare by Remaining Time (for SRTN)
int compare_remaining_time(PCB *a, PCB *b) {
    return a->remaining_time - b->remaining_time;
}

// Compare by Priority (lower value = higher priority)
int compare_priority(const void *a, const void *b) {
    const PCB *pcb1 = (const PCB *) a;
    const PCB *pcb2 = (const PCB *) b;

    // Lower priority value -> higher actual priority
    return pcb1->priority - pcb2->priority;
}

// Compare by Arrival Time (for FCFS)
int compare_arrival_time(PCB *a, PCB *b) {
    return a->arrival_time - b->arrival_time;
}
