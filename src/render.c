/* ===========================================================================
 * render.c — Apple-inspired light-theme rendering with visual polish
 * ===========================================================================
 * Clean white background, soft rounded cards, dark text, glossy blocks.
 *
 * ROUNDED RECTANGLES:
 *   SDL2 has no built-in rounded rect. We build one using:
 *     - 4 filled circles at corners (midpoint circle algorithm)
 *     - 2 overlapping rectangles for the body
 *
 * FEATURES:
 *   - Main menu with animated selection
 *   - Dynamic background (falling particles)
 *   - Lock delay pulse
 *   - Line clear flash animation
 *   - Hard drop trail + particles
 *   - Floating combo/special text
 *   - Piece spawn fade-in
 *   - Leaderboard + Controls sub-screens
 * =========================================================================== */

#include "render.h"
#include "piece.h"
#include "board.h"

/* ===========================================================================
 * COLOR PALETTE — Apple-style light theme
 * =========================================================================== */
static const SDL_Color BG_WINDOW     = { 242, 242, 247, 255 };  /* iOS light gray */
static const SDL_Color BG_BOARD      = { 255, 255, 255, 255 };  /* Pure white */
static const SDL_Color BG_CARD       = { 255, 255, 255, 255 };
static const SDL_Color GRID_LINE     = { 220, 220, 228, 255 };
static const SDL_Color BOARD_BORDER  = { 195, 195, 205, 255 };
static const SDL_Color BORDER_DIM    = { 210, 210, 218, 255 };
static const SDL_Color TEXT_PRIMARY  = {  28,  28,  30, 255 };
static const SDL_Color TEXT_SECONDARY= { 120, 120, 128, 255 };
static const SDL_Color TEXT_TERTIARY = { 160, 160, 168, 255 };
static const SDL_Color TEXT_SHADOW   = {   0,   0,   0,  35 };
static const SDL_Color CARD_SHADOW   = {   0,   0,   0,  18 };
static const SDL_Color PARTICLE_COLOR= { 180, 180, 190, 255 };
static const SDL_Color TRAIL_COLOR   = { 120, 120, 130, 255 };

/* ===========================================================================
 * ROUNDED RECT HELPERS (midpoint circle algorithm)
 * =========================================================================== */
