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

int initial_pos[6] = {10,20,30,40,50,60};

double *position_array;

int i;
int main(int argc, char *argv[]) {

    int shared_seg_size = 2 * sizeof(double);
    
    /// Open shared memory
    int shmfd = shm_open(SHMOBJ_PATH, O_RDWR, 0666);
    position_array = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    // Open semaphores
    sem_t *sem_id1 = sem_open(SEM_PATH_1, 0);
    sem_t *sem_id2 = sem_open(SEM_PATH_2, 0);

    
    while (1)
    {
        sem_wait(sem_id1); //wait reader

        for (i=0; i<2; ++i){
            position_array[i] = initial_pos[i];  
            initial_pos[i] = initial_pos[i] +10;
            }
        //sleep(1);

        sem_post(sem_id2); //start the read
    }
    
    /* Clean all and exit */
    sem_close(sem_id1);
    sem_close(sem_id2);
    sem_unlink(SEM_PATH_1);
    sem_unlink(SEM_PATH_2);

    return 0;
    }

