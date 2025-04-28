#include "scheduler.h"
#include "headers.h"
#include <signal.h>
#include <bits/sigaction.h>
#include <sys/shm.h>
#include <math.h>

SchedulingAlgorithm algorithm;
PCB *current_process = NULL;
PCBQueue rr_queue;
int process_count = 0;
int finished_processes = 0;
int start_time = -1;
int quantum = 0;
int generator_finished = 0;
int active_cpu_time = 0; // Time the CPU is actively running processes

int turnaround_times[100];
float wta_list[100];
int wait_times[100];
int time_slice_start = -1;

int semid; // Semaphore ID to sync with process generator

void log_process_event(const char *state, PCB *process, int finish_time) {
    FILE *log_file = fopen("scheduler.log", "a");
    if (!log_file) {
        perror("Failed to open scheduler.log");
        return;
    }

    int clk = get_clk();
    fprintf(log_file, "At time %d process %d %s arr %d total %d remain %d wait %d",
            clk, process->id_from_file, state, process->arrival_time,
            process->execution_time, process->remaining_time, process->waiting_time);

    if (strcmp(state, "finished") == 0) {
        int ta = finish_time - process->arrival_time;
        float wta = roundf(((float)ta / process->execution_time) * 100) / 100;
        fprintf(log_file, " TA %d WTA %.2f", ta, wta);
    }

    fprintf(log_file, "\n");
    fclose(log_file);
}

PCB *find_process_by_pid(pid_t pid)
{
    if (algorithm == RR) {
        for (int i = 0; i < rr_queue.size; i++) {
            size_t idx = (rr_queue.front + i) % rr_queue.capacity;
            if (rr_queue.data[idx].pid == pid) {
                return &rr_queue.data[idx];
            }
        }
    } else {
        for (int i = 0; i < ready_Heap->size; i++) {
            if (ready_Heap->processes[i]->pid == pid) {
                return ready_Heap->processes[i];
            }
        }
    }
    return NULL;
}

void handle_generator_completion(int signum)
{
    log_message(LOG_SYSTEM, "All processes received from generator");
    generator_finished = 1;
}

void update_process_remaining_time(PCB *process)
{
    if (process && process->shm_id != -1) {
        int *shm_remaining_time = (int *)shmat(process->shm_id, NULL, 0);
        if (shm_remaining_time != (int *)-1) {
            process->remaining_time = *shm_remaining_time;
            shmdt(shm_remaining_time);
        }
    }
}

void run_RR_Algorithm()
{
    if (current_process == NULL && rr_queue.size > 0) {
       
        printf("context switching\n in rr process = null");
        context_switching();
        // return;
    }

    if (current_process != NULL) {
        update_process_remaining_time(current_process);


        if (get_clk() - time_slice_start >= quantum && rr_queue.size > 0) {
            printf("i am context switch RR2\n");
            context_switching();
        }
    }
}

void run_SRTN_Algorithm()
{
    if (current_process) {
        update_process_remaining_time(current_process);
    }

    if (ready_Heap->size > 0) {
        PCB *shortest = ready_Heap->processes[0];
        if (current_process == NULL ||
            (current_process->remaining_time > shortest->remaining_time)) {
            context_switching();
        }
    }
}

void run_HPF_Algorithm()
{
    if (current_process == NULL && ready_Heap->size > 0) {
        context_switching();
    }
}

void handle_process_arrival(PCB *process)
{
    process_count++;
    process->remaining_time = process->execution_time;

    log_message(LOG_PROCESS, "Received Process %d (Runtime: %d, Priority: %d)",
               process->id_from_file, process->execution_time, process->priority);

    process->state = READY;
    process->start_time = -1;
    process->last_prempt_time = -1;

    if (algorithm == RR) {
        queue_enqueue(&rr_queue, *process);
    } else {
        PCB *heap_process = (PCB *)malloc(sizeof(PCB));
        *heap_process = *process;
        insert_process_min_heap(heap_process);
    }
}

int handle_message_queue(char key_char, int flags, int exit_on_error)
{
    int msgq_id;
    key_t key;

    key = ftok("keyfile", key_char);
    msgq_id = msgget(key, flags);

    if (msgq_id == -1) {
        perror("Error handling message queue");
        if (exit_on_error) {
            exit(EXIT_FAILURE);
        }
    }
    return msgq_id;
}

void receive_new_process(int msgq_id)
{
    if (msgq_id == -1)
        return;

    MsgBuffer message;
    while (msgrcv(msgq_id, (void *)&message, sizeof(message.pcb), 1, IPC_NOWAIT) != -1) {
        handle_process_arrival(&message.pcb);
        if (algorithm == SRTN && current_process != NULL) {
            run_SRTN_Algorithm();
        }
    }
}

