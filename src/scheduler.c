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


int main()
{
    // init_ready_Queue();
    algorithm = SRTN;

    PCB processA;
    PCB processB;
    PCB processC;
    PCB processD;
    PCB processE;

    processA.arrival_time = 0;
    processB.arrival_time = 2;
    processC.arrival_time = 4;
    processD.arrival_time = 6;
    processE.arrival_time = 8;

    processA.remaining_time = 4;
    processB.remaining_time = 2;
    processC.remaining_time = 1;
    processD.remaining_time = 7;
    processE.remaining_time = 5;

    create_process(processA);
    create_process(processB);
    create_process(processC);
    create_process(processD);
    create_process(processE);

    // insert_process(processA, algorithm);
    // insert_process(processB, algorithm);
    // insert_process(processC, algorithm);
    // insert_process(processD, algorithm);
    // insert_process(processE, algorithm);

    // print_ready_Queue();
    process_count = 5;
    run_SRTN_Algorithm();
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