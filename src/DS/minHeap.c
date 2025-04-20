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

// Swap two PCB structs
static void swap(PCB *a, PCB *b) {
    PCB temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify down to maintain heap property
static void heapify_down(int i) {
    int smallest = i;
    int left = left_child(i);
    int right = right_child(i);

    if (left < ready_Heap->size && ready_Heap->cmp(&ready_Heap->processes[left], &ready_Heap->processes[smallest]) <
        0) {
        smallest = left;
    }
    if (right < ready_Heap->size && ready_Heap->cmp(&ready_Heap->processes[right], &ready_Heap->processes[smallest]) <
        0) {
        smallest = right;
    }

    if (smallest != i) {
        swap(&ready_Heap->processes[i], &ready_Heap->processes[smallest]);
        heapify_down(smallest);
    }
}

// Heapify up to maintain heap property
static void heapify_up(int i) {
    while (i > 0 && ready_Heap->cmp(&ready_Heap->processes[i], &ready_Heap->processes[parent(i)]) < 0) {
        swap(&ready_Heap->processes[i], &ready_Heap->processes[parent(i)]);
        i = parent(i);
    }
}

// Create a new min-heap with a specific comparator
MinHeap *create_min_heap(Comparator cmp) {
    MinHeap *heap = (MinHeap *) malloc(sizeof(MinHeap));
    if (!heap) {
        printf("Error: Memory allocation failed for MinHeap\n");
        exit(1);
    }
    heap->capacity = MAX_PROCESSES;
    heap->size = 0;
    heap->processes = (PCB *) malloc(heap->capacity * sizeof(PCB));
    if (!heap->processes) {
        printf("Error: Memory allocation failed for processes array\n");
        free(heap);
        exit(1);
    }
    heap->cmp = cmp;
    return heap;
}

// Destroy the min-heap
void destroy_min_heap() {
    if (ready_Heap) {
        free(ready_Heap->processes);
        free(ready_Heap);
    }
}

// Insert a process into the heap
void insert_process_min_heap(PCB *process) {
    if (ready_Heap->size >= ready_Heap->capacity) {
        printf("Error: Heap is full\n");
        return;
    }

    PCB newProcess = *process;
    ready_Heap->processes[ready_Heap->size] = newProcess;
    int index = ready_Heap->size;
    ready_Heap->size++;
    heapify_up(index);
}

// Extract the process with minimum priority (based on comparator)
PCB *extract_min() {
    if (ready_Heap->size == 0) {
        return NULL;
    }

    // Store the minimum value
    PCB min = ready_Heap->processes[0];

    // Move last element to root and shrink heap
    ready_Heap->processes[0] = ready_Heap->processes[ready_Heap->size - 1];
    ready_Heap->size--;

    // Heapify the root
    heapify_down(0);

    // Return a copy of the minimum element
    static PCB min_pcb;
    min_pcb = min;
    return &min_pcb;
}

// Update remaining_time (or other fields depending on comparator) for a process identified by child_pid
void update_remaining_time(int child_pid, int new_time) {
    for (int i = 0; i < ready_Heap->size; i++) {
        if (ready_Heap->processes[i].child_pid == child_pid) {
            ready_Heap->processes[i].remaining_time = new_time;
            heapify_up(i);
            heapify_down(i);
            break;
        }
    }
}

// Print the heap (for debugging)
void print_minheap() {
    printf("Min-Heap (Ready Queue):\n");
    for (int i = 0; i < ready_Heap->size; i++) {
        if (ready_Heap->processes[i].state != TERMINATED) {
            printf("Process %d: Arrival=%d, Remaining=%d, PID=%d, State=%s, Priority=%d\n",
                   ready_Heap->processes[i].id_from_file,
                   ready_Heap->processes[i].arrival_time,
                   ready_Heap->processes[i].remaining_time,
                   ready_Heap->processes[i].id_from_file,
                   ProcessStateNames[ready_Heap->processes[i].state],
                   ready_Heap->processes[i].priority);
        }
    }
}
