#include "game.h"
#include "sprites.h"
#include "ui.h"
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

static float depth_scale(float y) {
    float t = (y - FIELD_Y) / (float)FIELD_H;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return 0.82f + 0.18f * t;
}

static void draw_health_bar(SDL_Renderer *r, float x, float y, float w, float hp, float max_hp) {
    float pct = hp / max_hp;
    if (pct > 1.0f) pct = 1.0f;
    if (pct < 0.0f) pct = 0.0f;
    SDL_SetRenderDrawColor(r, 30, 30, 30, 200);
    SDL_FRect shadow = {x - w/2 + 1, y + 1, w, 5};
    SDL_RenderFillRect(r, &shadow);
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_FRect bg = {x - w/2, y, w, 5};
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 70, 70, 70, 255);
    SDL_FRect top_edge = {x - w/2, y, w, 1};
    SDL_RenderFillRect(r, &top_edge);
    Uint8 cr = (Uint8)(255 * (1.0f - pct));
    Uint8 cg = (Uint8)(255 * pct);
    SDL_SetRenderDrawColor(r, cr, cg, 0, 255);
    SDL_FRect fill = {x - w/2, y + 1, w * pct, 3};
    SDL_RenderFillRect(r, &fill);
    SDL_SetRenderDrawColor(r, (Uint8)(cr * 0.5f), (Uint8)(cg * 0.5f), 0, 255);
    SDL_FRect bot = {x - w/2, y + 4, w * pct, 1};
    SDL_RenderFillRect(r, &bot);
    Uint8 hr = (Uint8)(cr + (255 - cr) * 0.4f);
    Uint8 hg = (Uint8)(cg + (255 - cg) * 0.4f);
    SDL_SetRenderDrawColor(r, hr, hg, 0, 180);
    SDL_FRect highlight = {x - w/2, y + 1, w * pct, 1};
    SDL_RenderFillRect(r, &highlight);
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
    float bevel = 3.0f;

    SDL_SetRenderDrawColor(r, 0, 0, 0, 40);
    SDL_FRect drop_shadow = {d->x - half + 3, d->y - half + 4, size, size};
    SDL_RenderFillRect(r, &drop_shadow);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 20);
    SDL_FRect drop_shadow2 = {d->x - half + 5, d->y - half + 6, size, size};
    SDL_RenderFillRect(r, &drop_shadow2);

    if (d->state == DICE_COOLDOWN_STATE) {
        SDL_SetRenderDrawColor(r, 80, 72, 65, 255);
    } else {
        SDL_SetRenderDrawColor(r, 120, 100, 60, 255);
    }
    SDL_FRect bottom_face = {d->x - half, d->y - half + bevel, size, size};
    SDL_RenderFillRect(r, &bottom_face);

    Uint8 br, bg_c, bb;
    if (d->state == DICE_COOLDOWN_STATE) { br = 100; bg_c = 90; bb = 80; }
    else if (d->state == DICE_DRAGGING) { br = 255; bg_c = 250; bb = 235; }
    else { br = 245; bg_c = 240; bb = 220; }
    SDL_SetRenderDrawColor(r, br, bg_c, bb, 255);
    SDL_FRect body = {d->x - half, d->y - half, size, size};
    SDL_RenderFillRect(r, &body);

    Uint8 hr = (Uint8)(br + (255 - br) * 0.3f);
    Uint8 hg = (Uint8)(bg_c + (255 - bg_c) * 0.3f);
    Uint8 hb = (Uint8)(bb + (255 - bb) * 0.3f);
    SDL_SetRenderDrawColor(r, hr, hg, hb, 255);
    SDL_FRect top_edge = {d->x - half, d->y - half, size, bevel};
    SDL_RenderFillRect(r, &top_edge);
    SDL_FRect left_edge = {d->x - half, d->y - half, bevel, size};
    SDL_RenderFillRect(r, &left_edge);

    SDL_SetRenderDrawColor(r, (Uint8)(br * 0.7f), (Uint8)(bg_c * 0.7f), (Uint8)(bb * 0.7f), 255);
    SDL_FRect bot_edge = {d->x - half, d->y + half - bevel, size, bevel};
    SDL_RenderFillRect(r, &bot_edge);
    SDL_FRect right_edge = {d->x + half - bevel, d->y - half, bevel, size};
    SDL_RenderFillRect(r, &right_edge);

    SDL_SetRenderDrawColor(r, d->state == DICE_READY ? 180 : 70, d->state == DICE_READY ? 160 : 60, d->state == DICE_READY ? 80 : 50, 255);
    SDL_RenderRect(r, &body);

    if (d->state != DICE_COOLDOWN_STATE) {
        draw_die_dots(r, d->x, d->y, size, d->face);
    } else {
        float pct = d->cooldown_timer / DICE_COOLDOWN;
        SDL_SetRenderDrawColor(r, 60, 55, 45, 200);
        SDL_FRect cd_bg = {d->x - half + 5, d->y + half - 12, size - 10, 7};
        SDL_RenderFillRect(r, &cd_bg);
        SDL_SetRenderDrawColor(r, 180, 160, 120, 255);
        SDL_FRect cd = {d->x - half + 5, d->y + half - 12, (size - 10) * (1.0f - pct), 7};
        SDL_RenderFillRect(r, &cd);
        SDL_SetRenderDrawColor(r, 220, 200, 160, 180);
        SDL_FRect cd_hi = {d->x - half + 5, d->y + half - 12, (size - 10) * (1.0f - pct), 2};
        SDL_RenderFillRect(r, &cd_hi);
    }
}

