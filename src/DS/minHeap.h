#ifndef MINHEAP_H
#define MINHEAP_H

#include <stdio.h>
#include <stdlib.h>
#include "../pcb.h"

#define MAX_PROCESSES 100

// Min-Heap struct
typedef struct
{
    PCB *processes; // Dynamic array of PCB structs
    int size;       // Current number of processes
    int capacity;   // Maximum capacity (MAX_PROCESSES)
} MinHeap;

// Function prototypes
MinHeap *create_min_heap();
void destroy_min_heap(MinHeap *heap);
void insert_process_min_heap(MinHeap *heap, PCB* process,int position);
PCB *extract_min(MinHeap *heap);
void update_remaining_time(MinHeap *heap, int child_pid, int new_time);
void print_minheap(MinHeap *heap);
PCB *get_ready_queue(); // Returns pointer to processes array (for compatibility)

#endif