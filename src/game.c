/* ===========================================================================
 * game.c — Core game logic
 * ===========================================================================
 * The heart of the game:
 *   - Initialization (SDL, window, fonts, audio)
 *   - Main game loop (input -> update -> render at ~60 FPS)
 *   - Gravity, piece locking, scoring
 *   - 7-bag integration, combo tracking
 *   - Particle system updates
 *   - Leaderboard persistence
 *   - Menu fade transitions
 * =========================================================================== */

#include "game.h"
#include "board.h"
#include "piece.h"
#include "render.h"
#include "audio.h"
#include "input.h"

/* ===========================================================================
 * LEADERBOARD — Simple file-based top-5 score storage
 * =========================================================================== */

void leaderboard_load(Game *game)
{
    memset(game->leaderboard, 0, sizeof(game->leaderboard));
    FILE *f = fopen(LEADERBOARD_FILE, "rb");
    if (!f) return;
    fread(game->leaderboard, sizeof(int), MAX_LEADERBOARD, f);
    fclose(f);
}

void leaderboard_save(Game *game)
{
    FILE *f = fopen(LEADERBOARD_FILE, "wb");
    if (!f) return;
    fwrite(game->leaderboard, sizeof(int), MAX_LEADERBOARD, f);
    fclose(f);
}

void leaderboard_add(Game *game, int score)
{
    if (score <= 0) return;
    int pos = MAX_LEADERBOARD;
    for (int i = 0; i < MAX_LEADERBOARD; i++) {
        if (score > game->leaderboard[i]) { pos = i; break; }
    }
    if (pos >= MAX_LEADERBOARD) return;
    for (int i = MAX_LEADERBOARD - 1; i > pos; i--)
        game->leaderboard[i] = game->leaderboard[i - 1];
    game->leaderboard[pos] = score;
}

/* ===========================================================================
 * PARTICLE SYSTEM
 * =========================================================================== */

void particles_init(Game *game)
{
    for (int i = 0; i < MAX_BG_PARTICLES; i++) {
        BGParticle *p = &game->bg_particles[i];
        p->active = true;
        p->x     = (float)(rand() % WINDOW_WIDTH);
        p->y     = (float)(rand() % WINDOW_HEIGHT);
        p->vy    = 10.0f + (float)(rand() % 20);
        p->size  = 2.0f + (float)(rand() % 4);
        p->alpha = 0.08f + (float)(rand() % 15) / 100.0f;
    }
}

void particles_update(Game *game, float dt)
{
    /* Background particles drift down and wrap */
    for (int i = 0; i < MAX_BG_PARTICLES; i++) {
        BGParticle *p = &game->bg_particles[i];
        if (!p->active) continue;
        p->y += p->vy * dt;
        if (p->y > WINDOW_HEIGHT + p->size) {
            p->y = -p->size;
            p->x = (float)(rand() % WINDOW_WIDTH);
        }
    }

    /* Drop particles move outward and fade */
    Uint32 now = SDL_GetTicks();
    for (int i = 0; i < MAX_DROP_PARTICLES; i++) {
        DropParticle *p = &game->drop_particles[i];
        if (!p->active) continue;
        Uint32 age = now - p->spawn_time;
        if (age >= DROP_PARTICLE_LIFE) { p->active = false; continue; }
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->vy += 200.0f * dt;  /* Gravity */
        p->alpha = 1.0f - (float)age / (float)DROP_PARTICLE_LIFE;
    }
}

void spawn_drop_particles(Game *game, int px, int py)
{
    for (int i = 0; i < 4; i++) {
        DropParticle *slot = NULL;
        for (int j = 0; j < MAX_DROP_PARTICLES; j++) {
            if (!game->drop_particles[j].active) { slot = &game->drop_particles[j]; break; }
        }
        if (!slot) return;
        slot->active = true;
        slot->x = (float)px + (float)(rand() % 8 - 4);
        slot->y = (float)py;
        slot->vx = (float)(rand() % 80 - 40);
        slot->vy = -(float)(rand() % 60 + 20);
        slot->alpha = 1.0f;
        slot->size = 2.0f + (float)(rand() % 3);
        slot->spawn_time = SDL_GetTicks();
    }
}

void spawn_float_text(Game *game, const char *text, float x, float y)
{
    FloatText *slot = NULL;
    Uint32 oldest_time = UINT32_MAX;
    int oldest_idx = 0;
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (!game->float_texts[i].active) { slot = &game->float_texts[i]; break; }
        if (game->float_texts[i].start_time < oldest_time) {
            oldest_time = game->float_texts[i].start_time;
            oldest_idx = i;
        }
    }
    if (!slot) slot = &game->float_texts[oldest_idx];
    slot->active = true;
    snprintf(slot->text, sizeof(slot->text), "%s", text);
    slot->x = x;
    slot->y = y;
    slot->scale = 1.5f;
    slot->alpha = 0.0f;
    slot->start_time = SDL_GetTicks();
}

