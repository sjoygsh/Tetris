/* ===========================================================================
 * audio.c — Sound effects and music using SDL2_mixer
 * ===========================================================================
 * Loads .wav files from assets/ and plays them at game events.
 * If files are missing, the game runs silently — sound is optional.
 * =========================================================================== */

#include "audio.h"

bool audio_init(Game *game)
{
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Warning: Could not initialize audio: %s\n", Mix_GetError());
        return false;
    }

    game->sfx_move     = Mix_LoadWAV("assets/move.wav");
    game->sfx_rotate   = Mix_LoadWAV("assets/rotate.wav");
    game->sfx_drop     = Mix_LoadWAV("assets/drop.wav");
    game->sfx_clear    = Mix_LoadWAV("assets/clear.wav");
    game->sfx_gameover = Mix_LoadWAV("assets/gameover.wav");

    game->music = Mix_LoadMUS("assets/music.ogg");
    if (game->music) {
        Mix_PlayMusic(game->music, -1);
        Mix_VolumeMusic(64);
    }

    return true;
}

void audio_play_move(Game *game)     { if (game->sfx_move)     Mix_PlayChannel(-1, game->sfx_move, 0); }
void audio_play_rotate(Game *game)   { if (game->sfx_rotate)   Mix_PlayChannel(-1, game->sfx_rotate, 0); }
void audio_play_drop(Game *game)     { if (game->sfx_drop)     Mix_PlayChannel(-1, game->sfx_drop, 0); }
void audio_play_clear(Game *game)    { if (game->sfx_clear)    Mix_PlayChannel(-1, game->sfx_clear, 0); }
void audio_play_gameover(Game *game) { Mix_HaltMusic(); if (game->sfx_gameover) Mix_PlayChannel(-1, game->sfx_gameover, 0); }

void audio_cleanup(Game *game)
{
    if (game->sfx_move)     Mix_FreeChunk(game->sfx_move);
    if (game->sfx_rotate)   Mix_FreeChunk(game->sfx_rotate);
    if (game->sfx_drop)     Mix_FreeChunk(game->sfx_drop);
    if (game->sfx_clear)    Mix_FreeChunk(game->sfx_clear);
    if (game->sfx_gameover) Mix_FreeChunk(game->sfx_gameover);
    if (game->music)        Mix_FreeMusic(game->music);
    Mix_CloseAudio();
}
