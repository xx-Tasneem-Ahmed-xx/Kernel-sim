#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "headers.h"

void init_ready_queue();

void init_scheduler();

void run_scheduler();

void handle_process_arrival(PCB new_process);

void get_message_ID(int *msgq_id, key_t *key);

void receive_new_process();

void run_HPF_Algorithm();


void run_SRTN_Algorithm();

void context_switching();


#endif