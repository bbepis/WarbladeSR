// controls.c: Key bindings: key and button names, scan-code translation, duplicate fixing, the
// input configuration menu, the on-screen keyboard.
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "globals.h"
#include "game.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// A browser receives key-up events only after the game yields. Tight polling
// prevents release events from arriving and freezes splash/menu/game-over exits.
void WaitForKeyRelease(int key)
{
    while (KeyDown(key)) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(1);
#endif
    }
}

// ConfigInputMenu's cursor rows: a device-select row plus 8 binding rows for each player.
enum { NUM_CONFIG_ROWS = 18, P1_DEVICE_ROW = 0, P2_DEVICE_ROW = 9 };

// Maps a virtual key code to the uppercase letter/digit/punctuation it enters for the
// on-screen serial/name keyboard; returns 0 for keys with no mapping. Digits 0-9 and
// '.' each have two key codes mapped to them (e.g. main row and numpad).
char KeyToChar(int key)
{
    char c = 0;

    // letters A-Z
    if (key == K_VK_A) c = 'A';
    if (key == K_VK_B) c = 'B';
    if (key == K_VK_C) c = 'C';
    if (key == K_VK_D) c = 'D';
    if (key == K_VK_E) c = 'E';
    if (key == K_VK_F) c = 'F';
    if (key == K_VK_G) c = 'G';
    if (key == K_VK_H) c = 'H';
    if (key == K_VK_I) c = 'I';
    if (key == K_VK_J) c = 'J';
    if (key == K_VK_K) c = 'K';
    if (key == K_VK_L) c = 'L';

    if (key == K_VK_M) c = 'M';
    if (key == K_VK_N) c = 'N';
    if (key == K_VK_O) c = 'O';
    if (key == K_VK_P) c = 'P';
    if (key == K_VK_Q) c = 'Q';
    if (key == K_VK_R) c = 'R';
    if (key == K_VK_S) c = 'S';
    if (key == K_VK_T) c = 'T';
    if (key == K_VK_U) c = 'U';
    if (key == K_VK_V) c = 'V';
    if (key == K_VK_W) c = 'W';
    if (key == K_VK_X) c = 'X';
    if (key == K_VK_Y) c = 'Y';
    if (key == K_VK_Z) c = 'Z';

    // digits 0-9, main row
    if (key == K_VK_0) c = '0';
    if (key == K_VK_1) c = '1';
    if (key == K_VK_2) c = '2';
    if (key == K_VK_3) c = '3';
    if (key == K_VK_4) c = '4';
    if (key == K_VK_5) c = '5';
    if (key == K_VK_6) c = '6';
    if (key == K_VK_7) c = '7';
    if (key == K_VK_8) c = '8';
    if (key == K_VK_9) c = '9';

    // punctuation
    if (key == K_VK_OEM_COMMA) c = ',';
    if (key == K_VK_DECIMAL) c = '.';
    if (key == K_VK_OEM_PERIOD) c = '.';

    // digits 0-9, numpad
    if (key == K_VK_NUM0) c = '0';
    if (key == K_VK_NUM1) c = '1';
    if (key == K_VK_NUM2) c = '2';
    if (key == K_VK_NUM3) c = '3';
    if (key == K_VK_NUM4) c = '4';
    if (key == K_VK_NUM5) c = '5';
    if (key == K_VK_NUM6) c = '6';
    if (key == K_VK_NUM7) c = '7';
    if (key == K_VK_NUM8) c = '8';
    if (key == K_VK_NUM9) c = '9';

    if (key == K_VK_SPACE) c = ' ';
    return c;
}

// Sets g_keyLatch[K_VK_A .. K_VK_A+35] (the on-screen keyboard's per-key "already
// handled this press" flags) all to 1.
void ResetFlags()
{
    (&g_keyLatch[K_VK_A])[0] = 1;
    (&g_keyLatch[K_VK_A])[1] = 1;
    (&g_keyLatch[K_VK_A])[2] = 1;
    (&g_keyLatch[K_VK_A])[3] = 1;
    (&g_keyLatch[K_VK_A])[4] = 1;
    (&g_keyLatch[K_VK_A])[5] = 1;
    (&g_keyLatch[K_VK_A])[6] = 1;
    (&g_keyLatch[K_VK_A])[7] = 1;
    (&g_keyLatch[K_VK_A])[8] = 1;
    (&g_keyLatch[K_VK_A])[9] = 1;

    (&g_keyLatch[K_VK_A])[10] = 1;
    (&g_keyLatch[K_VK_A])[11] = 1;
    (&g_keyLatch[K_VK_A])[12] = 1;
    (&g_keyLatch[K_VK_A])[13] = 1;
    (&g_keyLatch[K_VK_A])[14] = 1;
    (&g_keyLatch[K_VK_A])[15] = 1;
    (&g_keyLatch[K_VK_A])[16] = 1;
    (&g_keyLatch[K_VK_A])[17] = 1;
    (&g_keyLatch[K_VK_A])[18] = 1;
    (&g_keyLatch[K_VK_A])[19] = 1;

    (&g_keyLatch[K_VK_A])[20] = 1;
    (&g_keyLatch[K_VK_A])[21] = 1;
    (&g_keyLatch[K_VK_A])[22] = 1;
    (&g_keyLatch[K_VK_A])[23] = 1;
    (&g_keyLatch[K_VK_A])[24] = 1;
    (&g_keyLatch[K_VK_A])[25] = 1;
    (&g_keyLatch[K_VK_A])[26] = 1;
    (&g_keyLatch[K_VK_A])[27] = 1;
    (&g_keyLatch[K_VK_A])[28] = 1;
    (&g_keyLatch[K_VK_A])[29] = 1;

    (&g_keyLatch[K_VK_A])[30] = 1;
    (&g_keyLatch[K_VK_A])[31] = 1;
    (&g_keyLatch[K_VK_A])[32] = 1;
    (&g_keyLatch[K_VK_A])[33] = 1;
    (&g_keyLatch[K_VK_A])[34] = 1;
    (&g_keyLatch[K_VK_A])[35] = 1;
}

#define BV(off) device[((off) - 0x78) / 4]

