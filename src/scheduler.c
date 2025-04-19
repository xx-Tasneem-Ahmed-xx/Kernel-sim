#include "scheduler.h"

#include <sys/msg.h>

#include "headers.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include "DS/minHeap.h"
#include "process.h"
#define MAX_PROCESSES 100


SchedulingAlgorithm algorithm;
Node *ready_Queue = NULL;
PCB *current_process = NULL;
pid_t current_child_pid = -1;

int process_count = 0;
int next_pid = 1;
int total_cpu_time = 0;
int start_time = -1;
int end_time = 0;

// Forward declarations
void insert_process(PCB new_pcb, SchedulingAlgorithm algo);
void context_switching(void);

// Print the ready queue
void print_ready_queue()
{
    printf("Ready Queue:\n");
    for (int i = 0; i < process_count; i++)
    {
        if (ready_Queue[i].process.state != TERMINATED)
        {
            printf("Process %d: Arrival=%d, Remaining=%d, State=%s\n",
                   ready_Queue[i].process.pid,
                   ready_Queue[i].process.arrival_time,
                   ready_Queue[i].process.remaining_time,
                   ready_Queue[i].process.state == RUNNING ? "RUNNING" : ready_Queue[i].process.state == WAITING ? "WAITING"
                                                                                                                 : "TERMINATED");
        }
    }
}

// Signal handler for SIGCHLD
// void handle_sigchld(int sig)
// {
//     int status;
//     pid_t pid;
//
// }
void insert_ready(Node **head, PCB process)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->process = process;
    new_node->next = NULL;

    if (*head == NULL)
    {
        *head = new_node;
    }
    else
    {
        Node *temp = *head;
        while (temp->next != NULL)
            temp = temp->next;
        temp->next = new_node;
    }
}

// Fork and start a process
void create_process(PCB new_pcb)
{

    insert_ready(&ready_Queue, new_pcb);

    pid_t pid = fork();
    if (pid == 0)
    {
        char *args[] = {"./process", NULL};
        execvp(args[0], args);
        exit(0);
    }
    else if (pid > 0)
    {
        // Parent
    }
    else
    {
    }
}

// preempt a process
void preempt_process()
{
}

// Start a process
void start_process(int pid)
{
    ready_Queue[pid].process.remaining_time--;
    printf("process with pid= %d has started", pid);
}

// Resume a process
void resume_process()
{
}

void run_SRTN_Algorithm()
{
    int current_time = 0;
    int processes_done = 0;
    // PCB *readyQueue = get_ready_Queue();

    // Set up SIGCHLD handler
    // signal(SIGCHLD, handle_sigchld);


    MinHeap *mnHeap = create_min_heap();

    for (int i = 0; i < process_count; i++)
        insert_process_min_heap(mnHeap, ready_Queue[i].process, i);

    while (processes_done < process_count)
    {
        PCB *next_process = extract_min(mnHeap);
        if (current_process != next_process)
        {
            context_switching();
            current_process = next_process;
        }
        start_process(next_process->pid);
        insert_process_min_heap(mnHeap, *next_process, next_process->pid);
    }
    print_minheap(mnHeap);
}


void handle_process_arrival(PCB new_process) {
   
    new_process.remaining_time = new_process.total_runtime;
    new_process.waiting_time = 0;
    new_process.state = READY;

    insert_process(new_process, algorithm);
}

// Update this function to handle PCB directly
void handle_pcb_arrival(PCB new_pcb) {
    insert_process(new_pcb, algorithm);
}


void get_message_ID(int *msgq_id, key_t *key) {
    *key = ftok("keyfile", 'A');
    *msgq_id = msgget(*key, IPC_CREAT | 0666);

    if (*msgq_id == -1) {
        perror("Error accessing message queue");
        // exit(EXIT_FAILURE);
    }
}


// void receive_new_process(int msgq_id) {
//     if (msgq_id == -1) return;

//     MsgBuffer message;

//     while (msgrcv(msgq_id, (void *) &message, sizeof(message.pcb), 1, IPC_NOWAIT) != -1) {
//         handle_process_arrival(message.pcb);
//         printf("\nScheduler: Received process runtime %d at arrival time %d\n",
//                message.pcb.execution_time, message.pcb.arrival_time);
//     }
// }

int init_message_queue() {
    int msgq_id;
    key_t key;
    key = ftok("keyfile", 'A');
    msgq_id = msgget(key, 0666);

    if (msgq_id == -1) {
        perror("Scheduler: Error accessing message queue");
        exit(EXIT_FAILURE);
    }
    
    return msgq_id;
}

