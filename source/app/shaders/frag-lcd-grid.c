/******************************************************************************\
**
**  This file is part of the Hades GBA Emulator, and is made available under
**  the terms of the GNU General Public License version 2.
**
**  Copyright (C) 2021-2024 - The Hades Authors
**
\******************************************************************************/

#include "app/app.h"

/*
** LCD Grid effect.
*/
char const *SHADER_FRAG_LCD_GRID = GLSL(
    layout(location = 0) out vec4 frag_color;

    in vec2 v_uv;

    uniform sampler2D u_screen_map;

    void main() {
        vec4 color = texture(u_screen_map, v_uv);
        vec4 lcd = vec4(1.0);

        int offset_x = int(mod(gl_FragCoord.x, 3.0));
        int offset_y = int(mod(gl_FragCoord.y, 3.0));

        if (offset_x == 0 || offset_y == 0) {
            lcd = vec4(0.8);
        }

        frag_color = color * lcd;
        frag_color.a = 1.0f;
    }
);
