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
void insert_process_min_heap(MinHeap *heap, PCB *process, int position)
{
    if (heap->size >= heap->capacity)
    {
        printf("Error: Heap is full\n");
        return;
    }
    
    PCB newProcess = *process;
    heap->processes[heap->size] = newProcess;
    int index = heap->size;
    heap->size++;
    heapify_up(heap, index);
}

// Extract the process with minimum remaining_time
PCB *extract_min(MinHeap *heap)
{
    if (heap->size == 0)
    {
        return NULL;
    }

    // Create a static PCB to return (avoids memory leaks)
    static PCB min_pcb;
    min_pcb = heap->processes[0];

    // Move the last element to the root and reduce heap size
    heap->processes[0] = heap->processes[heap->size - 1];
    heap->size--;

    // Heapify the root only if the heap isn't empty now
    if (heap->size > 0) {
        heapify_down(heap, 0);
    }

    return &min_pcb;
}

// Check if a process with given pid exists in the heap
int process_exists(MinHeap *heap, int pid)
{
    for (int i = 0; i < heap->size; i++)
    {
        if (heap->processes[i].pid == pid)
        {
            return 1; // Process exists
        }
    }
    return 0; // Process not found
}

// Update process in heap by pid
void update_process(MinHeap *heap, PCB *updated_process)
{
    for (int i = 0; i < heap->size; i++)
    {
        if (heap->processes[i].pid == updated_process->pid)
        {
            // Update the process
            heap->processes[i] = *updated_process;
            
            // Fix heap property
            heapify_up(heap, i);
            heapify_down(heap, i);
            return;
        }
    }
    
    // If not found, insert it
    if (!process_exists(heap, updated_process->pid)) {
        insert_process_min_heap(heap, updated_process, updated_process->pid);
    }
}

// Remove a process from the heap by pid
void remove_process(MinHeap *heap, int pid)
{
    // Find the process
    int i;
    for (i = 0; i < heap->size; i++)
    {
        if (heap->processes[i].pid == pid)
        {
            break;
        }
    }
    
    // If process found
    if (i < heap->size)
    {
        // Move the last element to this position
        heap->processes[i] = heap->processes[heap->size - 1];
        heap->size--;
        
        // Fix heap property
        heapify_up(heap, i);
        heapify_down(heap, i);
    }
}

// Update remaining_time for a process identified by child_pid
void update_remaining_time(MinHeap *heap, int pid, int new_time)
{
    for (int i = 0; i < heap->size; i++)
    {
        if (heap->processes[i].pid == pid)
        {
            heap->processes[i].remaining_time = new_time;
            
            // Fix heap property
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
