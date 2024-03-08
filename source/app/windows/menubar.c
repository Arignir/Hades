/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2024 - The Hades Authors
**
\******************************************************************************/

#define _GNU_SOURCE

#include <string.h>
#include <cimgui.h>
#include <nfd.h>
#include <stdio.h>
#include <float.h>
#include "hades.h"
#include "app/app.h"
#include "compat.h"

static
void
app_win_menubar_file(
    struct app *app
) {
    if (igBeginMenu("File", true)) {
        if (igMenuItem_Bool("Open", NULL, false, true)) {
            nfdresult_t result;
            nfdchar_t *path;

            result = NFD_OpenDialog(
                &path,
                (nfdfilteritem_t[1]){(nfdfilteritem_t){ .name = "GBA Rom", .spec = "gba,zip,7z,rar"}},
                1,
                NULL
            );

            if (result == NFD_OKAY) {
                app_emulator_configure_and_run(app, path);
                NFD_FreePath(path);
            }
        }

        if (igBeginMenu("Open Recent", app->file.recent_roms[0] != NULL)) {
            uint32_t x;

            for (x = 0; x < array_length(app->file.recent_roms) && app->file.recent_roms[x]; ++x) {
                if (igMenuItem_Bool(hs_basename(app->file.recent_roms[x]), NULL, false, true)) {
                    char *path;

                    // app->file.recent_roms[] is modified by `app_emulator_configure_and_run()` so we need to copy
                    // the path to a safe space first.
                    path = strdup(app->file.recent_roms[x]);

                    app_emulator_configure_and_run(app, path);

                    free(path);
                }
            }
            igEndMenu();
        }

        if (igMenuItem_Bool("Open BIOS", NULL, false, true)) {
            nfdresult_t result;
            nfdchar_t *path;

            result = NFD_OpenDialog(
                &path,
                (nfdfilteritem_t[1]){(nfdfilteritem_t){ .name = "BIOS file", .spec = "bin,bios,raw"}},
                1,
                NULL
            );

            if (result == NFD_OKAY) {
                free(app->file.bios_path);
                app->file.bios_path = strdup(path);
                NFD_FreePath(path);
            }
        }

        igSeparator();

        if (igMenuItem_Bool("Settings", NULL, false, true)) {
            app->ui.settings.open = true;
            app->ui.settings.menu = 0;
        }

        igEndMenu();
    }
}

static
void
app_win_menubar_emulation(
    struct app *app
) {
    char const *bind;

    if (igBeginMenu("Emulation", true)) {

        if (igBeginMenu("Speed", app->emulation.is_started)) {
            uint32_t x;

            bind = SDL_GetKeyName(app->binds.keyboard[BIND_EMULATOR_FAST_FORWARD_TOGGLE]);
            if (igMenuItem_Bool("Fast Forward", bind ?: "", app->emulation.fast_forward, true)) {
                app->emulation.fast_forward ^= true;
                app_emulator_speed(app, app->emulation.fast_forward ? 0 : app->emulation.speed);
            }

            igSeparator();

            char const *speed[] = {
                "x1",
                "x2",
                "x3",
                "x4",
                "x5",
            };

            igBeginDisabled(app->emulation.fast_forward);
            for (x = 0; x < 5; ++x) {
                char const *bind;

                bind = SDL_GetKeyName(app->binds.keyboard[BIND_EMULATOR_SPEED_X1 + x]);
                if (igMenuItem_Bool(speed[x], bind ?: "", app->emulation.speed == x + 1, true)) {
                    app->emulation.speed = x + 1;
                    app->emulation.fast_forward = false;
                    app_emulator_speed(app, app->emulation.speed);
                }
            }
            igEndDisabled();

            igEndMenu();
        }

        igSeparator();

        if (igBeginMenu("Quick Save", app->emulation.is_started)) {
            size_t i;

            for (i = 0; i < MAX_QUICKSAVES; ++i) {
                char *text;

                if (app->file.qsaves[i].exist && app->file.qsaves[i].mtime) {
                    text = hs_format("%zu: %s", i + 1, app->file.qsaves[i].mtime);
                } else {
                    text = hs_format("%zu: <empty>", i + 1);
                }

                hs_assert(text);

                if (igMenuItem_Bool(text, NULL, false, true)) {
                    app_emulator_quicksave(app, i);
                }

                free(text);
            }

            igEndMenu();
        }

        if (igBeginMenu("Quick Load", app->emulation.is_started)) {
            size_t i;

            for (i = 0; i < MAX_QUICKSAVES; ++i) {
                char *text;

                if (app->file.qsaves[i].exist && app->file.qsaves[i].mtime) {
                    text = hs_format("%zu: %s", i + 1, app->file.qsaves[i].mtime);
                } else {
                    text = hs_format("%zu: <empty>", i + 1);
                }

                hs_assert(text);

                if (igMenuItem_Bool(text, NULL, false, app->file.qsaves[i].exist && app->file.qsaves[i].mtime)) {
                    app_emulator_quickload(app, i);
                }

                free(text);
            }

            igEndMenu();
        }

        igSeparator();

        bind = SDL_GetKeyName(app->binds.keyboard[BIND_EMULATOR_PAUSE]);
        if (igMenuItem_Bool("Pause", bind ?: "", !app->emulation.is_running, app->emulation.is_started)) {
            if (app->emulation.is_running) {
                app_emulator_pause(app);
            } else {
                app_emulator_run(app);
            }
        }

        if (igMenuItem_Bool("Stop", NULL, false, app->emulation.is_started)) {
            app_emulator_stop(app);
        }

        bind = SDL_GetKeyName(app->binds.keyboard[BIND_EMULATOR_RESET]);
        if (igMenuItem_Bool("Reset", bind ?: "", false, app->emulation.is_started)) {
            app_emulator_reset(app);
        }

        igSeparator();

        if (igMenuItem_Bool("Emulation Settings", NULL, false, true)) {
            app->ui.settings.open = true;
            app->ui.settings.menu = MENU_EMULATION;
        }

        igEndMenu();
    }
}

