#include "game.h"
#include <stdlib.h>
#include <string.h>

typedef struct { float hp, damage, cooldown, range; } UnitStats;
static const UnitStats UNIT_STATS[] = {
    [UNIT_NONE]   = {0,   0,  0,    0},
    [UNIT_SPEARMAN]={40,  10, 0.8f, 35},
    [UNIT_ARCHER] = {30,  8,  1.2f, 250},
    [UNIT_KNIGHT]  ={150, 3,  2.0f, 35},
    [UNIT_CAMEL]   ={35,  15, 2.5f, 120},
    [UNIT_SAGE]    ={25,  0,  3.0f, 0},
    [UNIT_ELEPHANT]={80,  25, 1.5f, 180},
};

typedef struct { float hp, speed, damage, cooldown; } EnemyStats;
static const EnemyStats ENEMY_STATS[] = {
    [ENEMY_VETALA]   = {30,  30, 5,  1.5f},
    [ENEMY_RAKSHASA] = {50,  50, 8,  1.2f},
    [ENEMY_PISACHA]  = {100, 20, 15, 2.0f},
    [ENEMY_MAYA]     = {150, 35, 20, 1.8f},
    [ENEMY_ASURA]    = {500, 15, 30, 2.5f},
};

float lane_center_x(int lane) {
    return lane * LANE_W + LANE_W / 2.0f;
}

static int random_range(int min, int max) {
    return min + (rand() % (max - min + 1));
}

