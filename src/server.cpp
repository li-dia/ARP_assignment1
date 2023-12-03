#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define SHMOBJ_PATH "/shm_POS"
#define SEM_PATH_1 "/sem_POS_1"
#define SEM_PATH_2 "/sem_POS_2"
#define POS_NUM 2

void handler_srv(int sig, siginfo_t *info, void *context) {
    time_t now;
    time(&now);
    char time_str[30];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Log the information to the file
    FILE *log_file = fopen("logs/server_log.txt", "a");
    if (log_file != NULL) {
        fprintf(log_file, "Signal %d received at %s\n", sig, time_str);
        fclose(log_file);
    } else {
        perror("Error opening log file in signal handler");
    }
}

double *position_array;

int main() {
    int shared_seg_size = (2 * sizeof(double));

    // Create or open shared memory
    int shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, 0666);
    if (shmfd == -1) {
        perror("Error creating/opening shared memory");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shmfd, shared_seg_size) == -1) {
        perror("Error truncating shared memory");
        exit(EXIT_FAILURE);
    }
    position_array = (double *)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (position_array == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }

    // Create or open semaphores
    sem_t *sem_id1 = sem_open(SEM_PATH_1, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem_id1 == SEM_FAILED) {
        perror("Error creating/opening semaphore 1");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_id2 = sem_open(SEM_PATH_2, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem_id2 == SEM_FAILED) {
        perror("Error creating/opening semaphore 2");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores
    if (sem_init(sem_id1, 1, 1) == -1) {
        perror("Error initializing semaphore 1");
        exit(EXIT_FAILURE);
    }
    if (sem_init(sem_id2, 1, 0) == -1) {
        perror("Error initializing semaphore 2");
        exit(EXIT_FAILURE);
    }

    // Set up signal handling
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler_srv;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }

    FILE *log_file = fopen("logs/position_log.txt", "w");
    if (log_file == NULL) {
        perror("position Error opening log file");
        exit(EXIT_FAILURE);
    }

    double prev_x = 0.0;
    double prev_y = 0.0;


    while (1) {
    /* Wait for the writer */
    int sem_wait_result;
    do {
        sem_wait_result = sem_wait(sem_id2);
    } while (sem_wait_result == -1 && errno == EINTR);

    if (sem_wait_result == -1) {
        perror("server: Error waiting for semaphore 2");
        exit(EXIT_FAILURE);
    }

    /* Get shared data */
    double current_x = position_array[0];
    double current_y = position_array[1];

    // Check if the values have changed before writing to the log file
    if (current_x != prev_x || current_y != prev_y) {
        time_t now;
        time(&now);
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(log_file, "Coordinates updated to (%f, %f) at %s\n", current_x, current_y, time_str);
    }

    // Update previous values
    prev_x = current_x;
    prev_y = current_y;

    // Close the file to ensure data is written immediately
    if (fflush(log_file) == EOF) {
        perror("Error flushing log file");
        exit(EXIT_FAILURE);
    }

    /* Restart the writer */
    if (sem_post(sem_id1) == -1) {
        perror("Error posting to semaphore 1");
        exit(EXIT_FAILURE);
    }
}
    // Clean up and exit
    if (munmap(position_array, shared_seg_size) == -1) {
        perror("Error unmapping shared memory");
        exit(EXIT_FAILURE);
    }
    if (close(shmfd) == -1) {
        perror("Error closing shared memory descriptor");
        exit(EXIT_FAILURE);
    }

    if (sem_close(sem_id1) == -1) {
        perror("Error closing semaphore 1");
        exit(EXIT_FAILURE);
    }
    if (sem_close(sem_id2) == -1) {
        perror("Error closing semaphore 2");
        exit(EXIT_FAILURE);
    }

    if (fclose(log_file) == EOF) {
        perror("Error closing log file");
        exit(EXIT_FAILURE);
    }

    return 0;
}
