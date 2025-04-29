#include "headers.h"
#include "pcb.h"
Node *ready_Queue = NULL;
int process_Count = 0;
const char* ProcessStateNames[] = {
    "READY",
    "RUNNING",
    "STOPPED",
    "TERMINATED"
};