/* ===========================================================================
 * game_init()
 * =========================================================================== */
bool game_init(Game *game)
{
    memset(game, 0, sizeof(Game));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        printf("ERROR: SDL init failed: %s\n", SDL_GetError());
        return false;
    }

    game->window = SDL_CreateWindow(
        "TETRIS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!game->window) {
        printf("ERROR: Window creation failed: %s\n", SDL_GetError());
        return false;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!game->renderer) {
        printf("ERROR: Renderer failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);

    if (TTF_Init() < 0) {
        printf("ERROR: TTF init failed: %s\n", TTF_GetError());
        return false;
    }

    game->font_small = TTF_OpenFont("assets/font_light.ttf", 12);
    game->font_label = TTF_OpenFont("assets/font_bold.ttf",  13);
    game->font       = TTF_OpenFont("assets/font.ttf",       16);
    game->font_score = TTF_OpenFont("assets/font_bold.ttf",  28);
    game->font_large = TTF_OpenFont("assets/font_bold.ttf",  36);
    game->font_title = TTF_OpenFont("assets/font_bold.ttf",  48);

    if (!game->font || !game->font_large || !game->font_small ||
        !game->font_score || !game->font_label || !game->font_title) {
        printf("ERROR: Font loading failed: %s\n", TTF_GetError());
        return false;
    }

    audio_init(game);
    srand((unsigned int)time(NULL));
    particles_init(game);
    leaderboard_load(game);

    game->running = true;
    game->state = STATE_MENU;
    game->menu_selection = MENU_START;

    return true;
}

/* ===========================================================================
 * game_reset()
 * =========================================================================== */
void game_reset(Game *game)
{
    leaderboard_load(game);
    board_init(&game->board);
    piece_bag_init(game);
    game->current   = piece_bag_next(game);
    game->next      = piece_bag_next(game);
    game->current.y = 2;

    game->score = 0;
    game->level = 1;
    game->lines_cleared = 0;
    game->combo_count = 0;
    game->last_clear_was_tetris = false;

    game->drop_interval     = INITIAL_DROP_INTERVAL;
    game->last_drop_time    = SDL_GetTicks();
    game->lock_timer_active = false;
    game->soft_dropping     = false;

    game->flash_count      = 0;
    game->spawning         = true;
    game->spawn_time       = SDL_GetTicks();
    game->piece_on_ground  = false;
    game->drop_trail_count = 0;

    for (int i = 0; i < MAX_FLOAT_TEXTS; i++)
        game->float_texts[i].active = false;
    for (int i = 0; i < MAX_DROP_PARTICLES; i++)
        game->drop_particles[i].active = false;
    for (int i = 0; i < 4; i++)
        game->drop_trails[i].active = false;

    game->state = STATE_PLAYING;
    if (game->music && !Mix_PlayingMusic())
        Mix_PlayMusic(game->music, -1);
}

/* ===========================================================================
 * game_lock_and_advance()
 * =========================================================================== */
void game_lock_and_advance(Game *game)
{
    board_lock_piece(&game->board, &game->current);

    int full_rows[4];
    int num_full = board_find_full_rows(&game->board, full_rows, 4);

    if (num_full > 0) {
        game->flash_count = num_full;
        for (int i = 0; i < num_full; i++)
            game->flash_rows[i] = full_rows[i];
        game->flash_start_time = SDL_GetTicks();
        audio_play_clear(game);
        return;
    }

    /* No lines cleared — reset combo */
    game->combo_count = 0;
    game->last_clear_was_tetris = false;
    game_spawn_next(game);
}

/* ===========================================================================
 * game_spawn_next()
 * =========================================================================== */
void game_spawn_next(Game *game)
{
    if (game->score > game->high_score)
        game->high_score = game->score;

    game->current   = game->next;
    game->current.y = 2;
    game->next      = piece_bag_next(game);

    game->spawning   = true;
    game->spawn_time = SDL_GetTicks();
    game->lock_timer_active = false;
    game->soft_dropping     = false;
    game->piece_on_ground   = false;

    if (!board_piece_fits(&game->board, &game->current)) {
        game->state = STATE_GAME_OVER;
        audio_play_gameover(game);
        leaderboard_add(game, game->score);
        leaderboard_save(game);
    }
    game->last_drop_time = SDL_GetTicks();
}

/* After flash animation completes */
static void game_finish_clear(Game *game)
{
    int cleared = board_clear_lines(&game->board);
    game->score += cleared;
    game->lines_cleared += cleared;
    game->level = (game->lines_cleared / LINES_PER_LEVEL) + 1;

    game->drop_interval = INITIAL_DROP_INTERVAL - (game->level - 1) * 50;
    if (game->drop_interval < MIN_DROP_INTERVAL)
        game->drop_interval = MIN_DROP_INTERVAL;

    game->combo_count++;

    /* Floating text for specials */
    float tx = (float)(BOARD_OFFSET_X + BOARD_WIDTH * CELL_SIZE / 2);
    float ty = (float)(BOARD_OFFSET_Y + BOARD_HEIGHT * CELL_SIZE / 2 - 30);

    if (cleared == 4) {
        if (game->last_clear_was_tetris)
            spawn_float_text(game, "Back-to-Back!", tx, ty - 25);
        spawn_float_text(game, "TETRIS!", tx, ty);
        game->last_clear_was_tetris = true;
    } else {
        game->last_clear_was_tetris = false;
    }

    if (game->combo_count >= 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Combo x%d", game->combo_count);
        spawn_float_text(game, buf, tx, ty + 25);
    }

    game->flash_count = 0;
    game_spawn_next(game);
}

/* Gravity */
static void update_gravity(Game *game)
{
    Uint32 now = SDL_GetTicks();
    int interval = game->soft_dropping ? SOFT_DROP_INTERVAL : game->drop_interval;
    if (now - game->last_drop_time < (Uint32)interval) return;

    Piece test = game->current;
    test.y++;

    if (board_piece_fits(&game->board, &test)) {
        game->current.y++;
        game->last_drop_time = now;
        game->lock_timer_active = false;
        game->piece_on_ground = false;
    } else {
        game->piece_on_ground = true;
        if (!game->lock_timer_active) {
            game->lock_timer_active = true;
            game->lock_timer = now;
        } else if (now - game->lock_timer >= LOCK_DELAY) {
            game_lock_and_advance(game);
        }
    }
}

/* ===========================================================================
 * game_run() — Main game loop
 * =========================================================================== */
void game_run(Game *game)
{
    Uint32 last_frame = SDL_GetTicks();

    while (game->running) {
        Uint32 frame_start = SDL_GetTicks();
        float dt = (float)(frame_start - last_frame) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        last_frame = frame_start;

        input_handle(game);

        /* Menu fade transition */
        if (game->state == STATE_MENU && game->menu_fading) {
            Uint32 elapsed = SDL_GetTicks() - game->menu_fade_start;
            game->menu_fade_alpha = (float)elapsed / (float)MENU_FADE_DURATION;
            if (game->menu_fade_alpha >= 1.0f) {
                game->menu_fade_alpha = 1.0f;
                game->menu_fading = false;
                game_reset(game);
            }
        }

        if (game->state == STATE_PLAYING) {
            if (game->spawning) {
                if (SDL_GetTicks() - game->spawn_time >= SPAWN_ANIM_DURATION)
                    game->spawning = false;
            }
            if (game->flash_count > 0) {
                if (SDL_GetTicks() - game->flash_start_time >= FLASH_DURATION)
                    game_finish_clear(game);
            } else {
                update_gravity(game);
            }
        }

        particles_update(game, dt);

        /* Update floating text */
        {
            Uint32 now = SDL_GetTicks();
            for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
                FloatText *ft = &game->float_texts[i];
                if (!ft->active) continue;
                Uint32 age = now - ft->start_time;
                if (age >= FLOAT_TEXT_DURATION) { ft->active = false; continue; }
                float progress = (float)age / (float)FLOAT_TEXT_DURATION;
                float sp = (float)age / 200.0f;
                if (sp > 1.0f) sp = 1.0f;
                ft->scale = 1.5f - 0.5f * sp;
                if (progress < 0.08f)       ft->alpha = progress / 0.08f;
                else if (progress > 0.75f)  ft->alpha = (1.0f - progress) / 0.25f;
                else                         ft->alpha = 1.0f;
                ft->y -= 15.0f * dt;
            }
        }

        /* Deactivate expired drop trails */
        {
            Uint32 now = SDL_GetTicks();
            for (int i = 0; i < 4; i++) {
                if (game->drop_trails[i].active &&
                    now - game->drop_trails[i].start_time >= DROP_TRAIL_DURATION)
                    game->drop_trails[i].active = false;
            }
        }

        render_frame(game);

        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < 16) SDL_Delay(16 - frame_time);
    }
}

/* ===========================================================================
 * game_cleanup()
 * =========================================================================== */
void game_cleanup(Game *game)
{
    audio_cleanup(game);
    if (game->font)       TTF_CloseFont(game->font);
    if (game->font_large) TTF_CloseFont(game->font_large);
    if (game->font_small) TTF_CloseFont(game->font_small);
    if (game->font_score) TTF_CloseFont(game->font_score);
    if (game->font_label) TTF_CloseFont(game->font_label);
    if (game->font_title) TTF_CloseFont(game->font_title);
    TTF_Quit();
    if (game->renderer) SDL_DestroyRenderer(game->renderer);
    if (game->window)   SDL_DestroyWindow(game->window);
    SDL_Quit();
}