static float random_float(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

/* ---------- animation ---------- */

static void anim_init(AnimComp *a) {
    a->state = ANIM_IDLE;
    a->frame = 0;
    a->frame_timer = 0;
    a->state_timer = 0;
    a->flash_active = false;
    a->flash_timer = 0;
    a->scale_x = 1.0f;
    a->scale_y = 1.0f;
    a->bob_phase = random_float(0, 6.28f);
}

static void anim_set_state(AnimComp *a, AnimState s) {
    if (a->state == s) return;
    a->state = s;
    a->frame = 0;
    a->frame_timer = 0;
    a->state_timer = 0;
}

static void anim_trigger_flash(AnimComp *a) {
    a->flash_active = true;
    a->flash_timer = 0.08f;
}

static void anim_update(AnimComp *a, float dt) {
    a->state_timer += dt;
    a->bob_phase += dt * 3.0f;

    if (a->flash_active) {
        a->flash_timer -= dt;
        if (a->flash_timer <= 0) a->flash_active = false;
    }

    switch (a->state) {
    case ANIM_IDLE:
    case ANIM_WALK:
        a->frame_timer += dt;
        if (a->frame_timer >= 0.4f) {
            a->frame_timer = 0;
            a->frame = 1 - a->frame;
        }
        a->scale_y = 1.0f + 0.04f * sinf(a->bob_phase);
        a->scale_x = 1.0f - 0.04f * sinf(a->bob_phase);
        break;
    case ANIM_ATTACK:
        if (a->state_timer < 0.1f) {
            a->scale_x = 1.2f;
            a->scale_y = 0.8f;
        } else if (a->state_timer < 0.2f) {
            a->scale_x = 0.85f;
            a->scale_y = 1.2f;
        } else {
            a->state = ANIM_IDLE;
            a->scale_x = 1.0f;
            a->scale_y = 1.0f;
        }
        break;
    case ANIM_HURT:
        a->scale_x = 1.15f;
        a->scale_y = 0.85f;
        if (a->state_timer >= 0.12f) {
            a->state = ANIM_IDLE;
            a->scale_x = 1.0f;
            a->scale_y = 1.0f;
        }
        break;
    }
}

/* ---------- particles ---------- */

static Particle *alloc_particle(Game *game) {
    for (int i = 0; i < MAX_PARTICLES; i++)
        if (!game->particles[i].active) return &game->particles[i];
    return NULL;
}

void spawn_particles(Game *game, float x, float y, int count, Uint8 r, Uint8 g, Uint8 b) {
    for (int i = 0; i < count; i++) {
        Particle *p = alloc_particle(game);
        if (!p) return;
        p->active = true;
        p->ptype = PTYPE_CIRCLE;
        p->x = x; p->y = y;
        p->origin_y = y;
        p->vx = random_float(-80, 80);
        p->vy = random_float(-120, 20);
        p->life = random_float(0.3f, 0.8f);
        p->max_life = p->life;
        p->r = r; p->g = g; p->b = b;
        p->size = random_float(2, 5);
        p->rotation = 0;
    }
}

static void spawn_blood(Game *game, float x, float y, float dir_x, float damage) {
    int count = (int)(damage / 5.0f);
    if (count < 3) count = 3;
    if (count > 15) count = 15;
    for (int i = 0; i < count; i++) {
        Particle *p = alloc_particle(game);
        if (!p) return;
        p->active = true;
        p->ptype = PTYPE_BLOOD_DROP;
        p->x = x + random_float(-4, 4);
        p->y = y + random_float(-4, 4);
        p->origin_y = y;
        p->vx = dir_x * random_float(40, 120) + random_float(-30, 30);
        p->vy = random_float(-140, -20);
        p->life = random_float(0.4f, 0.9f);
        p->max_life = p->life;
        int shade = random_range(0, 2);
        if (shade == 0) { p->r = 180; p->g = 20; p->b = 20; }
        else if (shade == 1) { p->r = 140; p->g = 10; p->b = 10; }
        else { p->r = 200; p->g = 30; p->b = 30; }
        p->size = random_float(1.5f, 3.5f);
        p->rotation = 0;
    }
}

static void spawn_impact_sparks(Game *game, float x, float y, int count) {
    for (int i = 0; i < count; i++) {
        Particle *p = alloc_particle(game);
        if (!p) return;
        p->active = true;
        p->ptype = PTYPE_SPARK;
        p->x = x; p->y = y;
        p->origin_y = y;
        p->vx = random_float(-100, 100);
        p->vy = random_float(-100, -20);
        p->life = random_float(0.08f, 0.2f);
        p->max_life = p->life;
        p->r = 255; p->g = 255; p->b = random_range(150, 255);
        p->size = random_float(1.0f, 2.5f);
        p->rotation = 0;
    }
}

static void spawn_death_smoke(Game *game, float x, float y, int count) {
    for (int i = 0; i < count; i++) {
        Particle *p = alloc_particle(game);
        if (!p) return;
        p->active = true;
        p->ptype = PTYPE_SMOKE;
        p->x = x + random_float(-8, 8);
        p->y = y + random_float(-4, 4);
        p->origin_y = y;
        p->vx = random_float(-20, 20);
        p->vy = random_float(-40, -10);
        p->life = random_float(0.5f, 1.2f);
        p->max_life = p->life;
        p->r = 80; p->g = 80; p->b = 80;
        p->size = random_float(3, 6);
        p->rotation = 0;
    }
}

static void add_decal(Game *game, float x, float y) {
    Decal *d = &game->decals[game->decal_head];
    d->x = x; d->y = y;
    d->size = random_float(3, 8);
    d->r = 100; d->g = 10; d->b = 10; d->a = 180;
    game->decal_head = (game->decal_head + 1) % MAX_DECALS;
}

static void spawn_death_burst(Game *game, float x, float y, bool is_boss) {
    spawn_blood(game, x, y, 0, is_boss ? 100.0f : 40.0f);
    spawn_death_smoke(game, x, y, is_boss ? 12 : 5);
    spawn_impact_sparks(game, x, y, is_boss ? 10 : 4);
    for (int i = 0; i < (is_boss ? 5 : 2); i++)
        add_decal(game, x + random_float(-10, 10), y + random_float(-5, 5));
    if (is_boss) {
        game->screen_flash_timer = 0.25f;
        game->flash_r = 255; game->flash_g = 200; game->flash_b = 100;
        game->shake_timer = 0.5f;
    }
}

/* ---------- init / start ---------- */

void game_init(Game *game) {
    game->state = STATE_TITLE;
    game->last_tick = SDL_GetTicks();
    game->dragging_dice = -1;
    srand((unsigned int)SDL_GetTicks());
}

void game_start(Game *game) {
    game->state = STATE_PLAYING;
    game->player_hp = PLAYER_MAX_HP;
    game->score = 0;
    game->wave = 0;
    game->wave_timer = 3.0f;
    game->enemies_remaining = 0;
    game->spawn_timer = 0;
    game->wave_active = false;
    game->dragging_dice = -1;
    game->shake_timer = 0;
    game->shake_x = 0; game->shake_y = 0;
    game->screen_flash_timer = 0;
    game->wave_banner_timer = 0;
    game->decal_head = 0;

    memset(game->units, 0, sizeof(game->units));
    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->projectiles, 0, sizeof(game->projectiles));
    memset(game->particles, 0, sizeof(game->particles));
    memset(game->decals, 0, sizeof(game->decals));

    for (int i = 0; i < MAX_DICE; i++) {
        Dice *d = &game->dice[i];
        d->state = DICE_READY;
        d->face = random_range(1, 6);
        d->home_x = (VIRTUAL_W / 2.0f) + (i - 1) * 100.0f;
        d->home_y = TRAY_Y + TRAY_H / 2.0f;
        d->x = d->home_x;
        d->y = d->home_y;
        d->cooldown_timer = 0;
        d->roll_anim_timer = 0;
    }
}

