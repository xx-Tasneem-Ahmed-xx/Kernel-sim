#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include "clk.h"
#include "headers.h"

#include "DS/priorityQueue.h"
#include "DS/Queue.h"

PriorityQueue pq;
ProcessQueue ArrivalQueue;

// IPC variables
int msgq_id;
key_t msgq_key;

void clear_resources(int signum);
void read_processes(const char *filename, PriorityQueue *pq);
void initializeIPC();

int main() {
    signal(SIGINT, clear_resources); // Ctrl+C cleanup

    printf("🧪 TESTING MODE: Round Robin\n");

    // Init queues
    pq_init(&pq, 20, SORT_BY_ARRIVAL_TIME);
    queue_init(&ArrivalQueue, 20);

    read_processes("processes.txt", &pq);
    initializeIPC();

    // Start scheduler with RR and quantum = 2
    pid_t scheduler_pid = fork();
    if (scheduler_pid == 0) {
        execl("./scheduler", "scheduler", "3", "2", NULL); // RR = 3, Quantum = 2
        perror("Failed to start scheduler");
        exit(EXIT_FAILURE);
    }

    // Start clock
    pid_t clk_pid = fork();
    if (clk_pid == 0) {
        init_clk();
        sync_clk();
        run_clk();
    }

    // Give the clock time to initialize
    sleep(1);

    sync_clk();

    // Main loop: send processes to scheduler at correct times
    while (!pq_empty(&pq)) {
        int current_time = get_clk();

        while (!pq_empty(&pq) && pq_top(&pq).arrival_time <= current_time) {
            Process p = pq_top(&pq);
            pq_pop(&pq);
            queue_enqueue(&ArrivalQueue, p);

            MsgBuffer msg;
            msg.mtype = 1;
            msg.process = p;

            if (msgsnd(msgq_id, &msg, sizeof(msg.process), !IPC_NOWAIT) == -1) {
                perror("Failed to send process");
                clear_resources(0);
            }
            
            printf("📤 Sent process %d to scheduler at time %d [runtime: %d]\n", p.id, current_time, p.run_time);
        }

        usleep(100000); // check every 0.1s
    }

    printf("✅ All processes sent. Waiting 5 seconds for scheduler to finish...\n");
    sleep(5);

    wait(NULL); // Wait for children (scheduler + clock)
    clear_resources(0);
    return 0;
}

void initializeIPC() {
    msgq_key = ftok("keyfile", 'A');
    if (msgq_key == -1) {
        perror("Error creating IPC key");
        exit(EXIT_FAILURE);
    }

    msgq_id = msgget(msgq_key, IPC_CREAT | 0666);
    if (msgq_id == -1) {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    printf("✅ IPC ready. Queue ID = %d\n", msgq_id);
}

void read_processes(const char *filename, PriorityQueue *pq) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Can't open processes.txt");
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
    (void)signum;
    pq_free(&pq);
    queue_free(&ArrivalQueue);
    msgctl(msgq_id, IPC_RMID, NULL);
    destroy_clk(1);
    printf("🧹 Cleaned up and exiting.\n");
    exit(0);
}
