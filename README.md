# Life

Conway's Game of Life written in C and SDL.

## Requirements

SDL Console requires [SDL2](https://www.libsdl.org/download-2.0.php).
Debian-based distros can use the following command to get the required
packages:

    # apt-get install libsdl2-dev

# Install

Once you have the requirements simply use the makefile, e.g. `make all`. 
Or, since the compilation is simply, compile with `clang` or `gcc` via 
`cc -o life main.c -lSDL2`.

## How to Play

    * Space pauses the game
    * Left-click places a block
    * Right-click clears a placed block
    * Escape exits the game.
