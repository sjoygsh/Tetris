/* ===========================================================================
 * board.c — Tetris game board logic
 * ===========================================================================
 * Handles the playing field: collision detection, locking pieces,
 * clearing completed lines.
 *
 * COORDINATE SYSTEM:
 *   board->cells[row][col]
 *   Row 0 is the TOP, row 19 is the BOTTOM (visible area).
 *   4 extra buffer rows above (indices 0-3 in the array) for spawning.
 *   Visible rows are at indices 4 through 23.
 * =========================================================================== */

#include "board.h"
#include "piece.h"

/* Clear the entire board to empty */
void board_init(Board *board)
{
    memset(board->cells, 0, sizeof(board->cells));
}

/* Check if a piece can exist at its position without overlapping anything.
 * This is THE most important function — called for every move/rotation. */
bool board_piece_fits(const Board *board, const Piece *piece)
{
    const int (*shape)[4] = piece_get_shape(piece->type, piece->rotation);

    for (int row = 0; row < PIECE_GRID_SIZE; row++) {
        for (int col = 0; col < PIECE_GRID_SIZE; col++) {
            if (!shape[row][col]) continue;

            int bx = piece->x + col;
            int by = piece->y + row;

            if (bx < 0 || bx >= BOARD_WIDTH)       return false;
            if (by >= BOARD_HEIGHT + 4)             return false;
            if (by < 0)                             continue;
            if (board->cells[by][bx] != 0)          return false;
        }
    }
    return true;
}

/* Permanently place a piece onto the board.
 * Stores piece->type + 1 so we know the color (0 = empty). */
void board_lock_piece(Board *board, const Piece *piece)
{
    const int (*shape)[4] = piece_get_shape(piece->type, piece->rotation);

    for (int row = 0; row < PIECE_GRID_SIZE; row++) {
        for (int col = 0; col < PIECE_GRID_SIZE; col++) {
            if (!shape[row][col]) continue;

            int bx = piece->x + col;
            int by = piece->y + row;

            if (by >= 0 && by < BOARD_HEIGHT + 4 &&
                bx >= 0 && bx < BOARD_WIDTH) {
                board->cells[by][bx] = piece->type + 1;
            }
        }
    }
}

/* Find and remove completed lines, shifting everything above down.
 * Works bottom-up so chained clears are handled correctly. */
int board_clear_lines(Board *board)
{
    int lines_cleared = 0;

    for (int row = BOARD_HEIGHT + 3; row >= 0; row--) {
        bool full = true;
        for (int col = 0; col < BOARD_WIDTH; col++) {
            if (board->cells[row][col] == 0) { full = false; break; }
        }

        if (full) {
            lines_cleared++;
            /* Shift everything above down by one row */
            for (int y = row; y > 0; y--) {
                memcpy(board->cells[y], board->cells[y - 1],
                       sizeof(int) * BOARD_WIDTH);
            }
            memset(board->cells[0], 0, sizeof(int) * BOARD_WIDTH);
            row++;  /* Re-check this row (new content shifted in) */
        }
    }
    return lines_cleared;
}

/* Find which rows are full WITHOUT modifying the board.
 * Used to set up the flash animation before actual clearing. */
int board_find_full_rows(const Board *board, int *out_rows, int max_rows)
{
    int count = 0;
    for (int row = BOARD_HEIGHT + 3; row >= 0 && count < max_rows; row--) {
        bool full = true;
        for (int col = 0; col < BOARD_WIDTH; col++) {
            if (board->cells[row][col] == 0) { full = false; break; }
        }
        if (full) {
            out_rows[count++] = row;
        }
    }
    return count;
}
