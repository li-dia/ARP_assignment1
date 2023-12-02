#include <ncurses.h>
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

    drone->x = Q1 / ((M / (T*T)) - (K / T));

    double Q2 = drone->force_y - M / (T*T)  * (drone->y2 - 2 * drone->y1) + K / T * drone->y1;

    drone->y = Q2 / ((M / (T*T)) - (K / T));

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
