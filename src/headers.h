#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "priorityQueue.h"
#include "clk.h"

typedef enum {RUNNING, READY, BLOCKED} ProcessState;

typedef struct PCB {
    int pid;
    int id_from_file;
    int arrival_time;
    int total_runtime;
    int remaining_time;
    int priority;
    ProcessState state;
} PCB;


typedef struct Node {
    PCB process;
    struct Node* next;
} Node;

#endif