/* ---------- sprite rendering ---------- */

static void draw_sprite(SDL_Renderer *r, SDL_Texture *atlas,
                        SDL_FRect src, float x, float y, float base_size,
                        AnimComp *anim, float extra_y_off, float ds) {
    float w = base_size * anim->scale_x * ds;
    float h = base_size * anim->scale_y * ds;
    float lean_off = anim->lean * ds;
    float dy = (base_size * ds - h) * 0.5f;
    SDL_FRect dst = {x - w/2 + lean_off, y - h/2 + dy + extra_y_off, w, h};

    SDL_SetTextureColorMod(atlas, 0, 0, 0);
    SDL_SetTextureAlphaMod(atlas, 50);
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
    SDL_FRect shadow_dst = {dst.x + 2 * ds, dst.y + 2 * ds, dst.w, dst.h};
    SDL_RenderTexture(r, atlas, &src, &shadow_dst);

    SDL_SetTextureColorMod(atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas, 255);
    SDL_RenderTexture(r, atlas, &src, &dst);

    Uint8 rim = (Uint8)(60 + 40 * ((y - FIELD_Y) / (float)FIELD_H));
    SDL_SetTextureColorMod(atlas, 255, 255, rim > 200 ? 200 : rim + 140);
    SDL_SetTextureAlphaMod(atlas, 35);
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_ADD);
    SDL_FRect highlight_dst = {dst.x - 1, dst.y - 1, dst.w, dst.h};
    SDL_RenderTexture(r, atlas, &src, &highlight_dst);

    if (anim->flash_active) {
        SDL_SetTextureColorMod(atlas, 255, 255, 255);
        SDL_SetTextureAlphaMod(atlas, 200);
        SDL_RenderTexture(r, atlas, &src, &dst);
    }

    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas, 255);
}

static void draw_ground_shadow(SDL_Renderer *r, float x, float y, float w, float ds) {
    float sw = w * ds;
    float hw = sw * 0.55f, hh = sw * 0.2f;
    for (int layer = 2; layer >= 0; layer--) {
        float mul = 1.0f + layer * 0.25f;
        Uint8 a = (Uint8)(40 - layer * 12);
        SDL_SetRenderDrawColor(r, 0, 0, 0, a);
        float lhw = hw * mul, lhh = hh * mul;
        for (float dy = -lhh; dy <= lhh; dy += 1.0f) {
            float span = lhw * sqrtf(1.0f - (dy * dy) / (lhh * lhh));
            SDL_RenderLine(r, x - span, y + dy, x + span, y + dy);
        }
    }
}

static void draw_unit_sprite(SDL_Renderer *r, SDL_Texture *atlas, Unit *u) {
    float ds = depth_scale(u->y);
    draw_ground_shadow(r, u->x, u->y + 14 * ds, 28, ds);
    SDL_FRect src = sprite_unit_src(u->type, u->anim.frame);
    draw_sprite(r, atlas, src, u->x, u->y, 36, &u->anim, 0, ds);
    if (u->hp < u->max_hp)
        draw_health_bar(r, u->x, u->y - 22 * ds, 30 * ds, u->hp, u->max_hp);
}

