// Tests for src/ui/title.c (splashes, logo flashes, menu prompt, the attract-mode cycle,
// ResetToTitle), src/game/hud.c (score popups, rows, FPS, boss bar), src/gfx/particles.c,
// explosions.c, stars.c, frames.c and resources.c.
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "support.h"

static void Screen(void)
{
    g_screenW = 800;
    g_screenH = 600;
    ResetClip();
    CLEAR_DRAW_COUNTERS();
}

// ---------------------------------------------------------------------------------------
// title.c
// ---------------------------------------------------------------------------------------

TEST(ui_AddLogoFlash_takes_free_slots_until_full)
{
    SeedRand(1);
    ClearFlashes();
    AddLogoFlash(100, 200);
    CHECK_EQ_INT(g_logoFlashes[0].active, 1);
    CHECK_NEAR(g_logoFlashes[0].x, 100, 0);
    CHECK_NEAR(g_logoFlashes[0].y, 200, 0);
    CHECK_EQ_INT(g_logoFlashes[0].r, 255);
    CHECK(g_logoFlashes[0].life >= 100 && g_logoFlashes[0].life <= 200);
    CHECK(g_logoFlashes[0].alpha >= 100 && g_logoFlashes[0].alpha <= 150);
    for (int i = 1; i < MAX_LOGO_FLASHES + 3; i++)
        AddLogoFlash(i, i);
    int n = 0;
    for (int i = 0; i < MAX_LOGO_FLASHES; i++)
        n += g_logoFlashes[i].active;
    CHECK_EQ_INT(n, MAX_LOGO_FLASHES);
    CHECK_NEAR(g_logoFlashes[MAX_LOGO_FLASHES - 1].x, MAX_LOGO_FLASHES - 1, 0);
    ClearFlashes();
    CHECK_EQ_INT(g_logoFlashes[MAX_LOGO_FLASHES - 1].active, 0);
    CHECK_EQ_INT(g_logoFlashes[0].active, 0);
}

TEST(ui_UpdateLogoFlashes_moves_fades_and_expires)
{
    ClearFlashes();
    g_logoFlashes[3].active = 1;
    g_logoFlashes[3].x = 10;
    g_logoFlashes[3].y = 20;
    g_logoFlashes[3].vx = 2;
    g_logoFlashes[3].vy = -3;
    g_logoFlashes[3].life = 1.5f;
    g_logoFlashes[3].alpha = 4;
    g_logoFlashes[3].fade = 3;
    UpdateLogoFlashes();
    CHECK_NEAR(g_logoFlashes[3].x, 12, 0);
    CHECK_NEAR(g_logoFlashes[3].y, 17, 0);
    CHECK_NEAR(g_logoFlashes[3].alpha, 1, 0);
    CHECK_EQ_INT(g_logoFlashes[3].active, 1);
    UpdateLogoFlashes();
    CHECK_NEAR(g_logoFlashes[3].alpha, 0, 0);   // clamped
    CHECK_NEAR(g_logoFlashes[3].life, -0.5, 1e-6);
    CHECK_EQ_INT(g_logoFlashes[3].active, 0);
}

TEST(ui_DrawFlashes_draws_each_active_flash)
{
    Screen();
    g_flashGfx = ImgLoad("flash", false, true);
    ClearFlashes();
    g_logoFlashes[0].active = 1;
    g_logoFlashes[5].active = 1;
    g_logoFlashes[48].active = 1;
    DrawFlashes();
    CHECK_EQ_INT(g_fake.blits, 3);
}

TEST(ui_ShowLogoSplash_runs_its_five_seconds)
{
    Screen();
    unsigned start = g_fake.millis;
    ShowLogoSplash();
    CHECK_STR(FakeImageName(g_splash), "emvsoftware.jpg");
    CHECK(g_fake.millis - start > 5000);
    CHECK(g_fake.millis - start < 9000);
}

TEST(ui_ShowTitleSplash_click_skips_after_a_second)
{
    Screen();
    g_fake.mouseLeft = true;
    unsigned start = g_fake.millis;
    ShowTitleSplash();
    CHECK_STR(FakeImageName(g_splash), "splashscreen.jpg");
    CHECK(g_fake.millis - start > 1000);
    CHECK(g_fake.millis - start <= 1000 + 2 * 16);
}

TEST(ui_ShowLogoSplash_without_the_image)
{
    Screen();
    g_fake.imageLoadFails = true;
    ShowLogoSplash();
    CHECK_EQ_INT(g_fake.flips, 0);
}

