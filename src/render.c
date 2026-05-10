#include "game.h"
#include "sprites.h"
#include <stdio.h>

static const struct { Uint8 r, g, b; } UNIT_COLORS[] = {
    [UNIT_NONE]     = {128, 128, 128},
    [UNIT_SPEARMAN] = {180, 130, 60},
    [UNIT_ARCHER]   = {70,  125, 55},
    [UNIT_KNIGHT]   = {160, 165, 180},
    [UNIT_CAMEL]    = {195, 165, 105},
    [UNIT_SAGE]     = {230, 220, 200},
    [UNIT_ELEPHANT] = {135, 135, 140},
};

static const struct { Uint8 r, g, b; } PROJ_COLORS[] = {
    [UNIT_NONE]     = {255, 220, 50},
    [UNIT_SPEARMAN] = {255, 220, 50},
    [UNIT_ARCHER]   = {200, 170, 80},
    [UNIT_KNIGHT]   = {200, 200, 210},
    [UNIT_CAMEL]    = {180, 80,  40},
    [UNIT_SAGE]     = {100, 255, 100},
    [UNIT_ELEPHANT] = {255, 120, 20},
};

static void draw_filled_circle(SDL_Renderer *r, float cx, float cy, float radius) {
    for (float y = -radius; y <= radius; y += 1.0f) {
        float hw = sqrtf(radius * radius - y * y);
        SDL_RenderLine(r, cx - hw, cy + y, cx + hw, cy + y);
    }
}

static void draw_health_bar(SDL_Renderer *r, float x, float y, float w, float hp, float max_hp) {
    float pct = hp / max_hp;
    if (pct > 1.0f) pct = 1.0f;
    if (pct < 0.0f) pct = 0.0f;
    SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
    SDL_FRect bg = {x - w/2, y, w, 4};
    SDL_RenderFillRect(r, &bg);
    Uint8 cr = (Uint8)(255 * (1.0f - pct));
    Uint8 cg = (Uint8)(255 * pct);
    SDL_SetRenderDrawColor(r, cr, cg, 0, 255);
    SDL_FRect fill = {x - w/2, y, w * pct, 4};
    SDL_RenderFillRect(r, &fill);
}

static void draw_die_dots(SDL_Renderer *r, float cx, float cy, float size, int face) {
    SDL_SetRenderDrawColor(r, 40, 40, 40, 255);
    float dr = size * 0.1f, off = size * 0.25f;
    if (face == 1 || face == 3 || face == 5) draw_filled_circle(r, cx, cy, dr);
    if (face >= 2) { draw_filled_circle(r, cx+off, cy-off, dr); draw_filled_circle(r, cx-off, cy+off, dr); }
    if (face >= 4) { draw_filled_circle(r, cx-off, cy-off, dr); draw_filled_circle(r, cx+off, cy+off, dr); }
    if (face == 6) { draw_filled_circle(r, cx-off, cy, dr); draw_filled_circle(r, cx+off, cy, dr); }
}

static void draw_die(SDL_Renderer *r, Dice *d) {
    float size = 50, half = 25;
    if (d->state == DICE_COOLDOWN_STATE) SDL_SetRenderDrawColor(r, 100, 90, 80, 255);
    else if (d->state == DICE_DRAGGING) SDL_SetRenderDrawColor(r, 255, 250, 235, 255);
    else SDL_SetRenderDrawColor(r, 245, 240, 220, 255);
    SDL_FRect body = {d->x-half, d->y-half, size, size};
    SDL_RenderFillRect(r, &body);
    SDL_SetRenderDrawColor(r, d->state == DICE_READY ? 200 : 80, d->state == DICE_READY ? 180 : 70, d->state == DICE_READY ? 100 : 60, 255);
    SDL_RenderRect(r, &body);
    if (d->state != DICE_COOLDOWN_STATE) {
        draw_die_dots(r, d->x, d->y, size, d->face);
    } else {
        float pct = d->cooldown_timer / DICE_COOLDOWN;
        SDL_SetRenderDrawColor(r, 180, 160, 120, 255);
        SDL_FRect cd = {d->x-half+5, d->y+half-10, (size-10)*(1.0f-pct), 5};
        SDL_RenderFillRect(r, &cd);
    }
}