static void fill_circle(SDL_Renderer *r, int cx, int cy, int rad)
{
    int x = rad, y = 0, err = 1 - rad;
    while (x >= y) {
        SDL_RenderDrawLine(r, cx - x, cy + y, cx + x, cy + y);
        SDL_RenderDrawLine(r, cx - x, cy - y, cx + x, cy - y);
        SDL_RenderDrawLine(r, cx - y, cy + x, cx + y, cy + x);
        SDL_RenderDrawLine(r, cx - y, cy - x, cx + y, cy - x);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

static void fill_rounded_rect(SDL_Renderer *r, int x, int y,
                               int w, int h, int rad, SDL_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    if (rad > w / 2) rad = w / 2;
    if (rad > h / 2) rad = h / 2;

    SDL_Rect vert  = { x + rad, y, w - 2 * rad, h };
    SDL_Rect horiz = { x, y + rad, w, h - 2 * rad };
    SDL_RenderFillRect(r, &vert);
    SDL_RenderFillRect(r, &horiz);

    fill_circle(r, x + rad,         y + rad,         rad);
    fill_circle(r, x + w - rad - 1, y + rad,         rad);
    fill_circle(r, x + rad,         y + h - rad - 1, rad);
    fill_circle(r, x + w - rad - 1, y + h - rad - 1, rad);
}

/* ===========================================================================
 * TEXT HELPERS
 * =========================================================================== */
static void draw_text(SDL_Renderer *r, TTF_Font *font,
                      const char *text, int x, int y, SDL_Color color)
{
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    if (tex) {
        SDL_Rect dest = { x, y, surf->w, surf->h };
        SDL_RenderCopy(r, tex, NULL, &dest);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

static void draw_text_centered(SDL_Renderer *r, TTF_Font *font,
                               const char *text, int ax, int aw,
                               int y, SDL_Color color)
{
    int tw, th;
    TTF_SizeText(font, text, &tw, &th);
    draw_text(r, font, text, ax + (aw - tw) / 2, y, color);
}

static void draw_text_shadow(SDL_Renderer *r, TTF_Font *font,
                             const char *text, int x, int y,
                             SDL_Color color, int dx, int dy)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Surface *ss = TTF_RenderText_Blended(font, text, TEXT_SHADOW);
    if (ss) {
        SDL_Texture *st = SDL_CreateTextureFromSurface(r, ss);
        if (st) {
            SDL_SetTextureAlphaMod(st, TEXT_SHADOW.a);
            SDL_Rect d = { x + dx, y + dy, ss->w, ss->h };
            SDL_RenderCopy(r, st, NULL, &d);
            SDL_DestroyTexture(st);
        }
        SDL_FreeSurface(ss);
    }
    draw_text(r, font, text, x, y, color);
}

static void draw_text_centered_shadow(SDL_Renderer *r, TTF_Font *font,
                                      const char *text, int ax, int aw,
                                      int y, SDL_Color color)
{
    int tw, th;
    TTF_SizeText(font, text, &tw, &th);
    draw_text_shadow(r, font, text, ax + (aw - tw) / 2, y, color, 1, 2);
}

/* ===========================================================================
 * GLOSSY BLOCK — one cell on the board
 * =========================================================================== */
static void draw_glossy_block(SDL_Renderer *r, int x, int y,
                              int size, SDL_Color color, Uint8 alpha)
{
    int inset = 2;
    SDL_Rect base = { x + inset, y + inset, size - inset * 2, size - inset * 2 };
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, alpha);
    SDL_RenderFillRect(r, &base);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    /* Top highlight */
    {
        int hh = (size - inset * 2) * 4 / 10;
        SDL_Rect hl = { x + inset, y + inset, size - inset * 2, hh };
        SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(alpha * 90 / 255));
        SDL_RenderFillRect(r, &hl);
    }

    /* Bottom shadow */
    {
        int sh = (size - inset * 2) * 3 / 10;
        SDL_Rect sd = { x + inset, y + size - inset - sh, size - inset * 2, sh };
        SDL_SetRenderDrawColor(r, 0, 0, 0, (Uint8)(alpha * 40 / 255));
        SDL_RenderFillRect(r, &sd);
    }

    /* Border */
    {
        SDL_Rect bdr = { x + inset, y + inset, size - inset * 2, size - inset * 2 };
        Uint8 br = (Uint8)(color.r * 3 / 4);
        Uint8 bg = (Uint8)(color.g * 3 / 4);
        Uint8 bb = (Uint8)(color.b * 3 / 4);
        SDL_SetRenderDrawColor(r, br, bg, bb, (Uint8)(alpha * 120 / 255));
        SDL_RenderDrawRect(r, &bdr);
    }
}

/* ===========================================================================
 * ROUNDED CARD — iOS-style with drop shadow
 * =========================================================================== */
static void draw_card(SDL_Renderer *r, int x, int y, int w, int h)
{
    int radius = 12;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    /* Drop shadow */
    fill_rounded_rect(r, x + 1, y + 3, w, h, radius, CARD_SHADOW);
    SDL_Color shadow2 = { 0, 0, 0, 10 };
    fill_rounded_rect(r, x, y + 2, w + 1, h + 1, radius + 1, shadow2);

    /* White body */
    fill_rounded_rect(r, x, y, w, h, radius, BG_CARD);

    /* Subtle bottom border */
    SDL_SetRenderDrawColor(r, BORDER_DIM.r, BORDER_DIM.g, BORDER_DIM.b, 50);
    SDL_RenderDrawLine(r, x + radius, y + h, x + w - radius, y + h);
}

/* ===========================================================================
 * BACKGROUND PARTICLES
 * =========================================================================== */
static void draw_bg_particles(SDL_Renderer *r, const Game *game)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_BG_PARTICLES; i++) {
        const BGParticle *p = &game->bg_particles[i];
        if (!p->active) continue;
        Uint8 a = (Uint8)(p->alpha * 255.0f);
        SDL_SetRenderDrawColor(r, PARTICLE_COLOR.r, PARTICLE_COLOR.g,
                               PARTICLE_COLOR.b, a);
        SDL_Rect rect = { (int)p->x, (int)p->y, (int)p->size, (int)p->size };
        SDL_RenderFillRect(r, &rect);
    }
}

