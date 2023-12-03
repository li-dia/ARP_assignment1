#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHM_NAME "/my_shared_memory_pids"
#define SEM_PATH_1 "/sem_PID_1"
#define SEM_PATH_2 "/sem_PID_2"
#define SHM_SIZE 1024  // Adjust the size as needed
#define MAX_PIDS 2

// if a process is here give it the key 
int spawn(char *program_path) {
    pid_t child_pid = fork();
    if (child_pid == 0) {
        //printf("child's pid %d\n", getpid());
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
    pid_t *pid_array = (pid_t*)mmap(NULL, sizeof(pid_t) * MAX_PIDS, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (pid_array == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    int status;

    // Create or open semaphores
    sem_t *sem_id1 = sem_open(SEM_PATH_1, O_CREAT, S_IRUSR | S_IWUSR, 0);
    sem_t *sem_id2 = sem_open(SEM_PATH_2, O_CREAT, S_IRUSR | S_IWUSR, 0);

    // Initialize semaphores
    sem_init(sem_id1, 1, 1); // initialized to 1
    sem_init(sem_id2, 1, 0); // initialized to 0

    sem_wait(sem_id1); //wait reader
    // Specify the path to the programs you want to run.
    char *program_path[MAX_PIDS] = {"./bin/dynamics", "./bin/server"};

    for (int i = 0; i < MAX_PIDS; i++) {
        pid_array[i] = spawn(program_path[i]);
        usleep(10);
        //printf("child %d created with pid: %d\n", i, pid_array[i]);
        fflush(stdout);
    }
    sem_post(sem_id2); //start the read
    // Print the PIDs stored in shared memory
    //printf("PIDs stored in shared memory: %d, %d, %d\n", (int)pid_array[0], (int)pid_array[1], (int)pid_array[2]);
    
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
        perror("master: shm_unlink failed");
        exit(EXIT_FAILURE);
    }

    sem_close(sem_id1);
    sem_close(sem_id2);
    return EXIT_SUCCESS;
}
