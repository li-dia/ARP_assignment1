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
#define WIDTH 80
#define HEIGHT 24
#define DRONE_CHAR '+'

/* Structures */
typedef struct {
    int x;
    int y;
    double force_x;
    double force_y;
} Drone;

/* Functions */
void handler_dyn(int sig, siginfo_t *info, void *context);
void init_ncurses();
void draw_window();
void draw_drone(Drone *drone);
void move_drone(Drone *drone);
void handle_input(Drone *drone);
void write_data_to_shm(Drone *drone, sem_t *sem_id1, sem_t *sem_id2);

double *position_array;

int main() {
    srand(time(NULL));

    init_ncurses();

    Drone drone = {WIDTH / 2, HEIGHT / 2, 0, 0};

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
        draw_drone(&drone);
        move_drone(&drone);
        write_data_to_shm(&drone, sem_id1, sem_id2);
        handle_input(&drone);
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
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_WHITE);    // Drone color (Blue on White)
    init_pair(2, COLOR_GREEN, COLOR_WHITE);   // Target color (Green on White)
    init_pair(3, COLOR_YELLOW, COLOR_WHITE);  // Obstacle color (Yellow on White)

    // Set background color to white
    if (has_colors()) {
        start_color();
        init_pair(4, COLOR_WHITE, COLOR_WHITE);
        bkgd(COLOR_PAIR(4));
    }

    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    timeout(0);
}

void draw_window() {
    clear();
    border(0, 0, 0, 0, 0, 0, 0, 0);
}

void draw_drone(Drone *drone) {
    attron(COLOR_PAIR(1));
    mvprintw(drone->y, drone->x, "%c", DRONE_CHAR);
    mvprintw(drone->y - 1, drone->x, " ");
    mvprintw(drone->y + 1, drone->x, " ");
    mvprintw(drone->y, drone->x - 1, " ");
    mvprintw(drone->y, drone->x + 1, " ");
    attroff(COLOR_PAIR(1));
}

void handle_input(Drone *drone) {
    int ch = getch();

    switch (ch) {
        case 'q':
            endwin();
            exit(0);
        case KEY_UP:
            drone->force_y -= 1.0;
            break;
        case KEY_DOWN:
            drone->force_y += 1.0;
            break;
        case KEY_LEFT:
            drone->force_x -= 1.0;
            break;
        case KEY_RIGHT:
            drone->force_x += 1.0;
            break;
        case 'u':
            drone->force_x -= 1.0;
            drone->force_y -= 1.0;
            break;
        case 'i':
            drone->force_x += 1.0;
            drone->force_y -= 1.0;
            break;
        case 'j':
            drone->force_x -= 1.0;
            drone->force_y += 1.0;
            break;
        case 'k':
            drone->force_x += 1.0;
            drone->force_y += 1.0;
            break;
        case 'r':
            drone->x = WIDTH / 2;
            drone->y = HEIGHT / 2;
            break;
    }
}

void move_drone(Drone *drone) {
    // Add damping factor for smoother movement
    double damping = 0.50;

    drone->x += (int)drone->force_x;
    drone->y += (int)drone->force_y;

    if (drone->x < 1) drone->x = 1;
    if (drone->x >= WIDTH - 1) drone->x = WIDTH - 2;
    if (drone->y < 1) drone->y = 1;
    if (drone->y >= HEIGHT - 1) drone->y = HEIGHT - 2;

    // Apply damping
    drone->force_x *= damping;
    drone->force_y *= damping;
}

void write_data_to_shm(Drone *drone, sem_t *sem_id1, sem_t *sem_id2) {
    sem_wait(sem_id1); // wait for the reader
    position_array[0] = drone->x;
    position_array[1] = drone->y;
    sem_post(sem_id2); // start the read
}