void run_algorithm(int algorithm)
{
    // Try to check if semaphore is available (value > 0)
    int sem_val = semctl(semid, 0, GETVAL);
    // printf("Semaphore value: %d\n",sem_val);
    if (sem_val <= 0) {
        // Semaphore is unavailable (0), process generator is sending processes
        // Don't run algorithm, just return
        return;
    }

    // Try to down the semaphore (non-blocking)
    int result = down_nb(semid);
    if (result < 0) {
        // Failed to acquire semaphore
        return;
    }
    
    // Run the algorithm as normal
    if (algorithm == HPF)
        run_HPF_Algorithm();
    else if (algorithm == SRTN)
        run_SRTN_Algorithm();
    else if (algorithm == RR)
        run_RR_Algorithm();
    
    // Release the semaphore
    up(semid);
}

void start_process(PCB *process)
{
    if (process == NULL) return;

    process->state = RUNNING;
    process->start_time = get_clk();
    if (start_time == -1)
        start_time = process->start_time;
    // Increment active CPU time
    active_cpu_time += process->execution_time;

    process->waiting_time = get_clk() - process->arrival_time;

    if (kill(process->pid, SIGCONT) < 0) {
        log_message(LOG_ERROR, "Failed to start process %d", process->pid);
    } else {
        log_process_state(process, "STARTED");
        log_process_event("started", process, -1);
    }
    time_slice_start = get_clk();
}

void resume_process(PCB *process)
{
    if (process == NULL) return;

    process->state = RUNNING;
    if (process->last_prempt_time != -1) {
        process->waiting_time += get_clk() - process->last_prempt_time;
    }

    if (kill(process->pid, SIGCONT) < 0) {
        log_message(LOG_ERROR, "Failed to resume process %d", process->pid);
    } else {
        log_process_state(process, "RESUMED");
        log_process_event("resumed", process, -1);
    }
    time_slice_start = get_clk();
}

void preempt_process(PCB *process)
{
    if (process == NULL) return;

    update_process_remaining_time(process);
    process->state = READY;
    process->last_prempt_time = get_clk();

    if (kill(process->pid, SIGSTOP) < 0) {
        log_message(LOG_ERROR, "Failed to preempt process %d", process->pid);
    } else {
        log_process_state(process, "PREEMPTED");
        log_process_event("stopped", process, -1);
        
        // Print progress bar for the preempted process
        printf("%sProcess %d Progress (Preempted):%s\n", COLOR_GREEN, process->pid, COLOR_RESET);
        print_progress_bar(process->execution_time - process->remaining_time, 
                          process->execution_time, 20);
    }
}

void context_switching()
{
    PCB *new_process = NULL;

    if (current_process != NULL) {
        update_process_remaining_time(current_process);
        if (current_process->remaining_time == 0) {
            return;
        }

        if (current_process->state != TERMINATED) {
            preempt_process(current_process);
            if (algorithm == RR) {
                queue_enqueue(&rr_queue, *current_process);
            } else {
                insert_process_min_heap(current_process);
            }
        }
    }

    log_message(LOG_SYSTEM, "Performing context switch at time %d", get_clk());

    if (algorithm == RR) {
        if (rr_queue.size > 0) {
            new_process = (PCB *)malloc(sizeof(PCB));
            *new_process = queue_front(&rr_queue);
            queue_dequeue(&rr_queue);
        }
    } else {
        if (ready_Heap->size > 0) {
            new_process = extract_min();
        }
    }

    current_process = new_process;

    if (current_process != NULL) {
        if (current_process->start_time == -1) {
            start_process(current_process);
        } else {
            resume_process(current_process);
        }
    } else {
        log_message(LOG_SYSTEM, "No process to schedule, CPU idle");
    }
}

void handle_process_completion(int signum)
{
    if (!current_process) {
        log_message(LOG_ERROR, "Received SIGUSR2 but no current process");
        return;
    }

    PCB *terminated_process = current_process;
    update_process_remaining_time(terminated_process);

    if (terminated_process->shm_id != -1) {
        shmctl(terminated_process->shm_id, IPC_RMID, NULL);
        terminated_process->shm_id = -1;
    }

    terminated_process->state = TERMINATED;

    int finish_time = get_clk();
    int ta = finish_time - terminated_process->arrival_time;
    float wta = roundf(((float)ta / terminated_process->execution_time) * 100) / 100;
    int wait = ta - terminated_process->execution_time;

    turnaround_times[finished_processes] = ta;
    wta_list[finished_processes] = wta;
    wait_times[finished_processes] = wait;

    log_process_state(terminated_process, "FINISHED");
    log_message(LOG_STAT, "Turnaround: %d, Weighted TA: %.2f, Wait: %d",
                ta, wta, wait);

    log_process_event("finished", terminated_process, finish_time);

    finished_processes++;
    printf("%sOverall Process Completion:%s\n", COLOR_GREEN, COLOR_RESET);
    print_progress_bar(finished_processes, process_count, 20);

    free(current_process);
    current_process = NULL;

    if (generator_finished && algorithm == SRTN) {
        context_switching();
    }
}

// void handle_sigchld(int signum)
// {
//     int status;
//     pid_t pid;

//     while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
//         PCB *terminated_process = find_process_by_pid(pid);

//         if (terminated_process && terminated_process->state != TERMINATED) {
//             if (terminated_process->shm_id != -1) {
//                 shmctl(terminated_process->shm_id, IPC_RMID, NULL);
//                 terminated_process->shm_id = -1;
//             }

