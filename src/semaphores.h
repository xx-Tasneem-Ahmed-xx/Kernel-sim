#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

// Union for semctl arguments
union semun { 
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

void down(int semid);
void up(int semid);
int down_nb(int semid);
int up_nb(int semid);

#endif // SEMAPHORES_H
