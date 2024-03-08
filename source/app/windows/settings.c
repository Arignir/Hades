/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2024 - The Hades Authors
**
\******************************************************************************/

#include <cimgui.h>
#include <nfd.h>
#include "hades.h"
#include "app/app.h"

static char const *menu_names[MENU_MAX] = {
    [MENU_EMULATION] = "Emulation",
    [MENU_VIDEO] = "Video",
    [MENU_AUDIO] = "Audio",
    [MENU_BINDINGS] = "Bindings",
};

static char const *texture_filters_names[TEXTURE_FILTER_LEN] = {
    [TEXTURE_FILTER_NEAREST] = "Nearest",
    [TEXTURE_FILTER_LINEAR] = "Linear",
};

static char const *pixel_color_filters_names[PIXEL_COLOR_FILTER_LEN] = {
    [PIXEL_COLOR_FILTER_NONE] = "None",
    [PIXEL_COLOR_FILTER_COLOR_CORRECTION] = "Color correction",
    [PIXEL_COLOR_FILTER_GREY_SCALE] = "Grey scale",
};

static char const *pixel_scaling_filters_names[PIXEL_SCALING_FILTER_LEN] = {
    [PIXEL_SCALING_FILTER_NONE] = "None",
    [PIXEL_SCALING_FILTER_LCD_GRID] = "LCD Grid",
    [PIXEL_SCALING_FILTER_LCD_GRID_WITH_RGB_STRIPES] = "LCD Grid /w RGB Stripes",
};

static char const *aspect_ratio_names[ASPECT_RATIO_LEN] = {
    [ASPECT_RATIO_RESIZE] = "Auto-Resize",
    [ASPECT_RATIO_BORDERS] = "Black Borders",
    [ASPECT_RATIO_STRETCH] = "Stretch",
};

static char const * const speed_names[] = {
    "x1 (60 fps)",
    "x2 (120 fps)",
    "x3 (180 fps)",
    "x4 (240 fps)",
    "x5 (300 fps)",
};

static char const * const display_size_names[] = {
    "x1",
    "x2",
    "x3",
    "x4",
    "x5",
};

char const * const binds_pretty_name[] = {
    [BIND_GBA_A] = "A",
    [BIND_GBA_B] = "B",
    [BIND_GBA_L] = "L",
    [BIND_GBA_R] = "R",
    [BIND_GBA_UP] = "Up",
    [BIND_GBA_DOWN] = "Down",
    [BIND_GBA_LEFT] = "Left",
    [BIND_GBA_RIGHT] = "Right",
    [BIND_GBA_START] = "Start",
    [BIND_GBA_SELECT] = "Select",

    [BIND_EMULATOR_SPEED_X1] = "Speed x1",
    [BIND_EMULATOR_SPEED_X2] = "Speed x2",
    [BIND_EMULATOR_SPEED_X3] = "Speed x3",
    [BIND_EMULATOR_SPEED_X4] = "Speed x4",
    [BIND_EMULATOR_SPEED_X5] = "Speed x5",
    [BIND_EMULATOR_FAST_FORWARD_TOGGLE] = "Fast Forward (Toggle)",
    [BIND_EMULATOR_FAST_FORWARD_HOLD] = "Fast Forward (Hold)",
    [BIND_EMULATOR_SCREENSHOT] = "Screenshot",
    [BIND_EMULATOR_QUICKSAVE] = "Quicksave",
    [BIND_EMULATOR_QUICKLOAD] = "Quickload",
    [BIND_EMULATOR_PAUSE] = "Pause",
    [BIND_EMULATOR_STOP] = "Stop",
    [BIND_EMULATOR_RESET] = "Reset",
};

