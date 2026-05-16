#include "sprites.h"

typedef struct { uint8_t r, g, b, a; } RGBA;
#define S SPRITE_SIZE
#define N (S*S)

/* 6-color palettes: 0=transparent, 1=body, 2=outline, 3=highlight/skin, 4=accent, 5=shadow */

static const RGBA pal_spearman[] = {
    {0,0,0,0},{94,118,166,255},{31,24,47,255},{226,178,135,255},{229,174,58,255},{47,64,96,255}};
static const RGBA pal_archer[] = {
    {0,0,0,0},{96,154,122,255},{31,24,47,255},{226,178,135,255},{142,83,83,255},{50,86,78,255}};
static const RGBA pal_knight[] = {
    {0,0,0,0},{118,154,178,255},{31,24,47,255},{226,178,135,255},{206,138,74,255},{56,77,110,255}};
static const RGBA pal_camel[] = {
    {0,0,0,0},{198,158,108,255},{31,24,47,255},{226,178,135,255},{209,112,65,255},{120,88,52,255}};
static const RGBA pal_sage[] = {
    {0,0,0,0},{222,150,70,255},{31,24,47,255},{233,222,205,255},{120,239,229,255},{150,92,40,255}};
static const RGBA pal_elephant[] = {
    {0,0,0,0},{140,128,170,255},{31,24,47,255},{210,200,225,255},{229,174,58,255},{82,74,110,255}};
static const RGBA pal_vetala[] = {
    {0,0,0,0},{92,168,151,255},{31,24,47,255},{201,238,218,255},{216,58,118,255},{43,89,92,255}};
static const RGBA pal_rakshasa[] = {
    {0,0,0,0},{174,71,101,255},{31,24,47,255},{239,135,118,255},{229,174,58,255},{92,39,70,255}};
static const RGBA pal_pisacha[] = {
    {0,0,0,0},{120,124,128,255},{31,24,47,255},{198,200,196,255},{142,160,120,255},{61,60,86,255}};
static const RGBA pal_maya[] = {
    {0,0,0,0},{139,86,171,255},{31,24,47,255},{218,177,229,255},{120,239,229,255},{77,48,113,255}};
static const RGBA pal_asura[] = {
    {0,0,0,0},{158,44,76,255},{31,24,47,255},{241,120,112,255},{229,174,58,255},{84,28,54,255}};

/* ====== PROCEDURAL SPRITE PAINTER ====== */

static uint8_t SB[N];

static void s_clear(void) { for (int i = 0; i < N; i++) SB[i] = 0; }

static void s_px(int x, int y, uint8_t idx) {
    if (x < 0 || y < 0 || x >= S || y >= S) return;
    SB[y * S + x] = idx;
}

static void s_rect(int x, int y, int w, int h, uint8_t idx) {
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            s_px(xx, yy, idx);
}

static void s_disc(int cx, int cy, int r, uint8_t idx) {
    for (int yy = -r; yy <= r; yy++)
        for (int xx = -r; xx <= r; xx++)
            if (xx * xx + yy * yy <= r * r + r / 2)
                s_px(cx + xx, cy + yy, idx);
}

static void s_line(int x0, int y0, int x1, int y1, uint8_t idx) {
    int dx = x1 - x0, dy = y1 - y0;
    int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    int steps = adx > ady ? adx : ady;
    if (steps == 0) { s_px(x0, y0, idx); return; }
    for (int i = 0; i <= steps; i++) {
        int x = x0 + dx * i / steps;
        int y = y0 + dy * i / steps;
        s_px(x, y, idx);
    }
}

static void s_thickline(int x0, int y0, int x1, int y1, int t, uint8_t idx) {
    for (int o = 0; o < t; o++) {
        s_line(x0 + o, y0, x1 + o, y1, idx);
        s_line(x0, y0 + o, x1, y1 + o, idx);
    }
}

/* add a dark outline around the silhouette (8-neighbour) */
static void s_outline(void) {
    static uint8_t tmp[N];
    for (int i = 0; i < N; i++) tmp[i] = SB[i];
    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            if (tmp[y * S + x] != 0) continue;
            int solid = 0;
            for (int dy = -1; dy <= 1 && !solid; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= S || ny >= S) continue;
                    uint8_t v = tmp[ny * S + nx];
                    if (v != 0 && v != 2) { solid = 1; break; }
                }
            if (solid) SB[y * S + x] = 2;
        }
    }
}