static void draw_enemy_sprite(SDL_Renderer *r, SDL_Texture *atlas, Enemy *e) {
    float ds = depth_scale(e->y);
    float base = (e->type == ENEMY_ASURA) ? 52.0f : 36.0f;
    draw_ground_shadow(r, e->x, e->y + 14 * ds, base * 0.8f, ds);
    SDL_FRect src = sprite_enemy_src(e->type, e->anim.frame);
    float walk_bob = sinf(e->walk_phase) * 2.0f * ds;
    draw_sprite(r, atlas, src, e->x, e->y, base, &e->anim, walk_bob, ds);
    float bar_off = (e->type == ENEMY_ASURA) ? 30.0f : 22.0f;
    draw_health_bar(r, e->x, e->y - bar_off * ds, 26 * ds, e->hp, e->max_hp);
}

static void draw_ornament_line(SDL_Renderer *r, float cx, float y, float half_w) {
    SDL_SetRenderDrawColor(r, 180, 140, 50, 180);
    SDL_RenderLine(r, cx - half_w, y, cx - 20, y);
    SDL_RenderLine(r, cx + 20, y, cx + half_w, y);
    draw_filled_circle(r, cx - half_w, y, 3);
    draw_filled_circle(r, cx + half_w, y, 3);
    draw_filled_circle(r, cx, y, 4);
}

/* ---------- screen renders ---------- */

static void render_splash(Game *game) {
    SDL_Renderer *r = game->renderer;
    SDL_SetRenderDrawColor(r, 10, 8, 18, 255);
    SDL_RenderClear(r);

    Uint8 alpha = (Uint8)(255 * game->splash_fade);

    SDL_SetRenderDrawColor(r, 180, 50, 50, (Uint8)(alpha * 0.15f));
    for (int i = 0; i < 3; i++) {
        float radius = 60.0f + i * 40.0f + sinf(game->splash_timer * 1.5f + i) * 10.0f;
        draw_filled_circle(r, VIRTUAL_W / 2.0f, 280, radius);
    }

    SDL_SetRenderDrawColor(r, 255, 200, 50, alpha);
    SDL_SetRenderScale(r, 5.0f, 5.0f);
    float tw1 = 7 * 8;
    SDL_RenderDebugText(r, (VIRTUAL_W / 5.0f - tw1) / 2, 52, "MAAYAVI");
    SDL_SetRenderScale(r, 1, 1);

    draw_ornament_line(r, VIRTUAL_W / 2.0f, 330, 120);

    SDL_SetRenderDrawColor(r, 200, 180, 140, (Uint8)(alpha * 0.8f));
    SDL_SetRenderScale(r, 2.0f, 2.0f);
    float tw3 = 17 * 8;
    SDL_RenderDebugText(r, (VIRTUAL_W / 2.0f - tw3) / 2, 200, "Dice Lane Defense");
    SDL_SetRenderScale(r, 1, 1);

    float die_x = VIRTUAL_W / 2.0f, die_y = 480;
    int die_face = ((int)(game->splash_timer * 3.0f) % 6) + 1;
    float die_rot = sinf(game->splash_timer * 2.0f) * 0.05f;
    float die_sz = 60 + die_rot * 100;
    SDL_SetRenderDrawColor(r, 245, 240, 220, alpha);
    SDL_FRect die = {die_x - die_sz/2, die_y - die_sz/2, die_sz, die_sz};
    SDL_RenderFillRect(r, &die);
    SDL_SetRenderDrawColor(r, 200, 180, 100, alpha);
    SDL_RenderRect(r, &die);
    if (alpha > 128) draw_die_dots(r, die_x, die_y, die_sz, die_face);

    if (game->splash_timer > 2.0f) {
        Uint64 t = SDL_GetTicks();
        float pulse = 0.5f + 0.5f * sinf((float)t * 0.004f);
        Uint8 v = (Uint8)(180 * pulse * game->splash_fade);
        SDL_SetRenderDrawColor(r, v, v, v, 255);
        SDL_SetRenderScale(r, 1.5f, 1.5f);
        float tw4 = 16 * 8;
        SDL_RenderDebugText(r, (VIRTUAL_W / 1.5f - tw4) / 2, 410, "TAP TO CONTINUE");
        SDL_SetRenderScale(r, 1, 1);
    }
}