char const * const binds_slug[] = {
    [BIND_GBA_A] = "a",
    [BIND_GBA_B] = "b",
    [BIND_GBA_L] = "l",
    [BIND_GBA_R] = "r",
    [BIND_GBA_UP] = "up",
    [BIND_GBA_DOWN] = "down",
    [BIND_GBA_LEFT] = "left",
    [BIND_GBA_RIGHT] = "right",
    [BIND_GBA_START] = "start",
    [BIND_GBA_SELECT] = "select",

    [BIND_EMULATOR_SPEED_X1] = "speed_x1",
    [BIND_EMULATOR_SPEED_X2] = "speed_x2",
    [BIND_EMULATOR_SPEED_X3] = "speed_x3",
    [BIND_EMULATOR_SPEED_X4] = "speed_x4",
    [BIND_EMULATOR_SPEED_X5] = "speed_x5",
    [BIND_EMULATOR_FAST_FORWARD_TOGGLE] = "fast_forward_toggle",
    [BIND_EMULATOR_FAST_FORWARD_HOLD] = "fast_forward_hold",
    [BIND_EMULATOR_SCREENSHOT] = "screenshot",
    [BIND_EMULATOR_QUICKSAVE] = "quicksave",
    [BIND_EMULATOR_QUICKLOAD] = "quickload",
    [BIND_EMULATOR_PAUSE] = "pause",
    [BIND_EMULATOR_STOP] = "stop",
    [BIND_EMULATOR_RESET] = "reset",
};

