
/*#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>


#define DRONE_CHAR '+'
#define M 1
#define K 1
#define T 100

int WIDTH,HEIGHT;


typedef struct {
    double x;
    double y;
    double x2;
    double x1;
    double y1;
    double y2;
    double force_x;
    double force_y;
} Drone;

typedef struct {
    int x;
    int y;
    int active;
} Target;

typedef struct {
    int x;
    int y;
    int active;
} Obstacle;

void init_ncurses();
void draw_window();
void draw_drone(Drone *drone);
void move_drone(Drone *drone);
void check_collision(Drone *drone, Target *target, Obstacle *obstacle);
void handle_input(Drone *drone);

int main() {
    srand(time(NULL));

    init_ncurses();

    Drone drone = {0, 0, 0, 0, 0, 0, 0, 0};





    while (1) {

        draw_window();
        draw_drone(&drone);
        move_drone(&drone);
        handle_input(&drone);
        // Print debug information in black
        attron(COLOR_PAIR(5));
        mvprintw(HEIGHT-3, 0, "Position: x=%.2f, y=%.2f", drone.x, drone.y);
        mvprintw(HEIGHT-2, 0, "Previous position: x1=%.2f, y1=%.2f", drone.x1, drone.y1);
        mvprintw(HEIGHT-5, 0, "Previous position: x2=%.2f, y2=%.2f", drone.x2, drone.y2);
        attroff(COLOR_PAIR(5));

        refresh();
        usleep(1000000); // sleep for 10ms (smoother movement)
    }

    endwin();
    return 0;
}

void init_ncurses() {

    initscr();
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_WHITE);    // Drone color (Blue on White)
    init_pair(2, COLOR_GREEN, COLOR_WHITE);   // Target color (Green on White)
    init_pair(3, COLOR_YELLOW, COLOR_WHITE);  // Obstacle color (Yellow on White)
    init_pair(5, COLOR_BLACK, COLOR_WHITE);   // Black text on black background

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
    border((char) 219, (char) 219, (char) 219, (char) 219, (char) 219, (char) 219, (char) 219, (char) 219);


    // Get current window size
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Update WIDTH and HEIGHT based on the current window size
    WIDTH = max_x;
    HEIGHT = max_y;
        // Print debug information in black
    attron(COLOR_PAIR(5));
    mvprintw(HEIGHT-4, 0, "width =%d, height=%d", WIDTH, HEIGHT);
    attroff(COLOR_PAIR(5));
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




void move_drone(Drone *drone) {

    double Q1 = drone->force_x - M * (drone->x2 - 2 * drone->x1) / (T*T)  + K * drone->x1 / T ;

    drone->x = Q1 ;// ((M / (T*T)) - (K / T))

    double Q2 = drone->force_y - M * (drone->y2 - 2 * drone->y1)  / (T*T) + K  * drone->y1 / T;

    drone->y = Q2 ;

    drone->x2 = drone->x1;
    drone->x1 = drone->x;

    drone->y2 = drone->y1;
    drone->y1 = drone->y;
    


    if (drone->x < 1) drone->x = 1;
    if (drone->x >= WIDTH - 1) drone->x = WIDTH - 2;
    if (drone->y < 1) drone->y = 1;
    if (drone->y >= HEIGHT - 1) drone->y = HEIGHT - 2;

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
        case 'Q':
            drone->force_x -= 1.0;
            drone->force_y -= 1.0;
            break;
        case 'E':
            drone->force_x += 1.0;
            drone->force_y -= 1.0;
            break;
        case 'Z':
            drone->force_x -= 1.0;
            drone->force_y += 1.0;
            break;
        case 'C':
            drone->force_x += 1.0;
            drone->force_y += 1.0;
            break;
        case 'r':
            drone->x = WIDTH / 2;
            drone->y = HEIGHT / 2;
            break;
    }
}


*/

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
#define SHMOBJ_PATH "/shm_AOS"
#define SEM_PATH_1 "/sem_AOS_1"
#define SEM_PATH_2 "/sem_AOS_2"

