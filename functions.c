#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h> // for shared segment
#include <string.h>
#include <pthread.h>
#include <unistd.h>


#include "shared_memory.h"
#include "functions.h"

// This is used for counting the total number of lines of the text file.
int count_lines(FILE *file)
{
    int number_of_lines = 0;
    int ch;

    while ((ch = getc(file)) != EOF)
    {
        if (ch == '\n')
        {
            ++number_of_lines;
        }
    }

    rewind(file); // Reset the pointer to the start of the file.
    return number_of_lines;
}

// This is used from the parent to find the correct semaphore in order to send the message.
int find_sem_index(pid_t pid)
{
    for (int i = 0; i < MAX_M; i++)
    {
        // Check if the process is not free and the pid matches
        if (shmem->process_table[i].pid == pid)
        {
            return shmem->process_table[i].sem_index; //Return the semaphore index
        }
    }
    // Return -1 if the pid is not found in the table
    return -1;
}

// This is used to find the pid when we want to terminate a specific process.
pid_t find_pid_by_index(int index) 
{
    for (int i = 0; i< MAX_M; i++)
    {
        if (shmem->process_table[i].process_index == index)
        {
            return shmem->process_table[i].pid;
        }
    }
    return -1;
}

// This is used from the handle_child_termination to get the index of semaphore table given by the pid of the child.
int find_pid_by_pid(pid_t pid) 
{
    for (int i = 0; i< MAX_M; i++)
    {
        if (shmem->process_table[i].pid == pid)
        {
            return i;
        }
    }
    return -1;
}

// This is used when the parent send a message to a child.
// For example, we want to send a message to child 2. Because it is
// the second child, it doesn't mean that it is also on the second spot
// of the table. So we irritate the table, until we find the second alive child.
pid_t find_occurrence(int number)
{
    int count = 0;
    for (int i = 0; i< MAX_M; i++)
    {
        if (shmem->process_table[i].pid != 0 && shmem->childinfo[i].active == 1)
        {
            if (count == number)
            {
                return shmem->process_table[i].pid;
            }
            count++;
        }
    }
    return -1;
} 

// This is the functions that the child goes when it gets signal SIGTERM.
void handle_child_termination(int sig) 
{
    // Close the files so i have no memory leaks
    fclose(CF);     // Close CF.txt
    fclose(text);   // Close text.txt
    exit(0);        // Exit the child process
}

// This is for printing the stats at the end of each process.
void print_stats(pid_t pid, int index)
{
    shmem->process_table[index].free = 0;  // Mark the semaphore as free
    shmem->childinfo[index].active = 0;
    printf("\n------------------------------------------------------\n");
    printf("[Child] Statistic from process: %d\n", pid);
    printf("[Child] Message Received: %d\n", shmem->childinfo[index].received_messages);
    printf("[Child] Active time: %d\n", shmem ->childinfo[index].end_timestamp - shmem->childinfo[index].start_timestamp);
    printf("------------------------------------------------------\n");
}

// This is for cleaning and destroying the semaphores at the end.
void destroy_semaphores() 
{
    for (int i = 0; i < MAX_M; i++) {
        // Destroy each semaphore
        if (sem_destroy(&shmem->semaphores[i]) != 0) 
        {
            perror("Failed to destroy semaphore");
        } 
        // else 
        // {
        //     printf("Semaphore %d destroyed successfully.\n", i);
        // }
    }
}

// This is for counting all the active children.
int count_active_children() 
{
    int count = 0;

    for (int i = 0; i < MAX_M; i++) {
        if (shmem->childinfo[i].active == 1) {
            count++;
        }
    }

    return count;
}
