#include "game.h"
#include "sprites.h"
#include "ui.h"
#include <stdio.h>

static const struct { Uint8 r, g, b; } UNIT_COLORS[] = {
    [UNIT_NONE]     = {117, 113, 143},
    [UNIT_SPEARMAN] = {94,  118, 166},
    [UNIT_ARCHER]   = {96,  154, 122},
    [UNIT_KNIGHT]   = {118, 154, 178},
    [UNIT_CAMEL]    = {151, 101, 141},
    [UNIT_SAGE]     = {120, 239, 229},
    [UNIT_ELEPHANT] = {117, 101, 157},
};

static const struct { Uint8 r, g, b; } PROJ_COLORS[] = {
    [UNIT_NONE]     = {229, 174, 58},
    [UNIT_SPEARMAN] = {229, 174, 58},
    [UNIT_ARCHER]   = {220, 232, 154},
    [UNIT_KNIGHT]   = {201, 238, 218},
    [UNIT_CAMEL]    = {204, 73, 118},
    [UNIT_SAGE]     = {120, 239, 229},
    [UNIT_ELEPHANT] = {241, 100, 112},
};

static Uint8 clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (Uint8)v;
}

static int pixel_noise(int x, int y, int salt) {
    unsigned n = (unsigned)(x * 1103515245u + y * 12345u + salt * 2654435761u);
    n ^= n >> 13;
    n *= 1274126177u;
    return (int)((n ^ (n >> 16)) & 31) - 15;
}

static void draw_filled_circle(SDL_Renderer *r, float cx, float cy, float radius) {
    for (float y = -radius; y <= radius; y += 1.0f) {
        float hw = sqrtf(radius * radius - y * y);
        SDL_RenderLine(r, cx - hw, cy + y, cx + hw, cy + y);
    }
}

static void draw_pixel_speckles(SDL_Renderer *r, int x, int y, int w, int h,
                                Uint8 cr, Uint8 cg, Uint8 cb, Uint8 alpha, int salt) {
    for (int yy = y; yy < y + h; yy += 3) {
        for (int xx = x; xx < x + w; xx += 3) {
            if ((pixel_noise(xx, yy, salt) & 7) == 0) {
                SDL_SetRenderDrawColor(r, cr, cg, cb, alpha);
                SDL_FRect dot = {(float)xx, (float)yy, 1.0f, 1.0f};
                SDL_RenderFillRect(r, &dot);
            }
        }
    }
}

static void draw_pixel_rect(SDL_Renderer *r, float x, float y, float w, float h,
                            Uint8 cr, Uint8 cg, Uint8 cb, Uint8 alpha) {
    SDL_SetRenderDrawColor(r, cr, cg, cb, alpha);
    SDL_FRect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}

static void draw_triangle(SDL_Renderer *r, float ax, float ay, float bx, float by, float cx, float cy) {
    float min_y = fminf(ay, fminf(by, cy));
    float max_y = fmaxf(ay, fmaxf(by, cy));
    for (float y = min_y; y <= max_y; y += 1.0f) {
        float xs[3];
        int count = 0;
        float px[3] = {ax, bx, cx};
        float py[3] = {ay, by, cy};
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            if ((py[i] <= y && py[j] > y) || (py[j] <= y && py[i] > y)) {
                float t = (y - py[i]) / (py[j] - py[i]);
                xs[count++] = px[i] + t * (px[j] - px[i]);
            }
        }
        if (count == 2) {
            if (xs[0] > xs[1]) { float tmp = xs[0]; xs[0] = xs[1]; xs[1] = tmp; }
            SDL_RenderLine(r, xs[0], y, xs[1], y);
        }
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
    SDL_SetRenderDrawColor(r, 31, 24, 47, 255);
    SDL_FRect shadow = {x - w/2 + 1, y + 1, w, 5};
    SDL_RenderFillRect(r, &shadow);
    SDL_SetRenderDrawColor(r, 42, 48, 74, 255);
    SDL_FRect bg = {x - w/2, y, w, 5};
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawColor(r, 117, 113, 143, 255);
    SDL_FRect top_edge = {x - w/2, y, w, 1};
    SDL_RenderFillRect(r, &top_edge);
    Uint8 cr = pct > 0.4f ? 229 : 204;
    Uint8 cg = pct > 0.4f ? 174 : 73;
    Uint8 cb = pct > 0.4f ? 58 : 118;
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    SDL_FRect fill = {x - w/2, y + 1, w * pct, 3};
    SDL_RenderFillRect(r, &fill);
    SDL_SetRenderDrawColor(r, 31, 24, 47, 255);
    SDL_FRect bot = {x - w/2, y + 4, w * pct, 1};
    SDL_RenderFillRect(r, &bot);
    SDL_SetRenderDrawColor(r, 241, 232, 154, 255);
    SDL_FRect highlight = {x - w/2, y + 1, w * pct, 1};
    SDL_RenderFillRect(r, &highlight);
}

