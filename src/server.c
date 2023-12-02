// initializer.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>

#define SHMOBJ_PATH "/shm_AOS"
#define SEM_PATH_1 "/sem_AOS_1"
#define SEM_PATH_2 "/sem_AOS_2"
#define POS_NUM 2

double *position_array;

int main() {
    int shared_seg_size = (2 * sizeof(double));

    // Create or open shared memory
    int shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, 0666);
    ftruncate(shmfd, shared_seg_size);
    position_array = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    // Create or open semaphores
    sem_t *sem_id1 = sem_open(SEM_PATH_1, O_CREAT, S_IRUSR | S_IWUSR, 0);
    sem_t *sem_id2 = sem_open(SEM_PATH_2, O_CREAT, S_IRUSR | S_IWUSR, 0);

    // Initialize semaphores
    sem_init(sem_id1, 1, 1); // initialized to 1
    sem_init(sem_id2, 1, 0); // initialized to 0

    FILE *log_file = fopen("logs/position_log.txt", "w");
    if (log_file == NULL) {
    perror("Error opening log file");
    exit(EXIT_FAILURE);}

    double prev_x = 0.0;
    double prev_y = 0.0;


    while (1)
    {
         /* Wait writer */
        sem_wait(sem_id2);
        printf("data read \n");

        /* Get shared data */
        double current_x = position_array[0];
        double current_y = position_array[1];

        // Check if the values have changed before writing to the log file
        if (current_x != prev_x || current_y != prev_y) {
            fprintf(log_file, "Coordinates updated to (%f, %f)\n", current_x, current_y);
            printf("Coordinates updated to (%f, %f)\n", current_x, current_y);
        }

        // Update previous values
        prev_x = current_x;
        prev_y = current_y;

        // Close the file to ensure data is written immediately
        fflush(log_file);

        /* Restart writer */
        sem_post(sem_id1);
    }
    

    // Clean up and exit
    munmap(position_array, shared_seg_size);
    close(shmfd);

    sem_close(sem_id1);
    sem_close(sem_id2);

    fclose(log_file);  // Close the log file

    return 0;
}
