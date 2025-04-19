#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>    // for fork, execl
#include <sys/types.h> // for pid_t
#include <sys/ipc.h>   // for IPC
#include <sys/msg.h>   // for message queues
#include <sys/wait.h>  // for wait()
#include "clk.h"

#include "DS/priorityQueue.h"
#include "DS/Queue.h"
#include "headers.h"


PriorityQueue pq; // Still using PriorityQueue for initial process data
PCBQueue ArrivalQueue; // Changed from ProcessQueue
PCBQueue BurstQueue;   // Changed from ProcessQueue

// IPC variables
int msgq_id;
key_t msgq_key;

typedef struct {
    SchedulingAlgorithm algorithm;
    int quantum;
} SchedulingParams;

void clear_resources(int signum);

void read_processes(char *filename, PriorityQueue *pq);

SchedulingParams userUi();

void initializeIPC();

void receive_terminated_processes();


int main(int argc, char *argv[]) {
    signal(SIGINT, clear_resources); // In case CTRL+C is pressed

    SchedulingParams params = userUi();

    if (params.algorithm == SRTN) {
        pq_init(&pq, 20, SORT_BY_ARRIVAL_THEN_PRIORITY);
    } else {
        pq_init(&pq, 20, SORT_BY_ARRIVAL_TIME);
    }
    queue_init(&ArrivalQueue, 20);
    queue_init(&BurstQueue, 20);

    read_processes("processes.txt", &pq);
    initializeIPC();

    // Fork scheduler
    pid_t scheduler_pid = fork();
    if (scheduler_pid == 0) {
        char algorithm_str[10];
        sprintf(algorithm_str, "%d", params.algorithm);
        if (params.algorithm == RR) {
            char quantum_str[10];
            sprintf(quantum_str, "%d", params.quantum);
            execl("./scheduler", "scheduler", algorithm_str, quantum_str, NULL);
        } else {
            execl("./scheduler", "scheduler", algorithm_str, NULL);
        }
        perror("Error executing scheduler");
        exit(EXIT_FAILURE);
    }

    // Fork clock
    pid_t clk_pid = fork();
    if (clk_pid == 0) {
        signal(SIGINT, clear_resources);
        init_clk();
        sync_clk();
        run_clk();
    }

    // Parent (process_generator) continues
    sync_clk();

    Process p;
    while (1) {
        if (pq_empty(&pq)) {
            printf("No more processes to schedule\n");
            break;
        }

        int current_time = get_clk();
        while (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time) {
            p = pq_top(&pq);
            
            // Convert Process to PCB before adding to ArrivalQueue
            PCB new_pcb;
            new_pcb.id_from_file = p.id;
            new_pcb.pid = p.id; // Temporary PID, scheduler will assign the real one
            new_pcb.arrival_time = p.arrival_time;
            new_pcb.total_runtime = p.run_time;
            new_pcb.remaining_time = p.run_time;
            new_pcb.priority = p.priority;
            new_pcb.waiting_time = 0;
            new_pcb.execution_time = 0;
            new_pcb.state = READY;
            
            queue_enqueue(&ArrivalQueue, new_pcb);
            pq_pop(&pq);

            printf("Process %d arrived at time %d\n", new_pcb.id_from_file, current_time);

            MsgBuffer message;
            message.mtype = 1;
            message.pcb = new_pcb; // Sending PCB instead of Process

            if (msgsnd(msgq_id, &message, sizeof(message.pcb), !IPC_NOWAIT) == -1) {
                perror("Error sending process to scheduler");
                exit(EXIT_FAILURE);
            }

            printf("Process %d sent to scheduler\n", new_pcb.id_from_file);
        }

        // Check for terminated processes
        receive_terminated_processes();

        usleep(100000); // sleep for 0.1 seconds
    }

    wait(NULL);
    clear_resources(0);

    return 0;
}


void initializeIPC() {
    msgq_key = ftok("keyfile", 'A');
    if (msgq_key == -1) {
        perror("Error creating message queue key");
        exit(EXIT_FAILURE);
    }

    msgq_id = msgget(msgq_key, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    printf("Message queue created successfully with ID: %d\n", msgq_id);
    
    // Initialize feedback message queue for terminated processes
    key_t feedback_key = ftok("keyfile", 'B');
    if (feedback_key == -1) {
        perror("Error creating feedback message queue key");
        exit(EXIT_FAILURE);
    }

    int feedback_msgq_id = msgget(feedback_key, IPC_CREAT | 0666);
    if (feedback_msgq_id == -1) {
        perror("Error creating feedback message queue");
        exit(EXIT_FAILURE);
    }

    printf("Feedback message queue created successfully with ID: %d\n", feedback_msgq_id);
}

// Function to handle terminated processes
void receive_terminated_processes() {
    key_t feedback_key = ftok("keyfile", 'B');
    int feedback_msgq_id = msgget(feedback_key, 0666);
    
    if (feedback_msgq_id == -1) {
        return;  // Queue not ready yet
    }
    
    MsgBuffer message;
    // Receive all available terminated process messages (type 2)
    while (msgrcv(feedback_msgq_id, &message, sizeof(message.pcb), 2, IPC_NOWAIT) != -1) {
        PCB terminated_pcb = message.pcb;
        
        // Add to burst queue
        queue_enqueue(&BurstQueue, terminated_pcb);
        
        printf("Process %d completed and added to Burst Queue\n", terminated_pcb.id_from_file);
        printf("  Final stats: Arrival=%d, Total runtime=%d, Waiting time=%d\n", 
               terminated_pcb.arrival_time, terminated_pcb.total_runtime, terminated_pcb.waiting_time);
    }
}

SchedulingParams userUi() {
    SchedulingParams params;
    int choice;

    printf("Select a scheduling algorithm:\n");
    printf("1. HPF (Non-preemptive Highest Priority First)\n");
    printf("2. SRTN (Shortest Remaining Time Next)\n");
    printf("3. RR (Round Robin)\n");
    printf("Enter your choice (1-3): ");

    scanf("%d", &choice);

    switch (choice) {
        case 1:
            params.algorithm = HPF;
            printf("Selected HPF algorithm\n");
            break;
        case 2:
            params.algorithm = SRTN;
            printf("Selected SRTN algorithm\n");
            break;
        case 3:
            params.algorithm = RR;
            printf("Selected RR algorithm\n");
            printf("Enter time quantum for Round Robin: ");
            scanf("%d", &params.quantum);
            printf("Time quantum set to: %d\n", params.quantum);
            break;
        default:
            printf("Invalid choice, defaulting to HPF\n");
            params.algorithm = HPF;
            break;
    }

    return params;
}

void read_processes(char *filename, PriorityQueue *pq) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open processes.txt");
        exit(EXIT_FAILURE);
    }

    char line[256];
    fgets(line, sizeof(line), file); // Skip header

    while (fgets(line, sizeof(line), file)) {
        Process p;
        if (sscanf(line, "%d %d %d %d", &p.id, &p.arrival_time, &p.run_time, &p.priority) == 4) {
            pq_push(pq, p);
        }
    }
    fclose(file);
}

void clear_resources(int signum) {
    (void) signum;
    pq_free(&pq);
    queue_free(&ArrivalQueue);
    queue_free(&BurstQueue);
    msgctl(msgq_id, IPC_RMID, NULL);
    destroy_clk(1);
    printf("Resources cleared, exiting.\n");
    exit(EXIT_SUCCESS);
}