static void draw_die_dots(SDL_Renderer *r, float cx, float cy, float size, int face) {
    SDL_SetRenderDrawColor(r, 31, 24, 47, 255);
    float dr = size * 0.1f, off = size * 0.25f;
    if (face == 1 || face == 3 || face == 5) draw_filled_circle(r, cx, cy, dr);
    if (face >= 2) { draw_filled_circle(r, cx+off, cy-off, dr); draw_filled_circle(r, cx-off, cy+off, dr); }
    if (face >= 4) { draw_filled_circle(r, cx-off, cy-off, dr); draw_filled_circle(r, cx+off, cy+off, dr); }
    if (face == 6) { draw_filled_circle(r, cx-off, cy, dr); draw_filled_circle(r, cx+off, cy, dr); }
}

static void draw_die(SDL_Renderer *r, Dice *d) {
    float size = 52, half = 26;
    float ox = 0, oy = 0, grow = 0;

    if (d->state == DICE_ROLLING) {
        /* tumble jitter that eases out as it settles */
        float k = 1.0f - d->roll_time / DICE_ROLL_TIME;
        if (k < 0) k = 0;
        float ph = d->anim_t * 40.0f;
        ox = sinf(ph) * 5.0f * k;
        oy = cosf(ph * 1.7f) * 4.0f * k;
        grow = sinf(d->anim_t * 30.0f) * 3.0f * k;
    }

    float cx = d->x + ox, cy = d->y + oy;
    half += grow * 0.5f; size += grow;

    SDL_SetRenderDrawColor(r, 20, 15, 32, 255);
    SDL_FRect shadow = {cx - half + 5, cy - half + 7, size, size};
    SDL_RenderFillRect(r, &shadow);

    Uint8 br = 226, bg_c = 232, bb = 214;
    if (d->state == DICE_COOLDOWN_STATE) { br = 92; bg_c = 103; bb = 132; }
    else if (d->state == DICE_DRAGGING) { br = 70; bg_c = 79; bb = 120; }
    else if (d->state == DICE_ROLLING) { br = 240; bg_c = 248; bb = 226; }

    /* glow for a settled, draggable die */
    if (d->state == DICE_ROLLED) {
        float pulse = 0.5f + 0.5f * sinf(d->anim_t * 5.0f);
        SDL_SetRenderDrawColor(r, 120, 239, 229, (Uint8)(70 + 90 * pulse));
        SDL_FRect g = {cx - half - 5, cy - half - 5, size + 10, size + 10};
        SDL_RenderFillRect(r, &g);
    }

    SDL_SetRenderDrawColor(r, 31, 24, 47, 255);
    SDL_FRect outline = {cx - half - 3, cy - half - 3, size + 6, size + 6};
    SDL_RenderFillRect(r, &outline);
    SDL_SetRenderDrawColor(r, br, bg_c, bb, 255);
    SDL_FRect body = {cx - half, cy - half, size, size};
    SDL_RenderFillRect(r, &body);
    SDL_SetRenderDrawColor(r, 70, 79, 120, 255);
    SDL_RenderRect(r, &body);
    draw_pixel_speckles(r, (int)body.x + 4, (int)body.y + 4, (int)body.w - 8, (int)body.h - 8, 31, 24, 47, 38, d->face * 17);

    if (d->state == DICE_READY) {
        /* "tap to roll": pulsing ring + neutral mark, face hidden */
        float pulse = 0.5f + 0.5f * sinf(d->anim_t * 4.0f);
        SDL_SetRenderDrawColor(r, 120, 239, 229, (Uint8)(120 + 110 * pulse));
        SDL_FRect ring = {cx - half + 4, cy - half + 4, size - 8, size - 8};
        SDL_RenderRect(r, &ring);
        SDL_SetRenderDrawColor(r, 64, 98, 135, 255);
        draw_filled_circle(r, cx, cy, 4 + 2 * pulse);
    } else if (d->state == DICE_COOLDOWN_STATE) {
        float pct = d->cooldown_timer / DICE_COOLDOWN;
        SDL_SetRenderDrawColor(r, 31, 24, 47, 255);
        SDL_FRect cd_bg = {cx - half + 5, cy + half - 12, size - 10, 7};
        SDL_RenderFillRect(r, &cd_bg);
        SDL_SetRenderDrawColor(r, 120, 239, 229, 255);
        SDL_FRect cd = {cx - half + 5, cy + half - 12, (size - 10) * (1.0f - pct), 7};
        SDL_RenderFillRect(r, &cd);
        SDL_SetRenderDrawColor(r, 226, 232, 214, 255);
        SDL_FRect cd_hi = {cx - half + 5, cy + half - 12, (size - 10) * (1.0f - pct), 2};
        SDL_RenderFillRect(r, &cd_hi);
    } else {
        draw_die_dots(r, cx, cy, size, d->face);
    }
}

