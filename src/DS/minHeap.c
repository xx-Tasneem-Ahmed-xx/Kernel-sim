#include "minHeap.h"
#include "../pcb.h"

// Helper functions for heap indices
static int parent(int i) { return (i - 1) / 2; }
static int left_child(int i) { return 2 * i + 1; }
static int right_child(int i) { return 2 * i + 2; }

// Swap two PCB structs
static void swap(PCB *a, PCB *b)
{
    PCB temp = *a;
    *a = *b;
    *b = temp;
}

// Heapify down to maintain min-heap property
static void heapify_down(MinHeap *heap, int i)
{
    int smallest = i;
    int left = left_child(i);
    int right = right_child(i);

    if (left < heap->size && heap->processes[left].remaining_time < heap->processes[smallest].remaining_time)
    {
        smallest = left;
    }
    if (right < heap->size && heap->processes[right].remaining_time < heap->processes[smallest].remaining_time)
    {
        smallest = right;
    }

    if (smallest != i)
    {
        swap(&heap->processes[i], &heap->processes[smallest]);
        heapify_down(heap, smallest);
    }
}

// Heapify up to maintain min-heap property
static void heapify_up(MinHeap *heap, int i)
{
    while (i > 0 && heap->processes[parent(i)].remaining_time > heap->processes[i].remaining_time)
    {
        swap(&heap->processes[i], &heap->processes[parent(i)]);
        i = parent(i);
    }
}

// Create a new min-heap
MinHeap *create_min_heap()
{
    MinHeap *heap = (MinHeap *)malloc(sizeof(MinHeap));
    if (!heap)
    {
        printf("Error: Memory allocation failed for MinHeap\n");
        exit(1);
    }
    heap->capacity = MAX_PROCESSES;
    heap->size = 0;
    heap->processes = (PCB *)malloc(heap->capacity * sizeof(PCB));
    if (!heap->processes)
    {
        printf("Error: Memory allocation failed for processes array\n");
        free(heap);
        exit(1);
    }
    return heap;
}

// Destroy the min-heap
void destroy_min_heap(MinHeap *heap)
{
    if (heap)
    {
        free(heap->processes);
        free(heap);
    }
}

// Insert a process into the heap
void insert_process_min_heap(MinHeap *heap, PCB process, int position)
{
    if (heap->size >= heap->capacity)
    {
        printf("Error: Heap is full\n");
        return;
    }
    process.pid = position;
    process.burst_time = process.remaining_time;
    process.execution_time = 0;
    process.waiting_time = 0;
    process.total_runtime = 0;
    process.state = WAITING;
    process.child_pid = -1;

    heap->processes[heap->size] = process;
    heap->size++;
    heapify_up(heap, heap->size - 1);
}

// Extract the process with minimum remaining_time
PCB *extract_min(MinHeap *heap)
{
    if (heap->size == 0)
    {
        return NULL;
    }
    if (heap->size == 1)
    {
        heap->size--;
        return &heap->processes[0];
    }

    PCB *min = &heap->processes[0];
    heap->processes[0] = heap->processes[heap->size - 1];
    heap->size--;
    heapify_down(heap, 0);

    return min;
}

// Update remaining_time for a process identified by child_pid
void update_remaining_time(MinHeap *heap, int child_pid, int new_time)
{
    for (int i = 0; i < heap->size; i++)
    {
        if (heap->processes[i].child_pid == child_pid)
        {
            heap->processes[i].remaining_time = new_time;
            heapify_up(heap, i);
            heapify_down(heap, i);
            break;
        }
    }
}

// Print the heap (for debugging)
void print_minheap(MinHeap *heap)
{
    printf("Min-Heap (Ready Queue):\n");
    for (int i = 0; i < heap->size; i++)
    {
        if (heap->processes[i].state != TERMINATED)
        {
            printf("Process %d: Arrival=%d, Remaining=%d, pid=%d State=%s\n",
                   heap->processes[i].pid,
                   heap->processes[i].arrival_time,
                   heap->processes[i].remaining_time,
                   heap->processes[i].pid,
                   heap->processes[i].state == RUNNING ? "RUNNING" : heap->processes[i].state == WAITING ? "WAITING"
                                                                                                         : "TERMINATED");
        }
    }
}

// Return pointer to processes array (for compatibility with prior code)
PCB *get_ready_queue()
{
    static MinHeap *heap = NULL;
    if (!heap)
    {
        heap = create_min_heap();
    }
    return heap->processes;
}