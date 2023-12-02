#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <semaphore.h>
#include <string.h>

#define SHM_pids "/my_shared_memory_pids"
#define SEM_PATH_1 "/sem_PID_1"
#define SEM_PATH_2 "/sem_PID_2"
#define MAX_PIDS 3

pid_t *pid_array;


int main() {
    // Create or open a shared memory object for PIDs
    int shm_fd_pids = shm_open(SHM_pids, O_RDWR, 0666);
    if (shm_fd_pids == -1) {
        perror("shm_open failed");
        return -1;
    }

    // Open semaphores
    sem_t *sem_id1 = sem_open(SEM_PATH_1, 0);
    sem_t *sem_id2 = sem_open(SEM_PATH_2, 0);
    
    /* Wait for Master to write */
    sem_wait(sem_id2);
    printf("data read \n");

    // Map the shared memory segment into the address space for PIDs
    pid_array = (pid_t*)mmap(NULL, MAX_PIDS * sizeof(pid_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_pids, 0);
    if (pid_array == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Display the received PIDs
    for (int i = 0; i < MAX_PIDS; ++i) {
        printf("Received PID %d: %d\n", i, (int)pid_array[i]);
    }
    fflush(stdout);

     /* Restart writer */
    sem_post(sem_id1);

    // Close the shared memory object for PIDs (not unlinking yet)
    if (close(shm_fd_pids) == -1) {
        perror("close failed");
        return -1;
    }

 /*
 // Set up signal handling
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler_wd;
    sigaction(SIGUSR1, &sa, NULL);
 */   

    const char *filename = "WD_log.txt";  
    bool brk = false;

   while (1) {

    sleep(5);

    for (int i = 0; i < MAX_PIDS; i++) {
        pid_t target_pid = pid_array[i];  // Get the PID of the target process

        if (kill(target_pid, SIGUSR1) == 0) {
            printf("Signal successfully sent to process with PID %d\n", target_pid);

            // Log that the targeted process is working
            FILE *file = fopen("WD_log.txt", "a");
            if (file != NULL) {
                fprintf(file, " Signal sent to process with PID %d.Process with PID %d is working.\n", target_pid, target_pid);
                fclose(file);
            } else {
                perror("Error opening file for writing");
            }
        } else {
            perror("Error sending signal");
            printf("Process with PID %d is not responding.\n", target_pid);

            // Perform cleanup actions, as in your original code
            for (int j = 0; j < MAX_PIDS; j++) {
                if (kill(pid_array[j], SIGKILL) == 0) {
                    printf("Sending a SIGKILL to all processes.\n", pid_array[j]);
                }
            }

            FILE *file = fopen("WD_log.txt", "a");
            if (file != NULL) {
                fprintf(file, "Process with PID %d is not responding.\n", target_pid);
                fprintf(file, "ALL PROCESSES ARE TERMINATED.\n");
                fclose(file);
            } else {
                perror("Error opening file for writing");
            }
        }
    }
}

    // Unmap the shared memory for PIDs
    if (munmap(pid_array, MAX_PIDS * sizeof(pid_t)) == -1) {
        perror("munmap failed");
        return -1;
    }

    // Unlink the shared memory object for PIDs
    if (shm_unlink(SHM_pids) == -1) {
        perror("shm_unlink failed");
        return -1;
    }

    sem_close(sem_id1);
    sem_close(sem_id2);

    return 0;
}