/* translucent "light" character that has been rolled but not yet placed */
static void draw_unit_token(SDL_Renderer *r, SDL_Texture *atlas, int type,
                            float cx, float cy, float scale, Uint8 alpha, float t) {
    if (type < 1 || type > 6) return;
    SDL_FRect src = sprite_unit_src(type, ((int)(t * 6.0f)) & 1);
    float s = 30.0f * scale;

    float pulse = 0.5f + 0.5f * sinf(t * 4.0f);
    SDL_SetRenderDrawColor(r, 120, 239, 229, (Uint8)(40 + 50 * pulse));
    draw_filled_circle(r, cx, cy + s * 0.15f, s * 0.62f);
    SDL_SetRenderDrawColor(r, 120, 239, 229, (Uint8)(70 + 60 * pulse));
    draw_filled_circle(r, cx, cy + s * 0.5f, s * 0.42f);

    SDL_FRect dst = {cx - s/2, cy - s/2, s, s};
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(atlas, 120, 239, 229);
    SDL_SetTextureAlphaMod(atlas, (Uint8)(alpha * 0.5f));
    SDL_FRect aura = {dst.x - 2, dst.y - 2, dst.w + 4, dst.h + 4};
    SDL_RenderTexture(r, atlas, &src, &aura);
    SDL_SetTextureColorMod(atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas, alpha);
    SDL_RenderTexture(r, atlas, &src, &dst);
    SDL_SetTextureAlphaMod(atlas, 255);
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

    SDL_SetTextureColorMod(atlas, 20, 15, 32);
    SDL_SetTextureAlphaMod(atlas, 145);
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
    SDL_FRect shadow_dst = {floorf(dst.x + 3 * ds), floorf(dst.y + 4 * ds), floorf(dst.w), floorf(dst.h)};
    SDL_RenderTexture(r, atlas, &src, &shadow_dst);

    SDL_SetTextureColorMod(atlas, 255, 255, 255);
    SDL_SetTextureAlphaMod(atlas, 255);
    dst.x = floorf(dst.x);
    dst.y = floorf(dst.y);
    dst.w = floorf(dst.w);
    dst.h = floorf(dst.h);
    SDL_RenderTexture(r, atlas, &src, &dst);

    SDL_SetTextureColorMod(atlas, 120, 239, 229);
    SDL_SetTextureAlphaMod(atlas, 28);
    SDL_SetTextureBlendMode(atlas, SDL_BLENDMODE_BLEND);
    SDL_FRect highlight_dst = {dst.x - 1, dst.y - 1, dst.w, dst.h};
    SDL_RenderTexture(r, atlas, &src, &highlight_dst);

    if (anim->flash_active) {
        SDL_SetTextureColorMod(atlas, 255, 255, 255);
        SDL_SetTextureAlphaMod(atlas, 220);
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
        Uint8 a = (Uint8)(135 - layer * 35);
        SDL_SetRenderDrawColor(r, 20, 15, 32, a);
        float lhw = hw * mul, lhh = hh * mul;
        for (float dy = -lhh; dy <= lhh; dy += 1.0f) {
            float span = lhw * sqrtf(1.0f - (dy * dy) / (lhh * lhh));
            SDL_RenderLine(r, x - span, y + dy, x + span, y + dy);
        }
    }
}

