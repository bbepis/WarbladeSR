// Tests for src/ui/controls.c (key names, the text-input key mapping, duplicate bindings, the
// input configuration screen) and src/ui/screens.c (the about screen's credit links).
#include "support.h"

// ---- key mapping and names ----

TEST(ui_KeyToChar_maps_letters_digits_and_punctuation)
{
    CHECK_EQ_INT(KeyToChar(K_VK_A), 'A');
    CHECK_EQ_INT(KeyToChar(K_VK_M), 'M');
    CHECK_EQ_INT(KeyToChar(K_VK_Z), 'Z');
    CHECK_EQ_INT(KeyToChar(K_VK_0), '0');
    CHECK_EQ_INT(KeyToChar(K_VK_9), '9');
    CHECK_EQ_INT(KeyToChar(K_VK_NUM0), '0');
    CHECK_EQ_INT(KeyToChar(K_VK_NUM7), '7');
    CHECK_EQ_INT(KeyToChar(K_VK_OEM_COMMA), ',');
    CHECK_EQ_INT(KeyToChar(K_VK_OEM_PERIOD), '.');
    CHECK_EQ_INT(KeyToChar(K_VK_DECIMAL), '.');
    CHECK_EQ_INT(KeyToChar(K_VK_SPACE), ' ');
    CHECK_EQ_INT(KeyToChar(K_VK_F1), 0);
    CHECK_EQ_INT(KeyToChar(K_VK_LEFT), 0);
}

TEST(ui_KeyName_pads_names)
{
    KeyName(K_VK_A);
    CHECK_STR(g_keyName, "A            ");
    KeyName(K_VK_7);
    CHECK_STR(g_keyName, "7            ");
    KeyName(K_VK_UP);
    CHECK_STR(g_keyName, "CURSOR UP    ");
    KeyName(K_VK_R_CONTROL);
    CHECK_STR(g_keyName, "RIGHT CTRL   ");
    KeyName(K_VK_NUM3);
    CHECK_STR(g_keyName, "NUMPAD 3     ");
    KeyName(K_VK_LWIN);
    CHECK_STR(g_keyName, "LEFT WINDOW  ");
    KeyName(K_VK_F5);
    CHECK_STR(g_keyName, "UNDEFINED    ");
}

TEST(ui_ButtonName_joystick_bits)
{
    ButtonName(1, 1);
    CHECK_STR(g_keyName, "BUTTON 1");
    ButtonName(1, 4);
    CHECK_STR(g_keyName, "BUTTON 3");
    ButtonName(2, 0x4000);
    CHECK_STR(g_keyName, "BUTTON 15");
    ButtonName(1, 3);
    CHECK_STR(g_keyName, "UNDEFINED");
    ButtonName(0, K_VK_Z);
    CHECK_STR(g_keyName, "Z            ");
}

TEST(ui_TrimSpaces_both_ends)
{
    char a[] = "  LEFT CTRL   ";
    TrimSpaces(a);
    CHECK_STR(a, "LEFT CTRL");
    char b[] = "X";
    TrimSpaces(b);
    CHECK_STR(b, "X");
    char c[] = "   ";
    TrimSpaces(c);
    CHECK_STR(c, "");
    TrimSpaces(NULL);
}

TEST(ui_ControlsText_keyboard_and_joystick)
{
    g_cfg.device[0] = DEVICE_KEYBOARD;
    g_cfg.left[0] = K_VK_LEFT;
    g_cfg.right[0] = K_VK_RIGHT;
    g_cfg.fire[0] = K_VK_L_CONTROL;
    g_cfg.rocket[0] = K_VK_SPACE;
    ControlsText(0);
    CHECK_STR(g_logBuf, "PL1: MOVE LEFT:CURSOR LEFT   MOVE RIGHT:CURSOR RIGHT   FIRE:LEFT CTRL   FIRE2:SPACE");
    g_cfg.device[1] = DEVICE_JOYSTICK1;
    g_cfg.joyFire[1] = 2;
    g_cfg.joyRocket[1] = 8;
    ControlsText(1);
    CHECK_STR(g_logBuf, "PL2: MOVE LEFT:X AXIS LEFT   MOVE RIGHT:X AXIS RIGHT   FIRE:BUTTON 2   FIRE2:BUTTON 4");
}

