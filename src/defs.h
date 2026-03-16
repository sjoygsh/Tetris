/* ===========================================================================
 * defs.h — Global definitions, constants, and data structures
 * ===========================================================================
 * Every constant, struct, and enum used across the game lives here.
 *
 * WHAT IS A HEADER FILE (.h)?
 *   Lists what's available (constants, function signatures, struct
 *   definitions) so other .c files can use them. Code lives in .c files.
 *
 * THIS VERSION FEATURES:
 *   - Apple-inspired light theme with rounded UI cards
 *   - Main menu with Start / Leaderboard / Controls / Quit
 *   - 7-bag randomizer for fair piece distribution
 *   - Particle system (background ambiance + hard-drop burst)
 *   - Combo / back-to-back tracking with floating text
 *   - Piece spawn fade-in animation
 *   - Lock-delay brightness pulse
 *   - Hard drop trail + particle burst effects
 *   - Line clear flash animation
 *   - Leaderboard persistence (top 5 scores)
 * =========================================================================== */

#ifndef DEFS_H
#define DEFS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>

/* ===========================================================================
 * GAME GRID
 * =========================================================================== */
#define BOARD_WIDTH   10
#define BOARD_HEIGHT  20
#define CELL_SIZE     32

/* ===========================================================================
 * WINDOW LAYOUT  —  |24px|320px board|220px sidebar| = 564 x 688
 * =========================================================================== */
#define BOARD_OFFSET_X 24
#define BOARD_OFFSET_Y 24
#define SIDEBAR_WIDTH  220
#define WINDOW_WIDTH   (BOARD_OFFSET_X + BOARD_WIDTH * CELL_SIZE + SIDEBAR_WIDTH)
#define WINDOW_HEIGHT  (BOARD_OFFSET_Y * 2 + BOARD_HEIGHT * CELL_SIZE)

/* ===========================================================================
 * GAME TIMING (ms)
 * =========================================================================== */
#define INITIAL_DROP_INTERVAL 800
#define MIN_DROP_INTERVAL     80
#define SOFT_DROP_INTERVAL    50
#define LOCK_DELAY            500
#define LINES_PER_LEVEL       10

/* ===========================================================================
 * ANIMATION TIMING (ms)
 * =========================================================================== */
#define FLASH_DURATION        300
#define SPAWN_ANIM_DURATION   80
#define DROP_TRAIL_DURATION   250
#define DROP_PARTICLE_LIFE    300
#define FLOAT_TEXT_DURATION   1200
#define MENU_FADE_DURATION    400

/* ===========================================================================
 * PARTICLE LIMITS
 * =========================================================================== */
#define MAX_BG_PARTICLES      50
#define MAX_DROP_PARTICLES    24
#define MAX_FLOAT_TEXTS        4

/* ===========================================================================
 * LEADERBOARD
 * =========================================================================== */
#define MAX_LEADERBOARD        5
#define LEADERBOARD_FILE "assets/scores.dat"

/* ===========================================================================
 * TETROMINO DEFINITIONS
 * =========================================================================== */
#define NUM_PIECE_TYPES 7
#define PIECE_GRID_SIZE 4
#define NUM_ROTATIONS   4

/* ===========================================================================
 * ENUMERATIONS
 * =========================================================================== */
typedef enum {
    PIECE_I = 0, PIECE_O, PIECE_T, PIECE_S, PIECE_Z, PIECE_J, PIECE_L
} PieceType;

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

typedef enum {
    MENU_START = 0,
    MENU_LEADERBOARD,
    MENU_CONTROLS,
    MENU_QUIT,
    MENU_ITEM_COUNT
} MenuItem;

/* ===========================================================================
 * DATA STRUCTURES
 * =========================================================================== */

typedef struct {
    PieceType type;
    int       rotation;
    int       x, y;
} Piece;

typedef struct {
    int cells[BOARD_HEIGHT + 4][BOARD_WIDTH];
} Board;

/* Background particle — small square drifting down for ambiance */
typedef struct {
    float x, y, vy, size, alpha;
    bool  active;
} BGParticle;

/* Drop particle — bursts outward on hard-drop landing */
typedef struct {
    float  x, y, vx, vy, alpha, size;
    Uint32 spawn_time;
    bool   active;
} DropParticle;

/* Floating text — animated combo / special announcements */
typedef struct {
    char   text[32];
    float  x, y, scale, alpha;
    Uint32 start_time;
    bool   active;
} FloatText;

/* Hard-drop trail — vertical streak showing drop path */
typedef struct {
    int    col, start_row, end_row;
    Uint32 start_time;
    bool   active;
} DropTrail;

/* ===========================================================================
 * GAME — Master struct
 * =========================================================================== */
typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    TTF_Font     *font_large;
    TTF_Font     *font_small;
    TTF_Font     *font_score;
    TTF_Font     *font_label;
    TTF_Font     *font_title;

    Mix_Chunk *sfx_move;
    Mix_Chunk *sfx_rotate;
    Mix_Chunk *sfx_drop;
    Mix_Chunk *sfx_clear;
    Mix_Chunk *sfx_gameover;
    Mix_Music *music;

    Board     board;
    Piece     current;
    Piece     next;

    /* 7-bag randomizer */
    int       bag[NUM_PIECE_TYPES];
    int       bag_index;

    int       score;
    int       level;
    int       lines_cleared;
    int       high_score;

    int       combo_count;
    bool      last_clear_was_tetris;

    Uint32    last_drop_time;
    Uint32    lock_timer;
    bool      lock_timer_active;
    int       drop_interval;

    int       flash_rows[4];
    int       flash_count;
    Uint32    flash_start_time;

    Uint32    spawn_time;
    bool      spawning;

    bool      piece_on_ground;

    DropTrail    drop_trails[4];
    int          drop_trail_count;
    DropParticle drop_particles[MAX_DROP_PARTICLES];

    BGParticle bg_particles[MAX_BG_PARTICLES];

    FloatText float_texts[MAX_FLOAT_TEXTS];

    int       menu_selection;
    float     menu_fade_alpha;
    Uint32    menu_fade_start;
    bool      menu_fading;
    bool      show_leaderboard;
    bool      show_controls;

    int       leaderboard[MAX_LEADERBOARD];

    GameState state;
    bool      running;
    bool      soft_dropping;
} Game;

#endif /* DEFS_H */
