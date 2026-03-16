/* ===========================================================================
 * board.h — Game board (playing field) functions
 * =========================================================================== */

#ifndef BOARD_H
#define BOARD_H

#include "defs.h"

/* Clear the board (all cells to 0) */
void board_init(Board *board);

/* Check if a piece fits at its current position (no collisions) */
bool board_piece_fits(const Board *board, const Piece *piece);

/* Lock a piece permanently onto the board */
void board_lock_piece(Board *board, const Piece *piece);

/* Find and remove completed lines. Returns number cleared. */
int board_clear_lines(Board *board);

/* Find full rows WITHOUT clearing. Used by flash animation. */
int board_find_full_rows(const Board *board, int *out_rows, int max_rows);

#endif /* BOARD_H */
