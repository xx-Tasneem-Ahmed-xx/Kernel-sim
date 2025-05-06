#include "clk.h"
#include "headers.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>

int runtime = 0;
int *shared_remaining_time = NULL; 
int shmid_p = -1;
pid_t scheduler_pid = -1; 
int semid = -1; 

int preTime = 0; 
int currTime = 0; 

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
    preTime = currTime; // Initialize preTime (now a global variable)

    // Print initialization message
    log_message(LOG_PROCESS, "Process %d initialized with runtime=%d", getpid(), runtime);
    log_message(LOG_PROCESS, "Process %d waiting for scheduler signal...", getpid());
    
    while (1)
    {
        // Wait for scheduler to signal this process to run
        // Use blocking down to wait for semaphore
        int result = down_nb(semid);
        if (result < 0) {
            continue;
        }        int current_clk = get_clk();
        while(get_clk() == current_clk) {
            // Busy wait for the next clock tick
            usleep(1000); // Sleep a bit to reduce CPU usage
        }
        
        // Process got permission to run for this tick
        log_message(LOG_PROCESS, "Process %d acquired semaphore at time %d", getpid(), get_clk());
        
        // Do work for this tick
        (*shared_remaining_time)--;
        log_message(LOG_PROCESS, "Process %d running at time %d, remaining time: %d", 
                   getpid(), get_clk(), *shared_remaining_time);
        
        if (*shared_remaining_time % 5 == 0 || *shared_remaining_time == 0)
        {
            printf("%sProcess %d Progress:%s\n", COLOR_GREEN, getpid(), COLOR_RESET);
            print_progress_bar(runtime - *shared_remaining_time, runtime, 20);
        }
        
        // Signal back to scheduler that this process is done for the tick
        // up(semid);
        
        if (*shared_remaining_time <= 0)
        {
            break;
        }
        
        // Wait for the next tick
    }
    
    // Print completion message
    log_message(LOG_PROCESS, "Process %d finished at time %d", getpid(), get_clk());
    printf("%sProcess %d Progress:%s\n", COLOR_GREEN, getpid(), COLOR_RESET);
    print_progress_bar(runtime, runtime, 20);

    // Notify scheduler of completion
    if (scheduler_pid > 0)
    {
        *shared_remaining_time = 0;
        up(semid);
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
    semid = atoi(argv[4]);

    run_process(runtime, shared_mem_id);
    destroy_clk(0);
    return 0;
}