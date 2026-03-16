/* ===========================================================================
 * input.c — Keyboard input handling
 * ===========================================================================
 * Translates keyboard events into game actions. Handles both menu
 * navigation and gameplay controls.
 *
 * CONTROLS:
 *   Menu:   Up/Down = navigate, Enter = select
 *   Game:   Arrows = move/rotate, Space = hard drop
 *           P = pause, R = restart, Esc = quit/back
 * =========================================================================== */

#include "input.h"
#include "board.h"
#include "piece.h"
#include "audio.h"
#include "game.h"

/* Try to move the piece horizontally. dx = -1 (left) or +1 (right). */
static void try_move(Game *game, int dx)
{
    Piece test = game->current;
    test.x += dx;

    if (board_piece_fits(&game->board, &test)) {
        game->current.x += dx;
        audio_play_move(game);
        if (game->lock_timer_active)
            game->lock_timer = SDL_GetTicks();
    }
}

/* Try to rotate clockwise with wall kicks.
 * Wall kicks: if rotation fails at current position, try nudging
 * the piece left/right/up to find a valid position. */
static void try_rotate(Game *game)
{
    Piece test = game->current;
    test.rotation = (test.rotation + 1) % NUM_ROTATIONS;

    /* Try in-place first */
    if (board_piece_fits(&game->board, &test)) {
        game->current.rotation = test.rotation;
        audio_play_rotate(game);
        if (game->lock_timer_active)
            game->lock_timer = SDL_GetTicks();
        return;
    }

    /* Wall kick offsets to try */
    int kicks[][2] = { {-1,0}, {1,0}, {0,-1}, {-2,0}, {2,0} };
    int n = sizeof(kicks) / sizeof(kicks[0]);

    for (int i = 0; i < n; i++) {
        Piece kicked = test;
        kicked.x += kicks[i][0];
        kicked.y += kicks[i][1];
        if (board_piece_fits(&game->board, &kicked)) {
            game->current = kicked;
            audio_play_rotate(game);
            if (game->lock_timer_active)
                game->lock_timer = SDL_GetTicks();
            return;
        }
    }
}

/* Hard drop — instantly drop piece to bottom, create visual effects. */
static void hard_drop(Game *game)
{
    int start_y = game->current.y;

    /* Find the piece's filled columns for the trail effect */
    const int (*shape)[4] = piece_get_shape(game->current.type,
                                            game->current.rotation);
    /* Track which columns have blocks (for drop trails) */
    int trail_cols[4];
    int trail_min_row[4];
    int trail_count = 0;

    for (int col = 0; col < PIECE_GRID_SIZE && trail_count < 4; col++) {
        int min_r = -1;
        for (int row = 0; row < PIECE_GRID_SIZE; row++) {
            if (shape[row][col] && min_r < 0) min_r = row;
        }
        if (min_r >= 0) {
            trail_cols[trail_count] = game->current.x + col;
            trail_min_row[trail_count] = start_y + min_r - 4;
            trail_count++;
        }
    }

    /* Move piece down until it hits something */
    while (board_piece_fits(&game->board, &game->current))
        game->current.y++;
    game->current.y--;

    int end_y = game->current.y;

    /* Only create effects if piece actually moved */
    if (end_y > start_y) {
        /* Create drop trails (vertical streaks) */
        game->drop_trail_count = trail_count;
        Uint32 now = SDL_GetTicks();
        for (int i = 0; i < trail_count; i++) {
            game->drop_trails[i].col = trail_cols[i];
            game->drop_trails[i].start_row = trail_min_row[i];
            /* Find the lowest block in this column of the piece */
            int max_r = 0;
            for (int row = 0; row < PIECE_GRID_SIZE; row++) {
                if (shape[row][trail_cols[i] - game->current.x])
                    max_r = row;
            }
            game->drop_trails[i].end_row = end_y + max_r - 4;
            game->drop_trails[i].start_time = now;
            game->drop_trails[i].active = true;
        }

        /* Spawn burst particles at the landing position */
        for (int col = 0; col < PIECE_GRID_SIZE; col++) {
            for (int row = PIECE_GRID_SIZE - 1; row >= 0; row--) {
                if (shape[row][col]) {
                    int px = BOARD_OFFSET_X + (game->current.x + col) * CELL_SIZE + CELL_SIZE / 2;
                    int py = BOARD_OFFSET_Y + (end_y + row - 4) * CELL_SIZE + CELL_SIZE;
                    spawn_drop_particles(game, px, py);
                    break;  /* Only bottom block of each column */
                }
            }
        }
    }

    audio_play_drop(game);
    game_lock_and_advance(game);
}