TEST(ui_GetPressedKeyName_reports_the_first_held_key)
{
    GetPressedKeyName();
    CHECK_EQ_INT(g_pressedKey, -1);
    FakePressKey(K_VK_Q);
    GetPressedKeyName();
    CHECK_EQ_INT(g_pressedKey, K_VK_Q);
    CHECK_STR(g_keyName, "Q            ");
    FakePressKey(K_VK_3);   // digits come first
    GetPressedKeyName();
    CHECK_EQ_INT(g_pressedKey, K_VK_3);
    FakeReleaseAllKeys();
    FakePressKey(K_VK_LWIN);
    GetPressedKeyName();
    CHECK_STR(g_keyName, "LEFT WINDOWS ");
}

TEST(ui_ResetFlags_sets_the_letter_and_digit_latches)
{
    memset(g_keyLatch, 0, sizeof g_keyLatch);
    ResetFlags();
    CHECK_EQ_INT(g_keyLatch[K_VK_A], 1);
    CHECK_EQ_INT(g_keyLatch[K_VK_Z], 1);
    CHECK_EQ_INT(g_keyLatch[K_VK_9], 1);
    CHECK_EQ_INT(g_keyLatch[K_VK_ESCAPE], 0);
    CHECK_EQ_INT(g_keyLatch[K_VK_NUM0], 0);
}

// ---- duplicate bindings ----

static void DistinctKeys(int p)
{
    g_cfg.device[p] = DEVICE_KEYBOARD;
    g_cfg.left[p] = K_VK_LEFT;
    g_cfg.right[p] = K_VK_RIGHT;
    g_cfg.up[p] = K_VK_UP;
    g_cfg.down[p] = K_VK_DOWN;
    g_cfg.fire[p] = K_VK_L_CONTROL;
    g_cfg.rocket[p] = K_VK_SPACE;
    g_cfg.key6[p] = K_VK_L_SHIFT;
    g_cfg.pause[p] = K_VK_P;
    g_cfg.profile[p] = K_VK_TAB;
}

TEST(ui_FixDuplicateKeys_leaves_distinct_keys)
{
    DistinctKeys(0);
    g_keysChanged = 0;
    FixDuplicateKeys(0);
    CHECK_EQ_INT(g_keysChanged, 0);
    CHECK_EQ_INT(g_cfg.rocket[0], K_VK_SPACE);
}

TEST(ui_FixDuplicateKeys_flags_a_single_keyboard_collision)
{
    DistinctKeys(0);
    g_cfg.playerKeys[0][0] = K_VK_1;
    g_cfg.menuKeys[0][0] = K_VK_F1;
    g_cfg.rocket[0] = g_cfg.left[0];
    g_keysChanged = 0;
    FixDuplicateKeys(0);
    CHECK_EQ_INT(g_cfg.rocket[0], K_VK_Q);
    CHECK_EQ_INT(g_keysChanged, 1);
}

TEST(ui_FixDuplicateKeys_resets_the_later_key_to_Q)
{
    DistinctKeys(1);
    g_cfg.rocket[1] = K_VK_LEFT;   // same as left
    g_cfg.profile[1] = K_VK_UP;    // same as up
    g_keysChanged = 0;
    FixDuplicateKeys(1);
    CHECK_EQ_INT(g_keysChanged, 1);
    CHECK_EQ_INT(g_cfg.left[1], K_VK_LEFT);
    CHECK_EQ_INT(g_cfg.rocket[1], K_VK_Q);
    CHECK_EQ_INT(g_cfg.up[1], K_VK_UP);
    CHECK_EQ_INT(g_cfg.profile[1], K_VK_Q);
    DistinctKeys(0);
    g_cfg.down[0] = K_VK_RIGHT;
    FixDuplicateKeys(0);
    CHECK_EQ_INT(g_cfg.down[0], K_VK_Q);
    CHECK_EQ_INT(g_cfg.right[0], K_VK_RIGHT);
}

