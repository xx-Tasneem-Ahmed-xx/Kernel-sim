#include "scheduler.h"

#include "headers.h"

SchedulingAlgorithm algorithm;
// Node *ready_Queue = NULL;
// MinHeap *ready_Heap = NULL;
PCB *current_process = NULL;
pid_t current_child_pid = -1;

PCBQueue rr_queue;
int process_count = 0;

// int process_Count = 0;
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

void calculate_and_write_metrics();

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

void handle_sigchld(int signum)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        PCB *p = find_process_by_pid(pid); // Fix type here
        if (!p)
            continue;

        int finish_time = get_clk();
        int ta = finish_time - p->arrival_time;
        float wta = (float)ta / p->execution_time;
        int wait = ta - p->execution_time;

        turnaround_times[finished_processes] = ta;
        wta_list[finished_processes] = wta;
        wait_times[finished_processes] = wait;
        finished_processes++;
    }
}

// Add a signal handler for SIGUSR1 from the process generator
void handle_generator_completion(int signum)
{
    printf("Received signal from process generator: all processes sent.\n");
    generator_finished = 1;
}

void run_RR_Algorithm()
{
    if (rr_queue.size == 0)
        return;

    context_switching();

    printf("RR: Running process %d (PID %d) with remaining time %d\n",
           current_process->id_from_file, current_process->pid, current_process->remaining_time);

    int slice = (current_process->remaining_time < quantum) ? current_process->remaining_time : quantum;

    for (int i = 0; i < slice; ++i)
    {
        sleep(1);
        current_process->remaining_time--; // Shouldn't be decreasing here !
        total_cpu_time++;
    }

    if (current_process->remaining_time > 0)
    {
        printf("RR: Process %d (PID %d) stopped with remaining time %d\n",
               current_process->id_from_file, current_process->pid, current_process->remaining_time);
    }
    else
    {
        printf("RR: Process %d (PID %d) completed\n", current_process->id_from_file, current_process->pid);
        // Process has finished execution
        int finish_time = get_clk(); // done already
        int ta = finish_time - current_process->arrival_time;
        float wta = (float)ta / current_process->execution_time;
        int wait = ta - current_process->execution_time;

        turnaround_times[finished_processes] = ta;
        wta_list[finished_processes] = wta;
        wait_times[finished_processes] = wait;
        finished_processes++;
    }
}

// Fork and start a process
void handle_process_arrival(PCB *process)
{
    pid_t pid;
    process_count++;
    printf("📥 Scheduler received Process %d (remaining time = %d)\n", process->id_from_file, process->remaining_time);

    process->state = READY;

    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(-1);
    }

    if (pid == 0)
    {
        char id_str[10], runtime_str[10];
        sprintf(runtime_str, "%d", process->remaining_time);
        // printf("Child process %d starting with runtime %d\n", new_pcb.id_from_file, new_pcb.execution_time);
        execl("./bin/process.out", "process.out", runtime_str, NULL);
        
        // If execl fails, we reach here
        perror("execl failed");
        exit(1);
    }
    else
    {
        process->pid = pid;
        kill(pid, SIGTSTP);
        if (algorithm == RR)
            queue_enqueue(&rr_queue, *process);
        else
            insert_process_min_heap(process);
    }
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

    // to_run->pid = getpid();
    // to_run->state = RUNNING;
    // sleep(to_run->total_runtime);
    // PCB* to_run =extract_min();
    // to_run->remaining_time = 0;
    // to_run->finish_time = get_clk();
    // to_run->state = TERMINATED;
    // ?    }

    // waitpid(pid, NULL, 0);
    printf("finishing at time %d running process with id=%d remaining=%d\n", get_clk(), to_run->id_from_file,
           to_run->remaining_time);
}

