#include "scheduler.h"
#include "headers.h"
SchedulingAlgorithm algorithm;
int process_count = 0;

// void init_PCB();


void run_scheduler() {
    // sync_clk();
    //TODO implement the scheduler :)
    // You may split it into multiple files
    //upon termination release the clock resources.


    // destroy_clk(0);
}

int main() {
    algorithm = RR;
    PCB processA;
    PCB processB;
    PCB processC;
    PCB processD;
    PCB processE;

    // processA.priority = 3;
    // processB.priority = 1;
    // processC.priority = 5;
    // processD.priority = 0;
    // processE.priority = 4;

    processA.remaining_time = 4;
    processB.remaining_time = 2;
    processC.remaining_time = 1;
    processD.remaining_time = 7;
    processE.remaining_time = 5;

    insert_process(processA, algorithm);
    insert_process(processB, algorithm);
    insert_process(processC, algorithm);
    insert_process(processD, algorithm);
    insert_process(processE, algorithm);
    print_ready_queue();
    return 0;
}
