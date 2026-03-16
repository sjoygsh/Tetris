/* ===========================================================================
 * audio.h — Sound effects and music
 * =========================================================================== */

#ifndef AUDIO_H
#define AUDIO_H

#include "defs.h"

bool audio_init(Game *game);
void audio_play_move(Game *game);
void audio_play_rotate(Game *game);
void audio_play_drop(Game *game);
void audio_play_clear(Game *game);
void audio_play_gameover(Game *game);
void audio_cleanup(Game *game);

#endif /* AUDIO_H */
