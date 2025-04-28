#include <sys/types.h> // For pid_t
#include <sys/ipc.h>   // For IPC_PRIVATE, etc.
#include <sys/sem.h>   // For semaphore functions
#include <stdio.h>     // For printf, perror
#include <stdlib.h>    // For exit
#include <unistd.h>    // For fork
#include <errno.h>     // For errno
#include <string.h>    // Add this for strerror

#include "semaphores.h" // Include own header

// Function to perform a semaphore "down" (decrement) operation - blocking version
void down(int semid) {
    struct sembuf op = {0, -1, 0}; // Operation: decrement semaphore 0 by 1
    if (semop(semid, &op, 1) == -1) { // Perform the operation
        perror("down failed");
        exit(1);
    }
}

// Function to perform a semaphore "up" (increment) operation - blocking version
void up(int semid) {
    struct sembuf op = {0, 1, 0}; // Operation: increment semaphore 0 by 1
    if (semop(semid, &op, 1) == -1) { // Perform the operation
        perror("up failed");
        exit(1);
    }
}

// Function to perform a non-blocking semaphore "down" (decrement) operation
// Returns: 0 on success, -1 if would block, -2 on other errors
int down_nb(int semid) {
    struct sembuf op;
    op.sem_num = 0;      // First semaphore in the set
    op.sem_op = -1;      // Decrement by 1
    op.sem_flg = IPC_NOWAIT; // Non-blocking operation
    
    int result = semop(semid, &op, 1);
    if (result == -1) {
        if (errno == EAGAIN) {
            // Would block, semaphore is 0
            return -1;
        } else {
            // Print detailed error info
            fprintf(stderr, "down_nb error: %s (errno=%d, semid=%d)\n", 
                    strerror(errno), errno, semid);
            return -2;
        }
    }
    return 0; // Success
}

// Function to perform a non-blocking semaphore "up" (increment) operation
// Returns: 0 on success, -1 on failure, sets errno accordingly
int up_nb(int semid) {
    struct sembuf op = {0, 1, IPC_NOWAIT}; // Non-blocking operation
    int result = semop(semid, &op, 1);
    if (result == -1) {
        perror("up_nb failed");
        return -1;
    }
    return 0; // Success
}
