#ifndef MEMEORY_MANAGER
#define MEMEORY_MANAGER

#include "headers.h"

typedef struct MemoryBlock {
    int size;
    int allocated;
    struct MemoryBlock *parent;
    struct MemoryBlock *left_child;
    struct MemoryBlock *right_child;
    pid_t process_pid;
    int id_from_file;
} MemoryBlock;

void initialize_memory_Segment(MemoryBlock **Memory_Segment, const int TOTAL_MEMORY_SIZE);

MemoryBlock *initialize_memory_Block(const int size);

int get_used_space(const MemoryBlock *root);

MemoryBlock *traverse_MemorySegment(MemoryBlock *root, const int needed_memory);

int highestPowerOf2(const int x);

bool allocate_memory(MemoryBlock *root, const pid_t process_pid, const int process_size,int time);

bool deallocate_memory(MemoryBlock *root, const pid_t process_pid,int time);

void merge_buddy_blocks(MemoryBlock *block);

MemoryBlock *get_Process_Memory_Segment(MemoryBlock *root, const int id_from_file, const int pid);

void update_id(const int pid_from_file, const pid_t pid, MemoryBlock *root);

void print_memory_segment(const MemoryBlock *root, const int level);

void print_memory(const MemoryBlock *root);

void destroy_memory_segment(MemoryBlock *root);

bool is_in_subtree(MemoryBlock *root, MemoryBlock *node);

void log_memory_event(int time, bool allocate, int bytes, int process_id, int start, int end);

#endif