/* ===========================================================================
 * BOARD
 * =========================================================================== */
static void draw_board_background(SDL_Renderer *r)
{
    int bx = BOARD_OFFSET_X, by = BOARD_OFFSET_Y;
    int bw = BOARD_WIDTH * CELL_SIZE, bh = BOARD_HEIGHT * CELL_SIZE;

    SDL_SetRenderDrawColor(r, BG_BOARD.r, BG_BOARD.g, BG_BOARD.b, 255);
    SDL_Rect bg = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &bg);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, GRID_LINE.r, GRID_LINE.g, GRID_LINE.b, 80);
    for (int col = 1; col < BOARD_WIDTH; col++) {
        int x = bx + col * CELL_SIZE;
        SDL_RenderDrawLine(r, x, by, x, by + bh);
    }
    for (int row = 1; row < BOARD_HEIGHT; row++) {
        int y = by + row * CELL_SIZE;
        SDL_RenderDrawLine(r, bx, y, bx + bw, y);
    }

    SDL_Rect border = { bx - 1, by - 1, bw + 2, bh + 2 };
    SDL_SetRenderDrawColor(r, BOARD_BORDER.r, BOARD_BORDER.g, BOARD_BORDER.b, 255);
    SDL_RenderDrawRect(r, &border);
}

static void draw_board_cells(SDL_Renderer *r, const Board *board)
{
    for (int row = 0; row < BOARD_HEIGHT; row++) {
        for (int col = 0; col < BOARD_WIDTH; col++) {
            int cell = board->cells[row + 4][col];
            if (cell != 0) {
                SDL_Color color = piece_get_color((PieceType)(cell - 1));
                int px = BOARD_OFFSET_X + col * CELL_SIZE;
                int py = BOARD_OFFSET_Y + row * CELL_SIZE;
                draw_glossy_block(r, px, py, CELL_SIZE, color, 255);
            }
        }
    }
}

/* ===========================================================================
 * GHOST PIECE
 * =========================================================================== */
static void draw_ghost(SDL_Renderer *r, const Game *game)
{
    Piece ghost = game->current;
    while (board_piece_fits(&game->board, &ghost)) ghost.y++;
    ghost.y--;
    if (ghost.y <= game->current.y) return;

    const int (*shape)[4] = piece_get_shape(ghost.type, ghost.rotation);
    SDL_Color color = piece_get_color(ghost.type);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int row = 0; row < PIECE_GRID_SIZE; row++) {
        for (int col = 0; col < PIECE_GRID_SIZE; col++) {
            if (!shape[row][col]) continue;
            int sr = ghost.y + row - 4;
            if (sr < 0) continue;
            int px = BOARD_OFFSET_X + (ghost.x + col) * CELL_SIZE;
            int py = BOARD_OFFSET_Y + sr * CELL_SIZE;

            SDL_Rect rect = { px + 3, py + 3, CELL_SIZE - 6, CELL_SIZE - 6 };
            SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 40);
            SDL_RenderFillRect(r, &rect);
            SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 80);
            SDL_RenderDrawRect(r, &rect);
        }
    }
}

