// Tests for src/core/main.c (GameMain: start-up, main loop, shutdown; GameInit; the focus
// hook; the crash report is in test_core_platform.c) and the start-up half of
// src/core/init.c (InitGame, LoadGameData, Shutdown), through the real boot.
#include <stdlib.h>
#include "support.h"

static const char *ReadUserFile(const char *rel)
{
    static char buf[16384];
    buf[0] = 0;
    FILE *f = fopen(FakeUserPath(rel), "rb");
    if (!f)
        return buf;
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = 0;
    fclose(f);
    return buf;
}

static const char *DebugLog(void)
{
    return ReadUserFile("warblade\\warblade.dbg");
}

static bool EndsWith(const char *s, const char *tail)
{
    size_t a = strlen(s), b = strlen(tail);
    return a >= b && strcmp(s + a - b, tail) == 0;
}

// A settings file as the game writes it (defaults, then `tweak`), so the boot takes the
// "valid settings" path (no presets prompt, the windowed byte honoured).
static void WriteSettingsFile(void (*tweak)(void))
{
    MakeGameDir();
    LoadSettings();     // no file: the defaults
    if (tweak)
        tweak();
    WriteSettings();
    memset(&g_cfg, 0, sizeof g_cfg);
}

#define BOOT_LOG                                                                   \
    "************ Warblade Debug information ************\r\n"                    \
    "Warblade SR 2.0, Copyright 1999-2009 Edgar M Vigdal\r\n"                  \
    "Hide mouse curosr is passed...\r\n"                                           \
    "Main window is created...\r\n"                                                \
    "Window is updated...\r\n"                                                     \
    "Pixel shader is initiated...\r\n"                                             \
    "Cursor is killed...\r\n"                                                      \
    "Loading of attack patterns is passed...\r\n"                                  \
    "Statistics loaded is passed...\r\n"                                           \
    "InitApplication is passed...\r\n"                                             \
    "Init game is entered...\r\n"                                                  \
    "Renderer : fake\n"                                                            \
    "Screens zeroed passed...\r\n"                                                 \
    "Restore Surfaces passed...\r\n"                                               \
    "Init fighter passed...\r\n"                                                   \
    "Set volum passed...\r\n"                                                      \
    "All samples loaded is passed...\r\n"                                          \
    "Setting Variables is passed...\r\n"                                           \
    "Init joysticks is passed...\r\n"                                              \
    "Erase all screens is passed...\r\n"                                           \
    "Calc time span is passed...\r\n"                                              \
    "Preoading of all level data is passed...\r\n"                                 \
    "EMV Software splash is passed...\r\n"                                         \
    "Game splash is passed...\r\n"                                                 \
    "Default account? is passed...\r\n"

#define SHUTDOWN_LOG_TAIL                                                          \
    "\r\n*** EXITING WARBLADE ***\r\n"                                             \
    "CLOSE MUSIC\r\n"                                                              \
    "FREE SAMPLES\r\n"                                                             \
    "SOUNDSYSTEM : Closing down...\r\n"                                            \
    "# of alien gfx allocated : 0 \n"                                              \
    "# of alien gfx released : 0 \n"                                               \
    "# of alien hit gfx allocated : 0 \n"                                          \
    "# of alien hit gfx released : 0 \n"                                           \
    "# of alien hitmasks allocated : 0 \n"                                         \
    "# of alien hitmasks released : 0 \n"                                          \
    "\n\n"                                                                         \
    "# of alien gfx released at exit: 0 \n"                                        \
    "# of alien hit gfx released at exit : 0 \n"                                   \
    "# of alien hitmasks released at exit: 0 \n"

// ---------------------------------------------------------------- the boot

TEST(core_BootGame_reaches_the_title_screen)
{
    BootGame();
    CHECK_EQ_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_saveMagic, 12345);
    CHECK(g_frameFunc == IntroFrame);
    CHECK_EQ_INT(g_gameMode, MODE_SINGLE);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_NORMAL);
    CHECK_EQ_INT(g_cfg.fps, 60);
    CHECK_EQ_INT(g_bgIndex, 1);
    CHECK_EQ_INT(g_introInit, 1);
    CHECK_EQ_INT(g_viewTransitionFlag, 2);
    CHECK_EQ_INT(g_musicRestartTime, g_time + 1500);
    CHECK_EQ_INT(g_menuIdleTimeout, g_time + MENU_IDLE_MS);
    CHECK(g_fake.pointerHidden);
    CHECK_EQ_INT(g_fake.maxFps, 60);
}

