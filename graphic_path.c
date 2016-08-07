#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_X   10
#define BOARD_Y   10
#define TILE_SIZE 20
#define PERIOD    250

#define WHITE    255, 255, 255, 255
#define GREY     128, 128, 128, 255
#define GREEN      0, 255,   0, 255
#define RED      255,   0,   0, 255
#define LGREY    200, 200, 200, 255
#define BLACK      0,   0,   0, 255

SDL_Renderer     *renderer;
SDL_Window       *window;
SDL_RendererInfo  renderer_info;
SDL_Event         e;

int board[BOARD_Y][BOARD_X] = {0};
int overlaid[BOARD_Y][BOARD_X] = {0};

typedef struct Coordinate {
    int x, y;
} Coordinate;

typedef struct Unit {
    Coordinate loc, target;
} Unit;

static inline int*
tile_from_mouseover (int x, int y)
{
    return &board[y / TILE_SIZE][x / TILE_SIZE];
}

static inline int
tile_in_array (int *unit, int **array, int index)
{
    int i;
    for (i = 0; i < index; i++)
        if (array[i] == unit)
            return 1;
    return 0;
}

static inline void
toggle_mouseover_tile (int *c, int **arr, int *len, int mousedown)
{
    if (!tile_in_array(c, arr, *len) && mousedown) {
        *c = !*c;
        arr[*len] = c;
        if (*len < BOARD_X * BOARD_Y)
            (*len)++;
    }
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
    static Coordinate last_visited = {-1, -1};
    static const Coordinate directions[8] = {
        {0, 1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1, 0}, {-1,1}
    };

    Coordinate dir = direction(u->target, u->loc);
    Coordinate next = add_coords(u->loc, dir);
    Coordinate shortest;
    int distances[8] = {0};
    float d, shortest_d = 0.0;
    int x, y, i, j, r;

    ///* test if the preferred direction is available */
    //for (i = 1; i <= r; i++) {
    //    x = u->loc.x + (dir.x * i);
    //    y = u->loc.y + (dir.y * i);

    //    if (x < 0 || x >= BOARD_X || y < 0 || y >= BOARD_Y)
    //        continue;

    //    /* if one of the squares is the target */
    //    if (y == u->target.y && x == u->target.x)
    //        break;

    //    /* a non-empty square */
    //    if (board[y][x] != 0)
    //        goto look_around;
    //}

    //last_visited = u->loc;
    //u->loc = next;
    //goto exit;

//look_around:
    /* look around and find the limits of each direction (limited by r) */
    for (i = 1; i; i++) {
        r = 0;
        for (j = 0; j < 8; j++) {
            /* only do checks if direction hasn't been limited */
            if (distances[j] == i - 1) {
                x = u->loc.x + (directions[j].x * i);
                y = u->loc.y + (directions[j].y * i);

                if (x < 0 || x >= BOARD_X || y < 0 || y >= BOARD_Y)
                    continue;

                /* if one of the squares is the target */
                if (y == u->target.y && x == u->target.x) {
                    last_visited = u->loc;
                    u->loc = next;
                    return;
                }

                /* a non-empty square */
                if (board[y][x] == 1)
                    continue;

                r++;
                distances[j]++;
                overlaid[y][x] = 1;
            }
        }

        /* if maxed out distances */
        if (r == 0)
            break;
    }
    
    /*
     * TODO:
     * 
     * How do I negatively weight the neighbors of L in the direction of the
     * target which are filled with obstacles? This is different than just
     * negatively weighting neighbors with just have obstacles.
     *
     * Maybe, if target's direction is -1, -1 of L, the neighbors at -1, 0; 
     * 0, -1; and -1, -1 fall under this weighting check because it is those 
     * which directly affect our forward movement.
     */

    /* At the limit of each direction L, score the neighbors of L */
    for (i = 0; i < 8; i++) {
        if (distances[i] == 0)
            continue;

        x = u->loc.x + (directions[i].x * distances[i]);
        y = u->loc.y + (directions[i].y * distances[i]);

        d = distance((Coordinate){x, y}, u->target);

        /* score immediately neighboring squares */
        for (j = 0; j < 8; j++) {
            next = (Coordinate) {
                .x = x + directions[j].x,
                .y = y + directions[j].y
            };

            if (next.x < 0 || next.x >= BOARD_X || 
                next.y < 0 || next.y >= BOARD_Y)
                continue;

            /* if one of the neighbors is the target */
            if (next.y == u->target.y && next.x == u->target.x) {
                shortest = (Coordinate){x, y};
                goto step;
            }
            
            /* a non-empty square */
            if (board[next.y][next.x] == 1)
                d = powf(d, 1.5);
                //d *= 1.5;
        }

        //if ((shortest_d == 0.0 || shortest_d > d) &&
        //    (x != last_visited.x && y != last_visited.y)) {
        if (shortest_d == 0.0 || shortest_d > d) {
            shortest = (Coordinate){x, y};
            shortest_d = d;
        }

        printf("%d, %d: %f\n", directions[i].x, directions[i].y, d);
    }
    printf("\n");

step:
    last_visited = u->loc;
    u->loc = add_coords(u->loc, direction(shortest, u->loc));

exit:
    return;
}

