#ifndef NK_SDL3_H
#define NK_SDL3_H

#include <SDL3/SDL.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

struct nk_sdl3 {
    SDL_Renderer *renderer;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    SDL_Texture *font_tex;
    int logical_w, logical_h;
};

struct nk_sdl3 *nk_sdl3_init(SDL_Renderer *renderer, int logical_w, int logical_h);
void nk_sdl3_shutdown(struct nk_sdl3 *sdl);
void nk_sdl3_handle_event(struct nk_sdl3 *sdl, SDL_Event *event);
void nk_sdl3_render(struct nk_sdl3 *sdl);
void nk_sdl3_new_frame(struct nk_sdl3 *sdl);

#endif
