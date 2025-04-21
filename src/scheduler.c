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

void calculate_and_write_metrics();

PCB* find_process_by_pid(pid_t pid) {
    for (int i = 0; i < rr_queue.size; i++) {
        size_t idx = (rr_queue.front + i) % rr_queue.capacity;
        if (rr_queue.data[idx].pid == pid) {
            return &rr_queue.data[idx];
        }
    }
    return NULL;
}
void context_switching(void);

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

void handle_sigchld(int signum) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        PCB* p = find_process_by_pid(pid); // Fix type here
        if (!p) continue;

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

void run_RR_Algorithm() {
    if (queue_empty(&rr_queue)) return;

    PCB p = queue_front(&rr_queue);
    queue_dequeue(&rr_queue);

    printf("RR: Running process %d (PID %d) with remaining time %d\n", 
           p.id_from_file, p.pid, p.remaining_time);

    if (p.start_time < 0) {
        p.start_time = get_clk();
        if (start_time == -1) start_time = p.start_time;
    }

    kill(p.pid, SIGCONT);

    int slice = (p.remaining_time < quantum) ? p.remaining_time : quantum;

    for (int i = 0; i < slice; ++i) {
        sleep(1);
        p.remaining_time--;
        total_cpu_time++;
    }

    if (p.remaining_time > 0) {
        printf("RR: Process %d (PID %d) stopped with remaining time %d\n", 
               p.id_from_file, p.pid, p.remaining_time);
        kill(p.pid, SIGSTOP);
        queue_enqueue(&rr_queue, p);
    } else {
        printf("RR: Process %d (PID %d) completed\n", p.id_from_file, p.pid);
        // Process has finished execution
        int finish_time = get_clk();
        int ta = finish_time - p.arrival_time;
        float wta = (float)ta / p.execution_time;
        int wait = ta - p.execution_time;

        turnaround_times[finished_processes] = ta;
        wta_list[finished_processes] = wta;
        wait_times[finished_processes] = wait;
        finished_processes++;
    }
}

void handle_process_arrival(PCB new_pcb) {
    process_count++;
    printf("📥 Scheduler received Process %d (remaining time = %d)\n", new_pcb.id_from_file, new_pcb.remaining_time);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        return;
    }
    
    if (pid == 0) { // Child process
        char id_str[10], runtime_str[10];
        sprintf(id_str, "%d", new_pcb.id_from_file);
        sprintf(runtime_str, "%d", new_pcb.execution_time);
        // printf("Child process %d starting with runtime %d\n", new_pcb.id_from_file, new_pcb.execution_time);
        execl("./bin/process.out", "process.out", id_str, runtime_str, NULL);
        
        // If execl fails, we reach here
        perror("execl failed");
        exit(1);
    } else { // Parent process
        // Store child's PID in PCB and then stop it until scheduled
        new_pcb.pid = pid;
        // printf("RR: Forked and enqueued Process %d (PID %d)\n", new_pcb.id_from_file, pid);
        // printf("📦 Process Count: %d\n", process_count);
        
        // Stop the child process immediately after creation
        kill(pid, SIGSTOP);
        
        // Now enqueue the PCB with the correct PID
        queue_enqueue(&rr_queue, new_pcb);
        insert_process_min_heap(&new_pcb);
    }
}

void run_HPF_Algorithm()
{
    PCB *to_run = extract_min();

    if (to_run == NULL)
        return;

    to_run->start_time = get_clk();

    // pid_t pid = fork();

    // if (pid == -1) { return; }
    // if (pid == 0) {
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

void run_SRTN_Algorithm() {
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
        if (exit_on_error) {
            exit(EXIT_FAILURE);
        }
    }
    
    return msgq_id;
}