/* ---------- spawning ---------- */

static void spawn_unit(Game *game, int lane, float y, UnitType type) {
    for (int i = 0; i < MAX_UNITS; i++) {
        if (!game->units[i].active) {
            Unit *u = &game->units[i];
            u->active = true;
            u->type = type;
            u->lane = lane;
            u->x = lane_center_x(lane);
            u->y = y;
            u->hp = UNIT_STATS[type].hp;
            u->max_hp = UNIT_STATS[type].hp;
            u->damage = UNIT_STATS[type].damage;
            u->attack_cooldown = UNIT_STATS[type].cooldown;
            u->range = UNIT_STATS[type].range;
            u->attack_timer = 0;
            anim_init(&u->anim);
            return;
        }
    }
}

static void spawn_enemy(Game *game, int lane, EnemyType type) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!game->enemies[i].active) {
            Enemy *e = &game->enemies[i];
            e->active = true;
            e->type = type;
            e->lane = lane;
            e->x = lane_center_x(lane);
            e->y = FIELD_Y - 20.0f;
            e->hp = ENEMY_STATS[type].hp;
            e->max_hp = ENEMY_STATS[type].hp;
            e->speed = ENEMY_STATS[type].speed;
            e->damage = ENEMY_STATS[type].damage;
            e->attack_timer = 0;
            e->attack_cooldown = ENEMY_STATS[type].cooldown;
            e->fighting = false;
            e->walk_phase = random_float(0, 6.28f);
            anim_init(&e->anim);
            anim_set_state(&e->anim, ANIM_WALK);
            return;
        }
    }
}

static void add_projectile(Game *game, int lane, float x, float y, float damage, UnitType src) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!game->projectiles[i].active) {
            Projectile *p = &game->projectiles[i];
            p->active = true;
            p->lane = lane;
            p->x = x; p->y = y;
            p->speed = -200.0f;
            p->damage = damage;
            p->source_type = src;
            return;
        }
    }
}

/* ---------- input ---------- */

static void finger_to_render(Game *game, float fx, float fy, float *rx, float *ry) {
    int ww, wh;
    SDL_GetWindowSize(game->window, &ww, &wh);
    SDL_RenderCoordinatesFromWindow(game->renderer, fx * ww, fy * wh, rx, ry);
}

