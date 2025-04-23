/*
 * This file is done for you.
 * Probably you will not need to change anything.
 * This file represents an emulated clock for simulation purpose only.
 * It is not a real part of operating system!
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "clk.h"

#define SHKEY 300

///==============================
// don't mess with this variable
volatile int *shmaddr = NULL;
//===============================

int shmid;

/* Clear the resources before exit */
void cleanup_(int signum) {
    shmctl(shmid, IPC_RMID, NULL);
    printf("Clock terminating!\n");
    exit(0);
}

/* Initialize the clock with shared memory */
void init_clk() {
    printf("Clock starting\n");
    signal(SIGINT, cleanup_);
    
    int clk = 0;
    // Create shared memory for one integer variable 4 bytes
    shmid = shmget(SHKEY, 4, IPC_CREAT | 0644);
    if ((long)shmid == -1) {
        perror("Error in creating shm!");
        exit(-1);
    }
    
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if ((long)shmaddr == -1) {
        perror("Error in attaching the shm in clock!");
        shmctl(shmid, IPC_RMID, NULL); // Clean up the shared memory
        exit(-1);
    }
    
    *shmaddr = clk; /* initialize shared memory */
}

/* Run the clock: increment the shared memory value every second */
void run_clk() {
    while (1) {
        sleep(1);
        (*shmaddr)++;
    }
}

/* Get the current clock value */
int get_clk() {
    return *shmaddr;
}

/* Synchronize with the clock: attach to the shared memory */
void sync_clk() {
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1) {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
    if ((long)shmaddr == -1) {
        perror("Error in attaching the shm in sync_clk!");
        exit(-1);
    }
}

/* Detach from shared memory and optionally terminate all processes */
void destroy_clk(short terminateAll) {
    shmdt(shmaddr);
    if (terminateAll) {
        killpg(getpgrp(), SIGINT);
    }
}