#ifndef PCB_H
#define PCB_H

typedef enum
{
    WAITING,
    READY,
    RUNNING,
    STOPPED,
    TERMINATED
} ProcessState;

typedef struct PCB
{
    int pid;          // OS PID
    int id_from_file; // ID from input file
    int arrival_time;
    int start_time;
    int remaining_time;
    int finish_time;
    int priority;
    int waiting_time;
    int execution_time;
    int child_pid;

    ProcessState state;
} PCB;

typedef struct Node
{
    PCB process;
    struct Node *next;
} Node;

// Extern the head of the list
extern Node *ready_Queue;
extern int process_Count;
extern const char* ProcessStateNames[];
// Function prototypes
// void insert_process(PCB new_process, int algorithm);
// void remove_process(int id_from_file);
// PCB *pick_next_process();
// void print_ready_queue();

#endif
