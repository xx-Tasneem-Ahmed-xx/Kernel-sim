#include "clk.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

int runtime = 0;
int *shared_remaining_time = NULL; // Pointer to shared memory
int shmid_p = -1;
int is_running = 0;

void cleanup()
{
    // Detach from shared memory
    if (shared_remaining_time != NULL)
    {
        shmdt(shared_remaining_time);
    }
}

void run_process(int total_runtime, int shared_mem_id)
{

    // Setup exit handler to clean up shared memory
    atexit(cleanup);

    // Attach to shared memory
    shmid_p = shared_mem_id;
    shared_remaining_time = (int *)shmat(shmid_p, NULL, 0);
    if (shared_remaining_time == (int *)-1)
    {
        perror("Process: shmat failed");
        exit(1);
    }

    // Initialize remaining time in shared memory
    *shared_remaining_time = total_runtime;

    sync_clk();

    int currTime = get_clk();
    int preTime = currTime;

    printf("Process initialized with shared memory ID %d and rem time = %d \n", shmid_p, *shared_remaining_time);

    while ((*shared_remaining_time) > 0)
    {

        currTime = get_clk();
        // printf("Process %d running at time %d, remaining time: %d\n", getpid(), currTime, *shared_remaining_time);

        // Do work for one time unit
        if (currTime != preTime)
        {
            preTime = currTime;
            (*shared_remaining_time)--;
            printf("Process %d running at time %d, remaining time: %d\n", getpid(), get_clk(), *shared_remaining_time);
        }

        // sleep(1); // 10ms delay
    }

    printf("Process finished at time %d\n", get_clk());
    exit(0);
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        printf("Usage: %s <runtime> <shared_memory_id>\n", argv[0]);
        return 1;
    }

    runtime = atoi(argv[1]);
    int shared_mem_id = atoi(argv[2]);

    printf("Process %d starting with runtime=%d and shared memory ID=%d\n", getpid(), runtime, shared_mem_id);

    run_process(runtime, shared_mem_id);
    destroy_clk(0);
    return 0;
}