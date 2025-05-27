#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>      // for keys
#include <sys/ipc.h>        // for shared segment & keys
#include <sys/shm.h>        // for shared segment
#include <time.h>           // time()
#include <signal.h>
#include <pthread.h>
#include <string.h>

#include "shared_memory.h"
#include "functions.h"

int nlines; // Number of lines that the text file has.

pthread_mutex_t lock_process_table = PTHREAD_MUTEX_INITIALIZER; // Mutex for locking the process_table so the children can find their semaphore

// This sem is used with the flag below.
// When flag == 1 it means that there is on going termination, so wait for the termination to end,
// and after that choose a child to send a message.
sem_t sem;
int flag = 0;

// When spawn == 1 it means that there is on going fork, so wait until the child is initialized, 
// to terminate a child or to send a message to a child.
int spawn = 0 ; 

void sigcont_handler(int signo) 
{
    // This means that the child complete the initialization after fork so the parent can operate
    // termination or send a message to a child. 
    printf("[Parent] Received SIGCONT.\n");
    spawn = 0;
}


int main(int argc, char *argv[]) 
{
    signal(SIGCONT, sigcont_handler); // Set up signal handler - When the child completes each initialization.
    
    /*-------------------------------------------------------------------------------------*/
    // Step 1: Read arguments from command line
    if (argc != 4) {      // Error check
        printf("Give <CF.txt> <text.txt> <M> \n");
        exit(EXIT_FAILURE);  
    }

    int M = atoi(argv[3]);             // Number of semaphores given by the user

    if(M > MAX_M) 
    {
        printf("Maximum M is %d.\n", MAX_M);
        exit(EXIT_FAILURE);  
    }


    // Step 2: We open the text files (command file & text file)
	CF = fopen(argv[1], "r");

    // Check if the opening went correctly 
	if (!CF){
		printf("Could not open command file.\n");
        // Close the files so i have no memory leaks
        fclose(CF);
        exit(EXIT_FAILURE);
    }

    text = fopen(argv[2], "r");

    // Check if the opening went correctly 
	if (!text){
		printf("Could not open command file.\n");
        // Close the files so i have no memory leaks
        fclose(text);
        fclose(CF);
        exit(EXIT_FAILURE);
    }

    // Count the number of lines in the file 
    nlines = count_lines(text);

    if(nlines < 10) {
        printf("We need a text with more than 10 lines!\n");
        // Close the files so i have no memory leaks
        fclose(text);
        fclose(CF);
        exit(EXIT_FAILURE);  
    }
    /*-------------------------------------------------------------------------------------*/

    key_t key;
	int shmid;

    // make the key for shared memory
	if ((key = ftok(argv[1], 'R')) == -1) {
		perror("ftok");
        // Close the files so i have no memory leaks
        fclose(CF);
        fclose(text);
		exit(1);
	}
    //printf("[Parent] Generated key: %d\n", key);

    connect_shmem(&shmid, key);
    shmem = attach_shmem(shmid);

    char line[250];

    // Initialize same variables 
    for (int i = 0; i < M; i++) 
    {
        shmem->process_table[i].free = 0;       // Mark all the semaphores as free 
        shmem->process_table[i].pid = 0;        // Mark all the pids as free
        shmem->childinfo[i].active = 0;         // Mark all the children as inactive
        shmem->child_pids[i] = 0;               // Initialize all the children pids to 0
    }

    // Initialize the table of semaphores
    for (int i = 0; i < M; i++) 
    {
        sem_init(&shmem->childinfo[i].message_read, 1, 1); 
    }

    // Initialize the semaphore used to synchronize termination and sending a message by the parent.
    sem_init(&sem, 1, 0);

    /*-------------------------------------------------------------------------------------*/

    int timestamp;      // The first argument of the CF file.
    int counter = 0;    // To compare it with the timestamp of the CF and know when to run each line.

    char process[10];   // Number of process eg C2
    char command[10];   // This is S for spawn, T for Terminate or EXIT
    
    char* result;

    while ((result = fgets(line, sizeof(line), CF)) != NULL)
    {

        // Clear the buffers
        memset(process, 0, sizeof(process));
        memset(command, 0, sizeof(command)); 
        timestamp = 0; 


        sscanf(line, "%d %s %s", &timestamp, process, command);

        printf("[Parent] Current line: %s\n", line);
        memset(line, 0, sizeof(line));  // Clear the buffers

        while (counter < timestamp) 
        {
            printf("[Parent] Counter = %d\n", counter);
            // Same as the parent process after fork
            
            // If flag == 1 it means that there is on going termination, so
            // wait until the termination is finished. 
            if (flag == 1)
            {
                sem_wait(&sem);
            }

            // If spawn == 1 it means that there is on going fork, so
            // wait until the child is initialized. 
            if (spawn == 1)
            {
                pause();
            }

            // Check if there are active childer, in order to send a line.
            // Otherwise, skip to the next iteration. 
            if (count_active_children() >= 1)
            {
                //printf("[Parent] It is time to send a message.\n");

                printf("[Parent] active_children: %d\n", count_active_children());
                int child = rand() % count_active_children();        // Choose a random active child
                pid_t selected_child = find_occurrence(child);      // Find the randoms child pid from the array
                // printf("[Parent] Selected child: %d\n", selected_child); 
                // printf("[Parent] Child: %d\n", child);

                int random_num = rand() % nlines;   // Choose a random line from the text file
                char random_line[256];
                fseek(text, 0, SEEK_SET);           // This resets the file position to allow re-reading from the beginning.
                for (int i = 0; i <= random_num; i++) {
                    fgets(random_line, sizeof(random_line), text);  // Read the file until the choosen line 
                }
                int index = find_sem_index(selected_child);     // Get the index of the semaphore of this process

                // Wait for the previous message to be processed
                if (strlen(shmem->childinfo[index].message) > 0 || shmem->childinfo[index].active == 0) 
                {
                    // Wait for the child to acknowledge the previous message
                    sem_wait(&shmem->childinfo[index].message_read);
                }

                // This is the critical section where the child is already sem_wait
                strncpy(shmem->childinfo[index].message, random_line, sizeof(shmem->childinfo[index].message) - 1);
                shmem->childinfo[index].message[sizeof(shmem->childinfo[index].message) - 1] = '\0';  // Null-terminate the message
                printf("[Parent] Parent sending to child %d message %s\n", selected_child, shmem->childinfo[index].message);
                sem_post(&shmem->semaphores[index]);    // Notify the child to get the message
            }
            counter++; // Increment counter until it matches timestamp
        }
        if (strcmp(command, "S") == 0)   // Spawn a new process with fork
        {
            // Born new process
            pid_t pid;
            spawn = 1;      // Mark the flag as there is going to be a fork

            if ((pid = fork()) < 0) 
            {
                perror("fork");
                // Close the files so i have no memory leaks
                fclose(text);
                fclose(CF);
                exit(1);
            }
            if (pid == 0) // Child process
            {
                //Find the fist available semaphore
                int sem_index = -1;
                pthread_mutex_lock(&lock_process_table);    // We lock the table for synchronization 

                for (int i = 0; i < M; i++) 
                {
                    if (shmem->process_table[i].free == 0)
                    {
                        shmem->process_table[i].free = 1;
                        sem_index = i;  // We find one available so we break 
                        break;
                    }
                }
                // We need to store all the valuable information to the process table 
                sem_wait(&shmem->childinfo[sem_index].message_read);    // wait until the child is ready to read a message

                shmem->process_table[sem_index].pid = getpid();         // We store the pid
                shmem->process_table[sem_index].sem_index = sem_index;  // We store the semaphore index that is going to have
                shmem->child_pids[sem_index] = getpid();                // We store the pid
                shmem->process_table[sem_index].process_index = atoi(process + 1);  // We store the number of the process given by the user

                printf("[Parent] Process child %d with semaphore position = %d.\n", getpid(), sem_index);

                pthread_mutex_unlock(&lock_process_table);

                // Modify the child info struck. This struck is only modified by the child so it doesn't need to be locked
                shmem -> childinfo[sem_index].received_messages = 0;        // Initialize the messages of the process 
                shmem -> childinfo[sem_index].start_timestamp = timestamp;  // Store the start time
                shmem -> childinfo[sem_index].end_timestamp = time(NULL);   // Initialize the end time 
                shmem -> childinfo[sem_index].my_sem = sem_index;           // We store the semaphore index that is going to have
                strcpy(shmem->childinfo[sem_index].message, "");            // Initialize the message buffer

                // printf("[Parent] Child: Ready to execute\n");
                child(argc,argv,sem_index,shmem);
                
                // Close the files so i have no memory leaks
                fclose(text);
                fclose(CF);
                exit(0);  
            }
            else // Parent process
            {
                printf("[Parent] Active_children %d.\n", count_active_children());

                // If flag == 1 it means that there is on going termination, so
                // wait until the termination is finished. 
                if (flag == 1)
                {
                    sem_wait(&sem);
                }

                // If spawn == 1 it means that there is on going fork, so
                // wait until the child is initialized.
                if (spawn == 1)
                {
                    pause();
                }

                // Check if there are active childer, in order to send a line.
                // Otherwise, skip to the next iteration.
                if (count_active_children() > 1)
                {
                    //printf("[Parent] It is time to send a message.\n");
                    int child = rand() % (count_active_children() - 1);         // Choose a random active child
                    pid_t selected_child = find_occurrence(child);              // Find the randoms child pid from the array
                    // printf("[Parent] Selected child: %d\n", selected_child); 
                    // printf("[Parent] Child: %d\n", child);

                    int random_num = rand() % nlines;                   // Choose a random line from the text file
                    char random_line[256];
                    fseek(text, 0, SEEK_SET);                           // This resets the file position to allow re-reading from the beginning.
                    for (int i = 0; i <= random_num; i++) {
                        fgets(random_line, sizeof(random_line), text);  // Read the file until the choosen line 
                    }
                    int index = find_sem_index(selected_child);         // Get the index of the semaphore of this process 

                    // Wait for the previous message to be processed
                    while (strlen(shmem->childinfo[index].message) > 0 || shmem->childinfo[index].active == 0) 
                    {
                        // Wait for the child to acknowledge the previous message
                        sem_wait(&shmem->childinfo[index].message_read);
                    }

                    // This is the critical section where the child is already sem_wait
                    strncpy(shmem->childinfo[index].message, random_line, sizeof(shmem->childinfo[index].message) - 1);
                    shmem->childinfo[index].message[sizeof(shmem->childinfo[index].message) - 1] = '\0';  // Null-terminate the message
                    printf("[Parent] Parent sending to child %d message %s\n", selected_child, shmem->childinfo[index].message);
                    sem_post(&shmem->semaphores[index]);    // Notify the child to get the message
                }
            }
        }
        else if (strcmp(command, "T") == 0) // Terminate the given process
        {
            // If spawn == 1 it means that there is on going fork, so
            // wait until the child is initialized.
            if (spawn == 1)
            {
                pause();
            }
            
            int process_index = atoi(process + 1);              // Assume process is in the format C1, C2 ect
            printf("[Parent] Terminating process: %s\n", process);

            // Find the child process index and terminate
            pid_t child_pid = find_pid_by_index(process_index);
            int index = find_pid_by_pid(child_pid);

            if (child_pid == -1)
            {
                printf("[Parent] There is no process with index %d.\n", process_index);
                
            }
            else 
            {
                // Mark the flag as there is going to be a termination of a process 
                flag = 1;

                // If the process is processing a message at the moment, wait for it to finish. 
                if (strlen(shmem->childinfo[index].message) > 0)
                {
                    sem_wait(&shmem->childinfo[index].message_read);
                }

                shmem->childinfo[index].end_timestamp = timestamp;        // Change the end timestamp to the one that the termination line has
                shmem->childinfo[index].active = 0;                       // Mark the child as inactive
                shmem->child_pids[index] = 0;                             // Mark the child as inactive
                
                pthread_mutex_lock(&lock_process_table);    
                shmem->process_table[index].pid = 0;                      // Initialize the process_table atrributes to 0 
                shmem->process_table[index].free = 0;
                shmem->process_table[index].process_index = 0;
                shmem->process_table[index].sem_index = 0;
                pthread_mutex_unlock(&lock_process_table);

                //printf("[Parent] Child_pid %d\n",child_pid);
                if (child_pid > 0) 
                {
                    print_stats(child_pid, index);  // Print the stats before killing it
                    kill(child_pid, SIGTERM);       // Send terminate signal to the child
                    waitpid(child_pid, NULL, 0);    // Wait for the child to terminate
                    printf("[Parent] Child %d has terminated.\n", child_pid);
                }
                
                // The termination ended, so change the flag and post the semaphore.
                flag = 0;
                sem_post(&sem);
            }
        }
        else if (strcmp(command, "EXIT") == 0)  
        {
            // This means that the CF ended, so we need to gather all the processes and print their stats.
            // We break the loop and wait for the processes outside. 
            break;
        }
        else 
        {
            printf("Error at the configuration file.\n");
            printf("Current line: %s.\n", line);
            return -1;
        }
        counter++;
    }

    // This means we received command == EXIT

    // Wait for all the process to end 
    // Check if there are still alive children
    if (count_active_children() > 0)
    {
        printf("[Parent] There are %d active children. We are terminating them.\n", count_active_children());
        
        for (int i = 0; i < M; i++) // Check the whole child_pids table to terminate them 
        {
            if (shmem->child_pids[i] !=0)
            {
                pid_t child_pid = shmem->child_pids[i];
                if (kill(child_pid, 0) == 0)    // Check if the child process is still active
                {  
                    printf("[Parent] Killing child process with PID: %d\n", child_pid);
                    shmem->childinfo[i].end_timestamp = timestamp;      // Change the end timestamp to the one that the termination line has
                    print_stats(child_pid, i);
                    kill(child_pid, SIGTERM);  // Send termination signal to the child
                }
            }
        }
    }

    // Close the files so i have no memory leaks
    fclose(CF);  
    fclose(text);

    if (sem_destroy(&sem) != 0) 
    {
        perror("Failed to destroy semaphore");
    } 

    destroy_semaphores();
    detach_shmem(shmem);
    shmctl(shmid, IPC_RMID,NULL);

    return 0;
}
