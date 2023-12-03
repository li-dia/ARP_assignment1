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


/*         Paths         */
#define SHMOBJ_PATH "/shm_POS"
#define SEM_PATH_1 "/sem_POS_1"
#define SEM_PATH_2 "/sem_POS_2"

/*         Constants         */
#define FORCE 1.0
#define M 1.0
#define K 1.0
#define T 0.1
#define DRONE_CHAR 'X'

int WIDTH, HEIGHT;

/*         Structures         */

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


/*         Prototypes         */

void handler_dyn(int sig, siginfo_t *info, void *context);
void init_ncurses();
void init_drone(Drone *drone);
void draw_window();
void draw_drone(int x, int y);
void drone_movement(int key, Drone *drone);
void write_data_to_shm(Drone *drone, sem_t *sem_id1, sem_t *sem_id2);

double *position_array;


/*            Functions              */

/*
* dynamics handler function
*/
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

/*
*  Initialize the ncurses library
*/
void init_ncurses() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    timeout(0);
}

/*
* set the initial position of the drone {0, 0, 0, 0, 0, 0, 0, 0}
*/
void init_drone(Drone *drone) {
    drone->x = 0.0;
    drone->y = 0.0;
    drone->x1 = 0.0;
    drone->y1 = 0.0;
    drone->x2 = 0.0;
    drone->y2 = 0.0;
    drone->fx = 0.0;
    drone->fy = 0.0;
}

/*
* Draw the window the drone moves in (size of the terminal)
*/
void draw_window() {
    clear();
    border((char) 219, (char) 219, (char) 219, (char) 219, (char) 219, (char) 219, (char) 219, (char) 219);

    // Get current window size

    int max_y, max_x;

    getmaxyx(stdscr, max_y, max_x);

    // Update WIDTH and HEIGHT based on the current window size

    WIDTH = max_x;

    HEIGHT = max_y;

        // Print debug information in black

    attron(COLOR_PAIR(5));

    // mvprintw(HEIGHT-4, 0, "width =%d, height=%d", WIDTH, HEIGHT);

    attroff(COLOR_PAIR(5)); 
}


/*
* Draw the drone
*/
void draw_drone(int x, int y) {
    attron(COLOR_PAIR(1));
    mvprintw(y, x, "%c", DRONE_CHAR);
    mvprintw(y - 1, x, " ");
    mvprintw(y + 1, x, " ");
    mvprintw(y, x - 1, " ");
    mvprintw(y, x + 1, " ");
    attroff(COLOR_PAIR(1));
}

/*
* method that handles the keyboard inputs and the drone movement
*/
void drone_movement(int key, Drone *drone) {

    switch (key) {
        case 'q':
            endwin();
            exit(0);
        case 'i':
            // printw("I changed the force TO GO UP \n");
            drone->fy -= FORCE;
            break;
        case 'j':
            // printw("I changed the force TO GO DOWN \n");
            drone->fx -= FORCE;
            break;
        case ',':
            drone->fy += FORCE;
            break;
        case 'l':
            drone->fx += FORCE;
            break;
        case 'u':

            // if (drone->fx >= drone->fy) drone->fy = drone->fx;
            // else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx -= FORCE ;
            drone->fy -= FORCE ;

            break;
        case 'o':
            // if (drone->fx >= drone->fy) drone->fy = drone->fx;
            // else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx += FORCE ;
            drone->fy -= FORCE ;
            break;
        case 'n':
            // if (drone->fx >= drone->fy) drone->fy = drone->fx;
            // else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx -= FORCE ;
            drone->fy += FORCE ;
            break;
        case ';':
            
            // if (drone->fx >= drone->fy) drone->fy = drone->fx;
            // else if(drone->fy > drone->fx) drone->fx = drone->fy;
            drone->fx += FORCE ;
            drone->fy += FORCE ;
            break;
        case 'k':
            drone->fx = 0 ;
            drone->fy = 0 ;
            break;

        case 'a':
            init_drone(drone);
            break;
    }

    // Euler's method for position calculation

    drone->x = ((2 * M + K * T) * drone->x1 - M * drone->x2 + T * T * drone->fx) / (K * T + M);
    drone->y = ((2 * M + K * T) * drone->y1 - M * drone->y2 + T * T * drone->fy) / (K * T + M);

    // Updating the previous positions accordingly
    
    drone->x2 = drone->x1;
    drone->y2 = drone->y1;
    drone->x1 = drone->x;
    drone->y1 = drone->y;

    // Ensure the drone stays within the window bounds
    if (drone->x < 1) drone->x = 1;
    if (drone->x >= WIDTH - 1) drone->x = WIDTH - 2;
    if (drone->y < 1) drone->y = 1;
    if (drone->y >= HEIGHT - 1) drone->y = HEIGHT - 2;
}

/*
* write data to shared memory
*/
void write_data_to_shm(Drone *drone, sem_t *sem_id1, sem_t *sem_id2) {
    sem_wait(sem_id1); // wait for the reader
    position_array[0] = drone->x;
    position_array[1] = drone->y;
    sem_post(sem_id2); // start the read
}
