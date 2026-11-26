// This file is included multiple times by QMK's RGB matrix implementation.
RGB_MATRIX_EFFECT(MANU_KEY_ZONES)
RGB_MATRIX_EFFECT(MANU_SNOW)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#define MANU_WHITE_LEVEL 26
#define MANU_ORANGE_RED 26
#define MANU_ORANGE_GREEN 6
#define MANU_RED_LEVEL 26

#define MANU_SNAKE_UNMAPPED 255
#define MANU_SNAKE_INTRO_MS 1000

#define MANU_SNOW_MAX_FLAKES 12
#define MANU_SNOW_SPAWN_MS 40
#define MANU_SNOW_FALL_MS 50

static uint8_t  manu_snake_step[RGB_MATRIX_LED_COUNT];
static uint8_t  manu_snake_total;
static uint32_t manu_snake_start;
static bool     manu_snake_map_built;

typedef struct {
    uint8_t row;
    uint8_t col;
    bool    active;
} manu_snowflake_t;

static bool             manu_snow_settled[RGB_MATRIX_LED_COUNT];
static manu_snowflake_t manu_snow_flakes[MANU_SNOW_MAX_FLAKES];
static uint32_t         manu_snow_last_spawn;
static uint32_t         manu_snow_last_fall;
static uint8_t          manu_snow_settled_count;
static uint8_t          manu_snow_total_leds;
static uint8_t          manu_col_count_row0;
static uint8_t          manu_row0_cols[MATRIX_COLS];
static bool             manu_snow_layout_built;

static bool manu_is_white_key(uint8_t index) {
    return (index >= 16 && index <= 28) || // Number and symbol row
           (index >= 32 && index <= 44) || // Q through backslash
           (index >= 47 && index <= 57) || // A through quote
           (index >= 61 && index <= 70) || // Z through slash
           index == 72 ||                  // Up
           (index >= 81 && index <= 83);   // Left, Down, Right
}

static void manu_build_snake_map(void) {
    if (manu_snake_map_built) {
        return;
    }

    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        manu_snake_step[i] = MANU_SNAKE_UNMAPPED;
    }

    manu_snake_total = 0;
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        if ((row & 1) == 0) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                uint8_t led_index = g_led_config.matrix_co[row][col];
                if (led_index == NO_LED) {
                    continue;
                }
                manu_snake_step[led_index] = manu_snake_total++;
            }
        } else {
            for (uint8_t col = MATRIX_COLS; col > 0; col--) {
                uint8_t led_index = g_led_config.matrix_co[row][col - 1];
                if (led_index == NO_LED) {
                    continue;
                }
                manu_snake_step[led_index] = manu_snake_total++;
            }
        }
    }

    manu_snake_map_built = true;
}

static void manu_build_snow_layout(void) {
    if (manu_snow_layout_built) {
        return;
    }

    manu_snow_total_leds = 0;
    manu_col_count_row0  = 0;

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        if (g_led_config.matrix_co[0][col] != NO_LED) {
            manu_row0_cols[manu_col_count_row0++] = col;
        }
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (g_led_config.matrix_co[row][col] != NO_LED) {
                manu_snow_total_leds++;
            }
        }
    }

    manu_snow_layout_built = true;
}

static uint8_t manu_matrix_led(uint8_t row, uint8_t col) {
    return g_led_config.matrix_co[row][col];
}

static bool manu_find_led_below(uint8_t row, uint8_t col, uint8_t *out_row, uint8_t *out_col) {
    for (uint8_t r = row + 1; r < MATRIX_ROWS; r++) {
        if (g_led_config.matrix_co[r][col] != NO_LED) {
            *out_row = r;
            *out_col = col;
            return true;
        }
    }
    return false;
}

static bool manu_snow_cell_occupied(uint8_t row, uint8_t col) {
    uint8_t led = manu_matrix_led(row, col);
    if (led != NO_LED && manu_snow_settled[led]) {
        return true;
    }

    for (uint8_t i = 0; i < MANU_SNOW_MAX_FLAKES; i++) {
        manu_snowflake_t *f = &manu_snow_flakes[i];
        if (f->active && f->row == row && f->col == col) {
            return true;
        }
    }

    return false;
}

static void manu_snow_settle_led_at(uint8_t row, uint8_t col) {
    uint8_t led = manu_matrix_led(row, col);

    if (led != NO_LED && !manu_snow_settled[led]) {
        manu_snow_settled[led] = true;
        manu_snow_settled_count++;
    }
}

static void manu_snow_settle_flake(uint8_t flake_idx) {
    manu_snowflake_t *f = &manu_snow_flakes[flake_idx];

    manu_snow_settle_led_at(f->row, f->col);
    f->active = false;
}

