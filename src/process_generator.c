#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>    // for fork, execl
#include <sys/types.h> // for pid_t
#include <sys/ipc.h>   // for IPC
#include <sys/msg.h>   // for message queues
#include <sys/wait.h>  // for wait()
#include "clk.h"
#include "priorityQueue.h"
#include "Queue.h"
#include "headers.h"

PriorityQueue pq;
ProcessQueue ArrivalQueue;
ProcessQueue BurstQueue;

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

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    signal(SIGINT, clear_resources); // In case CTRL+C is pressed

    SchedulingParams params = userUi();

    // Priority queue setup
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
        // printf("current time: %d\n", current_time);
        while (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time) {
            p = pq_top(&pq);
            queue_enqueue(&ArrivalQueue, p);
            pq_pop(&pq);

            printf("Process %d arrived at time %d\n", p.id, current_time);

            MsgBuffer message;
            message.mtype = 1;
            message.process = p;

            if (msgsnd(msgq_id, &message, sizeof(message.process), !IPC_NOWAIT) == -1) {
                perror("Error sending process to scheduler");
                exit(EXIT_FAILURE);
            }

            printf("Process %d sent to scheduler\n", p.id);
        }

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
