#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "headers.h"

void init_scheduler();

void run_scheduler();

void insert_Process(PCB process);

void remove_Process(PCB process);

void update_Processes(PCB process);

void run_HPF_Algorithm();
#endif