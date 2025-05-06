#include "headers.h"
#include "pcb.h"

const char* ProcessStateNames[] = {
    "WAITING",   // Added to match enum definition if used
    "READY",
    "RUNNING",
    "STOPPED",
    "TERMINATED"
};