TEST(ui_DrawMenuPrompt_space_or_fire)
{
    Screen();
    WinInit();
    g_time = 1000;
    g_uiBlinkTime = 1000;
    g_blinkRate = 500;
    g_uiBlink = 1;
    g_restartNeeded = 0;
    g_windowRenderer = RendererChoiceOf(g_cfg.renderer);
    g_timeTrialLocked = 0;
    g_cfg.device0 = DEVICE_KEYBOARD;
    DrawMenuPrompt();
    CHECK_EQ_INT(g_blitCount, 16);   // "PRESS SPACE TO PLAY"
    CHECK_NEAR(g_blit[0].destY, 600 - 0x2a, 0);
    g_blitCount = 0;
    g_cfg.device0 = DEVICE_JOYSTICK1;
    DrawMenuPrompt();
    CHECK_EQ_INT(g_blitCount, 15);   // "PRESS FIRE TO PLAY"
    g_blitCount = 0;
    g_restartNeeded = 1;
    DrawMenuPrompt();
    CHECK_EQ_INT(g_blitCount, 15 + 41);
    g_blitCount = 0;
    g_uiBlink = 0;
    g_restartNeeded = 0;
    DrawMenuPrompt();
    CHECK_EQ_INT(g_blitCount, 0);
}

TEST(ui_DrawMenuPrompt_hidden_behind_windows)
{
    Screen();
    WinInit();
    g_uiBlink = 1;
    g_time = 0;
    g_blinkRate = 500;
    WinOpen(0, 0, 10, 10, WIN_MODE_TILED);
    DrawMenuPrompt();
    CHECK_EQ_INT(g_blitCount, 0);
}

static void KeepMenuAwake(void)
{
    g_menuIdleTimeout = g_time + MENU_IDLE_MS;
}

// Runs frames until the attract screen changes (the menu's idle demo kept away); returns
// the milliseconds that took.
static unsigned WaitForNextScreen(void)
{
    int screen = g_attractScreen;
    unsigned start = g_fake.millis;
    for (int i = 0; i < 10000 && g_attractScreen == screen; i++) {
        KeepMenuAwake();
        g_fake.millis += 84;   // + 16 per frame: 100 ms per frame
        RunFrames(1);
    }
    return g_fake.millis - start;
}

TEST(ui_Title_attract_screens_cycle_on_idle_timeouts)
{
    BootGame();
    WinInit();
    RunFrames(1);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_INTRO);
    g_lastActivityTime = g_time;
    g_idleTimeoutMs = 60000;
    unsigned ms = WaitForNextScreen();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_ABOUT);
    CHECK(g_frameFunc == AboutScreen);
    CHECK(ms > 60000 && ms <= 60300);
    ms = WaitForNextScreen();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_MISSION);
    CHECK(ms > 30000 && ms <= 30300);
    ms = WaitForNextScreen();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HELP_CONTROLS);
    CHECK(g_frameFunc == HelpControls);
    CHECK(ms > 30000 && ms <= 30300);
    ms = WaitForNextScreen();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HELP_BONUSES);
    CHECK(ms > 15000 && ms <= 15300);
    ms = WaitForNextScreen();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_HALL_OF_FAME);
    CHECK(g_frameFunc == HallOfFame);
    CHECK(ms > 15000 && ms <= 15300);
    ms = WaitForNextScreen();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_INTRO);
    CHECK(g_frameFunc == IntroFrame);
    CHECK(ms > 15000 && ms <= 15300);
    CHECK_EQ_INT(g_state, STATE_TITLE);
}

TEST(ui_Title_wrap_to_the_intro_starts_a_streak)
{
    BootGame();
    WinInit();
    RunFrames(1);
    SeedRand(5);
    g_rngX = g_rngY = g_rngZ = 0;
    g_rngW = 1;
    g_rngT = 0;
    g_attractScreen = ATTRACT_HALL_OF_FAME;
    g_streakActive = 0;
    g_titleResetPending = 1;
    KeepMenuAwake();
    MenuHandler();
    CHECK_EQ_INT(g_attractScreen, ATTRACT_INTRO);
    CHECK_EQ_INT(g_streakActive, 1);
    CHECK_NEAR(g_streakAlphaStep, 6.375, 0);
    CHECK(g_streakColorR + g_streakColorG + g_streakColorB >= 50);
    CHECK_EQ_INT(g_idleTimeoutMs, 60000);
}

TEST(ui_Title_open_windows_hold_the_intro)
{
    BootGame();   // the PRESETS window is open
    RunFrames(2);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_INTRO);
    for (int i = 0; i < 100; i++) {
        KeepMenuAwake();
        g_fake.millis += 1000;
        RunFrames(1);
    }
    CHECK_EQ_INT(g_attractScreen, ATTRACT_INTRO);
    CHECK_EQ_INT(g_introBlockedByWindow, 1);
}

