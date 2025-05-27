#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>        // for shared segment
#include <string.h>
#include <pthread.h>

#include "shared_memory.h"

// This is same code as presented from mr Paskalis but separated in functions,
// eclass > έγγραφα > Φροντιστήριο > 2023-2024 > Semaphores_Lab1 > shm >shmdemo.c
void connect_shmem(int *shmid, key_t key)
{
    if ((*shmid = shmget(key, sizeof(shared_data), PERMS | IPC_CREAT)) == -1){
        perror("shmget");
        exit(1);
    }

}

shared_data* attach_shmem(int shmid) {
    shared_data *shmem = shmat(shmid, NULL, 0);  // Attach to the shared memory
    if (shmem == (shared_data *)(-1)) {
        perror("shmat");
        exit(1);
    }

    //Initialize semaphores
    for (int i = 0; i < MAX_M; i++) {
        // 1 because the semaphore is shared between processes 
        // 0 because we want the child to be blocked until the parent writes a message
        if (sem_init(&shmem->semaphores[i], 1, 0) != 0) {
            perror("sem_init failed");
            exit(1);
        }
    }

    return shmem;
}

void detach_shmem(shared_data* data)
{
    if (shmdt(data) == -1) {
		perror("shmdt");
		exit(1);
	}
}