TEST(core_BootGame_writes_the_start_up_log)
{
    BootGame();
    CHECK(FakeFileExists(FakeUserPath("warblade")));
    CHECK_STR(DebugLog(), BOOT_LOG);
}

TEST(core_BootGame_hooks_up_the_frame_functions)
{
    BootGame();
    CHECK(g_stateFn == SetViewHud);
    CHECK(g_drawHudFn == DrawHud1P);
    CHECK(g_drawBordersFn == DrawBorders);
    CHECK(g_drawLevelObjectsFn == DrawLevelObjectsNormal);
    CHECK(g_playerUpdateFn == UpdatePlayer);
    CHECK(g_itemsVsPlayerFn == ItemsVsPlayer);
    CHECK(g_bulletsVsPlayerFn == BulletsVsPlayer);
    CHECK(g_grabEnemyFn == ShieldGrabEnemies);
    CHECK(g_shipHudFn == ShipHud);
    CHECK(g_fnPtr == DrawStarsStill);   // the default settings have still stars
}

static void BrightGlow(void)
{
    g_cfg.bulletIntensity = 1;
    g_cfg.bgStars = 0;
}

TEST(core_BootGame_bright_bullets_and_glowing_stars_from_settings)
{
    WriteSettingsFile(BrightGlow);
    BootGame();
    CHECK(g_drawLevelObjectsFn == DrawLevelObjectsBright);
    CHECK(g_fnPtr == DrawStarsGlow);
}

TEST(core_BootGame_first_run_uses_default_settings)
{
    BootGame();
    // no WarBlade.inf: the defaults, and the hardware presets prompt
    CHECK_EQ_INT(g_cfg.sfxVol, 0xcc);
    CHECK_EQ_INT(g_cfg.profileSel, -1);
    CHECK_EQ_INT(g_cfg.renderer, RENDERER_AUTO);
    CHECK_EQ_INT(g_presets, 1);
    CHECK_EQ_INT(g_windowed, 0);
    // fullscreen: the cursor is clipped to the window
    CHECK(g_fake.pointerClipped);
}

TEST(core_BootGame_with_settings_skips_the_presets_prompt)
{
    WriteSettingsFile(NULL);
    BootGame();
    CHECK_EQ_INT(g_presets, 0);
    CHECK_EQ_INT(g_windowed, 1);
    CHECK_EQ_INT(g_fake.clipPointerCalls, 0);
}

TEST(core_BootGame_shift_opens_the_presets_prompt)
{
    WriteSettingsFile(NULL);
    FakePressKey(K_VK_R_SHIFT);
    BootGame();
    CHECK_EQ_INT(g_presets, 1);
}

TEST(core_BootGame_left_shift_opens_the_presets_prompt)
{
    WriteSettingsFile(NULL);
    FakePressKey(K_VK_L_SHIFT);
    BootGame();
    CHECK_EQ_INT(g_presets, 1);
}

TEST(core_BootGame_counts_joysticks)
{
    g_fake.joyPresent[1] = true;
    BootGame();
    CHECK_EQ_INT(g_joyCount, 1);
    CHECK_EQ_INT(g_joy0, 0);
    CHECK_EQ_INT(g_joy1, 1);
}

TEST(core_BootGame_counts_both_joysticks)
{
    g_fake.joyPresent[0] = true;
    g_fake.joyPresent[1] = true;
    BootGame();
    CHECK_EQ_INT(g_joyCount, 2);
    CHECK_EQ_INT(g_joy0, 1);
    CHECK_EQ_INT(g_joy1, 1);
}

TEST(core_BootGame_game_init_defaults)
{
    BootGame();
    CHECK_EQ_INT(g_diffEnemyFireChance, 6);
    CHECK_EQ_INT(g_diffShotFuseBase, 230);
    CHECK_EQ_INT(g_diffShotFuseRange, 240);
    CHECK_EQ_INT(g_hurryUpInterval, 120);
    CHECK_NEAR(g_diffTurretTrackChance, 20.0, 0);
    CHECK_NEAR(g_diffBonusDropRoll, 17.0, 0);
    CHECK_NEAR(g_diffEnemyTimerMul, 2.2, 1e-6);
    CHECK_EQ_INT(g_diffEnemyHpBonus, 75);
    CHECK_EQ_INT(g_moneySuckerBaseHp, 500);
    CHECK_EQ_INT(g_eliteHpBonus, 1500);
    CHECK_NEAR(g_speedBase, 4.0, 0);
    CHECK_NEAR(g_speedStep, 0.75, 0);
    CHECK_EQ_INT(g_bonusThresholdBase, 30000);
    CHECK_EQ_INT(g_fireDelayMin, 200);
    CHECK_EQ_INT(g_loadedLevel, -1);
    CHECK_EQ_INT(g_warpLevelR, -1);
    CHECK_EQ_INT(g_warpLevelL, -1);
    CHECK_EQ_INT(g_levelDataLoaded, 1);
}

