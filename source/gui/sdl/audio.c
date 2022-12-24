/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2023 - The Hades Authors
**
\******************************************************************************/

#include "hades.h"
#include "app.h"
#include "gui/gui.h"
#include "gba/gba.h"

/*
** Should be called roughly 23/24 times per second (48000 / 2048, see the the values in `gui_sdl_audio_init()`).
**
** We transfer the data contained in the apu_rbuffer to the SDL.
*/
static
void
gui_sdl_audio_callback(
    void *raw_app,
    uint8_t *raw_stream,
    int raw_stream_len
) {
    struct app *app;
    struct gba *gba;
    int16_t *stream;
    size_t len;
    size_t i;

    app = raw_app;
    gba = app->emulation.gba;
    stream = (int16_t *)raw_stream;
    len = raw_stream_len / (2 * sizeof(*stream));

    pthread_mutex_lock(&gba->apu.frontend_channels_mutex);
    for (i = 0; i < len; ++i) {
        uint32_t val;
        int16_t left;
        int16_t right;

        val = apu_rbuffer_pop(&gba->apu.frontend_channels);
        left = (int16_t)((val >> 16) & 0xFFFF);
        right = (int16_t)(val & 0xFFFF);

        stream[0] = (int16_t)(left * !app->audio.mute * app->audio.level);
        stream[1] = (int16_t)(right * !app->audio.mute * app->audio.level);
        stream += 2;
    }
    pthread_mutex_unlock(&gba->apu.frontend_channels_mutex);
}

void
gui_sdl_audio_init(
    struct app *app
) {
    SDL_AudioSpec want;
    SDL_AudioSpec have;

    want.freq = 48000;
    want.samples = 2048;
    want.format = AUDIO_S16;
    want.channels = 2;
    want.callback = gui_sdl_audio_callback;
    want.userdata = app;

    app->sdl.audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

    if (!app->sdl.audio_device) {
        logln(HS_ERROR, "Failed to initialize the audio device: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    gba_send_audio_resample_freq(app->emulation.gba, CYCLES_PER_SECOND / have.freq);

    SDL_PauseAudioDevice(app->sdl.audio_device, SDL_FALSE);
}

void
gui_sdl_audio_cleanup(
    struct app *app
) {
    SDL_CloseAudioDevice(app->sdl.audio_device);
}