/* ---------- sprite rendering ---------- */

static void draw_sprite(SDL_Renderer *r, SDL_Texture *atlas,
                        SDL_FRect src, float x, float y, float base_size,
                        AnimComp *anim, float extra_y_off) {
    float w = base_size * anim->scale_x;
    float h = base_size * anim->scale_y;
    float dy = (base_size - h) * 0.5f;
    SDL_FRect dst = {x - w/2, y - h/2 + dy + extra_y_off, w, h};

    SDL_SetTextureColorMod(atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas, 255);
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
    SDL_RenderTexture(r, atlas, &src, &dst);

    if (anim->flash_active) {
        SDL_SetTextureColorMod(atlas, 255, 255, 255);
        SDL_SetTextureAlphaMod(atlas, 180);
        SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_ADD);
        SDL_RenderTexture(r, atlas, &src, &dst);
        SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(atlas, 255);
    }
}

static void draw_ground_shadow(SDL_Renderer *r, float x, float y, float w) {
    SDL_SetRenderDrawColor(r, 0, 0, 0, 35);
    float hw = w * 0.5f, hh = w * 0.18f;
    for (float dy = -hh; dy <= hh; dy += 1.0f) {
        float span = hw * sqrtf(1.0f - (dy * dy) / (hh * hh));
        SDL_RenderLine(r, x - span, y + dy, x + span, y + dy);
    }
}

static void draw_unit_sprite(SDL_Renderer *r, SDL_Texture *atlas, Unit *u) {
    draw_ground_shadow(r, u->x, u->y + 14, 28);
    SDL_FRect src = sprite_unit_src(u->type, u->anim.frame);
    draw_sprite(r, atlas, src, u->x, u->y, 36, &u->anim, 0);
    if (u->hp < u->max_hp)
        draw_health_bar(r, u->x, u->y - 22, 30, u->hp, u->max_hp);
}

static void draw_enemy_sprite(SDL_Renderer *r, SDL_Texture *atlas, Enemy *e) {
    float base = (e->type == ENEMY_ASURA) ? 52.0f : 36.0f;
    draw_ground_shadow(r, e->x, e->y + 14, base * 0.8f);
    SDL_FRect src = sprite_enemy_src(e->type, e->anim.frame);
    float walk_bob = sinf(e->walk_phase) * 2.0f;
    draw_sprite(r, atlas, src, e->x, e->y, base, &e->anim, walk_bob);
    float bar_off = (e->type == ENEMY_ASURA) ? 30.0f : 22.0f;
    draw_health_bar(r, e->x, e->y - bar_off, 26, e->hp, e->max_hp);
}

/* ---------- screen renders ---------- */

static void render_title(Game *game) {
    SDL_Renderer *r = game->renderer;
    SDL_SetRenderDrawColor(r, 25, 20, 35, 255);
    SDL_RenderClear(r);

    SDL_SetRenderDrawColor(r, 255, 200, 50, 255);
    SDL_SetRenderScale(r, 4.0f, 4.0f);
    SDL_RenderDebugText(r, (VIRTUAL_W/4.0f - 11*8)/2, 40, "RANDOMANCER");
    SDL_SetRenderScale(r, 1, 1);

    SDL_SetRenderDrawColor(r, 200, 180, 140, 255);
    SDL_SetRenderScale(r, 2.0f, 2.0f);
    SDL_RenderDebugText(r, (VIRTUAL_W/2.0f - 17*8)/2, 120, "Dice Lane Defense");
    SDL_SetRenderScale(r, 1, 1);

    float die_x = VIRTUAL_W/2.0f, die_y = 360;
    SDL_SetRenderDrawColor(r, 245, 240, 220, 255);
    SDL_FRect die = {die_x-40, die_y-40, 80, 80};
    SDL_RenderFillRect(r, &die);
    SDL_SetRenderDrawColor(r, 200, 180, 100, 255);
    SDL_RenderRect(r, &die);
    draw_die_dots(r, die_x, die_y, 80, 6);

    Uint64 t = SDL_GetTicks();
    float a = 0.5f + 0.5f * sinf((float)t * 0.003f);
    Uint8 v = (Uint8)(200*a);
    SDL_SetRenderDrawColor(r, v, v, v, 255);
    SDL_SetRenderScale(r, 2.0f, 2.0f);
    SDL_RenderDebugText(r, (VIRTUAL_W/2.0f - 12*8)/2, 260, "TAP TO START");
    SDL_SetRenderScale(r, 1, 1);
}

