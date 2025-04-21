#ifndef COMPARATORS_H
#define COMPARATORS_H

#include "../pcb.h"

int compare_remaining_time(PCB *a, PCB *b);
int compare_priority(PCB *a, PCB *b);
int compare_arrival_time(PCB *a, PCB *b);

#endif
