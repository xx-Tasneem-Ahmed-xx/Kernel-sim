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

// Print the ready queue
void print_ready_queue()
{
    printf("Ready Queue:\n");
    Node *temp = ready_Queue;
    while (temp != NULL)
    {
        if (temp->process.state != TERMINATED)
        {
            printf("Process %d: Arrival=%d, Remaining=%d, State=%s\n",
                   temp->process.pid,
                   temp->process.arrival_time,
                   temp->process.remaining_time,
                   temp->process.state == RUNNING ? "RUNNING" : temp->process.state == WAITING ? "WAITING"
                                                                                               : "TERMINATED");
        }
        temp = temp->next;
    }
}

void insert_ready(Node **head, PCB process)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->process = process;
    new_node->next = NULL;

    printf("insert process with pid = %d\n", new_node->process.pid);

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
    process_count++;
}

// Fork and start a process
void create_process(PCB new_pcb)
{
    // Assign sequential PID before insertion
    new_pcb.pid = next_pid++;
    insert_ready(&ready_Queue, new_pcb);

    pid_t pid = fork();
    if (pid == 0)
    {
        char *args[] = {"./process.o", NULL};
        execvp(args[0], args);
        exit(0); // Notify scheduler via SIGCHLD
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

PCB *get_process(int pid)
{

    Node *temp = ready_Queue;
    while (temp != NULL)
    {
        if (temp->process.pid == pid)
        {
            return &(temp->process);
        }
        temp = temp->next;
    }
    return NULL;
}

// Resume a process
void resume_process()
{
}

// Start a process
void start_process(int pid)
{
    Node *temp = ready_Queue;
    while (temp != NULL)
    {
        if (temp->process.pid == pid)
        {
            temp->process.remaining_time--;
            printf("process with pid= %d has started (remaining time: %d)\n",
                   pid, temp->process.remaining_time);
            return;
        }
        temp = temp->next;
    }
    printf("process with pid= %d not found in ready queue\n", pid);
}

void run_SRTN_Algorithm()
{
    int current_time = 0;
    int processes_done = 0;
    int remaining_processes = process_count;


    MinHeap *mnHeap = create_min_heap();

    // Debug - print processes before insertion
    printf("Processes before insertion to min-heap:\n");
    Node *temp = ready_Queue;
    while (temp != NULL)
    {
        printf("Process %d: Arrival=%d, Remaining=%d\n",
               temp->process.pid,
               temp->process.arrival_time,
               temp->process.remaining_time);
        temp = temp->next;
    }

    // Convert linked list to min-heap
    temp = ready_Queue;
    while (temp != NULL)
    {
        insert_process_min_heap(mnHeap, &(temp->process), temp->process.pid);
        temp = temp->next;
    }

    // Debug - print the min-heap
    printf("\nMin-Heap after insertion:\n");
    print_minheap(mnHeap);

    // Use remaining_processes instead of modifying process_count
    while (remaining_processes > 0 && mnHeap->size > 0)
    {
        PCB *next_process = extract_min(mnHeap);
        if (next_process)
        {
            printf("\nNext process chosen by SRTN: Process %d with remaining time %d\n",
                   next_process->pid, next_process->remaining_time);

            if (current_process != next_process)
            {
                // context_switching();
                current_process = next_process;
            }

            // Let start_process handle decrementing the remaining time
            start_process(next_process->pid);

            // Get the updated remaining time from the linked list
            PCB *updated_process = get_process(next_process->pid);
            if (updated_process)
            {
                next_process->remaining_time = updated_process->remaining_time;
            }

            if (next_process->remaining_time > 0)
            {
                // Re-insert only if process still has work to do
                insert_process_min_heap(mnHeap, next_process, next_process->pid);
            }
            else
            {
                // Process is complete
                printf("Process %d completed\n", next_process->pid);
                processes_done++;
                remaining_processes--;
            }
        }
        else
        {
            // No process available
            break;
        }
    }

    printf("\nFinal state of Min-Heap:\n");
    print_minheap(mnHeap);
    printf("\nProcesses completed: %d\n", processes_done);

    destroy_min_heap(mnHeap);
}


int main()
{
    algorithm = SRTN;

    PCB processA = {0};
    PCB processB = {0};
    PCB processC = {0};
    PCB processD = {0};
    PCB processE = {0};

    processA.arrival_time = 0;
    processB.arrival_time = 2;
    processC.arrival_time = 4;
    processD.arrival_time = 6;
    processE.arrival_time = 8;

    processA.remaining_time = 4;
    processB.remaining_time = 2; // This should be chosen first (pid=2 with lowest remaining time)
    processC.remaining_time = 1;
    processD.remaining_time = 7;
    processE.remaining_time = 5;

    create_process(processA); // Will get pid=1
    create_process(processB); // Will get pid=2
    create_process(processC); // Will get pid=3
    create_process(processD); // Will get pid=4
    create_process(processE); // Will get pid=5

    run_SRTN_Algorithm();

    // Free the memory for ready_Queue
    Node *current = ready_Queue;
    while (current != NULL)
    {
        Node *temp = current;
        current = current->next;
        free(temp);
    }
    // print_ready_Queue();
}

void handle_process_arrival(Process new_process) {
    PCB new_pcb;

    new_pcb.id_from_file = new_process.id;
    new_pcb.arrival_time = new_process.arrival_time;
    new_pcb.total_runtime = new_process.run_time;
    new_pcb.priority = new_process.priority;
    new_pcb.remaining_time = new_process.run_time;
    new_pcb.waiting_time = 0;
    new_pcb.state = READY;

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


void receive_new_process(int msgq_id) {
    if (msgq_id == -1) return;

    MsgBuffer message;

    while (msgrcv(msgq_id, (void *) &message, sizeof(message.process), 1, IPC_NOWAIT) != -1) {
        handle_process_arrival(message.process);
        printf("\nScheduler: Received process runtime %d at arrival time %d\n",
               message.process.run_time, message.process.arrival_time);
    }
}


// int main(int argc, char *argv[]) {
//     algorithm = atoi(argv[1]); // 1=HPF, 2=SRTN, 3=RR
//     sync_clk();

//     int msgq_id;
//     key_t key;
//     get_message_ID(&msgq_id, &key);


//     printf("Scheduler: Waiting for message...\n");
//     printf("key file is %d and msg qeueu id is %d", key, msgq_id);
//     while (1) {
//         int current_time = get_clk();
//         receive_new_process(msgq_id);
//         usleep(50000);
//     }
//     destroy_clk(0);
//     printf("Scheduler: finished receiving...\n");
//     // print_ready_queue();

//     return 0;
// }