// Initialize feedback message queue for terminated processes
int init_feedback_message_queue() {
    int msgq_id;
    key_t key;
    key = ftok("keyfile", 'B'); // Using 'B' to differentiate from the main queue
    msgq_id = msgget(key, IPC_CREAT | 0666);

    if (msgq_id == -1) {
        perror("Scheduler: Error creating feedback message queue");
        exit(EXIT_FAILURE);
    }
    
    return msgq_id;
}

int main(int argc, char *argv[]) {
    algorithm = atoi(argv[1]); // 1=HPF, 2=SRTN, 3=RR
    if (algorithm == RR && argc > 2) {
        int quantum = atoi(argv[2]);
        printf("Scheduler: Using Round Robin with quantum %d\n", quantum);
    }

    sync_clk(); 

    int msgq_id = init_message_queue();
    int feedback_msgq_id = init_feedback_message_queue();

    printf("Scheduler: Waiting for processes...\n");
    while (1) {
        MsgBuffer message;
        ssize_t r = msgrcv(msgq_id, &message, sizeof(message.pcb), 1, IPC_NOWAIT);
        if (r > 0) {
            printf("Scheduler: Received process ID %d, arrival %d, runtime %d, priority %d\n",
                message.pcb.id_from_file, message.pcb.arrival_time,
                message.pcb.total_runtime, message.pcb.priority);
                handle_process_arrival(message.pcb); // Direct handling of PCB
        }

        // Scheduler logic here (e.g., pick next process, preempt, etc.)
        // ...existing code...

        usleep(100000); // Avoid busy waiting
    }

    destroy_clk(0); // Clean up clock resources
    return 0;
}

// Context switching implementation
void context_switching() {
    if (current_process != NULL && current_process->state == RUNNING) {
        current_process->state = READY;
        printf("Context switch from process %d\n", current_process->pid);
    }
    // if (ready_Queue != NULL) {
    //     current_process = ready_Queue;
    //     current_process->state = RUNNING;
    //     printf("Context switch to process %d\n", current_process->pid);
    // }
}

void insert_process(PCB new_pcb, SchedulingAlgorithm algo) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    if (!new_node) {
        printf("Error: Memory allocation failed for new process node\n");
        return;
    }
    
    new_node->process = new_pcb;
    new_node->next = NULL;
    
    if (ready_Queue == NULL) {
        ready_Queue = new_node;
        return;
    }
    
    Node *current = ready_Queue;
    Node *prev = NULL;
    
    switch (algo) {
        case HPF:
            while (current != NULL && current->process.priority <= new_pcb.priority) {
                prev = current;
                current = current->next;
            }
            break;
            
        case SRTN: 
            while (current != NULL && current->process.remaining_time <= new_pcb.remaining_time) {
                prev = current;
                current = current->next;
            }
            break;
            
        case RR: 
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = new_node;
            return; 
    }
    
    if (prev == NULL) {
        new_node->next = ready_Queue;
        ready_Queue = new_node;
    } else {
        new_node->next = current;
        prev->next = new_node;
    }
}

void terminate_process(int pid) {
    Node *current = ready_Queue;
    Node *prev = NULL;

    while (current != NULL && current->process.pid != pid) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Process with PID %d not found in the ready queue.\n", pid);
        return;
    }
    
    // Prepare process data to send back to process generator
    PCB terminated_process = current->process;
    int current_time = get_clk();
    
    // Create message containing process data
    MsgBuffer termination_msg;
    termination_msg.mtype = 2; // Use message type 2 for terminated processes
    
    // Set terminated state and update statistics before sending
    terminated_process.state = TERMINATED;
    terminated_process.execution_time = get_clk() - terminated_process.arrival_time;
    
    // Just send the complete PCB back
    termination_msg.pcb = terminated_process;
    
    // Get the feedback message queue ID
    key_t key = ftok("keyfile", 'B');
    int feedback_msgq_id = msgget(key, 0666);
    
    if (feedback_msgq_id == -1) {
        perror("Error accessing feedback message queue");
    } else {
        // Send the terminated process data back to process generator
        if (msgsnd(feedback_msgq_id, &termination_msg, sizeof(termination_msg.pcb), 0) == -1) {
            perror("Error sending terminated process data");
        } else {
            printf("Sent terminated process %d data back to process generator\n", pid);
        }
    }

    // Remove process from ready queue
    if (prev == NULL) {
        ready_Queue = current->next;
    } else {
        prev->next = current->next; 
    }

    free(current);
    
}