static
void
app_win_menubar_video(
    struct app *app
) {
    char const *bind;

    if (igBeginMenu("Video", true)) {

        /* Display Size */
        if (igBeginMenu("Display size", true)) {
            uint32_t x;
            int width;
            int height;

            static char const * const display_sizes[] = {
                "x1",
                "x2",
                "x3",
                "x4",
                "x5",
            };

            SDL_GetWindowSize(app->sdl.window, &width, &height);
            height = max(0, height - app->ui.menubar_size.y);

            for (x = 1; x <= 5; ++x) {
                if (igMenuItem_Bool(
                    display_sizes[x - 1],
                    NULL,
                    width == GBA_SCREEN_WIDTH * x * app->ui.scale && height == GBA_SCREEN_HEIGHT * x * app->ui.scale,
                    true
                )) {
                    app->video.display_size = x;
                    app->ui.win.resize = true;
                    app->ui.win.resize_with_ratio = false;
                }
            }

            igEndMenu();
        }

        igSeparator();

        /* Take a screenshot */
        bind = SDL_GetKeyName(app->binds.keyboard[BIND_EMULATOR_SCREENSHOT]);
        if (igMenuItem_Bool("Take Screenshot", bind ?: "", false, app->emulation.is_started)) {
            app_emulator_screenshot(app);
        }

        /* Pixel Color Effect */
        if (igBeginMenu("Color Effect", true)) {
            if (igMenuItem_Bool("None", NULL, app->video.pixel_color_filter == PIXEL_COLOR_FILTER_NONE, true)) {
                app->video.pixel_color_filter = PIXEL_COLOR_FILTER_NONE;
                app_sdl_video_rebuild_pipeline(app);
            }

            igSeparator();

            if (igMenuItem_Bool("Color Correction", NULL, app->video.pixel_color_filter == PIXEL_COLOR_FILTER_COLOR_CORRECTION, true)) {
                app->video.pixel_color_filter = PIXEL_COLOR_FILTER_COLOR_CORRECTION;
                app_sdl_video_rebuild_pipeline(app);
            }

            if (igMenuItem_Bool("Grey Scale", NULL, app->video.pixel_color_filter == PIXEL_COLOR_FILTER_GREY_SCALE, true)) {
                app->video.pixel_color_filter = PIXEL_COLOR_FILTER_GREY_SCALE;
                app_sdl_video_rebuild_pipeline(app);
            }

            igEndMenu();
        }

        /* Pixel Scaling Effect */
        if (igBeginMenu("Scaling Effect", true)) {
            if (igMenuItem_Bool("None", NULL, app->video.pixel_scaling_filter == PIXEL_SCALING_FILTER_NONE, true)) {
                app->video.pixel_scaling_filter = PIXEL_SCALING_FILTER_NONE;
                app_sdl_video_rebuild_pipeline(app);
            }

            igSeparator();

            if (igMenuItem_Bool("LCD Grid /w RGB Stripes", NULL, app->video.pixel_scaling_filter == PIXEL_SCALING_FILTER_LCD_GRID_WITH_RGB_STRIPES, true)) {
                app->video.pixel_scaling_filter = PIXEL_SCALING_FILTER_LCD_GRID_WITH_RGB_STRIPES;
                app_sdl_video_rebuild_pipeline(app);
            }

            if (igMenuItem_Bool("LCD Grid", NULL, app->video.pixel_scaling_filter == PIXEL_SCALING_FILTER_LCD_GRID, true)) {
                app->video.pixel_scaling_filter = PIXEL_SCALING_FILTER_LCD_GRID;
                app_sdl_video_rebuild_pipeline(app);
            }

            igEndMenu();
        }

        igSeparator();

        if (igMenuItem_Bool("Video Settings", NULL, false, true)) {
            app->ui.settings.open = true;
            app->ui.settings.menu = MENU_VIDEO;
        }

        igEndMenu();
    }
}