#define WIDTH 80
#define HEIGHT 24
#define DRONE_CHAR '+'
#define TARGET_CHAR 'O'
#define OBSTACLE_CHAR '*'

typedef struct {
    int x;
    int y;
    double force_x;
    double force_y;
} Drone;

typedef struct {
    int x;
    int y;
    int active;
} Target;

typedef struct {
    int x;
    int y;
    int active;
} Obstacle;

void init_ncurses();
void draw_window();
void draw_drone(Drone *drone);
void draw_target(Target *target);
void draw_obstacle(Obstacle *obstacle);
void move_drone(Drone *drone);
void generate_target(Target *target);
void generate_obstacle(Obstacle *obstacle);
void check_collision(Drone *drone, Target *target, Obstacle *obstacle);
void handle_input(Drone *drone);

double *position_array;

int main() {
    srand(time(NULL));

    init_ncurses();

    Drone drone = {WIDTH / 2, HEIGHT / 2, 0, 0};
    Target target = {0, 0, 0};
    Obstacle obstacle = {0, 0, 0};

    generate_target(&target);
    generate_obstacle(&obstacle);

    int shared_seg_size = 2 * sizeof(double);
    
    // Open shared memory
    int shmfd = shm_open(SHMOBJ_PATH, O_RDWR, 0666);
    position_array = (double*)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    // Open semaphores
    sem_t *sem_id1 = sem_open(SEM_PATH_1, 0);
    sem_t *sem_id2 = sem_open(SEM_PATH_2, 0);

    while (1) {

        draw_window();
        draw_drone(&drone);
        draw_target(&target);
        draw_obstacle(&obstacle);

        move_drone(&drone);

        sem_wait(sem_id1); //wait reader
        position_array[0] = drone.x;
        position_array[1] = drone.y;
        //sleep(1);
        sem_post(sem_id2); //start the read

        check_collision(&drone, &target, &obstacle);
        handle_input(&drone);
        
        refresh();
        usleep(10000); // sleep for 10ms (smoother movement)
    }

    endwin();

    /* Clean all and exit */
    sem_close(sem_id1);
    sem_close(sem_id2);
    sem_unlink(SEM_PATH_1);
    sem_unlink(SEM_PATH_2);

    return 0;
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

void draw_target(Target *target) {
    if (target->active) {
        attron(COLOR_PAIR(2));
        mvprintw(target->y, target->x, "%c", TARGET_CHAR);
        attroff(COLOR_PAIR(2));
    }
}

void draw_obstacle(Obstacle *obstacle) {
    if (obstacle->active) {
        attron(COLOR_PAIR(3));
        mvprintw(obstacle->y, obstacle->x, "%c", OBSTACLE_CHAR);
        attroff(COLOR_PAIR(3));
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

void generate_target(Target *target) {
    target->x = rand() % (WIDTH - 2) + 1;
    target->y = rand() % (HEIGHT - 2) + 1;
    target->active = 1;
}

void generate_obstacle(Obstacle *obstacle) {
    obstacle->x = rand() % (WIDTH - 2) + 1;
    obstacle->y = rand() % (HEIGHT - 2) + 1;
    obstacle->active = 1;
}

void check_collision(Drone *drone, Target *target, Obstacle *obstacle) {
    if (drone->x == target->x && drone->y == target->y) {
        target->active = 0;
        generate_target(target);
    }

    if (drone->x == obstacle->x && drone->y == obstacle->y) {
        obstacle->active = 0;
        generate_obstacle(obstacle);
    }
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
        case 'Q':
            drone->force_x -= 1.0;
            drone->force_y -= 1.0;
            break;
        case 'E':
            drone->force_x += 1.0;
            drone->force_y -= 1.0;
            break;
        case 'Z':
            drone->force_x -= 1.0;
            drone->force_y += 1.0;
            break;
        case 'C':
            drone->force_x += 1.0;
            drone->force_y += 1.0;
            break;
        case 'r':
            drone->x = WIDTH / 2;
            drone->y = HEIGHT / 2;
            break;
    }
}

