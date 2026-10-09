// Tests for src/ui/menu.c: the title menu's actions (hotkeys, menu entries, the settings page,
// starting games, the quit dialog, the profile windows), driven through the booted game.
#include "support.h"

// Boots to the title menu and drops the first-run "PRESETS" window.
static void BootMenu(void)
{
    BootGame();
    WinInit();
    RunFrames(1);
}

static int EntryIndex(int group)
{
    for (int i = 0; i < MAX_MENU_ENTRIES; i++)
        if (g_menuEntries[i].active && g_menuEntries[i].group == group)
            return i;
    TestFail(__FILE__, __LINE__, "no menu entry %d", group);
    return -1;
}

static void MoveTo(int x, int y)
{
    g_fake.mouseX = x;
    g_fake.mouseY = y;
    RunFrames(1);
}

// Moves to the centre of menu entry `group`, presses and releases the mouse there.
static void ClickEntry(int group)
{
    int i = EntryIndex(group);
    int x = g_menuEntries[i].x + g_menuEntries[i].w / 2;
    int y = g_menuEntries[i].y + g_menuEntries[i].h / 2;
    MoveTo(x, y);
    g_fake.mouseLeft = true;
    RunFrames(1);
    g_fake.mouseLeft = false;
    RunFrames(1);
}

// The settings page: its entries show (and take clicks) on the HELP_CONTROLS screen.
static void OpenSettingsPage(void)
{
    ClickEntry(MENUID_SETTINGS);
    RunFrames(2);
}

// Lets the transition lock (TRANSITION_LOCK_MS) run out.
static void WaitUnlock(void)
{
    RunFrames(40);
}

static void WaitWindowsGone(void)
{
    RunFrames(35);
}

// ---------------------------------------------------------------------------------------
// Hotkeys
// ---------------------------------------------------------------------------------------

TEST(ui_Menu_number_keys_pick_the_difficulty)
{
    BootMenu();
    TapKey(K_VK_1, 1);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_EASY);
    CHECK_EQ_INT(g_hofMode, HOF_EASY);
    CHECK_EQ_INT(g_cfg.fps, FPS_EASY);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HELP_CONTROLS);
    CHECK_EQ_INT(g_idleTimeoutMs, 15000);
    TapKey(K_VK_3, 1);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_HARD);
    CHECK_EQ_INT(g_hofMode, HOF_HARD);
    CHECK_EQ_INT(g_cfg.fps, FPS_HARD);
    TapKey(K_VK_4, 1);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_ACE);
    CHECK_EQ_INT(g_hofMode, HOF_ACE);
    CHECK_EQ_INT(g_cfg.fps, FPS_ACE_MENU);
    TapKey(K_VK_2, 1);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_NORMAL);
    CHECK_EQ_INT(g_hofMode, HOF_NORMAL);
    CHECK_EQ_INT(g_cfg.fps, FPS_NORMAL);
    CHECK_EQ_INT(FakePlayCount("tast"), 4);
}

TEST(ui_Menu_hotkeys_are_ignored_while_a_window_is_open)
{
    BootGame();   // the PRESETS window is open
    int diff = g_cfg.difficulty;
    int border = g_cfg.borderMode;
    TapKey(K_VK_4, 1);
    TapKey(K_VK_B, 1);
    CHECK_EQ_INT(g_cfg.difficulty, diff);
    CHECK_EQ_INT(g_cfg.borderMode, border);
}

TEST(ui_Menu_H_shows_the_hall_of_fame_for_the_difficulty)
{
    BootMenu();
    TapKey(K_VK_3, 1);
    g_hofMode = HOF_EASY;
    TapKey(K_VK_H, 1);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HALL_OF_FAME);
    CHECK_EQ_INT(g_hofMode, DIFF_HARD);
    CHECK_EQ_INT(g_idleTimeoutMs, 60000);
}

TEST(ui_Menu_up_and_down_browse_the_hall_of_fame_tables)
{
    BootMenu();
    g_hofMode = HOF_METEORSTORM;
    TapKey(K_VK_UP, 1);
    CHECK_EQ_INT(g_hofMode, HOF_TIME_TRIAL);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HALL_OF_FAME);
    TapKey(K_VK_UP, 1);
    CHECK_EQ_INT(g_hofMode, HOF_TIME_TRIAL);
    g_hofMode = 1;
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(g_hofMode, HOF_EASY);
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(g_hofMode, HOF_EASY);
}

TEST(ui_Menu_page_keys_change_the_sfx_volume_per_frame)
{
    BootMenu();
    g_cfg.sfxVol = 100;
    FakePressKey(K_VK_PAGEUP);
    RunFrames(5);
    FakeReleaseKey(K_VK_PAGEUP);
    CHECK_EQ_INT(g_cfg.sfxVol, 105);
    CHECK_EQ_INT(g_sfxVolTable[255], 105);
    CHECK_EQ_INT(g_sfxVolTable[100], 100 * 105 / 255);
    g_cfg.sfxVol = 254;
    FakePressKey(K_VK_PAGEUP);
    RunFrames(3);
    FakeReleaseKey(K_VK_PAGEUP);
    CHECK_EQ_INT(g_cfg.sfxVol, 255);
    g_cfg.sfxVol = 2;
    FakePressKey(K_VK_PAGEDOWN);
    RunFrames(4);
    FakeReleaseKey(K_VK_PAGEDOWN);
    CHECK_EQ_INT(g_cfg.sfxVol, 0);
    CHECK_EQ_INT(g_sfxVolTable[255], 0);
}

TEST(ui_Menu_plus_minus_change_the_music_volume)
{
    BootMenu();
    g_cfg.musicVolume = 10;
    FakePressKey(K_VK_ADD);
    RunFrames(3);
    FakeReleaseKey(K_VK_ADD);
    CHECK_EQ_INT(g_cfg.musicVolume, 13);
    g_cfg.musicVolume = 254;
    FakePressKey(K_VK_ADD);
    RunFrames(3);
    FakeReleaseKey(K_VK_ADD);
    CHECK_EQ_INT(g_cfg.musicVolume, 255);
    g_cfg.musicVolume = 1;
    FakePressKey(K_VK_SUBTRACT);
    RunFrames(3);
    FakeReleaseKey(K_VK_SUBTRACT);
    CHECK_EQ_INT(g_cfg.musicVolume, 0);
}