/* Handle menu input (arrow keys navigate, Enter selects) */
static void handle_menu_input(Game *game, SDL_Keycode key)
{
    if (game->show_leaderboard || game->show_controls) {
        /* Any key exits the sub-screen */
        if (key == SDLK_ESCAPE || key == SDLK_RETURN) {
            game->show_leaderboard = false;
            game->show_controls = false;
        }
        return;
    }

    switch (key) {
        case SDLK_UP:
            game->menu_selection--;
            if (game->menu_selection < 0)
                game->menu_selection = MENU_ITEM_COUNT - 1;
            break;

        case SDLK_DOWN:
            game->menu_selection++;
            if (game->menu_selection >= MENU_ITEM_COUNT)
                game->menu_selection = 0;
            break;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            switch (game->menu_selection) {
                case MENU_START:
                    /* Start fade transition to game */
                    game->menu_fading = true;
                    game->menu_fade_start = SDL_GetTicks();
                    game->menu_fade_alpha = 0.0f;
                    break;
                case MENU_LEADERBOARD:
                    leaderboard_load(game);
                    game->show_leaderboard = true;
                    break;
                case MENU_CONTROLS:
                    game->show_controls = true;
                    break;
                case MENU_QUIT:
                    game->running = false;
                    break;
            }
            break;

        case SDLK_ESCAPE:
            game->running = false;
            break;
    }
}

void input_handle(Game *game)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            game->running = false;
            return;
        }

        /* Key release: stop soft drop */
        if (event.type == SDL_KEYUP) {
            if (event.key.keysym.sym == SDLK_DOWN)
                game->soft_dropping = false;
        }

        if (event.type != SDL_KEYDOWN) continue;
        if (event.key.repeat) continue;

        SDL_Keycode key = event.key.keysym.sym;

        /* --- Menu state --- */
        if (game->state == STATE_MENU) {
            handle_menu_input(game, key);
            continue;
        }

        /* --- Global keys (any game state) --- */
        if (key == SDLK_ESCAPE) {
            /* Go back to menu instead of quitting */
            if (game->state == STATE_GAME_OVER) {
                leaderboard_add(game, game->score);
                leaderboard_save(game);
            }
            game->state = STATE_MENU;
            game->menu_fading = false;
            game->show_leaderboard = false;
            game->show_controls = false;
            continue;
        }

        if (key == SDLK_r) {
            game_reset(game);
            continue;
        }

        if (key == SDLK_p) {
            if (game->state == STATE_PLAYING) {
                game->state = STATE_PAUSED;
            } else if (game->state == STATE_PAUSED) {
                game->state = STATE_PLAYING;
                game->last_drop_time = SDL_GetTicks();
            }
            continue;
        }

        /* --- Gameplay keys (only while playing) --- */
        if (game->state != STATE_PLAYING) continue;

        switch (key) {
            case SDLK_LEFT:  try_move(game, -1); break;
            case SDLK_RIGHT: try_move(game, +1); break;
            case SDLK_DOWN:  game->soft_dropping = true; break;
            case SDLK_UP:    try_rotate(game); break;
            case SDLK_SPACE: hard_drop(game); break;
            default: break;
        }
    }
}