static void render_playing(Game *game) {
    SDL_Renderer *r = game->renderer;
    float sx = game->shake_x, sy = game->shake_y;

    SDL_SetRenderDrawColor(r, 30, 35, 25, 255);
    SDL_RenderClear(r);

    /* lanes */
    for (int i = 0; i < NUM_LANES; i++) {
        SDL_SetRenderDrawColor(r, (i%2==0)?35:40, (i%2==0)?50:55, (i%2==0)?30:35, 255);
        SDL_FRect lane = {i*LANE_W+sx, FIELD_Y+sy, (float)LANE_W, (float)FIELD_H};
        SDL_RenderFillRect(r, &lane);
    }

    /* lane highlight */
    if (game->dragging_dice >= 0 && game->drag_y >= FIELD_Y && game->drag_y <= FIELD_Y+FIELD_H) {
        int hl = (int)(game->drag_x / LANE_W);
        if (hl >= 0 && hl < NUM_LANES) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, 30);
            SDL_FRect h = {hl*LANE_W+sx, FIELD_Y+sy, (float)LANE_W, (float)FIELD_H};
            SDL_RenderFillRect(r, &h);
        }
    }

    /* lane dividers */
    SDL_SetRenderDrawColor(r, 60, 70, 50, 255);
    for (int i = 1; i < NUM_LANES; i++)
        SDL_RenderLine(r, i*LANE_W+sx, FIELD_Y+sy, i*LANE_W+sx, FIELD_Y+FIELD_H+sy);

    SDL_SetRenderDrawColor(r, 80, 90, 60, 255);
    SDL_FRect fb = {sx, FIELD_Y+sy, (float)VIRTUAL_W, (float)FIELD_H};
    SDL_RenderRect(r, &fb);

    /* blood decals */
    for (int i = 0; i < MAX_DECALS; i++) {
        Decal *d = &game->decals[i];
        if (d->a == 0) continue;
        SDL_SetRenderDrawColor(r, d->r, d->g, d->b, d->a);
        draw_filled_circle(r, d->x + sx, d->y + sy, d->size);
    }

    /* projectiles */
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) continue;
        int st = p->source_type;
        if (st < 0 || st > 6) st = 0;
        SDL_SetRenderDrawColor(r, PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 255);
        draw_filled_circle(r, p->x+sx, p->y+sy, 4);
        /* trail */
        SDL_SetRenderDrawColor(r, PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 100);
        draw_filled_circle(r, p->x+sx, p->y+sy+6, 3);
        draw_filled_circle(r, p->x+sx, p->y+sy+11, 2);
    }

    /* enemies */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        float ox = e->x, oy = e->y;
        e->x += sx; e->y += sy;
        draw_enemy_sprite(r, game->sprite_atlas, e);
        e->x = ox; e->y = oy;
    }

    /* units */
    for (int i = 0; i < MAX_UNITS; i++) {
        Unit *u = &game->units[i];
        if (!u->active) continue;
        float ox = u->x, oy = u->y;
        u->x += sx; u->y += sy;
        draw_unit_sprite(r, game->sprite_atlas, u);
        u->x = ox; u->y = oy;
    }

    /* particles */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &game->particles[i];
        if (!p->active) continue;
        float alpha = p->life / p->max_life;
        if (alpha < 0) alpha = 0;
        float px = p->x + sx, py = p->y + sy;

        switch (p->ptype) {
        case PTYPE_CIRCLE:
            SDL_SetRenderDrawColor(r, p->r, p->g, p->b, (Uint8)(255*alpha));
            draw_filled_circle(r, px, py, p->size * alpha);
            break;
        case PTYPE_BLOOD_DROP: {
            SDL_SetRenderDrawColor(r, p->r, p->g, p->b, (Uint8)(220*alpha));
            float sz = p->size;
            float cs = cosf(p->rotation), sn = sinf(p->rotation);
            float hw = sz * 2, hh = sz;
            /* elongated along velocity — approximate with 2 overlapping circles */
            draw_filled_circle(r, px + cs*hw*0.3f, py + sn*hw*0.3f, hh);
            draw_filled_circle(r, px - cs*hw*0.3f, py - sn*hw*0.3f, hh * 0.7f);
            break;
        }
        case PTYPE_SPARK:
            SDL_SetRenderDrawColor(r, p->r, p->g, p->b, (Uint8)(255*alpha));
            draw_filled_circle(r, px, py, p->size);
            break;
        case PTYPE_SMOKE:
            SDL_SetRenderDrawColor(r, p->r, p->g, p->b, (Uint8)(80*alpha));
            draw_filled_circle(r, px, py, p->size);
            break;
        }
    }

    /* screen flash */
    if (game->screen_flash_timer > 0) {
        float fa = game->screen_flash_timer / 0.25f;
        if (fa > 1.0f) fa = 1.0f;
        SDL_SetRenderDrawColor(r, game->flash_r, game->flash_g, game->flash_b, (Uint8)(120*fa));
        SDL_FRect full = {0, 0, (float)VIRTUAL_W, (float)VIRTUAL_H};
        SDL_RenderFillRect(r, &full);
    }

    /* tray */
    SDL_SetRenderDrawColor(r, 50, 35, 25, 255);
    SDL_FRect tray = {0, (float)TRAY_Y, (float)VIRTUAL_W, (float)TRAY_H};
    SDL_RenderFillRect(r, &tray);
    SDL_SetRenderDrawColor(r, 80, 60, 40, 255);
    SDL_RenderLine(r, 0, (float)TRAY_Y, (float)VIRTUAL_W, (float)TRAY_Y);
    SDL_RenderLine(r, 0, TRAY_Y+1.0f, (float)VIRTUAL_W, TRAY_Y+1.0f);

    /* dice */
    for (int i = 0; i < MAX_DICE; i++) {
        if (i == game->dragging_dice) continue;
        draw_die(r, &game->dice[i]);
    }
    if (game->dragging_dice >= 0)
        draw_die(r, &game->dice[game->dragging_dice]);

    /* HUD */
    SDL_SetRenderDrawColor(r, 20, 15, 30, 220);
    SDL_FRect hud = {0, 0, (float)VIRTUAL_W, (float)HUD_H};
    SDL_RenderFillRect(r, &hud);

    char buf[64];
    SDL_SetRenderScale(r, 1.5f, 1.5f);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    if (game->wave > 0) SDL_snprintf(buf, sizeof(buf), "Wave %d", game->wave);
    else SDL_snprintf(buf, sizeof(buf), "Get Ready!");
    SDL_RenderDebugText(r, 6, 8, buf);

    SDL_snprintf(buf, sizeof(buf), "Score %d", game->score);
    SDL_RenderDebugText(r, VIRTUAL_W/1.5f/2 - 30, 8, buf);

    SDL_SetRenderDrawColor(r, 255, 80, 80, 255);
    SDL_snprintf(buf, sizeof(buf), "HP %d", game->player_hp);
    SDL_RenderDebugText(r, VIRTUAL_W/1.5f - 50, 8, buf);
    SDL_SetRenderScale(r, 1, 1);

    /* wave banner */
    if (game->wave_banner_timer > 0) {
        float ba = 1.0f;
        if (game->wave_banner_timer < 0.5f) ba = game->wave_banner_timer / 0.5f;
        if (game->wave_banner_timer > 1.5f) ba = (2.0f - game->wave_banner_timer) / 0.5f;
        if (ba < 0) ba = 0;
        if (ba > 1) ba = 1;
        SDL_SetRenderDrawColor(r, 0, 0, 0, (Uint8)(160*ba));
        SDL_FRect banner = {0, VIRTUAL_H/2.0f - 30, (float)VIRTUAL_W, 60};
        SDL_RenderFillRect(r, &banner);
        SDL_SetRenderDrawColor(r, 255, 220, 50, (Uint8)(255*ba));
        SDL_SetRenderScale(r, 3.0f, 3.0f);
        SDL_snprintf(buf, sizeof(buf), "WAVE %d", game->wave_banner_number);
        float tw = (float)SDL_strlen(buf) * 8;
        SDL_RenderDebugText(r, (VIRTUAL_W/3.0f - tw)/2, (VIRTUAL_H/2.0f - 12)/3.0f, buf);
        SDL_SetRenderScale(r, 1, 1);
    }

    /* unit label while dragging */
    if (game->dragging_dice >= 0) {
        Dice *d = &game->dice[game->dragging_dice];
        const char *names[] = {"", "SPEAR", "ARCHER", "KNIGHT", "CAMEL", "SAGE", "ELEPHANT"};
        int face = d->face;
        if (face >= 1 && face <= 6) {
            SDL_SetRenderDrawColor(r, UNIT_COLORS[face].r, UNIT_COLORS[face].g, UNIT_COLORS[face].b, 255);
            SDL_SetRenderScale(r, 1.5f, 1.5f);
            float tw = (float)SDL_strlen(names[face]) * 8;
            SDL_RenderDebugText(r, d->x/1.5f - tw/2, (d->y - 40)/1.5f, names[face]);
            SDL_SetRenderScale(r, 1, 1);
        }
    }
}