TEST(ui_Menu_home_end_change_the_voice_volume_table)
{
    BootMenu();
    g_cfg.musicVol = 200;
    FakePressKey(K_VK_END);
    RunFrames(10);
    FakeReleaseKey(K_VK_END);
    CHECK_EQ_INT(g_cfg.musicVol, 190);
    CHECK_EQ_INT(g_musVolTable[255], 190);
    g_cfg.musicVol = 253;
    FakePressKey(K_VK_HOME);
    RunFrames(10);
    FakeReleaseKey(K_VK_HOME);
    CHECK_EQ_INT(g_cfg.musicVol, 255);
    CHECK_EQ_INT(g_musVolTable[255], 255);
    g_cfg.musicVol = 3;
    FakePressKey(K_VK_END);
    RunFrames(10);
    FakeReleaseKey(K_VK_END);
    CHECK_EQ_INT(g_cfg.musicVol, 0);
}

TEST(ui_Menu_N_changes_the_star_count)
{
    BootMenu();
    g_cfg.numStars = 600;
    FakePressKey(K_VK_N);
    RunFrames(3);
    CHECK_NEAR(g_cfg.numStars, 630, 0);
    FakePressKey(K_VK_L_SHIFT);
    RunFrames(5);
    FakeReleaseAllKeys();
    CHECK_NEAR(g_cfg.numStars, 580, 0);
    g_cfg.numStars = 2995;
    FakePressKey(K_VK_N);
    RunFrames(2);
    CHECK_NEAR(g_cfg.numStars, 3000, 0);
    g_cfg.numStars = 55;
    FakePressKey(K_VK_R_SHIFT);
    RunFrames(2);
    FakeReleaseAllKeys();
    CHECK_NEAR(g_cfg.numStars, 50, 0);
}

TEST(ui_Menu_B_cycles_the_border_mode)
{
    BootMenu();
    g_cfg.borderMode = BORDER_ON;
    TapKey(K_VK_B, 1);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_OFF);
    TapKey(K_VK_B, 1);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_BLACK);
    TapKey(K_VK_B, 1);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_ON);
    FakePressKey(K_VK_B);
    RunFrames(5);
    FakeReleaseKey(K_VK_B);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_OFF);   // once per press
}

TEST(ui_Menu_shift_B_cycles_the_background_brightness)
{
    BootMenu();
    int border = g_cfg.borderMode;
    g_cfg.bgTint = BG_BRIGHTNESS_MIN;
    int expect[] = { BG_BRIGHTNESS_DEFAULT, BG_BRIGHTNESS_PRESET, 85, 100, BG_BRIGHTNESS_MAX,
                     BG_BRIGHTNESS_MIN };
    FakePressKey(K_VK_L_SHIFT);
    for (int i = 0; i < 6; i++) {
        TapKey(K_VK_B, 1);
        CHECK_EQ_INT(g_cfg.bgTint, expect[i]);
    }
    g_cfg.bgTint = 3;
    TapKey(K_VK_B, 1);
    CHECK_EQ_INT(g_cfg.bgTint, BG_BRIGHTNESS_MIN);
    CHECK_EQ_INT(g_cfg.borderMode, border);
}

TEST(ui_Menu_alt_B_toggles_the_background)
{
    BootMenu();
    g_cfg.bgEnabled = 0;
    FakePressKey(K_VK_MENU);
    TapKey(K_VK_B, 1);
    CHECK_EQ_INT(g_cfg.bgEnabled, 1);
    TapKey(K_VK_B, 1);
    CHECK_EQ_INT(g_cfg.bgEnabled, 0);
}

TEST(ui_Menu_U_toggles_the_announcer)
{
    BootMenu();
    g_cfg.sfxOn = 1;
    TapKey(K_VK_U, 1);
    CHECK_EQ_INT(g_cfg.sfxOn, 0);
    TapKey(K_VK_U, 1);
    CHECK_EQ_INT(g_cfg.sfxOn, 1);
}

TEST(ui_Menu_W_toggles_windowed_and_saves_the_settings)
{
    BootMenu();
    g_cfg.windowed = 1;
    remove(FakeUserPath("warblade\\WarBlade.inf"));
    TapKey(K_VK_W, 1);
    CHECK_EQ_INT(g_cfg.windowed, 0);
    CHECK(g_fake.fullscreen);
    CHECK(FakeFileExists(FakeUserPath("warblade\\WarBlade.inf")));
    TapKey(K_VK_W, 1);
    CHECK_EQ_INT(g_cfg.windowed, 1);
    CHECK(!g_fake.fullscreen);
}

TEST(ui_Menu_C_F_I_Z_toggle_once_per_transition_lock)
{
    BootMenu();
    g_cfg.collisionDetail = COLLISION_NORMAL;
    g_cfg.particlesOn = 0;
    g_cfg.bulletIntensity = BULLETS_NORMAL;
    g_cfg.bgStars = 0;
    TapKey(K_VK_C, 1);
    CHECK_EQ_INT(g_cfg.collisionDetail, COLLISION_SIMPLE);
    TapKey(K_VK_C, 1);   // locked: no change
    CHECK_EQ_INT(g_cfg.collisionDetail, COLLISION_SIMPLE);
    WaitUnlock();
    TapKey(K_VK_F, 1);
    CHECK_EQ_INT(g_cfg.particlesOn, 1);
    WaitUnlock();
    TapKey(K_VK_I, 1);
    CHECK_EQ_INT(g_cfg.bulletIntensity, BULLETS_BRIGHT);
    WaitUnlock();
    TapKey(K_VK_I, 1);
    CHECK_EQ_INT(g_cfg.bulletIntensity, BULLETS_FLARE_FX);
    WaitUnlock();
    TapKey(K_VK_I, 1);
    CHECK_EQ_INT(g_cfg.bulletIntensity, BULLETS_NORMAL);
    WaitUnlock();
    TapKey(K_VK_Z, 1);
    CHECK_EQ_INT(g_cfg.bgStars, 1);
    CHECK(g_fnPtr == DrawStarsPlayer);
    WaitUnlock();
    TapKey(K_VK_Z, 1);
    CHECK_EQ_INT(g_cfg.bgStars, 0);
    CHECK(g_fnPtr == DrawStarsGlow);
}

