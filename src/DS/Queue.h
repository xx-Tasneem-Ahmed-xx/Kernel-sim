#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include <stddef.h>
#include "../pcb.h"  // Changed to include pcb.h

typedef struct {
    PCB* data;       // Changed from Process* to PCB*
    size_t front;
    size_t rear;       
    size_t size;        
    size_t capacity;    
} PCBQueue;          // Renamed from ProcessQueue to PCBQueue

PCBQueue* queue_init( size_t capacity);
void queue_free(PCBQueue* q);
int queue_empty(PCBQueue* q);
int queue_full(PCBQueue* q);
void queue_resize(PCBQueue* q, size_t new_capacity);
void queue_enqueue(PCBQueue* q, PCB value);  // Now takes PCB instead of Process
PCB queue_front(PCBQueue* q);               // Now returns PCB
void queue_dequeue(PCBQueue* q);
void queue_print(PCBQueue* q);

#endif