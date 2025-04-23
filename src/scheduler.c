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

int turnaround_times[100]; //??
float wta_list[100];       //??
int wait_times[100];       //??

// Add a flag to track if the process generator has finished sending processes
int generator_finished = 0;

PCB *find_process_by_pid(pid_t pid)
{
    for (int i = 0; i < rr_queue.size; i++)
    {
        size_t idx = (rr_queue.front + i) % rr_queue.capacity;
        if (rr_queue.data[idx].pid == pid)
        {
            return &rr_queue.data[idx];
        }
    }
    return NULL;
}

PCB *get_process(int pid)
{
    Node *temp = ready_Queue;
    while (temp != NULL)
    {
        if (temp->process.pid == pid)
        {
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
    if (process && process->shm_id != -1)
    {
        int *shm_remaining_time = (int *)shmat(process->shm_id, NULL, 0);
        if (shm_remaining_time != (int *)-1)
        {
            process->remaining_time = *shm_remaining_time;
            shmdt(shm_remaining_time);
        }
    }
}

// void run_RR_Algorithm()
// {
//     if (rr_queue.size == 0)
//         return;

//     context_switching();

//     // Update remaining time from shared memory
//     update_process_remaining_time(current_process);

//     printf("RR: Running process %d (PID %d) with remaining time %d\n",
//            current_process->id_from_file, current_process->pid, current_process->remaining_time);

//     int slice = (current_process->remaining_time < quantum) ? current_process->remaining_time : quantum;

//     // We don't need to decrement the remaining time here since
//     // the process will update the shared memory as it runs
// }

void run_SRTN_Algorithm()
{

    if (current_process)
        update_process_remaining_time(current_process);

    // int currTime = get_clk();
    // int preTime = currTime;

    // if (currTime != preTime)
    // {
    //     preTime = currTime;
    //     context_switching();
    //     print_minheap();
    // }
    // currTime = get_clk();
}

// Fork and start a process with shared memory for remaining time
void handle_process_arrival(PCB *process)
{
    pid_t pid;
    int shmid;
    int *shm_remaining_time;

    process_count++;
    printf("📥 Scheduler received Process %d (remaining time = %d)\n", process->id_from_file, process->remaining_time);

    // Create shared memory for remaining time
    shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("shmget failed");
        exit(-1);
    }

    // Attach shared memory
    shm_remaining_time = (int *)shmat(shmid, NULL, 0);
    if (shm_remaining_time == (int *)-1)
    {
        perror("shmat failed");
        exit(-1);
    }

    // Initialize shared memory with the process remaining time
    *shm_remaining_time = process->remaining_time;
    process->shm_id = shmid; // Store shared memory ID in the PCB

    // Detach from shared memory (scheduler will reattach when needed)
    shmdt(shm_remaining_time);

    process->state = READY;

    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(-1);
    }

    if (pid == 0)
    {
        char id_str[10], runtime_str[10], shmid_str[20];
        sprintf(runtime_str, "%d", process->remaining_time);
        sprintf(shmid_str, "%d", shmid); // Convert shared memory ID to string

        fflush(stdout);

        execl("./bin/process.out", "process.out", runtime_str, shmid_str, NULL);

        // If execl fails, we reach here
        perror("execl failed");
        exit(1);
    }
    else
    {
        process->pid = pid;
        // sleep(1); // Give the child process time to start
        kill(pid, SIGTSTP);
    }
    if (algorithm == RR)
        queue_enqueue(&rr_queue, *process);
    else
        insert_process_min_heap(process);

    printf("Scheduler: Process %d added to the queue\n", process->pid);
}

void run_HPF_Algorithm()
{
    PCB *to_run = extract_min();

    if (to_run == NULL)
        return;

    to_run->start_time = get_clk();

    printf("\nstarting at time =%d running process with id=%d runtime=%d priority=%d\n", get_clk(),
           to_run->id_from_file,
           to_run->execution_time, to_run->priority);

    // Update remaining time from shared memory before checking/deciding
    update_process_remaining_time(to_run);

    printf("finishing at time %d running process with id=%d remaining=%d\n", get_clk(), to_run->id_from_file,
           to_run->remaining_time);
}