TEST(ui_Menu_E_changes_the_spark_count)
{
    BootMenu();
    g_cfg.sparks = 20;
    FakePressKey(K_VK_E);
    RunFrames(3);
    CHECK_NEAR(g_cfg.sparks, 26, 0);
    CHECK_EQ_INT(g_maxSparks, 13);
    FakePressKey(K_VK_L_SHIFT);
    RunFrames(10);
    FakeReleaseAllKeys();
    CHECK_NEAR(g_cfg.sparks, 10, 0);
    CHECK_EQ_INT(g_maxSparks, 5);
    g_cfg.sparks = 149;
    FakePressKey(K_VK_E);
    RunFrames(2);
    FakeReleaseAllKeys();
    CHECK_NEAR(g_cfg.sparks, 150, 0);
    CHECK_EQ_INT(g_maxSparks, 75);
}

TEST(ui_Menu_alt_M_toggles_shuffle)
{
    BootMenu();
    g_cfg.shuffle = 0;
    FakePressKey(K_VK_MENU);
    TapKey(K_VK_M, 1);
    CHECK_EQ_INT(g_cfg.shuffle, 1);
    TapKey(K_VK_M, 1);
    CHECK_EQ_INT(g_cfg.shuffle, 0);
}

TEST(ui_Menu_M_cycles_the_music_format)
{
    BootMenu();
    g_cfg.musicFormat = MUSIC_FMT_MP3;   // no playlist: back to modules
    TapKey(K_VK_M, 1);
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_MOD);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HELP_CONTROLS);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\title.mus");
}

TEST(ui_Menu_T_swaps_the_players_bindings)
{
    BootMenu();
    int l0 = g_cfg.left[0], l1 = g_cfg.left[1];
    int d0 = g_cfg.device[0], d1 = g_cfg.device[1];
    CHECK_NE_INT(l0, l1);
    g_cfg.device[1] = DEVICE_JOYSTICK1;
    d1 = DEVICE_JOYSTICK1;
    TapKey(K_VK_T, 1);
    CHECK_EQ_INT(g_cfg.left[0], l1);
    CHECK_EQ_INT(g_cfg.left[1], l0);
    CHECK_EQ_INT(g_cfg.device[0], d1);
    CHECK_EQ_INT(g_cfg.device[1], d0);
    CHECK_EQ_INT(g_bindingsBannerUntil, g_time - 16 + 10000);
    TapKey(K_VK_T, 1);
    CHECK_EQ_INT(g_cfg.left[0], l0);
    CHECK_EQ_INT(g_cfg.device[0], d0);
}

TEST(ui_Menu_right_arrow_advances_the_attract_screen)
{
    BootMenu();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_INTRO);
    TapKey(K_VK_RIGHT, 1);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_ABOUT);
    CHECK(g_frameFunc == AboutScreen);
    CHECK_EQ_INT(g_idleTimeoutMs, 30000);
    TapKey(K_VK_RIGHT, 1);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_MISSION);
    CHECK(g_frameFunc == MissionScreen);
}

TEST(ui_Menu_F9_opens_the_input_configuration)
{
    BootMenu();
    g_cursor = 5;
    TapKey(K_VK_F9, 1);
    CHECK_EQ_INT(g_state, STATE_INPUT_CONFIG);
    CHECK_EQ_INT(g_cursor, 0);
    CHECK(g_frameFunc == ConfigInputMenu);
}

// ---------------------------------------------------------------------------------------
// Starting games
// ---------------------------------------------------------------------------------------

TEST(ui_Menu_space_starts_a_game)
{
    BootMenu();
    TapKey(K_VK_SPACE, 1);
    CHECK_NE_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_gameMode, MODE_SINGLE);
    CHECK_EQ_INT(g_timeTrialDeadline, g_time - 16 + 181000);
}

TEST(ui_Menu_space_waits_for_the_transition_lock)
{
    BootMenu();
    g_transitionLockUntil = g_time + 1000;
    g_transitionLock = 1;
    TapKey(K_VK_SPACE, 1);
    CHECK_EQ_INT(g_state, STATE_TITLE);
}

TEST(ui_Menu_F2_and_F3_start_two_player_games)
{
    BootMenu();
    TapKey(K_VK_F2, 1);
    CHECK_NE_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_gameMode, MODE_TWO_PLAYER);
}

TEST(ui_Menu_F3_starts_a_duel)
{
    BootMenu();
    TapKey(K_VK_F3, 1);
    CHECK_NE_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_gameMode, MODE_DUAL);
}

TEST(ui_Menu_F1_starts_single_player_at_the_difficulty_table)
{
    BootMenu();
    TapKey(K_VK_3, 1);
    g_hofMode = HOF_EASY;
    g_gameMode = MODE_DUAL;
    TapKey(K_VK_F1, 1);
    CHECK_NE_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_gameMode, MODE_SINGLE);
    CHECK_EQ_INT(g_hofMode, DIFF_HARD);
    CHECK_EQ_INT(g_autoplay, 0);
}

TEST(ui_Menu_idle_timeout_starts_the_demo)
{
    BootMenu();
    CHECK_EQ_INT(g_menuIdleTimeout - g_time, MENU_IDLE_MS - 16);
    g_fake.millis = g_menuIdleTimeout - 20;
    RunFrames(1);
    CHECK_EQ_INT(g_state, STATE_TITLE);
    g_fake.millis = g_menuIdleTimeout + 1;
    RunFrames(1);
    CHECK_NE_INT(g_state, STATE_TITLE);
    CHECK(g_playerUpdateFn == StateDemo);
}

TEST(ui_Menu_click_START_starts_a_game)
{
    BootMenu();
    g_gameMode = MODE_DUAL;
    ClickEntry(MENUID_START);
    CHECK_NE_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_gameMode, MODE_SINGLE);
}

TEST(ui_Menu_START_submenu_shows_on_hover)
{
    BootMenu();
    int start = EntryIndex(MENUID_START);
    int duel = EntryIndex(MENUID_START_2P_DUEL);
    CHECK_EQ_INT(g_menuEntries[duel].visible, 0);
    MoveTo(g_menuEntries[start].x + 4, g_menuEntries[start].y + 4);
    CHECK_EQ_INT(g_menuEntries[start].hover, 1);
    CHECK_EQ_INT(g_menuEntries[duel].visible, 1);
    ClickEntry(MENUID_START_2P_DUEL);
    CHECK_NE_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_gameMode, MODE_DUAL);
}

