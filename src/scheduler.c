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

// TODO Itegerate with SRTN


//todo swap with insert in pcb

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
void create_process(PCB new_pcb) {
    // Assign sequential PID before insertion
    // new_pcb.pid = next_pid++;


    pid_t pid = fork();
    if (pid == 0) {
        new_pcb.pid = getpid();

        char *args[] = {"./process.o", NULL};
        execvp(args[0], args);
        exit(0); // Notify scheduler via SIGCHLD
    } else if (pid > 0) {
        // Parent
    } else {
    }
}

// preempt a process
void preempt_process() {
}

//todo swap with get process in pcb

PCB *get_process(int pid) {
    Node *temp = ready_Queue;
    while (temp != NULL) {
        if (temp->process.pid == pid) {
            return &(temp->process);
        }
        temp = temp->next;
    }
    return NULL;
}

// Resume a process
void resume_process() {
}

// Start a process
void start_process(int pid) {
    Node *temp = ready_Queue;
    while (temp != NULL) {
        if (temp->process.pid == pid) {
            temp->process.remaining_time--;
            printf("process with pid= %d has started (remaining time: %d)\n",
                   pid, temp->process.remaining_time);
            return;
        }
        temp = temp->next;
    }
    printf("process with pid= %d not found in ready queue\n", pid);
}

// void run_SRTN_Algorithm() {
//     int current_time = 0;
//     int processes_done = 0;
//     int remaining_processes = process_Count;
//
//
//     // MinHeap *mnHeap = create_min_heap(compare_remaining_time);
//
//     // Debug - print processes before insertion
//     printf("Processes before insertion to min-heap:\n");
//     Node *temp = ready_Queue;
//     while (temp != NULL) {
//         printf("Process %d: Arrival=%d, Remaining=%d\n",
//                temp->process.pid,
//                temp->process.arrival_time,
//                temp->process.remaining_time);
//         temp = temp->next;
//     }
//
//     // Convert linked list to min-heap
//     temp = ready_Queue;
//     while (temp != NULL) {
//         insert_process_min_heap( &(temp->process));
//         temp = temp->next;
//     }
//
//     // Debug - print the min-heap
//     printf("\nMin-Heap after insertion:\n");
//     print_minheap(mnHeap);
//
//     // Use remaining_processes instead of modifying process_count
//     while (remaining_processes > 0 && mnHeap->size > 0) {
//         PCB *next_process = extract_min(mnHeap);
//         if (next_process) {
//             printf("\nNext process chosen by SRTN: Process %d with remaining time %d\n",
//                    next_process->pid, next_process->remaining_time);
//
//             if (current_process != next_process) {
//                 // context_switching();
//                 current_process = next_process;
//             }
//
//             // Let start_process handle decrementing the remaining time
//             start_process(next_process->pid);
//
//             // Get the updated remaining time from the linked list
//             PCB *updated_process = get_process(next_process->pid);
//             if (updated_process) {
//                 next_process->remaining_time = updated_process->remaining_time;
//             }
//
//             if (next_process->remaining_time > 0) {
//                 // Re-insert only if process still has work to do
//                 insert_process_min_heap(next_process);
//             } else {
//                 // Process is complete
//                 printf("Process %d completed\n", next_process->pid);
//                 processes_done++;
//                 remaining_processes--;
//             }
//         } else {
//             // No process available
//             break;
//         }
//     }
//
//     printf("\nFinal state of Min-Heap:\n");
//     print_minheap(mnHeap);
//     printf("\nProcesses completed: %d\n", processes_done);
//
//     destroy_min_heap(mnHeap);
// }

void run_HPF_Algorithm() {
    PCB *to_run = extract_min();

    if (to_run == NULL) return;

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

void handle_process_arrival(Process new_process) {
    PCB new_pcb;

    new_pcb.id_from_file = new_process.id;
    new_pcb.arrival_time = new_process.arrival_time;
    new_pcb.execution_time = new_process.execution_time;
    new_pcb.priority = new_process.priority;
    new_pcb.remaining_time = new_process.execution_time;
    new_pcb.waiting_time = 0;
    new_pcb.state = READY;

    insert_process_min_heap(&new_pcb);
}

void get_message_ID(int *msgq_id, key_t *key) {
    *key = ftok("keyfile", 'A');
    *msgq_id = msgget(*key, IPC_CREAT | 0666);

    if (*msgq_id == -1) {
        perror("Error accessing message queue");
        // exit(EXIT_FAILURE);
    }
}

void receive_new_process(int msgq_id) {
    if (msgq_id == -1) return;

    MsgBuffer message;

    while (msgrcv(msgq_id, (void *) &message, sizeof(message.process), 1, IPC_NOWAIT) != -1) {
        handle_process_arrival(message.process);
        printf("\nScheduler: Received process runtime %d at arrival time %d\n",
               message.process.execution_time, message.process.arrival_time);
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
