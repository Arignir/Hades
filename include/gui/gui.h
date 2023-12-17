/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2023 - The Hades Authors
**
\******************************************************************************/

#pragma once

#include "hades.h"
#include "app.h"

#define GLSL(src)           "#version 330 core\n" #src

/* gui/sdl/audio.c */
void gui_sdl_audio_init(struct app *app);
void gui_sdl_audio_cleanup(struct app *app);

/* gui/sdl/init.c */
void gui_sdl_init(struct app *app);
void gui_sdl_cleanup(struct app *app);

/* gui/sdl/input.c */
void gui_sdl_setup_default_binds(struct app *app);
void gui_sdl_handle_inputs(struct app *app);

/* gui/sdl/video.c */
void gui_sdl_video_init(struct app *app);
void gui_sdl_video_cleanup(struct app *app);
void gui_sdl_video_render_frame(struct app *app);
void gui_sdl_video_rebuild_pipeline(struct app *app);

/* gui/shaders/frag-color-correction.c */
extern char const *SHADER_FRAG_COLOR_CORRECTION;

/* gui/shaders/frag-lcd-grid.c */
extern char const *SHADER_FRAG_LCD_GRID;

/* gui/shaders/vertex-common.c */
extern char const *SHADER_VERTEX_COMMON;

/* gui/windows/keybinds.c */
void gui_win_keybinds_editor(struct app *app);

/* gui/windows/game.c */
void gui_win_game(struct app *app);

/* gui/windows/menubar.c */
void gui_win_menubar(struct app *app);

/* gui/windows/notif.c */
void gui_new_notification(struct app *app, enum ui_notification_kind, char const *msg, ...);
void gui_win_notifications(struct app *app);

/* config.c */
void gui_config_load(struct app *app);
void gui_config_save(struct app *app);
void gui_config_push_recent_rom(struct app *app, char const *path);
