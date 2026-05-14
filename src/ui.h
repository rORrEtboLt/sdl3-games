#ifndef UI_H
#define UI_H

#include "game.h"
#include "nk_sdl3.h"

void ui_init(Game *game);
void ui_shutdown(Game *game);
void ui_handle_event(Game *game, SDL_Event *event);
void ui_render_splash(Game *game);
void ui_render_menu(Game *game);
void ui_render_settings(Game *game);
void ui_render_game_over(Game *game);

#endif