/* ===========================================================================
 * CURRENT PIECE (with spawn animation + lock-delay pulse)
 * =========================================================================== */
static void draw_current_piece(SDL_Renderer *r, const Game *game)
{
    const Piece *p = &game->current;
    const int (*shape)[4] = piece_get_shape(p->type, p->rotation);
    SDL_Color color = piece_get_color(p->type);

    Uint8 alpha = 255;
    if (game->spawning) {
        Uint32 elapsed = SDL_GetTicks() - game->spawn_time;
        float progress = (float)elapsed / (float)SPAWN_ANIM_DURATION;
        if (progress > 1.0f) progress = 1.0f;
        alpha = (Uint8)(progress * 255.0f);
    }

    if (game->piece_on_ground && game->lock_timer_active) {
        float t = (float)(SDL_GetTicks() - game->lock_timer) / 150.0f;
        float pulse = 0.65f + 0.35f * sinf(t * 3.14159f * 2.0f);
        alpha = (Uint8)(pulse * (float)alpha);
    }

    for (int row = 0; row < PIECE_GRID_SIZE; row++) {
        for (int col = 0; col < PIECE_GRID_SIZE; col++) {
            if (!shape[row][col]) continue;
            int sr = p->y + row - 4;
            if (sr < 0) continue;
            int px = BOARD_OFFSET_X + (p->x + col) * CELL_SIZE;
            int py = BOARD_OFFSET_Y + sr * CELL_SIZE;
            draw_glossy_block(r, px, py, CELL_SIZE, color, alpha);
        }
    }
}

/* ===========================================================================
 * HARD DROP EFFECTS
 * =========================================================================== */
static void draw_drop_trails(SDL_Renderer *r, const Game *game)
{
    Uint32 now = SDL_GetTicks();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < 4; i++) {
        const DropTrail *t = &game->drop_trails[i];
        if (!t->active) continue;

        Uint32 age = now - t->start_time;
        float progress = (float)age / (float)DROP_TRAIL_DURATION;
        if (progress > 1.0f) progress = 1.0f;
        Uint8 alpha = (Uint8)(100.0f * (1.0f - progress));

        int x = BOARD_OFFSET_X + t->col * CELL_SIZE + CELL_SIZE / 2 - 2;
        int y_start = BOARD_OFFSET_Y + t->start_row * CELL_SIZE;
        int y_end   = BOARD_OFFSET_Y + t->end_row * CELL_SIZE + CELL_SIZE;
        if (y_start < BOARD_OFFSET_Y) y_start = BOARD_OFFSET_Y;

        SDL_SetRenderDrawColor(r, TRAIL_COLOR.r, TRAIL_COLOR.g,
                               TRAIL_COLOR.b, alpha);
        SDL_Rect trail = { x, y_start, 4, y_end - y_start };
        SDL_RenderFillRect(r, &trail);
    }
}

static void draw_drop_particles(SDL_Renderer *r, const Game *game)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_DROP_PARTICLES; i++) {
        const DropParticle *p = &game->drop_particles[i];
        if (!p->active) continue;
        Uint8 a = (Uint8)(p->alpha * 200.0f);
        SDL_SetRenderDrawColor(r, TEXT_SECONDARY.r, TEXT_SECONDARY.g,
                               TEXT_SECONDARY.b, a);
        SDL_Rect rect = { (int)p->x, (int)p->y, (int)p->size, (int)p->size };
        SDL_RenderFillRect(r, &rect);
    }
}

/* ===========================================================================
 * LINE-CLEAR FLASH
 * =========================================================================== */