static void draw_unit_sprite(SDL_Renderer *r, SDL_Texture *atlas, Unit *u) {
    float ds = depth_scale(u->y);
    draw_ground_shadow(r, u->x, u->y + 18 * ds, 32, ds);
    SDL_FRect src = sprite_unit_src(u->type, u->anim.frame);
    draw_sprite(r, atlas, src, u->x, u->y, 48, &u->anim, 0, ds);
    if (u->hp < u->max_hp)
        draw_health_bar(r, u->x, u->y - 28 * ds, 32 * ds, u->hp, u->max_hp);
}

static void draw_enemy_sprite(SDL_Renderer *r, SDL_Texture *atlas, Enemy *e) {
    float ds = depth_scale(e->y);
    float base = (e->type == ENEMY_ASURA) ? 68.0f : 46.0f;
    draw_ground_shadow(r, e->x, e->y + 18 * ds, base * 0.7f, ds);
    SDL_FRect src = sprite_enemy_src(e->type, e->anim.frame);
    float walk_bob = sinf(e->walk_phase) * 2.0f * ds;
    draw_sprite(r, atlas, src, e->x, e->y, base, &e->anim, walk_bob, ds);
    float bar_off = (e->type == ENEMY_ASURA) ? 38.0f : 28.0f;
    draw_health_bar(r, e->x, e->y - bar_off * ds, 28 * ds, e->hp, e->max_hp);
}

static void draw_ornament_line(SDL_Renderer *r, float cx, float y, float half_w) {
    SDL_SetRenderDrawColor(r, 120, 239, 229, 255);
    SDL_RenderLine(r, cx - half_w, y, cx - 20, y);
    SDL_RenderLine(r, cx + 20, y, cx + half_w, y);
    draw_filled_circle(r, cx - half_w, y, 3);
    draw_filled_circle(r, cx + half_w, y, 3);
    draw_filled_circle(r, cx, y, 4);
}

static void draw_shikhara(SDL_Renderer *r, float cx, float base_y, float w, float h) {
    for (float y = 0; y < h; y += 1.0f) {
        float t = y / h;
        float curve = 1.0f - t * t;
        float hw = (w * 0.5f) * (0.3f + 0.7f * curve);
        SDL_RenderLine(r, cx - hw, base_y - y, cx + hw, base_y - y);
    }
    draw_filled_circle(r, cx, base_y - h, w * 0.18f);
    draw_pixel_rect(r, cx - 2, base_y - h - w * 0.25f, 4, w * 0.15f, 229, 174, 58, 180);
}

