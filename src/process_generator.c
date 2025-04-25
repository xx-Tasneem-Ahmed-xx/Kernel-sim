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

PriorityQueue pq;
PCBQueue ArrivalQueue;
PCBQueue BurstQueue;

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
    params->algorithm = HPF;

    if (argc < 5) {
        fprintf(stderr, "Usage: %s -s <scheduling-algorithm> -f <processes-text-file> [-q <quantum>]\n", argv[0]);
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
    signal(SIGINT, clear_resources);

    SchedulingParams params;
    if (!parse_args(argc, argv, &params)) {
        exit(EXIT_FAILURE);
    }

    print_divider("OS Scheduler Simulation");

    char algorithm_name[50];
    if (params.algorithm == HPF) {
        strcpy(algorithm_name, "HPF");
    } else if (params.algorithm == SRTN) {
        strcpy(algorithm_name, "SRTN");
    } else if (params.algorithm == RR) {
        sprintf(algorithm_name, "Round Robin (q=%d)", params.quantum);
    } else {
        strcpy(algorithm_name, "Unknown");
    }

    log_message(LOG_SYSTEM, "Starting simulation with %s algorithm", algorithm_name);
    log_message(LOG_INFO, "Loading processes from %s", params.filename);

    int process_count = 0;

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

    sync_clk();

    Process p;
    while (1)
    {
        if (pq_empty(&pq))
        {
            log_message(LOG_INFO, "No more processes to schedule");
            break;
        }

        int current_time = get_clk();
        while (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time)
        {
            p = pq_top(&pq);

            PCB *new_pcb = (PCB *)malloc(sizeof(PCB));
            new_pcb->id_from_file = p.id;
            new_pcb->arrival_time = p.arrival_time;
            new_pcb->execution_time = p.execution_time;
            new_pcb->priority = p.priority;
            new_pcb->waiting_time = 0;
            new_pcb->start_time = -1;
            new_pcb->remaining_time = p.execution_time;

            createProcess(new_pcb);

            queue_enqueue(&ArrivalQueue, *new_pcb);

            pq_pop(&pq);

            log_message(LOG_PROCESS, "Process %d arrived at time %d, Runtime: %d, Priority: %d",
                       new_pcb->id_from_file, current_time, new_pcb->execution_time, new_pcb->priority);

            MsgBuffer message;
            message.mtype = 1;
            message.pcb = *new_pcb;

            if (msgsnd(msgq_id, &message, sizeof(message.pcb), !IPC_NOWAIT) == -1)
            {
                perror("Error sending process to scheduler");
                exit(EXIT_FAILURE);
            }

            process_count++;
        }

        receive_terminated_processes();

        usleep(100000);
    }

    print_divider("Process Generation Complete");
    log_message(LOG_SYSTEM, "All %d processes sent to scheduler", process_count);
    kill(scheduler_pid, SIGUSR1);

    int status;
    log_message(LOG_SYSTEM, "Waiting for scheduler process %d to terminate", scheduler_pid);
    waitpid(scheduler_pid, &status, 0);
    log_message(LOG_SYSTEM, "Scheduler process %d terminated", scheduler_pid);

    clear_resources(0);
    return 0;
}

void createProcess(PCB *process){
    int shmid;
    int *shm_remaining_time;
    pid_t pid;

    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(-1);
    }

    shm_remaining_time = (int *)shmat(shmid, NULL, 0);
    if (shm_remaining_time == (int *)-1) {
        perror("shmat failed");
        exit(-1);
    }

    *shm_remaining_time = process->remaining_time;
    process->shm_id = shmid;

    shmdt(shm_remaining_time);

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(-1);
    }

    if (pid == 0) {
        char runtime_str[10], shmid_str[20], scheduler_pid_str[20];
        sprintf(runtime_str, "%d", process->remaining_time);
        sprintf(shmid_str, "%d", shmid);
        sprintf(scheduler_pid_str, "%d", scheduler_pid);

        execl("./bin/process.out", "process.out", runtime_str, shmid_str, scheduler_pid_str, NULL);
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

    log_message(LOG_SYSTEM, "Message queue created with ID: %d", msgq_id);

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

    log_message(LOG_SYSTEM, "Feedback message queue created with ID: %d", feedback_msgq_id);
}

void receive_terminated_processes()
{
    key_t feedback_key = ftok("keyfile", 'B');
    int feedback_msgq_id = msgget(feedback_key, 0666);

    if (feedback_msgq_id == -1)
    {
        return;
    }

    MsgBuffer message;
    while (msgrcv(feedback_msgq_id, &message, sizeof(message.pcb), 2, IPC_NOWAIT) != -1)
    {
        PCB terminated_pcb = message.pcb;
        queue_enqueue(&BurstQueue, terminated_pcb);

        log_message(LOG_PROCESS, "Process %d completed", terminated_pcb.id_from_file);
        log_message(LOG_STAT, "Final stats: Arrival=%d, Runtime=%d, Waiting=%d",
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
    fgets(line, sizeof(line), file);

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
    log_message(LOG_SYSTEM, "Cleaned up and exiting");
    exit(0);
}