static void render_playing(Game *game) {
    SDL_Renderer *r = game->renderer;
    float sx = game->shake_x, sy = game->shake_y;

    SDL_SetRenderDrawColor(r, 25, 28, 20, 255);
    SDL_RenderClear(r);

    /* lanes with depth gradient */
    for (int i = 0; i < NUM_LANES; i++) {
        int band_h = 8;
        for (int row = 0; row < FIELD_H; row += band_h) {
            float t = (float)row / (float)FIELD_H;
            int base_r = (i % 2 == 0) ? 32 : 38;
            int base_g = (i % 2 == 0) ? 48 : 54;
            int base_b = (i % 2 == 0) ? 28 : 32;
            Uint8 lr = (Uint8)(base_r + (int)(t * 14));
            Uint8 lg = (Uint8)(base_g + (int)(t * 18));
            Uint8 lb = (Uint8)(base_b + (int)(t * 10));
            SDL_SetRenderDrawColor(r, lr, lg, lb, 255);
            float actual_h = (float)band_h;
            if (row + band_h > FIELD_H) actual_h = (float)(FIELD_H - row);
            SDL_FRect band = {i * LANE_W + sx, FIELD_Y + row + sy, (float)LANE_W, actual_h};
            SDL_RenderFillRect(r, &band);
        }
    }

    /* lane highlight */
    if (game->dragging_dice >= 0 && game->drag_y >= FIELD_Y && game->drag_y <= FIELD_Y + FIELD_H) {
        int hl = (int)(game->drag_x / LANE_W);
        if (hl >= 0 && hl < NUM_LANES) {
            SDL_SetRenderDrawColor(r, 255, 255, 200, 20);
            SDL_FRect h = {hl * LANE_W + sx, FIELD_Y + sy, (float)LANE_W, (float)FIELD_H};
            SDL_RenderFillRect(r, &h);
            SDL_SetRenderDrawColor(r, 255, 220, 100, 40);
            SDL_FRect h2 = {hl * LANE_W + sx + 1, FIELD_Y + sy, (float)LANE_W - 2, (float)FIELD_H};
            SDL_RenderRect(r, &h2);
        }
    }

    /* lane dividers with groove effect */
    for (int i = 1; i < NUM_LANES; i++) {
        float lx = i * LANE_W + sx;
        SDL_SetRenderDrawColor(r, 20, 25, 15, 255);
        SDL_RenderLine(r, lx - 1, FIELD_Y + sy, lx - 1, FIELD_Y + FIELD_H + sy);
        SDL_SetRenderDrawColor(r, 55, 65, 45, 255);
        SDL_RenderLine(r, lx, FIELD_Y + sy, lx, FIELD_Y + FIELD_H + sy);
        SDL_SetRenderDrawColor(r, 70, 85, 55, 200);
        SDL_RenderLine(r, lx + 1, FIELD_Y + sy, lx + 1, FIELD_Y + FIELD_H + sy);
    }

    /* field border with depth */
    SDL_SetRenderDrawColor(r, 90, 100, 65, 255);
    SDL_FRect fb = {sx, FIELD_Y + sy, (float)VIRTUAL_W, (float)FIELD_H};
    SDL_RenderRect(r, &fb);
    SDL_SetRenderDrawColor(r, 60, 70, 45, 255);
    SDL_FRect fb2 = {sx + 1, FIELD_Y + sy + 1, (float)VIRTUAL_W - 2, (float)FIELD_H - 2};
    SDL_RenderRect(r, &fb2);

    /* depth fog at top of field */
    for (int row = 0; row < 40; row++) {
        Uint8 a = (Uint8)(35 - row * 35 / 40);
        SDL_SetRenderDrawColor(r, 15, 12, 25, a);
        SDL_FRect fog = {sx, FIELD_Y + row + sy, (float)VIRTUAL_W, 1};
        SDL_RenderFillRect(r, &fog);
    }

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
        float ds = depth_scale(p->y);
        float px = p->x + sx, py = p->y + sy;
        /* outer glow */
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
        SDL_SetRenderDrawColor(r, PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 30);
        draw_filled_circle(r, px, py, 10 * ds);
        SDL_SetRenderDrawColor(r, PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 60);
        draw_filled_circle(r, px, py, 6 * ds);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        /* core */
        SDL_SetRenderDrawColor(r, PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 255);
        draw_filled_circle(r, px, py, 4 * ds);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
        draw_filled_circle(r, px - 1, py - 1, 2 * ds);
        /* trail */
        SDL_SetRenderDrawColor(r, PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 120);
        draw_filled_circle(r, px, py + 5 * ds, 3 * ds);
        SDL_SetRenderDrawColor(r, PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 60);
        draw_filled_circle(r, px, py + 10 * ds, 2 * ds);
        draw_filled_circle(r, px, py + 14 * ds, 1.5f * ds);
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

    /* tray with depth gradient */
    for (int row = 0; row < TRAY_H; row++) {
        float t = (float)row / (float)TRAY_H;
        Uint8 tr = (Uint8)(55 - (int)(t * 18));
        Uint8 tg = (Uint8)(38 - (int)(t * 12));
        Uint8 tb = (Uint8)(28 - (int)(t * 10));
        SDL_SetRenderDrawColor(r, tr, tg, tb, 255);
        SDL_FRect tray_line = {0, (float)(TRAY_Y + row), (float)VIRTUAL_W, 1};
        SDL_RenderFillRect(r, &tray_line);
    }
    SDL_SetRenderDrawColor(r, 100, 80, 55, 255);
    SDL_RenderLine(r, 0, (float)TRAY_Y, (float)VIRTUAL_W, (float)TRAY_Y);
    SDL_SetRenderDrawColor(r, 80, 60, 40, 255);
    SDL_RenderLine(r, 0, TRAY_Y + 1.0f, (float)VIRTUAL_W, TRAY_Y + 1.0f);
    SDL_SetRenderDrawColor(r, 35, 25, 18, 255);
    SDL_RenderLine(r, 0, TRAY_Y + 2.0f, (float)VIRTUAL_W, TRAY_Y + 2.0f);

    /* dice */
    for (int i = 0; i < MAX_DICE; i++) {
        if (i == game->dragging_dice) continue;
        draw_die(r, &game->dice[i]);
    }
    if (game->dragging_dice >= 0)
        draw_die(r, &game->dice[game->dragging_dice]);

    /* HUD with gradient */
    for (int row = 0; row < HUD_H; row++) {
        float t = (float)row / (float)HUD_H;
        Uint8 hr = (Uint8)(25 - (int)(t * 8));
        Uint8 hg = (Uint8)(18 - (int)(t * 6));
        Uint8 hb = (Uint8)(38 - (int)(t * 12));
        SDL_SetRenderDrawColor(r, hr, hg, hb, 230);
        SDL_FRect hud_line = {0, (float)row, (float)VIRTUAL_W, 1};
        SDL_RenderFillRect(r, &hud_line);
    }
    SDL_SetRenderDrawColor(r, 80, 65, 45, 200);
    SDL_RenderLine(r, 0, (float)HUD_H - 1, (float)VIRTUAL_W, (float)HUD_H - 1);
    SDL_SetRenderDrawColor(r, 40, 30, 20, 200);
    SDL_RenderLine(r, 0, (float)HUD_H, (float)VIRTUAL_W, (float)HUD_H);

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

void render_game(Game *game) {
    SDL_SetRenderDrawBlendMode(game->renderer, SDL_BLENDMODE_BLEND);
    switch (game->state) {
    case STATE_SPLASH:
        render_splash(game);
        break;
    case STATE_MENU:
        SDL_SetRenderDrawColor(game->renderer, 18, 14, 28, 255);
        SDL_RenderClear(game->renderer);
        ui_render_menu(game);
        break;
    case STATE_SETTINGS:
        SDL_SetRenderDrawColor(game->renderer, 18, 14, 28, 255);
        SDL_RenderClear(game->renderer);
        ui_render_settings(game);
        break;
    case STATE_PLAYING:
        render_playing(game);
        break;
    case STATE_GAME_OVER:
        SDL_SetRenderDrawColor(game->renderer, 15, 10, 20, 255);
        SDL_RenderClear(game->renderer);
        ui_render_game_over(game);
        break;
    }
    SDL_RenderPresent(game->renderer);
}
