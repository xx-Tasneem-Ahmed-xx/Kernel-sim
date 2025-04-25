#include "scheduler.h"

#include "headers.h"
#include <signal.h>
#include <bits/sigaction.h>
#include <sys/shm.h>

SchedulingAlgorithm algorithm;
PCB *current_process = NULL;
pid_t current_child_pid = -1;

PCBQueue rr_queue;
int process_count = 0;

int next_pid = 1;
int total_cpu_time = 0;
int finished_processes = 0;
int start_time = -1;
int quantum = 0;

int turnaround_times[100];
float wta_list[100];
int wait_times[100];

// Add a flag to track if the process generator has finished sending processes
int generator_finished = 0;

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
        // For HPF and SRTN, search in the heap
        for (int i = 0; i < ready_Heap->size; i++) {
            if (ready_Heap->processes[i]->pid == pid) {
                return ready_Heap->processes[i];
            }
        }
    }

    return NULL;
}

PCB *get_process(int pid)
{
    Node *temp = ready_Queue;
    while (temp != NULL) {
        if (temp->process.pid == pid) {
            return &(temp->process);
        }
        temp = temp->next;
    }
    return NULL;
}

// Add a signal handler for SIGUSR1 from the process generator
void handle_generator_completion(int signum)
{
    printf("Received signal from process generator: all processes sent.\n");
    generator_finished = 1;
}

// Update process remaining time from shared memory
void update_process_remaining_time(PCB *process)
{
    if (process && process->shm_id != -1) {
        int *shm_remaining_time = (int *)shmat(process->shm_id, NULL, 0);
        if (shm_remaining_time != (int *)-1) {
            // Store the value, not the pointer
            if (process->remaining_time == NULL) {
                // Allocate memory for remaining_time if it doesn't exist
                process->remaining_time = (int *)malloc(sizeof(int));
            }
            *(process->remaining_time) = *shm_remaining_time;
            shmdt(shm_remaining_time);
        }
    }
}

void run_RR_Algorithm()
{
    // If no current process and there are processes in the queue, do context switch
    if (current_process == NULL && rr_queue.size > 0) {
        context_switching();
        return;
    }

    // If we have a current process, check its remaining time from shared memory
    if (current_process != NULL) {
        update_process_remaining_time(current_process);



        // Check if quantum has expired
        static int time_slice_start = -1;
        if (time_slice_start == -1) {
            time_slice_start = get_clk();
        }

        if (get_clk() - time_slice_start >= quantum) {
            // Quantum expired, reset the timer and do context switch
            time_slice_start = -1;
            context_switching();
        }
    }
}

void run_SRTN_Algorithm()
{
    if (current_process) {
        update_process_remaining_time(current_process);
    }

    // Check if there's a process with shorter remaining time
    if (ready_Heap->size > 0) {
        PCB *shortest = ready_Heap->processes[0]; // Peek the top without extracting

        if (current_process == NULL ||
            (*(current_process->remaining_time) > *(shortest->remaining_time))) {
            // Need to context switch
            context_switching();
        }
    }
}

void run_HPF_Algorithm()
{
    // If no current process and heap is not empty, extract the highest priority process
    if (current_process == NULL && ready_Heap->size > 0) {
        context_switching();
        return;
    }

    // HPF is non-preemptive, so we just let the current process run until completion
}
// Fork and start a process with shared memory for remaining time
void handle_process_arrival(PCB *process)
{
    pid_t pid;
    int shmid;
    int *shm_remaining_time;

    process_count++;
    process->remaining_time = (int *)malloc(sizeof(int));
    *(process->remaining_time) = process->execution_time;

    printf("📥 Scheduler received Process %d (remaining time = %d) and execution time = %d\n",
           process->id_from_file, *(process->remaining_time),process->execution_time);

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
    *shm_remaining_time = *(process->remaining_time);
    process->shm_id = shmid; // Store shared memory ID in the PCB

    // Detach from shared memory (scheduler will reattach when needed)
    shmdt(shm_remaining_time);

    process->state = READY;
    process->start_time = -1;
    process->last_prempt_time = -1;

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(-1);
    }

    if (pid == 0) {
        char runtime_str[10], shmid_str[20];
        sprintf(runtime_str, "%d", *(process->remaining_time)); // Convert process runtime to string
        sprintf(shmid_str, "%d", shmid); // Convert shared memory ID to string

        fflush(stdout);

        execl("./bin/process.out", "process.out", runtime_str, shmid_str, NULL);

        // If execl fails, we reach here
        perror("execl failed");
        exit(1);
    } else {
        process->pid = pid;
        kill(pid, SIGSTOP); // Stop the process initially

        // Add to appropriate data structure
        if (algorithm == RR) {
            queue_enqueue(&rr_queue, *process);
        } else {
            PCB *heap_process = (PCB *)malloc(sizeof(PCB));
            *heap_process = *process;
            insert_process_min_heap(heap_process);
        }

        printf("Scheduler: Process %d added to the queue\n", process->id_from_file);
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
        // Initialize shared memory ID to -1 (will be set in handle_process_arrival)
        message.pcb.shm_id = -1;
        handle_process_arrival(&message.pcb);

        // For SRTN, we might need to preempt the current process
        if (algorithm == SRTN && current_process != NULL) {
            run_SRTN_Algorithm();
        }
    }
}