static void render_game_over(Game *game) {
    SDL_Renderer *r = game->renderer;
    SDL_SetRenderDrawColor(r, 15, 10, 20, 255);
    SDL_RenderClear(r);

    SDL_SetRenderDrawColor(r, 255, 60, 60, 255);
    SDL_SetRenderScale(r, 4.0f, 4.0f);
    SDL_RenderDebugText(r, (VIRTUAL_W/4.0f - 9*8)/2, 40, "GAME OVER");
    SDL_SetRenderScale(r, 1, 1);

    char buf[64];
    SDL_SetRenderScale(r, 2.0f, 2.0f);
    SDL_SetRenderDrawColor(r, 255, 200, 50, 255);
    SDL_snprintf(buf, sizeof(buf), "Score: %d", game->score);
    SDL_RenderDebugText(r, (VIRTUAL_W/2.0f - (float)SDL_strlen(buf)*8)/2, 120, buf);
    SDL_snprintf(buf, sizeof(buf), "Wave: %d", game->wave);
    SDL_RenderDebugText(r, (VIRTUAL_W/2.0f - (float)SDL_strlen(buf)*8)/2, 145, buf);

    Uint64 t = SDL_GetTicks();
    float a = 0.5f + 0.5f * sinf((float)t * 0.003f);
    Uint8 v = (Uint8)(200*a);
    SDL_SetRenderDrawColor(r, v, v, v, 255);
    SDL_RenderDebugText(r, (VIRTUAL_W/2.0f - 14*8)/2, 200, "TAP TO RESTART");
    SDL_SetRenderScale(r, 1, 1);
}

void render_game(Game *game) {
    SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);
    switch (game->state) {
    case STATE_TITLE:     render_title(game);     break;
    case STATE_PLAYING:   render_playing(game);   break;
    case STATE_GAME_OVER: render_game_over(game); break;
    }
    SDL_RenderPresent(game->renderer);
}
