#ifndef GAME_H
#define GAME_H

#include <SDL3/SDL.h>
#include <math.h>

#define VIRTUAL_W 400
#define VIRTUAL_H 700

#define HUD_H 40
#define FIELD_Y 108
#define FIELD_H 452
#define TRAY_Y (FIELD_Y + FIELD_H)
#define TRAY_H (VIRTUAL_H - TRAY_Y)

#define NUM_LANES 5
#define LANE_W (VIRTUAL_W / NUM_LANES)

#define MAX_UNITS 30
#define MAX_ENEMIES 60
#define MAX_PROJECTILES 40
#define MAX_PARTICLES 300
#define MAX_DICE 3
#define MAX_DECALS 40

#define DICE_COOLDOWN 4.0f
#define SMUSH_DAMAGE 30.0f
#define PLAYER_MAX_HP 10
#define WAVE_PAUSE 5.0f

typedef enum {
    UNIT_NONE = 0,
    UNIT_SPEARMAN = 1,
    UNIT_ARCHER = 2,
    UNIT_KNIGHT = 3,
    UNIT_CAMEL = 4,
    UNIT_SAGE = 5,
    UNIT_ELEPHANT = 6,
} UnitType;

typedef enum {
    ENEMY_VETALA,
    ENEMY_RAKSHASA,
    ENEMY_PISACHA,
    ENEMY_MAYA,
    ENEMY_ASURA,
} EnemyType;

typedef enum {
    DICE_READY,
    DICE_DRAGGING,
    DICE_COOLDOWN_STATE,
} DiceState;

typedef enum {
    STATE_SPLASH,
    STATE_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_GAME_OVER,
} GameState;

typedef enum {
    ANIM_IDLE,
    ANIM_ATTACK,
    ANIM_HURT,
    ANIM_WALK,
} AnimState;

typedef enum {
    PTYPE_CIRCLE,
    PTYPE_BLOOD_DROP,
    PTYPE_SPARK,
    PTYPE_SMOKE,
} ParticleType;

typedef struct {
    AnimState state;
    int frame;
    float frame_timer;
    float state_timer;
    bool flash_active;
    float flash_timer;
    float scale_x, scale_y;
    float bob_phase;
    float spawn_timer;
    float lean;
} AnimComp;

typedef struct {
    bool active;
    UnitType type;
    int lane;
    float x, y;
    float hp, max_hp;
    float attack_timer;
    float damage;
    float attack_cooldown;
    float range;
    AnimComp anim;
} Unit;

typedef struct {
    bool active;
    EnemyType type;
    int lane;
    float x, y;
    float hp, max_hp;
    float speed;
    float damage;
    float attack_timer;
    float attack_cooldown;
    bool fighting;
    AnimComp anim;
    float walk_phase;
} Enemy;

typedef struct {
    bool active;
    int lane;
    float x, y;
    float speed;
    float damage;
    UnitType source_type;
} Projectile;

typedef struct {
    bool active;
    float x, y;
    float vx, vy;
    float life;
    float max_life;
    Uint8 r, g, b;
    float size;
    ParticleType ptype;
    float rotation;
    float origin_y;
} Particle;

typedef struct {
    DiceState state;
    int face;
    float x, y;
    float home_x, home_y;
    float cooldown_timer;
    float roll_anim_timer;
} Dice;

typedef struct {
    float x, y, size;
    Uint8 r, g, b, a;
} Decal;

typedef struct {
    bool screen_shake;
    int difficulty;
} Settings;

typedef struct {
    GameState state;
    SDL_Renderer *renderer;
    SDL_Window *window;

    int player_hp;
    int score;
    int wave;

    float wave_timer;
    int enemies_remaining;
    float spawn_timer;
    bool wave_active;

    Unit units[MAX_UNITS];
    Enemy enemies[MAX_ENEMIES];
    Projectile projectiles[MAX_PROJECTILES];
    Particle particles[MAX_PARTICLES];
    Dice dice[MAX_DICE];
    Decal decals[MAX_DECALS];
    int decal_head;

    int dragging_dice;
    float drag_x, drag_y;

    float shake_timer;
    float shake_x, shake_y;

    float screen_flash_timer;
    Uint8 flash_r, flash_g, flash_b;

    float wave_banner_timer;
    int wave_banner_number;

    SDL_Texture *sprite_atlas;

    Uint64 last_tick;

    float splash_timer;
    float splash_fade;
    int menu_selection;
    int settings_selection;
    Settings settings;
    float menu_anim_timer;

    void *nk_sdl;
} Game;

void game_init(Game *game);
void game_start(Game *game);
void game_handle_event(Game *game, SDL_Event *event);
void game_update(Game *game, float dt);
void render_game(Game *game);

float lane_center_x(int lane);
void spawn_particles(Game *game, float x, float y, int count, Uint8 r, Uint8 g, Uint8 b);

#endif