TEST(ui_FixDuplicateKeys_unbinds_joystick_buttons)
{
    g_cfg.device[0] = DEVICE_JOYSTICK1;
    g_cfg.joyFire[0] = 1;
    g_cfg.joyRocket[0] = 2;
    g_cfg.joyBtn2[0] = 4;
    g_cfg.joyPause[0] = 2;
    g_cfg.joyProfile[0] = 16;
    g_keysChanged = 0;
    FixDuplicateKeys(0);
    CHECK_EQ_INT(g_keysChanged, 1);
    CHECK_EQ_INT(g_cfg.joyRocket[0], 2);
    CHECK_EQ_INT(g_cfg.joyPause[0], -1);
    CHECK_EQ_INT(g_cfg.joyProfile[0], 16);
}

TEST(ui_IsKeyFree_checks_the_playing_players)
{
    DistinctKeys(0);
    DistinctKeys(1);
    g_cfg.left[1] = K_VK_A;
    g_save.players[0].inputDevice = DEVICE_KEYBOARD;
    g_save.players[1].inputDevice = DEVICE_KEYBOARD;
    g_gameMode = MODE_SINGLE;
    CHECK(!IsKeyFree(K_VK_LEFT));
    CHECK(!IsKeyFree(K_VK_SPACE));
    CHECK(IsKeyFree(K_VK_P));        // pause isn't checked
    CHECK(IsKeyFree(K_VK_A));        // player 2 isn't playing
    g_gameMode = MODE_TWO_PLAYER;
    CHECK(!IsKeyFree(K_VK_A));
    g_save.players[0].inputDevice = DEVICE_JOYSTICK1;   // player 1's keys are free then
    CHECK(IsKeyFree(K_VK_LEFT));
    CHECK(!IsKeyFree(K_VK_RIGHT));                      // still player 2's
    g_save.players[1].inputDevice = DEVICE_JOYSTICK2;
    CHECK(IsKeyFree(K_VK_RIGHT));
}

// ---- the input configuration screen ----

static void OpenConfig(void)
{
    BootGame();
    WinInit();
    RunFrames(1);
    TapKey(K_VK_F9, 1);
}

TEST(ui_ConfigInputMenu_cursor_moves_and_clamps)
{
    OpenConfig();
    CHECK_EQ_INT(g_state, STATE_INPUT_CONFIG);
    CHECK_EQ_INT(g_cursor, 0);
    TapKey(K_VK_UP, 1);
    CHECK_EQ_INT(g_cursor, 0);
    for (int i = 0; i < 20; i++)
        TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(g_cursor, 17);
    TapKey(K_VK_UP, 1);
    CHECK_EQ_INT(g_cursor, 16);
}

TEST(ui_ConfigInputMenu_rebinds_a_key)
{
    OpenConfig();
    g_cfg.device[0] = DEVICE_KEYBOARD;
    TapKey(K_VK_DOWN, 1);         // row 1: player one MOVE LEFT
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_editing, 1);
    TapKey(K_VK_J, 1);
    CHECK_EQ_INT(g_editing, 0);
    CHECK_EQ_INT(g_cfg.left[0], K_VK_J);
}

TEST(ui_ConfigInputMenu_rebinds_player_two)
{
    OpenConfig();
    g_cfg.device[1] = DEVICE_KEYBOARD;
    g_cursor = 15;                 // player two FIRE ROCKET
    TapKey(K_VK_RETURN, 1);
    TapKey(K_VK_K, 1);
    CHECK_EQ_INT(g_cfg.rocket[1], K_VK_K);
    g_cursor = 17;                 // player two PROFILE INFO
    TapKey(K_VK_RETURN, 1);
    TapKey(K_VK_H, 1);
    CHECK_EQ_INT(g_cfg.profile[1], K_VK_H);
}