void run_SRTN_Algorithm()
{
    // TODO: Implement SRTN scheduling
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

void terminate_process(int pid)
{
    Node *current = ready_Queue;
    Node *prev = NULL;

    while (current != NULL && current->process.pid != pid)
    {
        prev = current;
        current = current->next;
    }

    if (current == NULL)
    {
        printf("Process with PID %d not found in the ready queue.\n", pid);
        return;
    }
}

void calculate_and_write_metrics()
{
    float sum_wait = 0, sum_wta = 0, std_dev = 0;
    for (int i = 0; i < finished_processes; i++)
    {
        sum_wait += wait_times[i];
        sum_wta += wta_list[i];
    }
    float avg_wait = sum_wait / finished_processes;
    float avg_wta = sum_wta / finished_processes;
    for (int i = 0; i < finished_processes; i++)
    {
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

void receive_new_process(int msgq_id)
{
    if (msgq_id == -1)
        return;

    MsgBuffer message;

    while (msgrcv(msgq_id, (void *)&message, sizeof(message.pcb), 1, IPC_NOWAIT) != -1)
    {
        handle_process_arrival(&message.pcb);
        // printf("\nScheduler: Received.pcb runtime %d at arrival time %d\n",
        //        message.pcb.execution_time, message.pcb.arrival_time);
        // printf("\n====================================MIN HEAP====================================\n");
        // print_minheap();
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
    kill(process->pid, SIGCONT);
}
// resume a process
void resume_process(PCB *process)
{
    process->state = RUNNING;
    process->waiting_time += get_clk() - process->last_prempt_time;
    kill(process->pid, SIGCONT);
}

void preempt_process(PCB *process)
{
    process->state = READY;
    process->last_prempt_time = get_clk();
    kill(process->pid, SIGTSTP);
}

// Context switching implementation
void context_switching()
{
    PCB *new_process = NULL;

    // If there is no current process, nothing to preempt
    if (current_process != NULL)
    {
        // Only add to queue if the process hasn't completed
        if (current_process->remaining_time > 0)
        {
            printf("Remaining time for process enqueued = %d\n", current_process->remaining_time);
            if (algorithm == RR)
            {
                queue_enqueue(&rr_queue, *current_process);
            }
            else if (algorithm == SRTN)
            {
                insert_process_min_heap(current_process);
            }

            // Preempt the current process
            preempt_process(current_process);
        }

        // Don't free current_process yet - we'll replace it after getting a new one
    }

    // Get the next process to run
    if (algorithm == RR)
    {
        if (rr_queue.size == 0)
        {
            // No process to run
            current_process = NULL;
            return;
        }

        // Allocate memory for new process
        new_process = (PCB *)malloc(sizeof(PCB));
        if (!new_process)
        {
            perror("Failed to allocate memory for new process");
            return;
        }

        // Get front process from queue
        *new_process = queue_front(&rr_queue);
        queue_dequeue(&rr_queue);
    }
    else
    {
        new_process = (PCB *)malloc(sizeof(PCB));
        if (!new_process)
        {
            perror("Failed to allocate memory for new process");
            return;
        }

        if (!extract_min(new_process))
        {
            // No process available
            free(new_process);
            current_process = NULL;
            return;
        }
    }

    // Now we can safely free the old current process
    if (current_process != NULL)
    {
        free(current_process);
    }

    // Update current process to the new one
    current_process = new_process;

    if (current_process->start_time < 0)
    {
        start_process(current_process);
    }
    else
    {
        resume_process(current_process);
    }

    printf("Context switch to process %d\n", current_process->pid);
}

int main(int argc, char *argv[])
{
    algorithm = atoi(argv[1]);
    if (argc > 2)
    {
        quantum = atoi(argv[2]);
    }
    sync_clk();

    int msgq_id = handle_message_queue('A', IPC_CREAT | 0666, 1);

    signal(SIGCHLD, handle_sigchld);              // Register signal handler for child termination
    signal(SIGUSR1, handle_generator_completion); // Register signal handler for generator completion

    queue_init(&rr_queue, 20); // Initialize the queue
    ready_Heap = create_min_heap(compare_priority);

    printf("Scheduler: Waiting for message...\n");
    printf("Scheduler PID: %d\n", getpid()); // Print scheduler's PID for the generator to use

    while (1)
    {
        receive_new_process(msgq_id);
        run_algorithm(algorithm);
        usleep(50000);

        // Only terminate when all processes have completed AND the generator has finished
        if (finished_processes > 0 && process_count == finished_processes && generator_finished)
        {
            printf("All processes have completed and generator is done. Writing metrics...\n");
            calculate_and_write_metrics();
            break;
        }
    }

    destroy_clk(0);
    printf("Scheduler: finished...\n");
    return 0;
}