static void draw_temple_backdrop(SDL_Renderer *r) {
    /* sky: deep indigo-purple gradient */
    draw_pixel_rect(r, 0, 0, VIRTUAL_W, VIRTUAL_H, 31, 24, 47, 255);
    draw_pixel_rect(r, 0, 0, VIRTUAL_W, 98, 42, 28, 56, 255);
    draw_pixel_speckles(r, 0, 0, VIRTUAL_W, 98, 229, 174, 58, 30, 29);

    /* crescent moon */
    SDL_SetRenderDrawColor(r, 226, 232, 214, 200);
    draw_filled_circle(r, VIRTUAL_W / 2.0f + 44, 40, 22);
    SDL_SetRenderDrawColor(r, 42, 28, 56, 255);
    draw_filled_circle(r, VIRTUAL_W / 2.0f + 52, 34, 20);

    /* shikhara temple silhouettes */
    SDL_SetRenderDrawColor(r, 21, 16, 42, 255);
    draw_shikhara(r, 50, 105, 48, 58);
    draw_shikhara(r, 200, 105, 64, 78);
    draw_shikhara(r, 340, 105, 44, 50);

    /* gopuram gateway (stepped pyramid) at centre */
    SDL_SetRenderDrawColor(r, 26, 22, 42, 255);
    for (int step = 0; step < 5; step++) {
        float sw = 70 - step * 10;
        float sy = 105 - step * 12;
        draw_pixel_rect(r, 200 - sw / 2, sy - 6, (int)sw, 8, 26, 22, 42, 255);
    }
    draw_pixel_rect(r, 196, 46, 8, 6, 229, 174, 58, 140);

    /* base wall: temple plinth with lotus crenellations */
    draw_pixel_rect(r, 0, FIELD_Y + FIELD_H - 8, VIRTUAL_W, 58, 31, 24, 47, 255);
    for (int x = 0; x < VIRTUAL_W; x += 36) {
        float cx = x + 18.0f;
        float cy = (float)(FIELD_Y + FIELD_H - 14);
        SDL_SetRenderDrawColor(r, 229, 174, 58, 100);
        draw_filled_circle(r, cx, cy, 6);
        SDL_SetRenderDrawColor(r, 31, 24, 47, 255);
        draw_filled_circle(r, cx, cy, 3);
        draw_pixel_rect(r, x + 6, FIELD_Y + FIELD_H - 6, 24, 4, 64, 98, 135, 200);
    }

    /* left pillar: lotus-column style */
    draw_pixel_rect(r, -8, 70, 36, FIELD_H + 18, 34, 28, 50, 255);
    draw_pixel_rect(r, 8, 88, 10, FIELD_H - 22, 64, 98, 135, 180);
    for (int y = 80; y < FIELD_Y + FIELD_H; y += 28) {
        SDL_SetRenderDrawColor(r, 229, 174, 58, 80);
        draw_filled_circle(r, 14, (float)y, 5);
        SDL_SetRenderDrawColor(r, 34, 28, 50, 255);
        draw_filled_circle(r, 14, (float)y, 2);
    }

    /* right pillar */
    draw_pixel_rect(r, VIRTUAL_W - 26, 100, 52, FIELD_H - 32, 34, 28, 50, 255);
    draw_pixel_rect(r, VIRTUAL_W - 18, 110, 10, FIELD_H - 50, 64, 98, 135, 180);
    for (int y = 108; y < FIELD_Y + FIELD_H; y += 28) {
        SDL_SetRenderDrawColor(r, 229, 174, 58, 80);
        draw_filled_circle(r, VIRTUAL_W - 12.0f, (float)y, 5);
        SDL_SetRenderDrawColor(r, 34, 28, 50, 255);
        draw_filled_circle(r, VIRTUAL_W - 12.0f, (float)y, 2);
    }

    /* warm speckles on pillars (saffron/kumkum stains) */
    draw_pixel_speckles(r, 0, 140, 30, FIELD_H - 70, 229, 174, 58, 50, 91);
    draw_pixel_speckles(r, VIRTUAL_W - 28, 140, 28, FIELD_H - 70, 229, 174, 58, 45, 93);
}

/* ---------- screen renders ---------- */