void run_algorithm(int algorithm)
{
    if (algorithm == HPF)
        run_HPF_Algorithm();
    else if (algorithm == SRTN)
        run_SRTN_Algorithm();
    else if (algorithm == RR)
        run_RR_Algorithm();
}

// Start a process
void start_process(PCB *process)
{
    if (process == NULL) return;

    process->state = RUNNING;
    process->start_time = get_clk();
    if (start_time == -1)
        start_time = process->start_time;
    process->waiting_time = process->start_time - process->arrival_time;

    if (kill(process->pid, SIGCONT) < 0) {
        perror("Error starting process");
    } else {
        printf("Process %d started\n", process->id_from_file);
    }
    fflush(stdout);
}

// Resume a process
void resume_process(PCB *process)
{
    if (process == NULL) return;

    process->state = RUNNING;
    if (process->last_prempt_time != -1) {
        process->waiting_time += get_clk() - process->last_prempt_time;
    }
    printf("Resuming process %d\n", process->pid);

    if (kill(process->pid, SIGCONT) < 0) {
        perror("Error resuming process");
    }
}

void preempt_process(PCB *process)
{
    if (process == NULL) return;

    // Update remaining time from shared memory before preempting
    update_process_remaining_time(process);

    process->state = READY;
    process->last_prempt_time = get_clk();
    printf("Preempting process %d\n", process->pid);

    if (kill(process->pid, SIGSTOP) < 0) {
        perror("Error stopping process");
    }
}

void context_switching()
{
    PCB *new_process = NULL;

    // If there's a current process, preempt it
    if (current_process != NULL) {
        // Update current process remaining time from shared memory before preempting
        update_process_remaining_time(current_process);

        // If the remaining time is 0, allow the process to terminate naturally
        if (*(current_process->remaining_time) == 0) {
            printf("Process %d has finished execution. Waiting for termination...\n", current_process->pid);
            // Do not preempt or add the process back to the queue
            return;
        }

        // Preempt the current process if it's not terminated
        if (current_process->state != TERMINATED) {
            // why update remaining time here? 
            preempt_process(current_process);

            // Put it back in the appropriate data structure
            if (algorithm == RR) {
                queue_enqueue(&rr_queue, *current_process);
            } else {
                insert_process_min_heap(current_process);
            }
        }
    }

    printf("------------------\n");

    // Get the next process to run
    if (algorithm == RR) {
        if (rr_queue.size > 0) {
            // Create a new PCB for the next process
            new_process = (PCB *)malloc(sizeof(PCB));
            *new_process = queue_front(&rr_queue);
            queue_dequeue(&rr_queue);
        }
    } else {
        // For HPF and SRTN
        if (ready_Heap->size > 0) {
            new_process = extract_min();
        }
    }

    // Set the current process to the new one
    current_process = new_process;

    // Start or resume the new process if there is one
    if (current_process != NULL) {
        if (current_process->start_time == -1) {
            start_process(current_process);
        } else {
            resume_process(current_process);
        }
        printf("Context switch to process %d\n", current_process->id_from_file);
    }
}