TEST(ui_Menu_submenu_hides_when_the_mouse_leaves)
{
    BootMenu();
    int start = EntryIndex(MENUID_START);
    int p2 = EntryIndex(MENUID_START_2P);
    MoveTo(g_menuEntries[start].x + 4, g_menuEntries[start].y + 4);
    CHECK_EQ_INT(g_menuEntries[p2].visible, 1);
    MoveTo(400, 300);
    CHECK_EQ_INT(g_menuEntries[p2].visible, 0);
    CHECK_EQ_INT(g_menuEntries[start].hover, 0);
}

// ---------------------------------------------------------------------------------------
// Menu entries and the settings page
// ---------------------------------------------------------------------------------------

TEST(ui_Menu_click_ABOUT_STORY_BONUSES_pick_the_attract_screen)
{
    BootMenu();
    ClickEntry(MENUID_ABOUT);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_ABOUT);
    CHECK(g_frameFunc == AboutScreen);
    ClickEntry(MENUID_STORY);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_MISSION);
    CHECK(g_frameFunc == MissionScreen);
    ClickEntry(MENUID_BONUSES);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HELP_BONUSES);
    CHECK(g_frameFunc == HelpBonuses);
    ClickEntry(MENUID_SETTINGS);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HELP_CONTROLS);
    CHECK(g_frameFunc == HelpControls);
}

TEST(ui_Menu_click_HISCORE_entries_pick_the_table)
{
    BootMenu();
    TapKey(K_VK_4, 1);
    ClickEntry(MENUID_HISCORE);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HALL_OF_FAME);
    CHECK_EQ_INT(g_hofMode, DIFF_ACE);
    int hs = EntryIndex(MENUID_HISCORE);
    MoveTo(g_menuEntries[hs].x + 4, g_menuEntries[hs].y + 4);
    ClickEntry(MENUID_HOF_METEORSTORM);
    CHECK_EQ_INT(g_hofMode, HOF_METEORSTORM);
    MoveTo(g_menuEntries[hs].x + 4, g_menuEntries[hs].y + 4);
    ClickEntry(MENUID_HOF_EASY);
    CHECK_EQ_INT(g_hofMode, HOF_EASY);
}

TEST(ui_Menu_settings_entries_only_take_clicks_on_the_settings_page)
{
    BootMenu();
    int diff = g_cfg.difficulty;
    ClickEntry(MENUID_DIFFICULTY_NEXT);   // hidden on the intro screen
    CHECK_EQ_INT(g_cfg.difficulty, diff);
    OpenSettingsPage();
    CHECK_EQ_INT(g_menuEntries[EntryIndex(MENUID_DIFFICULTY_NEXT)].visible, 1);
    ClickEntry(MENUID_DIFFICULTY_NEXT);
    CHECK_EQ_INT(g_cfg.difficulty, diff + 1);
}

TEST(ui_Menu_difficulty_arrows_clamp)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.difficulty = DIFF_ACE;
    ClickEntry(MENUID_DIFFICULTY_NEXT);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_ACE);
    CHECK_EQ_INT(g_hofMode, DIFF_ACE);
    WaitUnlock();
    ClickEntry(MENUID_DIFFICULTY_PREV);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_HARD);
    CHECK_EQ_INT(g_hofMode, DIFF_HARD);
    ClickEntry(MENUID_DIFFICULTY_PREV);   // locked
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_HARD);
    g_cfg.difficulty = DIFF_EASY;
    WaitUnlock();
    ClickEntry(MENUID_DIFFICULTY_PREV);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_EASY);
}

TEST(ui_Menu_volume_entries_mute_step_and_max)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.musicVolume = 100;
    ClickEntry(MENUID_MUSICVOLUME_UP);
    CHECK_EQ_INT(g_cfg.musicVolume, 104);
    WaitUnlock();
    ClickEntry(MENUID_MUSICVOLUME_DOWN);
    CHECK_EQ_INT(g_cfg.musicVolume, 100);
    WaitUnlock();
    ClickEntry(MENUID_MUSICVOLUME_MAX);
    CHECK_EQ_INT(g_cfg.musicVolume, 255);
    WaitUnlock();
    ClickEntry(MENUID_MUSICVOLUME_MUTE);
    CHECK_EQ_INT(g_cfg.musicVolume, 0);
    WaitUnlock();
    g_cfg.sfxVol = 2;
    ClickEntry(MENUID_SFXVOL_DOWN);
    CHECK_EQ_INT(g_cfg.sfxVol, 0);
    WaitUnlock();
    ClickEntry(MENUID_SFXVOL_MAX);
    CHECK_EQ_INT(g_cfg.sfxVol, 255);
    CHECK_EQ_INT(g_sfxVolTable[200], 200);
    WaitUnlock();
    ClickEntry(MENUID_SFXVOL_UP);
    CHECK_EQ_INT(g_cfg.sfxVol, 255);
    WaitUnlock();
    ClickEntry(MENUID_SFXVOL_MUTE);
    CHECK_EQ_INT(g_cfg.sfxVol, 0);
    CHECK_EQ_INT(g_sfxVolTable[200], 0);
    WaitUnlock();
    g_cfg.musicVol = 253;
    ClickEntry(MENUID_MUSICVOL_UP);
    CHECK_EQ_INT(g_cfg.musicVol, 255);
    WaitUnlock();
    ClickEntry(MENUID_MUSICVOL_DOWN);
    CHECK_EQ_INT(g_cfg.musicVol, 251);
    CHECK_EQ_INT(g_musVolTable[255], 251);
    WaitUnlock();
    ClickEntry(MENUID_MUSICVOL_MUTE);
    CHECK_EQ_INT(g_cfg.musicVol, 0);
    WaitUnlock();
    ClickEntry(MENUID_MUSICVOL_MAX);
    CHECK_EQ_INT(g_cfg.musicVol, 255);
}

TEST(ui_Menu_volume_down_clamps_at_zero)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.musicVolume = 3;
    ClickEntry(MENUID_MUSICVOLUME_DOWN);
    CHECK_EQ_INT(g_cfg.musicVolume, 0);
    WaitUnlock();
    g_cfg.musicVol = 1;
    ClickEntry(MENUID_MUSICVOL_DOWN);
    CHECK_EQ_INT(g_cfg.musicVol, 0);
    WaitUnlock();
    g_cfg.musicVolume = 254;
    ClickEntry(MENUID_MUSICVOLUME_UP);
    CHECK_EQ_INT(g_cfg.musicVolume, 255);
}