TEST(ui_ResetToTitle_returns_to_the_title)
{
    BootGame();
    WinInit();
    g_state = STATE_PAUSED;
    g_attractScreen = ATTRACT_HALL_OF_FAME;
    g_playerUpdateFn = StateDemo;
    g_cfg.musicFormat = MUSIC_FMT_MOD;
    g_playlistCount = 0;
    g_musicMode = MUSIC_BOSS;
    int zoom = FakePlayCount("zoom");
    g_time = 50000;
    ResetToTitle();
    CHECK_EQ_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_attractScreen, ATTRACT_INTRO);
    CHECK_EQ_INT(g_idleTimeoutMs, 30000);
    CHECK_EQ_INT(g_lastActivityTime, 50000);
    CHECK_EQ_INT(g_menuIdleTimeout, 50000 + MENU_IDLE_MS);
    CHECK_EQ_INT(g_transitionLock, 1);
    CHECK_EQ_INT(g_transitionLockUntil, 50000 + TRANSITION_LOCK_MS);
    CHECK_EQ_INT(g_inputCooldown, 100);
    CHECK(g_playerUpdateFn == UpdatePlayer);
    CHECK_EQ_INT(g_musicMode, MUSIC_TITLE);
    CHECK_EQ_INT(FakePlayCount("zoom"), zoom + 1);
    CHECK_NEAR(g_wordmarkShrinkW, 5500, 0);
    CHECK_EQ_INT(g_introStageCenter, 1);
    CHECK_EQ_INT(g_fake.maxFps, FPS_NORMAL);
}

TEST(ui_ResetToTitle_restores_the_time_trial_difficulty)
{
    BootGame();
    g_gameMode = MODE_TIME_TRIAL;
    g_savedDifficultyTT = DIFF_HARD;
    g_cfg.difficulty = DIFF_EASY;
    g_cfg.fps = 200;
    ResetToTitle();
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_HARD);
    CHECK_EQ_INT(g_cfg.fps, FPS_NORMAL);
}

// ---------------------------------------------------------------------------------------
// hud.c
// ---------------------------------------------------------------------------------------

TEST(ui_AddScorePopup_fills_free_slots)
{
    for (int i = 0; i < MAX_POPUPS; i++)
        g_popups[i].active = 0;
    g_popups[0].active = 1;
    AddScorePopup(100, 200, 12345, false);
    CHECK_EQ_INT(g_popups[1].active, 1);
    CHECK_STR(g_popups[1].text, "12.345");
    CHECK_NEAR(g_popups[1].life, 91, 0);
    CHECK_NEAR(g_popups[1].x, 100, 0);
    CHECK_NEAR(g_popups[1].y, 200, 0);
    CHECK_NEAR(g_popups[1].vy, -0.3, 1e-6);
    AddScorePopup(1, 2, 7, true);
    CHECK_NEAR(g_popups[2].life, 221, 0);
    CHECK_EQ_INT(g_popups[2].big, 1);
    for (int i = 0; i < 10; i++)
        AddScorePopup(1, 2, 7, false);
    int n = 0;
    for (int i = 0; i < MAX_POPUPS; i++)
        n += g_popups[i].active;
    CHECK_EQ_INT(n, MAX_POPUPS);
}

TEST(ui_UpdateScorePopups_rises_and_expires)
{
    for (int i = 0; i < MAX_POPUPS; i++)
        g_popups[i].active = 0;
    AddScorePopup(10, 100, 50, false);
    for (int i = 0; i < 91; i++)
        UpdateScorePopups();
    CHECK_EQ_INT(g_popups[0].active, 1);
    CHECK_NEAR(g_popups[0].y, 100 - 91 * 0.3, 0.01);
    UpdateScorePopups();
    CHECK_EQ_INT(g_popups[0].active, 0);
    AddScorePopup(10, 0, 50, false);   // off the top
    UpdateScorePopups();
    CHECK_EQ_INT(g_popups[0].active, 0);
}

TEST(ui_UpdateScorePopups_big_ones_blink_late)
{
    for (int i = 0; i < MAX_POPUPS; i++)
        g_popups[i].active = 0;
    AddScorePopup(10, 500, 50, true);
    for (int i = 0; i < 143; i++)   // life 221 -> 78
        UpdateScorePopups();
    CHECK_EQ_INT(g_popups[0].blinkCounter, 1);
    UpdateScorePopups();
    CHECK_EQ_INT(g_popups[0].blinkCounter, 2);
    CHECK_EQ_INT(g_popups[0].blinkPhase, 0);   // 4 > 2: back to 0
}

TEST(ui_DrawScorePopups_small_digits_and_colors)
{
    Screen();
    for (int i = 0; i < MAX_POPUPS; i++)
        g_popups[i].active = 0;
    AddScorePopup(100, 50, 5000, false);   // "5.000": color 2
    g_popups[0].life = 91;
    DrawScorePopups();
    CHECK_EQ_INT(g_blitCount, 5);
    CHECK_EQ_INT(g_blit[0].src.x1, 0 * 80 + 5 * 8 + 2 * 168);
    CHECK_EQ_INT(g_blit[1].src.x1, 80 + 10 * 8 + 2 * 168);   // the dot
    CHECK_NEAR(g_blit[1].destX, 107, 0);
    CHECK_NEAR(g_blit[2].destX, 111, 0);
    CHECK_NEAR(g_blit[3].destX, 117, 0);
    CHECK_EQ_INT(g_blit[0].src.y1, 0);   // frame 7 - 91/13 = 0
    g_blitCount = 0;
    g_popups[0].value = 30000;
    g_popups[0].life = 13;
    DrawScorePopups();
    CHECK_EQ_INT(g_blit[0].src.x1, 5 * 8);   // color 0
    CHECK_EQ_INT(g_blit[0].src.y1, 6 * 9);
}

