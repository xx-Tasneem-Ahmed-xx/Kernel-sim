#include <sys/types.h> 
#include <sys/ipc.h>   
#include <sys/sem.h>   
#include <stdio.h>     
#include <stdlib.h>    
#include <unistd.h>    
#include <errno.h>     
#include <string.h>    

#include "semaphores.h" // Include own header


void down(int semid) {
    struct sembuf op = {0, -1, 0}; 
    if (semop(semid, &op, 1) == -1) { 
        perror("down failed");
        exit(1);
    }
}

void up(int semid) {
    struct sembuf op = {0, 1, 0}; 
    if (semop(semid, &op, 1) == -1) {
        perror("up failed");
        exit(1);
    }
}


int down_nb(int semid) {
    struct sembuf op;
    op.sem_num = 0;      
    op.sem_op = -1;      
    op.sem_flg = IPC_NOWAIT; 
    
    int result = semop(semid, &op, 1);
    if (result == -1) {
        if (errno == EAGAIN) {
            return -1;
        } else {
            fprintf(stderr, "down_nb error: %s (errno=%d, semid=%d)\n", 
                    strerror(errno), errno, semid);
            return -2;
        }
    }
    return 0;
}


int up_nb(int semid) {
    struct sembuf op = {0, 1, IPC_NOWAIT}; 
    int result = semop(semid, &op, 1);
    if (result == -1) {
        perror("up_nb failed");
        return -1;
    }
    return 0; 
}