static void manu_snow_reset(void) {
    memset(manu_snow_settled, 0, sizeof(manu_snow_settled));

    for (uint8_t i = 0; i < MANU_SNOW_MAX_FLAKES; i++) {
        manu_snow_flakes[i].active = false;
    }

    manu_snow_settled_count = 0;
    manu_snow_last_spawn    = g_rgb_timer;
    manu_snow_last_fall     = g_rgb_timer;
}

static void manu_snow_try_spawn(void) {
    if (manu_snow_settled_count >= manu_snow_total_leds || manu_col_count_row0 == 0) {
        return;
    }

    int8_t slot = -1;
    for (uint8_t i = 0; i < MANU_SNOW_MAX_FLAKES; i++) {
        if (!manu_snow_flakes[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return;
    }

    for (uint8_t attempt = 0; attempt < 6; attempt++) {
        uint8_t col = manu_row0_cols[random8() % manu_col_count_row0];
        if (manu_snow_cell_occupied(0, col)) {
            continue;
        }

        manu_snow_flakes[slot].row    = 0;
        manu_snow_flakes[slot].col    = col;
        manu_snow_flakes[slot].active = true;
        return;
    }
}

static void manu_snow_tick_fall(void) {
    for (uint8_t i = 0; i < MANU_SNOW_MAX_FLAKES; i++) {
        manu_snowflake_t *f = &manu_snow_flakes[i];
        if (!f->active) {
            continue;
        }

        uint8_t next_row;
        uint8_t next_col;
        if (!manu_find_led_below(f->row, f->col, &next_row, &next_col)) {
            manu_snow_settle_flake(i);
            continue;
        }

        if (manu_snow_cell_occupied(next_row, next_col)) {
            manu_snow_settle_flake(i);
            continue;
        }

        manu_snow_settle_led_at(f->row, f->col);
        f->row = next_row;
        f->col = next_col;

        if (f->row == (MATRIX_ROWS - 1)) {
            manu_snow_settle_flake(i);
        }
    }
}

static void manu_set_zone_color(uint8_t index) {
    if (index == 77) {
        rgb_matrix_set_color(index, 0, 0, 0);
    } else if (index == 0 || index == 58) {
        rgb_matrix_set_color(index, MANU_RED_LEVEL, 0, 0);
    } else if (manu_is_white_key(index)) {
        rgb_matrix_set_color(index, MANU_WHITE_LEVEL, MANU_WHITE_LEVEL, MANU_WHITE_LEVEL);
    } else {
        rgb_matrix_set_color(index, MANU_ORANGE_RED, MANU_ORANGE_GREEN, 0);
    }
}

static bool MANU_KEY_ZONES(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    manu_build_snake_map();

    if (params->init) {
        manu_snake_start = g_rgb_timer;
    }

    uint32_t elapsed = g_rgb_timer - manu_snake_start;
    uint16_t step_ms = (manu_snake_total > 0) ? (uint16_t)(MANU_SNAKE_INTRO_MS / manu_snake_total) : MANU_SNAKE_INTRO_MS;
    if (step_ms < 1) {
        step_ms = 1;
    }

    uint8_t active_keys = (elapsed / step_ms > 255) ? 255 : (uint8_t)(elapsed / step_ms);
    bool    intro       = active_keys < manu_snake_total;

    for (uint8_t i = led_min; i < led_max; i++) {
        uint8_t step = manu_snake_step[i];
        if (intro && step != MANU_SNAKE_UNMAPPED && step > active_keys) {
            rgb_matrix_set_color(i, 0, 0, 0);
            continue;
        }
        manu_set_zone_color(i);
    }

    return rgb_matrix_check_finished_leds(led_max);
}

static bool MANU_SNOW(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    manu_build_snow_layout();

    if (params->init) {
        manu_snow_reset();
    }

    if (manu_snow_settled_count < manu_snow_total_leds) {
        uint32_t now = g_rgb_timer;

        if ((now - manu_snow_last_spawn) >= MANU_SNOW_SPAWN_MS) {
            manu_snow_last_spawn = now;
            manu_snow_try_spawn();
        }

        if ((now - manu_snow_last_fall) >= MANU_SNOW_FALL_MS) {
            manu_snow_last_fall = now;
            manu_snow_tick_fall();
        }
    }

    for (uint8_t i = led_min; i < led_max; i++) {
        bool lit = manu_snow_settled[i];

        if (!lit) {
            for (uint8_t f = 0; f < MANU_SNOW_MAX_FLAKES; f++) {
                manu_snowflake_t *flake = &manu_snow_flakes[f];
                if (!flake->active) {
                    continue;
                }
                if (manu_matrix_led(flake->row, flake->col) == i) {
                    lit = true;
                    break;
                }
            }
        }

        if (lit) {
            manu_set_zone_color(i);
        } else {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }

    return rgb_matrix_check_finished_leds(led_max);
}

#endif