TEST(ui_DrawRow_and_NewRank)
{
    Screen();
    DrawRow(10, 20, 3, 4);
    CHECK_EQ_INT(g_blitCount, 4);
    CHECK_NEAR(g_blit[3].destX, 10 + 3 * 19, 0);
    CHECK_EQ_INT(g_blit[3].src.x1, 48);
    CHECK_EQ_INT(g_blit[3].src.y1, 160);
    g_time = 1000;
    NewRank();
    CHECK_STR(g_alertMsg, "******  NEW RANK IS NOW AVAILABLE  ******");
    CHECK_EQ_INT(g_msgTimer, 6000);
    CHECK_EQ_INT(g_msgColor, 8);
}

TEST(ui_DrawFps_samples_once_a_second)
{
    Screen();
    g_lastFpsTime = g_fake.millis;
    g_frameCount = 0;
    g_fps = 0;
    for (int i = 0; i < 50; i++) {
        g_fake.millis += 20;
        DrawFps();
    }
    CHECK_EQ_INT(g_fps, 0);
    CHECK_EQ_INT(g_blitCount, 0);
    g_fake.millis += 20;
    DrawFps();   // 51 frames in 1020 ms
    CHECK_EQ_INT(g_fps, 50);
    CHECK_EQ_INT(g_frameCount, 0);
    CHECK_EQ_INT(g_blitCount, 5);   // "50 FPS"
    CHECK_NEAR(g_blit[0].destX, 400 - 25, 0);
}

TEST(ui_DrawBossBar_fill_follows_health)
{
    Screen();
    SeedRand(3);
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_bossIdx = -1;
    DrawBossBar();
    CHECK_EQ_INT(g_blitCount, 0);
    g_bossIdx = 2;
    g_enemies[0][2].hp = 50;
    g_enemies[0][2].maxHp = 100;
    DrawBossBar();
    // 15 frame tiles, 20 fill segments (half of 40% of 80... 0.5 * 80 / 2), "BOSS"
    CHECK_EQ_INT(g_blitCount, 15 + 20 + 4);
    CHECK_EQ_INT(g_blit[15].src.x1, 0x40);   // under 66%: the middle colour
}

// ---------------------------------------------------------------------------------------
// particles.c
// ---------------------------------------------------------------------------------------

TEST(ui_AddParticle_needs_particles_on)
{
    ClearParticles();
    g_cfg.particlesOn = 0;
    AddParticle(NULL, 1, 2, 3, 0, 0, 0, 0, 255, 255, 255, 200, 10, 0, -1, 0, 0, NULL, NULL, 0);
    CHECK_EQ_INT(CountParticles(), 0);
    g_cfg.particlesOn = 1;
    AddParticle(NULL, 1, 2, 3, 0, 0, 0, 0, 255, 255, 255, 200, 10, 0, -1, 0, 0, NULL, NULL, 0);
    AddParticle(NULL, 1, 2, 3, 0, 0, 0, 0, 255, 255, 255, 200, 10, 0, -1, 0, 0, NULL, NULL, 0);
    CHECK_EQ_INT(CountParticles(), 2);
    CHECK_NEAR(g_particles[0].alphaStep, 20, 0);
    ClearParticles();
    CHECK_EQ_INT(CountParticles(), 0);
}

TEST(ui_AddParticle_pool_holds_999)
{
    ClearParticles();
    g_cfg.particlesOn = 1;
    for (int i = 0; i < 1010; i++)
        AddParticle(NULL, 1, 2, 3, 0, 0, 0, 0, 255, 255, 255, 200, 10, 0, -1, 0, 0, NULL, NULL, 0);
    CHECK_EQ_INT(CountParticles(), MAX_PARTICLES - 1);
}

