#include "ui.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void setup_theme(struct nk_context *ctx) {
    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT]                    = nk_rgba(226, 232, 214, 255);
    table[NK_COLOR_WINDOW]                  = nk_rgba(31, 24, 47, 238);
    table[NK_COLOR_HEADER]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_BORDER]                  = nk_rgba(64, 98, 135, 255);
    table[NK_COLOR_BUTTON]                  = nk_rgba(64, 98, 135, 240);
    table[NK_COLOR_BUTTON_HOVER]            = nk_rgba(92, 168, 151, 245);
    table[NK_COLOR_BUTTON_ACTIVE]           = nk_rgba(204, 73, 118, 255);
    table[NK_COLOR_TOGGLE]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_TOGGLE_HOVER]            = nk_rgba(64, 98, 135, 245);
    table[NK_COLOR_TOGGLE_CURSOR]           = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_SELECT]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_SELECT_ACTIVE]           = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_SLIDER]                  = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_SLIDER_CURSOR]           = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_SLIDER_CURSOR_HOVER]     = nk_rgba(201, 238, 218, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE]    = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_PROPERTY]                = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_EDIT]                    = nk_rgba(20, 15, 32, 245);
    table[NK_COLOR_EDIT_CURSOR]             = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_COMBO]                   = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_CHART]                   = nk_rgba(42, 48, 74, 245);
    table[NK_COLOR_CHART_COLOR]             = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT]   = nk_rgba(229, 174, 58, 255);
    table[NK_COLOR_SCROLLBAR]               = nk_rgba(20, 15, 32, 245);
    table[NK_COLOR_SCROLLBAR_CURSOR]        = nk_rgba(64, 98, 135, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER]  = nk_rgba(92, 168, 151, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(120, 239, 229, 255);
    table[NK_COLOR_TAB_HEADER]              = nk_rgba(42, 48, 74, 245);
    nk_style_from_table(ctx, table);

    ctx->style.button.rounding = 0;
    ctx->style.button.padding = nk_vec2(12, 8);
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
                         nk_rgba(120, 239, 229, 230));

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
}

void ui_render_settings(Game *game) {
    struct nk_sdl3 *sdl = (struct nk_sdl3 *)game->nk_sdl;
    if (!sdl) return;
    struct nk_context *ctx = &sdl->ctx;

    nk_sdl3_new_frame(sdl);

    float panel_w = 360, panel_h = 580;
    float px = (VIRTUAL_W - panel_w) / 2;
    float py = 20;

    if (nk_begin(ctx, "settings", nk_rect(px, py, panel_w, panel_h),
                 NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER)) {

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_label_colored(ctx, "SETTINGS", NK_TEXT_CENTERED,
                         nk_rgba(226, 232, 214, 255));

        nk_layout_row_dynamic(ctx, 15, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Screen Shake", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 35, 1);
        if (nk_button_label(ctx, game->settings.screen_shake ? "ON" : "OFF"))
            game->settings.screen_shake = !game->settings.screen_shake;

        nk_layout_row_dynamic(ctx, 15, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Difficulty", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 35, 1);
        const char *diff_names[] = {"EASY", "NORMAL", "HARD"};
        if (nk_button_label(ctx, diff_names[game->settings.difficulty]))
            game->settings.difficulty = (game->settings.difficulty + 1) % 3;

        nk_layout_row_dynamic(ctx, 18, 1);
        const char *diff_desc[] = {
            "Fewer enemies, slower spawns",
            "Balanced challenge",
            "More enemies, faster spawns"
        };
        nk_label_colored(ctx, diff_desc[game->settings.difficulty], NK_TEXT_CENTERED,
                         nk_rgba(117, 113, 143, 230));

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label_colored(ctx, "How to Play", NK_TEXT_LEFT,
                         nk_rgba(120, 239, 229, 230));

        nk_layout_row_dynamic(ctx, 16, 1);
        nk_label_colored(ctx, "Tap a die to roll it.", NK_TEXT_LEFT, nk_rgba(201, 238, 218, 210));
        nk_layout_row_dynamic(ctx, 16, 1);
        nk_label_colored(ctx, "Drag the summoned warrior", NK_TEXT_LEFT, nk_rgba(201, 238, 218, 210));
        nk_layout_row_dynamic(ctx, 16, 1);
        nk_label_colored(ctx, "onto a lane to place it.", NK_TEXT_LEFT, nk_rgba(201, 238, 218, 210));
        nk_layout_row_dynamic(ctx, 16, 1);
        nk_label_colored(ctx, "Stop the demons!", NK_TEXT_LEFT, nk_rgba(201, 238, 218, 210));

        nk_layout_row_dynamic(ctx, 15, 1);
        nk_spacing(ctx, 1);

        nk_layout_row_dynamic(ctx, 45, 1);
        if (nk_button_label(ctx, "BACK")) {
            game->state = STATE_MENU;
            game->menu_anim_timer = 0;
        }
    }
    nk_end(ctx);

    nk_sdl3_render(sdl);
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
}
