/* ===========================================================================
 * input.h — Keyboard input handling
 * =========================================================================== */

#ifndef INPUT_H
#define INPUT_H

#include "defs.h"

/* Process all pending input events. Called once per frame. */
void input_handle(Game *game);

#endif /* INPUT_H */
