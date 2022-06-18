/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2022 - The Hades Authors
**
\******************************************************************************/

#ifndef GBA_GBA_H
# define GBA_GBA_H

# include <stdatomic.h>
# include "gba/core.h"
# include "gba/memory.h"
# include "gba/ppu.h"
# include "gba/io.h"
# include "gba/apu.h"
# include "gba/scheduler.h"
# include "gba/gpio.h"

enum gba_states {
    GBA_STATE_PAUSE = 0,
    GBA_STATE_RUN,
};

enum device_states {
    DEVICE_AUTO_DETECT = 0,
    DEVICE_ENABLED,
    DEVICE_DISABLED,
};

enum message_types {
    MESSAGE_EXIT,
    MESSAGE_BIOS,
    MESSAGE_ROM,
    MESSAGE_BACKUP,
    MESSAGE_BACKUP_TYPE,
    MESSAGE_SPEED,
    MESSAGE_RESET,
    MESSAGE_RUN,
    MESSAGE_PAUSE,
    MESSAGE_KEYINPUT,
    MESSAGE_QUICKLOAD,
    MESSAGE_QUICKSAVE,
    MESSAGE_AUDIO_RESAMPLE_FREQ,
    MESSAGE_COLOR_CORRECTION,
    MESSAGE_RTC,
};

enum keyinput {
    KEY_A,
    KEY_B,
    KEY_L,
    KEY_R,
    KEY_UP,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_START,
    KEY_SELECT,
};

struct message {
    enum message_types type;
    size_t size;
};

struct message_keyinput {
    struct message super;
    enum keyinput key;
    bool pressed;
};

struct message_backup_type {
    struct message super;
    enum backup_storage_types type;
};

struct message_speed {
    struct message super;
    uint32_t speed; // 0 means unbounded (no fps cap).
};

struct message_data {
    struct message super;
    uint8_t *data;
    size_t size;
    void (*cleanup)(void *);
};

struct message_audio_freq {
    struct message super;
    uint64_t resample_frequency;
};

struct message_color_correction {
    struct message super;
    bool color_correction;
};

struct message_device_state {
    struct message super;
    enum device_states state;
};

struct message_queue {
    struct message *messages;
    size_t length;
    size_t allocated_size;

    pthread_mutex_t lock;
};

struct game_entry;

struct gba {
    enum gba_states state;
    uint32_t speed;

    struct core core;
    struct memory memory;
    struct io io;
    struct ppu ppu;
    struct apu apu;
    struct scheduler scheduler;
    struct gpio gpio;

    /* Entry in the game database, if it exists. */
    struct game_entry *game_entry;

    /* Set to true when the emulation is started. Used to lock some options like backup type. */
    bool started;

    /* Stores if color correction is enabled. */
    bool color_correction;

    /* Stores the RTC-related settimgs */
    bool rtc_auto_detect;
    bool rtc_enabled;

    /* The message queue used by the frontend to communicate with the emulator. */
    struct message_queue message_queue;

    /* The emulator's screen as it is being rendered. */
    uint32_t framebuffer[GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT];

    /* The emulator's screen, refreshed each frame, used by the frontend */
    uint32_t framebuffer_frontend[GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT];
    pthread_mutex_t framebuffer_frontend_mutex;

    /* The frame counter, used for FPS calculations. */
    atomic_uint framecounter;
};

/* gba/gba.c */
void gba_init(struct gba *gba);
void gba_main_loop(struct gba *gba);
void gba_send_exit(struct gba *gba);
void gba_send_bios(struct gba *gba, uint8_t *data, void (*cleanup)(void *));
void gba_send_rom(struct gba *gba, uint8_t *data, size_t size, void (*cleanup)(void *));
void gba_send_backup(struct gba *gba, uint8_t *data, size_t size, void (*cleanup)(void *));
void gba_send_backup_type(struct gba *gba, enum backup_storage_types backup_type);
void gba_send_reset(struct gba *gba);
void gba_send_speed(struct gba *gba, uint32_t speed);
void gba_send_run(struct gba *gba);
void gba_send_pause(struct gba *gba);
void gba_send_keyinput(struct gba *gba, enum keyinput key, bool pressed);
void gba_send_quickload(struct gba *gba, char const *path);
void gba_send_quicksave(struct gba *gba, char const *path);
void gba_send_audio_resample_freq(struct gba *gba, uint64_t resample_freq);
void gba_send_color_correction(struct gba *gba, bool color_correction);
void gba_send_rtc(struct gba *gba, enum device_states state);

#endif /* GBA_GBA_H */