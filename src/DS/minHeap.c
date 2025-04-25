#include "minHeap.h"
#include "../pcb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MinHeap *ready_Heap = NULL;

// Helper functions for heap indices
static int parent(int i) { return (i - 1) / 2; }
static int left_child(int i) { return 2 * i + 1; }
static int right_child(int i) { return 2 * i + 2; }

// Swap two PCB pointers
static void swap(PCB **a, PCB **b)
{
    PCB *temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify down to maintain heap property
void heapify_down(int i)
{
    int smallest = i;
    int left = left_child(i);
    int right = right_child(i);

    if (left < ready_Heap->size && ready_Heap->cmp(ready_Heap->processes[left], ready_Heap->processes[smallest]) < 0)
    {
        smallest = left;
    }
    if (right < ready_Heap->size && ready_Heap->cmp(ready_Heap->processes[right], ready_Heap->processes[smallest]) < 0)
    {
        smallest = right;
    }

    if (smallest != i)
    {
        swap(&ready_Heap->processes[i], &ready_Heap->processes[smallest]);
        heapify_down(smallest);
    }
}

// Heapify up to maintain heap property
void heapify_up(int i)
{
    while (i > 0 && ready_Heap->cmp(ready_Heap->processes[i], ready_Heap->processes[parent(i)]) < 0)
    {
        swap(&ready_Heap->processes[i], &ready_Heap->processes[parent(i)]);
        i = parent(i);
    }
}

// Create a new min-heap with a specific comparator
MinHeap *create_min_heap(Comparator cmp)
{
    MinHeap *heap = (MinHeap *)malloc(sizeof(MinHeap));
    if (!heap)
    {
        printf("Error: Memory allocation failed for MinHeap\n");
        exit(1);
    }
    heap->capacity = MAX_PROCESSES;
    heap->size = 0;
    heap->processes = (PCB **)malloc(heap->capacity * sizeof(PCB *));
    if (!heap->processes)
    {
        printf("Error: Memory allocation failed for processes processes\n");
        free(heap);
        exit(1);
    }
    heap->cmp = cmp;
    return heap;
}

// Destroy the min-heap
void destroy_min_heap()
{
    if (ready_Heap)
    {
        // Free all PCB objects stored in the heap
        for (int i = 0; i < ready_Heap->size; i++) {
            if (ready_Heap->processes[i]) {
                // remaining_time is an integer, not a pointer, so don't free it
                free(ready_Heap->processes[i]);
            }
        }
        free(ready_Heap->processes);
        free(ready_Heap);
    }
}

// Insert a process into the heap
void insert_process_min_heap(PCB *process)
{
    if (ready_Heap->size >= ready_Heap->capacity)
    {
        printf("Error: Heap is full\n");
        return;
    }

    // Store the pointer directly
    ready_Heap->processes[ready_Heap->size] = process;
    int index = ready_Heap->size;
    ready_Heap->size++;
    heapify_up(index);
    // printf("Inserted process with pid=%d \n", process->pid);
}

// Extract the minimum element from the heap
PCB *extract_min() {
    if (ready_Heap->size == 0) {
        return NULL;
    }

    PCB *min = ready_Heap->processes[0];

    // Move the last element to the root
    ready_Heap->processes[0] = ready_Heap->processes[ready_Heap->size - 1];
    ready_Heap->size--;

    if (ready_Heap->size > 0) {
        heapify_down(0);
    }

    // printf("Extracted process with pid=%d \n", min->pid);
    return min;
}

// Update remaining_time for a process identified by pid
void update_process_in_heap(pid_t pid)
{
    for (int i = 0; i < ready_Heap->size; i++)
    {
        if (ready_Heap->processes[i]->pid == pid)
        {
            // The value has already been updated in the PCB
            // Just fix the heap ordering
            heapify_up(i);
            heapify_down(i);
            break;
        }
    }
}

// Find a process in the heap by its pid
PCB* find_process_in_heap(pid_t pid)
{
    for (int i = 0; i < ready_Heap->size; i++)
    {
        if (ready_Heap->processes[i]->pid == pid)
        {
            return ready_Heap->processes[i];
        }
    }
    return NULL;
}

// Print the heap (for debugging)
void print_minheap()
{
    printf("Min-Heap (Ready Queue), Size: %d\n", ready_Heap->size);
    for (int i = 0; i < ready_Heap->size; i++)
    {
        printf("Process %d: Arrival=%d, Remaining=%d, PID=%d, State=%d, Priority=%d,sh_id=%d\n",
               ready_Heap->processes[i]->id_from_file,
               ready_Heap->processes[i]->arrival_time,
               ready_Heap->processes[i]->remaining_time, // Using directly as an integer
               ready_Heap->processes[i]->pid,
               ready_Heap->processes[i]->state,
               ready_Heap->processes[i]->priority,
               ready_Heap->processes[i]->shm_id
            );

    }
}