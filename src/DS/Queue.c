#include "Queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

PCBQueue *queue_init(size_t capacity)
{
    PCBQueue *q = malloc(sizeof(PCBQueue));
    q->data = malloc(capacity * sizeof(PCB));
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = capacity;
    return q;
}


void queue_free(PCBQueue *q)
{
    free(q->data);
    q->data = NULL;
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = 0;
}

int queue_empty(PCBQueue *q)
{
    return q->size == 0;
}

int queue_full(PCBQueue *q)
{
    return q->size == q->capacity;
}

void queue_resize(PCBQueue *q, size_t new_capacity)
{
    PCB *new_data = (PCB *)malloc(new_capacity * sizeof(PCB));

    for (size_t i = 0; i < q->size; i++)
    {
        new_data[i] = q->data[(q->front + i) % q->capacity];
    }

    free(q->data);
    q->data = new_data;
    q->front = 0;
    q->rear = q->size;
    q->capacity = new_capacity;
}

void queue_enqueue(PCBQueue *q, PCB value)
{
    if (queue_full(q))
    {
        queue_resize(q, q->capacity * 2);
    }

    q->data[q->rear] = value;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;
    // queue_print(q); // Print the queue after enqueueing
}

PCB queue_front(PCBQueue *q)
{
    if (queue_empty(q))
    {
        PCB empty = {0}; // Initialize all fields to 0
        empty.pid = -1;  // Set an indicator value
        printf("Error: Attempt to access front of empty queue\n");
        return empty;
    }
    return q->data[q->front];
}

void queue_dequeue(PCBQueue *q)
{
    if (queue_empty(q))
    {
        printf("Error: Attempt to dequeue from empty queue\n");
        return;
    }

    q->front = (q->front + 1) % q->capacity;
    q->size--;
}

void queue_print(PCBQueue *q)
{
    printf("Queue contents (size: %zu):\n", q->size);
    for (size_t i = 0; i < q->size; i++)
    {
        size_t index = (q->front + i) % q->capacity;
        printf("Process ID: %d, PID: %d, Arrival: %d, Runtime: %d, Remaining: %d, Priority: %d\n",
               q->data[index].id_from_file, q->data[index].pid, q->data[index].arrival_time,
               q->data[index].execution_time,
               // Simply use the remaining_time as is, since it's an integer, not a pointer
               q->data[index].remaining_time,
               q->data[index].priority);
    }
}