// Swaps player 1's and player 2's key/joystick bindings (each device offset paired with the
// next one), bound to the "T" hotkey and menu item 175. `BV(off)` indexes g_cfg's per-device
// binding block by byte offset, the layout the original code addresses it with.
void SwapKeyBindings()
{
    int t;
    int t2;

    g_lastActivityTime = g_time;
    g_attractScreen = ATTRACT_HELP_CONTROLS;
    g_idleTimeoutMs = 15000;

    t2 = g_cfg.BV(0x78); g_cfg.BV(0x78) = g_cfg.BV(0x7c); g_cfg.BV(0x7c) = t2;
    t = g_cfg.BV(0x88); g_cfg.BV(0x88) = g_cfg.BV(0x8c); g_cfg.BV(0x8c) = t;
    t = g_cfg.BV(0x98); g_cfg.BV(0x98) = g_cfg.BV(0x9c); g_cfg.BV(0x9c) = t;
    t = g_cfg.BV(0xa8); g_cfg.BV(0xa8) = g_cfg.BV(0xac); g_cfg.BV(0xac) = t;
    t = g_cfg.BV(0xb8); g_cfg.BV(0xb8) = g_cfg.BV(0xbc); g_cfg.BV(0xbc) = t;
    t = g_cfg.BV(0xc8); g_cfg.BV(0xc8) = g_cfg.BV(0xcc); g_cfg.BV(0xcc) = t;
    t = g_cfg.BV(0xd8); g_cfg.BV(0xd8) = g_cfg.BV(0xdc); g_cfg.BV(0xdc) = t;
    t = g_cfg.BV(0xe8); g_cfg.BV(0xe8) = g_cfg.BV(0xec); g_cfg.BV(0xec) = t;
    t = g_cfg.BV(0x5f8); g_cfg.BV(0x5f8) = g_cfg.BV(0x5fc); g_cfg.BV(0x5fc) = t;
    t = g_cfg.BV(0x608); g_cfg.BV(0x608) = g_cfg.BV(0x60c); g_cfg.BV(0x60c) = t;
    t2 = g_cfg.BV(0x110); g_cfg.BV(0x110) = g_cfg.BV(0x114); g_cfg.BV(0x114) = t2;
    t2 = g_cfg.BV(0x120); g_cfg.BV(0x120) = g_cfg.BV(0x124); g_cfg.BV(0x124) = t2;
    t2 = g_cfg.BV(0x130); g_cfg.BV(0x130) = g_cfg.BV(0x134); g_cfg.BV(0x134) = t2;
    t2 = g_cfg.BV(0x618); g_cfg.BV(0x618) = g_cfg.BV(0x61c); g_cfg.BV(0x61c) = t2;
    t2 = g_cfg.BV(0x628); g_cfg.BV(0x628) = g_cfg.BV(0x62c); g_cfg.BV(0x62c) = t2;

    if (g_profileIndex != -1 && (g_gameMode == 0 || g_gameMode == 6) && g_playerUpdateFn != StateDemo) {
        UnpackAccount(g_profileIndex);
        g_hiscore = g_acc.settings.best;
        g_acc.settings = g_cfg;
        if (g_hiscore > g_acc.settings.best)
            g_acc.settings.best = g_hiscore;
        PackAccount(g_profileIndex);
        SaveAccount(g_profileIndex);
    }
    g_bindingsBannerUntil = g_time + 10000;
}

#undef BV

// Writes the display name of virtual key code `key` (game-internal key-code space,
// not a Windows VK_ code) into g_keyName, padded to a fixed width for menu layout.
void KeyName(int key)
{
    // Digits 0-9
    if (key == K_VK_0) {
        sprintf(g_keyName, "0            ");
    } else if (key == K_VK_1) {
        sprintf(g_keyName, "1            ");
    } else if (key == K_VK_2) {
        sprintf(g_keyName, "2            ");
    } else if (key == K_VK_3) {
        sprintf(g_keyName, "3            ");
    } else if (key == K_VK_4) {
        sprintf(g_keyName, "4            ");
    } else if (key == K_VK_5) {
        sprintf(g_keyName, "5            ");
    } else if (key == K_VK_6) {
        sprintf(g_keyName, "6            ");
    } else if (key == K_VK_7) {
        sprintf(g_keyName, "7            ");
    } else if (key == K_VK_8) {
        sprintf(g_keyName, "8            ");
    } else if (key == K_VK_9) {
        sprintf(g_keyName, "9            ");

    // Letters A-M
    } else if (key == K_VK_A) {
        sprintf(g_keyName, "A            ");
    } else if (key == K_VK_B) {
        sprintf(g_keyName, "B            ");
    } else if (key == K_VK_C) {
        sprintf(g_keyName, "C            ");
    } else if (key == K_VK_D) {
        sprintf(g_keyName, "D            ");
    } else if (key == K_VK_E) {
        sprintf(g_keyName, "E            ");
    } else if (key == K_VK_F) {
        sprintf(g_keyName, "F            ");

    } else if (key == K_VK_G) {
        sprintf(g_keyName, "G            ");
    } else if (key == K_VK_H) {
        sprintf(g_keyName, "H            ");
    } else if (key == K_VK_I) {
        sprintf(g_keyName, "I            ");
    } else if (key == K_VK_J) {
        sprintf(g_keyName, "J            ");
    } else if (key == K_VK_K) {
        sprintf(g_keyName, "K            ");
    } else if (key == K_VK_L) {
        sprintf(g_keyName, "L            ");
    } else if (key == K_VK_M) {
        sprintf(g_keyName, "M            ");

    // Letters N-Z
    } else if (key == K_VK_N) {
        sprintf(g_keyName, "N            ");
    } else if (key == K_VK_O) {
        sprintf(g_keyName, "O            ");
    } else if (key == K_VK_P) {
        sprintf(g_keyName, "P            ");
    } else if (key == K_VK_Q) {
        sprintf(g_keyName, "Q            ");
    } else if (key == K_VK_R) {
        sprintf(g_keyName, "R            ");
    } else if (key == K_VK_S) {
        sprintf(g_keyName, "S            ");

    } else if (key == K_VK_T) {
        sprintf(g_keyName, "T            ");
    } else if (key == K_VK_U) {
        sprintf(g_keyName, "U            ");
    } else if (key == K_VK_V) {
        sprintf(g_keyName, "V            ");
    } else if (key == K_VK_W) {
        sprintf(g_keyName, "W            ");
    } else if (key == K_VK_X) {
        sprintf(g_keyName, "X            ");
    } else if (key == K_VK_Y) {
        sprintf(g_keyName, "Y            ");
    } else if (key == K_VK_Z) {
        sprintf(g_keyName, "Z            ");

    // Cursor keys
    } else if (key == K_VK_UP) {
        sprintf(g_keyName, "CURSOR UP    ");
    } else if (key == K_VK_DOWN) {
        sprintf(g_keyName, "CURSOR DOWN  ");
    } else if (key == K_VK_LEFT) {
        sprintf(g_keyName, "CURSOR LEFT  ");
    } else if (key == K_VK_RIGHT) {
        sprintf(g_keyName, "CURSOR RIGHT ");

    // Modifier keys
    } else if (key == K_VK_L_CONTROL) {
        sprintf(g_keyName, "LEFT CTRL    ");
    } else if (key == K_VK_R_CONTROL) {
        sprintf(g_keyName, "RIGHT CTRL   ");
    } else if (key == K_VK_L_SHIFT) {
        sprintf(g_keyName, "LEFT SHIFT   ");
    } else if (key == K_VK_R_SHIFT) {
        sprintf(g_keyName, "RIGHT SHIFT  ");

    // Numpad keys
    } else if (key == K_VK_NUM0) {
        sprintf(g_keyName, "NUMPAD 0     ");
    } else if (key == K_VK_NUM1) {
        sprintf(g_keyName, "NUMPAD 1     ");
    } else if (key == K_VK_NUM2) {
        sprintf(g_keyName, "NUMPAD 2     ");
    } else if (key == K_VK_NUM3) {
        sprintf(g_keyName, "NUMPAD 3     ");
    } else if (key == K_VK_NUM4) {
        sprintf(g_keyName, "NUMPAD 4     ");
    } else if (key == K_VK_NUM5) {
        sprintf(g_keyName, "NUMPAD 5     ");
    } else if (key == K_VK_NUM6) {
        sprintf(g_keyName, "NUMPAD 6     ");
    } else if (key == K_VK_NUM7) {
        sprintf(g_keyName, "NUMPAD 7     ");
    } else if (key == K_VK_NUM8) {
        sprintf(g_keyName, "NUMPAD 8     ");
    } else if (key == K_VK_NUM9) {
        sprintf(g_keyName, "NUMPAD 9     ");

    // Remaining named keys
    } else if (key == K_VK_SPACE) {
        sprintf(g_keyName, "SPACE        ");
    } else if (key == K_VK_MENU) {
        sprintf(g_keyName, "MENU         ");
    } else if (key == K_VK_TAB) {
        sprintf(g_keyName, "TAB          ");
    } else if (key == K_VK_LWIN) {
        sprintf(g_keyName, "LEFT WINDOW  ");
    } else {
        sprintf(g_keyName, "UNDEFINED    ");
    }
}

