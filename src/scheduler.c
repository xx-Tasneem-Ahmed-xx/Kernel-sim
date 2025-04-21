#include "scheduler.h"

#include "headers.h"
#define MAX_PROCESSES 100

SchedulingAlgorithm algorithm;
// Node *ready_Queue = NULL;
// MinHeap *ready_Heap = NULL;
PCB *current_process = NULL;
pid_t current_child_pid = -1;

// int process_Count = 0;
int next_pid = 1;
int total_cpu_time = 0;
int start_time = -1;
int end_time = 0;

// void insert_process(PCB new_pcb, SchedulingAlgorithm algo);
void context_switching(void);

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
void create_process(PCB new_pcb)
{
    // Assign sequential PID before insertion
    // new_pcb.pid = next_pid++;

    pid_t pid = fork();
    if (pid == 0)
    {
        new_pcb.pid = getpid();

        char *args[] = {"./process.o", NULL};
        execvp(args[0], args);
        exit(0); // Notify scheduler via SIGCHLD
    }
    else if (pid > 0)
    {
        // Parent
    }
    else
    {
    }
}

// preempt a process
void preempt_process()
{
}

// todo swap with get process in pcb

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

// Resume a process
void resume_process()
{
}

// Start a process
void start_process(int pid)
{
    Node *temp = ready_Queue;
    while (temp != NULL)
    {
        if (temp->process.pid == pid)
        {
            temp->process.remaining_time--;
            printf("process with pid= %d has started (remaining time: %d)\n",
                   pid, temp->process.remaining_time);
            return temp->process.remaining_time; // Return the updated value
        }
        temp = temp->next;
    }
    printf("process with pid= %d not found in ready queue\n", pid);
    return -1;
}

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

void get_message_ID(int *msgq_id, key_t *key);

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

void get_message_ID(int *msgq_id, key_t *key)
{
    *key = ftok("keyfile", 'A');
    *msgq_id = msgget(*key, IPC_CREAT | 0666);

    if (*msgq_id == -1)
    {
        perror("Error accessing message queue");
        // exit(EXIT_FAILURE);
    }
}


int init_message_queue()
{
    int msgq_id;
    key_t key;
    key = ftok("keyfile", 'A');
    msgq_id = msgget(key, 0666);

    if (msgq_id == -1)
    {
        perror("Scheduler: Error accessing message queue");
        exit(EXIT_FAILURE);
    }

    return msgq_id;
}

// Initialize feedback message queue for terminated processes
int init_feedback_message_queue()
{
    int msgq_id;
    key_t key;
    key = ftok("keyfile", 'B'); // Using 'B' to differentiate from the main queue
    msgq_id = msgget(key, IPC_CREAT | 0666);

    if (msgq_id == -1)
    {
        perror("Scheduler: Error creating feedback message queue");
        exit(EXIT_FAILURE);
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

void receive_new_process(int msgq_id) {
    if (msgq_id == -1) return;

    MsgBuffer message;

    while (msgrcv(msgq_id, (void *) &message, sizeof(message.pcb), 1, IPC_NOWAIT) != -1) {
        insert_process_min_heap(&message.pcb);
        printf("\nScheduler: Received.pcb runtime %d at arrival time %d\n",
               message.pcb.execution_time, message.pcb.arrival_time);
        printf("\n====================================MIN HEAP====================================\n");
        print_minheap();

        // todo pick from min heap and run
        // create_process();
    }
}

// void run_algorithm(int algorithm) {
//     // if (algorithm == HPF)
//     //     run_HPF_Algorithm();
//
//     if (algorithm == SRTN)
//         run_SRTN_Algorithm();
//
//     // if (algorithm == RR)
//     // run_RR_Algorithm();
// }

int main(int argc, char *argv[]) {
    algorithm = atoi(argv[1]); // 1=HPF, 2=SRTN, 3=RR
    sync_clk();

    int msgq_id;
    key_t key;
    get_message_ID(&msgq_id, &key);

    ready_Heap = create_min_heap(compare_priority);

    printf("Scheduler: Waiting for message...\n");
    while (1) {
        receive_new_process(msgq_id);
        run_HPF_Algorithm();
        // run_algorithm(algorithm);
        usleep(50000);
    }
    destroy_clk(0);
    printf("Scheduler: finished...\n");


    return 0;
}