int handle_message_queue(char key_char, int flags, int exit_on_error)
{
    int msgq_id;
    key_t key;

    key = ftok("keyfile", key_char);
    msgq_id = msgget(key, flags);

    if (msgq_id == -1)
    {
        perror("Error handling message queue");
        if (exit_on_error)
        {
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

    while (msgrcv(msgq_id, (void *)&message, sizeof(message.pcb), 1, IPC_NOWAIT) != -1)
    {
        // Initialize shared memory ID to -1 (will be set in handle_process_arrival)
        message.pcb.shm_id = -1;
        handle_process_arrival(&message.pcb);
        if (algorithm == SRTN)
            context_switching();
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
    process->state = RUNNING;
    process->start_time = get_clk();
    if (start_time == -1)
        start_time = process->start_time;
    process->waiting_time = get_clk() - process->arrival_time;
    if (kill(process->pid, SIGCONT))
        printf("error in starting process %d\n", process->pid);
    else
        printf("Process %d started\n", process->pid);
    fflush(stdout);
    // usleep(50000); // Sleep for 50ms to simulate process execution
}

// Resume a process
void resume_process(PCB *process)
{
    process->state = RUNNING;
    process->waiting_time += get_clk() - process->last_prempt_time;
    printf("Resuming process %d\n", process->pid);

    kill(process->pid, SIGCONT);
}

void preempt_process(PCB *process)
{
    // Update remaining time from shared memory before preempting
    update_process_remaining_time(process);

    process->state = READY;
    process->last_prempt_time = get_clk();
    printf("Preempting process %d\n", process->pid);
    kill(process->pid, SIGTSTP);
}

void context_switching()
{

    if (current_process == NULL)
    {
        // No current process, just start the new one
        if (algorithm == RR)
        {

            *current_process = queue_front(&rr_queue);
            queue_dequeue(&rr_queue);
        }
        else
        {
            current_process = extract_min();
        }
        if (current_process->start_time == -1)
            start_process(current_process);
        else
            resume_process(current_process);
        return;
    }

    // Update current process remaining time from shared memory before preempting
    update_process_remaining_time(current_process);

    PCB *new_process = NULL;
    // Preempt the current process
    preempt_process(current_process);

    if (current_process->state != TERMINATED)
        if (algorithm == RR)
        {
            queue_enqueue(&rr_queue, *current_process);
        }
        else
        {
            insert_process_min_heap(current_process);
        }

    printf("------------------\n");

    // while (new_process->state == TERMINATED && ready_Heap->size > 0)
    // {
    //     printf("heap size: %d\n", ready_Heap->size);
    //     new_process = extract_min();
    // }

    if (algorithm == RR)
    {
        if (rr_queue.size > 0)
        {
            *new_process = queue_front(&rr_queue);
            queue_dequeue(&rr_queue);
        }
    }
    else
    {
        new_process = extract_min();
    }

    current_process = new_process;
    if (current_process->start_time == -1)
        start_process(current_process);
    else
    {
        resume_process(current_process);
    }
    printf("Context switch to process %d\n", current_process->pid);
}

void handle_sigchld(int signum)
{
    int status;
    pid_t pid;
    printf("SIGCHLD handler triggered\n");
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Child process with PID %d terminated\n", pid);

        // Clean up shared memory
        if (current_process->shm_id != -1)
        {
            shmctl(current_process->shm_id, IPC_RMID, NULL);
            current_process->shm_id = -1;
        }
        current_process->state = TERMINATED;

        int finish_time = get_clk();
        int ta = finish_time - current_process->arrival_time;
        float wta = (float)ta / current_process->execution_time;
        int wait = ta - current_process->execution_time;

        turnaround_times[finished_processes] = ta;
        wta_list[finished_processes] = wta;
        wait_times[finished_processes] = wait;
        finished_processes++;
        // if (generator_finished)
        //     context_switching();

        printf("-------------Received termination signal. Terminating process %d...\n", pid);

        // If this is the current process, set current_process to NULL
        if (current_process && current_process->pid == pid)
        {
            free(current_process);
            current_process = NULL;
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

    algorithm = atoi(argv[1]);
    if (algorithm == SRTN)
    {
        ready_Heap = create_min_heap(compare_remaining_time);
    }
    else if (algorithm == HPF)
    {
        ready_Heap = create_min_heap(compare_priority);
    }

    if (argc > 2)
    {
        quantum = atoi(argv[2]);
    }
    sync_clk();

    int msgq_id = handle_message_queue('A', IPC_CREAT | 0666, 1);

    queue_init(&rr_queue, 20); // Initialize the queue

    printf("Scheduler: Waiting for message...\n");
    printf("Scheduler PID: %d\n", getpid()); // Print scheduler's PID for the generator to use

    while (1)
    {
        receive_new_process(msgq_id);

        // If we have a current process, update its remaining time from shared memory
        if (current_process)
        {
            update_process_remaining_time(current_process);
        }

        run_algorithm(algorithm);
        // usleep(50000);

        if (generator_finished && rr_queue.size == 0 && (ready_Heap->size == 0) && current_process == NULL)
        {
            printf("Scheduler: All processes finished.\n");
            break;
        }
        if (generator_finished)
        {
            // printf("QUEUE SIZE: %d\n", rr_queue.size);
            // printf("HEAP SIZE: %d\n", ready_Heap->size);
        }
    }

    destroy_clk(0);
    printf("Scheduler: finished...\n");
    return 0;
}

// Modified run_RR_Algorithm function
void run_RR_Algorithm()
{
    // If no current process and there are processes in the queue, do context switch
    if (current_process == NULL && rr_queue.size > 0)
    {
        context_switching();
        return;
    }

    // If we have a current process, check its remaining time from shared memory
    if (current_process != NULL)
    {
        update_process_remaining_time(current_process);

        // If process has finished, handle completion
        if (current_process->remaining_time <= 0)
        {
            printf("Process %d (PID %d) completed execution\n",
                   current_process->id_from_file, current_process->pid);

            // Clean up shared memory
            if (current_process->shm_id != -1)
            {
                shmctl(current_process->shm_id, IPC_RMID, NULL);
            }

            free(current_process);
            current_process = NULL;

            // Try to context switch to next process
            context_switching();
            return;
        }

        // Check if quantum has expired (handle in context_switching)
        context_switching();
    }
}