static void render_splash(Game *game) {
    SDL_Renderer *r = game->renderer;
    draw_temple_backdrop(r);
    Uint8 alpha = (Uint8)(255 * game->splash_fade);

    SDL_SetRenderDrawColor(r, 120, 239, 229, (Uint8)(alpha * 0.2f));
    draw_filled_circle(r, VIRTUAL_W / 2.0f, 270, 94 + sinf(game->splash_timer) * 4);
    SDL_SetRenderDrawColor(r, 226, 232, 214, alpha);
    SDL_SetRenderScale(r, 5.0f, 5.0f);
    float tw1 = 7 * 8;
    SDL_RenderDebugText(r, (VIRTUAL_W / 5.0f - tw1) / 2, 52, "MAAYAVI");
    SDL_SetRenderScale(r, 1, 1);

    draw_ornament_line(r, VIRTUAL_W / 2.0f, 330, 120);

    SDL_SetRenderDrawColor(r, 120, 239, 229, (Uint8)(alpha * 0.9f));
    SDL_SetRenderScale(r, 2.0f, 2.0f);
    float tw3 = 17 * 8;
    SDL_RenderDebugText(r, (VIRTUAL_W / 2.0f - tw3) / 2, 200, "Dice Lane Defense");
    SDL_SetRenderScale(r, 1, 1);

    float die_x = VIRTUAL_W / 2.0f, die_y = 480;
    int die_face = ((int)(game->splash_timer * 3.0f) % 6) + 1;
    float die_rot = sinf(game->splash_timer * 2.0f) * 0.05f;
    float die_sz = 60 + die_rot * 100;
    SDL_SetRenderDrawColor(r, 226, 232, 214, alpha);
    SDL_FRect die = {die_x - die_sz/2, die_y - die_sz/2, die_sz, die_sz};
    SDL_RenderFillRect(r, &die);
    SDL_SetRenderDrawColor(r, 31, 24, 47, alpha);
    SDL_RenderRect(r, &die);
    if (alpha > 128) draw_die_dots(r, die_x, die_y, die_sz, die_face);

    if (game->splash_timer > 2.0f) {
        Uint64 t = SDL_GetTicks();
        float pulse = 0.5f + 0.5f * sinf((float)t * 0.004f);
        Uint8 v = (Uint8)(180 * pulse * game->splash_fade);
        SDL_SetRenderDrawColor(r, clamp_u8(v + 60), clamp_u8(v + 70), clamp_u8(v + 30), 255);
        SDL_SetRenderScale(r, 1.5f, 1.5f);
        float tw4 = 16 * 8;
        SDL_RenderDebugText(r, (VIRTUAL_W / 1.5f - tw4) / 2, 410, "TAP TO CONTINUE");
        SDL_SetRenderScale(r, 1, 1);
    }
}

