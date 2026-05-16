#define NK_IMPLEMENTATION
#include "nk_sdl3.h"
#include <string.h>

struct nk_sdl3 *nk_sdl3_init(SDL_Renderer *renderer, int logical_w, int logical_h) {
    struct nk_sdl3 *sdl = SDL_calloc(1, sizeof(struct nk_sdl3));
    if (!sdl) return NULL;

    sdl->renderer = renderer;
    sdl->logical_w = logical_w;
    sdl->logical_h = logical_h;

    nk_init_default(&sdl->ctx, 0);

    nk_font_atlas_init_default(&sdl->atlas);
    nk_font_atlas_begin(&sdl->atlas);
    struct nk_font *font = nk_font_atlas_add_default(&sdl->atlas, 18, 0);
    const void *image;
    int w, h;
    image = nk_font_atlas_bake(&sdl->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

    SDL_Surface *surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (surf) {
        memcpy(surf->pixels, image, (size_t)(w * h * 4));
        sdl->font_tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_DestroySurface(surf);
        if (sdl->font_tex) {
            SDL_SetTextureBlendMode(sdl->font_tex, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(sdl->font_tex, SDL_SCALEMODE_LINEAR);
        }
    }

    struct nk_draw_null_texture null_tex;
    nk_font_atlas_end(&sdl->atlas, nk_handle_ptr(sdl->font_tex), &null_tex);
    if (font) {
        font->handle.height = 16;
        nk_style_set_font(&sdl->ctx, &font->handle);
    }

    nk_input_begin(&sdl->ctx);

    return sdl;
}

void nk_sdl3_shutdown(struct nk_sdl3 *sdl) {
    if (!sdl) return;
    nk_font_atlas_clear(&sdl->atlas);
    nk_free(&sdl->ctx);
    if (sdl->font_tex) SDL_DestroyTexture(sdl->font_tex);
    SDL_free(sdl);
}

void nk_sdl3_handle_event(struct nk_sdl3 *sdl, SDL_Event *event) {
    struct nk_context *ctx = &sdl->ctx;
    float mx, my;

    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION:
        SDL_RenderCoordinatesFromWindow(sdl->renderer,
            event->motion.x, event->motion.y, &mx, &my);
        nk_input_motion(ctx, (int)mx, (int)my);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        int down = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        SDL_RenderCoordinatesFromWindow(sdl->renderer,
            event->button.x, event->button.y, &mx, &my);
        nk_input_motion(ctx, (int)mx, (int)my);
        if (event->button.button == SDL_BUTTON_LEFT)
            nk_input_button(ctx, NK_BUTTON_LEFT, (int)mx, (int)my, down);
        else if (event->button.button == SDL_BUTTON_RIGHT)
            nk_input_button(ctx, NK_BUTTON_RIGHT, (int)mx, (int)my, down);
        else if (event->button.button == SDL_BUTTON_MIDDLE)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)mx, (int)my, down);
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
        nk_input_scroll(ctx, nk_vec2(event->wheel.x, event->wheel.y));
        break;
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP: {
        int down = (event->type == SDL_EVENT_FINGER_DOWN);
        int ww, wh;
        SDL_GetWindowSize(SDL_GetRenderWindow(sdl->renderer), &ww, &wh);
        SDL_RenderCoordinatesFromWindow(sdl->renderer,
            event->tfinger.x * ww, event->tfinger.y * wh, &mx, &my);
        nk_input_motion(ctx, (int)mx, (int)my);
        nk_input_button(ctx, NK_BUTTON_LEFT, (int)mx, (int)my, down);
        break;
    }
    case SDL_EVENT_FINGER_MOTION: {
        int ww, wh;
        SDL_GetWindowSize(SDL_GetRenderWindow(sdl->renderer), &ww, &wh);
        SDL_RenderCoordinatesFromWindow(sdl->renderer,
            event->tfinger.x * ww, event->tfinger.y * wh, &mx, &my);
        nk_input_motion(ctx, (int)mx, (int)my);
        break;
    }
    default: break;
    }
}