TEST(core_BootGame_init_game_opens_the_window_and_title_music)
{
    BootGame();
    CHECK_STR(g_songName, "title");
    CHECK_EQ_INT(g_fake.windowsCreated, 1);
    CHECK_STR(g_fake.lastRenderDriver, "");
}

// ---------------------------------------------------------------- tables built at start-up

TEST(core_BootGame_builds_the_degree_tables)
{
    BootGame();
    // NOTE: the names are swapped: g_cosDeg holds sines, g_sinDeg cosines
    CHECK_NEAR(g_cosDeg[0], 0.0, 1e-6);
    CHECK_NEAR(g_cosDeg[90], 1.0, 1e-6);
    CHECK_NEAR(g_cosDeg[30], 0.5, 1e-6);
    CHECK_NEAR(g_sinDeg[0], 1.0, 1e-6);
    CHECK_NEAR(g_sinDeg[60], 0.5, 1e-6);
    CHECK_NEAR(g_sinDeg[180], -1.0, 1e-6);
    CHECK_NEAR(g_cosDeg[359], sin(359 * M_PI / 180), 1e-6);
    // the 0.1-degree tables too
    CHECK_NEAR(g_sinTable[900], 1.0, 1e-5);
    CHECK_NEAR(g_sinTableFine[900], 1.0, 1e-5);
    CHECK(g_shipDefs[5] == (ShipDef *)g_shipStats5);
}

TEST(core_BootGame_builds_the_grab_and_colour_tables)
{
    BootGame();
    CHECK_NEAR(g_grabZoneWidthTable[0], 4.0, 0);
    CHECK_NEAR(g_grabZoneWidthTable[10], 9.0, 0);
    CHECK_NEAR(g_grabZoneWidthTable[99], 53.5, 0);
    int seen[7] = {0};
    for (int i = 0; i < 512; i++) {
        CHECK_MSG(g_perfectColorTable[i] >= 0 && g_perfectColorTable[i] < 7,
                  "g_perfectColorTable[%d] = %d", i, g_perfectColorTable[i]);
        seen[g_perfectColorTable[i]]++;
    }
    for (int c = 0; c < 7; c++)
        CHECK(seen[c] > 0);
}

TEST(core_BootGame_scales_the_bonus_volumes)
{
    BootGame();
    // the largest sprite (160 x 193) maps to 800, clamped to 255
    CHECK_EQ_INT(g_bonusGfxArea[0], 64 * 53);
    CHECK_EQ_INT(g_bonusGfxArea[9], 160 * 193);
    CHECK_EQ_INT(g_bonusVolume[9], 255);
    CHECK_EQ_INT(g_bonusVolume[0], (int)(64 * 53 * (float)(800.0 / 30880)));
    CHECK_EQ_INT(g_bonusVolume[0], 87);
    CHECK_EQ_INT(g_bonusVolume[1], 21);     // 32 x 26
    CHECK_EQ_INT(g_bonusVolume[40], 13);    // 32 x 16
    CHECK_EQ_INT(g_bonusVolume[19], 255);   // 144 x 136 -> 507
}

TEST(core_BootGame_seeds_the_starfield)
{
    BootGame();
    int star1 = 0, others = 0;
    for (int i = 0; i < NUM_STARS; i++) {
        CHECK(g_stars[i].y >= -550.0f && g_stars[i].y <= 600.0f);
        CHECK(g_stars[i].x >= 0.0f && g_stars[i].x <= 800.0f);
        CHECK(g_stars[i].z >= 1.0f && g_stars[i].z <= 16.0f);
        CHECK(g_starA[i] >= 1.0f && g_starA[i] <= 100.0f);
        if (g_starGfx[i] == g_gfxStar1)
            star1++;
        else if (g_starGfx[i] == g_starSprite || g_starGfx[i] == g_gfxStar3)
            others++;
        else
            CHECK_MSG(0, "star %d has no sprite", i);
        // the fast band (60-100) is the 2% that get the big sprites
        if (g_starA[i] >= 60.0f)
            CHECK(g_starGfx[i] != g_gfxStar1);
    }
    CHECK(others > 0 && others < NUM_STARS / 20);
    CHECK_EQ_INT(star1 + others, NUM_STARS);
}