int
main (int argc, char **argv)
{
    Unit unit = { {1, 8}, {8, 1} };

    uint32_t current_time, last_time;
    int *tile, *mouseover_tiles[BOARD_X * BOARD_Y];
    int mouseover_index, mousedown, shift_down, save, step, paused;
    int x, y;
    SDL_Rect r;

    //board[1][1] = 1; board[2][1] = 1; board[3][1] = 1; board[3][2] = 1; board[3][3] = 1; board[3][4] = 1; board[3][5] = 1; board[3][6] = 1; board[3][7] = 1; board[3][8] = 1; board[4][4] = 1; board[4][7] = 1; board[5][7] = 1; board[6][7] = 1;

    //board[1][5] = 1; board[2][5] = 1; board[3][5] = 1; board[4][5] = 1; board[4][6] = 1; board[4][7] = 1; board[4][8] = 1;
    //unit.target = (Coordinate) { 6, 3 };

    //board[2][1] = 1; board[2][2] = 1; board[2][3] = 1; board[2][4] = 1; board[2][5] = 1; board[2][6] = 1; board[2][7] = 1; board[2][8] = 1; board[3][1] = 1; board[3][8] = 1; board[4][1] = 1; board[4][8] = 1; board[5][1] = 1; board[5][8] = 1;
    //unit.target = (Coordinate) {4, 1};
    //unit.loc = (Coordinate) {4, 8};
    
    board[1][1] = 1; board[1][8] = 1; board[2][1] = 1; board[2][8] = 1; board[3][1] = 1; board[3][8] = 1; board[4][1] = 1; board[4][8] = 1; board[5][1] = 1; board[5][2] = 1; board[5][7] = 1; board[5][8] = 1; board[6][2] = 1; board[6][3] = 1; board[6][4] = 1; board[6][6] = 1; board[6][7] = 1; board[7][4] = 1; board[7][5] = 1; board[7][6] = 1;
    unit.target = (Coordinate) {5, 5};

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(BOARD_X * TILE_SIZE, BOARD_Y * TILE_SIZE,
      SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_GetRendererInfo(renderer, &renderer_info);

    current_time    = SDL_GetTicks();
    last_time       = current_time;
    paused          = 1;
    step            = 0;
    save            = 0;
    mouseover_index = 0;
    mousedown       = 0; 
    shift_down      = 0;

    while(1) {   
        current_time = SDL_GetTicks();

        SDL_RenderClear(renderer);

        for (y = 0; y < BOARD_Y; y++) {
            for (x = 0; x < BOARD_X; x++) {
                if (board[y][x] == 1)
                    SDL_SetRenderDrawColor(renderer, BLACK);
                else
                    SDL_SetRenderDrawColor(renderer, WHITE);
                r = (SDL_Rect) {x*TILE_SIZE, y*TILE_SIZE,TILE_SIZE,TILE_SIZE};
                SDL_RenderFillRect(renderer, &r); 
            }
        }

        r = (SDL_Rect) {unit.target.x*TILE_SIZE, 
            unit.target.y*TILE_SIZE, TILE_SIZE, TILE_SIZE };
        SDL_SetRenderDrawColor(renderer, RED);
        SDL_RenderFillRect(renderer, &r); 

        for (y = 0; y < BOARD_Y; y++) {
            for (x = 0; x < BOARD_X; x++) {
                if (overlaid[y][x] == 1) {
                    r = (SDL_Rect) {x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE};
                    SDL_SetRenderDrawColor(renderer, LGREY);
                    SDL_RenderFillRect(renderer, &r); 
                }
            }
        }

        r = (SDL_Rect) {unit.loc.x*TILE_SIZE,
            unit.loc.y*TILE_SIZE,TILE_SIZE,TILE_SIZE };
        SDL_SetRenderDrawColor(renderer, GREEN);
        SDL_RenderFillRect(renderer, &r); 

        SDL_RenderPresent(renderer);
input:
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                case SDLK_ESCAPE: case SDL_QUIT:
                    if (shift_down)
                        save = 1;
                    goto quit;
                    break;

                case SDLK_SPACE:
                    paused = !paused;
                    break;

                case SDLK_RIGHT:
                    if (paused && !step)
                        step = 1;
                    break;

                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    shift_down = 1;
                    break;
                }
                break;

            case SDL_KEYUP:
                switch(e.key.keysym.sym) {
                /* when stop pressing shift, back to regular unit type */
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    shift_down = 0;
                    break;
                }
                break;

            case SDL_MOUSEMOTION:
                tile = tile_from_mouseover(e.button.x, e.button.y);
                toggle_mouseover_tile(tile, mouseover_tiles, 
                        &mouseover_index, mousedown);
                break;

            case SDL_MOUSEBUTTONUP:
                mousedown = 0;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (shift_down) {
                    switch (e.button.button) {
                    case SDL_BUTTON_LEFT:
                        unit.loc = (Coordinate) {
                            e.button.x / TILE_SIZE, 
                            e.button.y / TILE_SIZE
                        };
                        break;

                    case SDL_BUTTON_RIGHT:
                        unit.target = (Coordinate) {
                            e.button.x / TILE_SIZE, 
                            e.button.y / TILE_SIZE
                        };
                        break;
                    };
                } else {
                    mousedown = 1;
                    mouseover_index = 0;
                    tile = tile_from_mouseover(e.button.x, e.button.y);
                    toggle_mouseover_tile(tile, mouseover_tiles, 
                            &mouseover_index, mousedown);
                }
                break;
            }
        }

        if ((paused && step) || 
            (!paused && (current_time - last_time > PERIOD))) {
            for (y = 0; y < BOARD_Y; y++)
                for (x = 0; x < BOARD_X; x++)
                    overlaid[y][x] = 0;
            step = 0;
            move_unit(&unit);
            last_time = current_time;
        }
    }

quit:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    if (save) {
        for (y = 0; y < BOARD_Y; y++)
            for (x = 0; x < BOARD_X; x++)
                if (board[y][x])
                    printf("board[%d][%d] = 1;\n", y, x);
        printf("unit.target = (Coordinate) {%d, %d};\n", unit.target.x, unit.target.y);
        printf("unit.loc = (Coordinate) {%d, %d};\n", unit.loc.x, unit.loc.y);
    }

    return 0;
}