static void draw_line_clear_flash(SDL_Renderer *r, const Game *game)
{
    if (game->flash_count <= 0) return;

    Uint32 elapsed = SDL_GetTicks() - game->flash_start_time;
    float progress = (float)elapsed / (float)FLASH_DURATION;
    if (progress > 1.0f) progress = 1.0f;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < game->flash_count; i++) {
        int sr = game->flash_rows[i] - 4;
        if (sr < 0 || sr >= BOARD_HEIGHT) continue;

        int px = BOARD_OFFSET_X;
        int py = BOARD_OFFSET_Y + sr * CELL_SIZE;
        Uint8 alpha = (Uint8)(220.0f * (1.0f - progress));

        SDL_SetRenderDrawColor(r, 255, 255, 255, alpha);
        SDL_Rect row_rect = { px, py, BOARD_WIDTH * CELL_SIZE, CELL_SIZE };
        SDL_RenderFillRect(r, &row_rect);

        if (alpha > 60) {
            int ly = py + CELL_SIZE / 2;
            SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(alpha * 0.7f));
            SDL_RenderDrawLine(r, px, ly, px + BOARD_WIDTH * CELL_SIZE, ly);
        }
    }
}

/* ===========================================================================
 * FLOATING TEXT
 * =========================================================================== */
static void draw_float_texts(SDL_Renderer *r, const Game *game)
{
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        const FloatText *ft = &game->float_texts[i];
        if (!ft->active || ft->alpha <= 0.01f) continue;

        Uint8 a = (Uint8)(ft->alpha * 255.0f);
        SDL_Color color = { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b, a };

        SDL_Surface *surf = TTF_RenderText_Blended(game->font_large, ft->text, color);
        if (!surf) continue;
        SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
        if (tex) {
            SDL_SetTextureAlphaMod(tex, a);
            int w = (int)(surf->w * ft->scale);
            int h = (int)(surf->h * ft->scale);
            SDL_Rect dest = {
                (int)(ft->x - w / 2), (int)(ft->y - h / 2), w, h
            };
            SDL_RenderCopy(r, tex, NULL, &dest);
            SDL_DestroyTexture(tex);
        }
        SDL_FreeSurface(surf);
    }
}

/* ===========================================================================
 * SIDEBAR
 * =========================================================================== */
