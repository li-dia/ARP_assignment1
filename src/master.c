#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define SHM_NAME "/my_shared_memory_pids"
#define SHM_SIZE 1024  // Adjust the size as needed
#define MAX_PIDS 3

// if a process is here give it the key 
int spawn(char *program_path) {
    pid_t child_pid = fork();
    if (child_pid == 0) {
        printf("child's pid %d\n", getpid());
        if (execlp(program_path, program_path, NULL) == -1) { //  ./bin/dynamics
            perror("execlp failed");
            exit(EXIT_FAILURE);
        }
    } else if (child_pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    return child_pid;
}


int main() {
    // Create or open a shared memory object
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    // Set the size of the shared memory segment
    if (ftruncate(shm_fd, sizeof(pid_t) * MAX_PIDS) == -1) {
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory segment into the address space
    pid_t *pid_array = mmap(NULL, sizeof(pid_t) * MAX_PIDS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (pid_array == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    int status;

    // Specify the path to the programs you want to run.
    char *program_path[MAX_PIDS] = {"./bin/dynamics", "./bin/server", "./...."};

    for (int i = 0; i < MAX_PIDS; i++) {
        pid_array[i] = spawn(program_path[i]);
        usleep(10);
        printf("child %d created with pid: %d\n", i, pid_array[i]);
        fflush(stdout);
    }

    // Print the PIDs stored in shared memory
    printf("PIDs stored in shared memory: %d, %d, %d\n", (int)pid_array[0], (int)pid_array[1], (int)pid_array[2]);
    
    // Wait for all child processes to finish
    for (int i = 0; i < MAX_PIDS; i++) {
        wait(&status);
    }


    // Cleanup
    if (munmap(pid_array, sizeof(pid_t) * MAX_PIDS) == -1) {
        perror("munmap failed");
        exit(EXIT_FAILURE);
    }

    if (close(shm_fd) == -1) {
        perror("close failed");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink failed");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