void handle_sigchld(int signum)
{
    int status;
    pid_t pid;
    printf("SIGCHLD handler triggered\n");

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Child process with PID %d terminated\n", pid);

        // Find the process that terminated
        PCB *terminated_process = NULL;

        // Check if it's the current process
        if (current_process && current_process->pid == pid) {
            terminated_process = current_process;
        } else {
            // Search for it in our data structures
            terminated_process = find_process_by_pid(pid);
        }

        if (terminated_process) {
            // Clean up shared memory
            if (terminated_process->shm_id != -1) {
                shmctl(terminated_process->shm_id, IPC_RMID, NULL);
                terminated_process->shm_id = -1;
            }

            terminated_process->state = TERMINATED;

            int finish_time = get_clk();
            int ta = finish_time - terminated_process->arrival_time;
            float wta = (float)ta / terminated_process->execution_time;
            int wait = ta - terminated_process->execution_time;

            turnaround_times[finished_processes] = ta;
            wta_list[finished_processes] = wta;
            wait_times[finished_processes] = wait;
            finished_processes++;

            printf("-------------Received termination signal. Terminating process %d...\n", pid);

            // If this is the current process, free it and set current_process to NULL
            if (current_process && current_process->pid == pid) {
                free(current_process->remaining_time);
                free(current_process);
                current_process = NULL;

                // Try to schedule the next process if generator is finished and using SRTN
                if (generator_finished && algorithm == SRTN) {
                    context_switching();
                }
            } else {
                // Free the remaining time if allocated
                if (terminated_process->remaining_time) {
                    free(terminated_process->remaining_time);
                }
                // Note: we don't free the process itself if it's not the current process
                // as it's part of a data structure that will handle freeing it
            }
        }
    }
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;               // Set the signal handler
    sa.sa_flags = SA_NOCLDSTOP;                   // Prevent SIGCHLD for stopped children
    sigemptyset(&sa.sa_mask);                     // No additional signals to block
    sigaction(SIGCHLD, &sa, NULL);                // Register the signal handler
    signal(SIGUSR1, handle_generator_completion); // Register signal handler for generator completion

    if (argc < 2) {
        printf("Usage: %s <algorithm> [quantum]\n", argv[0]);
        exit(1);
    }

    algorithm = atoi(argv[1]);
    if (algorithm == SRTN) {
        ready_Heap = create_min_heap(compare_remaining_time);
    } else if (algorithm == HPF) {
        ready_Heap = create_min_heap(compare_priority);
    } else if (algorithm == RR) {
        if (argc < 3) {
            printf("RR algorithm requires quantum value\n");
            exit(1);
        }
        quantum = atoi(argv[2]);
        queue_init(&rr_queue, 100); // Initialize with larger capacity
    } else {
        printf("Invalid algorithm selection\n");
        exit(1);
    }
    
    // why even assign quantum when it's not RR?
    if (argc > 2 && algorithm != RR) {
        quantum = atoi(argv[2]);
    }
    sync_clk();

    int msgq_id = handle_message_queue('A', IPC_CREAT | 0666, 1);

    printf("Scheduler: Waiting for message...\n");
    printf("Scheduler PID: %d\n", getpid()); // Print scheduler's PID for the generator to use

    while (1) {
        receive_new_process(msgq_id);

        // Run the appropriate scheduling algorithm
        run_algorithm(algorithm);

        // Check if all processes are finished
        if (generator_finished &&
            (algorithm == RR ? rr_queue.size == 0 : ready_Heap->size == 0) &&
            current_process == NULL) {
            printf("Scheduler: All processes finished.\n");
            break;
        }
    }

    // Clean up and print statistics
    // printf("\n===== Scheduling Statistics =====\n");
    // printf("Total processes: %d\n", process_count);
    // printf("Finished processes: %d\n", finished_processes);
    //
    // float avg_ta = 0, avg_wta = 0, avg_wait = 0;
    // for (int i = 0; i < finished_processes; i++) {
    //     avg_ta += turnaround_times[i];
    //     avg_wta += wta_list[i];
    //     avg_wait += wait_times[i];
    // }
    //
    // if (finished_processes > 0) {
    //     avg_ta /= finished_processes;
    //     avg_wta /= finished_processes;
    //     avg_wait /= finished_processes;
    // }
    //
    // printf("Average turnaround time: %.2f\n", avg_ta);
    // printf("Average weighted turnaround time: %.2f\n", avg_wta);
    // printf("Average waiting time: %.2f\n", avg_wait);
    //
    destroy_clk(0);
    printf("Scheduler: finished...\n");
    return 0;
}