static void draw_sidebar(SDL_Renderer *r, const Game *game)
{
    int sx = BOARD_OFFSET_X + BOARD_WIDTH * CELL_SIZE;
    int card_x = sx + 18;
    int card_w = SIDEBAR_WIDTH - 36;
    int y = BOARD_OFFSET_Y;

    /* NEXT PIECE */
    draw_card(r, card_x, y, card_w, 120);
    draw_text_shadow(r, game->font_label, "NEXT", card_x + 16, y + 10,
                     TEXT_SECONDARY, 0, 1);
    {
        const int (*shape)[4] = piece_get_shape(game->next.type, 0);
        SDL_Color color = piece_get_color(game->next.type);
        int psz = 22;
        int min_c = 4, max_c = 0, min_r = 4, max_r = 0;
        for (int ro = 0; ro < 4; ro++)
            for (int co = 0; co < 4; co++)
                if (shape[ro][co]) {
                    if (co < min_c) min_c = co;
                    if (co > max_c) max_c = co;
                    if (ro < min_r) min_r = ro;
                    if (ro > max_r) max_r = ro;
                }
        int pw = (max_c - min_c + 1) * psz;
        int ph = (max_r - min_r + 1) * psz;
        int ox = card_x + (card_w - pw) / 2 - min_c * psz;
        int oy = y + 32 + (75 - ph) / 2 - min_r * psz;

        for (int ro = 0; ro < PIECE_GRID_SIZE; ro++)
            for (int co = 0; co < PIECE_GRID_SIZE; co++)
                if (shape[ro][co])
                    draw_glossy_block(r, ox + co * psz, oy + ro * psz,
                                     psz, color, 255);
    }
    y += 138;

    /* SCORE */
    draw_card(r, card_x, y, card_w, 75);
    draw_text_shadow(r, game->font_label, "SCORE", card_x + 16, y + 10,
                     TEXT_SECONDARY, 0, 1);
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", game->score);
        draw_text_shadow(r, game->font_score, buf, card_x + 16, y + 32,
                         TEXT_PRIMARY, 1, 2);
    }
    y += 88;

    /* BEST */
    draw_card(r, card_x, y, card_w, 75);
    draw_text_shadow(r, game->font_label, "BEST", card_x + 16, y + 10,
                     TEXT_SECONDARY, 0, 1);
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", game->high_score);
        draw_text_shadow(r, game->font_score, buf, card_x + 16, y + 32,
                         TEXT_PRIMARY, 1, 2);
    }
    y += 88;

    /* LEVEL & LINES side by side */
    {
        int hw = (card_w - 10) / 2;
        draw_card(r, card_x, y, hw, 75);
        draw_text_shadow(r, game->font_label, "LEVEL", card_x + 14, y + 10,
                         TEXT_SECONDARY, 0, 1);
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", game->level);
            draw_text_shadow(r, game->font_score, buf, card_x + 14, y + 32,
                             TEXT_PRIMARY, 1, 2);
        }

        int rx = card_x + hw + 10;
        draw_card(r, rx, y, hw, 75);
        draw_text_shadow(r, game->font_label, "LINES", rx + 14, y + 10,
                         TEXT_SECONDARY, 0, 1);
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", game->lines_cleared);
            draw_text_shadow(r, game->font_score, buf, rx + 14, y + 32,
                             TEXT_PRIMARY, 1, 2);
        }
    }
    y += 90;

    /* COMBO (if active) */
    if (game->combo_count >= 2) {
        char buf[32];
        snprintf(buf, sizeof(buf), "COMBO x%d", game->combo_count);
        draw_text_shadow(r, game->font_label, buf, card_x + 8, y,
                         TEXT_PRIMARY, 0, 1);
        y += 22;
    }

    /* CONTROLS */
    y += 6;
    draw_text_shadow(r, game->font_label, "CONTROLS", card_x + 4, y,
                     TEXT_TERTIARY, 0, 1);
    y += 22;

    const char *controls[][2] = {
        { "<  >",  "Move"      },
        { "Up",    "Rotate"    },
        { "Down",  "Soft Drop" },
        { "Space", "Hard Drop" },
        { "P",     "Pause"     },
        { "R",     "Restart"   },
        { "Esc",   "Menu"      },
    };
    int n = sizeof(controls) / sizeof(controls[0]);
    for (int i = 0; i < n; i++) {
        draw_text(r, game->font_small, controls[i][0], card_x + 8, y,
                  TEXT_PRIMARY);
        draw_text(r, game->font_small, controls[i][1], card_x + 68, y,
                  TEXT_TERTIARY);
        y += 19;
    }
}

/* ===========================================================================
 * OVERLAY SCREENS
 * =========================================================================== */
static void draw_pause_overlay(SDL_Renderer *r, const Game *game)
{
    int bx = BOARD_OFFSET_X, by = BOARD_OFFSET_Y;
    int bw = BOARD_WIDTH * CELL_SIZE, bh = BOARD_HEIGHT * CELL_SIZE;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 190);
    SDL_Rect overlay = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &overlay);

    int cw = 220, ch = 90;
    int cx = bx + (bw - cw) / 2, cy = by + (bh - ch) / 2;

    SDL_Color s = { 0, 0, 0, 20 };
    fill_rounded_rect(r, cx + 1, cy + 4, cw, ch, 16, s);
    fill_rounded_rect(r, cx, cy, cw, ch, 16, BG_CARD);

    draw_text_centered_shadow(r, game->font_large, "PAUSED",
                              cx, cw, cy + 12, TEXT_PRIMARY);
    draw_text_centered(r, game->font_small, "Press P to resume",
                       cx, cw, cy + 58, TEXT_TERTIARY);
}

