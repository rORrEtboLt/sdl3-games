#include "ui.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void draw_corner_ornament(SDL_Renderer *r, float x, float y, int flip_x, int flip_y) {
    float dx = flip_x ? -1.0f : 1.0f;
    float dy = flip_y ? -1.0f : 1.0f;
    SDL_SetRenderDrawColor(r, 229, 174, 58, 140);
    SDL_RenderLine(r, x, y, x + 14 * dx, y);
    SDL_RenderLine(r, x, y, x, y + 14 * dy);
    SDL_RenderLine(r, x + 5 * dx, y, x, y + 5 * dy);
    SDL_FRect dot = {x + 2 * dx - 1, y + 2 * dy - 1, 3, 3};
    SDL_RenderFillRect(r, &dot);
}

static void draw_panel_ornaments(SDL_Renderer *r, float px, float py, float pw, float ph) {
    draw_corner_ornament(r, px, py, 0, 0);
    draw_corner_ornament(r, px + pw, py, 1, 0);
    draw_corner_ornament(r, px, py + ph, 0, 1);
    draw_corner_ornament(r, px + pw, py + ph, 1, 1);
    SDL_SetRenderDrawColor(r, 229, 174, 58, 100);
    float cx = px + pw / 2;
    SDL_FRect diamond = {cx - 3, py - 4, 6, 6};
    SDL_RenderFillRect(r, &diamond);
    SDL_FRect bottom_diamond = {cx - 3, py + ph - 2, 6, 6};
    SDL_RenderFillRect(r, &bottom_diamond);
}

static void draw_lotus_divider(SDL_Renderer *r, float cx, float y, float half_w) {
    SDL_SetRenderDrawColor(r, 229, 174, 58, 100);
    SDL_RenderLine(r, cx - half_w, y, cx - 12, y);
    SDL_RenderLine(r, cx + 12, y, cx + half_w, y);
    SDL_SetRenderDrawColor(r, 229, 174, 58, 160);
    SDL_FRect center = {cx - 2, y - 2, 5, 5};
    SDL_RenderFillRect(r, &center);
    SDL_FRect left = {cx - 8, y - 1, 3, 3};
    SDL_FRect right = {cx + 6, y - 1, 3, 3};
    SDL_RenderFillRect(r, &left);
    SDL_RenderFillRect(r, &right);
}

static void setup_theme(struct nk_context *ctx) {
    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT]                    = nk_rgba(226, 232, 214, 255);
    table[NK_COLOR_WINDOW]                  = nk_rgba(31, 24, 47, 238);
    table[NK_COLOR_HEADER]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_BORDER]                  = nk_rgba(64, 98, 135, 255);
    table[NK_COLOR_BUTTON]                  = nk_rgba(184, 135, 42, 240);
    table[NK_COLOR_BUTTON_HOVER]            = nk_rgba(229, 174, 58, 250);
    table[NK_COLOR_BUTTON_ACTIVE]           = nk_rgba(204, 73, 118, 255);
    table[NK_COLOR_TOGGLE]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_TOGGLE_HOVER]            = nk_rgba(64, 98, 135, 245);
    table[NK_COLOR_TOGGLE_CURSOR]           = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_SELECT]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_SELECT_ACTIVE]           = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_SLIDER]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_SLIDER_CURSOR]           = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_SLIDER_CURSOR_HOVER]     = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE]    = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_PROPERTY]                = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_EDIT]                    = nk_rgba(20, 15, 32, 245);
    table[NK_COLOR_EDIT_CURSOR]             = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_COMBO]                   = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_CHART]                   = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_CHART_COLOR]             = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT]   = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_SCROLLBAR]               = nk_rgba(20, 15, 32, 245);
    table[NK_COLOR_SCROLLBAR_CURSOR]        = nk_rgba(64, 98, 135, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER]  = nk_rgba(229, 174, 58, 200);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_TAB_HEADER]              = nk_rgba(42, 48, 74, 245);
    nk_style_from_table(ctx, table);

    ctx->style.button.rounding = 0;
    ctx->style.button.padding = nk_vec2(12, 8);
    ctx->style.button.text_normal = nk_rgba(31, 24, 47, 255);
    ctx->style.button.text_hover = nk_rgba(31, 24, 47, 255);
    ctx->style.button.text_active = nk_rgba(226, 232, 214, 255);
    ctx->style.window.spacing = nk_vec2(8, 8);
    ctx->style.window.padding = nk_vec2(16, 16);
}

void ui_init(Game *game) {
    struct nk_sdl3 *sdl = nk_sdl3_init(game->renderer, VIRTUAL_W, VIRTUAL_H);
    if (sdl) setup_theme(&sdl->ctx);
    game->nk_sdl = sdl;
}