TEST(ui_UpdateParticles_moves_by_mode_and_expires)
{
    BootGame();   // the trig tables
    ClearParticles();
    g_cfg.particlesOn = 1;
    // dir 0: vx = cos 0 * 2 = 2, vy = -sin 0 * 2 = 0; gravity 0.5
    AddParticle(NULL, 100, 100, 10, -4, 350, 20, 0, 1, 2, 3, 100, 2.5f, 2, -1, 0.5f, 0, NULL, NULL, 0);
    AddParticle(NULL, 100, 100, 10, 0, 0, 0, 0, 1, 2, 3, 100, 10, 2, -1, 0.5f, 1, NULL, NULL, 0);
    AddParticle(NULL, 100, 100, 10, 0, 0, 0, 0, 1, 2, 3, 100, 10, 2, -1, 0.5f, 2, NULL, NULL, 0);
    AddParticle(NULL, 100, 100, 1.5f, -1, 0, 0, 0, 1, 2, 3, 100, 10, 0, -1, 0, 3, NULL, NULL, 0);
    g_particles[1].vx = 3;   // mode 1 must ignore its horizontal velocity
    g_particles[2].vx = 3;
    UpdateParticles();
    CHECK_NEAR(g_particles[0].x, 100 + g_cosDeg[0] * 2, 1e-4);
    CHECK_NEAR(g_particles[0].vy, -g_sinDeg[0] * 2 + 0.5, 1e-4);
    CHECK_NEAR(g_particles[0].size, 6, 0);
    CHECK_NEAR(g_particles[0].angle, 10, 1e-4);   // 370 wraps
    CHECK_NEAR(g_particles[0].alpha, 60, 1e-4);
    CHECK_NEAR(g_particles[1].x, 100, 0);          // vertical only
    CHECK_NEAR(g_particles[2].y, 100, 0);          // horizontal only
    CHECK_NEAR(g_particles[2].x, 103, 1e-4);
    CHECK_NEAR(g_particles[3].size, 1, 0);         // a subpixel size is clamped immediately
    UpdateParticles();
    CHECK_NEAR(g_particles[0].size, 2, 0);
    UpdateParticles();
    CHECK_NEAR(g_particles[0].size, 1, 0);         // not below 1
    CHECK_EQ_INT(g_particles[0].active, 0);        // life 2.5 - 3 < 0
    CHECK_EQ_INT(g_particles[1].active, 1);
}

TEST(ui_UpdateParticles_kill_flag_clears_everything)
{
    ClearParticles();
    g_cfg.particlesOn = 1;
    int kill = 0;
    AddParticle(NULL, 1, 1, 3, 0, 0, 0, 0, 1, 1, 1, 100, 50, 0, -1, 0, 0, NULL, NULL, 0);
    AddParticle(NULL, 1, 1, 3, 0, 0, 0, 0, 1, 1, 1, 100, 50, 0, -1, 0, 0, NULL, &kill, 0);
    UpdateParticles();
    CHECK_EQ_INT(CountParticles(), 2);
    kill = 1;
    UpdateParticles();
    CHECK_EQ_INT(CountParticles(), 0);
    CHECK_EQ_INT(kill, 0);
}

TEST(ui_DrawParticles_queues_one_stretch_each)
{
    Screen();
    ClearParticles();
    g_cfg.particlesOn = 1;
    int xref = 300;
    AddParticle(NULL, 100, 50, 10, 0, 0, 0, 0, 1, 2, 3, 400, 10, 0, -1, 0, 0, NULL, NULL, 0);
    AddParticle(NULL, 100, 50, 10, 0, 0, 0, 0, 1, 2, 3, 100, 10, 0, -1, 0, 0, &xref, NULL, 0);
    AddParticle(NULL, 100, 50, 10, 0, 0, 0, 0, 1, 2, 3, 100, 10, 0, -1, 0, 2, NULL, NULL, 0);
    DrawParticles();
    CHECK_EQ_INT(g_stretchRotCount, 3);
    CHECK_NEAR(g_stretchRot[0].x1, 94, 1e-4);   // size 10 * 0.6
    CHECK_NEAR(g_stretchRot[0].x2, 106, 1e-4);
    CHECK_EQ_INT(g_stretchRot[0].a, 255);       // alpha clamped
    CHECK_NEAR(g_stretchRot[1].x1, 294, 1e-4);  // follows *xref
    CHECK_NEAR(g_stretchRot[2].y1, 0, 0);       // full-height column
    CHECK_NEAR(g_stretchRot[2].y2, 600, 0);
    g_cfg.particlesOn = 0;
    DrawParticles();
    CHECK_EQ_INT(g_stretchRotCount, 3);
}

TEST(ui_SpawnSlots_spawns_one_more_than_asked)
{
    SeedRand(2);
    ClearSlots();
    SpawnSlots(10, 20, 4, 255, 128, 0);
    CHECK_EQ_INT(g_slotCount, 5);
    int n = 0;
    for (int i = 0; i < MAX_SLOTS; i++)
        n += g_slots[i].active;
    CHECK_EQ_INT(n, 5);
    CHECK_NEAR(g_slots[4].x, 10, 0);
    CHECK_NEAR(g_slots[4].g, 128, 0);
    CHECK_NEAR(g_slots[0].dr, g_slots[4].dr, 0);   // one fade rate per call
    ClearSlots();
    CHECK_EQ_INT(g_slotCount, 0);
    CHECK_EQ_INT(g_slots[0].active, 0);
}

