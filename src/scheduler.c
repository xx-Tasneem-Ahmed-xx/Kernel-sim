#include "scheduler.h"

#include <sys/msg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "headers.h"
#include "pcb.h"
#include "DS/Queue.h"

#define MAX_PROCESSES 100

SchedulingAlgorithm algorithm = RR; // Only RR
ProcessQueue rr_queue;
int process_count = 0;
int total_cpu_time = 0;
int finished_processes = 0;
int start_time = -1;
int quantum = 0;

int turnaround_times[100];
float wta_list[100];
int wait_times[100];

void calculate_and_write_metrics();

Process* find_process_by_pid(pid_t pid) {
    for (int i = 0; i < rr_queue.size; i++) {
        size_t idx = (rr_queue.front + i) % rr_queue.capacity;
        if (rr_queue.data[idx].os_pid == pid) {
            return &rr_queue.data[idx];
        }
    }
    return NULL;
}

void handle_sigchld(int signum) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        Process* p = find_process_by_pid(pid);
        if (!p) continue;

        int finish_time = get_clk();
        int ta = finish_time - p->arrival_time;
        float wta = (float)ta / p->run_time;
        int wait = ta - p->run_time;

        turnaround_times[finished_processes] = ta;
        wta_list[finished_processes] = wta;
        wait_times[finished_processes] = wait;
        finished_processes++;
    }
}

void run_rr_scheduler() {
    if (queue_empty(&rr_queue)) return;

    Process p = queue_front(&rr_queue);
    queue_dequeue(&rr_queue);

    if (!p.started) {

        p.start_time = get_clk();
        if (start_time == -1) start_time = p.start_time;
        p.started = true;
    }

    kill(p.os_pid, SIGCONT);

    int slice = (p.remaining_time < quantum) ? p.remaining_time : quantum;

    for (int i = 0; i < slice; ++i) {
        sleep(1);
        p.remaining_time--;
        total_cpu_time++;
    }

    if (p.remaining_time > 0) {
        kill(p.os_pid, SIGSTOP);
        queue_enqueue(&rr_queue, p);
    }
}

void handle_process_arrival(Process new_process) {
    new_process.remaining_time = new_process.run_time;
    new_process.started = false;

    process_count++;
    printf("📥 Scheduler received Process %d (remaining time = %d)\n", new_process.id, new_process.remaining_time);


    pid_t pid = fork();
    if (pid == 0) {

        printf("RR: Forked and enqueued Process %d (PID %d)\n", new_process.id, pid);
        printf("📦 Process Count: %d\n", ++process_count);

        char id_str[10], runtime_str[10];
        sprintf(id_str, "%d", new_process.id);
        sprintf(runtime_str, "%d", new_process.run_time);
        execl("./process.out", "process.out", id_str, runtime_str, NULL);
        exit(1);
    }

    new_process.os_pid = pid;
    kill(pid, SIGSTOP);
    queue_enqueue(&rr_queue, new_process);
}

void get_message_ID(int *msgq_id, key_t *key) {
    *key = ftok("keyfile", 'A');
    *msgq_id = msgget(*key, IPC_CREAT | 0666);
}

void receive_new_process(int msgq_id) {
    if (msgq_id == -1) return;

    MsgBuffer message;
    while (msgrcv(msgq_id, (void *)&message, sizeof(message.process), 1, IPC_NOWAIT) != -1) {
        handle_process_arrival(message.process);
    }
}

void calculate_and_write_metrics() {
    float sum_wait = 0, sum_wta = 0, std_dev = 0;
    for (int i = 0; i < finished_processes; i++) {
        sum_wait += wait_times[i];
        sum_wta += wta_list[i];
    }
    float avg_wait = sum_wait / finished_processes;
    float avg_wta = sum_wta / finished_processes;
    for (int i = 0; i < finished_processes; i++) {
        std_dev += pow(wta_list[i] - avg_wta, 2);
    }
    std_dev = sqrt(std_dev / finished_processes);

    int end_time = get_clk();
    float cpu_util = ((float)total_cpu_time / (end_time - start_time)) * 100;

    FILE *f = fopen("scheduler.perf", "w");
    fprintf(f, "CPU Utilization = %.2f%%\n", cpu_util);
    fprintf(f, "Avg Waiting Time = %.2f\n", avg_wait);
    fprintf(f, "Avg WTA = %.2f\n", avg_wta);
    fprintf(f, "Std WTA = %.2f\n", std_dev);
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./scheduler RR <quantum>\n");
        return 1;
    }

    quantum = atoi(argv[2]);
    sync_clk();

    queue_init(&rr_queue, 20);
    signal(SIGCHLD, handle_sigchld);

    int msgq_id;
    key_t key;
    get_message_ID(&msgq_id, &key);

    while (1) {
        receive_new_process(msgq_id);
        printf("Scheduler tick at time %d\n", get_clk());
        printf("Current finished = %d, total = %d\n", finished_processes, process_count);


        if (start_time == -1 && !queue_empty(&rr_queue))
            start_time = get_clk();

        if (!queue_empty(&rr_queue))
            run_rr_scheduler();

        if (finished_processes == process_count) {
            calculate_and_write_metrics();
            break;
        }

        usleep(50000);
    }

    destroy_clk(0);
    return 0;
}
