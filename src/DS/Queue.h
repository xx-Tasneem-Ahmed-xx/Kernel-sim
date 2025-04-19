#ifndef PROCESS_QUEUE_H
#define PROCESS_QUEUE_H

#include <stddef.h>
#include "priorityQueue.h"  



typedef struct {
    Process* data;
    size_t front;
    size_t rear;       
    size_t size;        
    size_t capacity;    
} ProcessQueue;

void queue_init(ProcessQueue* q, size_t capacity);
void queue_free(ProcessQueue* q);
int queue_empty(ProcessQueue* q);
int queue_full(ProcessQueue* q);
void queue_resize(ProcessQueue* q, size_t new_capacity);
void queue_enqueue(ProcessQueue* q, Process value);
Process queue_front(ProcessQueue* q);
void queue_dequeue(ProcessQueue* q);
void queue_print(ProcessQueue* q);

#endif