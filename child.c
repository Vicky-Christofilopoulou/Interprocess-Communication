#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>      // for keys
#include <sys/ipc.h>        // for shared segment & keys
#include <sys/shm.h>        // for shared segment
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include "shared_memory.h"
#include "functions.h"

void child(int argc, char *argv[],int sem_index, shared_data* shmem)
{
    //printf("[Child] This is the child process %d.\n", getpid());

    // Register the signal handler
    signal(SIGTERM, handle_child_termination);

    // Check if the child is active and haven't recieved SIGTERM
    shmem -> childinfo[sem_index].active = 1;
    sem_post(&shmem->childinfo[sem_index].message_read);

    // Mark the flag as 0, because the child finished its initialization,
    // so notify, the ones who have been waiting. 
    spawn = 0;
    kill(getppid(), SIGCONT);

    while (shmem->childinfo[sem_index].active == 1)
    {
        // Wait until the parent writes a message 
        //printf("[Child] Now i am waiting %d ...\n", getpid());

        sem_wait(&shmem->semaphores[sem_index]);

        // This is the critical section
        // 1. Wake up
        // 2. Ready the message
        // 3. Clean the buffer
        // 4. Increase the counter
        // 5. Notify the parent that you read the message
        // 6. Wait for new messages
        
        printf("[Child] Child %d: Woke up, message: %s\n", getpid(), shmem->childinfo[sem_index].message);
        strncpy(shmem -> childinfo[sem_index].message, "", sizeof(shmem -> childinfo[sem_index].message) - 1);  // Clear message after processing
        shmem -> childinfo[sem_index].message[sizeof(shmem -> childinfo[sem_index].message) - 1] = '\0';        // Null terminate
        shmem->childinfo[sem_index].received_messages = shmem->childinfo[sem_index].received_messages + 1;

        // Signal the parent that processing is complete
        sem_post(&shmem->childinfo[sem_index].message_read);
    }
    //printf("[Child] This is the end of the child process.\n");
}
