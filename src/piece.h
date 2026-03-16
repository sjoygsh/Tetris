/* ===========================================================================
 * piece.h — Tetromino shapes, colors, and the 7-bag randomizer
 * =========================================================================== */

#ifndef PIECE_H
#define PIECE_H

#include "defs.h"

/* Get the 4x4 shape grid for a piece type at a rotation (0-3).
 * Returns a pointer to a [4][4] int array (1 = filled, 0 = empty). */
const int (*piece_get_shape(PieceType type, int rotation))[4];

/* Get the color for a piece type.
 * Each piece has a unique Apple-inspired soft color. */
SDL_Color piece_get_color(PieceType type);

/* --- 7-Bag Randomizer ---
 * Modern Tetris uses a "bag" system: one of each piece is put in a bag,
 * shuffled, then drawn in order. When empty, a new bag is made.
 * This prevents long droughts of any single piece type. */

/* Fill and shuffle the bag. Called at game start and when bag runs out. */
void piece_bag_init(Game *game);

/* Draw the next piece from the bag. Refills automatically when empty. */
Piece piece_bag_next(Game *game);

#endif /* PIECE_H */
