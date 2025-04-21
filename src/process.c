#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "clk.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

int remaining_time = 0;
int id = -1;

// Signal handler for resume (SIGCONT)
void handle_continue(int signum) {
    // Just resume, no action needed
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: ./process.out <id> <runtime>\n");
        return 1;
    }

    id = atoi(argv[1]);
    remaining_time = atoi(argv[2]);

    signal(SIGCONT, handle_continue);  // Resume execution when scheduled
    signal(SIGSTOP, SIG_IGN);          // SIGSTOP is managed by OS

    sync_clk();  // Attach to simulated clock

    while (remaining_time > 0) {
        pause(); // Wait for SIGCONT

        // Simulate 1 time unit of work
        printf("Process %d running at time %d\n", id, get_clk());
        sleep(1);
        remaining_time--;
    }

    printf("Process %d finished at time %d\n", id, get_clk());

    destroy_clk(0);  // Detach from clock
    return 0;
}