void nk_sdl3_new_frame(struct nk_sdl3 *sdl) {
    nk_input_end(&sdl->ctx);
}

static void nk_sdl3_render_cmd(struct nk_sdl3 *sdl, const struct nk_command *cmd) {
    SDL_Renderer *r = sdl->renderer;

    switch (cmd->type) {
    case NK_COMMAND_NOP: break;
    case NK_COMMAND_SCISSOR: {
        const struct nk_command_scissor *s = (const struct nk_command_scissor *)cmd;
        SDL_Rect clip = {s->x, s->y, s->w, s->h};
        SDL_SetRenderClipRect(r, &clip);
        break;
    }
    case NK_COMMAND_LINE: {
        const struct nk_command_line *l = (const struct nk_command_line *)cmd;
        SDL_SetRenderDrawColor(r, l->color.r, l->color.g, l->color.b, l->color.a);
        SDL_RenderLine(r, (float)l->begin.x, (float)l->begin.y,
                         (float)l->end.x, (float)l->end.y);
        break;
    }
    case NK_COMMAND_RECT: {
        const struct nk_command_rect *rc = (const struct nk_command_rect *)cmd;
        SDL_SetRenderDrawColor(r, rc->color.r, rc->color.g, rc->color.b, rc->color.a);
        SDL_FRect fr = {(float)rc->x, (float)rc->y, (float)rc->w, (float)rc->h};
        SDL_RenderRect(r, &fr);
        break;
    }
    case NK_COMMAND_RECT_FILLED: {
        const struct nk_command_rect_filled *rc = (const struct nk_command_rect_filled *)cmd;
        SDL_SetRenderDrawColor(r, rc->color.r, rc->color.g, rc->color.b, rc->color.a);
        SDL_FRect fr = {(float)rc->x, (float)rc->y, (float)rc->w, (float)rc->h};
        SDL_RenderFillRect(r, &fr);
        break;
    }
    case NK_COMMAND_CIRCLE: {
        const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
        SDL_SetRenderDrawColor(r, c->color.r, c->color.g, c->color.b, c->color.a);
        float cx = c->x + c->w / 2.0f, cy = c->y + c->h / 2.0f;
        float rx = c->w / 2.0f, ry = c->h / 2.0f;
        for (float y = -ry; y <= ry; y += 1.0f) {
            float hw = rx * sqrtf(1.0f - (y * y) / (ry * ry));
            SDL_RenderLine(r, cx - hw, cy + y, cx + hw, cy + y);
        }
        break;
    }
    case NK_COMMAND_CIRCLE_FILLED: {
        const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
        SDL_SetRenderDrawColor(r, c->color.r, c->color.g, c->color.b, c->color.a);
        float cx = c->x + c->w / 2.0f, cy = c->y + c->h / 2.0f;
        float rx = c->w / 2.0f, ry = c->h / 2.0f;
        for (float y = -ry; y <= ry; y += 1.0f) {
            float hw = rx * sqrtf(1.0f - (y * y) / (ry * ry));
            SDL_RenderLine(r, cx - hw, cy + y, cx + hw, cy + y);
        }
        break;
    }
    case NK_COMMAND_TRIANGLE: {
        const struct nk_command_triangle *t = (const struct nk_command_triangle *)cmd;
        SDL_SetRenderDrawColor(r, t->color.r, t->color.g, t->color.b, t->color.a);
        SDL_RenderLine(r, (float)t->a.x, (float)t->a.y, (float)t->b.x, (float)t->b.y);
        SDL_RenderLine(r, (float)t->b.x, (float)t->b.y, (float)t->c.x, (float)t->c.y);
        SDL_RenderLine(r, (float)t->c.x, (float)t->c.y, (float)t->a.x, (float)t->a.y);
        break;
    }
    case NK_COMMAND_TRIANGLE_FILLED: {
        const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
        SDL_SetRenderDrawColor(r, t->color.r, t->color.g, t->color.b, t->color.a);
        float miny = (float)t->a.y, maxy = (float)t->a.y;
        if (t->b.y < miny) miny = (float)t->b.y;
        if (t->c.y < miny) miny = (float)t->c.y;
        if (t->b.y > maxy) maxy = (float)t->b.y;
        if (t->c.y > maxy) maxy = (float)t->c.y;
        for (float y = miny; y <= maxy; y += 1.0f) {
            float xs[3], cnt = 0;
            float pts[][2] = {{(float)t->a.x,(float)t->a.y},{(float)t->b.x,(float)t->b.y},{(float)t->c.x,(float)t->c.y}};
            for (int i = 0; i < 3 && cnt < 3; i++) {
                int j = (i + 1) % 3;
                float y0 = pts[i][1], y1 = pts[j][1];
                if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y)) {
                    float frac = (y - y0) / (y1 - y0);
                    xs[(int)cnt++] = pts[i][0] + frac * (pts[j][0] - pts[i][0]);
                }
            }
            if (cnt >= 2) {
                float x0 = xs[0], x1 = xs[1];
                if (x0 > x1) { float tmp = x0; x0 = x1; x1 = tmp; }
                SDL_RenderLine(r, x0, y, x1, y);
            }
        }
        break;
    }
    case NK_COMMAND_TEXT: {
        const struct nk_command_text *txt = (const struct nk_command_text *)cmd;
        if (!txt->font) break;
        SDL_SetRenderDrawColor(r, txt->foreground.r, txt->foreground.g, txt->foreground.b, txt->foreground.a);
        char buf[256];
        int len = txt->length < 255 ? txt->length : 255;
        memcpy(buf, txt->string, len);
        buf[len] = '\0';
        float font_h = txt->font->height;
        float scale = font_h / 8.0f;
        if (scale < 1.0f) scale = 1.0f;
        if (scale > 4.0f) scale = 4.0f;
        SDL_Rect old_clip;
        bool had_clip = SDL_GetRenderClipRect(r, &old_clip);
        if (had_clip && (old_clip.w > 0 || old_clip.h > 0)) {
            SDL_Rect scaled_clip = {
                (int)(old_clip.x / scale),
                (int)(old_clip.y / scale),
                (int)ceilf(old_clip.w / scale),
                (int)ceilf(old_clip.h / scale)
            };
            SDL_SetRenderScale(r, scale, scale);
            SDL_SetRenderClipRect(r, &scaled_clip);
        } else {
            SDL_SetRenderScale(r, scale, scale);
        }
        float tx = (float)txt->x / scale;
        float ty = ((float)txt->y + ((float)txt->h - font_h) * 0.5f) / scale;
        SDL_RenderDebugText(r, tx, ty, buf);
        SDL_SetRenderScale(r, 1.0f, 1.0f);
        if (had_clip && (old_clip.w > 0 || old_clip.h > 0)) {
            SDL_SetRenderClipRect(r, &old_clip);
        }
        break;
    }
    case NK_COMMAND_IMAGE: {
        const struct nk_command_image *img = (const struct nk_command_image *)cmd;
        SDL_Texture *tex = (SDL_Texture *)img->img.handle.ptr;
        if (!tex) break;
        SDL_FRect src = {(float)img->img.region[0], (float)img->img.region[1],
                         (float)img->img.region[2], (float)img->img.region[3]};
        SDL_FRect dst = {(float)img->x, (float)img->y, (float)img->w, (float)img->h};
        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
        SDL_RenderTexture(r, tex, &src, &dst);
        break;
    }
    default: break;
    }
}

void nk_sdl3_render(struct nk_sdl3 *sdl) {
    const struct nk_command *cmd = 0;
    SDL_SetRenderDrawBlendMode(sdl->renderer, SDL_BLENDMODE_BLEND);
    nk_foreach(cmd, &sdl->ctx) {
        nk_sdl3_render_cmd(sdl, cmd);
    }
    SDL_SetRenderClipRect(sdl->renderer, NULL);

    nk_clear(&sdl->ctx);
    nk_input_begin(&sdl->ctx);
}
