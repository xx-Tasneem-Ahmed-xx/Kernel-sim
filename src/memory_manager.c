#include "memory_manager.h"

const int TOTAL_MEMORY_SIZE = 1024;
int allocated_memory = 0;
int free_memory = 0;
MemoryBlock *Memory_Segment = NULL;

void initialize_memory_Segment() {
    Memory_Segment = (MemoryBlock *) malloc(sizeof(MemoryBlock));
    if (Memory_Segment == NULL) {
        printf("Memory segment allocation failed\n");
        return;
    }
    Memory_Segment->size = TOTAL_MEMORY_SIZE;
    Memory_Segment->parent = NULL;
    Memory_Segment->left_child = NULL;
    Memory_Segment->right_child = NULL;
    Memory_Segment->process = NULL;
    Memory_Segment->allocated = 0;
}

MemoryBlock *initialize_memory_Block(int size) {
    MemoryBlock *block = (MemoryBlock *) malloc(sizeof(MemoryBlock));
    if (block == NULL) {
        printf("Memory block allocation failed\n");
        return NULL;
    }
    block->size = size;
    block->left_child = NULL;
    block->right_child = NULL;
    block->parent = NULL;
    block->process = NULL;
    block->allocated = 0;
    return block;
}

int highestPowerOf2(int x) {
    if (x <= 0) return 1;
    int power = 1;
    while (power < x) power *= 2;
    return power;
}

MemoryBlock *traverse_MemorySegment(MemoryBlock *root, int needed_memory) {
    if (root == NULL || root->process != NULL || root->size < needed_memory)
        return NULL;

    if (root->size == needed_memory && root->process == NULL) {
        return root;
    }

    // Recursively try left child
    if (root->left_child == NULL) {
        root->left_child = initialize_memory_Block(root->size / 2);
        root->left_child->parent = root;
    }
    MemoryBlock *left_result = traverse_MemorySegment(root->left_child, needed_memory);
    if (left_result != NULL) return left_result;

    // Recursively try right child
    if (root->right_child == NULL) {
        root->right_child = initialize_memory_Block(root->size / 2);
        root->right_child->parent = root;
    }
    return traverse_MemorySegment(root->right_child, needed_memory);
}

bool allocate_memory(PCB *process) {
    const int memory_needed = highestPowerOf2(process->memory_size);

    if (Memory_Segment == NULL) {
        initialize_memory_Segment();
    }

    MemoryBlock *new_block = traverse_MemorySegment(Memory_Segment, memory_needed);
    if (new_block != NULL) {
        new_block->process = process;
        new_block->allocated = process->memory_size;
        log_message(LOG_INFO, "Memory segment allocated successfully MEMORY allocated=%d memory needed=%d\n",
                    new_block->allocated, new_block->size);
        return true;
    }
    log_message(LOG_ERROR, "Memory segment cant allocate memory needed=%d\n",
                process->memory_size);
    return false;
}

MemoryBlock *get_Process_Memory_Segment(MemoryBlock *root, int pid) {
    if (root == NULL)
        return NULL;

    if (root->process && root->process->pid == pid)
        return root;

    MemoryBlock *left_result = get_Process_Memory_Segment(root->left_child, pid);
    if (left_result) return left_result;

    return get_Process_Memory_Segment(root->right_child, pid);
}

bool deallocate_memory(PCB *process) {
    MemoryBlock *to_delete = get_Process_Memory_Segment(Memory_Segment, process->pid);

    if (to_delete == NULL)
        return false;

    to_delete->process = NULL;
    free(to_delete);
    return true;
}


void log_memory_event(int time, bool allocate, int bytes, int process_id, int start, int end) {
    FILE *mem_file = fopen("memory.log", "a");
    if (!mem_file) {
        perror("Failed to open memory.log");
        return;
    }
    if (allocate) {
        fprintf(mem_file, "At time %d allocated %d bytes for process %d from %d to %d\n",
                time, bytes, process_id, start, end);
    } else {
        fprintf(mem_file, "At time %d freed %d bytes from process %d from %d to %d\n",
                time, bytes, process_id, start, end);
    }
    fclose(mem_file);
}