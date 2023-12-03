
#include "./../headerFiles/dynamics.h"

/*         Prototypes         */
void handler_dyn(int sig, siginfo_t *info, void *context);
void init_ncurses();
void init_drone(Drone *drone);
void draw_window();
void draw_drone(int x, int y);
void drone_movement(int key, Drone *drone);
void write_data_to_shm(Drone *drone, sem_t *sem_id1, sem_t *sem_id2);


int main() {
    srand(time(NULL));

    init_ncurses();
    Drone drone ;
    init_drone(&drone);
    int ch;

    int shared_seg_size = 2 * sizeof(double);

    // Open shared memory
    int shmfd = shm_open(SHMOBJ_PATH, O_RDWR, 0666);
    if (shmfd == -1) {
        perror("Error opening shared memory");
        exit(EXIT_FAILURE);
    }
    position_array = (double *)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (position_array == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }

    // Open semaphores
    sem_t *sem_id1 = sem_open(SEM_PATH_1, 0);
    if (sem_id1 == SEM_FAILED) {
        perror("Error opening semaphore 1");
        exit(EXIT_FAILURE);
    }
    sem_t *sem_id2 = sem_open(SEM_PATH_2, 0);
    if (sem_id2 == SEM_FAILED) {
        perror("Error opening semaphore 2");
        exit(EXIT_FAILURE);
    }

    // Set up signal handling
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler_dyn;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }

    while (1) {

        draw_window();
        draw_drone((int)drone.x, (int)drone.y);
        ch = getch();
        drone_movement(ch, &drone);

        write_data_to_shm(&drone, sem_id1, sem_id2);
        refresh();
        usleep(10000); // sleep for 10ms (smoother movement)
    }

    endwin();

    // Clean all and exit
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

    return 0;
}