TEST(ui_UpdateSlots_fades_and_stops)
{
    ClearSlots();
    g_slots[7].active = 1;
    g_slotCount = 1;
    g_slots[7].pos = 1;
    g_slots[7].vel = 2;
    g_slots[7].accel = 1.5f;
    g_slots[7].r = 100;
    g_slots[7].g = 1;
    g_slots[7].b = 100;
    g_slots[7].dr = g_slots[7].dg = g_slots[7].db = 3;
    UpdateSlots(1.0f);
    CHECK_NEAR(g_slots[7].pos, 3, 0);
    CHECK_NEAR(g_slots[7].vel, 0.5, 0);
    CHECK_NEAR(g_slots[7].r, 97, 0);
    CHECK_NEAR(g_slots[7].g, 0, 0);
    CHECK_EQ_INT(g_slots[7].active, 1);
    UpdateSlots(1.0f);   // velocity below zero: done
    CHECK_EQ_INT(g_slots[7].active, 0);
    CHECK_EQ_INT(g_slotCount, 0);
}

TEST(ui_DrawSlots_plots_inside_the_rect)
{
    BootGame();
    ClearSlots();
    g_fake.pixels = 0;
    g_slotCount = 2;
    g_slots[0].active = 1;
    g_slots[0].x = 100;
    g_slots[0].y = 100;
    g_slots[0].pos = 0;
    g_slots[1].active = 1;
    g_slots[1].x = 900;
    g_slots[1].y = 100;
    g_slots[1].pos = 0;
    DrawSlots(0, 800, 0, 600);
    CHECK_EQ_INT(g_fake.pixels, 1);
}

TEST(ui_SpawnSpark_and_UpdateSparks)
{
    BootGame();
    Screen();
    for (int i = 0; i < MAX_SPARKS; i++)
        g_sparks[i].active = 0;
    g_state = STATE_TITLE;
    g_frameDt = 1.0f;
    g_cfg.particlesOn = 1;
    g_flag = 0;
    SpawnSpark(100, 200, 1, 2, 3, 4, 2.0f, 0, 1, 100, 0, 20);
    CHECK_EQ_INT(g_flag, 1);
    CHECK_EQ_INT(g_sparks[0].active, 1);
    CHECK_NEAR(g_sparks[0].alpha, 255, 0);
    UpdateSparks();
    // drawn as a sprite, a trail spark spawned (interval 0) and drawn in the same pass,
    // delay 1 -> 0 so it fades
    CHECK_EQ_INT(g_stretchFCount, 2);
    CHECK_EQ_INT(g_sparks[1].active, 1);
    CHECK_EQ_INT(g_sparks[1].moving, 0);
    CHECK_EQ_INT(g_sparks[1].size, 10);
    CHECK_NEAR(g_sparks[0].alpha, 155, 0);
    CHECK_NEAR(g_sparks[0].vy, g_sinDeg[0] * 2 + 0.04, 1e-4);
    UpdateSparks();
    CHECK_NEAR(g_sparks[0].alpha, 55, 0);
    UpdateSparks();
    CHECK_EQ_INT(g_sparks[0].active, 0);
    CHECK_EQ_INT(g_sparks[1].active, 0);   // the trails faded too
    g_cfg.particlesOn = 0;
    g_fake.pixels = 0;
    SpawnSpark(100, 200, 1, 2, 3, 4, 2.0f, 0, 5, 100, 50, 20);
    UpdateSparks();
    CHECK_EQ_INT(g_fake.pixels, 1);   // particles off: a pixel instead
    CHECK_NEAR(g_sparks[0].alpha, 255, 0);   // still delayed
    CHECK_EQ_INT(g_sparks[0].delay, 4);
}

TEST(ui_SpawnSparks_takes_count_slots)
{
    BootGame();
    SeedRand(9);
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
        g_explosionParticles[i].active = 0;
    g_explosionParticles[1].active = 1;
    SpawnSparks(3, 50, 60, 1, 2, 1, 2);
    int n = 0;
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
        n += g_explosionParticles[i].active;
    CHECK_EQ_INT(n, 4);
    CHECK_EQ_INT(g_explosionParticles[3].active, 1);
    CHECK_EQ_INT(g_explosionParticles[4].active, 0);
    CHECK_EQ_INT(g_explosionParticles[0].b, 255);
    CHECK_NEAR(g_explosionParticles[0].alpha, 350, 0);
    SpawnSparksRGB(2, 5, 6, 1, 2, 1, 2, 10, 20, 30);
    CHECK_EQ_INT(g_explosionParticles[4].active, 1);
    CHECK_EQ_INT(g_explosionParticles[5].active, 1);
    CHECK_EQ_INT(g_explosionParticles[6].active, 0);
    CHECK_EQ_INT(g_explosionParticles[5].g, 20);
}

// ---------------------------------------------------------------------------------------
// explosions.c
// ---------------------------------------------------------------------------------------

TEST(ui_SpawnSmall_runs_13_frames)
{
    SeedRand(4);
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
        g_explosions[i].active = 0;
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_save.players[0].hyperspaceFade = 0;
    SpawnSmall(10, 20);
    CHECK_EQ_INT(g_explosions[0].active, 1);
    CHECK_EQ_INT(g_explosions[0].type, 10);
    g_explosions[0].delay = 0;
    g_explosions[0].timer = 0;
    for (int i = 0; i < 12; i++)
        UpdateExplosions();
    CHECK_EQ_INT(g_explosions[0].frame, 12);
    CHECK_EQ_INT(g_explosions[0].active, 1);
    UpdateExplosions();
    CHECK_EQ_INT(g_explosions[0].active, 0);
}

