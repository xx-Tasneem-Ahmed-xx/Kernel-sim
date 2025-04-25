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
#include <sys/shm.h>

#include "DS/priorityQueue.h"
#include "DS/Queue.h"

PriorityQueue pq;      // Still using PriorityQueue for initial process data
PCBQueue ArrivalQueue; // Changed from ProcessQueue
PCBQueue BurstQueue;   // Changed from ProcessQueue

// IPC variables
int msgq_id;
key_t msgq_key;

pid_t scheduler_pid;


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
void createProcess(PCB *process);





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
    
    // Add divider and logging information about the scheduler configuration
    print_divider("OS Scheduler Simulation");
    log_message(LOG_SYSTEM, "Starting simulation with %s algorithm", 
        params.algorithm == HPF ? "HPF" : 
        params.algorithm == SRTN ? "SRTN" : 
        params.algorithm == RR ? "Round Robin (q=%d)" : "Unknown", 
        params.quantum);
    log_message(LOG_INFO, "Loading processes from %s", params.filename);
    
    int process_count = 0; // Track number of processes sent

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

    scheduler_pid = fork();
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
        // Replace printf with log_message
        if (pq_empty(&pq))
        {
            log_message(LOG_INFO, "No more processes to schedule");
            break;
        }

        int current_time = get_clk();
        while (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time)
        {
            p = pq_top(&pq);

            // Convert Process to PCB before adding to ArrivalQueue
            PCB *new_pcb = (PCB *)malloc(sizeof(PCB));
            new_pcb->id_from_file = p.id;
            new_pcb->arrival_time = p.arrival_time;
            new_pcb->execution_time = p.execution_time;
            new_pcb->priority = p.priority;
            new_pcb->waiting_time = 0;
            new_pcb->start_time = -1;
            new_pcb->remaining_time = p.execution_time; // Initialize as int

            createProcess(new_pcb);

            log_message(LOG_DEBUG, "Process %d assigned shm_id %d", new_pcb->id_from_file, new_pcb->shm_id);

            queue_enqueue(&ArrivalQueue, *new_pcb);

            pq_pop(&pq);

            // Replace printf with log_message
            log_message(LOG_PROCESS, "Process %d arrived at time %d, Runtime: %d, Priority: %d", 
                       new_pcb->id_from_file, current_time, new_pcb->execution_time, new_pcb->priority);

            MsgBuffer message;
            message.mtype = 1;
            message.pcb = *new_pcb; // Sending PCB instead of Process

            if (msgsnd(msgq_id, &message, sizeof(message.pcb), !IPC_NOWAIT) == -1)
            {
                perror("Error sending process to scheduler");
                exit(EXIT_FAILURE);
            }

            // Replace printf with log_message
            log_message(LOG_DEBUG, "Process %d sent to scheduler", new_pcb->id_from_file);
            
            process_count++; // Increment process count
        }

        // Check for terminated processes
        receive_terminated_processes();

        usleep(100000); // sleep for 0.1 seconds
    }

    // Send completion signal to the scheduler
    print_divider("Process Generation Complete");
    log_message(LOG_SYSTEM, "All %d processes sent to scheduler", process_count);
    kill(scheduler_pid, SIGUSR1);

    // Wait specifically for the scheduler process to terminate
    int status;
    log_message(LOG_SYSTEM, "Waiting for scheduler process %d to terminate", scheduler_pid);
    waitpid(scheduler_pid, &status, 0);
    log_message(LOG_SYSTEM, "Scheduler process %d terminated", scheduler_pid);
    
    // Note: Intentionally not waiting for the clock process
    
    clear_resources(0);
    return 0;
}

void createProcess(PCB *process){
    int shmid;
    int *shm_remaining_time;
    pid_t pid;
     // Create shared memory for remaining time
     shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
     if (shmid == -1) {
         perror("shmget failed");
         exit(-1);
     }
 
     // Attach shared memory
     shm_remaining_time = (int *)shmat(shmid, NULL, 0);
     if (shm_remaining_time == (int *)-1) {
         perror("shmat failed");
         exit(-1);
     }
 
     // Initialize shared memory with the process remaining time
     *shm_remaining_time = process->remaining_time;
    //  printf("Shared memory created with ID: %d\n", shmid);
     process->shm_id = shmid; // Store shared memory ID in the PCB
 
     // Detach from shared memory (scheduler will reattach when needed)
     shmdt(shm_remaining_time);

     
    pid = fork(); // Fork a new process
    if (pid == -1) {
        perror("fork");
        exit(-1);
    }

    if (pid == 0) { // Child process
        char runtime_str[10], shmid_str[20], scheduler_pid_str[20];
        sprintf(runtime_str, "%d", process->remaining_time); // Convert process runtime to string
        sprintf(shmid_str, "%d", shmid); // Convert shared memory ID to string
        sprintf(scheduler_pid_str, "%d", scheduler_pid); // Convert scheduler PID to string

        execl("./bin/process.out", "process.out", runtime_str, shmid_str, scheduler_pid_str, NULL); // Execute the process with scheduler PID
        perror("execl failed");
        exit(1);
    } else { 
        process->pid = pid;
        kill(pid, SIGSTOP); 

    }
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

    log_message(LOG_SYSTEM, "Message queue created successfully with ID: %d", msgq_id);

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

    log_message(LOG_SYSTEM, "Feedback message queue created successfully with ID: %d", feedback_msgq_id);
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

        log_message(LOG_PROCESS, "Process %d completed and added to Burst Queue", terminated_pcb.id_from_file);
        log_message(LOG_STAT, "  Final stats: Arrival=%d, Total runtime=%d, Waiting time=%d",
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
    log_message(LOG_SYSTEM, "🧹 Cleaned up and exiting.");
    exit(0);
}
