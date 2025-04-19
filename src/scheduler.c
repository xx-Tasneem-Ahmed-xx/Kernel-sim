#include "scheduler.h"

#include <sys/msg.h>

#include "headers.h"

SchedulingAlgorithm algorithm;
int process_count = 0;


void run_scheduler() {
    // sync_clk();
    //TODO implement the scheduler :)
    // You may split it into multiple files
    //upon termination release the clock resources.


    // destroy_clk(0);
}

void handle_process_arrival(Process new_process) {
    PCB new_pcb;

    new_pcb.id_from_file = new_process.id;
    new_pcb.arrival_time = new_process.arrival_time;
    new_pcb.total_runtime = new_process.run_time;
    new_pcb.priority = new_process.priority;
    new_pcb.remaining_time = new_process.run_time;
    new_pcb.waiting_time = 0;
    new_pcb.state = READY;

    insert_process(new_pcb, algorithm);
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
               message.process.run_time, message.process.arrival_time);
    }
}


int main(int argc, char *argv[]) {
    algorithm = atoi(argv[1]); // 1=HPF, 2=SRTN, 3=RR
    sync_clk();

    int msgq_id;
    key_t key;
    get_message_ID(&msgq_id, &key);


    printf("Scheduler: Waiting for message...\n");
    printf("key file is %d and msg qeueu id is %d", key, msgq_id);
    while (1) {
        int current_time = get_clk();
        receive_new_process(msgq_id);
        usleep(50000);
    }
    destroy_clk(0);
    printf("Scheduler: finished receiving...\n");
    // print_ready_queue();
    return 0;
}