void ui_shutdown(Game *game) {
    nk_sdl3_shutdown((struct nk_sdl3 *)game->nk_sdl);
    game->nk_sdl = NULL;
}

void ui_handle_event(Game *game, SDL_Event *event) {
    struct nk_sdl3 *sdl = (struct nk_sdl3 *)game->nk_sdl;
    if (sdl) nk_sdl3_handle_event(sdl, event);
}

void ui_render_splash(Game *game) {
    struct nk_sdl3 *sdl = (struct nk_sdl3 *)game->nk_sdl;
    if (!sdl) return;
    struct nk_context *ctx = &sdl->ctx;

    nk_sdl3_new_frame(sdl);

    float pw = 300, ph = 200;
    float px = (VIRTUAL_W - pw) / 2, py = 400;

    if (nk_begin(ctx, "splash_ui", nk_rect(px, py, pw, ph),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {
        if (game->splash_timer > 2.0f) {
            nk_layout_row_dynamic(ctx, 20, 1);
            nk_spacing(ctx, 1);
            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "TAP TO CONTINUE")) {
                game->state = STATE_MENU;
                game->menu_anim_timer = 0;
            }
        }
    }
    nk_end(ctx);

    nk_sdl3_render(sdl);
    draw_panel_ornaments(game->renderer, px, py, pw, ph);
}

