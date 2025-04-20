#include "priorityQueue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void pq_init(PriorityQueue* pq, size_t capacity, SortMode mode) {
    pq->data = (Process*)malloc(capacity * sizeof(Process));
    pq->size = 0;
    pq->capacity = capacity;
    pq->sort_mode = mode; 
}

void pq_free(PriorityQueue* pq) {
    free(pq->data);
    pq->data = NULL;
    pq->size = 0;
    pq->capacity = 0;
}

int pq_empty(PriorityQueue* pq) {
    return pq->size == 0;
}

void pq_resize(PriorityQueue* pq, size_t new_capacity) {
    Process* new_data = (Process*)malloc(new_capacity * sizeof(Process));
    memcpy(new_data, pq->data, pq->size * sizeof(Process));
    free(pq->data);
    pq->data = new_data;
    pq->capacity = new_capacity;
}

int compare_processes(Process* a, Process* b, SortMode mode) {
    if (mode == SORT_BY_ARRIVAL_TIME) {
        return a->arrival_time - b->arrival_time;
    } else {
        if (a->arrival_time != b->arrival_time)
            return a->arrival_time - b->arrival_time;
        else
            return a->priority - b->priority;
    }
}

void pq_push(PriorityQueue* pq, Process value) {
    if (pq->size >= pq->capacity) {
        pq_resize(pq, pq->capacity * 2);
    }
    
    size_t i = pq->size++;
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        
        if (compare_processes(&pq->data[parent], &value, pq->sort_mode) <= 0)
            break;
            
        pq->data[i] = pq->data[parent];
        i = parent;
    }
    pq->data[i] = value;
}

Process pq_top(PriorityQueue* pq) {
    return pq->data[0];
}

void pq_pop(PriorityQueue* pq) {
    pq->data[0] = pq->data[--pq->size];
    
    size_t i = 0;
    while (1) {
        size_t smallest = i;
        size_t left = 2 * i + 1;
        size_t right = 2 * i + 2;
        
        if (left < pq->size && 
            compare_processes(&pq->data[left], &pq->data[smallest], pq->sort_mode) < 0)
            smallest = left;
            
        if (right < pq->size && 
            compare_processes(&pq->data[right], &pq->data[smallest], pq->sort_mode) < 0)
            smallest = right;
            
        if (smallest == i)
            break;
            
        Process temp = pq->data[i];
        pq->data[i] = pq->data[smallest];
        pq->data[smallest] = temp;
        
        i = smallest;
    }
}

void pq_print(PriorityQueue* pq) {
    printf("Queue contents (size: %zu):\n", pq->size);
    for (size_t i = 0; i < pq->size; i++) {
        printf("Process ID: %d, Arrival Time: %d, Run Time: %d, Priority: %d\n",
               pq->data[i].id, pq->data[i].arrival_time, pq->data[i].execution_time, pq->data[i].priority);
    }
    printf("Sort mode: %s\n", 
           pq->sort_mode == SORT_BY_ARRIVAL_TIME ? "Arrival Time Only" : "Arrival Time + Priority");
}