void game_handle_event(Game *game, SDL_Event *event) {
    float mx, my;
    switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_FINGER_DOWN: {
        if (event->type == SDL_EVENT_FINGER_DOWN)
            finger_to_render(game, event->tfinger.x, event->tfinger.y, &mx, &my);
        else
            SDL_RenderCoordinatesFromWindow(game->renderer, event->button.x, event->button.y, &mx, &my);

        if (game->state == STATE_TITLE) { game_start(game); return; }
        if (game->state == STATE_GAME_OVER) { game->state = STATE_TITLE; return; }

        if (game->state == STATE_PLAYING && game->dragging_dice < 0) {
            for (int i = 0; i < MAX_DICE; i++) {
                Dice *d = &game->dice[i];
                if (d->state != DICE_READY) continue;
                float dx = mx - d->x, dy = my - d->y;
                if (dx*dx + dy*dy < 40*40) {
                    d->state = DICE_DRAGGING;
                    d->roll_anim_timer = 0;
                    game->dragging_dice = i;
                    game->drag_x = mx; game->drag_y = my;
                    break;
                }
            }
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_FINGER_MOTION: {
        if (event->type == SDL_EVENT_FINGER_MOTION)
            finger_to_render(game, event->tfinger.x, event->tfinger.y, &mx, &my);
        else
            SDL_RenderCoordinatesFromWindow(game->renderer, event->motion.x, event->motion.y, &mx, &my);
        if (game->dragging_dice >= 0) {
            game->drag_x = mx; game->drag_y = my;
            game->dice[game->dragging_dice].x = mx;
            game->dice[game->dragging_dice].y = my;
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_FINGER_UP: {
        if (event->type == SDL_EVENT_FINGER_UP)
            finger_to_render(game, event->tfinger.x, event->tfinger.y, &mx, &my);
        else
            SDL_RenderCoordinatesFromWindow(game->renderer, event->button.x, event->button.y, &mx, &my);

        if (game->dragging_dice >= 0) {
            Dice *d = &game->dice[game->dragging_dice];
            if (my >= FIELD_Y && my <= FIELD_Y + FIELD_H) {
                int lane = (int)(mx / LANE_W);
                if (lane < 0) lane = 0;
                if (lane >= NUM_LANES) lane = NUM_LANES - 1;
                d->face = random_range(1, 6);
                UnitType type = (UnitType)d->face;

                for (int i = 0; i < MAX_ENEMIES; i++) {
                    Enemy *e = &game->enemies[i];
                    if (!e->active || e->lane != lane) continue;
                    if (fabsf(e->y - my) < 25) {
                        e->hp -= SMUSH_DAMAGE;
                        spawn_blood(game, e->x, e->y, 0, SMUSH_DAMAGE);
                        spawn_impact_sparks(game, e->x, e->y, 6);
                        anim_trigger_flash(&e->anim);
                        anim_set_state(&e->anim, ANIM_HURT);
                        if (e->hp <= 0) {
                            e->active = false;
                            game->score += (e->type == ENEMY_ASURA) ? 50 : 10;
                            spawn_death_burst(game, e->x, e->y, e->type == ENEMY_ASURA);
                        }
                    }
                }

                spawn_unit(game, lane, my, type);
                spawn_particles(game, lane_center_x(lane), my, 6, 255, 255, 255);
                d->state = DICE_COOLDOWN_STATE;
                d->cooldown_timer = DICE_COOLDOWN;
            } else {
                d->state = DICE_READY;
            }
            d->x = d->home_x; d->y = d->home_y;
            game->dragging_dice = -1;
        }
        break;
    }
    default: break;
    }
}

/* ---------- update helpers ---------- */

static void update_dice(Game *game, float dt) {
    for (int i = 0; i < MAX_DICE; i++) {
        Dice *d = &game->dice[i];
        if (d->state == DICE_COOLDOWN_STATE) {
            d->cooldown_timer -= dt;
            if (d->cooldown_timer <= 0) { d->state = DICE_READY; d->face = random_range(1, 6); }
        } else if (d->state == DICE_DRAGGING) {
            d->roll_anim_timer += dt;
            if (d->roll_anim_timer >= 0.1f) { d->roll_anim_timer = 0; d->face = random_range(1, 6); }
        }
    }
}

static Enemy *find_closest_enemy_in_lane(Game *game, int lane, float y, float range) {
    Enemy *closest = NULL;
    float closest_dist = range;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &game->enemies[i];
        if (!e->active || e->lane != lane) continue;
        float dist = y - e->y;
        if (dist > 0 && dist < closest_dist) { closest_dist = dist; closest = e; }
    }
    return closest;
}

static Unit *find_lowest_hp_unit_nearby(Game *game, int lane, float y) {
    Unit *lowest = NULL;
    float lowest_pct = 1.0f;
    for (int i = 0; i < MAX_UNITS; i++) {
        Unit *u = &game->units[i];
        if (!u->active || abs(u->lane - lane) > 1 || fabsf(u->y - y) > 150) continue;
        float pct = u->hp / u->max_hp;
        if (pct < 1.0f && pct < lowest_pct) { lowest_pct = pct; lowest = u; }
    }
    return lowest;
}

static void damage_enemy(Game *game, Enemy *e, float dmg, float dir_x) {
    e->hp -= dmg;
    anim_trigger_flash(&e->anim);
    anim_set_state(&e->anim, ANIM_HURT);
    spawn_blood(game, e->x, e->y, dir_x, dmg);
    spawn_impact_sparks(game, e->x, e->y, 3);
    if (e->hp <= 0) {
        e->active = false;
        game->score += (e->type == ENEMY_ASURA) ? 50 : 10;
        spawn_death_burst(game, e->x, e->y, e->type == ENEMY_ASURA);
    }
}

static void damage_unit(Game *game, Unit *u, float dmg) {
    u->hp -= dmg;
    anim_trigger_flash(&u->anim);
    anim_set_state(&u->anim, ANIM_HURT);
    spawn_blood(game, u->x, u->y, 0, dmg);
    if (u->hp <= 0) {
        u->active = false;
        spawn_death_burst(game, u->x, u->y, false);
    }
}

static void update_units(Game *game, float dt) {
    for (int i = 0; i < MAX_UNITS; i++) {
        Unit *u = &game->units[i];
        if (!u->active) continue;
        anim_update(&u->anim, dt);
        u->attack_timer += dt;
        if (u->attack_timer < u->attack_cooldown) continue;

        switch (u->type) {
        case UNIT_SPEARMAN:
        case UNIT_KNIGHT: {
            Enemy *t = find_closest_enemy_in_lane(game, u->lane, u->y, u->range);
            if (t) {
                anim_set_state(&u->anim, ANIM_ATTACK);
                u->attack_timer = 0;
                damage_enemy(game, t, u->damage, 0);
            }
            break;
        }
        case UNIT_ARCHER:
        case UNIT_ELEPHANT: {
            Enemy *t = find_closest_enemy_in_lane(game, u->lane, u->y, u->range);
            if (t) {
                add_projectile(game, u->lane, u->x, u->y - 15, u->damage, u->type);
                anim_set_state(&u->anim, ANIM_ATTACK);
                u->attack_timer = 0;
            }
            break;
        }
        case UNIT_CAMEL: {
            bool hit = false;
            for (int j = 0; j < MAX_ENEMIES; j++) {
                Enemy *e = &game->enemies[j];
                if (!e->active) continue;
                float dx = e->x - u->x, dy = e->y - u->y;
                if (sqrtf(dx*dx+dy*dy) < u->range && dy < 0) {
                    damage_enemy(game, e, u->damage, 0);
                    hit = true;
                }
            }
            if (hit) { anim_set_state(&u->anim, ANIM_ATTACK); u->attack_timer = 0; }
            break;
        }
        case UNIT_SAGE: {
            Unit *t = find_lowest_hp_unit_nearby(game, u->lane, u->y);
            if (t) {
                t->hp += 15;
                if (t->hp > t->max_hp) t->hp = t->max_hp;
                anim_set_state(&u->anim, ANIM_ATTACK);
                u->attack_timer = 0;
                spawn_particles(game, t->x, t->y, 3, 100, 255, 100);
            }
            break;
        }
        default: break;
        }
    }
}

static void update_enemies(Game *game, float dt) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &game->enemies[i];
        if (!e->active) continue;
        anim_update(&e->anim, dt);

        e->fighting = false;
        Unit *blocker = NULL;
        float bd = 999;
        for (int j = 0; j < MAX_UNITS; j++) {
            Unit *u = &game->units[j];
            if (!u->active || u->lane != e->lane) continue;
            if (u->y > e->y && u->y - e->y < 35) {
                float d = u->y - e->y;
                if (d < bd) { bd = d; blocker = u; }
            }
        }

        if (blocker) {
            e->fighting = true;
            if (e->anim.state == ANIM_WALK) anim_set_state(&e->anim, ANIM_IDLE);
            e->attack_timer += dt;
            if (e->attack_timer >= e->attack_cooldown) {
                e->attack_timer = 0;
                anim_set_state(&e->anim, ANIM_ATTACK);
                damage_unit(game, blocker, e->damage);
            }
        } else {
            if (e->anim.state != ANIM_WALK && e->anim.state != ANIM_HURT && e->anim.state != ANIM_ATTACK)
                anim_set_state(&e->anim, ANIM_WALK);
            e->y += e->speed * dt;
            e->walk_phase += e->speed * dt * 0.15f;
            if (e->y >= FIELD_Y + FIELD_H) {
                e->active = false;
                game->player_hp--;
                game->shake_timer = 0.3f;
                game->screen_flash_timer = 0.15f;
                game->flash_r = 255; game->flash_g = 0; game->flash_b = 0;
                spawn_particles(game, e->x, (float)(FIELD_Y + FIELD_H), 10, 255, 0, 0);
                if (game->player_hp <= 0) game->state = STATE_GAME_OVER;
            }
        }
    }
}

