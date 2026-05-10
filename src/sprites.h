#ifndef SPRITES_H
#define SPRITES_H

#include <SDL3/SDL.h>

#define SPRITE_SIZE 24
#define ATLAS_W 256
#define ATLAS_H 256

SDL_Texture *sprite_atlas_init(SDL_Renderer *renderer);
void sprite_atlas_cleanup(SDL_Texture *atlas);

SDL_FRect sprite_unit_src(int unit_type, int frame);
SDL_FRect sprite_enemy_src(int enemy_type, int frame);

#endif
