#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BOARD_X  10
#define BOARD_Y  10

typedef struct Coordinate {
    int x, y;
} Coordinate;

char board[BOARD_Y][BOARD_X] = {
    {'.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.'},
    {'.','.','|','|','|','|','|','.','.','.'},
    {'.','.','.','.','.','.','|','.','.','.'},
    {'.','.','.','.','.','.','|','.','.','.'},
    {'.','.','.','.','.','.','|','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.'},
    {'.','.','.','.','.','.','.','.','.','.'}
};

char overlaid[BOARD_Y][BOARD_X] = {0};

typedef struct Unit {
    Coordinate loc, target, intermediate;
    int has_intermediate;
} Unit;

typedef void (*BoardFunction)(int, int);

void
each_tile (BoardFunction f)
{
    int x, y;
    for (y = 0; y < BOARD_Y; y++)
        for (x = 0; x < BOARD_X; x++)
            f(x, y);
}

void
print (int x, int y)
{
    move(y, x);
    addch(board[y][x]);
}

static inline Coordinate
direction (const Coordinate target, const Coordinate now)
{
    Coordinate d = { target.x - now.x, target.y - now.y };
    if (d.x > 0) d.x = 1;
    if (d.x < 0) d.x = -1;
    if (d.y > 0) d.y = 1;
    if (d.y < 0) d.y = -1;
    return d;
}

static inline Coordinate
add_coords (const Coordinate a, const Coordinate b)
{
    return (Coordinate) { a.x + b.x, a.y + b.y };
}

static inline float
distance (const Coordinate a, const Coordinate b)
{
    return sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
}

void
move_unit (Unit *u)
{
    static const Coordinate directions[8] = {
        {0, 1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1, 0}, {-1,1}
    };
    static Coordinate last_visited = {-1, -1};

    Coordinate dir = direction(u->target, u->loc);
    Coordinate preferred_step = add_coords(u->loc, dir);
    Coordinate shortest;
    float d, shortest_d = 0;
    int distances[8] = {0};
    int x, y, i, j;
    int r = 5; /* look-around radius */

    /* test if the preferred direction is available */
    for (i = 1; i <= r; i++) {
        x = u->loc.x + (dir.x * i);
        y = u->loc.y + (dir.y * i);

        if (x < 0 || x > BOARD_X || y < 0 || y > BOARD_Y)
            continue;

        /* a non-empty square */
        if (board[y][x] != '.')
            goto look_around;
    }

    last_visited = u->loc;
    u->loc = preferred_step;
    goto exit;

look_around:
    /* look around and find the limits of each direction (limited by r) */
    for (i = 1; i <= r; i++) {
        for (j = 0; j < 8; j++) {
            x = u->loc.x + (directions[j].x * i);
            y = u->loc.y + (directions[j].y * i);

            if (x < 0 || x > BOARD_X || y < 0 || y > BOARD_Y)
                continue;

            if (board[y][x] == '|')
                continue;

            /* only increment the distance if it hasn't been limited */
            if (distances[j] == i - 1) {
                distances[j]++;
                overlaid[y][x] = j + '0';
            }
        }
    }

check_distance:
    /* find lowest distance, preferring directions that haven't been limited */
    for (j = 0; j < 8; j++) {
        if (distances[j] != r)
            continue;

        /* these distances have already been bounds checked above */
        x = u->loc.x + (directions[j].x * r);
        y = u->loc.y + (directions[j].y * r);

        d = distance((Coordinate){x, y}, u->target);
        if (shortest_d == 0 || shortest_d > d) {
            shortest = (Coordinate){x, y};
            shortest_d = d;
        }
    }

    /* lower range if no lowest distance was found */
    if (shortest_d == 0.0) {
        r--;
        goto check_distance;
    }

    last_visited = u->loc;
    u->loc = add_coords(u->loc, direction(shortest, u->loc));

exit:
    return;
}

void
step (Unit *u)
{
    int x, y;
    clear();
    each_tile(print);

    move(u->loc.y, u->loc.x);
    addch('@');

    move(u->target.y, u->target.x);
    addch('X');

    move(0, 11);

    move_unit(u);

    for (y = 0; y < BOARD_Y; y++) {
        for (x = 0; x < BOARD_Y; x++) {
            if (overlaid[y][x] != '\0') {
                move(y, x);
                addch(overlaid[y][x]);
                overlaid[y][x] = '\0';
            }
        }
    }

    refresh();
}

int
main (int argc, char **argv)
{
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    Unit unit;
    unit.loc = (Coordinate) {1, 8};
    unit.target =(Coordinate) {8, 1};
    //Unit unit = { {6, 7}, {8, 1} };

    while(1) {   
        step(&unit);
input:
        switch(getch()) {
        case KEY_LEFT:
        case KEY_RIGHT:
            break;

        case 27: /* ESC */
        case 'q':
            goto exit;
            break;

        default:
            goto input;
            break;
        }
    }

exit:
    clear();
    endwin();

    return 0;
}