void ui_render_menu(Game *game) {
    struct nk_sdl3 *sdl = (struct nk_sdl3 *)game->nk_sdl;
    if (!sdl) return;
    struct nk_context *ctx = &sdl->ctx;

    nk_sdl3_new_frame(sdl);

    float panel_w = 340, panel_h = 520;
    float px = (VIRTUAL_W - panel_w) / 2;
    float py = 30;

    if (nk_begin(ctx, "menu", nk_rect(px, py, panel_w, panel_h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

        nk_layout_row_dynamic(ctx, 15, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label_colored(ctx, "M A A Y A V I", NK_TEXT_CENTERED,
                         nk_rgba(226, 232, 214, 255));

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label_colored(ctx, "Dice Lane Defense", NK_TEXT_CENTERED,
                         nk_rgba(229, 174, 58, 200));

        nk_layout_row_dynamic(ctx, 50, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 50, 1);
        if (nk_button_label(ctx, "PLAY")) {
            game_start(game);
        }

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 50, 1);
        if (nk_button_label(ctx, "SETTINGS")) {
            game->state = STATE_SETTINGS;
            game->settings_selection = 0;
        }

        nk_layout_row_dynamic(ctx, 30, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label_colored(ctx, "v1.0", NK_TEXT_CENTERED,
                         nk_rgba(117, 113, 143, 180));
    }
    nk_end(ctx);

    nk_sdl3_render(sdl);

    float menu_px = (VIRTUAL_W - 340) / 2, menu_py = 30;
    draw_panel_ornaments(game->renderer, menu_px, menu_py, 340, 520);
    draw_lotus_divider(game->renderer, VIRTUAL_W / 2.0f, menu_py + 95, 100);
}

void ui_render_settings(Game *game) {
    struct nk_sdl3 *sdl = (struct nk_sdl3 *)game->nk_sdl;
    if (!sdl) return;
    struct nk_context *ctx = &sdl->ctx;

    nk_sdl3_new_frame(sdl);

    float panel_w = 310, panel_h = 540;
    float px = (VIRTUAL_W - panel_w) / 2;
    float py = 40;

    struct nk_vec2 old_pad = ctx->style.window.padding;
    ctx->style.window.padding = nk_vec2(10, 12);

    if (nk_begin(ctx, "settings", nk_rect(px, py, panel_w, panel_h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

        nk_layout_row_dynamic(ctx, 8, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label_colored(ctx, "SETTINGS", NK_TEXT_CENTERED,
                         nk_rgba(229, 174, 58, 255));

        nk_layout_row_dynamic(ctx, 12, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 18, 1);
        nk_label(ctx, "Screen Shake", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 35, 1);
        if (nk_button_label(ctx, game->settings.screen_shake ? "ON" : "OFF"))
            game->settings.screen_shake = !game->settings.screen_shake;

        nk_layout_row_dynamic(ctx, 12, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 18, 1);
        nk_label(ctx, "Difficulty", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 35, 1);
        const char *diff_names[] = {"EASY", "NORMAL", "HARD"};
        if (nk_button_label(ctx, diff_names[game->settings.difficulty]))
            game->settings.difficulty = (game->settings.difficulty + 1) % 3;

        nk_layout_row_dynamic(ctx, 16, 1);
        const char *diff_desc[] = {
            "Fewer, slower foes",
            "Balanced play",
            "More, faster foes"
        };
        nk_label_colored(ctx, diff_desc[game->settings.difficulty], NK_TEXT_CENTERED,
                         nk_rgba(117, 113, 143, 230));

        nk_layout_row_dynamic(ctx, 16, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 18, 1);
        nk_label_colored(ctx, "How to Play", NK_TEXT_LEFT,
                         nk_rgba(229, 174, 58, 200));

        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label_colored(ctx, "Tap die to roll.", NK_TEXT_LEFT, nk_rgba(201, 238, 218, 210));
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label_colored(ctx, "Drag unit to a lane.", NK_TEXT_LEFT, nk_rgba(201, 238, 218, 210));
        nk_layout_row_dynamic(ctx, 15, 1);
        nk_label_colored(ctx, "Stop the demons!", NK_TEXT_LEFT, nk_rgba(201, 238, 218, 210));

        nk_layout_row_dynamic(ctx, 12, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 40, 1);
        if (nk_button_label(ctx, "BACK")) {
            game->state = STATE_MENU;
            game->menu_anim_timer = 0;
        }
    }
    nk_end(ctx);

    ctx->style.window.padding = old_pad;

    nk_sdl3_render(sdl);

    draw_panel_ornaments(game->renderer, px, py, panel_w, panel_h);
    draw_lotus_divider(game->renderer, VIRTUAL_W / 2.0f, py + 55, 90);
}

void ui_render_game_over(Game *game) {
    struct nk_sdl3 *sdl = (struct nk_sdl3 *)game->nk_sdl;
    if (!sdl) return;
    struct nk_context *ctx = &sdl->ctx;

    nk_sdl3_new_frame(sdl);

    float panel_w = 340, panel_h = 400;
    float px = (VIRTUAL_W - panel_w) / 2;
    float py = 80;

    if (nk_begin(ctx, "gameover", nk_rect(px, py, panel_w, panel_h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

        nk_layout_row_dynamic(ctx, 15, 1);
        nk_spacing(ctx, 1);

        char buf[64];
        SDL_snprintf(buf, sizeof(buf), "Wave %d Reached", game->wave);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label_colored(ctx, buf, NK_TEXT_CENTERED, nk_rgba(255, 200, 50, 255));

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacing(ctx, 1);

        SDL_snprintf(buf, sizeof(buf), "Score: %d", game->score);
        nk_layout_row_dynamic(ctx, 26, 1);
        nk_label_colored(ctx, buf, NK_TEXT_CENTERED, nk_rgba(255, 220, 80, 255));

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_spacing(ctx, 1);

#ifdef __EMSCRIPTEN__
        /* Share button -- primary action (gold styling) */
        nk_style_push_color(ctx, &ctx->style.button.normal.data.color, nk_rgba(180, 140, 30, 255));
        nk_style_push_color(ctx, &ctx->style.button.hover.data.color, nk_rgba(210, 170, 40, 255));
        nk_style_push_color(ctx, &ctx->style.button.active.data.color, nk_rgba(230, 190, 50, 255));
        nk_layout_row_dynamic(ctx, 50, 1);
        if (nk_button_label(ctx, "CHALLENGE A FRIEND")) {
            EM_ASM({
                var score = $0;
                var url = window.location.origin + window.location.pathname + '?score=' + score;
                var text = 'I scored ' + score + ' in Maayavi — can you beat me? ' + url;
                navigator.clipboard.writeText(text).then(function() {
                    var fb = document.getElementById('share-feedback');
                    if (fb) { fb.textContent = 'Link copied!'; fb.style.display = 'block'; }
                    setTimeout(function() { if (fb) fb.style.display = 'none'; }, 2000);
                }).catch(function() {
                    var fb = document.getElementById('share-fallback');
                    if (fb) {
                        fb.style.display = 'block';
                        var inp = document.getElementById('share-fallback-input');
                        if (inp) { inp.value = text; inp.select(); }
                    }
                });
            }, game->score);
        }
        nk_style_pop_color(ctx);
        nk_style_pop_color(ctx);
        nk_style_pop_color(ctx);

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 40, 1);
        if (nk_button_label(ctx, "PLAY AGAIN")) {
            game_start(game);
        }
#else
        nk_layout_row_dynamic(ctx, 50, 1);
        if (nk_button_label(ctx, "BACK TO MENU")) {
            game->state = STATE_MENU;
            game->menu_anim_timer = 0;
        }
#endif
    }
    nk_end(ctx);

    nk_sdl3_render(sdl);

    float go_px = (VIRTUAL_W - 340) / 2, go_py = 80;
    draw_panel_ornaments(game->renderer, go_px, go_py, 340, 400);
    draw_lotus_divider(game->renderer, VIRTUAL_W / 2.0f, go_py + 80, 100);
}
