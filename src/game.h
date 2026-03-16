/* ===========================================================================
 * game.h — Core game logic header
 * =========================================================================== */

#ifndef GAME_H
#define GAME_H

#include "defs.h"

bool game_init(Game *game);
void game_reset(Game *game);
void game_run(Game *game);
void game_lock_and_advance(Game *game);
void game_spawn_next(Game *game);
void game_cleanup(Game *game);

/* Leaderboard */
void leaderboard_load(Game *game);
void leaderboard_save(Game *game);
void leaderboard_add(Game *game, int score);

/* Particle / effect systems */
void particles_init(Game *game);
void particles_update(Game *game, float dt);
void spawn_drop_particles(Game *game, int px, int py);
void spawn_float_text(Game *game, const char *text, float x, float y);

#endif /* GAME_H */
