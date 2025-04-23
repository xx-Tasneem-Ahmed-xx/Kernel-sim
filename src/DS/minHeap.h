#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include "../pcb.h"
#include "../headers.h"

#define MAX_PROCESSES 1024

typedef int (*Comparator)(PCB *, PCB *); // Comparator function pointer type

typedef struct
{
    int capacity;
    int size;
    PCB **processes;
    Comparator cmp;
} MinHeap;

extern MinHeap *ready_Heap;

// Function declarations
MinHeap *create_min_heap(Comparator cmp);

void heapify_down(int i);
void heapify_up(int i);
static void swap(PCB *a, PCB *b);
void destroy_min_heap();

void insert_process_min_heap(PCB *process);

PCB *extract_min();

void update_remaining_time(int child_pid, int new_time);

void print_minheap();

#endif // MIN_HEAP_H