// Writes the display name of `button` into g_keyName: a key name (via KeyName) when
// `type` is 0 (keyboard), otherwise a joystick button bitmask -> "BUTTON n" name.
void ButtonName(int type, int button)
{
    if (type == 0) {
        KeyName(button);
        return;
    }

    switch (button) {
    case 1: sprintf(g_keyName, "BUTTON 1"); break;
    case 2: sprintf(g_keyName, "BUTTON 2"); break;
    case 4: sprintf(g_keyName, "BUTTON 3"); break;
    case 8: sprintf(g_keyName, "BUTTON 4"); break;
    case 0x10: sprintf(g_keyName, "BUTTON 5"); break;
    case 0x20: sprintf(g_keyName, "BUTTON 6"); break;
    case 0x40: sprintf(g_keyName, "BUTTON 7"); break;
    case 0x80: sprintf(g_keyName, "BUTTON 8"); break;
    case 0x100: sprintf(g_keyName, "BUTTON 9"); break;
    case 0x200: sprintf(g_keyName, "BUTTON 10"); break;
    case 0x400: sprintf(g_keyName, "BUTTON 11"); break;
    case 0x800: sprintf(g_keyName, "BUTTON 12"); break;
    case 0x1000: sprintf(g_keyName, "BUTTON 13"); break;
    case 0x2000: sprintf(g_keyName, "BUTTON 14"); break;
    case 0x4000: sprintf(g_keyName, "BUTTON 15"); break;
    default: sprintf(g_keyName, "UNDEFINED"); break;
    }
}