TEST(core_BootGame_builds_the_flash_ramps)
{
    BootGame();
    float *wr = &g_colR[0].step0, *wg = &g_colG[0].step0, *wb = &g_colB[0].step0;
    float *cr = &g_colR[1].step0, *cg = &g_colG[1].step0;
    float *og = &g_colG[2].step0, *ob = &g_colB[2].step0, *orr = &g_colR[2].step0;
    CHECK_NEAR(wr[0], 1.0, 0);
    CHECK_NEAR(wg[15], 0.5, 1e-5);
    CHECK_NEAR(wb[29], 1.0 / 30, 1e-5);
    CHECK_NEAR(cr[0], 0.0, 0);
    CHECK_NEAR(cr[29], 0.0, 0);
    CHECK_NEAR(cg[0], 1.0, 0);
    CHECK_NEAR(cg[3], 0.9, 1e-5);
    CHECK_NEAR(orr[6], 0.8, 1e-5);
    CHECK_NEAR(og[0], 0.5, 0);
    CHECK_NEAR(og[6], 0.4, 1e-5);
    CHECK_NEAR(og[29], 0.5 - 29.0 / 60, 1e-5);
    CHECK_NEAR(ob[10], 0.0, 0);
}

TEST(core_BootGame_flare_table)
{
    BootGame();
    CHECK(g_gfxTable[0] == g_gfxFlare1);
    CHECK(g_gfxTable[10] == g_gfxFlare11);
    CHECK(g_gfxTable[20] == g_gfxFlare21);
    CHECK(g_gfxTable[21] == g_gfxFlarePlanet);   // there is no flare22
    CHECK(g_gfxTable[22] == g_gfxFlare23);
    CHECK(g_gfxTable[33] == g_gfxFlare34);
    CHECK_STR(FakeImageName(g_gfxTable[21]), "flareplanet.tga");
    CHECK(strstr(FakeImageName(g_gfxFlare11), "flare11.tga") != NULL);
    // NOTE: entry 34 is set before g_gfxFlareSpark is loaded, so on the first start-up it
    // is NULL (as in the original)
    CHECK(g_gfxTable[34] == NULL);
    CHECK(g_gfxFlareSpark != NULL);
}

// ---------------------------------------------------------------- the whole program

TEST(core_GameMain_runs_until_quit_and_shuts_down)
{
    g_fake.quitAfterFlips = 1000;
    CHECK_EQ_INT(GameMain(), 1);
    CHECK(g_fake.flips >= 1000);
    CHECK_STR(DebugLog(), BOOT_LOG SHUTDOWN_LOG_TAIL);
}

static int s_countersFlip;
static void SetCountersLate(void)
{
    // once the main loop runs, pretend levels loaded some alien graphics
    if (g_saveMagic == 12345 && ++s_countersFlip == 1) {
        g_gfxLoaded = 3;
        g_alienGfxFreedA = 2;
        g_gfx2Loaded = 5;
        g_alienGfxFreedB = 1;
        g_hmaLoaded = 7;
        g_alienGfxFreedM = 6;
        g_freedA = 9;
        g_freedB = 8;
        g_freedC = 4;
        // and hit masks for Shutdown to free
        g_ship1Hma = malloc(16);
        g_hmaGuard = malloc(16);
        g_hmaRocket = malloc(16);
        g_fake.quit = true;
    }
}

TEST(core_GameMain_logs_the_alien_counters_at_exit)
{
    g_fake.onFlip = SetCountersLate;
    CHECK_EQ_INT(GameMain(), 1);
    const char *log = DebugLog();
    CHECK_MSG(EndsWith(log,
                       "# of alien gfx allocated : 3 \n"
                       "# of alien gfx released : 2 \n"
                       "# of alien hit gfx allocated : 5 \n"
                       "# of alien hit gfx released : 1 \n"
                       "# of alien hitmasks allocated : 7 \n"
                       "# of alien hitmasks released : 6 \n"
                       "\n\n"
                       "# of alien gfx released at exit: 9 \n"
                       "# of alien hit gfx released at exit : 8 \n"
                       "# of alien hitmasks released at exit: 4 \n"),
              "log ends: %s", log + strlen(log) - 300);
    CHECK(g_ship1Hma == NULL);
    CHECK(g_hmaGuard == NULL);
    CHECK(g_hmaRocket == NULL);
}

