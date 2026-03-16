/* ===========================================================================
 * render.h — All drawing/rendering functions (Apple light theme)
 * =========================================================================== */

#ifndef RENDER_H
#define RENDER_H

#include "defs.h"

/* Draw the entire game frame (~60 times/sec).
 * Handles menu, gameplay, pause, and game-over screens. */
void render_frame(Game *game);

#endif /* RENDER_H */
