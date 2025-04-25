#include "headers.h"
#include "pcb.h"
Node *ready_Queue = NULL;
int process_Count = 0;
const char* ProcessStateNames[] = {
    "READY",
    "RUNNING",
    "STOPPED",
    "TERMINATED"
};
// void insert_process(PCB new_process, int algorithm) {
//     Node *new_node = (Node *) malloc(sizeof(Node));
//     new_node->process = new_process;
//     new_node->next = NULL;
//
//     if (ready_Queue == NULL) {
//         ready_Queue = new_node;
//         return;
//     }
//
//     Node *current = ready_Queue;
//     Node *prev = NULL;
//
//     if (algorithm == HPF) {
//         while (current != NULL && current->process.priority <= new_process.priority) {
//             prev = current;
//             current = current->next;
//         }
//     } else if (algorithm == SRTN) {
//         while (current != NULL && current->process.remaining_time <= new_process.remaining_time) {
//             prev = current;
//             current = current->next;
//         }
//     } else if (algorithm == RR) {
//         while (current->next != NULL) {
//             current = current->next;
//         }
//         current->next = new_node;
//         return;
//     }
//
//     // Insert at the right position (for HPF and SRTN)
//     if (prev == NULL) {
//         new_node->next = ready_Queue;
//         ready_Queue = new_node;
//     } else {
//         new_node->next = current;
//         prev->next = new_node;
//     }
// }
//
// void remove_process(int id_from_file) {
//     if (ready_Queue == NULL) {
//         return;
//     }
//
//     if (ready_Queue->process.id_from_file == id_from_file) {
//         Node *todelete = ready_Queue;
//         ready_Queue = ready_Queue->next;
//         free(todelete);
//         return;
//     }
//
//     Node *ptr = ready_Queue;
//     while (ptr->next != NULL && ptr->next->process.id_from_file != id_from_file) {
//         ptr = ptr->next;
//     }
//
//     if (ptr->next == NULL) {
//         return;
//     }
//
//     Node *todelete = ptr->next;
//     ptr->next = todelete->next;
//     free(todelete);
// }
//
// PCB *pick_next_process() {
//     if (ready_Queue == NULL)
//         return NULL;
//     return &(ready_Queue->process);
// }
//
// void print_ready_queue() {
//     printf("Ready Queue:\n");
//     Node *current = ready_Queue;
//     if (current == NULL) {
//         printf("The ready queue is empty.\n");
//         return;
//     }
//
//     while (current != NULL) {
//         if (current->process.state != TERMINATED) {
//             printf("Process %d: Arrival = %d, Runtime = %d, Remaining = %d, State = %s\n",
//                    current->process.pid,
//                    current->process.arrival_time,
//                    current->process.remaining_time,
//                    current->process.remaining_time,
//                    current->process.state == RUNNING
//                        ? "RUNNING"
//                        : current->process.state == WAITING
//                              ? "WAITING"
//                              : "TERMINATED");
//         }
//         current = current->next;
//     }
// }