static void update_projectiles(Game *game, float dt) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *p = &game->projectiles[i];
        if (!p->active) continue;
        p->y += p->speed * dt;
        if (p->y < FIELD_Y - 10) { p->active = false; continue; }
        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &game->enemies[j];
            if (!e->active || e->lane != p->lane) continue;
            if (fabsf(e->y - p->y) < 15) {
                p->active = false;
                damage_enemy(game, e, p->damage, 0);
                break;
            }
        }
    }
}

static void update_particles(Game *game, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &game->particles[i];
        if (!p->active) continue;

        switch (p->ptype) {
        case PTYPE_CIRCLE:
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->vy += 200 * dt;
            p->life -= dt;
            break;
        case PTYPE_BLOOD_DROP:
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->vy += 300 * dt;
            p->rotation = atan2f(p->vy, p->vx);
            p->life -= dt;
            if (p->vy > 0 && p->y > p->origin_y + 15) {
                add_decal(game, p->x, p->y);
                p->active = false;
                continue;
            }
            break;
        case PTYPE_SPARK:
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->life -= dt * 3.0f;
            break;
        case PTYPE_SMOKE:
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->vy -= 30 * dt;
            p->size += dt * 8.0f;
            p->life -= dt;
            break;
        }

        if (p->life <= 0) p->active = false;
    }
}