static
void
app_win_menubar_audio(
    struct app *app
) {
    if (igBeginMenu("Audio", true)) {
        /* VSync */
        if (igMenuItem_Bool("Mute", NULL, app->audio.mute, true)) {
            app->audio.mute ^= 1;
        }

        igSeparator();

        if (igMenuItem_Bool("Audio Settings", NULL, false, true)) {
            app->ui.settings.open = true;
            app->ui.settings.menu = MENU_AUDIO;
        }

        igEndMenu();
    }
}

static
void
app_win_menubar_help(
    struct app *app
) {
    bool open_about;

    open_about = false;

    if (igBeginMenu("Help", true)) {

        /* Report Issue */
        if (igMenuItem_Bool("Report Issue", NULL, false, true)) {
            hs_open_url("https://github.com/Arignir/Hades/issues/new");
        }

        igSeparator();

        /* About */
        if (igMenuItem_Bool("About", NULL, false, true)) {
            open_about = true;
        }
        igEndMenu();
    }

    if (open_about) {
        igOpenPopup_Str("About", ImGuiPopupFlags_None);
    }

    // Always center the modal
    igSetNextWindowPos(
        (ImVec2){.x = app->ui.ioptr->DisplaySize.x * 0.5f, .y = app->ui.ioptr->DisplaySize.y * 0.5f},
        ImGuiCond_Always,
        (ImVec2){.x = 0.5f, .y = 0.5f}
    );

    if (
        igBeginPopupModal(
            "About",
            NULL,
            ImGuiWindowFlags_Popup
              | ImGuiWindowFlags_Modal
              | ImGuiWindowFlags_NoResize
              | ImGuiWindowFlags_NoMove
        )
    ) {
        igText("Hades");
        igSpacing();
        igSeparator();
        igSpacing();
        igText("Version: %s", HADES_VERSION);
        igText("Build date: %s", __DATE__);
        igSpacing();
        igSeparator();
        igSpacing();
        igText("Software written by Arignir");
        igText("Icon designed by Totushi");
        igSpacing();
        igText("Thank you for using Hades <3");
        igSpacing();
        if (igButton("Close", (ImVec2){.x = igGetFontSize() * 4.f, .y = igGetFontSize() * 1.5f})) {
            igCloseCurrentPopup();
        }
        igEndPopup();
    }
}

static
void
app_win_menubar_fps_counter(
    struct app *app
) {
    /* FPS Counter */
    if (app->emulation.is_started && app->emulation.is_running && igGetWindowWidth() >= GBA_SCREEN_WIDTH * 2 * app->ui.scale) {
        float spacing;
        ImVec2 out;

        spacing = igGetStyle()->ItemSpacing.x;

        igSameLine(igGetWindowWidth() - (app->ui.menubar_fps_width + spacing * 2), 1);
        igText("FPS: %u (%u%%)", app->emulation.fps, (unsigned)(app->emulation.fps / 60.0 * 100.0));
        igGetItemRectSize(&out);
        app->ui.menubar_fps_width = out.x;
    }
}

void
app_win_menubar(
    struct app *app
) {
    if (igBeginMainMenuBar()) {

        /* File */
        app_win_menubar_file(app);

        /* Emulation */
        app_win_menubar_emulation(app);

        /* Video */
        app_win_menubar_video(app);

        /* Audio */
        app_win_menubar_audio(app);

        /* Help */
        app_win_menubar_help(app);

        /* FPS */
        app_win_menubar_fps_counter(app);

        /* Capture the height of the menu bar */
        igGetWindowSize(&app->ui.menubar_size);

        igEndMainMenuBar();
    }
}
