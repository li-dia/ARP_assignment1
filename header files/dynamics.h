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

/*            Paths            */

#define SHMOBJ_PATH "/shm_AOS"
#define SEM_PATH_1 "/sem_AOS_1"
#define SEM_PATH_2 "/sem_AOS_2"

/*            Constants            */

#define WIDTH 80
#define HEIGHT 24
#define DRONE_CHAR '+'
#define TARGET_CHAR 'O'
#define OBSTACLE_CHAR '*'

/*            Structures               */

typedef struct {
    int x;
    int y;
    double force_x;
    double force_y;
} Drone;

/*            Functions              */

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
