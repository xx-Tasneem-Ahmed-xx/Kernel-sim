#include "clk.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

void termination_handler()
{
}

void run_process(int runtime)
{
    signal(SIGTERM, termination_handler);

    init_clk();
    int start_time = get_clk();
    while (1)
    {
        if (get_clk() == start_time + runtime)
            break;
    }
    destroy_clk(0);
    raise(SIGTERM);
    exit(0);
}

/* Add main function to make process.c a standalone executable */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <runtime>\n", argv[0]);
        return 1;
    }
    
    int runtime = atoi(argv[1]);
    run_process(runtime);
    
    return 0;
}