TEST(ui_explosion_types_end_differently)
{
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
        g_explosions[i].active = 0;
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_save.players[0].hyperspaceFade = 0;
    g_explosions[0] = (Explosion){ .active = 1, .type = 3 };
    g_explosions[1] = (Explosion){ .active = 1, .type = -1, .alpha = 12 };
    g_explosions[2] = (Explosion){ .active = 1, .type = 3, .spin = 1, .scale = 30, .scaleMul = 0.5f,
                                   .angle = 359, .angleVel = 2 };
    UpdateExplosions();
    CHECK_EQ_INT(g_explosions[1].alpha, 7);
    CHECK_NEAR(g_explosions[2].angle, 1, 1e-4);
    CHECK_EQ_INT(g_explosions[2].active, 0);      // 15 < 20: gone
    UpdateExplosions();
    UpdateExplosions();
    CHECK_EQ_INT(g_explosions[1].active, 0);      // 12 - 15 < 0
    for (int i = 0; i < 10; i++)
        UpdateExplosions();
    CHECK_EQ_INT(g_explosions[0].frame, 13);
    CHECK_EQ_INT(g_explosions[0].active, 1);
    UpdateExplosions();
    CHECK_EQ_INT(g_explosions[0].active, 0);
}

TEST(ui_SpawnExplosion_centers_and_sparks)
{
    BootGame();
    SeedRand(6);
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
        g_explosions[i].active = 0;
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
        g_explosionParticles[i].active = 0;
    g_cfg.sparks = 10;
    g_maxSparks = 5;
    SpawnExplosion(100, 200, 40, 20, 50, 2, 0, 1, 2, 3, 4, 5, 6);
    CHECK_NEAR(g_explosions[0].x, 120, 0);
    CHECK_NEAR(g_explosions[0].y, 210, 0);
    CHECK_NEAR(g_explosions[0].scale, 255, 0);
    CHECK(g_explosions[0].alpha >= 50 && g_explosions[0].alpha < 85);
    CHECK(g_explosions[0].delay >= 2 && g_explosions[0].delay < 5);
    int n = 0;
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
        n += g_explosionParticles[i].active;
    CHECK(n >= 5 && n < 10);
    CHECK_EQ_INT(g_explosionParticles[0].r, 4);
    SpawnBigExplosion(0, 0, 10, 10, 2, 0, 7, 8, 9);
    CHECK_NEAR(g_explosions[1].scale, 400, 0);
    CHECK_NEAR(g_explosions[1].x, 5, 0);
    int m = 0;
    for (int i = 0; i < MAX_EXPLOSION_PARTICLES; i++)
        m += g_explosionParticles[i].active;
    CHECK_EQ_INT(m - n, 20);
}

TEST(ui_SpawnHugeExplosion_flashes_the_screen)
{
    BootGame();
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
        g_explosions[i].active = 0;
    for (int j = 0; j < MAX_FLASH_RINGS; j++)
        g_flash[j].active = 0;
    g_flash[0].active = 1;
    g_cfg.sparks = 2;
    g_flashOverlayActive = 0;
    SpawnHugeExplosion(100, 100, 20, 20, 2, 0, 9, 8, 7, 2);
    CHECK_EQ_INT(g_flashOverlayActive, 1);
    CHECK_EQ_INT(g_flash[1].active, 1);
    CHECK_EQ_INT(g_flash[2].active, 0);
    CHECK_EQ_INT(g_flash[1].x, 110);
    CHECK(g_flash[1].gfx == g_gfxFlareBomb3);
    CHECK_NEAR(g_explosions[0].scale, 600, 0);
    SpawnHugeExplosion(100, 100, 20, 20, 2, 0, 9, 8, 7, 0);
    CHECK_EQ_INT(g_flash[2].active, 0);
}

// ---------------------------------------------------------------------------------------
// resources.c, frames.c, stars.c
// ---------------------------------------------------------------------------------------

TEST(ui_LoadGraphic_lowercases_the_name)
{
    Image *img = LoadGraphic("Gfx\\Logos.PNG", true, false);
    CHECK_STR(FakeImageName(img), "gfx\\logos.png");
    img = LoadGraphic2("ABC.JPG", false, false);
    CHECK_STR(FakeImageName(img), "abc.jpg");
    g_fake.imageLoadFails = true;
    CHECK(LoadGraphic("x.png", true, false) == NULL);
    CHECK(LoadGraphic2("x.png", true, false) == NULL);
}

TEST(ui_LoadHma_without_the_file_returns_null)
{
    CHECK(LoadHma("nothing", 3, 2) == NULL);
}

