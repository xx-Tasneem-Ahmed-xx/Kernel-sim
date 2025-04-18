#include "scheduler.h"

int algorithm;
int process_count = 0;
Node *ready_Queue = NULL;


// void init_PCB();

void run_scheduler() {
    // sync_clk();

    //TODO implement the scheduler :)
    // You may split it into multiple files
    //upon termination release the clock resources.

    // destroy_clk(0);
}

void insert_Process(PCB process) {
    Node *new_node = (Node *) malloc(sizeof(Node));
    new_node->process = process;

    if (ready_Queue == NULL || ready_Queue->process.priority > process.priority) {
        new_node->next = ready_Queue;
        ready_Queue = new_node;
        return;
    }

    Node *ptr = ready_Queue;
    Node *prev_ptr = ready_Queue;
    while (ptr != NULL && ptr->process.priority < process.priority) {
        prev_ptr = ptr;
        ptr = ptr->next;
    }

    new_node->next = ptr;
    prev_ptr->next = new_node;
}

void remove_Process(PCB process) {
    if (ready_Queue == NULL) {
        return;
    }

    if (ready_Queue->process.id_from_file == process.id_from_file) {
        Node *todelete = ready_Queue;
        ready_Queue = ready_Queue->next;
        free(todelete);
        return;
    }

    Node *ptr = ready_Queue;
    while (ptr->next != NULL && ptr->next->process.id_from_file != process.id_from_file) {
        ptr = ptr->next;
    }

    if (ptr->next == NULL) {
        return;
    }

    Node *todelete = ptr->next;
    ptr->next = todelete->next;
    free(todelete);
}

void update_Processes(PCB process) {
}
void print_PriorityQueue() {
    if (ready_Queue == NULL) {
        printf("The queue is empty.\n");
        return;
    }

    Node *ptr = ready_Queue;
    printf("Process Priorities in Queue:\n");
    while (ptr != NULL) {
        printf("Priority: %d\n", ptr->process.priority);
        ptr = ptr->next;
    }
}

int main() {

    return 0;
}
