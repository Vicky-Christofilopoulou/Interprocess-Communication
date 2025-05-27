#include <stdio.h>
#include <stdlib.h>

FILE* CF;   // For the configuration file
FILE* text; // For the text file

extern int spawn;

// The functions implemented in functions.c
int count_lines(FILE *file);        // This is used for counting the total number of lines of the text file.
int find_sem_index(pid_t pid);      // This is used from the parent to find the correct semaphore in order to send the message.
pid_t find_pid_by_index(int index); // This is used to find the pid when we want to terminate a specific process.
int find_pid_by_pid(pid_t pid);     // This is used from the handle_child_termination to get the index of semaphore table given by the pid of the child.
void child(int argc, char *argv[],int sem_index, shared_data* shmem);   // This is the child process which we will call from fork.
pid_t find_occurrence(int number);      // This is used when the parent send a message to a child.
void handle_child_termination(int sig); // This is the functions that the child goes when it gets signal SIGTERM.
void print_stats(pid_t pid, int index); // This is for printing the stats at the end of each process.
void destroy_semaphores();              // This is for cleaning and destroying the semaphores at the end.
int count_active_children();            // This is for counting all the active children.