TEST(ui_LoadHma_reads_a_lowercase_mask_with_the_requested_size)
{
    const unsigned char mask[] = {1, 2, 3, 4, 5, 6};
    FakePacAdd("fighter1.hma", mask, sizeof mask);
    unsigned char *loaded = LoadHma("Fighter1", 3, 2);
    CHECK(loaded != NULL);
    CHECK_MEM(loaded, mask, sizeof mask);
    free(loaded);
}

TEST(ui_LoadHma_zero_fills_short_files_and_rejects_invalid_dimensions)
{
    const unsigned char shortMask[] = {10, 20, 30};
    FakePacAdd("short.hma", shortMask, sizeof shortMask);
    unsigned char *loaded = LoadHma("short", 2, 2);
    CHECK(loaded != NULL);
    const unsigned char want[] = {10, 20, 30, 0};
    CHECK_MEM(loaded, want, sizeof want);
    free(loaded);

    CHECK(LoadHma("short", 0, 2) == NULL);
    CHECK(LoadHma("short", 2, -1) == NULL);
    CHECK(LoadHma("short", INT_MAX, INT_MAX) == NULL);
    char tooLongName[509];
    memset(tooLongName, 'x', sizeof tooLongName - 1);
    tooLongName[sizeof tooLongName - 1] = 0;
    CHECK(LoadHma(tooLongName, 1, 1) == NULL);
}

TEST(ui_DrawFlash_fades_over_30_frames)
{
    Screen();
    g_gfxLogos = ImgLoad("logos", false, true);
    g_cfg.bgEnabled = 0;
    g_flashOverlayActive = 1;
    g_fadeStep = 0;
    DrawFlash();
    CHECK_EQ_INT(g_fake.blits, 0);   // only with the background on
    g_cfg.bgEnabled = 1;
    for (int i = 0; i < 29; i++)
        DrawFlash();
    CHECK_EQ_INT(g_flashOverlayActive, 1);
    DrawFlash();
    CHECK_EQ_INT(g_flashOverlayActive, 0);
    CHECK_EQ_INT(g_fake.blits, 30);
    DrawFlash();
    CHECK_EQ_INT(g_fake.blits, 30);
}

TEST(ui_UpdateMenuStars_wraps_the_field)
{
    g_cfg.numStars = 2;
    g_starZNear = 1;
    g_starZFar = 100;
    g_starXMin = -50;
    g_starXMax = 50;
    g_starYMin = -40;
    g_starYMax = 40;
    g_save.players[0].starVelX = 5;
    g_save.players[0].starSpeed = -5;
    g_save.players[0].starVelZ = 3;
    g_starX[0] = 48;
    g_starY[0] = -38;
    g_starZ[0] = 99;
    g_starX[1] = 0;
    g_starY[1] = 0;
    g_starZ[1] = 10;
    g_angX = g_angY = g_angZ = 10;
    g_angVelX = g_angVelY = g_angVelZ = 0;
    UpdateMenuStars();
    CHECK_NEAR(g_starX[0], 53 - 100, 1e-4);
    CHECK_NEAR(g_starY[0], -43 + 80, 1e-4);
    CHECK_NEAR(g_starZ[0], 1, 0);
    CHECK_NEAR(g_starX[1], 5, 0);
    CHECK_NEAR(g_starZ[1], 13, 0);
}

TEST(ui_UpdateMenuStars_angle_wraps)
{
    SeedRand(8);
    g_cfg.numStars = 0;
    g_angX = 359.9f;
    g_angVelX = 0.2f;
    g_angY = 0.1f;
    g_angVelY = -0.2f;
    g_angZ = 5;
    g_angVelZ = 0;
    UpdateMenuStars();
    CHECK_NEAR(g_angX, 0.1, 1e-3);
    CHECK_NEAR(g_angY, 359.9, 1e-3);
    CHECK(g_angVelX >= -0.2f && g_angVelX <= 0.2f && g_angVelX != 0.2f);
}

TEST(ui_DrawDotGlyph_plots_each_point)
{
    int xs[3] = { 0, 1, 2 }, ys[3] = { 0, 1, 2 };
    DrawDotGlyph(10, 10, 3, 3, xs, ys);
    CHECK_EQ_INT(g_fake.pixels, 3);
    g_screenW = 800;
    g_screenH = 600;
    DrawDotSignature();
    CHECK_EQ_INT(g_fake.pixels, 3 + 13 + 13 + 9 + 12 + 10 + 12 + 8 + 6);
}

TEST(ui_InitStarRotation_picks_the_star_renderer)
{
    g_cfg.bgStars = 1;
    InitStarRotation();
    CHECK(g_fnPtr == DrawStarsStill);
    CHECK_NEAR(g_starSpeedX, 3, 0);
    CHECK(g_angX >= 0 && g_angX <= 360);
    g_cfg.bgStars = 0;
    InitStarRotation();
    CHECK(g_fnPtr == DrawStarsGlow);
}