TEST(ui_ConfigInputMenu_device_rows_take_no_key)
{
    OpenConfig();
    g_cursor = 9;
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_editing, 0);
    g_cursor = 0;
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_editing, 0);
}

TEST(ui_ConfigInputMenu_rebinding_to_a_used_key_resets_the_other)
{
    OpenConfig();
    DistinctKeys(0);
    g_cursor = 2;                  // MOVE RIGHT
    TapKey(K_VK_RETURN, 1);
    TapKey(K_VK_UP, 1);            // already MOVE UP
    CHECK_EQ_INT(g_cfg.right[0], K_VK_UP);
    CHECK_EQ_INT(g_cfg.up[0], K_VK_Q);
}

TEST(ui_ConfigInputMenu_escape_cancels_editing)
{
    OpenConfig();
    g_cursor = 3;
    int up = g_cfg.up[0];
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_editing, 1);
    FakePressKey(K_VK_ESCAPE);
    RunFrames(1);
    FakeReleaseKey(K_VK_ESCAPE);
    CHECK_EQ_INT(g_editing, 0);
    CHECK_EQ_INT(g_inputCooldown, 10);
    CHECK_EQ_INT(g_state, STATE_INPUT_CONFIG);
    CHECK_EQ_INT(g_cfg.up[0], up);
}

TEST(ui_ConfigInputMenu_left_right_pick_the_device)
{
    OpenConfig();
    g_cfg.device[0] = DEVICE_KEYBOARD;
    g_joyCount = 0;
    TapKey(K_VK_RIGHT, 1);
    CHECK_EQ_INT(g_cfg.device[0], DEVICE_KEYBOARD);   // no joystick
    g_joyCount = 2;
    TapKey(K_VK_RIGHT, 1);
    CHECK_EQ_INT(g_cfg.device[0], DEVICE_JOYSTICK1);
    TapKey(K_VK_LEFT, 1);
    CHECK_EQ_INT(g_cfg.device[0], DEVICE_JOYSTICK2);
    TapKey(K_VK_RIGHT, 1);
    CHECK_EQ_INT(g_cfg.device[0], DEVICE_KEYBOARD);
    g_cursor = 9;
    g_cfg.device[1] = DEVICE_KEYBOARD;
    g_joyCount = 1;
    TapKey(K_VK_RIGHT, 1);
    CHECK_EQ_INT(g_cfg.device[1], DEVICE_JOYSTICK1);
    TapKey(K_VK_RIGHT, 1);
    CHECK_EQ_INT(g_cfg.device[1], DEVICE_KEYBOARD);
    CHECK_EQ_INT(g_cfg.device[0], DEVICE_KEYBOARD);
}

TEST(ui_ConfigInputMenu_T_swaps_the_players)
{
    OpenConfig();
    DistinctKeys(0);
    DistinctKeys(1);
    g_cfg.fire[1] = K_VK_R_CONTROL;
    g_cfg.pause[1] = K_VK_O;
    g_cfg.device[1] = DEVICE_JOYSTICK2;
    g_cfg.joyFire[0] = 1;
    g_cfg.joyFire[1] = 8;
    TapKey(K_VK_T, 1);
    CHECK_EQ_INT(g_cfg.fire[0], K_VK_R_CONTROL);
    CHECK_EQ_INT(g_cfg.fire[1], K_VK_L_CONTROL);
    CHECK_EQ_INT(g_cfg.pause[0], K_VK_O);
    CHECK_EQ_INT(g_cfg.device[0], DEVICE_JOYSTICK2);
    CHECK_EQ_INT(g_cfg.device[1], DEVICE_KEYBOARD);
    CHECK_EQ_INT(g_cfg.joyFire[0], 8);
    CHECK_EQ_INT(g_cfg.joyFire[1], 1);
}