//             terminated_process->state = TERMINATED;

//             int finish_time = get_clk();
//             int ta = finish_time - terminated_process->arrival_time;
//             float wta = roundf(((float)ta / terminated_process->execution_time) * 100) / 100;
//             int wait = ta - terminated_process->execution_time;

//             turnaround_times[finished_processes] = ta;
//             wta_list[finished_processes] = wta;
//             wait_times[finished_processes] = wait;

//             log_process_state(terminated_process, "FINISHED");
//             log_message(LOG_STAT, "Turnaround: %d, Weighted TA: %.2f, Wait: %d",
//                         ta, wta, wait);

//             log_process_event("finished", terminated_process, finish_time);

//             finished_processes++;
//             printf("%sOverall Process Completion:%s\n", COLOR_GREEN, COLOR_RESET);
//             print_progress_bar(finished_processes, process_count, 20);

//             if (current_process && current_process->pid == pid) {
//                 free(current_process);
//                 current_process = NULL;

//                 if (generator_finished && algorithm == SRTN) {
//                     context_switching();
//                 }
//             }
//         }
//     }
// }

int main(int argc, char *argv[])
{
    FILE *log_file = fopen("scheduler.log", "w");
    if (log_file) {
        fclose(log_file);
    }

    // struct sigaction sa_child;
    // sa_child.sa_handler = handle_process_completion;
    // sa_child.sa_flags = SA_NOCLDSTOP;
    // sigemptyset(&sa_child.sa_mask);
    // sigaction(SIGCHLD, &sa_child, NULL);

    signal(SIGUSR2, handle_process_completion);
    signal(SIGUSR1, handle_generator_completion);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <algorithm> [quantum] <semaphore_id>\n", argv[0]);
        exit(1);
    }

    algorithm = atoi(argv[1]);
    const char* algorithm_name = "";

    if (algorithm == SRTN) {
        algorithm_name = "Shortest Remaining Time Next";
        ready_Heap = create_min_heap(compare_remaining_time);
    } else if (algorithm == HPF) {
        algorithm_name = "Highest Priority First";
        ready_Heap = create_min_heap(compare_priority);
    } else if (algorithm == RR) {
        algorithm_name = "Round Robin";
        if (argc < 4) {
            log_message(LOG_ERROR, "RR algorithm requires quantum value and semaphore ID");
            exit(1);
        }
        quantum = atoi(argv[3]);
        
        queue_init(&rr_queue, 100);
    } else {
        if (argc < 3) {
            log_message(LOG_ERROR, "Missing semaphore ID argument");
            exit(1);
        }
    }
    semid = atoi(argv[2]);
    printf("semaphore id in scheduler: %d\n", semid);

    print_divider("Scheduler Started");
    log_message(LOG_SYSTEM, "Algorithm: %s", algorithm_name);
    if (algorithm == RR) {
        log_message(LOG_SYSTEM, "Quantum: %d", quantum);
    }

    sync_clk();

    int msgq_id = handle_message_queue('A', IPC_CREAT | 0666, 1);

    while (1) {
        receive_new_process(msgq_id);
        
        run_algorithm(algorithm);

        if (generator_finished &&
            (algorithm == RR ? rr_queue.size == 0 : ready_Heap->size == 0) &&
            current_process == NULL) {
            break;
        }
    }

    print_divider("Scheduler Statistics");

    int total_time = get_clk() - start_time;
    float cpu_util = total_time > 0 ? ((float)active_cpu_time / total_time) * 100 : 0;

    float avg_ta = 0, avg_wta = 0, avg_wait = 0, std_wta = 0;

    for (int i = 0; i < finished_processes; i++) {
        avg_ta += turnaround_times[i];
        avg_wta += wta_list[i];
        avg_wait += wait_times[i];
    }

    if (finished_processes > 0) {
        avg_ta /= finished_processes;
        avg_wta /= finished_processes;
        avg_wait /= finished_processes;

        for (int i = 0; i < finished_processes; i++) {
            std_wta += pow(wta_list[i] - avg_wta, 2);
        }
        std_wta = sqrt(std_wta / finished_processes);
    }

    log_message(LOG_STAT, "Total processes: %d", process_count);
    log_message(LOG_STAT, "CPU utilization: %.2f%%", cpu_util);
    log_message(LOG_STAT, "Avg turnaround time: %.2f", avg_ta);
    log_message(LOG_STAT, "Avg weighted turnaround time: %.2f", avg_wta);
    log_message(LOG_STAT, "Std weighted turnaround time: %.2f", std_wta);
    log_message(LOG_STAT, "Avg waiting time: %.2f", avg_wait);

    FILE* perf_file = fopen("scheduler.perf", "w");
    if (perf_file) {
        fprintf(perf_file, "CPU utilization = %.2f%%\n", cpu_util);
        fprintf(perf_file, "Avg WTA = %.2f\n", avg_wta);
        fprintf(perf_file, "Avg Waiting = %.2f\n", avg_wait);
        fprintf(perf_file, "Std WTA = %.2f\n", std_wta);
        fclose(perf_file);
    }

    print_divider("Simulation Complete");

    destroy_clk(0);
    return 0;
}