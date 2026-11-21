// This file is included multiple times by QMK's RGB matrix implementation.
RGB_MATRIX_EFFECT(MANU_KEY_ZONES)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#define MANU_WHITE_LEVEL 26
#define MANU_ORANGE_RED 26
#define MANU_ORANGE_GREEN 6
#define MANU_RED_LEVEL 26

static bool manu_is_white_key(uint8_t index) {
    return (index >= 16 && index <= 28) || // Number and symbol row
           (index >= 32 && index <= 44) || // Q through backslash
           (index >= 47 && index <= 57) || // A through quote
           (index >= 61 && index <= 70) || // Z through slash
           index == 72 ||                  // Up
           (index >= 81 && index <= 83);   // Left, Down, Right
}

static bool MANU_KEY_ZONES(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    for (uint8_t i = led_min; i < led_max; i++) {
        if (i == 77) {
            // Space
            rgb_matrix_set_color(i, 0, 0, 0);
        } else if (i == 0 || i == 58) {
            // Esc and Enter
            rgb_matrix_set_color(i, MANU_RED_LEVEL, 0, 0);
        } else if (manu_is_white_key(i)) {
            rgb_matrix_set_color(i, MANU_WHITE_LEVEL, MANU_WHITE_LEVEL, MANU_WHITE_LEVEL);
        } else {
            // Function, modifier, and other control keys
            rgb_matrix_set_color(i, MANU_ORANGE_RED, MANU_ORANGE_GREEN, 0);
        }
    }

    return rgb_matrix_check_finished_leds(led_max);
}

#endif
