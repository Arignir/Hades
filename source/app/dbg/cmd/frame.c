/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2024 - The Hades Authors
**
\******************************************************************************/

#include "hades.h"
#include "app/app.h"
#include "app/dbg.h"

void
debugger_cmd_frame(
    struct app *app,
    size_t argc __unused,
    struct arg const *argv __unused
) {
    if (!app->debugger.is_started) {
        logln(HS_ERROR, "%s%s%s", g_red, "This command cannot be used when no game is running.", g_reset);
        return;
    }

    if (argc == 0) {
        app_emulator_frame(app, 1);
        debugger_wait_for_emulator(app);
        debugger_dump_context_auto(app);
    } else if (argc == 1) {
        if (debugger_check_arg_type(CMD_FRAME, &argv[0], ARGS_INTEGER)) {
            return;
        }

        app_emulator_frame(app, argv[0].value.i64);
        debugger_wait_for_emulator(app);
        debugger_dump_context_auto(app);
    } else {
        printf("Usage: %s\n", g_commands[CMD_FRAME].usage);
        return;
    }
}
