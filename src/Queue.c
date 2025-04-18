#include "Queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void queue_init(ProcessQueue* q, size_t capacity) {
    q->data = (Process*)malloc(capacity * sizeof(Process));
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = capacity;
}

void queue_free(ProcessQueue* q) {
    free(q->data);
    q->data = NULL;
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = 0;
}

int queue_empty(ProcessQueue* q) {
    return q->size == 0;
}

int queue_full(ProcessQueue* q) {
    return q->size == q->capacity;
}

void queue_resize(ProcessQueue* q, size_t new_capacity) {
    Process* new_data = (Process*)malloc(new_capacity * sizeof(Process));
    
    for (size_t i = 0; i < q->size; i++) {
        new_data[i] = q->data[(q->front + i) % q->capacity];
    }
    
    free(q->data);
    q->data = new_data;
    q->front = 0;
    q->rear = q->size;
    q->capacity = new_capacity;
}

void queue_enqueue(ProcessQueue* q, Process value) {
    if (queue_full(q)) {
        queue_resize(q, q->capacity * 2);
    }
    
    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
}

Process queue_front(ProcessQueue* q) {
    if (queue_empty(q)) {
        Process empty = {-1, -1, -1, -1};
        printf("Error: Attempt to access front of empty queue\n");
        return empty;
    }
    return q->data[q->front];
}

void queue_dequeue(ProcessQueue* q) {
    if (queue_empty(q)) {
        printf("Error: Attempt to dequeue from empty queue\n");
        return;
    }
    
    q->front = (q->front + 1) % q->capacity;
    q->size--;
}

void queue_print(ProcessQueue* q) {
    printf("Queue contents (size: %zu):\n", q->size);
    for (size_t i = 0; i < q->size; i++) {
        size_t index = (q->front + i) % q->capacity;
        printf("Process ID: %d, Arrival Time: %d, Run Time: %d, Priority: %d\n",
               q->data[index].id, q->data[index].arrival_time, 
               q->data[index].run_time, q->data[index].priority);
    }
}
