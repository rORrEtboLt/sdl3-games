#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "game.h"
#include "sprites.h"

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    Game *game = SDL_calloc(1, sizeof(Game));
    if (!game) return SDL_APP_FAILURE;

    game->window = SDL_CreateWindow("Randomancer",
        VIRTUAL_W, VIRTUAL_H,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!game->window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_free(game);
        return SDL_APP_FAILURE;
    }

    game->renderer = SDL_CreateRenderer(game->window, NULL);
    if (!game->renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(game->window);
        SDL_free(game);
        return SDL_APP_FAILURE;
    }

    SDL_SetRenderLogicalPresentation(game->renderer,
        VIRTUAL_W, VIRTUAL_H, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(game->renderer, 1);

    game->sprite_atlas = sprite_atlas_init(game->renderer);
    if (!game->sprite_atlas) {
        SDL_Log("Sprite atlas creation failed");
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(game->window);
        SDL_free(game);
        return SDL_APP_FAILURE;
    }

    game_init(game);

    *appstate = game;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Game *game = (Game *)appstate;

    Uint64 now = SDL_GetTicks();
    float dt = (float)(now - game->last_tick) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;
    game->last_tick = now;

    game_update(game, dt);
    render_game(game);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    game_handle_event((Game *)appstate, event);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)result;
    Game *game = (Game *)appstate;
    if (game) {
        sprite_atlas_cleanup(game->sprite_atlas);
        if (game->renderer) SDL_DestroyRenderer(game->renderer);
        if (game->window) SDL_DestroyWindow(game->window);
        SDL_free(game);
    }
}