static
void
app_win_settings_emulation(
    struct app *app
) {
    ImGuiViewport *vp;
    int32_t speed;

    vp = igGetMainViewport();
    speed = app->emulation.speed ? app->emulation.speed - 1 : 0;

    igTextWrapped("Emulation Settings");
    igSpacing();
    igSeparator();
    igSpacing();

    igSeparatorText("BIOS");

    if (igBeginTable("##EmulationSettingsBIOS", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##EmulationSettingsBIOSLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##EmulationSettingsBIOSValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // BIOS Path
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("BIOS Path");

        igTableNextColumn();
        igBeginDisabled(true);
        igInputText("##BiosPath", app->file.bios_path, strlen(app->file.bios_path), ImGuiInputTextFlags_ReadOnly, NULL, NULL);
        igEndDisabled();
        igSameLine(0.0f, -1.0f);
        if (igButton("Choose", (ImVec2){ 50.f, 0.f})) {
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

        // Skip BIOS
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Skip BIOS Intro");

        igTableNextColumn();
        igCheckbox("##SkipBIOS", &app->emulation.skip_bios);

        igEndTable();
    }

    igSeparatorText("Speed");

    if (igBeginTable("##EmulationSettingsSpeed", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##EmulationSettingsSpeedLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##EmulationSettingsSpeedValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // Fast Forward
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Fast Forward");

        igTableNextColumn();
        if (igCheckbox("##FastForward", &app->emulation.fast_forward)) {
            app_emulator_speed(app, app->emulation.fast_forward ? 0 : app->emulation.speed);
        }

        // Speed
        igBeginDisabled(app->emulation.fast_forward);
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Speed");

        igTableNextColumn();
        if (igCombo_Str_arr("##Speed", &speed, speed_names, array_length(speed_names), 0)) {
            app->emulation.speed = speed + 1;
            app->emulation.fast_forward = false;
            app_emulator_speed(app, app->emulation.speed);
        }
        igEndDisabled();

        igEndTable();
    }

    igSeparatorText("Backup Storage");

    if (igBeginTable("##EmulationSettingsBackupStorage", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##EmulationSettingsBackupStorageLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##EmulationSettingsBackupStorageValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // Backup Storage Auto-Detect
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Auto-Detect");

        igTableNextColumn();
        igCheckbox("##BackupStorageTypeAutoDetect", &app->emulation.backup_storage.autodetect);

        // Backup Storage Type
        igBeginDisabled(app->emulation.backup_storage.autodetect);
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Type");

        igTableNextColumn();
        igCombo_Str_arr("##BackupStorageType", (int *)&app->emulation.backup_storage.type, backup_storage_names, array_length(backup_storage_names), 0);
        igEndDisabled();

        igEndTable();
    }

    igSeparatorText("GPIO Devices");

    if (igBeginTable("##EmulationSettingsGPIODevices", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##EmulationSettingsGPIODevicesLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##EmulationSettingsGPIODevicesValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // GPIO Device Auto-Detect
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Auto-Detect");

        igTableNextColumn();
        igCheckbox("##GPIODeviceTypeAutoDetect", &app->emulation.gpio_device.autodetect);

        // GPIO Device Type
        igBeginDisabled(app->emulation.gpio_device.autodetect);
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Type");

        igTableNextColumn();
        igCombo_Str_arr("##GPIODeviceType", (int *)&app->emulation.gpio_device.type, gpio_device_names, array_length(gpio_device_names), 0);
        igEndDisabled();

        igEndTable();
    }

    igSeparatorText("Misc");

    if (igBeginTable("##EmulationSettingsMisc", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##EmulationSettingsMiscLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##EmulationSettingsMiscValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // Pause game when the window loses focus
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Auto-Pause");

        igTableNextColumn();
        igCheckbox("##AutoPause", &app->emulation.auto_pause);

#ifdef WITH_DEBUGGER
        // Pause game after a reset
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Pause Game After Reset");

        igTableNextColumn();
        igCheckbox("##PauseGameAfterReset", &app->emulation.pause_on_reset);
#endif

        igEndTable();
    }
}

static
void
app_win_settings_video(
    struct app *app
) {
    int32_t display_size;
    ImGuiViewport *vp;
    uint32_t i;

    vp = igGetMainViewport();

    igTextWrapped("Video Settings");
    igSpacing();
    igSeparator();
    igSpacing();

    igSeparatorText("Display");

    if (igBeginTable("##VideoSettingsDisplay", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##VideoSettingsDisplayLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##VideoSettingsDisplayValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // VSync
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("VSync");

        igTableNextColumn();
        if (igCheckbox("##VSync", &app->video.vsync)) {
            SDL_GL_SetSwapInterval(app->video.vsync);
        }

        // Display Size
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Display Size");

        igTableNextColumn();

        display_size = -1;
        for (i = 1; i < array_length(display_size_names) + 1; ++i) {
            if (vp->WorkSize.x == GBA_SCREEN_WIDTH * i * app->ui.scale && vp->WorkSize.y == GBA_SCREEN_HEIGHT * i * app->ui.scale) {
                display_size = i;
                break;
            }
        }

        if (igBeginCombo("##DisplaySize", display_size > 0 ? display_size_names[display_size - 1] : "<Other>", ImGuiComboFlags_None)) {
            for (i = 1; i < array_length(display_size_names) + 1; ++i) {
                bool is_selected;

                is_selected = (display_size == i);
                if (igSelectable_Bool(display_size_names[i - 1], is_selected, ImGuiSelectableFlags_None, (ImVec2){ 0.f, 0.f })) {
                    app->video.display_size = i;
                    app->ui.win.resize = true;
                    app->ui.win.resize_with_ratio = false;
                }

                if (is_selected) {
                    igSetItemDefaultFocus();
                }
            }
            igEndCombo();
        }

        // Aspect Ratio
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Aspect Ratio");

        igTableNextColumn();
        if (igCombo_Str_arr("##AspectRatio", (int *)&app->video.aspect_ratio, aspect_ratio_names, ASPECT_RATIO_LEN, 0)) {
            // Force a resize of the window if the "auto-resize" option is selected
            if (app->video.aspect_ratio == ASPECT_RATIO_RESIZE) {
                app->ui.win.resize = true;
                app->ui.win.resize_with_ratio = true;
                app->ui.win.resize_ratio = min(app->ui.game.width / ((float)GBA_SCREEN_WIDTH * app->ui.scale), app->ui.game.height / ((float)GBA_SCREEN_HEIGHT * app->ui.scale));
            }
        }

        igEndTable();
    }

    igSeparatorText("Filters");

    if (igBeginTable("##VideoSettingsFilters", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##VideoSettingsFiltersLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##VideoSettingsFiltersValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // Texture Filter
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Texture Filter");

        igTableNextColumn();
        if (igCombo_Str_arr("##TextureFilters", (int *)&app->video.texture_filter, texture_filters_names, TEXTURE_FILTER_LEN, 0)) {
            app_sdl_video_rebuild_pipeline(app);
        }

        // Color Filter
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Color Filter");

        igTableNextColumn();
        if (igCombo_Str_arr("##ColorFilter", (int *)&app->video.pixel_color_filter, pixel_color_filters_names, PIXEL_COLOR_FILTER_LEN, 0)) {
            app_sdl_video_rebuild_pipeline(app);
        }

        // Scaling Filter
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Scaling Filter");

        igTableNextColumn();
        if (igCombo_Str_arr("##ScalingFilter", (int *)&app->video.pixel_scaling_filter, pixel_scaling_filters_names, PIXEL_SCALING_FILTER_LEN, 0)) {
            app_sdl_video_rebuild_pipeline(app);
        }

        igEndTable();
    }
}

static
void
app_win_settings_audio(
    struct app *app
) {
    ImGuiViewport *vp;
    float level;

    vp = igGetMainViewport();
    level = app->audio.level * 100.f;

    igTextWrapped("Audio Settings");
    igSpacing();
    igSeparator();
    igSpacing();

    if (igBeginTable("##AudioSettings", 2, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
        igTableSetupColumn("##AudioSettingsLabel", ImGuiTableColumnFlags_WidthFixed, vp->WorkSize.x / 5.f, 0);
        igTableSetupColumn("##AudioSettingsValue", ImGuiTableColumnFlags_WidthStretch, 0.f, 0);

        // Mute
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Mute");

        igTableNextColumn();
        igCheckbox("##Mute", &app->audio.mute);

        // Audio level
        igBeginDisabled(app->audio.mute);
        igTableNextRow(ImGuiTableRowFlags_None, 0.f);
        igTableNextColumn();
        igTextWrapped("Audio Level");

        igTableNextColumn();
        if (igSliderFloat("##SoundLevel", &level, 0.0f, 100.0f, "%.0f%%", ImGuiSliderFlags_None)) {
            app->audio.level = max(0.0f, min(level / 100.f, 1.f));
        }
        igEndDisabled();

        igEndTable();
    }
}

static
void
app_win_settings_bindings_bind_keyboard(
    struct app *app,
    size_t bind
) {
    SDL_Keycode *keycode;
    char const *name;
    char label[128];
    size_t j;

    igTableNextRow(ImGuiTableRowFlags_None, 0.f);

    igTableNextColumn();
    igTextWrapped(binds_pretty_name[bind]);

    for (j = 0; j < 2; ++j) {
        keycode = (j == 0) ? &app->binds.keyboard[bind] : &app->binds.keyboard_alt[bind];
        name = SDL_GetKeyName(*keycode);

        if (keycode == app->ui.settings.keybindings_editor.keyboard_target) {
            snprintf(label, sizeof(label), ">> %s <<##BindingsSettingsKeyboard%zu", name ?: " ", bind * 10 + j);
        } else {
            snprintf(label, sizeof(label), "%s##BindingsSettingsKeyboard%zu", name ?: "", bind * 10 + j);
        }

        igTableNextColumn();
        if (igButton(label, (ImVec2){ -1.f, 0.f })) {
            app->ui.settings.keybindings_editor.keyboard_target = keycode;
            app->ui.settings.keybindings_editor.controller_target = NULL;
        }
    }
}

static
void
app_win_settings_bindings_bind_controller(
    struct app *app,
    size_t bind
) {
    SDL_GameControllerButton *button;
    char const *name;
    char label[128];
    size_t j;

    igTableNextRow(ImGuiTableRowFlags_None, 0.f);

    igTableNextColumn();
    igTextWrapped(binds_pretty_name[bind]);

    for (j = 0; j < 2; ++j) {
        button = (j == 0) ? &app->binds.controller[bind] : &app->binds.controller_alt[bind];
        name = SDL_GameControllerGetStringForButton(*button);

        if (button == app->ui.settings.keybindings_editor.controller_target) {
            snprintf(label, sizeof(label), ">> %s <<##BindingsSettingsController%zu", name ?: " ", bind * 10 + j);
        } else {
            snprintf(label, sizeof(label), "%s##BindingsSettingsController%zu", name ?: "", bind * 10 + j);
        }

        igTableNextColumn();
        if (igButton(label, (ImVec2){ -1.f, 0.f })) {
            app->ui.settings.keybindings_editor.controller_target = button;
            app->ui.settings.keybindings_editor.keyboard_target = NULL;
        }
    }
}

static
void
app_win_settings_bindings(
    struct app *app
) {
    size_t bind;

    igTextWrapped("Bindings");
    igSpacing();
    igSeparator();
    igSpacing();

    if (igBeginTabBar("##BindingsSettings", ImGuiTabBarFlags_None)) {
        if (igBeginTabItem("Keyboard", NULL, ImGuiTabItemFlags_None)) {

            igSeparatorText("GBA");

            if (igBeginTable("##BindingsSettingsKeyboardGBA", 3, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
                igTableSetupColumn("##BindingsSettingsKeyboardGBALabel", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsKeyboardGBABindMain", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsKeyboardGBABindAlt", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);

                for (bind = BIND_GBA_MIN; bind <= BIND_GBA_MAX; ++bind) {
                    app_win_settings_bindings_bind_keyboard(app, bind);
                }

                igEndTable();
            }

            igSeparatorText("Emulator");

            if (igBeginTable("##BindingsSettingsKeyboardEmulator", 3, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
                igTableSetupColumn("##BindingsSettingsKeyboardEmulatorLabel", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsKeyboardEmulatorBindMain", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsKeyboardEmulatorBindAlt", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);

                for (bind = BIND_EMULATOR_MIN; bind <= BIND_EMULATOR_MAX; ++bind) {
                    app_win_settings_bindings_bind_keyboard(app, bind);
                }
                igEndTable();
            }

            igEndTabItem();
        }
        if (igBeginTabItem("Controller", NULL, ImGuiTabItemFlags_None)) {

            igSeparatorText("GBA");

            if (igBeginTable("##BindingsSettingsControllerGBA", 3, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
                igTableSetupColumn("##BindingsSettingsControllerGBALabel", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsControllerGBABindMain", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsControllerGBABindAlt", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);

                for (bind = BIND_GBA_MIN; bind <= BIND_GBA_MAX; ++bind) {
                    app_win_settings_bindings_bind_controller(app, bind);
                }

                igEndTable();
            }

            igSeparatorText("Emulator");

            if (igBeginTable("##BindingsSettingsControllerEmulator", 3, ImGuiTableFlags_None, (ImVec2){ .x = 0.f, .y = 0.f }, 0.f)) {
                igTableSetupColumn("##BindingsSettingsControllerEmulatorLabel", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsControllerEmulatorBindMain", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);
                igTableSetupColumn("##BindingsSettingsControllerEmulatorBindAlt", ImGuiTableColumnFlags_WidthStretch, 1.f, 0);

                for (bind = BIND_EMULATOR_MIN; bind <= BIND_EMULATOR_MAX; ++bind) {
                    app_win_settings_bindings_bind_controller(app, bind);
                }
                igEndTable();
            }

            igEndTabItem();
        }
        igEndTabBar();
    }
}

static void (*menu_callbacks[MENU_MAX])(struct app *) = {
    [MENU_EMULATION] =              &app_win_settings_emulation,
    [MENU_VIDEO] =                  &app_win_settings_video,
    [MENU_AUDIO] =                  &app_win_settings_audio,
    [MENU_BINDINGS] =               &app_win_settings_bindings,
};

void
app_win_settings(
    struct app *app
) {
    ImGuiViewport *vp;

    vp = igGetMainViewport();

    igSetNextWindowPos(vp->WorkPos, ImGuiCond_Always, (ImVec2){0.f, 0.f});
    igSetNextWindowSize(vp->WorkSize, ImGuiCond_Always);

    if (igBegin(
        "Settings",
        NULL,
        ImGuiWindowFlags_None
          | ImGuiWindowFlags_NoMove
          | ImGuiWindowFlags_NoResize
          | ImGuiWindowFlags_AlwaysAutoResize
          | ImGuiWindowFlags_NoTitleBar
          | ImGuiWindowFlags_NoNavInputs
          | ImGuiWindowFlags_NoNavFocus
    )) {
        uint32_t i;

        if (igBeginChild_Str("##SettingsMenu", (ImVec2){ vp->WorkSize.x / 4.f, 0.f}, ImGuiChildFlags_Border , ImGuiWindowFlags_None)) {
            for (i = 0; i < MENU_MAX; ++i) {
                if (igSelectable_Bool(menu_names[i], app->ui.settings.menu == i, ImGuiSelectableFlags_None, (ImVec2){ 0.f, 0.f})) {
                    app->ui.settings.menu = i;
                }
            }
            igEndChild();
        }

        igSameLine(0.0f, -1.0f);

        igBeginGroup();

        if (igBeginChild_Str("##SettingsVariables", (ImVec2){ 0.f, -igGetFrameHeightWithSpacing()}, ImGuiChildFlags_Border, ImGuiWindowFlags_None)) {
            if (menu_callbacks[app->ui.settings.menu]) {
                menu_callbacks[app->ui.settings.menu](app);
            }
            igEndChild();
        }

        if (igButton("Close", (ImVec2){ 0.f, 0.f})) {
            app->ui.settings.open = false;
        }

        igEndGroup();

        igEnd();
    }
}