TEST(ui_Menu_toggle_entries)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.sfxOn = 1;
    ClickEntry(MENUID_SFX_TOGGLE_NEXT);
    CHECK_EQ_INT(g_cfg.sfxOn, 0);
    WaitUnlock();
    g_cfg.borderMode = BORDER_BLACK;
    ClickEntry(MENUID_BORDER_MODE_PREV);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_ON);
    WaitUnlock();
    ClickEntry(MENUID_BORDER_MODE_NEXT);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_OFF);
    WaitUnlock();
    g_cfg.bgEnabled = 1;
    ClickEntry(MENUID_BG_ENABLED_PREV);
    CHECK_EQ_INT(g_cfg.bgEnabled, 0);
    WaitUnlock();
    g_cfg.collisionDetail = COLLISION_SIMPLE;
    ClickEntry(MENUID_COLLISION_DETAIL_NEXT);
    CHECK_EQ_INT(g_cfg.collisionDetail, COLLISION_NORMAL);
    WaitUnlock();
    ClickEntry(MENUID_COLLISION_DETAIL_PREV);
    CHECK_EQ_INT(g_cfg.collisionDetail, COLLISION_SIMPLE);
    WaitUnlock();
    g_cfg.particlesOn = 1;
    ClickEntry(MENUID_PARTICLES_PREV);
    CHECK_EQ_INT(g_cfg.particlesOn, 0);
    WaitUnlock();
    g_cfg.bulletIntensity = BULLETS_FLARE_FX;
    ClickEntry(MENUID_BULLET_INTENSITY_PREV);
    CHECK_EQ_INT(g_cfg.bulletIntensity, BULLETS_NORMAL);
    WaitUnlock();
    g_cfg.bgStars = 1;
    ClickEntry(MENUID_BG_STARS_NEXT);
    CHECK_EQ_INT(g_cfg.bgStars, 0);
    CHECK(g_fnPtr == DrawStarsGlow);
}

TEST(ui_Menu_brightness_arrows_walk_the_ladder)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.bgTint = BG_BRIGHTNESS_MAX;
    int down[] = { 100, 85, BG_BRIGHTNESS_PRESET, BG_BRIGHTNESS_DEFAULT, BG_BRIGHTNESS_MIN,
                   BG_BRIGHTNESS_MAX };
    for (int i = 0; i < 6; i++) {
        ClickEntry(MENUID_BG_BRIGHTNESS_PREV);
        CHECK_EQ_INT(g_cfg.bgTint, down[i]);
        WaitUnlock();
    }
    ClickEntry(MENUID_BG_BRIGHTNESS_NEXT);
    CHECK_EQ_INT(g_cfg.bgTint, BG_BRIGHTNESS_MIN);
    WaitUnlock();
    ClickEntry(MENUID_BG_BRIGHTNESS_NEXT);
    CHECK_EQ_INT(g_cfg.bgTint, BG_BRIGHTNESS_DEFAULT);
}

TEST(ui_Menu_star_count_arrows)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.numStars = 100;
    ClickEntry(MENUID_NUM_STARS_UP);
    CHECK_NEAR(g_cfg.numStars, 125, 0);
    WaitUnlock();
    ClickEntry(MENUID_NUM_STARS_DOWN);
    CHECK_NEAR(g_cfg.numStars, 100, 0);
    WaitUnlock();
    g_cfg.numStars = 60;
    ClickEntry(MENUID_NUM_STARS_DOWN);
    CHECK_NEAR(g_cfg.numStars, 50, 0);
    WaitUnlock();
    g_cfg.numStars = 2990;
    ClickEntry(MENUID_NUM_STARS_UP);
    CHECK_NEAR(g_cfg.numStars, 3000, 0);
}

TEST(ui_Menu_alien_buffer_arrows_need_a_restart)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.alienBuffer = 5;
    g_restartNeeded = 0;
    ClickEntry(MENUID_ALIEN_BUFFER_DOWN);
    CHECK_EQ_INT(g_cfg.alienBuffer, 5);
    CHECK_EQ_INT(g_restartNeeded, 1);
    WaitUnlock();
    ClickEntry(MENUID_ALIEN_BUFFER_UP);
    CHECK_EQ_INT(g_cfg.alienBuffer, 6);
    WaitUnlock();
    g_cfg.alienBuffer = 100;
    ClickEntry(MENUID_ALIEN_BUFFER_UP);
    CHECK_EQ_INT(g_cfg.alienBuffer, 100);
}

TEST(ui_Menu_spark_arrows)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.sparks = 30;
    ClickEntry(MENUID_SPARKS_UP);
    CHECK_NEAR(g_cfg.sparks, 32, 0);
    CHECK_EQ_INT(g_maxSparks, 16);
    ClickEntry(MENUID_SPARKS_DOWN);
    CHECK_NEAR(g_cfg.sparks, 30, 0);
}

TEST(ui_Menu_shuffle_entry_rebuilds_the_menu_label)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.shuffle = 1;
    ClickEntry(MENUID_TOGGLE_SHUFFLE);
    CHECK_EQ_INT(g_cfg.shuffle, 0);
    CHECK_STR(g_menuEntries[EntryIndex(MENUID_TOGGLE_SHUFFLE)].text, "USE RANDOM MODE");
}

TEST(ui_Menu_screenmode_entry_toggles_fullscreen)
{
    BootMenu();
    OpenSettingsPage();
    g_cfg.windowed = 1;
    ClickEntry(MENUID_TOGGLE_WINDOWED);
    CHECK_EQ_INT(g_cfg.windowed, 0);
    CHECK(g_fake.fullscreen);
}

TEST(ui_Menu_vsync_and_interpolation_entries)
{
    BootMenu();
    OpenSettingsPage();
    bool vsync = g_fake.vsync;
    ClickEntry(MENUID_TOGGLE_VSYNC);
    CHECK_EQ_INT(g_fake.vsync, !vsync);
    CHECK_EQ_INT(g_cfg.vsyncOff, !g_fake.vsync);
    int interp = g_cfg.interpolation;
    ClickEntry(MENUID_TOGGLE_INTERPOLATION);
    CHECK_EQ_INT(g_cfg.interpolation, (interp + 1) % SYS_INTERP_COUNT);
    CHECK_EQ_INT(g_fake.interpolation, g_cfg.interpolation);
    CHECK_EQ_INT(g_fake.vsync, !vsync);
}

