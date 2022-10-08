/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2022 - The Hades Authors
**
\******************************************************************************/

#define _GNU_SOURCE
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS

#include <cimgui.h>
#include "hades.h"
#include "app.h"
#include "gui/gui.h"

void
gui_win_game(
    struct app *app
) {
    float game_scale;
    float rel_x;
    float rel_y;
    int width;
    int height;

    /* Resize the window to keep the correct aspect ratio */
    SDL_GetWindowSize(app->sdl.window, &width, &height);
    height = max(0, height - app->ui.menubar_size.y);
    game_scale = min(width / (float)GBA_SCREEN_WIDTH, height / (float)GBA_SCREEN_HEIGHT);
    rel_x = (width  - (GBA_SCREEN_WIDTH  * game_scale)) * 0.5f;
    rel_y = (height - (GBA_SCREEN_HEIGHT * game_scale)) * 0.5f;

    igPushStyleVarVec2(ImGuiStyleVar_WindowPadding, (ImVec2){.x = 0, .y = 0});
    igPushStyleVarFloat(ImGuiStyleVar_WindowBorderSize, 0);

    igSetNextWindowPos(
        (ImVec2){
            .x = rel_x,
            .y = (float)app->ui.menubar_size.y + rel_y,
        },
        ImGuiCond_Always,
        (ImVec2){.x = 0, .y = 0}
    );

    igSetNextWindowSize(
        (ImVec2){
            .x = GBA_SCREEN_WIDTH * game_scale,
            .y = GBA_SCREEN_HEIGHT * game_scale
        },
        ImGuiCond_Always
    );

    igBegin(
        "Game",
        NULL,
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground
    );

    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);

    glBindTexture(GL_TEXTURE_2D, app->sdl.game_texture);

    if (app->video.texture_filter.refresh) {
        switch (app->video.texture_filter.kind) {
            case TEXTURE_FILTER_NEAREST: {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                break;
            };
            case TEXTURE_FILTER_LINEAR: {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                break;
            };

        }
        app->video.texture_filter.refresh = false;
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    pthread_mutex_lock(&app->emulation.gba->framebuffer_frontend_mutex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, (uint8_t *)app->emulation.gba->framebuffer_frontend);
    pthread_mutex_unlock(&app->emulation.gba->framebuffer_frontend_mutex);

    glBindTexture(GL_TEXTURE_2D, last_texture);

    igImage(
        (void *)(uintptr_t)app->sdl.game_texture,
        (ImVec2){.x = GBA_SCREEN_WIDTH * game_scale, .y = GBA_SCREEN_HEIGHT * game_scale},
        (ImVec2){.x = 0, .y = 0},
        (ImVec2){.x = 1, .y = 1},
        (ImVec4){.x = 1, .y = 1, .z = 1, .w = 1},
        (ImVec4){.x = 0, .y = 0, .z = 0, .w = 0}
    );

    igEnd();

    igPopStyleVar(2);
}