// Context switching implementation
void context_switching()
{
    if (current_process != NULL && current_process->state == RUNNING)
    {
        current_process->state = READY;
        printf("Context switch from process %d\n", current_process->pid);
    }
    // if (ready_Queue != NULL) {
    //     current_process = ready_Queue;
    //     current_process->state = RUNNING;
    //     printf("Context switch to process %d\n", current_process->pid);
    // }
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


void receive_new_process(int msgq_id) {
    if (msgq_id == -1) return;

    MsgBuffer message;

    while (msgrcv(msgq_id, (void *) &message, sizeof(message.pcb), 1, IPC_NOWAIT) != -1) {
        handle_process_arrival(message.pcb);
        // printf("\nScheduler: Received.pcb runtime %d at arrival time %d\n",
        //        message.pcb.execution_time, message.pcb.arrival_time);
        // printf("\n====================================MIN HEAP====================================\n");
        // print_minheap();
    }
}


void run_algorithm(int algorithm) {
    if (algorithm == HPF)
        run_HPF_Algorithm();
    else if (algorithm == SRTN)
        run_SRTN_Algorithm();
    else if (algorithm == RR)
        run_RR_Algorithm();
}

int main(int argc, char *argv[]) {
    algorithm = atoi(argv[1]);
    if(argc>2){
        quantum = atoi(argv[2]); 
    }
    sync_clk();

    int msgq_id = handle_message_queue('A', IPC_CREAT | 0666, 1);

    signal(SIGCHLD, handle_sigchld); // Register signal handler for child termination
    
    queue_init(&rr_queue, 20); // Initialize the queue
    ready_Heap = create_min_heap(compare_priority);

    printf("Scheduler: Waiting for message...\n");
    while (1) {
        receive_new_process(msgq_id);
        run_algorithm(algorithm);
        usleep(50000);
        
        // Check if all processes have completed
        if (finished_processes > 0 && process_count == finished_processes) {
            printf("All processes have completed. Writing metrics...\n");
            calculate_and_write_metrics();
            break;
        }
    }
    
    destroy_clk(0);
    printf("Scheduler: finished...\n");
    return 0;
}
// void insert_process(PCB new_pcb, SchedulingAlgorithm algo);

// todo swap with insert in pcb

// void insert_ready(Node **head, PCB process)
// {
//     Node *new_node = (Node *)malloc(sizeof(Node));
//     new_node->process = process;
//     new_node->next = NULL;
//
//     printf("insert process with pid = %d\n", new_node->process.pid);
//
//     if (*head == NULL)
//     {
//         *head = new_node;
//     }
//     else
//     {
//         Node *temp = *head;
//         while (temp->next != NULL)
//             temp = temp->next;
//         temp->next = new_node;
//     }
//     process_Count++;
// }

// Fork and start a process

// // Start a process
// void start_process(int pid)
// {
//     Node *temp = ready_Queue;
//     while (temp != NULL)
//     {
//         if (temp->process.pid == pid)
//         {
//             temp->process.remaining_time--;
//             printf("process with pid= %d has started (remaining time: %d)\n",
//                    pid, temp->process.remaining_time);
//             return temp->process.remaining_time; // Return the updated value
//         }
//         temp = temp->next;
//     }
//     printf("process with pid= %d not found in ready queue\n", pid);
//     return -1;
// }

// void run_SRTN_Algorithm()
// {
//     // Create a new min heap for each run
//     MinHeap *mnHeap = create_min_heap();

//     // Only add non-terminated processes to the min-heap
//     Node *temp = ready_Queue;
//     while (temp != NULL)
//     {
//         if (temp->process.state != TERMINATED && temp->process.remaining_time > 0)
//         {
//             insert_process_min_heap(mnHeap, &(temp->process), temp->process.pid);
//         }
//         temp = temp->next;
//     }

//     if (mnHeap->size == 0)
//     {
//         // No processes to run
//         destroy_min_heap(mnHeap);
//         return;
//     }

//     printf("\nMin-Heap initial state:\n");
//     print_minheap(mnHeap);

//     PCB *next_process = extract_min(mnHeap);
//     if (next_process)
//     {
//         printf("Running process %d with remaining time %d\n",
//                next_process->pid, next_process->remaining_time);

//         // Actually execute the process for one time unit
//         next_process->remaining_time--;
//         printf("After execution: Process %d remaining time is now %d\n",
//                next_process->pid, next_process->remaining_time);

//         // Update the process in the ready queue
//         Node *ready_node = ready_Queue;
//         while (ready_node != NULL)
//         {
//             if (ready_node->process.pid == next_process->pid)
//             {
//                 ready_node->process.remaining_time = next_process->remaining_time;
//                 break;
//             }
//             ready_node = ready_node->next;
//         }

//         // Check if process is complete
//         if (next_process->remaining_time <= 0)
//         {
//             printf("Process %d completed!\n", next_process->pid);

//             // Update process state to TERMINATED
//             ready_node = ready_Queue;
//             while (ready_node != NULL)
//             {
//                 if (ready_node->process.pid == next_process->pid)
//                 {
//                     ready_node->process.state = TERMINATED;
//                     break;
//                 }
//                 ready_node = ready_node->next;
//             }

//             // Count how many processes are now completed
//             int processes_done = 0;
//             ready_node = ready_Queue;
//             while (ready_node != NULL)
//             {
//                 if (ready_node->process.state == TERMINATED)
//                 {
//                     processes_done++;
//                 }
//                 ready_node = ready_node->next;
//             }
//             printf("Processes completed: %d\n", processes_done);
//         }
//         else
//         {
//             // Re-insert the process with updated remaining time only if not completed
//             insert_process_min_heap(next_process);
//         }
//     }

//     destroy_min_heap(mnHeap);
// }
// void handle_process_arrival(PCB new_process)
// {

//     new_process.remaining_time = new_process.total_runtime;
//     new_process.waiting_time = 0;
//     new_process.state = READY;

//     insert_process(new_process, algorithm);
// }


// void handle_process_arrival(Process new_process)
// {
//     PCB new_pcb;

//     new_pcb.id_from_file = new_process.id;
//     new_pcb.arrival_time = new_process.arrival_time;
//     new_pcb.execution_time = new_process.execution_time;
//     new_pcb.priority = new_process.priority;
//     new_pcb.remaining_time = new_process.execution_time;
//     new_pcb.waiting_time = 0;
//     new_pcb.state = READY;

//     insert_process_min_heap(&new_pcb);
// }


// void insert_process(PCB new_pcb, SchedulingAlgorithm algo)
// {
//     Node *new_node = (Node *)malloc(sizeof(Node));
//     if (!new_node)
//     {
//         printf("Error: Memory allocation failed for new process node\n");
//         return;
//     }

//     new_node->process = new_pcb;
//     new_node->next = NULL;

//     if (ready_Queue == NULL)
//     {
//         ready_Queue = new_node;
//         return;
//     }

//     Node *current = ready_Queue;
//     Node *prev = NULL;

//     switch (algo)
//     {
//     case HPF:
//         while (current != NULL && current->process.priority <= new_pcb.priority)
//         {
//             prev = current;
//             current = current->next;
//         }
//         break;

//     case SRTN:
//         while (current != NULL && current->process.remaining_time <= new_pcb.remaining_time)
//         {
//             prev = current;
//             current = current->next;
//         }
//         break;

//     case RR:
//         while (current->next != NULL)
//         {
//             current = current->next;
//         }
//         current->next = new_node;
//         return;
//     }

//     if (prev == NULL)
//     {
//         new_node->next = ready_Queue;
//         ready_Queue = new_node;
//     }
//     else
//     {
//         new_node->next = current;
//         prev->next = new_node;
//     }
// }