static EnemyType enemy_type_for_wave(int wave) {
    int r = random_range(0, 100);
    if (wave <= 2) return ENEMY_VETALA;
    if (wave <= 4) return (r < 60) ? ENEMY_VETALA : ENEMY_RAKSHASA;
    if (wave <= 7) {
        if (r < 30) return ENEMY_VETALA;
        if (r < 70) return ENEMY_RAKSHASA;
        return ENEMY_PISACHA;
    }
    if (r < 15) return ENEMY_VETALA;
    if (r < 40) return ENEMY_RAKSHASA;
    if (r < 70) return ENEMY_PISACHA;
    return ENEMY_MAYA;
}

static void update_waves(Game *game, float dt) {
    if (!game->wave_active) {
        game->wave_timer -= dt;
        if (game->wave_timer <= 0) {
            game->wave++;
            game->wave_active = true;
            game->enemies_remaining = game->wave + 2;
            if (game->wave % 5 == 0) game->enemies_remaining += 1;
            game->spawn_timer = 0;
            game->wave_banner_timer = 2.0f;
            game->wave_banner_number = game->wave;
        }
    } else {
        if (game->enemies_remaining > 0) {
            game->spawn_timer -= dt;
            if (game->spawn_timer <= 0) {
                int lane = random_range(0, NUM_LANES - 1);
                if (game->wave % 5 == 0 && game->enemies_remaining == 1)
                    spawn_enemy(game, lane, ENEMY_ASURA);
                else
                    spawn_enemy(game, lane, enemy_type_for_wave(game->wave));
                game->enemies_remaining--;
                float interval = 2.0f - game->wave * 0.1f;
                if (interval < 0.5f) interval = 0.5f;
                game->spawn_timer = interval;
            }
        } else {
            bool any = false;
            for (int i = 0; i < MAX_ENEMIES; i++)
                if (game->enemies[i].active) { any = true; break; }
            if (!any) {
                game->wave_active = false;
                game->wave_timer = WAVE_PAUSE;
                game->score += game->wave * 5;
            }
        }
    }
}

static void update_shake(Game *game, float dt) {
    if (game->shake_timer > 0) {
        game->shake_timer -= dt;
        game->shake_x = random_float(-4, 4);
        game->shake_y = random_float(-4, 4);
    } else { game->shake_x = 0; game->shake_y = 0; }
}

static void update_decals(Game *game, float dt) {
    for (int i = 0; i < MAX_DECALS; i++) {
        Decal *d = &game->decals[i];
        if (d->a > 0) {
            float fade = dt * 12.0f;
            if (fade < 1.0f) fade = 1.0f;
            if (fade >= d->a) d->a = 0;
            else d->a -= (Uint8)fade;
        }
    }
}

static void update_screen_flash(Game *game, float dt) {
    if (game->screen_flash_timer > 0) game->screen_flash_timer -= dt;
    if (game->wave_banner_timer > 0) game->wave_banner_timer -= dt;
}

void game_update(Game *game, float dt) {
    if (game->state != STATE_PLAYING) return;
    update_dice(game, dt);
    update_units(game, dt);
    update_enemies(game, dt);
    update_projectiles(game, dt);
    update_particles(game, dt);
    update_waves(game, dt);
    update_shake(game, dt);
    update_decals(game, dt);
    update_screen_flash(game, dt);
}
