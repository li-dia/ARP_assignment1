#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>

/* Paths */
#define SHMOBJ_PATH "/shm_POS"
#define SEM_PATH_1 "/sem_POS_1"
#define SEM_PATH_2 "/sem_POS_2"

/* Constants */
#define FORCE 1.0
#define M 1.0
#define K 1.0
#define T 0.1
#define DRONE_CHAR 'X'
#define WIDTH 100
#define HEIGHT 50

/* Structures */
typedef struct {
    double x;  // current position on x axis 
    double y;  // current position on y axis 
    double x1; // position on x axis at (i-1)
    double y1; // position on y axis at (i-1)
    double x2; // position on x axis at (i-2)
    double y2; // position on y axis at (i-2)
    double fx; // movement force on x axis 
    double fy; // movement force on y axis 

} Drone;

/* Functions */
void handler_dyn(int sig, siginfo_t *info, void *context);
void init_ncurses();
void init_drone(Drone *drone);
void draw_window();
void draw_drone(int x, int y);
void euler_method(int key, Drone *drone);
void write_data_to_shm(Drone *drone, sem_t *sem_id1, sem_t *sem_id2);

// void draw_window();
// void draw_drone(Drone *drone);
// void move_drone(Drone *drone);
// void handle_input(Drone *drone);

double *position_array;

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
        euler_method(ch, &drone);

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

void handler_dyn(int sig, siginfo_t *info, void *context) {
    time_t now;
    time(&now);
    char time_str[30];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    FILE *log_file = fopen("logs/dynamics_log.txt", "a");
    if (log_file != NULL) {
        fprintf(log_file, "Signal %d received at %s\n", sig, time_str);
        fclose(log_file);
    } else {
        perror("Error opening log file in signal handler");
    }
}


void init_ncurses() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    timeout(0);
}

void init_drone(Drone *drone) {
    drone->x = WIDTH / 2.0;
    drone->y = HEIGHT / 2.0;
    drone->x1 = 0.0;
    drone->y1 = 0.0;
    drone->x2 = 0.0;
    drone->y2 = 0.0;
    drone->fx = 0.0;
    drone->fy = 0.0;
}

void draw_window() {
    clear();
    border('|', '|', '-', '-', ' ', ' ', ' ', ' '); 
}

void draw_drone(int x, int y) {
    attron(COLOR_PAIR(1));
    mvprintw(y, x, "%c", DRONE_CHAR);
    mvprintw(y - 1, x, " ");
    mvprintw(y + 1, x, " ");
    mvprintw(y, x - 1, " ");
    mvprintw(y, x + 1, " ");
    attroff(COLOR_PAIR(1));
}

void euler_method(int key, Drone *drone) {

    switch (key) {
        case 'q':
            endwin();
            exit(0);
        case KEY_UP:
            printw("I changed the force TO GO UP \n");
            drone->fy -= FORCE;
            // drone->vy -= FORCE;
            break;
        case KEY_LEFT:
            printw("I changed the force TO GO DOWN \n");
            drone->fx -= FORCE;
            // drone->vx -= FORCE;
            break;
        case KEY_DOWN:
            // drone->vy += FORCE;
            drone->fy += FORCE;
            break;
        case KEY_RIGHT:
            drone->fx += FORCE;
            // drone->vx += FORCE;
            break;
        case 'u':

            if (drone->fx >= drone->fy) drone->fy = drone->fx;
            else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx -= FORCE ;
            drone->fy -= FORCE ;

            break;
        case 'i':
            if (drone->fx >= drone->fy) drone->fy = drone->fx;
            else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx += FORCE ;
            drone->fy -= FORCE ;
            break;
        case 'j':
            if (drone->fx >= drone->fy) drone->fy = drone->fx;
            else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx -= FORCE ;
            drone->fy += FORCE ;
            break;
        case 'k':
            
            if (drone->fx >= drone->fy) drone->fy = drone->fx;
            else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx += FORCE ;
            drone->fy += FORCE ;
            break;
        case 'r':
            drone->fx = 0 ;
            drone->fy = 0 ;
            break;

        case 'a':
            init_drone(drone);
            break;
    }

    // Euler's method for position calculation
    // printw("I changed the position \n");
    drone->x = ((2 * M + K * T) * drone->x1 - M * drone->x2 + T * T * drone->fx) / (K * T + M);
    drone->y = ((2 * M + K * T) * drone->y1 - M * drone->y2 + T * T * drone->fy) / (K * T + M);

    // Updating the previous positions accordingly
    
    drone->x2 = drone->x1;
    drone->y2 = drone->y1;
    drone->x1 = drone->x;
    drone->y1 = drone->y;


    // drone->x += drone->vx * T;
    // drone->y += drone->vy * T;

    // Ensure the drone stays within the window bounds
    if (drone->x < 1) drone->x = 1;
    if (drone->x >= WIDTH - 1) drone->x = WIDTH - 2;
    if (drone->y < 1) drone->y = 1;
    if (drone->y >= HEIGHT - 1) drone->y = HEIGHT - 2;
}


void write_data_to_shm(Drone *drone, sem_t *sem_id1, sem_t *sem_id2) {
    sem_wait(sem_id1); // wait for the reader
    position_array[0] = drone->x;
    position_array[1] = drone->y;
    sem_post(sem_id2); // start the read
}