static void draw_gameover_overlay(SDL_Renderer *r, const Game *game)
{
    int bx = BOARD_OFFSET_X, by = BOARD_OFFSET_Y;
    int bw = BOARD_WIDTH * CELL_SIZE, bh = BOARD_HEIGHT * CELL_SIZE;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
    SDL_Rect overlay = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &overlay);

    int cw = 260, ch = 150;
    int cx = bx + (bw - cw) / 2, cy = by + (bh - ch) / 2;

    SDL_Color s = { 0, 0, 0, 20 };
    fill_rounded_rect(r, cx + 1, cy + 4, cw, ch, 16, s);
    fill_rounded_rect(r, cx, cy, cw, ch, 16, BG_CARD);

    SDL_Color soft_red = { 220, 60, 60, 255 };
    draw_text_centered_shadow(r, game->font_large, "GAME OVER",
                              cx, cw, cy + 12, soft_red);

    {
        char buf[48];
        snprintf(buf, sizeof(buf), "Score: %d", game->score);
        draw_text_centered(r, game->font, buf, cx, cw, cy + 62, TEXT_PRIMARY);
    }

    if (game->score >= game->high_score && game->score > 0) {
        SDL_Color gold = { 200, 160, 40, 255 };
        draw_text_centered(r, game->font_label, "NEW BEST!",
                           cx, cw, cy + 90, gold);
    }

    draw_text_centered(r, game->font_small, "R=Restart  Esc=Menu",
                       cx, cw, cy + 118, TEXT_TERTIARY);
}

/* ===========================================================================
 * MAIN MENU
 * =========================================================================== */