TEST(ui_ConfigInputMenu_draws_the_bindings)
{
    OpenConfig();
    g_cfg.device[0] = DEVICE_KEYBOARD;
    g_cfg.device[1] = DEVICE_JOYSTICK1;
    g_cfg.left[0] = K_VK_LEFT;
    RunFrames(1);
    // the last text drawn is the cursor mark at row 0's line
    CHECK_EQ_INT(g_curY, 100);
}

// ---- the about screen ----

static int LinkIndex(int id)
{
    for (int i = 0; i < g_linkCount; i++)
        if (g_links[i].id == id)
            return i;
    TestFail(__FILE__, __LINE__, "no link %d", id);
    return -1;
}

static void ClickLink(int id)
{
    int i = LinkIndex(id);
    g_fake.mouseX = (g_links[i].x1 + g_links[i].x2) / 2;
    g_fake.mouseY = (g_links[i].y1 + g_links[i].y2) / 2;
    RunFrames(1);
    g_fake.mouseLeft = true;
    RunFrames(1);
    g_fake.mouseLeft = false;
    RunFrames(1);
}

static void OpenAbout(void)
{
    BootGame();
    WinInit();
    RunFrames(1);
    TapKey(K_VK_RIGHT, 1);   // intro -> about
    RunFrames(1);
    CHECK(g_frameFunc == AboutScreen);
}

TEST(ui_AboutScreen_credit_link_opens_a_window)
{
    OpenAbout();
    ClickLink(1);
    int w = -1;
    for (int i = 0; i < MAX_WINDOWS; i++)
        if (g_windows[i].active)
            w = i;
    CHECK(w >= 0);
    CHECK_STR(g_windows[w].texts[0].text, "EDGAR VIGDAL");
    CHECK_STR(FakeImageName(g_creditPic), "edgar.jpg");
    CHECK_STR(g_windows[w].links[0].text2, "mailto:edgar@warblade.as");
    CHECK_EQ_INT(g_windows[w].selF, MENUID_CLOSE);
}

TEST(ui_AboutScreen_window_link_opens_its_url)
{
    OpenAbout();
    ClickLink(3);
    RunFrames(30);
    int w = -1;
    for (int i = 0; i < MAX_WINDOWS; i++)
        if (g_windows[i].active)
            w = i;
    CHECK_STR(g_windows[w].texts[0].text, "JOSHUA WAYNE");
    int x = g_windows[w].x + g_windows[w].links[1].x + 4;
    int y = g_windows[w].y + g_windows[w].links[1].y + 4;
    g_fake.mouseX = x;
    g_fake.mouseY = y;
    RunFrames(1);
    g_fake.mouseLeft = true;
    RunFrames(1);
    g_fake.mouseLeft = false;
    RunFrames(1);
    // OpenUrl sends dead sites to their archived copies
    CHECK_STR(g_fake.lastUrl, "https://web.archive.org/web/20030227224434/http://karthesios.tripod.com:80/");
}

TEST(ui_AboutScreen_engine_link_opens_the_sdl_site)
{
    OpenAbout();
    ClickLink(25);
    CHECK_STR(g_fake.lastUrl, "https://www.libsdl.org/");
    CHECK_EQ_INT(g_transitionLock, 1);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_ABOUT);
    CHECK_EQ_INT(g_idleTimeoutMs, 15000);
}

TEST(ui_AboutScreen_shows_the_total_play_time)
{
    OpenAbout();
    g_cfg.playTime = 125LL * 10000000;   // FILETIME units (100 ns): 2 min 5 s
    RunFrames(1);
    CHECK_STR(g_logBuf, "TOTAL GAME TIME : 02 MINUTES. 05 SECONDS");
    g_cfg.playTime = 0;
    RunFrames(1);
    CHECK_STR(g_logBuf, "NO GAME TIME RECORDED");
}