static void render_playing(Game *game) {
    SDL_Renderer *r = game->renderer;
    float sx = game->shake_x, sy = game->shake_y;

    draw_temple_backdrop(r);

    /* checkerboard graveyard lanes */
    draw_pixel_rect(r, sx, FIELD_Y + sy, VIRTUAL_W, FIELD_H, 6, 116, 83, 255);
    for (int i = 0; i < NUM_LANES; i++) {
        for (int row = 0; row < 5; row++) {
            float pad = 8.0f;
            float cell_x = i * LANE_W + pad + sx;
            float cell_y = FIELD_Y + 17.0f + row * 82.0f + sy;
            Uint8 shade = ((i + row) & 1) ? 78 : 86;
            draw_pixel_rect(r, cell_x, cell_y, LANE_W - pad * 2, 58, shade, 137, 108, 220);
            draw_pixel_rect(r, cell_x, cell_y, LANE_W - pad * 2, 3, 105, 164, 132, 230);
            draw_pixel_speckles(r, (int)cell_x + 2, (int)cell_y + 4, (int)(LANE_W - pad * 2 - 4), 50, 133, 186, 144, 40, i * 13 + row * 31);
        }
    }
    draw_pixel_speckles(r, 34, FIELD_Y + 12, VIRTUAL_W - 68, FIELD_H - 34, 113, 174, 139, 88, 37);

    /* lane highlight */
    if (game->dragging_dice >= 0 && game->drag_y >= FIELD_Y && game->drag_y <= FIELD_Y + FIELD_H) {
        int hl = (int)(game->drag_x / LANE_W);
        if (hl >= 0 && hl < NUM_LANES) {
            draw_pixel_rect(r, hl * LANE_W + sx + 3, FIELD_Y + sy, LANE_W - 6, FIELD_H, 120, 239, 229, 38);
            SDL_SetRenderDrawColor(r, 120, 239, 229, 210);
            SDL_FRect h2 = {hl * LANE_W + sx + 4, FIELD_Y + sy + 4, (float)LANE_W - 8, (float)FIELD_H - 8};
            SDL_RenderRect(r, &h2);
        }
    }

    /* lane dividers */
    for (int i = 1; i < NUM_LANES; i++) {
        float lx = i * LANE_W + sx;
        SDL_SetRenderDrawColor(r, 0, 80, 65, 190);
        SDL_RenderLine(r, lx - 1, FIELD_Y + sy, lx - 1, FIELD_Y + FIELD_H + sy);
        SDL_SetRenderDrawColor(r, 34, 141, 101, 140);
        SDL_RenderLine(r, lx, FIELD_Y + sy, lx, FIELD_Y + FIELD_H + sy);
    }

    SDL_SetRenderDrawColor(r, 31, 24, 47, 255);
    SDL_FRect fb = {sx, FIELD_Y + sy, (float)VIRTUAL_W, (float)FIELD_H};
    SDL_RenderRect(r, &fb);
    SDL_SetRenderDrawColor(r, 64, 98, 135, 255);
    SDL_FRect fb2 = {sx + 1, FIELD_Y + sy + 1, (float)VIRTUAL_W - 2, (float)FIELD_H - 2};
    SDL_RenderRect(r, &fb2);

    /* blood decals */
    for (int i = 0; i < MAX_DECALS; i++) {
        Decal *d = &game->decals[i];
        if (d->a == 0) continue;
        SDL_SetRenderDrawColor(r, 204, 0, 83, d->a);
        draw_filled_circle(r, d->x + sx, d->y + sy, d->size);
        SDL_SetRenderDrawColor(r, 99, 31, 69, d->a);
        draw_filled_circle(r, d->x + sx + 1, d->y + sy + 1, d->size * 0.45f);
    }

    /* projectiles */
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) continue;
        int st = p->source_type;
        if (st < 0 || st > 6) st = 0;
        float ds = depth_scale(p->y);
        float px = p->x + sx, py = p->y + sy;
        draw_pixel_rect(r, floorf(px - 3 * ds), floorf(py - 3 * ds), 7 * ds, 7 * ds, 31, 24, 47, 255);
        draw_pixel_rect(r, floorf(px - 2 * ds), floorf(py - 2 * ds), 5 * ds, 5 * ds,
                        PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 255);
        draw_pixel_rect(r, floorf(px - 1 * ds), floorf(py - 8 * ds), 2 * ds, 4 * ds,
                        PROJ_COLORS[st].r, PROJ_COLORS[st].g, PROJ_COLORS[st].b, 150);
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
            SDL_SetRenderDrawColor(r, p->r, p->g, p->b, (Uint8)(220*alpha));
            draw_pixel_rect(r, floorf(px), floorf(py), floorf(p->size), floorf(p->size), p->r, p->g, p->b, (Uint8)(220*alpha));
            break;
        case PTYPE_BLOOD_DROP: {
            SDL_SetRenderDrawColor(r, 204, 0, 83, (Uint8)(235*alpha));
            float sz = p->size;
            float cs = cosf(p->rotation), sn = sinf(p->rotation);
            float hw = sz * 2, hh = sz;
            /* elongated along velocity — approximate with 2 overlapping circles */
            draw_filled_circle(r, px + cs*hw*0.3f, py + sn*hw*0.3f, hh);
            draw_filled_circle(r, px - cs*hw*0.3f, py - sn*hw*0.3f, hh * 0.7f);
            break;
        }
        case PTYPE_SPARK:
            draw_pixel_rect(r, floorf(px), floorf(py), 3, 3, p->r, p->g, p->b, (Uint8)(235*alpha));
            break;
        case PTYPE_SMOKE:
            draw_pixel_rect(r, floorf(px), floorf(py), p->size + 3, p->size + 2, 64, 98, 135, (Uint8)(90*alpha));
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

    /* card tray */
    draw_pixel_rect(r, 0, TRAY_Y, VIRTUAL_W, TRAY_H, 31, 24, 47, 255);
    draw_pixel_speckles(r, 0, TRAY_Y + 8, VIRTUAL_W, TRAY_H - 16, 64, 98, 135, 48, 77);
    SDL_SetRenderDrawColor(r, 64, 98, 135, 255);
    SDL_RenderLine(r, 0, (float)TRAY_Y, (float)VIRTUAL_W, (float)TRAY_Y);
    SDL_SetRenderDrawColor(r, 20, 15, 32, 255);
    SDL_RenderLine(r, 0, TRAY_Y + 1.0f, (float)VIRTUAL_W, TRAY_Y + 1.0f);

    /* dice tray */
    for (int i = 0; i < MAX_DICE; i++)
        draw_die(r, &game->dice[i]);

    /* rolled-but-unplaced character hovers above its die */
    for (int i = 0; i < MAX_DICE; i++) {
        Dice *d = &game->dice[i];
        if (d->state == DICE_ROLLED && i != game->dragging_dice) {
            float bob = sinf(d->anim_t * 3.5f) * 4.0f;
            draw_unit_token(r, game->sprite_atlas, d->face,
                            d->home_x, d->home_y - 42 + bob, 1.0f, 175, d->anim_t);
        }
    }

    /* the rolled character in hand follows the cursor */
    if (game->dragging_dice >= 0) {
        Dice *d = &game->dice[game->dragging_dice];
        draw_unit_token(r, game->sprite_atlas, d->face,
                        game->drag_x, game->drag_y, 1.4f, 235, d->anim_t);
    }

    /* HUD */
    draw_pixel_rect(r, 0, 0, VIRTUAL_W, HUD_H, 31, 24, 47, 245);
    SDL_SetRenderDrawColor(r, 64, 98, 135, 255);
    SDL_RenderLine(r, 0, (float)HUD_H - 1, (float)VIRTUAL_W, (float)HUD_H - 1);

    char buf[64];
    SDL_SetRenderScale(r, 1.5f, 1.5f);
    SDL_SetRenderDrawColor(r, 226, 232, 214, 255);
    if (game->wave > 0) SDL_snprintf(buf, sizeof(buf), "Wave %d", game->wave);
    else SDL_snprintf(buf, sizeof(buf), "Get Ready!");
    SDL_RenderDebugText(r, 6, 8, buf);

    SDL_snprintf(buf, sizeof(buf), "Score %d", game->score);
    SDL_RenderDebugText(r, VIRTUAL_W/1.5f/2 - 30, 8, buf);

    SDL_SetRenderDrawColor(r, 229, 174, 58, 255);
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
        SDL_SetRenderDrawColor(r, 31, 24, 47, (Uint8)(210*ba));
        SDL_FRect banner = {0, VIRTUAL_H/2.0f - 30, (float)VIRTUAL_W, 60};
        SDL_RenderFillRect(r, &banner);
        SDL_SetRenderDrawColor(r, 120, 239, 229, (Uint8)(255*ba));
        SDL_SetRenderScale(r, 3.0f, 3.0f);
        SDL_snprintf(buf, sizeof(buf), "WAVE %d", game->wave_banner_number);
        float tw = (float)SDL_strlen(buf) * 8;
        SDL_RenderDebugText(r, (VIRTUAL_W/3.0f - tw)/2, (VIRTUAL_H/2.0f - 12)/3.0f, buf);
        SDL_SetRenderScale(r, 1, 1);
    }

    /* unit label while dragging the rolled character */
    if (game->dragging_dice >= 0) {
        Dice *d = &game->dice[game->dragging_dice];
        const char *names[] = {"", "SPEAR", "ARCHER", "KNIGHT", "CAMEL", "SAGE", "ELEPHANT"};
        int face = d->face;
        if (face >= 1 && face <= 6) {
            SDL_SetRenderDrawColor(r, UNIT_COLORS[face].r, UNIT_COLORS[face].g, UNIT_COLORS[face].b, 255);
            SDL_SetRenderScale(r, 1.5f, 1.5f);
            float tw = (float)SDL_strlen(names[face]) * 8;
            SDL_RenderDebugText(r, game->drag_x/1.5f - tw/2, (game->drag_y - 52)/1.5f, names[face]);
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
        draw_temple_backdrop(game->renderer);
        ui_render_menu(game);
        break;
    case STATE_SETTINGS:
        draw_temple_backdrop(game->renderer);
        ui_render_settings(game);
        break;
    case STATE_PLAYING:
        render_playing(game);
        break;
    case STATE_GAME_OVER:
        draw_temple_backdrop(game->renderer);
        ui_render_game_over(game);
        break;
    }
    SDL_RenderPresent(game->renderer);
}