TEST(ui_Menu_S_cycles_interpolation_and_alt_V_toggles_vsync)
{
    BootMenu();
    int interp = g_cfg.interpolation;
    TapKey(K_VK_S, 1);
    CHECK_EQ_INT(g_cfg.interpolation, (interp + 1) % SYS_INTERP_COUNT);
    TapKey(K_VK_S, 1);
    CHECK_EQ_INT(g_cfg.interpolation, (interp + 2) % SYS_INTERP_COUNT);
    bool vsync = g_fake.vsync;
    FakePressKey(K_VK_MENU);
    TapKey(K_VK_V, 1);
    CHECK_EQ_INT(g_fake.vsync, !vsync);
}

TEST(ui_Menu_config_entry_opens_the_input_configuration)
{
    BootMenu();
    OpenSettingsPage();
    ClickEntry(MENUID_CONFIG);
    CHECK_EQ_INT(g_state, STATE_INPUT_CONFIG);
}

// ---------------------------------------------------------------------------------------
// The quit dialog
// ---------------------------------------------------------------------------------------

TEST(ui_Menu_escape_opens_the_quit_dialog_and_closes_it_again)
{
    BootMenu();
    TapKey(K_VK_ESCAPE, 1);
    CHECK_EQ_INT(g_quitToWindowsWinOpen, 1);
    int w = g_quitWinWin;
    CHECK_EQ_INT(g_windows[w].active, 1);
    CHECK_STR(g_windows[w].texts[0].text, "QUIT TO WINDOWS?");
    CHECK_EQ_INT(g_windows[w].menuItems[0].id, MENUID_QUIT_TO_WINDOWS_YES);
    CHECK_STR(g_windows[w].menuItems[1].text, "         NO!!         ");
    CHECK_EQ_INT(g_windows[w].menuItems[0].checked, 1);
    TapKey(K_VK_ESCAPE, 1);
    CHECK_EQ_INT(g_windows[w].visible, 0);
    WaitWindowsGone();
    CHECK(!AnyWindowActive());
    CHECK(!g_fake.quit);
}

TEST(ui_Menu_quit_dialog_NO_continues)
{
    BootMenu();
    TapKey(K_VK_ESCAPE, 1);
    TapKey(K_VK_DOWN, 1);
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_quitToWindowsWinOpen, 0);
    CHECK(!g_fake.quit);
    WaitWindowsGone();
    CHECK(!AnyWindowActive());
}

TEST(ui_Menu_quit_dialog_YES_saves_and_terminates)
{
    BootMenu();
    g_sfxGoodbye = 0;   // the goodbye jingle waits on the real clock
    remove(FakeUserPath("warblade\\WarBlade.inf"));
    TapKey(K_VK_ESCAPE, 1);
    TapKey(K_VK_RETURN, 1);   // YES is checked
    CHECK(g_fake.quit);
    CHECK(FakeFileExists(FakeUserPath("warblade\\WarBlade.inf")));
}

TEST(ui_QuitToWindowsDialog_wording_in_a_game)
{
    WinInit();
    g_state = STATE_PAUSED;
    QuitToWindowsDialog();
    CHECK_STR(g_windows[g_quitWinWin].menuItems[1].text, "  NO!! CONTINUE GAME  ");
    CHECK_EQ_INT(g_windows[g_quitWinWin].selF, MENUID_QUIT_TO_WINDOWS_YES);
}

TEST(ui_QuitGameDialog_offers_four_choices)
{
    WinInit();
    QuitGameDialog();
    int w = g_quitGameWin;
    CHECK_EQ_INT(g_windows[w].nF, 3);
    CHECK_EQ_INT(g_windows[w].menuItems[0].id, MENUID_QUIT_GAME_CONFIRM);
    CHECK_EQ_INT(g_windows[w].menuItems[1].id, MENUID_RETIRE_CONFIRM);
    CHECK_EQ_INT(g_windows[w].menuItems[2].id, MENUID_QUIT_GAME_TO_WINDOWS);
    CHECK_EQ_INT(g_windows[w].menuItems[3].id, MENUID_QUIT_GAME_CONTINUE);
    CHECK_EQ_INT(g_windows[w].selF, MENUID_QUIT_GAME_CONFIRM);
    CHECK_EQ_INT(g_windows[w].menuItems[0].checked, 1);
}

TEST(ui_Menu_QUIT_YES_entry_terminates)
{
    BootMenu();
    g_sfxGoodbye = 0;
    int q = EntryIndex(MENUID_QUIT_YES);
    int parent = EntryIndex(g_menuEntries[q].parent);
    MoveTo(g_menuEntries[parent].x + 4, g_menuEntries[parent].y + 4);
    ClickEntry(MENUID_QUIT_YES);
    CHECK(g_fake.quit);
}

// ---------------------------------------------------------------------------------------
// The first-run presets
// ---------------------------------------------------------------------------------------

static void PickPreset(int index)
{
    BootGame();
    for (int i = 0; i <= index; i++)
        TapKey(K_VK_DOWN, 1);
    TapKey(K_VK_RETURN, 1);
}

TEST(ui_Menu_preset_very_old_pc)
{
    PickPreset(0);
    CHECK_NEAR(g_cfg.numStars, 100, 0);
    CHECK_NEAR(g_cfg.sparks, 15, 0);
    CHECK_EQ_INT(g_maxSparks, 7);
    CHECK_EQ_INT(g_cfg.collisionDetail, COLLISION_SIMPLE);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_OFF);
    CHECK_EQ_INT(g_cfg.particlesOn, 0);
    CHECK_EQ_INT(g_cfg.sfxOn, 0);
    CHECK_EQ_INT(g_cfg.bgStars, 1);
    CHECK(g_fnPtr == DrawStarsPlayer);
    CHECK_EQ_INT(g_restartNeeded, 1);
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_MOD);
}

