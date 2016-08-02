#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BOARD_X   10
#define BOARD_Y   10
#define TILE_SIZE 10
#define PERIOD    250

#define WHITE    255, 255, 255, 255

SDL_Renderer     *renderer;
SDL_Window       *window;
SDL_RendererInfo  renderer_info;
SDL_Event         e;

enum Team {
    DEAD = 0, 
    RED  = 10, 
    BLUE = 20
};

enum Type {
    NONE = 0,
    LITE = 1,
    REG  = 2,
    DARK = 3
};

typedef struct Coordinate {
    int x, y;
    float distance;
} Coordinate;

typedef struct Unit {
    /* coords will change depending on hue of units, darker being stronger */
    Coordinate target_friendly;
    Coordinate target_enemy;
    int nearby_friendlies;
    int nearby_enemies;
    int team;
    int type;
} Unit;

typedef struct Board {
    Unit unit[BOARD_Y][BOARD_X];
    Unit next[BOARD_Y][BOARD_X];
} Board;

typedef void (*BoardFunction)(Board*, int, int);

static inline Coordinate
direction (Coordinate *c, int x, int y)
{
    Coordinate d = { c->x - x, c->y - y };
    if (d.x > 0) d.x = 1;
    if (d.x < 0) d.x = -1;
    if (d.y > 0) d.y = 1;
    if (d.y < 0) d.y = -1;
    return d;
}

static inline float
distance (float x1, float y1, float x2, float y2)
{
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

/*
 * x, y is the current unit's coordinates. c is the type of target unit 
 * (friendly or enemy). m, n is the coordinates of whichever unit type we're
 * testing.
 *
 * If the unit@x,y is friendly and they're of the same type (dark, reg, lite) 
 * then the closest one becomes the target. If the unit@x,y is a higher type
 * (e.g. darker > reg > lite) then it becomes target over any closer lesser
 * types.
 */
static inline void
set_target (Board *b, Coordinate *t, int x, int y, int m, int n)
{
    float d;

    d = distance(x, y, m, n);

    /* if the potential target is of a lesser type */
    if (b->unit[y][x].type > b->unit[n][m].type)
        return;

    /* if current target has greater type than potential target */
    if (b->unit[t->y][t->x].type > b->unit[n][m].type)
        return;

    /* no target has been set */
    if (t->distance == 0)
        goto set;

    /* if target has been set but a greater unit has been found */
    if (b->unit[n][m].type > b->unit[t->y][t->x].type)
        goto set;

    /* if a greater unit was already found but this is a closer one */
    if (t->distance > d && b->unit[t->y][t->x].type > b->unit[y][x].type)
        goto set;

    /* if the distance is closer & units are same type */
    if (t->distance > d && b->unit[y][x].type == b->unit[n][m].type) {
set:
        t->x = m;
        t->y = n;
        t->distance = d;
    }
}

void
update_neighbors (Board *b, int ux, int uy)
{
    const int radius = 4;
    int x_min, x_max, y_min, y_max, x, y;
    Unit *u = &b->unit[uy][ux];

    if (u->team == DEAD)
        return;

    for (x_min = ux; x_min > (ux - radius) && x_min > 0; x_min--) ;
    for (x_max = ux; x_max < (ux + radius + 1) && x_max < BOARD_X; x_max++) ;
    for (y_min = uy; y_min > (uy - radius) && y_min > 0; y_min--) ;
    for (y_max = uy; y_max < (uy + radius + 1) && y_max < BOARD_Y; y_max++) ;

    //printf("%d| x_min: %d, x_max: %d\n", ux, x_min, x_max);
    //printf("%d| y_min: %d, y_max: %d\n", uy, y_min, y_max);

    //printf("(%d, %d) @ radius %d :  (%d, %d) -> (%d, %d)\n", ux, uy, radius,
    //        x_min, y_min, x_max, y_max);

    for (y = y_min; y < y_max; y++) {
        for (x = x_min; x < x_max; x++) {
            if (&b->unit[y][x] == u || b->unit[y][x].team == DEAD)
                continue;

            if (u->team != b->unit[y][x].team) {
                u->nearby_enemies++;
                set_target(b, &u->target_enemy, ux, uy, x, y);
            }
            else {
                u->nearby_friendlies++;
                set_target(b, &u->target_friendly, ux, uy, x, y);
            }
        }
    }
}

void
each_unit (BoardFunction f, Board *b)
{
    int x, y;
    for (y = 0; y < BOARD_Y; y++)
        for (x = 0; x < BOARD_X; x++)
            f(b, x, y);
}

static inline void
set_unit (Unit units[BOARD_Y][BOARD_X], int x, int y, enum Team t, enum Type f)
{
    units[y][x] = (Unit) { 
        .target_friendly = (Coordinate) {x, y, 0}, 
        .target_enemy = (Coordinate) {0, 0, 0}, 
        .nearby_friendlies = 0, 
        .nearby_enemies = 0, 
        .team = t, 
        .type = f 
    };
}

void
init (Board *b, int x, int y)
{
    set_unit(b->unit, x, y, DEAD, NONE);
    set_unit(b->next, x, y, DEAD, NONE);
}

void
print (Board *b, int x, int y)
{
    SDL_Rect r = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
    switch (b->unit[y][x].team + b->unit[y][x].type) {
    case RED + LITE:
        SDL_SetRenderDrawColor(renderer, 255, 102, 102, 255);
        break;
    case RED + REG:
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        break;
    case RED + DARK:
        SDL_SetRenderDrawColor(renderer, 128, 0, 0, 255);
        break;
    case BLUE + LITE:
        SDL_SetRenderDrawColor(renderer, 128, 128, 255, 255);
        break;
    case BLUE + REG:
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        break;
    case BLUE + DARK:
        SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255);
        break;
    default:
        SDL_SetRenderDrawColor(renderer, WHITE);
        break;
    }
    SDL_RenderFillRect(renderer, &r); 
}

