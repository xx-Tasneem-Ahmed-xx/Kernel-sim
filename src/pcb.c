#include "headers.h"
#include "pcb.h"

Node* ready_Queue = NULL;

void insert_process(PCB new_process, int algorithm) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->process = new_process;
    new_node->next = NULL;

    if (ready_Queue == NULL) {
        ready_Queue = new_node;
        return;
    }

    Node* current = ready_Queue;
    Node* prev = NULL;

    if (algorithm == HPF) {
        while (current != NULL && current->process.priority <= new_process.priority) {
            prev = current;
            current = current->next;
        }
    } else if (algorithm == SRTN) {
        while (current != NULL && current->process.remaining_time <= new_process.remaining_time) {
            prev = current;
            current = current->next;
        }
    } else if (algorithm == RR) {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
        return;
    }

    // Insert at the right position (for HPF and SRTN)
    if (prev == NULL) {
        new_node->next = ready_Queue;
        ready_Queue = new_node;
    } else {
        new_node->next = current;
        prev->next = new_node;
    }
}

void remove_process(int id_from_file) {
    if (ready_Queue == NULL) {
        return;
    }

    if (ready_Queue->process.id_from_file == id_from_file) {
        Node *todelete = ready_Queue;
        ready_Queue = ready_Queue->next;
        free(todelete);
        return;
    }

    Node *ptr = ready_Queue;
    while (ptr->next != NULL && ptr->next->process.id_from_file != id_from_file) {
        ptr = ptr->next;
    }

    if (ptr->next == NULL) {
        return;
    }

    Node *todelete = ptr->next;
    ptr->next = todelete->next;
    free(todelete);
}

PCB* pick_next_process() {
    if (ready_Queue == NULL)
        return NULL;
    return &(ready_Queue->process);
}

void print_ready_queue() {
    Node *current = ready_Queue;

    if (current == NULL) {
        printf("The ready queue is empty.\n");
        return;
    }

    printf("Ready Queue (sorted by priority):\n");
    printf("PID\tPriority\tRemainging time\n");
    printf("-----------------------------------------------------------------------------------\n");

    while (current != NULL) {
        PCB process = current->process;
        printf("%d\t%d\t%d\n",
               process.pid,
               process.priority,
               process.remaining_time
        );

        current = current->next;
    }
}
