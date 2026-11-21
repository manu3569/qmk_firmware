/* Copyright 2024 ~ 2026 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "keychron_common.h"

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
};

enum tap_dance_keycodes {
    TD_A_UMLAUT,
    TD_O_UMLAUT,
    TD_U_UMLAUT,
};

#define UMLAUT_HOLD_TERM 500

#define FN_MAC MO(MAC_FN)
#define FN_WIN MO(WIN_FN)

typedef struct {
    uint16_t base_keycode;
    uint32_t lowercase_codepoint;
    uint32_t uppercase_codepoint;
    uint16_t pressed_at;
    bool output_emitted;
} umlaut_tap_dance_t;

static umlaut_tap_dance_t umlaut_a = {KC_A, 0x00E4, 0x00C4, 0, false};
static umlaut_tap_dance_t umlaut_o = {KC_O, 0x00F6, 0x00D6, 0, false};
static umlaut_tap_dance_t umlaut_u = {KC_U, 0x00FC, 0x00DC, 0, false};

static bool umlaut_shifted_at_press(tap_dance_state_t *state) {
    uint8_t mods = state->weak_mods;
#ifndef NO_ACTION_ONESHOT
    mods |= state->oneshot_mods;
#endif
    return (mods & MOD_MASK_SHIFT) != 0;
}

static void umlaut_tap_letter(umlaut_tap_dance_t *umlaut, tap_dance_state_t *state) {
    uint16_t code = umlaut_shifted_at_press(state) ? S(umlaut->base_keycode) : umlaut->base_keycode;
    tap_code16(code);
}

static void umlaut_on_each_tap(tap_dance_state_t *state, void *user_data) {
    umlaut_tap_dance_t *umlaut = (umlaut_tap_dance_t *)user_data;

    (void)state;
    umlaut->pressed_at     = timer_read();
    umlaut->output_emitted = false;
}

static void umlaut_on_dance_finished(tap_dance_state_t *state, void *user_data) {
    umlaut_tap_dance_t *umlaut = (umlaut_tap_dance_t *)user_data;
    bool                shifted = umlaut_shifted_at_press(state);

    if (timer_elapsed(umlaut->pressed_at) >= UMLAUT_HOLD_TERM) {
        register_unicode(shifted ? umlaut->uppercase_codepoint : umlaut->lowercase_codepoint);
        umlaut->output_emitted = true;
    } else if (state->interrupted) {
        umlaut_tap_letter(umlaut, state);
        umlaut->output_emitted = true;
    }
}

static void umlaut_on_each_release(tap_dance_state_t *state, void *user_data) {
    umlaut_tap_dance_t *umlaut = (umlaut_tap_dance_t *)user_data;
    bool                shifted = umlaut_shifted_at_press(state);

    if (!umlaut->output_emitted) {
        if (timer_elapsed(umlaut->pressed_at) >= UMLAUT_HOLD_TERM) {
            register_unicode(shifted ? umlaut->uppercase_codepoint : umlaut->lowercase_codepoint);
        } else {
            umlaut_tap_letter(umlaut, state);
        }
    }

    // Finish immediately on release so short taps do not wait for the hold threshold.
    state->finished = true;
}

tap_dance_action_t tap_dance_actions[] = {
    [TD_A_UMLAUT] = {
        .fn        = {.on_each_tap = umlaut_on_each_tap, .on_dance_finished = umlaut_on_dance_finished, .on_each_release = umlaut_on_each_release},
        .user_data = &umlaut_a,
    },
    [TD_O_UMLAUT] = {
        .fn        = {.on_each_tap = umlaut_on_each_tap, .on_dance_finished = umlaut_on_dance_finished, .on_each_release = umlaut_on_each_release},
        .user_data = &umlaut_o,
    },
    [TD_U_UMLAUT] = {
        .fn        = {.on_each_tap = umlaut_on_each_tap, .on_dance_finished = umlaut_on_dance_finished, .on_each_release = umlaut_on_each_release},
        .user_data = &umlaut_u,
    },
};

uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    (void)record;

    switch (keycode) {
        case TD(TD_A_UMLAUT):
        case TD(TD_O_UMLAUT):
        case TD(TD_U_UMLAUT):
            return UMLAUT_HOLD_TERM;
        default:
            return TAPPING_TERM;
    }
}

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_ansi_84(
        KC_ESC,              KC_BRID,  KC_BRIU,  KC_MCTRL, KC_LNPAD, UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  KC_SNAP,  KC_DEL,   UG_NEXT,
        KC_GRV,              KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,
        KC_TAB,              KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     TD(TD_U_UMLAUT), KC_I, TD(TD_O_UMLAUT), KC_P,    KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,
        MT(MOD_LCTL, KC_ESC), TD(TD_A_UMLAUT),   KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,             KC_HOME,
        KC_LSFT,                       KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,  KC_UP,    KC_END,
        KC_LCTL,              KC_LOPTN, KC_LCMMD,                               KC_SPC,                                 KC_RCMMD, FN_MAC,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [MAC_FN] = LAYOUT_ansi_84(
        _______,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,     KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,   _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  _______,   _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        _______,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  _______,   _______,  _______,  _______,  _______,  _______,            _______,            _______,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,   _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,
        _______,  _______,  _______,                                _______,                                  _______,  _______,  _______,  _______,  _______,  _______),

    [WIN_BASE] = LAYOUT_ansi_84(
        KC_ESC,              KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   KC_PSCR,  KC_DEL,   UG_NEXT,
        KC_GRV,              KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,
        KC_TAB,              KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     TD(TD_U_UMLAUT), KC_I, TD(TD_O_UMLAUT), KC_P,    KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,
        MT(MOD_LCTL, KC_ESC), TD(TD_A_UMLAUT),   KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,             KC_HOME,
        KC_LSFT,                       KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,  KC_UP,    KC_END,
        KC_LCTL,              KC_LGUI,  KC_LALT,                                KC_SPC,                                 KC_RALT,  FN_WIN,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [WIN_FN] = LAYOUT_ansi_84(
        _______,  KC_BRID,  KC_BRIU,  KC_TASK,  KC_FILE,  UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,
        _______,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  _______,  _______,  _______,  _______,  _______,  _______,            _______,            _______,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,  _______,  _______)
};