static void draw_menu(SDL_Renderer *r, const Game *game)
{
    /* Leaderboard sub-screen */
    if (game->show_leaderboard) {
        int cw = 300, ch = 260;
        int cx = (WINDOW_WIDTH - cw) / 2;
        int cy = (WINDOW_HEIGHT - ch) / 2;

        SDL_Color s = { 0, 0, 0, 15 };
        fill_rounded_rect(r, cx + 1, cy + 4, cw, ch, 16, s);
        fill_rounded_rect(r, cx, cy, cw, ch, 16, BG_CARD);

        draw_text_centered_shadow(r, game->font_large, "LEADERBOARD",
                                  cx, cw, cy + 14, TEXT_PRIMARY);

        for (int i = 0; i < MAX_LEADERBOARD; i++) {
            char buf[48];
            if (game->leaderboard[i] > 0)
                snprintf(buf, sizeof(buf), "%d.  %d", i + 1, game->leaderboard[i]);
            else
                snprintf(buf, sizeof(buf), "%d.  ---", i + 1);
            draw_text(r, game->font, buf, cx + 50, cy + 60 + i * 32, TEXT_PRIMARY);
        }

        draw_text_centered(r, game->font_small, "Press Enter or Esc to return",
                           cx, cw, cy + ch - 28, TEXT_TERTIARY);
        return;
    }

    /* Controls sub-screen */
    if (game->show_controls) {
        int cw = 340, ch = 320;
        int cx = (WINDOW_WIDTH - cw) / 2;
        int cy = (WINDOW_HEIGHT - ch) / 2;

        SDL_Color s = { 0, 0, 0, 15 };
        fill_rounded_rect(r, cx + 1, cy + 4, cw, ch, 16, s);
        fill_rounded_rect(r, cx, cy, cw, ch, 16, BG_CARD);

        draw_text_centered_shadow(r, game->font_large, "CONTROLS",
                                  cx, cw, cy + 14, TEXT_PRIMARY);

        const char *ctrl_keys[]  = { "Left / Right", "Up", "Down",
                                     "Space", "P", "R", "Esc" };
        const char *ctrl_descs[] = { "Move piece sideways",
                                     "Rotate piece clockwise",
                                     "Soft drop (fall faster)",
                                     "Hard drop (instant slam)",
                                     "Pause / Resume game",
                                     "Restart current game",
                                     "Return to main menu" };
        int nctrl = 7;
        int cy_start = cy + 60;

        for (int i = 0; i < nctrl; i++) {
            draw_text(r, game->font_label, ctrl_keys[i],
                      cx + 24, cy_start + i * 33, TEXT_PRIMARY);
            draw_text(r, game->font_small, ctrl_descs[i],
                      cx + 150, cy_start + i * 33 + 2, TEXT_SECONDARY);
        }

        draw_text_centered(r, game->font_small, "Press Enter or Esc to return",
                           cx, cw, cy + ch - 28, TEXT_TERTIARY);
        return;
    }

    /* Title */
    draw_text_centered_shadow(r, game->font_title, "TETRIS",
                              0, WINDOW_WIDTH, WINDOW_HEIGHT / 4 - 30,
                              TEXT_PRIMARY);

    /* Menu items */
    const char *items[] = { "START", "LEADERBOARD", "CONTROLS", "QUIT" };
    int menu_y = WINDOW_HEIGHT / 2 - 20;
    int item_spacing = 44;

    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        int y = menu_y + i * item_spacing;
        bool selected = (i == game->menu_selection);

        if (selected) {
            float t = (float)SDL_GetTicks() / 300.0f;
            float pulse = 0.85f + 0.15f * sinf(t * 3.14159f * 2.0f);

            int tw, th;
            TTF_SizeText(game->font, items[i], &tw, &th);
            int ix = (WINDOW_WIDTH - tw) / 2;

            /* Selection highlight */
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_Color sel_bg = { 0, 0, 0, (Uint8)(pulse * 12) };
            fill_rounded_rect(r, ix - 30, y - 6, tw + 60, th + 12, 10, sel_bg);

            /* Arrow */
            draw_text(r, game->font, ">", ix - 22, y, TEXT_PRIMARY);

            /* Text */
            SDL_Color c = { TEXT_PRIMARY.r, TEXT_PRIMARY.g, TEXT_PRIMARY.b,
                            (Uint8)(pulse * 255) };
            draw_text_centered(r, game->font, items[i], 0, WINDOW_WIDTH, y, c);
        } else {
            draw_text_centered(r, game->font, items[i],
                               0, WINDOW_WIDTH, y, TEXT_SECONDARY);
        }
    }

    /* Footer */
    draw_text_centered(r, game->font_small, "Up/Down = Navigate   Enter = Select",
                       0, WINDOW_WIDTH, WINDOW_HEIGHT - 40, TEXT_TERTIARY);

    /* Fade transition */
    if (game->menu_fading) {
        Uint8 a = (Uint8)(game->menu_fade_alpha * 255.0f);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, BG_WINDOW.r, BG_WINDOW.g, BG_WINDOW.b, a);
        SDL_Rect full = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
        SDL_RenderFillRect(r, &full);
    }
}

/* ===========================================================================
 * render_frame() — Main render, called every frame
 * =========================================================================== */
void render_frame(Game *game)
{
    SDL_Renderer *r = game->renderer;

    /* Clear to light gray background */
    SDL_SetRenderDrawColor(r, BG_WINDOW.r, BG_WINDOW.g, BG_WINDOW.b, 255);
    SDL_RenderClear(r);

    draw_bg_particles(r, game);

    if (game->state == STATE_MENU) {
        draw_menu(r, game);
        SDL_RenderPresent(r);
        return;
    }

    draw_board_background(r);
    draw_board_cells(r, &game->board);
    draw_drop_trails(r, game);

    if (game->state == STATE_PLAYING || game->state == STATE_PAUSED) {
        draw_ghost(r, game);
        draw_current_piece(r, game);
    }

    draw_line_clear_flash(r, game);
    draw_drop_particles(r, game);
    draw_float_texts(r, game);
    draw_sidebar(r, game);

    if (game->state == STATE_PAUSED)
        draw_pause_overlay(r, game);
    else if (game->state == STATE_GAME_OVER)
        draw_gameover_overlay(r, game);

    SDL_RenderPresent(r);
}