TEST(ui_Menu_preset_old_pc)
{
    PickPreset(1);
    CHECK_NEAR(g_cfg.numStars, 300, 0);
    CHECK_NEAR(g_cfg.sparks, 30, 0);
    CHECK_EQ_INT(g_maxSparks, 15);
    CHECK_EQ_INT(g_cfg.collisionDetail, COLLISION_NORMAL);
    CHECK_EQ_INT(g_cfg.bgEnabled, 0);
}

TEST(ui_Menu_preset_normal_pc)
{
    PickPreset(2);
    CHECK_NEAR(g_cfg.numStars, 750, 0);
    CHECK_NEAR(g_cfg.sparks, 100, 0);
    CHECK_EQ_INT(g_maxSparks, 50);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_ON);
    CHECK_EQ_INT(g_cfg.particlesOn, 1);
    CHECK_EQ_INT(g_cfg.sfxOn, 1);
    CHECK_EQ_INT(g_cfg.bgStars, 0);
    CHECK(g_fnPtr == DrawStarsGlow);
    CHECK_EQ_INT(g_cfg.bulletIntensity, BULLETS_NORMAL);
}

TEST(ui_Menu_preset_very_powerful_pc)
{
    PickPreset(3);
    CHECK_NEAR(g_cfg.numStars, 1000, 0);
    CHECK_NEAR(g_cfg.sparks, 150, 0);
    CHECK_EQ_INT(g_maxSparks, 75);
    CHECK_EQ_INT(g_cfg.bulletIntensity, BULLETS_FLARE_FX);
    CHECK_EQ_INT(g_cfg.bgEnabled, 1);
    CHECK_EQ_INT(g_cfg.sfxVol, 204);
    CHECK_EQ_INT(g_cfg.musicVolume, 179);
    CHECK_EQ_INT(g_cfg.alienBuffer, 5);
}

// ---------------------------------------------------------------------------------------
// Profiles: the list window, creating one, logging in
// ---------------------------------------------------------------------------------------

static void Type(const char *s)
{
    for (; *s; s++) {
        enum EKeyboardLayout k;
        if (*s >= 'A' && *s <= 'Z')
            k = K_VK_A + (*s - 'A');
        else if (*s >= '0' && *s <= '9')
            k = K_VK_0 + (*s - '0');
        else
            k = K_VK_SPACE;
        TapKey(k, 1);
    }
}

static int TopWindow(void)
{
    int top = -1;
    for (int w = 0; w < MAX_WINDOWS; w++)
        if (g_windows[w].active && g_windows[w].visible)
            top = w;
    return top;
}

static int ItemIndex(int w, int id)
{
    for (int k = 0; k <= g_windows[w].nF; k++)
        if (g_windows[w].menuItems[k].id == id)
            return k;
    TestFail(__FILE__, __LINE__, "no item %d in window %d", id, w);
    return -1;
}

static void ClickItem(int w, int id)
{
    RunFrames(30);   // slid in and laid out
    int k = ItemIndex(w, id);
    int x = g_windows[w].x + g_windows[w].menuItems[k].x + g_windows[w].menuItems[k].w / 2;
    int y = g_windows[w].y + g_windows[w].menuItems[k].y + g_windows[w].menuItems[k].h / 2;
    MoveTo(x, y);
    g_fake.mouseLeft = true;
    RunFrames(1);
    g_fake.mouseLeft = false;
    RunFrames(1);
}

TEST(ui_Menu_A_toggles_the_profile_list)
{
    BootMenu();
    TapKey(K_VK_A, 1);
    int w = TopWindow();
    CHECK(w >= 0);
    CHECK_EQ_INT(g_profileWin, w);
    CHECK_STR(g_windows[w].texts[0].text, "USER PROFILES");
    CHECK_EQ_INT(g_windows[w].nF, 1);   // NEW, CLOSE
    CHECK_EQ_INT(g_windows[w].menuItems[0].id, MENUID_PROFILE_NEW);
    CHECK_EQ_INT(g_windows[w].menuItems[1].id, MENUID_CLOSE);
    CHECK_EQ_INT(g_windows[w].selF, MENUID_CLOSE);
    TapKey(K_VK_A, 1);
    CHECK_EQ_INT(g_windows[w].visible, 0);
}

TEST(ui_Menu_profile_list_close_button)
{
    BootMenu();
    TapKey(K_VK_A, 1);
    int w = TopWindow();
    TapKey(K_VK_RETURN, 1);   // CLOSE is selected
    CHECK_EQ_INT(g_windows[w].visible, 1);   // WinClose on a sliding window only starts the slide
    CHECK_NEAR(g_windows[w].slideY, 1.2, 1e-6);
    WaitWindowsGone();
    CHECK(!AnyWindowActive());
}

static int OpenNewProfileWindow(void)
{
    TapKey(K_VK_A, 1);
    TapKey(K_VK_DOWN, 1);     // NEW
    TapKey(K_VK_RETURN, 1);
    return TopWindow();
}

TEST(ui_Menu_new_profile_window_has_two_fields)
{
    BootMenu();
    int w = OpenNewProfileWindow();
    CHECK_STR(g_windows[w].texts[0].text, "NEW PROFILE");
    CHECK_EQ_INT(g_windows[w].nH, 1);
    CHECK_EQ_INT(g_windows[w].edits[0].len, 29);
    CHECK_EQ_INT(g_windows[w].edits[1].len, 15);
    CHECK_EQ_INT(g_windows[w].edits[1].masked, 1);
    CHECK_EQ_INT(g_windows[w].firstH, 0);
    CHECK_EQ_INT(g_windows[w].selF, MENUID_NEW_PROFILE_OK);
    CHECK(AnyWindowHasEdit());
}

TEST(ui_Menu_typing_fills_the_focused_field)
{
    BootMenu();
    int w = OpenNewProfileWindow();
    // NOTE: nothing in the menu ever re-arms the space key's latch (ResetFlags covers only
    // A-Z and 0-9), so spaces can't be typed at all; arm it to see the leading-space rule.
    g_keyLatch[K_VK_SPACE] = 1;
    Type(" ACE7");   // a leading space is dropped
    CHECK_STR(g_windows[w].edits[0].buf, "ACE7");
    CHECK_EQ_INT(g_windows[w].edits[0].cursor, 4);
    TapKey(K_VK_BACK, 1);
    CHECK_STR(g_windows[w].edits[0].buf, "ACE");
    CHECK_EQ_INT(g_windows[w].edits[0].cursor, 3);
    TapKey(K_VK_TAB, 1);
    CHECK_EQ_INT(g_windows[w].firstH, 1);
    Type("PW");
    CHECK_STR(g_windows[w].edits[1].buf, "PW");
    CHECK_STR(g_windows[w].edits[0].buf, "ACE");
    TapKey(K_VK_RETURN, 1);   // two fields: Enter moves the focus
    CHECK_EQ_INT(g_windows[w].firstH, 0);
}