/*
 * Set each cell into its next generation.
 */
void
next (Board *board, int x, int y)
{
    board->unit[y][x] = board->next[y][x];
}

void
tick (Board *board, int x, int y)
{
    Coordinate dir;
    Unit *u = &board->unit[y][x];

    if (u->team == DEAD)
        return;

    //printf("%d, %d friendlies: %d\n", x, y, u->nearby_friendlies);

    /* if there are nearbly friendlies, move to the target one */
    if (u->nearby_friendlies > 0) {
        dir = direction(&u->target_friendly, x, y);
        //printf("%d, %d : Target friendly: %d, %d\n", x, y, dir.x, dir.y);
        //printf("%d, %d : Target friendly: %d, %d\n", x, y, 
        //      u->target_friendly.x, u->target_friendly.y);
        if (board->next[y + dir.y][x + dir.x].team == DEAD) {
            board->next[y + dir.y][x + dir.x] = *u;
            set_unit(board->next, x, y, DEAD, NONE);
        }
    }
}

/*
 * Print and step from this generation into the next.
 */
void
step (Board *board)
{
    int i;
    each_unit(update_neighbors, board);
    /* copy the units to the next portion so each unit's position is known */
    for(i = 0; i < BOARD_X; i++)
        memcpy(&board->next[i], &board->unit[i], sizeof(board->unit[0]));
    each_unit(tick, board);
    each_unit(next, board);
}

int
main (int argc, char **argv)
{
    Board game;
    Unit *unit;
    uint32_t current_time, last_time;
    int paused, mousedown, mouseover_index;
    int *mouseover_units[BOARD_X * BOARD_Y];

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(BOARD_X * TILE_SIZE, BOARD_Y * TILE_SIZE,
      SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_GetRendererInfo(renderer, &renderer_info);

    each_unit(init, &game);
    current_time    = SDL_GetTicks();
    last_time       = current_time;
    unit            = NULL;
    paused          = 0;
    mousedown       = 0;
    mouseover_index = 0;

    set_unit(game.unit, 1, 1, BLUE, REG);
    set_unit(game.unit, 3, 2, BLUE, REG);
    set_unit(game.unit, 4, 2, BLUE, REG);
    set_unit(game.unit, 5, 1, BLUE, REG);
    set_unit(game.unit, 7, 3, BLUE, DARK);
                              
    set_unit(game.unit, 8, 8,  RED, REG);
    set_unit(game.unit, 6, 6,  RED, REG);
    set_unit(game.unit, 5, 5,  RED, REG);
    set_unit(game.unit, 6, 8,  RED, REG);
    set_unit(game.unit, 3, 8,  RED, DARK);

    while(1) {   
        current_time = SDL_GetTicks();

        SDL_RenderClear(renderer);
        each_unit(print, &game);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                case SDLK_ESCAPE: case SDL_QUIT:
                    goto quit;
                    break;

                case SDLK_SPACE:
                    paused = !paused;
                    break;

                case SDLK_RIGHT:
                case SDLK_LEFT:
                    break;
                }
                break;

                case SDL_MOUSEMOTION:
                    //cell = cell_from_mouseover(e.button.x, e.button.y, &game);
                    //toggle_mouseover_cell(cell, mouseover_cells, 
                    //        &mouseover_index, mousedown);
                    break;

            case SDL_MOUSEBUTTONUP:
                mousedown = 0;
                break;

            case SDL_MOUSEBUTTONDOWN:
                mousedown = 1;
                mouseover_index = 0;
                //cell = cell_from_mouseover(e.button.x, e.button.y, &game);
                //toggle_mouseover_cell(cell, mouseover_cells, &mouseover_index, 
                //        mousedown);
                break;
            }
        }

        if (!paused && (current_time - last_time > PERIOD)) {
            step(&game);
            mouseover_index = 0;
            last_time = current_time;
        }
    }

quit:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
