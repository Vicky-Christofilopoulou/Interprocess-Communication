#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#define SHM_SIZE sizeof(shared_data) 
#define MAX_M 1000    // Because the M is given by the user we set an upper bounder
#define PERMS 0666
#define MAX_MESSAGE_LENGTH  1024

// This table stores important information for the process and the semaphore index
typedef struct {
    int free;           // 0 means the semaphore is free, and 1 it is not
    pid_t pid;          // This holds the pid of the process that uses the semaphore
    int sem_index;      // This is the semaphore index for the specific process
    int process_index;  // This is the number of the process eg 1 fro C1
} ProcessEntry;

// For each process we need to store the statistics and some more useful information 
typedef struct {
    int active;                 // 0 if the process is not active, 1 if it is
    int received_messages;      // This is the number of messages received by the child
    int start_timestamp;        // This is the start timestamp
    int end_timestamp;          // This is the end timestamp
    char message[256];          // The buffer so the parent can write the message
    int my_sem;                 // The index of each own semaphore for the semaphores table
    sem_t message_read;
} ChildInfo;

// This is the data which are being shared through the shared memory
typedef struct shared_data {
    ProcessEntry process_table[MAX_M];  // For each process important information
    sem_t semaphores[MAX_M];                // A table with all the semaphores (available and used)
    ChildInfo childinfo[MAX_M];         // For each process their statistics
    pid_t child_pids[MAX_M];            // For each generated process store the pid
} shared_data;

shared_data *shmem;

// The functions implemented in shared_memory.c
void connect_shmem(int *shmid, key_t key);
shared_data* attach_shmem(int shmid);
void detach_shmem(shared_data* data);
