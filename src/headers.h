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
#include "priorityQueue.h"
#include "clk.h"
#include "pcb.h"

typedef enum { HPF, SRTN, RR } SchedulingAlgorithm;

typedef struct {
    long mtype;
    Process process;
} MsgBuffer;

#endif
