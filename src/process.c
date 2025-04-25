#include "clk.h"
#include "headers.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

int runtime = 0;
int *shared_remaining_time = NULL; // Pointer to shared memory
int shmid_p = -1;
pid_t scheduler_pid = -1;  // Store scheduler's PID

void cleanup()
{
    // Detach from shared memory
    if (shared_remaining_time != NULL)
    {
        shmdt(shared_remaining_time);
    }
}

void run_process(int remaining_time, int shared_mem_id)
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
    *shared_remaining_time = remaining_time;

    sync_clk();

    int currTime = get_clk();
    int preTime = currTime;

    // Print initialization message
    log_message(LOG_PROCESS, "Process %d initialized with runtime=%d", getpid(), runtime);

    while ((*shared_remaining_time) > 0)
    {
        currTime = get_clk();
        if (currTime != preTime)
        {
            preTime = currTime;
            (*shared_remaining_time)--;

            // Print progress every 5 ticks or on last tick
            if (*shared_remaining_time % 5 == 0 || *shared_remaining_time == 0)
            {
                log_message(LOG_PROCESS, "Process %d running at time %d, remaining time: %d", 
                           getpid(), get_clk(), *shared_remaining_time);
                printf("%sProcess %d Progress:%s\n", COLOR_GREEN, getpid(), COLOR_RESET);
                print_progress_bar(runtime - *shared_remaining_time, runtime, 20);
            }
        }
    }

    // Print completion message
    log_message(LOG_PROCESS, "Process %d finished at time %d", getpid(), get_clk());
    printf("%sProcess %d Progress:%s\n", COLOR_GREEN, getpid(), COLOR_RESET);
    print_progress_bar(runtime, runtime, 20);

    // Notify scheduler of completion
    if (scheduler_pid > 0)
    {
        kill(scheduler_pid, SIGUSR2);
    }

    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <runtime> <shared_memory_id> <scheduler_pid>\n", argv[0]);
        return 1;
    }

    runtime = atoi(argv[1]);
    int shared_mem_id = atoi(argv[2]);
    scheduler_pid = atoi(argv[3]);

    run_process(runtime, shared_mem_id);
    destroy_clk(0);
    return 0;
}