TEST(ui_Menu_typing_stops_at_the_field_length)
{
    BootMenu();
    int w = OpenNewProfileWindow();
    TapKey(K_VK_TAB, 1);
    Type("ABCDEFGHIJKLMNOPQ");   // 17 letters into 15
    CHECK_STR(g_windows[w].edits[1].buf, "ABCDEFGHIJKLMNO");
    CHECK_EQ_INT(g_windows[w].edits[1].cursor, 15);
}

TEST(ui_Menu_hotkeys_are_off_while_typing)
{
    BootMenu();
    OpenNewProfileWindow();
    int diff = g_cfg.difficulty;
    float stars = g_cfg.numStars;
    int vol = g_cfg.sfxVol;
    Type("4N");
    FakePressKey(K_VK_PAGEUP);
    RunFrames(3);
    FakeReleaseKey(K_VK_PAGEUP);
    CHECK_EQ_INT(g_cfg.difficulty, diff);
    CHECK_NEAR(g_cfg.numStars, stars, 0);
    CHECK_EQ_INT(g_cfg.sfxVol, vol);
}

TEST(ui_Menu_create_a_normal_profile)
{
    BootMenu();
    int w = OpenNewProfileWindow();
    Type("PILOT");
    TapKey(K_VK_TAB, 1);
    Type("SECRET");
    ClickItem(w, MENUID_NEW_PROFILE_OK);
    CHECK(FakeFileExists(FakeUserPath("warblade\\profiles\\profile000.acc")));
    CHECK_EQ_INT(g_profileCount, 1);
    int list = TopWindow();
    CHECK_EQ_INT(g_profileWin, list);
    CHECK_STR(g_windows[list].texts[0].text, "USER PROFILES");
    CHECK(strstr(g_windows[list].texts[1].text, "PILOT") != NULL);
    CHECK_EQ_INT(g_windows[list].menuItems[0].id, 30);
    CHECK_STR(g_windows[list].menuItems[0].text, "USE PROFILE");
    CHECK_EQ_INT(g_windows[list].menuItems[1].id, 3000);
    CHECK_EQ_INT(GetProfileEasyFlag(0), 0);
}

TEST(ui_Menu_new_profile_needs_name_and_password)
{
    BootMenu();
    int w = OpenNewProfileWindow();
    Type("PILOT");
    ClickItem(w, MENUID_NEW_PROFILE_OK);
    CHECK(!FakeFileExists(FakeUserPath("warblade\\profiles\\profile000.acc")));
    CHECK_EQ_INT(g_profileCount, 0);
    CHECK_EQ_INT(TopWindow(), w);
}

TEST(ui_Menu_create_an_easy_profile)
{
    BootMenu();
    int w = OpenNewProfileWindow();
    Type("ROOKIE");
    TapKey(K_VK_TAB, 1);
    Type("PW");
    ClickItem(w, MENUID_NEW_PROFILE_EASY);
    int warn = TopWindow();
    CHECK_STR(g_windows[warn].texts[0].text, "W A R N I N G");
    CHECK_STR(g_newName, "ROOKIE");
    CHECK_STR(g_newPass, "PW");
    ClickItem(warn, MENUID_EASY_PROFILE_YES);
    CHECK_EQ_INT(g_profileCount, 1);
    CHECK(FakeFileExists(FakeUserPath("warblade\\profiles\\profile000.acc")));
    CHECK_EQ_INT(GetProfileEasyFlag(0), 1);
}

static void CreateProfile(const char *name, const char *pass)
{
    int w = OpenNewProfileWindow();
    Type(name);
    TapKey(K_VK_TAB, 1);
    Type(pass);
    ClickItem(w, MENUID_NEW_PROFILE_OK);
}

TEST(ui_Menu_use_profile_logs_in_with_the_password)
{
    BootMenu();
    CreateProfile("PILOT", "PW12");
    int list = TopWindow();
    ClickItem(list, 30);   // USE PROFILE, slot 0
    int login = TopWindow();
    CHECK_STR(g_windows[login].texts[0].text, "USE PROFILE");
    CHECK_EQ_INT(g_selProfile, 0);
    CHECK_EQ_INT(g_profileIndex, -1);
    Type("PW12");
    TapKey(K_VK_RETURN, 1);   // one field: Enter submits (LOGIN OK)
    CHECK_EQ_INT(g_profileIndex, 0);
    CHECK_EQ_INT(g_loggedIn, 1);
}

TEST(ui_Menu_wrong_password_clears_the_field)
{
    BootMenu();
    CreateProfile("PILOT", "PW12");
    ClickItem(TopWindow(), 30);
    int login = TopWindow();
    Type("PW13");
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_profileIndex, -1);
    CHECK_EQ_INT(g_loggedIn, 0);
    CHECK_STR(g_windows[login].edits[0].buf, "");
    CHECK_EQ_INT(g_windows[login].edits[0].cursor, 0);
    Type("PW1");   // a prefix isn't enough either
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_profileIndex, -1);
}

TEST(ui_Menu_easy_profile_forces_easy_difficulty_on_login)
{
    BootMenu();
    int w = OpenNewProfileWindow();
    Type("ROOKIE");
    TapKey(K_VK_TAB, 1);
    Type("PW");
    ClickItem(w, MENUID_NEW_PROFILE_EASY);
    ClickItem(TopWindow(), MENUID_EASY_PROFILE_YES);
    TapKey(K_VK_3, 1);   // the list window is open: ignored
    ClickItem(TopWindow(), 30);
    Type("PW");
    TapKey(K_VK_RETURN, 1);
    CHECK_EQ_INT(g_profileIndex, 0);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_EASY);
    CHECK_EQ_INT(g_hofMode, HOF_EASY);
    CHECK_EQ_INT(g_cfg.fps, FPS_EASY);
}
