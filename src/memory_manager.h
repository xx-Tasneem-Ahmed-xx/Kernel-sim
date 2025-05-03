#ifndef MEMEORY_MANAGER
#define MEMEORY_MANAGER

#include "headers.h"

typedef struct MemoryBlock
{
    int size;
    int allocated;
    struct MemoryBlock *parent;
    struct MemoryBlock *left_child;
    struct MemoryBlock *right_child;
    pid_t process_pid;
    int id_from_file;
} MemoryBlock;

extern MemoryBlock *mp[10000];
extern MemoryBlock *Memory_Segment; // root node of the memory segment
extern const int TOTAL_MEMORY_SIZE;
extern int allocated_memory;
extern int free_memory;

void initialize_memory_Segment();
MemoryBlock *initialize_memory_Block(int size);
bool is_Memory_Available(PCB *process);
MemoryBlock *traverse_MemorySegment(MemoryBlock *root, int needed_memory);
int highestPowerOf2(int x);
bool allocate_memory(pid_t pid,int size);
bool try_allocate_memory(pid_t pid,int size);
bool deallocate_memory(pid_t pid);
void update_id(int pid_from_file, pid_t pid);
MemoryBlock *get_Process_Memory_Segment(MemoryBlock *root, int pid);
MemoryBlock *try_get_Process_Memory_Segment(MemoryBlock *root, int id_from_file);
void printMemorySegment(MemoryBlock *root);

// TODO FREE THE MEMORY_SEGMENT

#endif
