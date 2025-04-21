#include "clk.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int runtime = 0;
int remaining_time = 0;
int is_running = 0;

void termination_handler(int signum)
{
    printf("Process received SIGTERM, cleaning up...\n");
    exit(0);
}

void continue_handler(int signum)
{
    // Process resumed by scheduler
    is_running = 1;
    printf("Process resumed at time %d, remaining time: %d\n", get_clk(), remaining_time);
}
void stop_handler(int signum)
{
    // Process resumed by scheduler
    is_running = 0;
    printf("Process stopped at time %d, remaining time: %d\n", get_clk(), remaining_time);
}

void run_process(int total_runtime)
{
    signal(SIGTERM, termination_handler);
    signal(SIGCONT, continue_handler);
    signal(SIGTSTP, stop_handler);

    sync_clk();

    remaining_time = total_runtime;

    // printf("REM %d\n", remaining_time);
    // Process will be immediately stopped by scheduler after creation
    // using SIGSTOP, so we wait until we're scheduled
    while (remaining_time > 0)
    {
        // Wait for SIGCONT from scheduler
        while (!is_running)
        {
            pause();
        }

        // Do work for one time unit
        // sleep(1); // Simulate one unit of work
        remaining_time--;

        printf("Process ran for 1 time unit at time %d, remaining: %d\n", get_clk(), remaining_time);
    }

    printf("Process finished at time %d\n", get_clk());
    destroy_clk(0);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <runtime>\n", argv[0]);
        return 1;
    }

    runtime = atoi(argv[1]);
    // printf("runtime sen\n",runtime);
    run_process(runtime);

    return 0;
}