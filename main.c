/*
 * Life of Life
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* gcc -o life main.c -framework SDL2 */

#define BOARD_X   120
#define BOARD_Y   80
#define TILE_SIZE 7
#define LIVE      1
#define DEAD      0
#define WHITE     255, 255, 255, 255
#define BLACK       0,   0,   0, 255
#define HALF_SEC  500

SDL_Renderer     *renderer;
SDL_Window       *window;
SDL_RendererInfo  renderer_info;
SDL_Event         e;

struct Life;
typedef void (*CellFunction)(struct Life*, int, int);

typedef struct Life {
    int cell[BOARD_Y][BOARD_X];
    int next[BOARD_Y][BOARD_X];
    CellFunction tick;
} Life;

/*
 * Count each neighbor either being dead or living.  This board does not loop
 * so edges & corners will have less possible neighbors.
 */
int
cell_neighbors (Life *life, int x, int y, int type)
{
    int neighbors = 0;

    /* top */
    if (y > 0 && life->cell[y - 1][x] == type)
            neighbors++;
    /* right */
    if (x < BOARD_X && life->cell[y][x + 1] == type)
            neighbors++;
    /* bottom */
    if (y < BOARD_Y && life->cell[y + 1][x] == type)
            neighbors++;
    /* left */
    if (x > 0 && life->cell[y][x - 1] == type)
            neighbors++;

    /* top left */
    if ((x > 0 && y > 0) && life->cell[y - 1][x - 1] == type)
            neighbors++;
    /* top right */
    if ((x < BOARD_X && y > 0) && life->cell[y - 1][x + 1] == type)
            neighbors++;
    /* bottom right */
    if ((x < BOARD_X && y < BOARD_Y) && life->cell[y + 1][x + 1] == type)
            neighbors++;
    /* bottom left */
    if ((x > 0 && y < BOARD_Y) && life->cell[y + 1][x - 1] == type)
            neighbors++;

    return neighbors;
}

void
each_cell (CellFunction f, Life *life)
{
    int x, y;
    for (y = 0; y < BOARD_Y; y++)
        for (x = 0; x < BOARD_X; x++)
            f(life, x, y);
}

void
init (Life *life, int x, int y)
{
    life->cell[y][x] = 0;
    life->next[y][x] = 0;
}

void
print (Life *life, int x, int y)
{
    SDL_Rect r = { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
    if (life->cell[y][x])
        SDL_RenderFillRect(renderer, &r); 
}

/*
 * Tick each cell of the game state according to the rules of Life.
 */
void
tick_forward (Life *life, int x, int y)
{
    int neighbors = cell_neighbors(life, x, y, LIVE);

    if (life->cell[y][x]) {
        /* 
         * Any live cell with fewer than two live neighbours dies, as if caused
         * by under-population. Any live cell with more than three live
         * neighbours dies, as if by over-population.
         */
        if (neighbors < 2 || neighbors > 3)
            life->next[y][x] = 0;
        /* 
         * Any live cell with two or three live neighbours lives on to the next
         * generation.
         */
        else
            life->next[y][x] = 1;
    } 
    /* 
     * Any dead cell with exactly three live neighbours becomes a live cell, as
     * if by reproduction.
     */
    else if (neighbors == 3) {
        life->next[y][x] = 1;
    }
}

/*
 * Tick each cell backward according to the reverse rules of Life.
 */
void
tick_backward (Life *life, int x, int y)
{
    int neighbors = cell_neighbors(life, x, y, DEAD);

    if (life->cell[y][x] == 0) {
        /* 
         * Any dead cell with fewer than two dead neighbours becomes a live
         * cell. Any dead cell with more than three dead neighbours becomes a
         * live cell.
         */
        if (neighbors < 2 || neighbors > 3)
            life->next[y][x] = 1;
        /* 
         * Any dead cell with two or three live neighbours stay the same on the
         * next generation.
         */
        else
            life->next[y][x] = 0;
    } 
    /* 
     * Any live cell with exactly three live neighbours becomes a dead cell.
     */
    else if (neighbors == 3) {
        life->next[y][x] = 0;
    }
}

/*
 * Set each cell into its next generation.
 */
void
next (Life *life, int x, int y)
{
    life->cell[y][x] = life->next[y][x];
}

/*
 * Step from this generation into the next.
 */
void
step (Life *life)
{
    each_cell(life->tick, life);
    each_cell(next, life);
}

static inline int*
cell_from_mouseover (int x, int y, Life *life)
{
    return &life->cell[y / TILE_SIZE][x / TILE_SIZE];
}

static inline int
cell_in_array (int *cell, int **array, int index)
{
    int i;
    for (i = 0; i < index; i++)
        if (array[i] == cell)
            return 1;
    return 0;
}

static inline void
toggle_mouseover_cell (int *cell, int **array, int *index, int mousedown)
{
    if (!cell_in_array(cell, array, *index) && mousedown) {
        *cell = !*cell;
        array[*index] = cell;
        if (*index < BOARD_X * BOARD_Y)
            (*index)++;
    }
}

int
main (int argc, char **argv)
{
    Life life;
    uint32_t current_time, last_time;
    int *cell, paused, mousedown, mouseover_index;
    int *mouseover_cells[BOARD_X * BOARD_Y];

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(BOARD_X * TILE_SIZE, BOARD_Y * TILE_SIZE,
      SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_GetRendererInfo(renderer, &renderer_info);

    each_cell(init, &life);
    life.tick       = tick_forward;
    current_time    = SDL_GetTicks();
    last_time       = current_time;
    cell            = NULL;
    paused          = 0;
    mousedown       = 0;
    mouseover_index = 0;

    while(1) {   
        current_time = SDL_GetTicks();

        SDL_SetRenderDrawColor(renderer, WHITE);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, BLACK);
        each_cell(print, &life);
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
                }
                break;

                case SDL_MOUSEMOTION:
                    cell = cell_from_mouseover(e.button.x, e.button.y, &life);
                    toggle_mouseover_cell(cell, mouseover_cells, 
                            &mouseover_index, mousedown);
                    break;

            case SDL_MOUSEBUTTONUP:
                mousedown = 0;
                break;

            case SDL_MOUSEBUTTONDOWN:
                mousedown = 1;
                mouseover_index = 0;
                cell = cell_from_mouseover(e.button.x, e.button.y, &life);
                toggle_mouseover_cell(cell, mouseover_cells, &mouseover_index, 
                        mousedown);
                break;
            }
        }

        if (!paused && (current_time - last_time > HALF_SEC)) {
            step(&life);
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
