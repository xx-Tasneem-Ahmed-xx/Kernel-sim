#include "clk.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>

// Color and formatting definitions
#define COLOR_RESET   "\x1B[0m"
#define COLOR_GREEN   "\x1B[32m"
#define BOLD          "\x1B[1m"

int runtime = 0;
int *shared_remaining_time = NULL; // Pointer to shared memory
int shmid_p = -1;
int is_running = 0;
pid_t scheduler_pid = -1;  // Added to store scheduler's PID

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

    // Update initialization message
    printf("%s[PROCESS %d]%s Initialized with runtime=%d\n", 
           COLOR_GREEN, getpid(), COLOR_RESET, runtime);

    while ((*shared_remaining_time) > 0)
    {

        currTime = get_clk();
        // Update running messages - make this less verbose
        if (currTime != preTime)
        {
            preTime = currTime;
            (*shared_remaining_time)--;
            
            // Only print every 5 clock ticks or on the last tick
            if (*shared_remaining_time % 5 == 0 || *shared_remaining_time == 0) {
                printf("%s[PROCESS %d]%s Running at time %d, remaining time: %d\n", 
                       COLOR_GREEN, getpid(), COLOR_RESET, get_clk(), *shared_remaining_time);
            }
        }

        // sleep(1); // 10ms delay
    }

    // Update completion message
    printf("%s[PROCESS %d]%s %sFinished%s at time %d\n", 
           COLOR_GREEN, getpid(), COLOR_RESET, BOLD, COLOR_RESET, get_clk());
    
    // Notify scheduler that this process has completed
    if (scheduler_pid > 0) {
        printf("Sending completion signal to scheduler (PID: %d)\n", scheduler_pid);
        kill(scheduler_pid, SIGUSR2);
    } else {
        printf("Warning: Scheduler PID not set, cannot send completion notification\n");
    }
    
    exit(0);
}

int main(int argc, char *argv[])
{

    if (argc < 4)
    {
        printf("Usage: %s <runtime> <shared_memory_id> <scheduler_pid>\n", argv[0]);
        return 1;
    }

    runtime = atoi(argv[1]);
    int shared_mem_id = atoi(argv[2]);
    scheduler_pid = atoi(argv[3]);  // Get scheduler PID from command line

    printf("%s[PROCESS %d]%s Starting with runtime=%d, shared memory ID=%d, scheduler PID=%d\n", 
           COLOR_GREEN, getpid(), COLOR_RESET, runtime, shared_mem_id, scheduler_pid);

    run_process(runtime, shared_mem_id);
    destroy_clk(0);
    return 0;
}