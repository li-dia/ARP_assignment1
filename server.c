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

char *position_array;

int main() {
    int shared_seg_size = (1 * sizeof(int));

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


    while (1)
    {
        sleep(10);
    }
    

    // Clean up and exit
    munmap(position_array, shared_seg_size);
    close(shmfd);

    sem_close(sem_id1);
    sem_close(sem_id2);

    return 0;
}
