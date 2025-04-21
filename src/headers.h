#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <time.h>

// data structure files
#include "DS/priorityQueue.h"
#include "DS/minHeap.h"
#include "DS/Queue.h"
#include "DS/comparators.h"

// header files
#include "clk.h"
#include "pcb.h"
#include "process.h"

typedef enum { HPF, SRTN, RR } SchedulingAlgorithm;

typedef struct {
    long mtype;
    PCB pcb;
} MsgBuffer;



#endif