TEST(core_GameMain_quit_before_the_splashes_skips_them)
{
    g_fake.quit = true;
    CHECK_EQ_INT(GameMain(), 1);
    // only the loading screen and start-up flips: no splash loop, no main loop
    CHECK(g_fake.flips < 10);
    CHECK_EQ_INT(g_saveMagic, 12345);
    const char *log = DebugLog();
    CHECK(strstr(log, "EMV Software splash is passed...\r\nGame splash is passed...\r\n") != NULL);
    CHECK(EndsWith(log, SHUTDOWN_LOG_TAIL));
    CHECK(g_splash == NULL);
}

TEST(core_GameMain_fails_when_graphics_dont_load)
{
    g_fake.imageLoadFails = true;
    CHECK_EQ_INT(GameMain(), 0);
    const char *log = DebugLog();
    CHECK(strstr(log, "Init game is entered...\r\n") != NULL);
    CHECK(strstr(log, "Screens zeroed passed...\r\n") != NULL);
    CHECK(strstr(log, "Restore Surfaces passed") == NULL);
    CHECK_EQ_INT(g_saveMagic, 0);
}

TEST(core_GameMain_fails_without_a_window)
{
    g_fake.createWindowFails = 1;
    CHECK_EQ_INT(GameMain(), 0);
    const char *log = DebugLog();
    CHECK(EndsWith(log, "ERROR :  Could not open window\nOpenScreen Failed"));
    CHECK_EQ_INT(g_fake.imagesLoaded, 0);
}

// ---------------------------------------------------------------- the focus hook

TEST(core_OnFocusChange_pauses_gameplay)
{
    g_soundEnabled = 1;
    g_state = STATE_PLAYING;
    OnFocusChange(false);
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    CHECK_EQ_INT(g_savedState, STATE_PLAYING);
    CHECK_EQ_INT(g_pauseCount, 1);
    CHECK_EQ_INT(g_soundPaused, 1);
    CHECK(!g_fake.pointerHidden);
    OnFocusChange(true);
    CHECK_EQ_INT(g_state, STATE_PAUSED);    // stays paused
    CHECK_EQ_INT(g_soundPaused, 0);
    CHECK(g_fake.pointerHidden);
}

TEST(core_OnFocusChange_only_pauses_sound_in_menus)
{
    static const int menus[] = {STATE_PAUSED, STATE_TITLE, STATE_HISCORE_TABLE,
                                STATE_POST_ROUND_IDLE, STATE_ENTER_HISCORE, STATE_INPUT_CONFIG,
                                STATE_END_SEQUENCE, STATE_UNUSED_21};
    g_soundEnabled = 1;
    for (unsigned i = 0; i < sizeof menus / sizeof menus[0]; i++) {
        g_state = menus[i];
        g_soundPaused = 0;
        OnFocusChange(false);
        CHECK_MSG(g_state == menus[i], "state %d became %d", menus[i], g_state);
        CHECK_EQ_INT(g_soundPaused, 1);
        CHECK_EQ_INT(g_pauseCount, 0);
    }
}

TEST(core_OnFocusChange_pauses_every_game_state)
{
    static const int games[] = {STATE_PLAYING, STATE_SHOP, STATE_BONUS_RACE,
                                STATE_MEMORY_STATION, STATE_RESPAWN, STATE_MALFUNCTION,
                                STATE_METEOR_STORM, STATE_GEM_DROP, STATE_SHOP_GATE,
                                STATE_GET_READY};
    for (unsigned i = 0; i < sizeof games / sizeof games[0]; i++) {
        g_state = games[i];
        OnFocusChange(false);
        CHECK_MSG(g_state == STATE_PAUSED, "state %d not paused", games[i]);
        CHECK_EQ_INT(g_savedState, games[i]);
    }
}

TEST(core_OnFocusChange_after_boot_on_the_title_screen)
{
    BootGame();
    int sound = g_soundEnabled;
    OnFocusChange(false);
    CHECK_EQ_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_soundPaused, sound ? 1 : 0);
    CHECK(!g_fake.pointerHidden);
    RunFrames(3);
    CHECK_EQ_INT(g_state, STATE_TITLE);
}

TEST(core_GameMain_focus_hook_installed)
{
    BootGame();
    CHECK(g_fake.focusCallback == OnFocusChange);
    // losing focus in the middle of a game pauses it
    g_state = STATE_PLAYING;
    g_fake.focusCallback(false);
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    CHECK_EQ_INT(g_savedState, STATE_PLAYING);
}
