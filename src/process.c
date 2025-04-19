#include "clk.h"
#include "signal.h"

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
