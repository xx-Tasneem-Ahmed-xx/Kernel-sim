#pragma once
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "headers.h"

void run_scheduler();

void handle_process_arrival(PCB *new_process);

void get_message_ID(int *msgq_id, key_t *key);

void receive_new_process();

// void run_HPF_Algorithm();
void context_switching(void);

void run_SRTN_Algorithm();


#endif