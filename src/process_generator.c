#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include "clk.h"
#include "headers.h"

#include "DS/priorityQueue.h"
#include "DS/Queue.h"
#include "headers.h"
PriorityQueue pq;      // Still using PriorityQueue for initial process data
PCBQueue ArrivalQueue; // Changed from ProcessQueue
PCBQueue BurstQueue;   // Changed from ProcessQueue

// IPC variables
int msgq_id;
key_t msgq_key;

typedef struct
{
    SchedulingAlgorithm algorithm;
    int quantum;
    const char* filename;
} SchedulingParams;

void clear_resources(int signum);
void read_processes(const char *filename, PriorityQueue *pq);
void initializeIPC();
void receive_terminated_processes();

// Add function to parse command-line arguments
int parse_args(int argc, char *argv[], SchedulingParams *params) {
    params->quantum = 0;
    params->filename = NULL;
    params->algorithm = HPF; // default

    if (argc < 5) {
        fprintf(stderr, "Usage: %s -s <scheduling-algorithm> -f <processes-text-file> [-q <quantum>]\n", argv[0]);
        fprintf(stderr, "Example: %s -s rr -f processes.txt -q 2\n", argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "rr") == 0) {
                params->algorithm = RR;
            } else if (strcmp(argv[i+1], "hpf") == 0) {
                params->algorithm = HPF;
            } else if (strcmp(argv[i+1], "srtn") == 0) {
                params->algorithm = SRTN;
            } else {
                fprintf(stderr, "Unknown scheduling algorithm: %s\n", argv[i+1]);
                return 0;
            }
            i++;
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            params->filename = argv[i+1];
            i++;
        } else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc) {
            params->quantum = atoi(argv[i+1]);
            i++;
        }
    }

    if (params->filename == NULL) {
        fprintf(stderr, "Missing process file (-f <filename>)\n");
        return 0;
    }
    if (params->algorithm == RR && params->quantum <= 0) {
        fprintf(stderr, "Round Robin requires a positive quantum (-q <quantum>)\n");
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, clear_resources); // In case CTRL+C is pressed

    SchedulingParams params;
    if (!parse_args(argc, argv, &params)) {
        exit(EXIT_FAILURE);
    }

    if (params.algorithm == SRTN)
    {
        pq_init(&pq, 20, SORT_BY_ARRIVAL_THEN_PRIORITY);
    }
    else
    {
        pq_init(&pq, 20, SORT_BY_ARRIVAL_TIME);
    }
    queue_init(&ArrivalQueue, 20);

    read_processes(params.filename, &pq);
    initializeIPC();

    // Start scheduler with RR and quantum = 2
    pid_t scheduler_pid = fork();
    if (scheduler_pid == 0)
    {
        char algorithm_str[10];
        sprintf(algorithm_str, "%d", params.algorithm);
        if (params.algorithm == RR)
        {
            char quantum_str[10];
            sprintf(quantum_str, "%d", params.quantum);
            execl("./bin/scheduler", "scheduler", algorithm_str, quantum_str, NULL);
        }
        else
        {
            execl("./bin/scheduler", "scheduler", algorithm_str, NULL);
        }
        perror("Error executing scheduler");
        exit(EXIT_FAILURE);
    }

    // Start clock
    pid_t clk_pid = fork();
    if (clk_pid == 0)
    {
        signal(SIGINT, clear_resources);
        init_clk();
        sync_clk();
        run_clk();
    }

    // Parent (process_generator) continues
    sync_clk();

    Process p;
    while (1)
    {
        if (pq_empty(&pq))
        {
            printf("No more processes to schedule\n");
            break;
        }

        int current_time = get_clk();
        while (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time)
        {
            p = pq_top(&pq);

            // Convert Process to PCB before adding to ArrivalQueue
            PCB new_pcb;
            new_pcb.id_from_file = p.id;
            new_pcb.pid = p.id; 
            new_pcb.arrival_time = p.arrival_time;
            new_pcb.execution_time = p.execution_time;
            new_pcb.remaining_time = p.execution_time;
            new_pcb.priority = p.priority;
            new_pcb.waiting_time = 0;
            new_pcb.execution_time = 0;
            new_pcb.start_time=-1;

            queue_enqueue(&ArrivalQueue, new_pcb);
            pq_pop(&pq);

            printf("Process %d arrived at time %d\n", new_pcb.id_from_file, current_time);

            MsgBuffer message;
            message.mtype = 1;
            message.pcb = new_pcb; // Sending PCB instead of Process

            if (msgsnd(msgq_id, &message, sizeof(message.pcb), !IPC_NOWAIT) == -1)
            {
                perror("Error sending process to scheduler");
                exit(EXIT_FAILURE);
            }

            printf("Process %d sent to scheduler\n", new_pcb.id_from_file);
        }

        // Check for terminated processes
        receive_terminated_processes();

        usleep(100000); // sleep for 0.1 seconds
    }

    printf("✅ All processes sent. Waiting 5 seconds for scheduler to finish...\n");
    sleep(5);

    // Send completion signal to the scheduler
    printf("Process Generator: All processes sent. Notifying scheduler...\n");
    kill(scheduler_pid, SIGUSR1);

    wait(NULL); // Wait for children (scheduler + clock)
    clear_resources(0);
    return 0;
}

