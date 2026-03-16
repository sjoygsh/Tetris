/* ===========================================================================
 * piece.c — Tetromino shapes, colors, and the 7-bag randomizer
 * ===========================================================================
 * Contains:
 *   1. Shape definitions for all 7 pieces in all 4 rotations (SRS standard)
 *   2. Apple-inspired soft color palette for each piece
 *   3. The 7-bag randomizer for fair piece distribution
 *
 * COLOR PALETTE:
 *   Each piece has a unique, refined color inspired by iOS system accents.
 *   Desaturated and balanced — easy on the eyes, distinct at a glance:
 *   I=Soft Teal, O=Warm Gold, T=Soft Lavender, S=Mint Green,
 *   Z=Soft Coral, J=Sky Blue, L=Warm Orange.
 *
 * 7-BAG RANDOMIZER:
 *   Pure random (rand() % 7) can give long streaks of the same piece
 *   or long droughts without a piece you need. The 7-bag system puts
 *   one of each piece in a "bag", shuffles it, and draws from it.
 *   You're guaranteed to see every piece at least once per 7 pieces.
 * =========================================================================== */

#include "piece.h"

/* ===========================================================================
 * PIECE SHAPE DATA — shapes[type][rotation][row][col]
 * ===========================================================================
 * Super Rotation System (SRS) standard rotations.
 * Each piece has 4 rotation states, each stored as a 4x4 grid.
 * =========================================================================== */
static const int shapes[NUM_PIECE_TYPES][NUM_ROTATIONS][PIECE_GRID_SIZE][PIECE_GRID_SIZE] = {
    /* PIECE_I */
    {
        { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} },
        { {0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0} },
        { {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} },
    },
    /* PIECE_O */
    {
        { {0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0} },
    },
    /* PIECE_T */
    {
        { {0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,1,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0} },
    },
    /* PIECE_S */
    {
        { {0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,1,0}, {0,0,1,0}, {0,0,0,0} },
        { {0,0,0,0}, {0,1,1,0}, {1,1,0,0}, {0,0,0,0} },
        { {1,0,0,0}, {1,1,0,0}, {0,1,0,0}, {0,0,0,0} },
    },
    /* PIECE_Z */
    {
        { {1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,0,1,0}, {0,1,1,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,0,0}, {0,1,1,0}, {0,0,0,0} },
        { {0,1,0,0}, {1,1,0,0}, {1,0,0,0}, {0,0,0,0} },
    },
    /* PIECE_J */
    {
        { {1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,1,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,1,0}, {0,0,1,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,0,0}, {1,1,0,0}, {0,0,0,0} },
    },
    /* PIECE_L */
    {
        { {0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0} },
        { {0,1,0,0}, {0,1,0,0}, {0,1,1,0}, {0,0,0,0} },
        { {0,0,0,0}, {1,1,1,0}, {1,0,0,0}, {0,0,0,0} },
        { {1,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,0,0,0} },
    },
};

/* ===========================================================================
 * PIECE COLORS — Apple-inspired soft palette
 * ===========================================================================
 * Each piece has a unique, refined color inspired by iOS system accents.
 * Desaturated and balanced — easy on the eyes, distinct at a glance.
 * =========================================================================== */
static const SDL_Color piece_colors[NUM_PIECE_TYPES] = {
    {  90, 210, 220, 255 },  /* I = Soft Teal     */
    { 240, 210,  80, 255 },  /* O = Warm Gold     */
    { 175, 120, 220, 255 },  /* T = Soft Lavender */
    { 100, 210, 130, 255 },  /* S = Mint Green    */
    { 240, 100, 100, 255 },  /* Z = Soft Coral    */
    {  90, 140, 240, 255 },  /* J = Sky Blue      */
    { 245, 165,  80, 255 },  /* L = Warm Orange   */
};

/* Return the 4x4 shape for a piece type and rotation */
const int (*piece_get_shape(PieceType type, int rotation))[4]
{
    return shapes[type][rotation];
}

/* Return the color for a piece type */
SDL_Color piece_get_color(PieceType type)
{
    return piece_colors[type];
}

/* ===========================================================================
 * 7-BAG RANDOMIZER
 * ===========================================================================
 * Fisher-Yates shuffle: start from the end of the array, swap each
 * element with a random element before it (including itself).
 * This gives a perfectly uniform shuffle.
 * =========================================================================== */

/* Fill the bag with pieces 0-6 and shuffle them */
void piece_bag_init(Game *game)
{
    /* Fill: bag = {0, 1, 2, 3, 4, 5, 6} */
    for (int i = 0; i < NUM_PIECE_TYPES; i++) {
        game->bag[i] = i;
    }

    /* Fisher-Yates shuffle */
    for (int i = NUM_PIECE_TYPES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = game->bag[i];
        game->bag[i] = game->bag[j];
        game->bag[j] = tmp;
    }

    game->bag_index = 0;
}

/* Draw the next piece from the bag. If bag is empty, refill and reshuffle. */
Piece piece_bag_next(Game *game)
{
    /* If we've used all 7 pieces, make a new bag */
    if (game->bag_index >= NUM_PIECE_TYPES) {
        piece_bag_init(game);
    }

    /* Create a piece from the next bag entry */
    Piece p;
    p.type     = (PieceType)game->bag[game->bag_index];
    p.rotation = 0;
    p.x        = BOARD_WIDTH / 2 - 2;  /* Center horizontally */
    p.y        = 0;
    game->bag_index++;

    return p;
}
