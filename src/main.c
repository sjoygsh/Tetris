/* ===========================================================================
 * main.c — Entry point for the Tetris game
 * ===========================================================================
 * Flow: Initialize -> Run game loop -> Cleanup -> Exit
 * =========================================================================== */

#include "game.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Game game;

    if (!game_init(&game)) {
        printf("Failed to initialize. Exiting.\n");
        game_cleanup(&game);
        return 1;
    }

    printf("TETRIS\n");
    printf("Controls: Arrows=Move/Rotate, Space=Drop, P=Pause, R=Restart, Esc=Menu\n");

    game_run(&game);
    game_cleanup(&game);

    printf("Thanks for playing!\n");
    return 0;
}