void initializeIPC()
{
    msgq_key = ftok("keyfile", 'A');
    if (msgq_key == -1)
    {
        perror("Error creating message queue key");
        exit(EXIT_FAILURE);
    }

    msgq_id = msgget(msgq_key, IPC_CREAT | 0666);
    if (msgq_id == -1)
    {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    printf("Message queue created successfully with ID: %d\n", msgq_id);

    // Initialize feedback message queue for terminated processes
    key_t feedback_key = ftok("keyfile", 'B');
    if (feedback_key == -1)
    {
        perror("Error creating feedback message queue key");
        exit(EXIT_FAILURE);
    }

    int feedback_msgq_id = msgget(feedback_key, IPC_CREAT | 0666);
    if (feedback_msgq_id == -1)
    {
        perror("Error creating feedback message queue");
        exit(EXIT_FAILURE);
    }

    printf("Feedback message queue created successfully with ID: %d\n", feedback_msgq_id);
}

// Function to handle terminated processes
void receive_terminated_processes()
{
    key_t feedback_key = ftok("keyfile", 'B');
    int feedback_msgq_id = msgget(feedback_key, 0666);

    if (feedback_msgq_id == -1)
    {
        return; // Queue not ready yet
    }

    MsgBuffer message;
    // Receive all available terminated process messages (type 2)
    while (msgrcv(feedback_msgq_id, &message, sizeof(message.pcb), 2, IPC_NOWAIT) != -1)
    {
        PCB terminated_pcb = message.pcb;

        // Add to burst queue
        queue_enqueue(&BurstQueue, terminated_pcb);

        printf("Process %d completed and added to Burst Queue\n", terminated_pcb.id_from_file);
        printf("  Final stats: Arrival=%d, Total runtime=%d, Waiting time=%d\n",
               terminated_pcb.arrival_time, terminated_pcb.execution_time, terminated_pcb.waiting_time);
    }
}

void read_processes(const char *filename, PriorityQueue *pq)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open processes.txt");
        exit(EXIT_FAILURE);
    }

    char line[256];
    fgets(line, sizeof(line), file); // Skip header

    while (fgets(line, sizeof(line), file))
    {
        Process p;
        if (sscanf(line, "%d %d %d %d", &p.id, &p.arrival_time, &p.execution_time, &p.priority) == 4) {
            pq_push(pq, p);
        }
    }

    fclose(file);
}

void clear_resources(int signum)
{
    (void)signum;
    pq_free(&pq);
    queue_free(&ArrivalQueue);
    msgctl(msgq_id, IPC_RMID, NULL);
    destroy_clk(1);
    printf("🧹 Cleaned up and exiting.\n");
    exit(0);
}