// Resolves duplicate key/joystick-button bindings for `player`'s control scheme: for every
// pair of the player's bound actions that share the same key or button, the later one in
// the pair is reset to a default (K_VK_Q, i.e. Q, for keyboard; -1/unbound for
// joystick) and g_keysChanged is set. Keyboard and joystick bindings are checked separately
// based on the player's selected device.
void FixDuplicateKeys(int player)
{
    if ((&g_cfg.device0)[player] == DEVICE_KEYBOARD) {
        // Every pair of keyboard bindings, later one reset to Q (K_VK_Q) on a clash
        if (g_cfg.left[player] == g_cfg.right[player]) {
            g_cfg.right[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.left[player] == g_cfg.up[player]) {
            g_cfg.up[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.left[player] == g_cfg.down[player]) {
            g_cfg.down[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.left[player] == g_cfg.playerKeys[0][player]) {
            g_cfg.playerKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }

        if (g_cfg.left[player] == g_cfg.rocket[player]) {
            g_cfg.rocket[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.left[player] == g_cfg.key6[player]) {
            g_cfg.key6[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.left[player] == g_cfg.menuKeys[0][player]) {
            g_cfg.menuKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.left[player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }

        if (g_cfg.right[player] == g_cfg.up[player]) {
            g_cfg.up[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.right[player] == g_cfg.down[player]) {
            g_cfg.down[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.right[player] == g_cfg.playerKeys[0][player]) {
            g_cfg.playerKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.right[player] == g_cfg.rocket[player]) {
            g_cfg.rocket[player] = K_VK_Q;
            g_keysChanged = 1;
        }

        if (g_cfg.right[player] == g_cfg.key6[player]) {
            g_cfg.key6[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.right[player] == g_cfg.menuKeys[0][player]) {
            g_cfg.menuKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.right[player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }

        if (g_cfg.up[player] == g_cfg.down[player]) {
            g_cfg.down[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.up[player] == g_cfg.playerKeys[0][player]) {
            g_cfg.playerKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.up[player] == g_cfg.rocket[player]) {
            g_cfg.rocket[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.up[player] == g_cfg.key6[player]) {
            g_cfg.key6[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.up[player] == g_cfg.menuKeys[0][player]) {
            g_cfg.menuKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.up[player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }

        if (g_cfg.down[player] == g_cfg.playerKeys[0][player]) {
            g_cfg.playerKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.down[player] == g_cfg.rocket[player]) {
            g_cfg.rocket[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.down[player] == g_cfg.key6[player]) {
            g_cfg.key6[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.down[player] == g_cfg.menuKeys[0][player]) {
            g_cfg.menuKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.down[player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }

        if (g_cfg.playerKeys[0][player] == g_cfg.rocket[player]) {
            g_cfg.rocket[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.playerKeys[0][player] == g_cfg.key6[player]) {
            g_cfg.key6[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.playerKeys[0][player] == g_cfg.menuKeys[0][player]) {
            g_cfg.menuKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.playerKeys[0][player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }

        if (g_cfg.rocket[player] == g_cfg.key6[player]) {
            g_cfg.key6[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.rocket[player] == g_cfg.menuKeys[0][player]) {
            g_cfg.menuKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.rocket[player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.key6[player] == g_cfg.menuKeys[0][player]) {
            g_cfg.menuKeys[0][player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.key6[player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }
        if (g_cfg.menuKeys[0][player] == g_cfg.profile[player]) {
            g_cfg.profile[player] = K_VK_Q;
            g_keysChanged = 1;
        }
    } else {

        // Every pair of joystick bindings, later one unbound (-1) on a clash
        if (g_cfg.joy[0][player] == g_cfg.joyRocket[player]) {
            g_cfg.joyRocket[player] = -1;
            g_keysChanged = 1;
        }
        if (g_cfg.joy[0][player] == g_cfg.joyBtn2[player]) {
            g_cfg.joyBtn2[player] = -1;
            g_keysChanged = 1;
        }
        if (g_cfg.joy[0][player] == g_cfg.joyPause[player]) {
            g_cfg.joyPause[player] = -1;
            g_keysChanged = 1;
        }
        if (g_cfg.joy[0][player] == g_cfg.joyProfile[player]) {
            g_cfg.joyProfile[player] = -1;
            g_keysChanged = 1;
        }

        if (g_cfg.joyRocket[player] == g_cfg.joyBtn2[player]) {
            g_cfg.joyBtn2[player] = -1;
            g_keysChanged = 1;
        }
        if (g_cfg.joyRocket[player] == g_cfg.joyPause[player]) {
            g_cfg.joyPause[player] = -1;
            g_keysChanged = 1;
        }
        if (g_cfg.joyRocket[player] == g_cfg.joyProfile[player]) {
            g_cfg.joyProfile[player] = -1;
            g_keysChanged = 1;
        }

        if (g_cfg.joyBtn2[player] == g_cfg.joyPause[player]) {
            g_cfg.joyPause[player] = -1;
            g_keysChanged = 1;
        }
        if (g_cfg.joyBtn2[player] == g_cfg.joyProfile[player]) {
            g_cfg.joyProfile[player] = -1;
            g_keysChanged = 1;
        }
        if (g_cfg.joyPause[player] == g_cfg.joyProfile[player]) {
            g_cfg.joyProfile[player] = -1;
            g_keysChanged = 1;
        }
    }
}

// Strips leading and trailing whitespace from `s` in place (used to clean up the
// fixed-width names KeyName/ButtonName produce).
void TrimSpaces(char *s)
{
    char *p;
    if (s) {
        if (isspace(*s)) {
            p = s + 1;
            while (isspace(*p))
                p++;
            strcpy(s, p);
        }
        if (strlen(s) > 0) {
            p = s + strlen(s) - 1;
            while (p >= s && isspace(*p))
                p--;
            *++p = 0;
        }
    }
}

// Builds the "PLn: MOVE LEFT:.. MOVE RIGHT:.. FIRE:.. FIRE2:.." summary line for `player`
// into g_logBuf, from their current key/joystick-axis bindings.
void ControlsText(int player)
{
    char b1[128];
    char b2[128];
    char b3[128];
    char b4[128];

    if ((&g_cfg.device0)[player] == DEVICE_KEYBOARD) {
        ButtonName((&g_cfg.device0)[player], g_cfg.left[player]);
        sprintf(b1, "%s", g_keyName);
        ButtonName((&g_cfg.device0)[player], g_cfg.right[player]);
        sprintf(b2, "%s", g_keyName);
        ButtonName((&g_cfg.device0)[player], g_cfg.playerKeys[0][player]);
        sprintf(b3, "%s", g_keyName);
        ButtonName((&g_cfg.device0)[player], g_cfg.rocket[player]);
        sprintf(b4, "%s", g_keyName);
    } else {
        sprintf(b1, "X AXIS LEFT");
        sprintf(b2, "X AXIS RIGHT");
        ButtonName((&g_cfg.device0)[player], g_cfg.joy[0][player]);
        sprintf(b3, "%s", g_keyName);
        ButtonName((&g_cfg.device0)[player], g_cfg.joyRocket[player]);
        sprintf(b4, "%s", g_keyName);
    }

    TrimSpaces(b1);
    TrimSpaces(b2);
    TrimSpaces(b3);
    TrimSpaces(b4);
    sprintf(g_logBuf, "PL%d: MOVE LEFT:%s   MOVE RIGHT:%s   FIRE:%s   FIRE2:%s", player + 1, b1, b2, b3, b4);
}

// Scans the supported keys in a fixed priority order and, for the first one currently
// held, sets g_pressedKey to its game key code and g_keyName to its display name. Used
// while the "press a key" prompt is showing in ConfigInputMenu. Leaves g_pressedKey at -1
// if none of the supported keys are held.
void GetPressedKeyName()
{
    g_pressedKey = -1;

    // Digits 0-4
    if (KeyDown(K_VK_0)) {
        g_pressedKey = K_VK_0;
        sprintf(g_keyName, "0            ");
    }
    else if (KeyDown(K_VK_1)) {
        g_pressedKey = K_VK_1;
        sprintf(g_keyName, "1            ");
    }
    else if (KeyDown(K_VK_2)) {
        g_pressedKey = K_VK_2;
        sprintf(g_keyName, "2            ");
    }
    else if (KeyDown(K_VK_3)) {
        g_pressedKey = K_VK_3;
        sprintf(g_keyName, "3            ");
    }
    else if (KeyDown(K_VK_4)) {
        g_pressedKey = K_VK_4;
        sprintf(g_keyName, "4            ");
    }

    // Digits 5-9
    else if (KeyDown(K_VK_5)) {
        g_pressedKey = K_VK_5;
        sprintf(g_keyName, "5            ");
    }
    else if (KeyDown(K_VK_6)) {
        g_pressedKey = K_VK_6;
        sprintf(g_keyName, "6            ");
    }
    else if (KeyDown(K_VK_7)) {
        g_pressedKey = K_VK_7;
        sprintf(g_keyName, "7            ");
    }
    else if (KeyDown(K_VK_8)) {
        g_pressedKey = K_VK_8;
        sprintf(g_keyName, "8            ");
    }
    else if (KeyDown(K_VK_9)) {
        g_pressedKey = K_VK_9;
        sprintf(g_keyName, "9            ");
    }

    // Letters A-F
    else if (KeyDown(K_VK_A)) {
        g_pressedKey = K_VK_A;
        sprintf(g_keyName, "A            ");
    }
    else if (KeyDown(K_VK_B)) {
        g_pressedKey = K_VK_B;
        sprintf(g_keyName, "B            ");
    }
    else if (KeyDown(K_VK_C)) {
        g_pressedKey = K_VK_C;
        sprintf(g_keyName, "C            ");
    }
    else if (KeyDown(K_VK_D)) {
        g_pressedKey = K_VK_D;
        sprintf(g_keyName, "D            ");
    }
    else if (KeyDown(K_VK_E)) {
        g_pressedKey = K_VK_E;
        sprintf(g_keyName, "E            ");
    }
    else if (KeyDown(K_VK_F)) {
        g_pressedKey = K_VK_F;
        sprintf(g_keyName, "F            ");
    }

    // Letters G-L
    else if (KeyDown(K_VK_G)) {
        g_pressedKey = K_VK_G;
        sprintf(g_keyName, "G            ");
    }
    else if (KeyDown(K_VK_H)) {
        g_pressedKey = K_VK_H;
        sprintf(g_keyName, "H            ");
    }
    else if (KeyDown(K_VK_I)) {
        g_pressedKey = K_VK_I;
        sprintf(g_keyName, "I            ");
    }
    else if (KeyDown(K_VK_J)) {
        g_pressedKey = K_VK_J;
        sprintf(g_keyName, "J            ");
    }
    else if (KeyDown(K_VK_K)) {
        g_pressedKey = K_VK_K;
        sprintf(g_keyName, "K            ");
    }
    else if (KeyDown(K_VK_L)) {
        g_pressedKey = K_VK_L;
        sprintf(g_keyName, "L            ");
    }

    // Letters M-R
    else if (KeyDown(K_VK_M)) {
        g_pressedKey = K_VK_M;
        sprintf(g_keyName, "M            ");
    }
    else if (KeyDown(K_VK_N)) {
        g_pressedKey = K_VK_N;
        sprintf(g_keyName, "N            ");
    }
    else if (KeyDown(K_VK_O)) {
        g_pressedKey = K_VK_O;
        sprintf(g_keyName, "O            ");
    }
    else if (KeyDown(K_VK_P)) {
        g_pressedKey = K_VK_P;
        sprintf(g_keyName, "P            ");
    }
    else if (KeyDown(K_VK_Q)) {
        g_pressedKey = K_VK_Q;
        sprintf(g_keyName, "Q            ");
    }
    else if (KeyDown(K_VK_R)) {
        g_pressedKey = K_VK_R;
        sprintf(g_keyName, "R            ");
    }

    // Letters S-X
    else if (KeyDown(K_VK_S)) {
        g_pressedKey = K_VK_S;
        sprintf(g_keyName, "S            ");
    }
    else if (KeyDown(K_VK_T)) {
        g_pressedKey = K_VK_T;
        sprintf(g_keyName, "T            ");
    }
    else if (KeyDown(K_VK_U)) {
        g_pressedKey = K_VK_U;
        sprintf(g_keyName, "U            ");
    }
    else if (KeyDown(K_VK_V)) {
        g_pressedKey = K_VK_V;
        sprintf(g_keyName, "V            ");
    }
    else if (KeyDown(K_VK_W)) {
        g_pressedKey = K_VK_W;
        sprintf(g_keyName, "W            ");
    }
    else if (KeyDown(K_VK_X)) {
        g_pressedKey = K_VK_X;
        sprintf(g_keyName, "X            ");
    }

    // Letters Y-Z
    else if (KeyDown(K_VK_Y)) {
        g_pressedKey = K_VK_Y;
        sprintf(g_keyName, "Y            ");
    }
    else if (KeyDown(K_VK_Z)) {
        g_pressedKey = K_VK_Z;
        sprintf(g_keyName, "Z            ");
    }

    // Cursor keys
    else if (KeyDown(K_VK_UP)) {
        g_pressedKey = K_VK_UP;
        sprintf(g_keyName, "CURSOR UP    ");
    }
    else if (KeyDown(K_VK_DOWN)) {
        g_pressedKey = K_VK_DOWN;
        sprintf(g_keyName, "CURSOR DOWN  ");
    }
    else if (KeyDown(K_VK_LEFT)) {
        g_pressedKey = K_VK_LEFT;
        sprintf(g_keyName, "CURSOR LEFT  ");
    }
    else if (KeyDown(K_VK_RIGHT)) {
        g_pressedKey = K_VK_RIGHT;
        sprintf(g_keyName, "CURSOR RIGHT ");
    }

    // Modifier keys
    else if (KeyDown(K_VK_L_CONTROL)) {
        g_pressedKey = K_VK_L_CONTROL;
        sprintf(g_keyName, "LEFT CTRL    ");
    }
    else if (KeyDown(K_VK_R_CONTROL)) {
        g_pressedKey = K_VK_R_CONTROL;
        sprintf(g_keyName, "RIGHT CTRL   ");
    }
    else if (KeyDown(K_VK_L_SHIFT)) {
        g_pressedKey = K_VK_L_SHIFT;
        sprintf(g_keyName, "LEFT SHIFT   ");
    }
    else if (KeyDown(K_VK_R_SHIFT)) {
        g_pressedKey = K_VK_R_SHIFT;
        sprintf(g_keyName, "RIGHT SHIFT  ");
    }

    // Numpad 0-4
    else if (KeyDown(K_VK_NUM0)) {
        g_pressedKey = K_VK_NUM0;
        sprintf(g_keyName, "NUMPAD 0     ");
    }
    else if (KeyDown(K_VK_NUM1)) {
        g_pressedKey = K_VK_NUM1;
        sprintf(g_keyName, "NUMPAD 1     ");
    }
    else if (KeyDown(K_VK_NUM2)) {
        g_pressedKey = K_VK_NUM2;
        sprintf(g_keyName, "NUMPAD 2     ");
    }
    else if (KeyDown(K_VK_NUM3)) {
        g_pressedKey = K_VK_NUM3;
        sprintf(g_keyName, "NUMPAD 3     ");
    }
    else if (KeyDown(K_VK_NUM4)) {
        g_pressedKey = K_VK_NUM4;
        sprintf(g_keyName, "NUMPAD 4     ");
    }

    // Numpad 5-9
    else if (KeyDown(K_VK_NUM5)) {
        g_pressedKey = K_VK_NUM5;
        sprintf(g_keyName, "NUMPAD 5     ");
    }
    else if (KeyDown(K_VK_NUM6)) {
        g_pressedKey = K_VK_NUM6;
        sprintf(g_keyName, "NUMPAD 6     ");
    }
    else if (KeyDown(K_VK_NUM7)) {
        g_pressedKey = K_VK_NUM7;
        sprintf(g_keyName, "NUMPAD 7     ");
    }
    else if (KeyDown(K_VK_NUM8)) {
        g_pressedKey = K_VK_NUM8;
        sprintf(g_keyName, "NUMPAD 8     ");
    }
    else if (KeyDown(K_VK_NUM9)) {
        g_pressedKey = K_VK_NUM9;
        sprintf(g_keyName, "NUMPAD 9     ");
    }

    // Remaining named keys
    else if (KeyDown(K_VK_SPACE)) {
        g_pressedKey = K_VK_SPACE;
        sprintf(g_keyName, "SPACE        ");
    }
    else if (KeyDown(K_VK_MENU)) {
        g_pressedKey = K_VK_MENU;
        sprintf(g_keyName, "MENU         ");
    }
    else if (KeyDown(K_VK_TAB)) {
        g_pressedKey = K_VK_TAB;
        sprintf(g_keyName, "TAB          ");
    }
    else if (KeyDown(K_VK_LWIN)) {
        g_pressedKey = K_VK_LWIN;
        sprintf(g_keyName, "LEFT WINDOWS ");  // NOTE: KeyName() spells this "LEFT WINDOW" for the same code
    }
}

// Draws and drives the input-configuration screen: both players' device (keyboard/
// joystick) and per-action bindings, the row cursor, the swap-players shortcut (T), the
// "press a key" capture flow (via GetPressedKeyName / GetFlagMask), navigation (cursor
// keys change the selected row or the device; enter starts rebinding), and Escape to save
// (writes the profile's settings) and return to the main menu.
void ConfigInputMenu()
{
    int x = 400;
    // Y position of each of the 18 cursor rows (2 device rows + 8 action rows per player).
    int lines[NUM_CONFIG_ROWS] = { 100, 0x7a, 0x86, 0x92, 0x9e, 0xaa, 0xb6, 0xc2, 0xce,
                      0xfa, 0x110, 0x11c, 0x128, 0x134, 0x140, 0x14c, 0x158, 0x164 };
    int swapKeyBind;     // scratch for swapping a keyboard-key binding
    int swapDeviceBind;  // scratch for swapping a device selection or a joystick-button-mask binding

    // ---- setup ----
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawMenuText("@@CONFIG INPUT DEVICES", POS_CENTERED, 0x16, 2);
    Blit((g_screenW >> 1) - 0xac, 4, 0, g_gfxLogos, 0, 0x11e, 0x30, 0x2c);
    Blit((g_screenW >> 1) + 0x7e, 4, 0, g_gfxLogos, 0x30, 0x11e, 0x30, 0x2c);

    // Player one: device row, then its move/fire/rocket/pause/profile bindings
    DrawMenuText("PLAYER ONE   :                ", POS_CENTERED, 100, 1);
    if (g_cfg.device[0] == DEVICE_KEYBOARD)
        DrawMenuText("KEYBOARD", x, g_curY, 2);
    else if (g_cfg.device[0] == DEVICE_JOYSTICK1)
        DrawMenuText("JOYSTICK 1 / JOYPAD 1", x, g_curY, 2);
    else
        DrawMenuText("JOYSTICK 2 / JOYPAD 2", x, g_curY, 2);
    g_textCursorY = g_textCursorY + 10;

    if (g_cfg.device[0] == DEVICE_KEYBOARD) {
        DrawMenuText("   MOVE LEFT :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.left[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("  MOVE RIGHT :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.right[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("     MOVE UP :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.up[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("   MOVE DOWN :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.down[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);

        DrawMenuText(" FIRE WEAPON :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.fire[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" FIRE ROCKET :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.rocket[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" PAUSE GAME  :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.pause[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("PROFILE INFO :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.profile[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
    }
    else {
        // Joystick: X/Y axis for move, still explicit bindings for fire/rocket/pause/profile
        DrawMenuText("   MOVE LEFT :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("X AXIS", x, g_curY, 2);
        DrawMenuText("  MOVE RIGHT :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("X AXIS", x, g_curY, 2);
        DrawMenuText("     MOVE UP :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("Y AXIS", x, g_curY, 2);
        DrawMenuText("   MOVE DOWN :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("Y AXIS", x, g_curY, 2);

        DrawMenuText(" FIRE WEAPON :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.joyFire[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" FIRE ROCKET :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.joyRocket[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" PAUSE GAME  :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.joyPause[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("PROFILE INFO :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[0], g_cfg.joyProfile[0]);
        DrawMenuText(g_keyName, x, g_curY, 2);
    }

    // Player two: same layout as player one
    DrawMenuText("PLAYER TWO   :                ", POS_CENTERED, 0xfa, 1);
    if (g_cfg.device[1] == DEVICE_KEYBOARD)
        DrawMenuText("KEYBOARD", x, g_curY, 2);
    else if (g_cfg.device[1] == DEVICE_JOYSTICK1)
        DrawMenuText("JOYSTICK 1 / JOYPAD 1", x, g_curY, 2);
    else
        DrawMenuText("JOYSTICK 2 / JOYPAD 2", x, g_curY, 2);
    g_textCursorY = g_textCursorY + 10;

    if (g_cfg.device[1] == DEVICE_KEYBOARD) {
        DrawMenuText("   MOVE LEFT :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.left[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("  MOVE RIGHT :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.right[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("     MOVE UP :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.up[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("   MOVE DOWN :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.down[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);

        DrawMenuText(" FIRE WEAPON :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.fire[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" FIRE ROCKET :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.rocket[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" PAUSE GAME  :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.pause[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("PROFILE INFO :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.profile[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
    }
    else {
        DrawMenuText("   MOVE LEFT :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("X AXIS", x, g_curY, 2);
        DrawMenuText("  MOVE RIGHT :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("X AXIS", x, g_curY, 2);
        DrawMenuText("     MOVE UP :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("Y AXIS", x, g_curY, 2);
        DrawMenuText("   MOVE DOWN :                ", POS_CENTERED, g_textAutoY, 1);
        DrawMenuText("Y AXIS", x, g_curY, 2);

        DrawMenuText(" FIRE WEAPON :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.joyFire[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" FIRE ROCKET :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.joyRocket[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText(" PAUSE GAME  :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.joyPause[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
        DrawMenuText("PROFILE INFO :                ", POS_CENTERED, g_textAutoY, 1);
        ButtonName(g_cfg.device[1], g_cfg.joyProfile[1]);
        DrawMenuText(g_keyName, x, g_curY, 2);
    }

    // Help text and blink-driven cursor mark below the binding rows
    g_textCursorY = g_textCursorY + 20;
    if (g_editing != 0 && g_uiBlink != 0)
        DrawMenuText("PRESS A KEY FOR SELECTED FUNCTION", POS_CENTERED, g_textAutoY, 0);
    else
        DrawMenuText(" ", POS_CENTERED, g_textAutoY, 6);
    g_textCursorY = g_textCursorY + 30;
    DrawMenuText("USE CURSOR KEYS TO MOVE CURSOR UP OR DOWN", POS_CENTERED, g_textAutoY, 6);
    DrawMenuText("USE LEFT AND RIGHT CURSOR KEYS TO CHANGE INPUT DEVICE", POS_CENTERED, g_textAutoY, 6);
    DrawMenuText("PRESS ENTER TO SELECT A NEW KEY", POS_CENTERED, g_textAutoY, 6);
    g_textCursorY = g_textCursorY + 10;
    DrawMenuText("PRESS ESC TO RETURN TO MAIN MENU", POS_CENTERED, g_textAutoY, 6);
    g_textCursorY = g_textCursorY + 20;
    if (g_time - g_uiBlinkTime > g_blinkRate) {
        g_uiBlinkTime = g_time;
        g_uiBlink = g_uiBlink == 0;
    }
    if (g_editing != 0) {
        DrawMenuText("#", x - 15, lines[g_cursor], 2);
        DrawMenuText("#", x + 0xe1, lines[g_cursor], 2);
    }
    else
        DrawMenuText("#", x - 15, lines[g_cursor], 0);

    // T swaps player one's and player two's entire control scheme (device + every binding).
    if (KeyDown(K_VK_T)) {
        if (g_keyLatch[K_VK_T] != 0) {
            swapDeviceBind = g_cfg.device[0];
            g_cfg.device[0] = g_cfg.device[1];
            g_cfg.device[1] = swapDeviceBind;

            // Swap keyboard bindings
            swapKeyBind = g_cfg.fire[0];
            g_cfg.fire[0] = g_cfg.fire[1];
            g_cfg.fire[1] = swapKeyBind;
            swapKeyBind = g_cfg.left[0];
            g_cfg.left[0] = g_cfg.left[1];
            g_cfg.left[1] = swapKeyBind;
            swapKeyBind = g_cfg.right[0];
            g_cfg.right[0] = g_cfg.right[1];
            g_cfg.right[1] = swapKeyBind;
            swapKeyBind = g_cfg.up[0];
            g_cfg.up[0] = g_cfg.up[1];
            g_cfg.up[1] = swapKeyBind;
            swapKeyBind = g_cfg.down[0];
            g_cfg.down[0] = g_cfg.down[1];
            g_cfg.down[1] = swapKeyBind;
            swapKeyBind = g_cfg.rocket[0];
            g_cfg.rocket[0] = g_cfg.rocket[1];
            g_cfg.rocket[1] = swapKeyBind;

            swapKeyBind = g_cfg.key6[0];
            g_cfg.key6[0] = g_cfg.key6[1];
            g_cfg.key6[1] = swapKeyBind;
            swapKeyBind = g_cfg.pause[0];
            g_cfg.pause[0] = g_cfg.pause[1];
            g_cfg.pause[1] = swapKeyBind;
            swapKeyBind = g_cfg.profile[0];
            g_cfg.profile[0] = g_cfg.profile[1];
            g_cfg.profile[1] = swapKeyBind;

            // Swap joystick bindings
            swapDeviceBind = g_cfg.joyFire[0];
            g_cfg.joyFire[0] = g_cfg.joyFire[1];
            g_cfg.joyFire[1] = swapDeviceBind;
            swapDeviceBind = g_cfg.joyRocket[0];
            g_cfg.joyRocket[0] = g_cfg.joyRocket[1];
            g_cfg.joyRocket[1] = swapDeviceBind;
            swapDeviceBind = g_cfg.joyBtn2[0];
            g_cfg.joyBtn2[0] = g_cfg.joyBtn2[1];
            g_cfg.joyBtn2[1] = swapDeviceBind;
            swapDeviceBind = g_cfg.joyPause[0];
            g_cfg.joyPause[0] = g_cfg.joyPause[1];
            g_cfg.joyPause[1] = swapDeviceBind;
            swapDeviceBind = g_cfg.joyProfile[0];
            g_cfg.joyProfile[0] = g_cfg.joyProfile[1];
            g_cfg.joyProfile[1] = swapDeviceBind;
            g_keyLatch[K_VK_T] = 0;
            PlayClick();
        }
    }
    else
        g_keyLatch[K_VK_T] = 1;

    // ---- Escape: cancel editing, or save and return to the main menu ----
    if (KeyDown(K_VK_ESCAPE)) {
        if (g_editing != 0) {
            g_editing = 0;
            g_inputCooldown = 10;
            PlayClick();
        }
        else {
            WaitForKeyRelease(K_VK_ESCAPE);
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 25000;
            g_state = STATE_TITLE;
            ClearPlayers();

            // Save the new settings into the active profile, if any
            if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
                g_playerUpdateFn != StateDemo) {
                UnpackAccount(g_profileIndex);
                g_hiscore = g_acc.settings.best;
                g_acc.settings = g_cfg;
                if (g_hiscore > g_acc.settings.best)
                    g_acc.settings.best = g_hiscore;
                PackAccount(g_profileIndex);
                SaveAccount(g_profileIndex);
            }
        }
    }

    // ---- editing: key capture ----
    if (g_editing != 0) {
        GetPressedKeyName();
        // g_cursor row -> action: 0/9 = device select (handled below via left/right), 1-8 and
        // 10-17 = player one's / player two's move/fire/rocket/pause/profile bindings.
        if (g_pressedKey != -1) {
            switch (g_cursor) {
            case 1:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.left[0] = g_pressedKey;
                g_editing = 0;
                break;
            case 2:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.right[0] = g_pressedKey;
                g_editing = 0;
                break;

            case 3:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.up[0] = g_pressedKey;
                g_editing = 0;
                break;
            case 4:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.down[0] = g_pressedKey;
                g_editing = 0;
                break;

            case 5:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.fire[0] = g_pressedKey;
                g_editing = 0;
                break;
            case 6:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.rocket[0] = g_pressedKey;
                g_editing = 0;
                break;
            case 7:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.pause[0] = g_pressedKey;
                g_editing = 0;
                break;
            case 8:
                if (g_cfg.device[0] == DEVICE_KEYBOARD)
                    g_cfg.profile[0] = g_pressedKey;
                g_editing = 0;
                break;

            case 10:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.left[1] = g_pressedKey;
                g_editing = 0;
                break;
            case 11:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.right[1] = g_pressedKey;
                g_editing = 0;
                break;
            case 12:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.up[1] = g_pressedKey;
                g_editing = 0;
                break;
            case 13:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.down[1] = g_pressedKey;
                g_editing = 0;
                break;

            case 14:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.fire[1] = g_pressedKey;
                g_editing = 0;
                break;
            case 15:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.rocket[1] = g_pressedKey;
                g_editing = 0;
                break;
            case 16:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.pause[1] = g_pressedKey;
                g_editing = 0;
                break;
            case 17:
                if (g_cfg.device[1] == DEVICE_KEYBOARD)
                    g_cfg.profile[1] = g_pressedKey;
                g_editing = 0;
                break;
            }
        }

        // Joystick fire/rocket/pause/profile bindings are captured via a button mask instead
        switch (g_cursor) {
        case 5:
            if (g_cfg.device[0] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyFire[0] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[0] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyFire[0] = GetFlagMask(1);
                g_editing = 0;
            }
            break;
        case 6:
            if (g_cfg.device[0] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyRocket[0] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[0] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyRocket[0] = GetFlagMask(1);
                g_editing = 0;
            }
            break;

        case 7:
            if (g_cfg.device[0] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyPause[0] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[0] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyPause[0] = GetFlagMask(1);
                g_editing = 0;
            }
            break;
        case 8:
            if (g_cfg.device[0] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyProfile[0] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[0] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyProfile[0] = GetFlagMask(1);
                g_editing = 0;
            }
            break;

        case 14:
            if (g_cfg.device[1] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyFire[1] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[1] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyFire[1] = GetFlagMask(1);
                g_editing = 0;
            }
            break;
        case 15:
            if (g_cfg.device[1] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyRocket[1] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[1] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyRocket[1] = GetFlagMask(1);
                g_editing = 0;
            }
            break;

        case 16:
            if (g_cfg.device[1] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyPause[1] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[1] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyPause[1] = GetFlagMask(1);
                g_editing = 0;
            }
            break;

        case 17:
            if (g_cfg.device[1] == DEVICE_JOYSTICK1 && GetFlagMask(0) != 0) {
                g_cfg.joyProfile[1] = GetFlagMask(0);
                g_editing = 0;
            }
            if (g_cfg.device[1] == DEVICE_JOYSTICK2 && GetFlagMask(1) != 0) {
                g_cfg.joyProfile[1] = GetFlagMask(1);
                g_editing = 0;
            }
            break;
        }
    }
    else {
        // Not editing: cursor keys move the row cursor or (rows 0/9) change the device
        if (KeyDown(K_VK_UP)) {
            if (g_keyLatch[K_VK_UP] != 0) {
                PlayClick();
                if (g_cursor > 0)
                    g_cursor = g_cursor - 1;
                g_keyLatch[K_VK_UP] = 0;
            }
        }
        else
            g_keyLatch[K_VK_UP] = 1;

        if (KeyDown(K_VK_DOWN)) {
            if (g_keyLatch[K_VK_DOWN] != 0) {
                PlayClick();
                if (g_cursor < NUM_CONFIG_ROWS - 1)
                    g_cursor = g_cursor + 1;
                g_keyLatch[K_VK_DOWN] = 0;
            }
        }
        else
            g_keyLatch[K_VK_DOWN] = 1;

        if (KeyDown(K_VK_LEFT)) {
            if (g_keyLatch[0] != 0) {
                PlayClick();
                if (g_cursor == P1_DEVICE_ROW) {
                if (g_cfg.device[0] == DEVICE_KEYBOARD && g_joyCount > 0)
                    g_cfg.device[0] = DEVICE_JOYSTICK1;
                else if (g_cfg.device[0] == DEVICE_JOYSTICK1 && g_joyCount > 1)
                    g_cfg.device[0] = DEVICE_JOYSTICK2;
                else
                    g_cfg.device[0] = DEVICE_KEYBOARD;
                }
                if (g_cursor == P2_DEVICE_ROW) {
                if (g_cfg.device[1] == DEVICE_KEYBOARD && g_joyCount > 0)
                    g_cfg.device[1] = DEVICE_JOYSTICK1;
                else if (g_cfg.device[1] == DEVICE_JOYSTICK1 && g_joyCount > 1)
                    g_cfg.device[1] = DEVICE_JOYSTICK2;
                else
                    g_cfg.device[1] = DEVICE_KEYBOARD;
                }
                g_keyLatch[0] = 0;
            }
        }
        else
            g_keyLatch[0] = 1;

        if (KeyDown(K_VK_RIGHT)) {
            if (g_keyLatch[K_VK_RIGHT] != 0) {
                PlayClick();
                if (g_cursor == P1_DEVICE_ROW) {
                if (g_cfg.device[0] == DEVICE_KEYBOARD && g_joyCount > 0)
                    g_cfg.device[0] = DEVICE_JOYSTICK1;
                else if (g_cfg.device[0] == DEVICE_JOYSTICK1 && g_joyCount > 1)
                    g_cfg.device[0] = DEVICE_JOYSTICK2;
                else
                    g_cfg.device[0] = DEVICE_KEYBOARD;
                }
                if (g_cursor == P2_DEVICE_ROW) {
                if (g_cfg.device[1] == DEVICE_KEYBOARD && g_joyCount > 0)
                    g_cfg.device[1] = DEVICE_JOYSTICK1;
                else if (g_cfg.device[1] == DEVICE_JOYSTICK1 && g_joyCount > 1)
                    g_cfg.device[1] = DEVICE_JOYSTICK2;
                else
                    g_cfg.device[1] = DEVICE_KEYBOARD;
                }
                g_keyLatch[K_VK_RIGHT] = 0;
            }
        }
        else
            g_keyLatch[K_VK_RIGHT] = 1;

        if (KeyDown(K_VK_RETURN) && g_cursor != P1_DEVICE_ROW && g_cursor != P2_DEVICE_ROW &&
            (g_cursor < 9 || g_cursor > 8)) {
            if (g_keyLatch[K_VK_RETURN] != 0) {
                g_inputCooldown = 15;
                g_editing = 1;
                PlayClick();
                g_keyLatch[K_VK_RETURN] = 0;
            }
        }
        else
            g_keyLatch[K_VK_RETURN] = 1;
    }

    // ---- fixups ----
    FixDuplicateKeys(0);
    FixDuplicateKeys(1);
    if (g_time - g_uiBlinkTime > g_blinkRate) {
        g_uiBlinkTime = g_time;
        g_uiBlink = g_uiBlink == 0;
    }
}

// True if `key` isn't already bound to any of player 1's (and, in versus/co-op modes,
// player 2's) movement/fire/rocket keys.
bool IsKeyFree(int key)
{
    bool result = true;
    if (g_save.players[0].inputDevice == DEVICE_KEYBOARD) {
        if (g_cfg.left[0] == key)
            return false;
        if (g_cfg.right[0] == key)
            return false;
        if (g_cfg.down[0] == key)
            return false;
        if (g_cfg.up[0] == key)
            return false;
        if (g_cfg.fire[0] == key)
            return false;
        if (g_cfg.rocket[0] == key)
            return false;
    }

    if ((g_gameMode == MODE_TWO_PLAYER || g_gameMode == MODE_DUAL || g_gameMode == MODE_TEAM) &&
        g_save.players[1].inputDevice == DEVICE_KEYBOARD) {
        if (g_cfg.left[1] == key)
            return false;
        if (g_cfg.right[1] == key)
            return false;
        if (g_cfg.down[1] == key)
            return false;
        if (g_cfg.up[1] == key)
            return false;
        if (g_cfg.fire[1] == key)
            return false;
        if (g_cfg.rocket[1] == key)
            return false;
    }
    return result;
}