/* ---- shared body parts ---- */

/* turban: rounded wrap with a small jewel/feather */
static void part_turban(int cx, int top, uint8_t band, uint8_t jewel) {
    s_disc(cx, top + 3, 5, band);
    s_rect(cx - 5, top + 3, 11, 3, band);
    s_px(cx, top - 2, jewel);
    s_px(cx, top - 1, jewel);
    s_px(cx + 1, top, jewel);
}

/* simple humanoid torso+legs facing right; dir=+1 player, -1 enemy */
static void part_body(int cx, int sx, int sy, uint8_t body, uint8_t shade, int legf) {
    s_rect(cx - 4, sy, 9, 8, body);          /* torso */
    s_rect(cx - 4, sy + 5, 9, 2, shade);     /* waist sash */
    /* legs / dhoti */
    s_rect(cx - 4, sy + 8, 4, 7 + (legf ? -1 : 0), body);
    s_rect(cx + 1, sy + 8, 4, 7 + (legf ? 0 : -1), body);
    (void)sx;
}

/* ---- UNIT PAINTERS ---- (player units face right, +x) */

static void paint_spearman(uint8_t *out, int f) {
    s_clear();
    int cx = 14;
    /* spear (held in right hand) */
    s_thickline(23 + f, 1, 23 + f, 30, 1, 4);
    s_rect(22 + f, 0, 4, 4, 4);              /* spear head */
    s_px(24 + f, 2, 5);
    /* head + turban */
    s_disc(cx, 8, 4, 3);                     /* face */
    part_turban(cx, 2, 1, 4);
    s_rect(cx - 3, 10, 6, 1, 5);             /* chin/beard line */
    /* body */
    part_body(cx, 0, 12, 1, 5, f);
    /* arm reaching to spear */
    s_thickline(cx + 3, 14, 23 + f, 12, 1, 3);
    /* small round shield on left */
    s_disc(cx - 6, 17, 3, 4);
    s_disc(cx - 6, 17, 1, 5);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_archer(uint8_t *out, int f) {
    s_clear();
    int cx = 13;
    /* bow arc on the right */
    for (int y = 4; y <= 24; y++) {
        int bend = (y - 14) * (y - 14) / 18;
        s_px(24 - bend, y, 4);
    }
    /* bowstring */
    s_line(24, 4, 19 + f, 14, 5);
    s_line(19 + f, 14, 24, 24, 5);
    /* arrow */
    s_line(15, 14, 26, 14, 5);
    s_px(26, 14, 4); s_px(25, 13, 4); s_px(25, 15, 4);
    /* head + turban */
    s_disc(cx, 8, 4, 3);
    part_turban(cx, 2, 1, 4);
    /* body */
    part_body(cx, 0, 12, 1, 5, f);
    /* quiver on back */
    s_rect(cx - 7, 11, 3, 8, 4);
    s_px(cx - 6, 9, 5); s_px(cx - 5, 9, 5);
    /* draw arm */
    s_thickline(cx + 3, 14, 19 + f, 14, 1, 3);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_knight(uint8_t *out, int f) {
    s_clear();
    int cx = 13;
    /* raised curved talwar sabre (arc to upper-right) */
    int hx = cx + 5, hy = 16 - f * 2;
    int arc[8][2] = {
        {hx, hy}, {hx + 3, hy - 3}, {hx + 5, hy - 7}, {hx + 6, hy - 11},
        {hx + 6, hy - 15}, {hx + 5, hy - 18}, {hx + 4, hy - 20}, {hx + 3, hy - 21}
    };
    for (int i = 0; i < 7; i++)
        s_thickline(arc[i][0], arc[i][1], arc[i + 1][0], arc[i + 1][1], 2, 4);
    s_rect(hx - 1, hy, 4, 3, 5);             /* hilt */
    s_px(hx, hy + 3, 4);
    /* head + conical Rajput helm with plume */
    s_disc(cx, 9, 4, 3);                     /* face */
    s_disc(cx, 5, 4, 1);                     /* helm dome */
    s_rect(cx - 4, 6, 9, 3, 1);
    s_line(cx, 0, cx, 4, 4);                 /* plume */
    s_px(cx - 1, 1, 4); s_px(cx + 1, 1, 4);
    s_rect(cx - 3, 11, 6, 1, 5);             /* moustache */
    /* armoured body */
    part_body(cx, 0, 13, 1, 5, f);
    s_rect(cx - 3, 14, 7, 6, 3);             /* breastplate */
    s_rect(cx - 1, 15, 3, 4, 4);             /* central boss */
    /* sword arm reaching to hilt */
    s_thickline(cx + 3, 15, hx, hy, 2, 3);
    /* big round shield (dhal) on left arm */
    s_disc(cx - 7, 19, 5, 4);
    s_disc(cx - 7, 19, 3, 1);
    s_disc(cx - 7, 19, 1, 4);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_camel(uint8_t *out, int f) {
    s_clear();
    /* long legs (walk swap) */
    s_rect(9, 19, 2, 12 - (f ? 1 : 0), 5);
    s_rect(13, 20, 2, 11 - (f ? 0 : 1), 1);
    s_rect(18, 19, 2, 12 - (f ? 1 : 0), 5);
    s_rect(22, 20, 2, 11 - (f ? 0 : 1), 1);
    /* body */
    s_disc(16, 16, 6, 1);
    s_rect(10, 14, 13, 6, 1);
    /* tall hump */
    s_disc(15, 10, 4, 1);
    /* curved neck + head facing right */
    s_thickline(21, 15, 26, 6, 3, 1);
    s_disc(27, 5, 2, 1);                     /* head */
    s_rect(28, 5, 4, 2, 1);                  /* snout */
    s_px(31, 6, 5);
    s_px(26, 2, 1); s_px(26, 3, 1);          /* ear */
    s_px(28, 4, 2);                          /* eye */
    /* decorated saddle blanket */
    s_rect(10, 13, 13, 2, 4);
    s_px(12, 12, 4); s_px(16, 12, 4); s_px(20, 12, 4);
    /* turbaned lancer seated on hump */
    s_rect(12, 6, 7, 6, 5);                  /* rider torso */
    s_disc(15, 4, 2, 3);                     /* face */
    part_turban(15, 0, 4, 1);
    s_thickline(17, 2, 31, 11 + f, 1, 4);    /* couched lance */
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_sage(uint8_t *out, int f) {
    s_clear();
    int cx = 16;
    /* staff with glow tip */
    s_thickline(8, 3, 8, 30, 1, 5);
    s_disc(8, 3, 2, 4);
    /* saffron robe (wider at base) */
    for (int y = 13; y <= 29; y++) {
        int w = 3 + (y - 13);
        s_rect(cx - w / 2, y, w, 1, 1);
    }
    s_rect(cx - 2, 13, 4, 16, 5);            /* robe centre fold */
    /* head + topknot (jata) */
    s_disc(cx, 9, 4, 3);
    s_disc(cx, 4, 2, 5);                     /* knot */
    s_rect(cx - 1, 4, 2, 4, 5);
    /* long white beard */
    for (int y = 11; y <= 19; y++) {
        int w = 5 - (y - 11) / 3;
        s_rect(cx - w / 2, y, w, 1, 3);
    }
    /* blessing hand raised */
    s_disc(cx + 5, 15 - f, 2, 3);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_elephant(uint8_t *out, int f) {
    s_clear();
    /* four thick legs (slow sway) */
    s_rect(7, 23, 4, 8 - (f ? 1 : 0), 5);
    s_rect(12, 24, 4, 7, 1);
    s_rect(18, 24, 4, 7, 1);
    s_rect(23, 23, 4, 8 - (f ? 0 : 1), 5);
    /* massive rounded body */
    s_disc(15, 18, 9, 1);
    s_rect(8, 14, 16, 9, 1);
    /* big head at front-right */
    s_disc(25, 19, 6, 1);
    /* floppy ear */
    s_disc(21, 17, 5, 3);
    s_disc(21, 17, 3, 5);
    /* eye */
    s_px(27, 17, 2);
    /* white tusks */
    s_line(28, 23, 31, 27, 3);
    s_line(27, 24, 30, 28, 3);
    /* trunk curling down at the front */
    s_thickline(29, 21, 29, 27, 2, 1);
    s_px(28, 28, 1); s_px(27, 29, 1); s_px(28, 30, 1);
    /* ornate forehead plate */
    s_rect(24, 13, 7, 4, 4);
    s_px(27, 12, 4);
    /* decorated drape along body */
    s_rect(8, 22, 16, 2, 4);
    s_px(10, 24, 4); s_px(14, 24, 4); s_px(18, 24, 4); s_px(22, 24, 4);
    /* howdah carriage + canopy on the back */
    s_rect(9, 5, 12, 8, 4);
    s_rect(10, 6, 10, 6, 5);
    s_rect(7, 2, 16, 3, 4);                  /* canopy roof */
    s_px(8, 1, 4); s_px(14, 0, 4); s_px(21, 1, 4);
    s_disc(14, 8, 2, 3);                     /* mahout rider */
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

/* ---- ENEMY PAINTERS ---- (face left, toward player) */

static void paint_vetala(uint8_t *out, int f) {
    s_clear();
    int cx = 16;
    /* bat-like cloak */
    s_line(cx - 9, 10, cx, 14, 4);
    s_line(cx + 9, 10, cx, 14, 4);
    s_disc(cx - 7, 13, 3, 4);
    s_disc(cx + 7, 13, 3, 4);
    /* gaunt body, no legs (hovering) -> wisps */
    s_rect(cx - 3, 11, 7, 9, 1);
    s_line(cx - 2, 20, cx - 3, 28, 1);
    s_line(cx + 1, 20, cx + 2, 27, 1);
    s_line(cx, 20, cx, 29, 1);
    /* outstretched arms */
    s_thickline(cx - 3, 13, cx - 9, 9 - f, 1, 1);
    s_thickline(cx + 3, 13, cx + 9, 9 - f, 1, 1);
    /* head + fangs + glowing eyes */
    s_disc(cx, 7, 4, 1);
    s_px(cx - 2, 7, 3); s_px(cx + 2, 7, 3);  /* eyes */
    s_px(cx - 2, 11, 3); s_px(cx + 2, 11, 3);/* fangs */
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_rakshasa(uint8_t *out, int f) {
    s_clear();
    int cx = 16;
    /* horns */
    s_line(cx - 4, 4, cx - 7, -1, 4);
    s_line(cx + 4, 4, cx + 7, -1, 4);
    /* head */
    s_disc(cx, 7, 5, 1);
    s_rect(cx - 3, 9, 7, 2, 5);              /* snarl */
    s_px(cx - 2, 9, 3); s_px(cx + 1, 9, 3);  /* fangs */
    s_px(cx - 2, 6, 4); s_px(cx + 2, 6, 4);  /* eyes */
    /* broad muscular torso */
    s_rect(cx - 6, 12, 13, 9, 1);
    s_disc(cx - 6, 13, 3, 1);                /* shoulder */
    s_disc(cx + 6, 13, 3, 1);
    s_rect(cx - 3, 14, 7, 4, 5);             /* chest shadow */
    /* clawed arms */
    s_thickline(cx - 6, 14, cx - 9, 20 + f, 1, 1);
    s_thickline(cx + 6, 14, cx + 9, 20 - f, 1, 1);
    s_px(cx - 10, 21 + f, 4); s_px(cx + 10, 21 - f, 4);
    /* legs */
    s_rect(cx - 5, 21, 4, 9 - (f ? 1 : 0), 1);
    s_rect(cx + 2, 21, 4, 9 - (f ? 0 : 1), 1);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_pisacha(uint8_t *out, int f) {
    s_clear();
    int cx = 16;
    /* hunched gaunt ghoul */
    s_disc(cx + 2, 9, 4, 1);                 /* tilted head */
    s_px(cx, 8, 4); s_px(cx + 4, 8, 4);      /* sunken eyes */
    s_rect(cx + 1, 12, 2, 1, 5);             /* grin */
    /* curved spine / ribcage */
    for (int y = 12; y <= 22; y++) {
        int w = 6 - (y - 12) / 4;
        s_rect(cx - 2, y, w, 1, 1);
    }
    s_px(cx - 1, 15, 5); s_px(cx - 1, 18, 5);/* ribs */
    /* long dangling arms */
    s_thickline(cx + 2, 13, cx - 6 + f, 24, 1, 1);
    s_thickline(cx + 3, 14, cx + 8 - f, 23, 1, 1);
    s_px(cx - 7 + f, 25, 4);
    /* spindly legs */
    s_line(cx - 1, 22, cx - 3, 30, 1);
    s_line(cx + 2, 22, cx + 4, 29, 1);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_maya(uint8_t *out, int f) {
    s_clear();
    int cx = 16;
    /* flowing robe, floating */
    for (int y = 12; y <= 28; y++) {
        int w = 3 + (y - 12) * 3 / 4;
        s_rect(cx - w / 2, y, w, 1, 1);
    }
    s_rect(cx - 2, 12, 4, 16, 5);
    /* crown / horned headdress */
    s_disc(cx, 7, 4, 3);
    s_rect(cx - 4, 2, 9, 3, 4);
    s_px(cx - 4, 1, 4); s_px(cx, 0, 4); s_px(cx + 4, 1, 4);
    s_px(cx - 2, 7, 2); s_px(cx + 2, 7, 2);  /* eyes */
    /* four arms with conjured orbs */
    s_thickline(cx - 3, 14, cx - 9, 11 + f, 1, 1);
    s_thickline(cx + 3, 14, cx + 9, 11 - f, 1, 1);
    s_thickline(cx - 3, 17, cx - 8, 22 - f, 1, 1);
    s_thickline(cx + 3, 17, cx + 8, 22 + f, 1, 1);
    s_disc(cx - 10, 11 + f, 2, 4);
    s_disc(cx + 10, 11 - f, 2, 4);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

static void paint_asura(uint8_t *out, int f) {
    s_clear();
    int cx = 16;
    /* towering multi-armed demon boss */
    /* triple-horned crowned head */
    s_disc(cx, 8, 5, 1);
    s_line(cx - 5, 5, cx - 8, -1, 4);
    s_line(cx + 5, 5, cx + 8, -1, 4);
    s_line(cx, 3, cx, -2, 4);
    s_rect(cx - 4, 2, 9, 2, 4);              /* crown band */
    s_px(cx - 3, 7, 4); s_px(cx + 2, 7, 4);  /* burning eyes */
    s_rect(cx - 3, 11, 7, 2, 5);             /* fanged maw */
    s_px(cx - 2, 11, 3); s_px(cx, 11, 3); s_px(cx + 2, 11, 3);
    /* massive torso */
    s_rect(cx - 8, 13, 17, 12, 1);
    s_disc(cx - 8, 14, 4, 1);
    s_disc(cx + 8, 14, 4, 1);
    s_rect(cx - 4, 15, 9, 6, 5);             /* armoured chest */
    s_rect(cx - 2, 16, 5, 4, 4);             /* gem */
    /* four arms wielding blades */
    s_thickline(cx - 8, 15, cx - 13, 8 - f, 2, 1);
    s_thickline(cx + 8, 15, cx + 13, 8 + f, 2, 1);
    s_thickline(cx - 8, 19, cx - 12, 26 + f, 2, 1);
    s_thickline(cx + 8, 19, cx + 12, 26 - f, 2, 1);
    s_thickline(cx - 13, 8 - f, cx - 14, 0 - f, 1, 4);   /* upper blades */
    s_thickline(cx + 13, 8 + f, cx + 14, 0 + f, 1, 4);
    s_px(cx - 13, 27 + f, 4); s_px(cx + 13, 27 - f, 4);  /* lower fists */
    /* legs */
    s_rect(cx - 6, 25, 5, 7 - (f ? 1 : 0), 1);
    s_rect(cx + 2, 25, 5, 7 - (f ? 0 : 1), 1);
    s_outline();
    for (int i = 0; i < N; i++) out[i] = SB[i];
}

/* ====== SPRITE TABLE ====== */

typedef void (*Painter)(uint8_t *out, int frame);

typedef struct {
    const RGBA *palette;
    Painter paint;
} SpriteEntry;

static const SpriteEntry unit_sprites[] = {
    [0] = { pal_spearman, paint_spearman },
    [1] = { pal_archer,   paint_archer   },
    [2] = { pal_knight,   paint_knight   },
    [3] = { pal_camel,    paint_camel    },
    [4] = { pal_sage,     paint_sage     },
    [5] = { pal_elephant, paint_elephant },
};

static const SpriteEntry enemy_sprites[] = {
    [0] = { pal_vetala,   paint_vetala   },
    [1] = { pal_rakshasa, paint_rakshasa },
    [2] = { pal_pisacha,  paint_pisacha  },
    [3] = { pal_maya,     paint_maya     },
    [4] = { pal_asura,    paint_asura    },
};

/* ====== ATLAS BUILD ====== */
/*
  Row 0 (y=0):   units idx 0–4 => cols 0–9 (10 cells for 5 types × 2 frames)
  Row 1 (y=S):   unit idx 5 (elephant) cols 0–1, enemies idx 0–3 cols 2–9
  Row 2 (y=2S):  enemy idx 4 (asura) cols 0–1
*/

static uint8_t clamp_u8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8_t)v;
}

static uint32_t watercolor_hash(int x, int y, int salt) {
    uint32_t n = (uint32_t)(x * 374761393u + y * 668265263u + salt * 362437u);
    n = (n ^ (n >> 13)) * 1274126177u;
    return n ^ (n >> 16);
}

static void put_pixel(uint8_t *pixels, int pitch, int x, int y, RGBA c) {
    int off = y * pitch + x * 4;
    pixels[off + 0] = c.r;
    pixels[off + 1] = c.g;
    pixels[off + 2] = c.b;
    pixels[off + 3] = c.a;
}

static void blit_sprite(uint8_t *pixels, int pitch,
                        int ax, int ay,
                        const RGBA *pal, const uint8_t *data) {
    int salt = ax * 17 + ay * 31;
    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            uint8_t idx = data[y * S + x];
            RGBA c = pal[idx];
            if (idx != 0 && idx != 2 && ((watercolor_hash(x, y, salt) & 15u) == 0u)) {
                c.r = clamp_u8(c.r + 16);
                c.g = clamp_u8(c.g + 16);
                c.b = clamp_u8(c.b + 16);
            }
            put_pixel(pixels, pitch, ax + x, ay + y, c);
        }
    }
}

static void blit_painter(uint8_t *px, int pitch, int ax, int ay,
                         const SpriteEntry *e, int frame) {
    static uint8_t buf[N];
    e->paint(buf, frame);
    blit_sprite(px, pitch, ax, ay, e->palette, buf);
}

SDL_Surface *sprite_atlas_build_surface(void) {
    SDL_Surface *surf = SDL_CreateSurface(ATLAS_W, ATLAS_H, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return NULL;
    SDL_memset(surf->pixels, 0, (size_t)(surf->pitch * ATLAS_H));
    uint8_t *px = (uint8_t *)surf->pixels;
    int pitch = surf->pitch;

    /* row 0: first 5 unit types (idx 0-4) */
    for (int t = 0; t < 5; t++)
        for (int f = 0; f < 2; f++)
            blit_painter(px, pitch, (t * 2 + f) * S, 0, &unit_sprites[t], f);

    /* row 1 cols 0-1: unit idx 5 (elephant) */
    for (int f = 0; f < 2; f++)
        blit_painter(px, pitch, f * S, S, &unit_sprites[5], f);

    /* row 1 cols 2-9: enemy idx 0-3 */
    for (int t = 0; t < 4; t++)
        for (int f = 0; f < 2; f++)
            blit_painter(px, pitch, (2 + t * 2 + f) * S, S, &enemy_sprites[t], f);

    /* row 2 cols 0-1: enemy idx 4 (asura) */
    for (int f = 0; f < 2; f++)
        blit_painter(px, pitch, f * S, 2 * S, &enemy_sprites[4], f);

    return surf;
}

SDL_Texture *sprite_atlas_init(SDL_Renderer *renderer) {
    SDL_Surface *surf = sprite_atlas_build_surface();
    if (!surf) return NULL;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);
    if (!tex) return NULL;
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

void sprite_atlas_cleanup(SDL_Texture *atlas) {
    if (atlas) SDL_DestroyTexture(atlas);
}

SDL_FRect sprite_unit_src(int unit_type, int frame) {
    int idx = unit_type - 1;
    if (idx < 0 || idx > 5) idx = 0;
    int f = (frame & 1);
    if (idx < 5) {
        float col = (float)(idx * 2 + f);
        return (SDL_FRect){ col * S, 0, S, S };
    }
    /* elephant (idx 5) is on row 1 cols 0-1 */
    return (SDL_FRect){ (float)(f * S), (float)S, S, S };
}

SDL_FRect sprite_enemy_src(int enemy_type, int frame) {
    int idx = enemy_type;
    if (idx < 0 || idx > 4) idx = 0;
    int f = (frame & 1);
    if (idx < 4) {
        float col = (float)(2 + idx * 2 + f);
        return (SDL_FRect){ col * S, (float)S, S, S };
    }
    /* asura (idx 4) is on row 2 cols 0-1 */
    return (SDL_FRect){ (float)(f * S), (float)(2 * S), S, S };
}
