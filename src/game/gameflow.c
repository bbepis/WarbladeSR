// gameflow.c: Game flow: GameFrame's state dispatch, NewGame and mode setup, hotkeys,
// pause/resume, kill/escape credit, game over, the stall watchdog.
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "game.h"

// How long after opening the profile/stats window before it can be re-triggered (TAB / the
// profile input) again.
enum { PROFILE_SUBMIT_COOLDOWN_MS = 750 };

// Perfect wave-level clear reward: below MAX_ROCKETS, top up rockets instead of the cash payout.
enum {
    MAX_ROCKETS         = 50,
    ROCKET_TOPUP_AMOUNT = 10,
    PERFECT_CLEAR_SCORE = 50000,
};

// 0-255 volume shown to the player as a 0-100 percentage in the sfx/music/voice volume banners.
#define VOLUME_DISPLAY_SCALE 2.55


// True when the current mode's game-over condition is met (lives at or below the ship's minimum energy);
// in versus mode (1), switches to the other player if only the current one has run out, and only reports
// game over once both players have.
int IsGameOver()
{
    switch (g_gameMode) {
    case MODE_SINGLE:
        if (g_save.players[g_curPlayer].lives <= g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy)
            return 1;
        else
            return 0;

    case MODE_TWO_PLAYER:
        if (g_save.players[0].lives <= g_shipDefs[g_save.players[0].ship]->minEnergy &&
            g_save.players[1].lives <= g_shipDefs[g_save.players[1].ship]->minEnergy) {
            return 1;
        } else {
            if (g_save.players[g_curPlayer].lives <= g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy)
                SwitchPlayer();
            return 0;
        }
        break;

    case MODE_DUAL:
        if (g_save.players[0].lives <= g_shipDefs[g_save.players[0].ship]->minEnergy &&
            g_save.players[1].lives <= g_shipDefs[g_save.players[1].ship]->minEnergy)
            return 1;
        else
            return 0;
        break;

    case MODE_TEAM:
        return 0;
        break;

    case MODE_UNUSED_4:
        return 0;
        break;

    case MODE_ACE_TOURNAMENT:
        return 0;
        break;

    case MODE_TIME_TRIAL:
        if (g_save.players[g_curPlayer].lives <= g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy)
            return 1;
        else
            return 0;
    }
    return 0;
}

// Credits player p with killing one enemy. In versus mode also counts it as an "escape" against the other
// player. Once killed+escaped covers every enemy for the level, stops the level timer, marks the player
// done, and — on a perfect wave-level clear (no boss) — awards the rocket-based perfect bonus (or, if not
// yet at max rockets, tops up rockets instead) and unlocks the corresponding secret.
void CreditKill(int p)
{
    g_save.players[p].killed++;
    if (g_gameMode == MODE_DUAL) {
        if (p == 0)
            g_save.players[1].escaped++;
        else
            g_save.players[0].escaped++;
    }
    g_save.players[p].done = 0;
    if (g_save.players[p].killed + g_save.players[p].escaped >= g_save.players[p].totalEnemies) {
        TimerStop1();
        g_save.players[p].done = 1;

        if (!g_isBossLevel && g_isWaveLevel) {
            // Profile play (not a demo) also unlocks secret #27 for a perfect wave-level clear; the reward
            // logic itself (rockets/score) is otherwise identical to the non-profile branch below.
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                g_save.players[g_curPlayer].secretFound27 = 1;
                MarkSecretFound(g_profileIndex, 27);

                if (g_save.players[g_curPlayer].rockets >= MAX_ROCKETS) {
                    ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer, PERFECT_CLEAR_SCORE);
                    AddScorePopup((int)g_save.players[g_curPlayer].x,
                                         (int)(g_save.players[g_curPlayer].y - 30.0),
                                         PERFECT_CLEAR_SCORE, 0);
                } else {
                    g_save.players[g_curPlayer].rockets = g_save.players[g_curPlayer].rockets
                        + ROCKET_TOPUP_AMOUNT;
                    if (g_save.players[g_curPlayer].rockets > MAX_ROCKETS)
                        g_save.players[g_curPlayer].rockets = MAX_ROCKETS;
                    sprintf(g_alertMsg, "10 ROCKETS ADDED");
                    g_msgColor = 0;
                    g_msgTimer = g_time + 1500;
                }

            } else {
                if (g_save.players[g_curPlayer].rockets >= MAX_ROCKETS) {
                    ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer, PERFECT_CLEAR_SCORE);
                    AddScorePopup((int)g_save.players[g_curPlayer].x,
                                         (int)(g_save.players[g_curPlayer].y - 30.0),
                                         PERFECT_CLEAR_SCORE, 0);
                } else {
                    g_save.players[g_curPlayer].rockets = g_save.players[g_curPlayer].rockets
                        + ROCKET_TOPUP_AMOUNT;
                    if (g_save.players[g_curPlayer].rockets > MAX_ROCKETS)
                        g_save.players[g_curPlayer].rockets = MAX_ROCKETS;
                    sprintf(g_alertMsg, "10 ROCKETS ADDED");
                    g_msgColor = 0;
                    g_msgTimer = g_time + 1500;
                }
            }
        }

        if (g_save.players[p].doneTime == 0)
            g_save.players[p].doneTime = g_time + 3000;
    }
}

// Credits player p with an enemy escaping (versus mode counts it for both players). Same
// killed+escaped-reaches-total completion check as CreditKill, without the perfect-clear bonus.
void CreditEscape(int p)
{
    if (g_gameMode == MODE_DUAL) {
        g_save.players[0].escaped++;
        g_save.players[1].escaped++;
    } else {
        g_save.players[p].escaped++;
    }
    g_save.players[p].done = 0;
    if (g_save.players[p].killed + g_save.players[p].escaped >= g_save.players[p].totalEnemies) {
        TimerStop1();
        g_save.players[p].done = 1;
        if (g_save.players[p].doneTime == 0)
            g_save.players[p].doneTime = g_time + 3000;
    }
}

// Recovers a level that's gotten stuck: if nothing has happened for 600 frames, snaps the kill count back
// up to account for escapees; if the player has been idle-stalled for 45s, force-finishes the level.
void LevelStallWatchdog()
{
    int p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = 0;
    if (g_levelIdleCounter > 600) {
        g_levelIdleCounter = 0;
        g_save.players[p].done = 0;
        g_save.players[p].killed = g_save.players[p].totalEnemies - g_save.players[p].escaped;
    }
    if (g_time - g_playerStallTime > 45000) {
        g_save.players[p].levelTransitioning = 0;
        g_save.players[p].done = 1;
        if (g_save.players[p].doneTime == 0)
            g_save.players[p].doneTime = g_time + 3000;
        g_playerStallTime = g_time;
    }

    if (g_save.players[p].done == 0) {
        if (g_save.players[p].killed + g_save.players[p].escaped >= g_save.players[p].totalEnemies) {
            TimerStop1();
            g_save.players[p].done = 1;
            if (g_save.players[p].doneTime == 0)
                g_save.players[p].doneTime = g_time + 3000;
        }
    }
}

// Unpauses gameplay: restores g_state from PauseGame, clears any pause-menu item/laser
// leftovers, folds the paused duration into g_pausedDuration, and resumes sound.
void ResumeGame()
{
    g_state = g_savedState;
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (g_items[i].alive != 0 && g_items[i].type == ITEM_STAR)
            g_items[i].alive = 0;
    }

    // Fold the time spent paused into g_pausedDuration.
    StampTimeE();
    g_pauseStartStamp = g_timeD;
    g_pauseEndStamp = g_timeE;
    g_timeD = 0;
    g_timeE = 0;
    g_pausedDuration += g_pauseEndStamp - g_pauseStartStamp;
    g_pauseEndStamp = 0;
    g_pauseStartStamp = 0;

    // Resume sound/music and clear the pause-menu item/laser leftovers.
    SoundResume();
    SetSfxVolume(g_cfg.sfxVol);
    ApplyMusicVolume();
    EmptyViewChangeHook();
    for (int j = 0; j < MAX_MAP_OBJS; j++) {
        if (g_mapObjs[j].active != 0 && g_mapObjs[j].laser != 0)
            g_mapObjs[j].active = 0;
    }
    EmptyPostTransitionHook();
    g_buttonsOn = 0;
    HidePointer();
}

// Pauses gameplay: saves g_state (as STATE_PAUSED), stamps the pause start time, and
// arms the 10 rotating/fading "PAUSED" FX layers plus a random tint-cycle start color.
void PauseGame()
{
    if (g_state != STATE_PAUSED) {
        g_pauseCount++;
        g_savedState = g_state;
        StampTimeD();
        g_state = STATE_PAUSED;
        CLEAR_DRAW_COUNTERS();
        g_keyLatch[K_VK_P] = 0;
        g_pauseLastTick = g_time;

        // Random starting color and per-channel drift speed for the "PAUSED" tint cycle.
        g_fxRed = RandFloat(0.0f, 255.0f);
        g_fxColorG = RandFloat(0.0f, 255.0f);
        g_fxBlueColorLevel = RandFloat(0.0f, 255.0f);
        g_fxColorSpeedR = RandFloat(-1.0f, 1.0f);
        g_fxGreenVel = RandFloat(-1.0f, 1.0f);
        g_fxBlueVel = RandFloat(-1.0f, 1.0f);

        // Set up the ring of "PAUSED" sprites, each starting a bit dimmer than the last.
        int c = 255;
        float a = 0.0f;
        for (int i = 0; i < MAX_FX; i++) {
            g_fx[i].active = 1;
            g_fx[i].gfx = g_gfxPause;
            g_fx[i].scale = 60.0f;
            g_fx[i].speed = 350.0f;
            g_fx[i].x = (float)(g_screenW / 2 - g_fx[i].speed / 2.0);
            g_fx[i].y = (float)(g_screenH / 2 - g_fx[i].scale / 2.0);
            g_fx[i].alpha = (float)c;
            g_fx[i].unusedTimer10 = 0.0f;
            a = a + 2.0;
            if (a < 0.0)
                a = a + 360.0;
            c = c - 25;
        }

        SoundPause();
    }
}

// In-game debug/options hotkeys, polled once per frame regardless of pause state: F6
// restarts music, Esc/P/pause-input open the quit dialog or pause, B cycles the border
// mode (+Shift: background tint, +Alt: nebula toggle), TAB/profile-input opens the
// profile window (submitting session stats first), V cycles the announcer voice, E/N
// (+Shift to reverse) adjust spark/star counts, F toggles flare particles (+Alt+Shift:
// FPS display), I cycles bullet-render intensity, Z toggles point/flare stars, W toggles
// fullscreen, S cycles interpolation, Alt+V toggles vsync, M cycles the music format, U toggles voice on/off, and PageUp/Down, +/-,
// Home/End adjust the sfx/music/voice volumes. The cheat code and its number keys are
// handled by CheatHotkeys (cheats.c).
void Hotkeys()
{
    int voice;
    int tries;
    unsigned char done;

    CheatHotkeys();

    // ---- F6: restart music ----
    if (KeyDown(K_VK_F6) == true) {
        if (g_keyLatch[K_VK_F6] != 0) {
            g_songName = "warblade";
            g_musicMode = MUSIC_CUSTOM;
            if (g_gameMode == MODE_TIME_TRIAL) {
                g_songName = "timetrial";
                g_musicMode = MUSIC_TIMETRIAL;
            }
            StartMusic();
            g_keyLatch[K_VK_F6] = 0;
        }
    } else {
        g_keyLatch[K_VK_F6] = 1;
    }

    // ---- Escape: quit dialog / pause ----
    if (KeyDown(K_VK_ESCAPE) == true) {
        if (g_keyLatch[K_VK_ESCAPE] != 0) {
            if (g_playerUpdateFn != StateDemo) {
                WinCloseAll();
                g_profileWinOpen = 0;
                g_buttonsOn = 0;
                HidePointer();
                g_quitGameWinOpen = 1;
                g_buttonsOn = 1;
                PauseGame();
                QuitGameDialog();
            } else {
                ResetToTitle();
                g_inputCooldown = 100;
            }
            g_keyLatch[K_VK_ESCAPE] = 0;
        }
    } else {
        g_keyLatch[K_VK_ESCAPE] = 1;
    }

    // ---- B (+Shift/+Alt): border mode / background tint / nebula ----
    if (KeyDown(K_VK_B) == true && IsKeyFree(K_VK_B)) {
        if (g_keyLatch[K_VK_B] != 0) {
            if (!KeyDown(K_VK_L_SHIFT) == true && !KeyDown(K_VK_R_SHIFT) == true &&
                !KeyDown(K_VK_MENU) == true) {

                // B: cycle the border mode (scrolling border / no scrolling / solid black).
                if (g_cfg.borderMode == BORDER_BLACK) {
                    g_cfg.borderMode = BORDER_ON;
                    sprintf(g_optionMsg, "BORDER : SCROLLING IS ON");
                    g_msgTime = g_time + MSG_DURATION_MS;
                    ImgSetBlitColor(g_gfxBorderEasy, 1.0f, 1.0f, 1.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderNormal, 1.0f, 1.0f, 1.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderHard, 1.0f, 1.0f, 1.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderAce, 1.0f, 1.0f, 1.0f, 1.0f);
                } else if (g_cfg.borderMode == BORDER_ON) {
                    g_cfg.borderMode = BORDER_OFF;
                    sprintf(g_optionMsg, "BORDER : SCROLLING IS OFF");
                    g_msgTime = g_time + MSG_DURATION_MS;
                    ImgSetBlitColor(g_gfxBorderEasy, 1.0f, 1.0f, 1.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderNormal, 1.0f, 1.0f, 1.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderHard, 1.0f, 1.0f, 1.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderAce, 1.0f, 1.0f, 1.0f, 1.0f);

                } else {
                    g_cfg.borderMode = BORDER_BLACK;
                    sprintf(g_optionMsg, "BORDER : BLACK BORDER");
                    g_msgTime = g_time + MSG_DURATION_MS;
                    ImgSetBlitColor(g_gfxBorderEasy, 0.0f, 0.0f, 0.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderNormal, 0.0f, 0.0f, 0.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderHard, 0.0f, 0.0f, 0.0f, 1.0f);
                    ImgSetBlitColor(g_gfxBorderAce, 0.0f, 0.0f, 0.0f, 1.0f);
                }

                g_keyLatch[K_VK_B] = 0;
                PlayClick();
            }

            // Shift+B: cycle the background tint brightness.
            if (KeyDown(K_VK_L_SHIFT) == true || KeyDown(K_VK_R_SHIFT) == true) {
                switch (g_cfg.bgTint) {  // cycle through 6 tint levels, 15 apart, wrapping to the first
                case BG_BRIGHTNESS_MIN: g_cfg.bgTint = BG_BRIGHTNESS_DEFAULT; break;
                case BG_BRIGHTNESS_DEFAULT: g_cfg.bgTint = BG_BRIGHTNESS_PRESET; break;
                case BG_BRIGHTNESS_PRESET: g_cfg.bgTint = 0x55; break;
                case 0x55: g_cfg.bgTint = 0x64; break;
                case 0x64: g_cfg.bgTint = BG_BRIGHTNESS_MAX; break;
                case BG_BRIGHTNESS_MAX: g_cfg.bgTint = BG_BRIGHTNESS_MIN; break;
                default: g_cfg.bgTint = BG_BRIGHTNESS_MIN;
                }
                sprintf(g_optionMsg, "BACKGROUND LIGHT : %d", (g_cfg.bgTint - BG_BRIGHTNESS_MIN) / BG_BRIGHTNESS_STEP + 1);
                g_msgTime = g_time + MSG_DURATION_MS;
                g_keyLatch[K_VK_B] = 0;
                PlayClick();
            }

            // Alt+B: toggle the background nebula.
            if (KeyDown(K_VK_MENU) == true) {
                if (g_cfg.bgEnabled == 0) {
                    g_cfg.bgEnabled = 1;
                    sprintf(g_optionMsg, "BACKGROUND NEBULA ON");
                    g_msgTime = g_time + MSG_DURATION_MS;
                } else {
                    g_cfg.bgEnabled = 0;
                    sprintf(g_optionMsg, "BACKGROUND NEBULA OFF");
                    g_msgTime = g_time + MSG_DURATION_MS;
                }
                g_keyLatch[K_VK_B] = 0;
                PlayClick();
            }
        }
    } else {
        g_keyLatch[K_VK_B] = 1;
    }

    // ---- W: toggle fullscreen / windowed (same as the title screen's W) ----
    if (KeyDown(K_VK_W) == true && IsKeyFree(K_VK_W)) {
        if (g_keyLatch[K_VK_W] != 0) {
            g_cfg.windowed = !g_cfg.windowed;
            SysSetFullscreen(!g_cfg.windowed);
            sprintf(g_optionMsg, g_cfg.windowed ? "SCREEN MODE : WINDOWED" : "SCREEN MODE : FULLSCREEN");
            g_msgTime = g_time + MSG_DURATION_MS;
            MergeSettings(g_profileIndex);
            WriteSettings();
            g_keyLatch[K_VK_W] = 0;
            PlayClick();
        }
    } else {
        g_keyLatch[K_VK_W] = 1;
    }

    // ---- S: cycle interpolation (same as the title screen's S) ----
    if (KeyDown(K_VK_S) == true && IsKeyFree(K_VK_S)) {
        if (g_keyLatch[K_VK_S] != 0) {
            sprintf(g_optionMsg, "%s", CycleInterpolation());
            g_msgTime = g_time + MSG_DURATION_MS;
            g_keyLatch[K_VK_S] = 0;
            PlayClick();
        }
    } else {
        g_keyLatch[K_VK_S] = 1;
    }

    // ---- P: pause ----
    if (KeyDown(K_VK_P) == true) {
        if (g_keyLatch[K_VK_P] != 0) {
            if (g_playerUpdateFn != StateDemo) {
                PauseGame();
            } else {
                ResetToTitle();
                g_inputCooldown = 100;
            }
        }
    } else {
        g_keyLatch[K_VK_P] = 1;
    }

    // ---- pause input (device): pause ----
    if (InputPause(g_curPlayer)) {
        if (g_save.players[g_curPlayer].keyLatchPause != 0) {
            if (g_playerUpdateFn != StateDemo) {
                PauseGame();
            } else {
                ResetToTitle();
                g_inputCooldown = 100;
            }
            g_save.players[g_curPlayer].keyLatchPause = 0;
        }
    } else {
        g_save.players[g_curPlayer].keyLatchPause = 1;
    }

    // ---- TAB: profile/stats window ----
    if (KeyDown(K_VK_TAB) == true && !KeyDown(K_VK_MENU) == true && !g_profileWinOpen &&
        g_time > g_statSubmitCooldown && g_playerUpdateFn != StateDemo) {
        if (g_keyLatch[K_VK_TAB] != 0) {
            if (g_playerUpdateFn != StateDemo) {
                if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
                    g_playerUpdateFn != StateDemo) {

                    // Bank scores, kill/perfect stats and best times into the profile.
                    if (g_gameMode == MODE_SINGLE)
                        UpdateHighScore(g_profileIndex, g_save.players[g_curPlayer].score);
                    UpdateMeteorStormScore(g_profileIndex, g_save.players[g_curPlayer].bonusHighScore);
                    if (g_gameMode == MODE_TIME_TRIAL)
                        UpdateTimeTrialScore(g_profileIndex, g_save.players[g_curPlayer].score);
                    AddStats(g_profileIndex, g_perfectCount, g_killCount);
                    CheckRatioMedal(g_profileIndex);
                    if (g_gameMode == MODE_SINGLE)
                        UpdateBestTime(g_profileIndex, g_timerMin1);
                    if (g_gameMode == MODE_SINGLE)
                        UpdateFastestMeteorStorm(g_profileIndex, g_timerMin2);
                    AddScoreStat(g_profileIndex, g_sessionScore);
                    g_sessionScore = 0;
                    AddHitsStat(g_profileIndex, g_hits);
                    g_hits = 0;
                    if (g_gameMode == MODE_SINGLE) {
                        UpdateHighestLevel(g_profileIndex, g_save.players[g_curPlayer].level);
                        AddLevelsPlayed(g_profileIndex, g_pendingLevelsPlayed);
                        g_pendingLevelsPlayed = 0;
                        UpdateHighestRank(g_profileIndex, g_save.players[g_curPlayer].rank);
                    }
                    UpdateHighestMoney(g_profileIndex, g_moneyMax);

                    // Pause and pop up the profile/stats window.
                    g_profileWinOpen = 1;
                    PauseGame();
                    g_profileReadOnly = 0;
                    ProfileWindow(1);
                    g_statSubmitCooldown = g_time + PROFILE_SUBMIT_COOLDOWN_MS;
                }
            } else {
                ResetToTitle();
                g_inputCooldown = 100;
            }
            g_keyLatch[K_VK_TAB] = 0;
        }
    } else {
        g_keyLatch[K_VK_TAB] = 1;
    }

    // ---- profile input (device): profile/stats window ----
    if (InputProfile(g_curPlayer) && !g_profileWinOpen && g_time > g_statSubmitCooldown &&
        g_playerUpdateFn != StateDemo) {
        if (g_save.players[g_curPlayer].keyLatchProfile != 0) {
            if (g_playerUpdateFn != StateDemo) {
                if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
                    g_playerUpdateFn != StateDemo) {

                    // Bank scores, kill/perfect stats and best times into the profile.
                    if (g_gameMode == MODE_SINGLE)
                        UpdateHighScore(g_profileIndex, g_save.players[g_curPlayer].score);
                    UpdateMeteorStormScore(g_profileIndex, g_save.players[g_curPlayer].bonusHighScore);
                    if (g_gameMode == MODE_TIME_TRIAL)
                        UpdateTimeTrialScore(g_profileIndex, g_save.players[g_curPlayer].score);
                    AddStats(g_profileIndex, g_perfectCount, g_killCount);
                    CheckRatioMedal(g_profileIndex);
                    if (g_gameMode == MODE_SINGLE)
                        UpdateBestTime(g_profileIndex, g_timerMin1);
                    if (g_gameMode == MODE_SINGLE)
                        UpdateFastestMeteorStorm(g_profileIndex, g_timerMin2);
                    AddScoreStat(g_profileIndex, g_sessionScore);
                    g_sessionScore = 0;
                    AddHitsStat(g_profileIndex, g_hits);
                    g_hits = 0;
                    if (g_gameMode == MODE_SINGLE) {
                        UpdateHighestLevel(g_profileIndex, g_save.players[g_curPlayer].level);
                        AddLevelsPlayed(g_profileIndex, g_pendingLevelsPlayed);
                        g_pendingLevelsPlayed = 0;
                        UpdateHighestRank(g_profileIndex, g_save.players[g_curPlayer].rank);
                    }
                    UpdateHighestMoney(g_profileIndex, g_moneyMax);

                    // Pause and pop up the profile/stats window.
                    g_profileWinOpen = 1;
                    PauseGame();
                    g_profileReadOnly = 0;
                    ProfileWindow(1);
                    g_statSubmitCooldown = g_time + PROFILE_SUBMIT_COOLDOWN_MS;
                }
            } else {
                ResetToTitle();
                g_inputCooldown = 100;
            }
            g_save.players[g_curPlayer].keyLatchProfile = 0;
        }
    } else {
        g_save.players[g_curPlayer].keyLatchProfile = 1;
    }

    // ---- ALT + V: toggle vsync (plain V below skips Alt, and resets the shared latch) ----
    if (KeyDown(K_VK_MENU) == true && KeyDown(K_VK_V) == true && IsKeyFree(K_VK_V) &&
        g_keyLatch[K_VK_V] != 0) {
        sprintf(g_optionMsg, "%s", ToggleVSync());
        g_msgTime = g_time + MSG_DURATION_MS;
        g_keyLatch[K_VK_V] = 0;
        PlayClick();
    }

    // ---- V: announcer voice ----
    if (KeyDown(K_VK_V) == true && IsKeyFree(K_VK_V)) {
        if (!KeyDown(K_VK_MENU) == true && g_keyLatch[K_VK_V] != 0) {
            g_keyLatch[K_VK_V] = 0;
            g_cfg.sfxOn = 1;
            if (g_cfg.sfxOn != 0) {
                // Start from the next voice after the current one (profile voice if a profile is active).
                SoundResetQueue();
                tries = 0;
                if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
                    g_playerUpdateFn != StateDemo)
                    voice = GetProfileVoiceIndex(g_profileIndex) + 1;
                else
                    voice = g_cfg.voice + 1;
                done = 0;

                // Search forward, wrapping at 99, giving up after two full wraps.
                do {
                    if (VoiceExists(voice)) {
                        if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
                            g_playerUpdateFn != StateDemo)
                            SetProfileVoiceIndex(g_profileIndex, voice);
                        else
                            g_cfg.voice = voice;

                        // Show a loading banner while the new voice pack streams in.
                        sprintf(g_alertMsg, "*  L O A D I N G   V O I C E  *  :%d", voice);
                        g_msgColor = 2;
                        g_msgTimer = g_time + 1000;
                        g_frameFunc();
                        FlushBlit(0);
                        FlushStretchF();
                        FlushStretchRot();
                        FlushStretchI();
                        FlushStretchRot2();
                        FlushBlit2(0);
                        FlipBuffer(0);

                        // Load the new pack and give an audio cue that it's ready.
                        LoadVoices();
                        EmptyPostTransitionHook();
                        if (g_sfxGetReady != 0)
                            SoundPlayVoice(g_sfxGetReady, -1, 0xff, 0.0f, 0xff, g_sndFlags);
                        done = 1;
                    } else {
                        voice++;
                        if (voice > 99) {
                            voice = 1;
                            tries++;
                        }
                        if (tries > 2)
                            done = 1;
                    }
                } while (!done);
            }
        }
    } else {
        g_keyLatch[K_VK_V] = 1;
    }

    // ---- E (+Shift to reverse): explosion spark count ----
    if (KeyDown(K_VK_E) == true && IsKeyFree(K_VK_E)) {
        if (!KeyDown(K_VK_L_SHIFT) == true && !KeyDown(K_VK_R_SHIFT) == true) {
            g_cfg.sparks += 2.0;
            if (g_cfg.sparks > 150.0)
                g_cfg.sparks = 150.0;
            g_maxSparks = ((int)g_cfg.sparks >> 1 < 5) ? 5 : (int)g_cfg.sparks >> 1;
            sprintf(g_optionMsg, "MAX EXPLOSION SPARKS:%d", (int)g_cfg.sparks);
            g_msgTime = g_time + MSG_DURATION_MS;
        }
        if (KeyDown(K_VK_L_SHIFT) == true || KeyDown(K_VK_R_SHIFT) == true) {
            g_cfg.sparks -= 2.0;
            if (g_cfg.sparks < 10.0)
                g_cfg.sparks = 10.0;
            g_maxSparks = ((int)g_cfg.sparks >> 1 < 5) ? 5 : (int)g_cfg.sparks >> 1;
            sprintf(g_optionMsg, "MAX EXPLOSION SPARKS:%d", (int)g_cfg.sparks);
            g_msgTime = g_time + MSG_DURATION_MS;
        }
    }

    // ---- N (+Shift to reverse): star count ----
    if (KeyDown(K_VK_N) == true && IsKeyFree(K_VK_N)) {
        if (!KeyDown(K_VK_L_SHIFT) == true && !KeyDown(K_VK_R_SHIFT) == true) {
            g_cfg.numStars += 10.0;
            if (g_cfg.numStars > 3000.0)
                g_cfg.numStars = 3000.0;
            sprintf(g_optionMsg, "NUMBERS OF STARS : %d", (int)g_cfg.numStars);
            g_msgTime = g_time + MSG_DURATION_MS;
        }
        if (KeyDown(K_VK_L_SHIFT) == true || KeyDown(K_VK_R_SHIFT) == true) {
            g_cfg.numStars -= 10.0;
            if (g_cfg.numStars < 50.0)
                g_cfg.numStars = 50.0;
            sprintf(g_optionMsg, "NUMBERS OF STARS : %d", (int)g_cfg.numStars);
            g_msgTime = g_time + MSG_DURATION_MS;
        }
    }

    // ---- F / Alt+Shift+F: flare particles / FPS display ----
    if (KeyDown(K_VK_F) == true && !AnyWindowActive() && IsKeyFree(K_VK_F)) {
        if (g_keyLatch[K_VK_F] != 0) {
            // Alt+Shift+F: toggle the FPS display.
            if (KeyDown(K_VK_MENU) == true &&
                (KeyDown(K_VK_L_SHIFT) == true || KeyDown(K_VK_R_SHIFT) == true)) {
                g_keyLatch[K_VK_F] = 0;
                PlayClick();
                if ((g_showFps = !g_showFps) != 0) {
                    sprintf(g_optionMsg, "SHOW FRAME RATE");
                    g_msgTime = g_time + MSG_DURATION_MS;
                } else {
                    sprintf(g_optionMsg, "HIDE FRAME RATE");
                    g_msgTime = g_time + MSG_DURATION_MS;
                }
            }

            // Plain F: toggle bullet flare particle effects.
            if (!KeyDown(K_VK_MENU) == true && KeyDown(K_VK_L_SHIFT) != true &&
                KeyDown(K_VK_R_SHIFT) != true) {
                g_keyLatch[K_VK_F] = 0;
                PlayClick();
                g_cfg.particlesOn = !g_cfg.particlesOn;
                if (g_cfg.particlesOn) {
                    sprintf(g_optionMsg, "USE FLARE EFFECTS");
                    g_msgTime = g_time + MSG_DURATION_MS;
                } else {
                    sprintf(g_optionMsg, "NO FLARE EFFECTS");
                    g_msgTime = g_time + MSG_DURATION_MS;
                }
            }
        }
    } else {
        g_keyLatch[K_VK_F] = 1;
    }

    // ---- I: bullet-render intensity ----
    if (KeyDown(K_VK_I) == true && IsKeyFree(K_VK_I)) {
        if (g_keyLatch[K_VK_I] != 0) {
            g_keyLatch[K_VK_I] = 0;
            PlayClick();
            g_cfg.bulletIntensity++;
            if (g_cfg.bulletIntensity > BULLETS_FLARE_FX)
                g_cfg.bulletIntensity = BULLETS_NORMAL;
            if (g_cfg.bulletIntensity == BULLETS_NORMAL) {
                sprintf(g_optionMsg, "BULLET INTENSITY :NORMAL");
                g_msgTime = g_time + MSG_DURATION_MS;
            }
            if (g_cfg.bulletIntensity == BULLETS_BRIGHT) {
                sprintf(g_optionMsg, "BULLET INTENSITY :BRIGHT");
                g_msgTime = g_time + MSG_DURATION_MS;
            }
            if (g_cfg.bulletIntensity == BULLETS_FLARE_FX) {
                sprintf(g_optionMsg, "BULLET INTENSITY :FLARE FX");
                g_msgTime = g_time + MSG_DURATION_MS;
            }

            if (g_cfg.bulletIntensity == BULLETS_BRIGHT)
                g_drawLevelObjectsFn = DrawLevelObjectsBright;
            else
                g_drawLevelObjectsFn = DrawLevelObjectsNormal;
        }
    } else {
        g_keyLatch[K_VK_I] = 1;
    }

    // ---- Z: point vs flare stars ----
    if (KeyDown(K_VK_Z) == true && IsKeyFree(K_VK_Z)) {
        if (g_keyLatch[K_VK_Z] != 0) {
            g_cfg.bgStars = !g_cfg.bgStars;
            if (g_cfg.bgStars)
                g_fnPtr = DrawStarsPlayer;
            else
                g_fnPtr = DrawSpriteStars;
            g_keyLatch[K_VK_Z] = 0;
            PlayClick();
            if (g_cfg.bgStars) {
                sprintf(g_optionMsg, "POINT STARS ON");
                g_msgTime = g_time + MSG_DURATION_MS;
            } else {
                sprintf(g_optionMsg, "FLARE STARS ON");
                g_msgTime = g_time + MSG_DURATION_MS;
            }
        }
    } else {
        g_keyLatch[K_VK_Z] = 1;
    }

    if (g_autoplay && g_save.players[g_curPlayer].weapon < WEAPON_QUAD)
        g_save.players[g_curPlayer].weapon = RandRange(WEAPON_QUAD, WEAPON_WAR_PLASMA);

    // ---- M: music format ----
    if (KeyDown(K_VK_M) == true && IsKeyFree(K_VK_M)) {
        if (g_keyLatch[K_VK_M] != 0) {
            g_lastActivityTime = g_time;
            g_attractScreen = 3;
            g_idleTimeoutMs = ATTRACT_SCREEN_MS;
            CycleMusicFormat();
            g_keyLatch[K_VK_M] = 0;
            PlayClick();
        }
    } else {
        g_keyLatch[K_VK_M] = 1;
    }

    // ---- U: voice on/off ----
    if (KeyDown(K_VK_U) == true && IsKeyFree(K_VK_U)) {
        if (g_keyLatch[K_VK_U] != 0) {
            g_lastActivityTime = g_time;
            g_attractScreen = 3;
            g_idleTimeoutMs = ATTRACT_SCREEN_MS;
            g_cfg.sfxOn = !g_cfg.sfxOn;
            if (g_cfg.sfxOn) {
                sprintf(g_optionMsg, "VOICE ON");
                g_msgTime = g_time + MSG_DURATION_MS;
            } else {
                sprintf(g_optionMsg, "VOICE OFF");
                g_msgTime = g_time + MSG_DURATION_MS;
            }
            g_keyLatch[K_VK_U] = 0;
            PlayClick();
        }
    } else {
        g_keyLatch[K_VK_U] = 1;
    }

    // PageDown/PageUp: sfx volume.
    // ---- PageDown/PageUp: sfx volume ----
    if (KeyDown(K_VK_PAGEDOWN) == true && IsKeyFree(K_VK_PAGEDOWN)) {
        g_lastActivityTime = g_time;
        g_attractScreen = 3;
        g_idleTimeoutMs = ATTRACT_SCREEN_MS;
        g_cfg.sfxVol--;
        if (g_cfg.sfxVol < 0)
            g_cfg.sfxVol = 0;
        SetSfxVolume(g_cfg.sfxVol);
        sprintf(g_optionMsg, "EFFECT VOLUM : %d", (int)(g_cfg.sfxVol / VOLUME_DISPLAY_SCALE));
        g_msgTime = g_time + MSG_DURATION_MS;
    }
    if (KeyDown(K_VK_PAGEUP) == true && IsKeyFree(K_VK_PAGEUP)) {
        g_lastActivityTime = g_time;
        g_attractScreen = 3;
        g_idleTimeoutMs = ATTRACT_SCREEN_MS;
        g_cfg.sfxVol++;
        if (g_cfg.sfxVol > 0xff)
            g_cfg.sfxVol = 0xff;
        SetSfxVolume(g_cfg.sfxVol);
        sprintf(g_optionMsg, "EFFECT VOLUM : %d", (int)(g_cfg.sfxVol / VOLUME_DISPLAY_SCALE));
        g_msgTime = g_time + MSG_DURATION_MS;
    }

    // Numpad -/+: music volume; numpad *: restart the current song.
    // ---- numpad -/+/*: music volume / restart song ----
    if (KeyDown(K_VK_SUBTRACT) == true && IsKeyFree(K_VK_SUBTRACT)) {
        g_cfg.musicVolume--;
        if (g_cfg.musicVolume < 0)
            g_cfg.musicVolume = 0;
        sprintf(g_optionMsg, "MUSIC VOLUM : %d", (int)(g_cfg.musicVolume / VOLUME_DISPLAY_SCALE));
        g_msgTime = g_time + MSG_DURATION_MS;
        ApplyMusicVolume();
    }
    if (KeyDown(K_VK_ADD) == true && IsKeyFree(K_VK_ADD)) {
        g_cfg.musicVolume++;
        if (g_cfg.musicVolume > 0xff)
            g_cfg.musicVolume = 0xff;
        sprintf(g_optionMsg, "MUSIC VOLUM : %d", (int)(g_cfg.musicVolume / VOLUME_DISPLAY_SCALE));
        g_msgTime = g_time + MSG_DURATION_MS;
        ApplyMusicVolume();
    }
    if (KeyDown(K_VK_MULTIPLY) == true && IsKeyFree(K_VK_MULTIPLY))
        StartMusic();

    // End/Home: voice volume.
    // ---- End/Home: voice volume ----
    if (KeyDown(K_VK_END) == true && IsKeyFree(K_VK_END)) {
        g_cfg.musicVol--;
        if (g_cfg.musicVol < 0)
            g_cfg.musicVol = 0;
        SetMusicVolTable(g_cfg.musicVol);
        sprintf(g_optionMsg, "VOICE VOLUM : %d", (int)(g_cfg.musicVol / VOLUME_DISPLAY_SCALE));
        g_msgTime = g_time + MSG_DURATION_MS;
    }
    if (KeyDown(K_VK_HOME) == true && IsKeyFree(K_VK_HOME)) {
        g_cfg.musicVol++;
        if (g_cfg.musicVol > 0xff)
            g_cfg.musicVol = 0xff;
        SetMusicVolTable(g_cfg.musicVol);
        sprintf(g_optionMsg, "VOICE VOLUM : %d", (int)(g_cfg.musicVol / VOLUME_DISPLAY_SCALE));
        g_msgTime = g_time + MSG_DURATION_MS;
    }
}

#define FRAME_TIME() \
    g_frameTime = g_time; g_frameDeltaMs = g_frameTime - g_lastFrameTick; g_lastFrameTick = g_frameTime

// Shared per-frame gameplay update pipeline: particles/popups/explosions/debris/slots/map
// objects/hyperspace/items/level objects/enemies, then loop-sample fades and the player.
// Identical in STATE_MALFUNCTION_DEATH, STATE_MALFUNCTION, STATE_RESPAWN, STATE_GET_READY
// and STATE_PLAYING (which only differ before/after this block).
#define GAMEPLAY_UPDATE_CORE() \
    UpdateParticles(); \
    UpdateScorePopups(); \
    UpdateExplosions(); \
    UpdateExplosionDebris(); \
    UpdateSlots(g_frameDt); \
    UpdateMapObjects(); \
    UpdateHyperspace(); \
    UpdateItems(); \
    UpdateLevelObjects(); \
    UpdateEnemies(); \
    FadeLoopSamples(); \
    g_playerUpdateFn()

#define CAMERA_RESET() \
    g_shopSelItem = 0; g_offX = 0; g_shopSlideVelX = 0; g_offY = -600.0f; g_shopSlideVelY = 40.0f; \
    g_shopBounceY = -30.0f

#define PLAYER_DIED() \
    g_save.players[g_curPlayer].hyperspaceOutTimer = g_save.players[g_curPlayer].hyperspaceInDuration; \
    if (g_gameMode == MODE_DUAL) { \
        if (!g_save.players[0].alienLock) { g_save.players[0].shieldL = 0; g_save.players[0].shieldR = 0; } \
        g_save.players[0].freezeTimer = 0; \
        if (!g_save.players[1].alienLock) { g_save.players[1].shieldL = 0; g_save.players[1].shieldR = 0; } \
        g_save.players[1].freezeTimer = 0; \
    } else { \
        if (!g_save.players[g_curPlayer].alienLock) { \
            g_save.players[g_curPlayer].shieldL = 0; \
            g_save.players[g_curPlayer].shieldR = 0; \
        } \
        g_save.players[g_curPlayer].freezeTimer = 0; \
    } \
    g_deathSeqActive = 1; \
    g_save.players[g_curPlayer].levelFinished = 0; \
    g_state = STATE_MALFUNCTION_DEATH

#define CONTINUE_BODY() \
    g_save.players[i].shieldHitFlashSpeed += 0.5; \
    if (g_save.players[i].shieldHitFlashSpeed > 8.0) g_save.players[i].shieldHitFlashSpeed = 8.0f; \
    g_save.players[i].enemyHpBonusRoll += 2; \
    if (g_save.players[i].enemyHpBonusRoll > 60) g_save.players[i].enemyHpBonusRoll = 60; \
    g_save.players[i].levelFinished = 1; \
    CAMERA_RESET()

#define LEVEL_DONE() \
    g_transitionLockUntil = g_time + TRANSITION_LOCK_MS; \
    g_transitionLock = 1; \
    OnLevelComplete(); \
    if (g_save.players[g_curPlayer].done == 0) { \
        g_save.players[g_curPlayer].done = 1; \
        if (g_save.players[g_curPlayer].doneTime == 0) \
            g_save.players[g_curPlayer].doneTime = g_time + 3000; \
    } \
    g_save.players[g_curPlayer].killed = g_save.players[g_curPlayer].totalEnemies; \
    g_save.players[g_curPlayer].escaped = 0; \
    ResetObjectsKeep(); \
    g_save.players[0].energy = 0; \
    g_save.players[1].energy = 0; \
    g_save.players[2].energy = 0; \
    g_save.players[3].energy = 0; \
    SetHurryUpTimer(); \
    StartNextLevel()

// Top-level per-frame update, called once per rendered frame: reads mouse/clamps it to
// the window, restarts music/voice queues on their timers, handles the global screenshot
// hotkey and the cheat-detected kill switch, then dispatches on g_state to update the
// current screen/mode (menus, paused overlay, the various gameplay sub-states, bonus
// stages, shop, hiscore/profile screens) and pick g_frameFunc for the matching renderer.
void GameFrame()
{
    int i = 0;
    int moved = 0;
    int cnt;
    int r;

    g_time = SysMillis();
    if (g_time == 0) g_time = SysMillis();
    if (g_time > g_transitionLockUntil) g_transitionLock = 0;

    g_prevMouseX = g_mouseX;
    g_prevMouseY = g_mouseY;
    g_mouseX = MouseX();
    g_mouseY = MouseY();
    if (!g_windowed) {
        ClipCursorOn();
        if (g_mouseX < 0) MouseWarp(0, g_mouseY);
        g_mouseX = MouseX();
        if (g_mouseX >= (int)g_screenW) MouseWarp(g_screenW, g_mouseY);
        g_mouseX = MouseX();
        if (g_mouseY < 0) MouseWarp(g_mouseX, 0);
        g_mouseY = MouseY();
        if (g_mouseY >= (int)g_screenH) MouseWarp(g_mouseX, g_screenH);
        g_mouseY = MouseY();
    }
    if (g_prevMouseX == g_mouseX && g_prevMouseY == g_mouseY)
        moved = 0;
    else
        moved = 1;
    if (moved) {
        g_buttonsOn = 1;
        g_idleFrames = 0;
    }

    if (g_musicRestartTime != 0 && g_time > g_musicRestartTime) {
        g_musicRestartTime = 0;
        StartMusic();
        if (g_state == STATE_PAUSED) SoundPause();
    }
    if (!g_loginWinOpen) {
        g_loginWinOpen = true;
        SoundQueueAdd(g_sfxWelcome, 0x32, 0);
    }

    // Advance the bonus-icon animation frames while not paused.
    if (g_state != STATE_PAUSED) {
        g_bonusIconAnimTimer -= 1.0;
        if (g_bonusIconAnimTimer < 0.0) {
            g_bonusIconAnimTimer = 6.0f;
            g_bonusIconFrame1++;
            if (g_bonusIconFrame1 > 9) g_bonusIconFrame1 = 0;
            g_bonusIconFrame2++;
            if (g_bonusIconFrame2 > 9) g_bonusIconFrame2 = 0;
            g_bonusIconFrame3++;
            if (g_bonusIconFrame3 > 9) g_bonusIconFrame3 = 0;
            g_bonusIconFrame4++;
            if (g_bonusIconFrame4 > 9) g_bonusIconFrame4 = 0;
            g_bonusIconFrame5++;
            if (g_bonusIconFrame5 > 9) g_bonusIconFrame5 = 0;
        }
    }

    // Loop the music when the current song has finished, dropping the hiscore-skip
    // gate once its lock has expired.
    if (g_songEndTime != 0 && g_time > g_songEndTime) {
        if (g_songLengthMs > 1740000) {
            StartMusic();
        } else {
            StartMusic();
            if (g_hiscoreSkipGateActive != 0 && g_time > g_rankLockUntil) {
                g_rankLockUntil = g_time;
                g_rankPopupMinTime = g_time;
                g_hiscoreSkipGateActive = 0;
                g_rankMsgActive = 0;
                g_promoSpecialRank = false;
                g_promoRingActive = false;
            }
        }
    }
    if (g_state == STATE_PLAYING) {
        g_buttonsOn = 0;
        HidePointer();
    }

    g_exitSoundElapsed = g_time - g_exitSoundStartTime;
    SoundQueueUpdate();
    if (g_gameMode == MODE_TIME_TRIAL && g_time > g_timeTrialDeadline && g_timeTrialDeadline != 0xffffffff)
        ShowHiscoreTable();
    if (g_inputCooldown > 0) g_inputCooldown--;

    if (KeyDown(K_VK_F7) == true) {
        if (g_screenshotKeyEdge != 0) {
            TakeScreenshot();
            g_screenshotKeyEdge = 0;
            PlayClick();
        }
    } else {
        g_screenshotKeyEdge = 1;
    }

    switch (g_state) {
    case STATE_TITLE:  // title/menu screen
        UpdateMenuStars();
        MenuHandler();
        break;
    case STATE_PAUSED:  // paused: handles the quit/profile windows and their close hotkeys, keeps timers frozen
        // ---- F6: restart music ----
        if (KeyDown(K_VK_F6) == true) {
            if (g_keyLatch[K_VK_F6] != 0) {
                g_songName = "warblade";
                g_musicMode = MUSIC_CUSTOM;
                if (g_gameMode == MODE_TIME_TRIAL) {
                    g_songName = "timetrial";
                    g_musicMode = MUSIC_TIMETRIAL;
                }
                StartMusic();
                g_keyLatch[K_VK_F6] = 0;
            }
        } else {
            g_keyLatch[K_VK_F6] = 1;
        }

        // "Quit to Windows" confirmation dialog: Escape (or the dialog closing itself) resumes.
        if (g_quitToWindowsWinOpen || g_dialogWinOpen) {
            if (moved) WinClearMenuChecks();
            if (KeyDown(K_VK_ESCAPE) == true || !g_quitToWindowsWinOpen) {
                if (g_keyLatch[K_VK_ESCAPE] != 0) {
                    ResumeGame();
                    PlaySample();
                    WinCloseAll();
                    g_quitToWindowsWinOpen = false;
                    g_buttonsOn = 0;
                    HidePointer();
                    g_keyLatch[K_VK_ESCAPE] = 0;
                }
            } else {
                g_keyLatch[K_VK_ESCAPE] = 1;
            }
            MenuUpdate(moved != 0);
        }

        // "Quit game" confirmation dialog: same handling as above.
        if (g_quitGameWinOpen || g_dialogWinOpen) {
            if (moved) WinClearMenuChecks();
            if (KeyDown(K_VK_ESCAPE) == true || !g_quitGameWinOpen) {
                if (g_keyLatch[K_VK_ESCAPE] != 0) {
                    ResumeGame();
                    PlaySample();
                    WinCloseAll();
                    g_quitGameWinOpen = false;
                    g_buttonsOn = 0;
                    HidePointer();
                    g_keyLatch[K_VK_ESCAPE] = 0;
                }
            } else {
                g_keyLatch[K_VK_ESCAPE] = 1;
            }
            MenuUpdate(moved != 0);
        }

        // Profile/stats window: TAB or the profile hotkey closes it and resumes.
        if (g_profileWinOpen) {
            if (moved) WinClearMenuChecks();
            MenuUpdate(moved != 0);
            if (KeyDown(K_VK_TAB) == true && !KeyDown(K_VK_MENU) == true &&
                g_profileWinOpen && g_time > g_statSubmitCooldown && g_playerUpdateFn != StateDemo) {
                if (g_keyLatch[K_VK_TAB] != 0) {
                    ResumeGame();
                    PlaySample();
                    WinCloseAll();
                    g_profileWinOpen = false;
                    g_buttonsOn = 0;
                    HidePointer();
                    g_keyLatch[K_VK_TAB] = 0;
                    g_statSubmitCooldown = g_time + PROFILE_SUBMIT_COOLDOWN_MS;
                }
            } else {
                g_keyLatch[K_VK_TAB] = 1;
            }

            if (InputProfile(g_curPlayer) && g_profileWinOpen && g_time > g_statSubmitCooldown) {
                if (g_save.players[g_curPlayer].keyLatchProfile != 0) {
                    ResumeGame();
                    PlaySample();
                    WinCloseAll();
                    g_profileWinOpen = false;
                    g_buttonsOn = 0;
                    HidePointer();
                    g_save.players[g_curPlayer].keyLatchProfile = 0;
                    g_statSubmitCooldown = g_time + PROFILE_SUBMIT_COOLDOWN_MS;
                }
            } else {
                g_save.players[g_curPlayer].keyLatchProfile = 1;
            }
        }

        // ---- resume inputs ----
        // With no dialog/window open, P, the pause hotkey, space or fire all resume the game.
        if (KeyDown(K_VK_P) == true && !AnyWindowHasEdit() && !g_profileWinOpen &&
            !g_quitGameWinOpen && !g_quitToWindowsWinOpen) {
            if (g_keyLatch[K_VK_P] != 0) {
                g_keyLatch[K_VK_P] = 0;
                ResumeGame();
            }
        } else {
            g_keyLatch[K_VK_P] = 1;
        }
        if (InputPause(g_curPlayer) && !AnyWindowHasEdit() && !g_profileWinOpen && !g_quitGameWinOpen &&
            !g_quitToWindowsWinOpen) {
            if (g_save.players[g_curPlayer].keyLatchPause != 0) {
                g_save.players[g_curPlayer].keyLatchPause = 0;
                ResumeGame();
            }
        } else {
            g_save.players[g_curPlayer].keyLatchPause = 1;
        }
        if ((KeyDown(K_VK_SPACE) == true || InputFire(0) || InputFire(1)) &&
            !AnyWindowHasEdit() && !g_profileWinOpen && !g_quitGameWinOpen && !g_quitToWindowsWinOpen)
            ResumeGame();

        // Resuming: shift every per-player timer forward by however long the game was paused.
        for (i = 0; i < NUM_PLAYERS; i++) {
            if (g_save.players[i].drunkModeTimer > 0)
                g_save.players[i].drunkModeTimer += g_time - g_pauseLastTick;
            if (g_save.players[i].freezeTimer > 0) g_save.players[i].freezeTimer += g_time - g_pauseLastTick;
            if (g_save.players[i].scoreMult2Timer > 0)
                g_save.players[i].scoreMult2Timer += g_time - g_pauseLastTick;
            if (g_save.players[i].scoreMult5Timer > 0)
                g_save.players[i].scoreMult5Timer += g_time - g_pauseLastTick;
            if (g_save.players[i].scoopTimer > 0) g_save.players[i].scoopTimer += g_time - g_pauseLastTick;
            if (g_save.players[i].shieldTimer > 0) g_save.players[i].shieldTimer += g_time - g_pauseLastTick;
            if (g_save.players[i].mirrorTime > 0) g_save.players[i].mirrorTime += g_time - g_pauseLastTick;
            if (g_save.players[i].time > 0) g_save.players[i].time += g_time - g_pauseLastTick;
        }

        // Same shift for the various global stage/message/countdown timers.
        if (g_memStageDeadline > 0) g_memStageDeadline = g_time - g_pauseLastTick + g_memStageDeadline;
        if (g_gridAnimTime > 0) g_gridAnimTime = g_time - g_pauseLastTick + g_gridAnimTime;
        if (g_memPickResolveTime > 0) g_memPickResolveTime = g_time - g_pauseLastTick + g_memPickResolveTime;
        if (g_pairsTickTime > 0) g_pairsTickTime = g_time - g_pauseLastTick + g_pairsTickTime;
        if (g_pauseShiftedTimerE > 0) g_pauseShiftedTimerE = g_time - g_pauseLastTick + g_pauseShiftedTimerE;
        if (g_levelBannerTime > 0) g_levelBannerTime = g_time - g_pauseLastTick + g_levelBannerTime;
        if (g_msgTimer > 0) g_msgTimer = g_time - g_pauseLastTick + g_msgTimer;
        if (g_alertTextTime > 0) g_alertTextTime = g_time - g_pauseLastTick + g_alertTextTime;
        if (g_msgTime > 0) g_msgTime = g_time - g_pauseLastTick + g_msgTime;
        if (g_bonusResultsTime > 0) g_bonusResultsTime = g_time - g_pauseLastTick + g_bonusResultsTime;
        if (g_perfectCheckDelay > 0) g_perfectCheckDelay = g_time - g_pauseLastTick + g_perfectCheckDelay;
        if (g_timeTrialDeadline > 0) g_timeTrialDeadline = g_time - g_pauseLastTick + g_timeTrialDeadline;
        if (g_rankLockUntil > 0) g_rankLockUntil = g_time - g_pauseLastTick + g_rankLockUntil;
        if (g_rankPopupMinTime > 0) g_rankPopupMinTime = g_time - g_pauseLastTick + g_rankPopupMinTime;
        if (g_rankPopupMinTime > g_time + 4000) {
            g_rankLockUntil = g_time + 1200000;
            g_rankPopupMinTime = g_time + 4000;
        }

        g_playerStallTime = g_time;
        g_pauseLastTick = g_time;
        UpdateStarItems();

        // Title-screen pulse/color-cycle effect bookkeeping.
        if (--g_pulseCycleTimer < 0) {
            if (g_pulseHoldCount > 0) {
                g_pulseHoldCount--;
                g_fxAngleStep += 0.2f;
            } else if (g_fxAngleStep > 0.0) {
                g_fxAngleStep -= 0.3f;
            } else {
                g_pulseCycleTimer = 200;
                g_pulseHoldCount = 200;
                g_fxAngleStep = 0;
            }
        }
        g_fxSpinAngle += 3.0;
        if (g_fxSpinAngle >= 360.0) g_fxSpinAngle = 0;
        g_fxRed += g_fxColorSpeedR;
        if (g_fxRed < 0.0) g_fxColorSpeedR = 0 - g_fxColorSpeedR;
        if (g_fxRed > 255.0) g_fxColorSpeedR = 0 - g_fxColorSpeedR;
        g_fxColorG += g_fxGreenVel;
        if (g_fxColorG < 0.0) g_fxGreenVel = 0 - g_fxGreenVel;
        if (g_fxColorG > 255.0) g_fxGreenVel = 0 - g_fxGreenVel;
        g_fxBlueColorLevel += g_fxBlueVel;
        if (g_fxBlueColorLevel < 0.0) g_fxBlueVel = 0 - g_fxBlueVel;
        if (g_fxBlueColorLevel > 255.0) g_fxBlueVel = 0 - g_fxBlueVel;

        CLEAR_DRAW_COUNTERS();

        // Restore whichever per-frame render function was active before the pause.
        if (g_savedState == STATE_BONUS_RACE) g_frameFunc = RenderMeteorStormRaceFrame;
        else if (g_savedState == STATE_MEMORY_STATION) g_frameFunc = RenderMemoryStationFrame;
        else if (g_savedState == STATE_SHOP) g_frameFunc = Shop;
        else if (g_savedState == STATE_SHOP_GATE) g_frameFunc = RenderShopGateFrame;
        else g_frameFunc = RenderGameplayFrame;
        break;

    case STATE_MALFUNCTION_DEATH:  // warp-malfunction death sequence (entered by PLAYER_DIED)
        Hotkeys();
        FRAME_TIME();
        WarpMalfunction();
        g_itemsVsPlayerFn();
        GAMEPLAY_UPDATE_CORE();
        UpdateGetReadyRespawn();
        g_frameFunc = RenderGameplayFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_MALFUNCTION:  // gameplay with the warp-malfunction alarm ticking
        Hotkeys();
        FRAME_TIME();
        if (g_frameTime > g_malfunctionAlarmTime && g_malfunctionBeepCount > 0) {
            g_malfunctionAlarmTime = g_frameTime + 1600;
            g_malfunctionBeepCount--;
            SoundQueueAdd(g_sfxWarpMalfunction, 0x32, 0);
        }

        g_itemsVsPlayerFn();
        PlayerShotsHitEnemies();
        GAMEPLAY_UPDATE_CORE();
        if (g_save.players[g_curPlayer].levelFinished != 0 &&
            g_save.players[g_curPlayer].trackKillsFlag == 0) {
            SoundPlay(g_sfxWarp, -1, 0xff, 0, 0x7f, g_sndFlags);
            PLAYER_DIED();
        }

        LevelStallWatchdog();
        i = g_curPlayer;
        if (g_gameMode == MODE_DUAL) i = 0;
        if (g_save.players[i].done != 0 && g_time > g_save.players[i].doneTime) {
            g_save.players[i].levelFinished = 1;
            CAMERA_RESET();
            g_save.players[i].levelTransitioning = 1;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        g_bulletsVsPlayerFn();
        g_frameFunc = RenderGameplayFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_RESPAWN:  // respawning after death; checks for game over before the new "get ready"
        if (IsGameOver()) ShowHiscoreTable();
        Hotkeys();
        FRAME_TIME();
        g_itemsVsPlayerFn();
        PlayerShotsHitEnemies();
        SpawnMoneySucker();
        GAMEPLAY_UPDATE_CORE();
        UpdateGetReadyRespawn();
        g_frameFunc = RenderGameplayFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_GET_READY:  // "get ready" for a freshly started level
        Hotkeys();
        FRAME_TIME();
        g_itemsVsPlayerFn();
        PlayerShotsHitEnemies();
        SpawnMoneySucker();
        GAMEPLAY_UPDATE_CORE();
        UpdateGetReadyNewLevel();
        g_frameFunc = RenderGameplayFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_PLAYING:  // normal gameplay
        Hotkeys();
        FRAME_TIME();
        g_itemsVsPlayerFn();
        PlayerShotsHitEnemies();
        SpawnMoneySucker();
        HurryUp();
        SpawnEliteFlyby();

        GAMEPLAY_UPDATE_CORE();
        if (g_save.players[g_curPlayer].levelFinished != 0 &&
            g_save.players[g_curPlayer].trackKillsFlag == 0 && g_noSpritesDrawn != 0) {
            SoundPlay(g_sfxWarp, -1, 0xff, 0, 0x7f, g_sndFlags);
            SoundPlay(g_sfxWarp3, -1, 0xff, 0, 0x7f, g_sndFlags);
            PLAYER_DIED();
        }

        LevelStallWatchdog();
        i = g_curPlayer;
        if (g_gameMode == MODE_DUAL) i = 0;
        if (g_save.players[i].done != 0 && g_time > g_save.players[i].doneTime) {
            if (g_gameMode == MODE_TIME_TRIAL && g_musicMode != MUSIC_TIMETRIAL) PlayGameMusic();

            // Bonus-tally rounds show the per-kill breakdown before moving on.
            if (g_save.players[i].trackKillsFlag != 0) {
                if (g_save.players[i].bonusResultsInitDone == 0) {
                    if (g_gameMode == MODE_DUAL) {
                        g_save.players[0].bonusTally = 0;
                        g_save.players[0].perfectDone = 0;
                        g_save.players[0].bonusResultsInitDone = 1;
                        g_save.players[1].bonusTally = 0;
                        g_save.players[1].perfectDone = 0;
                        g_save.players[1].bonusResultsInitDone = 1;
                    } else {
                        g_save.players[i].bonusTally = 0;
                        g_save.players[i].perfectDone = 0;
                        g_save.players[i].bonusResultsInitDone = 1;
                    }
                    g_bonusResultsTime = g_time + 4000;

                } else if (g_bonusResultsTime == 0) {
                    if (g_save.players[i].level > 0 && IsSpecialLevel(g_save.players[i].level) &&
                        g_deathSeqActive == 0) {
                        CONTINUE_BODY();
                        g_save.players[i].levelTransitioning = 1;
                        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                        g_transitionLock = 1;
                    } else if (g_save.players[i].levelFinished == 0 &&
                               g_save.players[i].levelTransitioning == 0) {
                        StartNextLevel();
                    }
                }

            // Otherwise just advance: continue into a special level or start the next one.
            } else {
                if (g_save.players[i].level > 0 && IsSpecialLevel(g_save.players[i].level) &&
                    g_deathSeqActive == 0) {
                    CONTINUE_BODY();
                    if (g_gameMode != MODE_TIME_TRIAL) g_save.players[i].levelTransitioning = 1;
                    g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                    g_transitionLock = 1;
                } else if (g_save.players[i].levelFinished == 0 &&
                           g_save.players[i].levelTransitioning == 0) {
                    StartNextLevel();
                }
            }
        }

        g_bulletsVsPlayerFn();
        g_grabEnemyFn();
        g_frameFunc = RenderGameplayFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_MEMORY_STATION:  // memory-station bonus stage
        ResetPlayerTimers();
        Hotkeys();
        UpdateParticles();
        UpdateScorePopups();
        UpdateMemoryStationIntro();
        FinishMemoryStation();
        if (g_state != STATE_MEMORY_STATION) {
            g_frameFunc = RenderGameplayFrame;
            CLEAR_DRAW_COUNTERS();
            break;
        }
        UpdateHyperspace();
        if (g_memoryIntro == 0 && g_memoryDone == 0) MemoryBonusUpdate();
        g_frameFunc = RenderMemoryStationFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_BONUS_RACE:  // bonus race stage
        ResetPlayerTimers();
        Hotkeys();
        g_itemsVsPlayerFn();
        UpdateMeteorStormIntroGate();
        UpdateParticles();
        MeteorStormUpdate();
        UpdateHyperspace();
        UpdateItems();
        UpdateScorePopups();
        UpdateSlots(g_frameDt);
        UpdateExplosions();
        g_playerUpdateFn();
        g_meterShowUntil = g_time + 1000;
        if (g_raceActive == 0) MeteorStormCollide();
        g_frameFunc = RenderMeteorStormRaceFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_METEOR_STORM:  // meteor storm bonus stage
        Hotkeys();
        FRAME_TIME();
        g_meterShowUntil = g_time + 10000;
        UpdateParticles();
        UpdateMeteorStormIntroGate();
        UpdateHyperspace();
        UpdateScorePopups();
        UpdateSlots(g_frameDt);
        UpdateExplosions();
        g_playerUpdateFn();

        // The storm has ended: mark whichever player(s) finished, killing every remaining rock.
        if (g_raceActive == 0) {
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            g_raceDistanceLeft = 1.0f;
            OnMeteorStormLevelEnd();
            if (g_gameMode == MODE_DUAL) {
                if (g_save.players[0].done == 0) {
                    g_save.players[0].done = 1;
                    if (g_save.players[0].doneTime == 0) g_save.players[0].doneTime = g_time + 3000;
                }
                g_save.players[0].killed = g_save.players[0].totalEnemies;
                g_save.players[0].escaped = 0;
            } else {
                if (g_save.players[g_curPlayer].done == 0) {
                    g_save.players[g_curPlayer].done = 1;
                    if (g_save.players[g_curPlayer].doneTime == 0)
                        g_save.players[g_curPlayer].doneTime = g_time + 3000;
                }
                g_save.players[g_curPlayer].killed = g_save.players[g_curPlayer].totalEnemies;
                g_save.players[g_curPlayer].escaped = 0;
            }

            // Clear the field and hand off to normal gameplay for the round-end tally.
            ResetObjectsKeep();
            g_save.players[0].energy = 0;
            g_save.players[1].energy = 0;
            g_save.players[2].energy = 0;
            g_save.players[3].energy = 0;
            BankBonusScore();
            SetHurryUpTimer();
            g_state = STATE_PLAYING;
            g_bonusMeterStarted = false;
            g_meterV = 0.04f;
            g_meterDirUp = false;
        }
        g_frameFunc = RenderMeteorStormResultsFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_SHOP_GATE:  // shop gate / end-of-round rank popup, decides whether to enter the shop or advance
        Hotkeys();
        FRAME_TIME();
        UpdateParticles();
        UpdateHiscoreSkipGate();
        UpdateExplosions();
        UpdateExplosionDebris();
        UpdateSlots(g_frameDt);
        UpdateHyperspace();
        g_playerUpdateFn();

        // Random firework flourish while waiting at the gate.
        if (RandRange(0, 100) < 2 && g_state != STATE_PAUSED) {
            SpawnFirework();
            SoundPlay(g_sfxExplo3, -1, RandRange(30, 100), g_panTable[ClampX(400)], 0x7f, g_sndFlags);
        }

        // Vs mode sends player 2 into the shop first, then advances; other modes go straight through.
        if (g_hiscoreSkipGateActive == 0) {
            if (g_gameMode == MODE_DUAL) {
                if (g_shopCurPlayer == 0) {
                    if (g_save.players[1].money >= 50.0 &&
                        g_save.players[1].lives > g_shipDefs[g_save.players[1].ship]->minEnergy) {
                        cnt = 2;
                        if (g_sfxShop1 != 0) cnt++;
                        if (g_sfxShop2 != 0) cnt++;
                        if (g_sfxShop3 != 0) cnt++;
                        if (g_sfxShop4 != 0) cnt++;
                        if (g_sfxShop5 != 0) cnt++;
                        cnt = RandRange(0, cnt);
                        if (cnt == 0) SoundQueueAdd(g_sfxShop1, 0, 0);
                        if (cnt == 1) SoundQueueAdd(g_sfxShop2, 0, 0);
                        if (cnt == 2) SoundQueueAdd(g_sfxShop3, 0, 0);
                        if (cnt == 3) SoundQueueAdd(g_sfxShop4, 0, 0);
                        if (cnt == 4) SoundQueueAdd(g_sfxShop5, 0, 0);

                        g_shopCurPlayer = 1;
                        CAMERA_RESET();
                        CheckProfileBonus();
                        PlayShopMusic();
                        if (g_save.players[g_shopCurPlayer].autofireUnlocked == 0)
                            g_save.players[g_shopCurPlayer].autofire =
                                g_save.players[g_shopCurPlayer].superAuto;
                        g_shopTransition = 500.0f;
                        g_state = STATE_SHOP;
                        g_buttonsOn = 0;
                        HidePointer();
                    } else {
                        LEVEL_DONE();
                    }
                } else {
                    LEVEL_DONE();
                }
            } else {
                LEVEL_DONE();
            }
        }
        g_frameFunc = RenderShopGateFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_GEM_DROP:  // Gem Drop bonus stage
        ResetPlayerTimers();
        Hotkeys();
        FRAME_TIME();
        UpdateGemDropIntroGate();
        UpdateParticles();
        UpdateScorePopups();
        if (g_gemDropIntroActive == 0) GemDropUpdate();
        UpdateHyperspace();
        g_playerUpdateFn();
        if (g_gemDropIntroActive == 0) {
            if (g_gameMode != MODE_DUAL) GemDropCollide(g_curPlayer);
            if (g_gameMode == MODE_DUAL) {
                r = RandRange(0, 2);
                GemDropCollide(r);
                r = !r;
                GemDropCollide(r);
            }
        }
        g_frameFunc = RenderGemDropFrame;
        CLEAR_DRAW_COUNTERS();
        break;

    case STATE_SHOP:  // in the shop
        g_shopEntryFlag = 1;
        g_stateFn = SetViewHud;
        if (g_gameMode != MODE_DUAL) g_shopCurPlayer = g_curPlayer;
        UpdateParticles();
        UpdateHyperspace();
        g_playerUpdateFn();
        g_frameFunc = Shop;
        CLEAR_DRAW_COUNTERS();
        g_save.players[g_curPlayer].levelFinished = 0;
        g_deathSeqActive = 0;
        break;

    case STATE_HISCORE_TABLE:  // hiscore/tally table
        UpdateGameOverSequence();
        UpdateMenuStars();
        break;

    case STATE_POST_ROUND_IDLE:  // post-round idle timeout (attract-mode countdown on the tally screen)
        PostRoundIdleTimeout();
        UpdateMenuStars();
        break;

    case STATE_ENTER_HISCORE:  // entering a hiscore name
        g_frameFunc = EnterHiscore;
        CLEAR_DRAW_COUNTERS();
        UpdateMenuStars();
        break;

    case STATE_INPUT_CONFIG:  // input-configuration menu
        g_frameFunc = ConfigInputMenu;
        CLEAR_DRAW_COUNTERS();
        UpdateMenuStars();
        break;
    }
}

#undef FRAME_TIME
#undef GAMEPLAY_UPDATE_CORE
#undef CAMERA_RESET
#undef PLAYER_DIED
#undef CONTINUE_BODY
#undef LEVEL_DONE

// Starts a new run: resets per-run counters and timers, randomizes ship choice(s) when
// not resuming, resets difficulty/state handlers, rebuilds the bonus-item drop weight
// table (zeroing most of it for time-trial mode), and picks the item/bullet/player
// update functions for the current game mode before dropping into STATE_RESPAWN (new level).
void NewGame(bool resetLevel)
{
#ifdef __EMSCRIPTEN__
    if (!WebCanPlay()) {
        g_state = STATE_TITLE;
        WebOpenLogin(0);
        return;
    }
#endif
    // ---- reset per-run counters, timers and pause-duration bookkeeping ----
    g_warpMalfunctionCount = 0;
    g_malfunctionTimer = RandRange(-2000, 6000) + 21000;
    g_endWobbleActive = 0;
    g_endWobbleEnabled = 0;
    g_endPic = 1;
    g_playerBroke = 0;
    g_restartNeeded = 0;
    g_warpWhooshPlayed = 0;
    g_speedPctCache = -1;
    g_newGameResetVal = 0;

    if (resetLevel)
        ResetProfileLives(g_profileIndex);
    ClearHiscores();
    g_hiscoreTransitionFlag = 0;
    g_pausedDuration = 0;
    g_pauseEndStamp = 0;
    g_pauseStartStamp = 0;
    g_profilePlayTimeAdded = 0;
    g_playTimeAdded = 0;
    g_enemyFrameCounter = 0;
    g_meterY = 550.0f;

    // Randomize the ship(s) for a fresh run, unless we're just re-entering after closing a window.
    if (!g_newGameOnClose) {
        switch (g_gameMode) {
        case MODE_SINGLE:
            g_save.players[0].ship = RandRange(0, NUM_SHIPS);
            break;
        case MODE_TWO_PLAYER:
            g_save.players[0].ship = RandRange(0, NUM_SHIPS);
            g_save.players[1].ship = RandRange(0, NUM_SHIPS);
            break;
        case MODE_DUAL:
            g_save.players[0].ship = RandRange(0, NUM_SHIPS);
            g_save.players[1].ship = RandRange(0, NUM_SHIPS);
            break;
        case MODE_TEAM:
            break;
        case MODE_UNUSED_4:
            break;
        case MODE_TIME_TRIAL:
            g_save.players[0].ship = RandRange(0, NUM_SHIPS);
            break;
        }
    }

    g_shopItems = 0x53;
    g_superGemDrop = 0;
    g_timeMax = 45;
    g_lastEliteSpawnLevel = 0;
    g_comboLevel = 0;
    g_comboStep = 0;
    g_diffHpBonusA = 0.0f;
    g_diffHpBonusB = 0.0f;
    memset(g_levelRecs, 0, 0x20);  // NOTE: only clears g_levelRecs[0] (sizeof(LevelRec)), not the whole array
    g_armourAddedCount = 10;
    g_livesGainedCount = 20;
    g_deathsCount = 50;
    SysSetMaxFps(60);

    // Sync easy-mode settings from the profile account (lowers difficulty and fps).
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo) {
        g_newGamePending = 1;
        g_itemSteered = 0;
        UnpackAccount(g_profileIndex);
        g_hiscore = g_acc.settings.best;
        g_acc.settings = g_cfg;
        if (g_hiscore > g_acc.settings.best)
            g_acc.settings.best = g_hiscore;
        if (g_acc.easy) {
            g_cfg.difficulty = DIFF_EASY;
            g_hofMode = HOF_EASY;
            g_cfg.fps = FPS_EASY;
            SysSetMaxFps(54);
            DoNothing();
        }
        PackAccount(g_profileIndex);
        SaveAccount(g_profileIndex);
    }

    g_buttonsOn = 0;
    HidePointer();
    g_perfectCount = 0;
    g_killCount = 0;
    g_pendingLevelsPlayed = 0;
    g_sessionScore = 0;
    g_hits = 0;
    g_moneyMax = 0;
    g_blit3Count = 0;

    if (g_resetFlag != 0) {
        EnsureTitleMusic();
        g_resetFlag = 0;
    }
    AudioStop();
    AudioStart();
    WinCloseAll();
    if (g_playerUpdateFn != StateDemo)
        g_cfg.gamesPlayed++;
    StampTimeA();

    // Frame rate follows difficulty: easy runs a bit slower.
    if (g_cfg.difficulty == DIFF_EASY)
        SysSetMaxFps(54);
    if (g_cfg.difficulty == DIFF_NORMAL)
        SysSetMaxFps(60);
    if (g_cfg.difficulty == DIFF_HARD)
        SysSetMaxFps(60);
    if (g_cfg.difficulty == DIFF_ACE)
        SysSetMaxFps(60);
    g_save.players[g_curPlayer].gameSpeedSetting = g_cfg.fps;
    DoNothing();

    // Set up difficulty/state handlers and clear the field for the new run.
    EmptyPostTransitionHook();
    SetDifficulty();
    SetStateByMode();
    g_curPlayer = 0;
    LoadBestScore();
    SetSfxVolume(g_cfg.sfxVol);
    SetMusicVolTable(g_cfg.musicVol);
    ResetAllObjects();
    ClearSlots();
    UpdateBestScore();
    g_getReadyFlagA = 0;
    g_introDone = 0;
    g_levelBannerTime = 0;

    g_save.players[0].done = 0;
    g_save.players[0].doneTime = 0;
    g_save.players[1].done = 0;
    g_save.players[1].doneTime = 0;
    g_save.players[2].done = 0;
    g_save.players[2].doneTime = 0;
    g_save.players[3].done = 0;
    g_save.players[3].doneTime = 0;

    g_timerA = g_time + 2000;
    g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
    g_transitionLock = 1;
    SetHurryUpTimer();
    g_deathSeqActive = 0;

    g_save.players[0].levelFinished = 0;
    g_save.players[0].levelTransitioning = 0;
    g_save.players[0].hyperspaceFade = 0.0f;
    g_save.players[0].scrollSpeedY = 0.0f;
    g_save.players[1].levelFinished = 0;
    g_save.players[1].levelTransitioning = 0;
    g_save.players[1].hyperspaceFade = 0.0f;
    g_save.players[1].scrollSpeedY = 0.0f;
    g_save.players[2].levelFinished = 0;
    g_save.players[2].levelTransitioning = 0;
    g_save.players[2].hyperspaceFade = 0.0f;
    g_save.players[2].scrollSpeedY = 0.0f;
    g_save.players[3].levelFinished = 0;
    g_save.players[3].levelTransitioning = 0;
    g_save.players[3].hyperspaceFade = 0.0f;
    g_save.players[3].scrollSpeedY = 0.0f;

    // Per-pickup-type spawn weight table (index = pickup type), rebuilt fresh each new game.
    g_bonusWeight[ITEM_EXTRA_LIFE] = 10;
    g_bonusWeight[ITEM_MONEY_DOUBLER] = 8;
    g_bonusWeight[ITEM_MIRROR] = 5;
    g_bonusWeight[ITEM_MONEY_BOMB] = 25;
    g_bonusWeight[ITEM_GEM_BOMB] = 35;
    g_bonusWeight[ITEM_METEOR_STORM] = 50;
    g_bonusWeight[ITEM_MEMORY_STATION] = 50;
    g_bonusWeight[ITEM_RANDOM_BONUS] = 111;
    g_bonusWeight[ITEM_WEAPON_SINGLE] = 85;
    g_bonusWeight[ITEM_WEAPON_DOUBLE] = 85;
    g_bonusWeight[ITEM_WEAPON_TRIPLE] = 85;
    g_bonusWeight[ITEM_WARP] = 0;
    g_bonusWeight[ITEM_WEAPON_QUAD] = 20;
    g_bonusWeight[ITEM_ARMOUR] = 25;
    g_bonusWeight[ITEM_SUCKER_BLUE_MONEY] = 35;
    g_bonusWeight[ITEM_SUCKER_GEMS] = 35;
    g_bonusWeight[ITEM_SUCKER_MULTIPLIER] = 35;
    g_bonusWeight[ITEM_EXTRA_TIME] = 15;

    g_bonusWeight[ITEM_MONEY_SMALL] = 300;
    g_bonusWeight[ITEM_MONEY_MEDIUM] = 150;
    g_bonusWeight[ITEM_MONEY_LARGE] = 75;
    g_bonusWeight[ITEM_MONEY_BLUE] = 30;
    g_bonusWeight[ITEM_FREEZE] = 15;
    g_bonusWeight[ITEM_AUTOFIRE] = 60;
    g_bonusWeight[ITEM_LETTER_E] = 45;
    g_bonusWeight[ITEM_LETTER_X] = 45;
    g_bonusWeight[ITEM_LETTER_T] = 45;
    g_bonusWeight[ITEM_LETTER_R] = 45;
    g_bonusWeight[ITEM_LETTER_A] = 45;
    g_bonusWeight[ITEM_SCOOP] = 140;
    g_bonusWeight[ITEM_DRUNK] = 40;
    g_bonusWeight[ITEM_EXTRA_BULLET_SPEED] = 0;

    // Time trial mode: faster, harder, and reroutes to its own difficulty/speed/pickups.
    if (g_gameMode == MODE_TIME_TRIAL) {
        g_savedDifficultyTT = g_cfg.difficulty;
        g_cfgBackup = g_cfg.fps;
        g_cfg.difficulty = DIFF_NORMAL;
        g_cfg.fps = FPS_HARD;
        SetStateByMode();
        EmptyPostTransitionHook();
        SetDifficulty();
        g_gameSpeedMul = 7.0f / 6.0f;
        g_bgIndex = 5;
        // Time trial: disable most pickup types, keep only a couple of time-trial-specific ones.
        g_bonusWeight[ITEM_METEOR_STORM] = 0;
        g_bonusWeight[ITEM_MEMORY_STATION] = 0;
        g_bonusWeight[ITEM_RANDOM_BONUS] = 0;
        g_bonusWeight[ITEM_WEAPON_SINGLE] = 0;
        g_bonusWeight[ITEM_WEAPON_DOUBLE] = 0;
        g_bonusWeight[ITEM_WEAPON_TRIPLE] = 0;
        g_bonusWeight[ITEM_WARP] = 0;
        g_bonusWeight[ITEM_WEAPON_QUAD] = 0;
        g_bonusWeight[ITEM_ARMOUR] = 0;
        g_bonusWeight[ITEM_SUCKER_BLUE_MONEY] = 0;
        g_bonusWeight[ITEM_SUCKER_GEMS] = 0;
        g_bonusWeight[ITEM_SUCKER_MULTIPLIER] = 0;

        g_bonusWeight[ITEM_MONEY_BOMB] = 0;
        g_bonusWeight[ITEM_EXTRA_LIFE] = 0;
        g_bonusWeight[ITEM_EXTRA_TIME] = 0;
        g_bonusWeight[ITEM_MONEY_SMALL] = 0;
        g_bonusWeight[ITEM_MONEY_MEDIUM] = 0;
        g_bonusWeight[ITEM_MONEY_LARGE] = 0;
        g_bonusWeight[ITEM_MONEY_BLUE] = 0;
        g_bonusWeight[ITEM_MONEY_DOUBLER] = 0;
        g_bonusWeight[ITEM_FREEZE] = 0;
        g_bonusWeight[ITEM_AUTOFIRE] = 0;
        g_bonusWeight[ITEM_LETTER_E] = 0;
        g_bonusWeight[ITEM_LETTER_X] = 0;
        g_bonusWeight[ITEM_LETTER_T] = 0;
        g_bonusWeight[ITEM_LETTER_R] = 0;
        g_bonusWeight[ITEM_LETTER_A] = 0;
        g_bonusWeight[ITEM_DRUNK] = 10;
        g_bonusWeight[ITEM_EXTRA_BULLET_SPEED] = 15;
    }

    g_tallyDonePending = 0;
    PlayGameMusic();
    g_levelBannerTime = 0;
    g_msgTimer = 0;
    g_alertTextTime = 0;
    g_msgTime = 0;
    g_bonusResultsTime = 0;
    g_save.players[g_curPlayer].trackKillsFlag = 0;
    g_save.players[g_curPlayer].bonusResultsInitDone = 0;
    g_viewTransitionFlag = 2;
    g_stateFn = SetViewHud;

    // Pick the drawing/update function pointers for this game mode.
    if (g_cfg.bgStars != 0)
        g_fnPtr = DrawStarsPlayer;
    else
        g_fnPtr = DrawSpriteStars;
    g_drawBordersFn = DrawBorders;
    if (g_cfg.bulletIntensity == BULLETS_BRIGHT)
        g_drawLevelObjectsFn = DrawLevelObjectsBright;
    else
        g_drawLevelObjectsFn = DrawLevelObjectsNormal;
    if (g_gameMode == MODE_DUAL) {
        g_itemsVsPlayerFn = ItemsVsBothPlayers;
        g_bulletsVsPlayerFn = BulletsVsBothPlayers;
        g_grabEnemyFn = ShieldGrabEnemiesBothPlayers;
        g_shipHudFn = ShipHudAll;
        g_playerUpdateFn = UpdatePlayers;
    } else if (g_playerUpdateFn != StateDemo) {
        g_itemsVsPlayerFn = ItemsVsPlayer;
        g_bulletsVsPlayerFn = BulletsVsPlayer;
        g_grabEnemyFn = ShieldGrabEnemies;
        g_shipHudFn = ShipHud;
        g_playerUpdateFn = UpdatePlayer;
    }
    ApplyStatUnlocks();

    // Autoplay/demo mode: give the bot a maxed-out loadout.
    if (g_autoplay) {
        g_autoplayCanFire = 1;
        g_save.players[g_curPlayer].lives =
            g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy +
            g_shipDefs[g_save.players[g_curPlayer].ship]->cost * 10;
        g_save.players[g_curPlayer].score = 0;
        g_save.players[g_curPlayer].level = 1;
        g_save.players[g_curPlayer].bullets = 50;
        g_save.players[g_curPlayer].armour =
            g_shipDefs[g_save.players[g_curPlayer].ship]->baseArmour +
            g_shipDefs[g_save.players[g_curPlayer].ship]->maxArmourBonus;
        g_save.players[g_curPlayer].money = 10000;
        g_save.players[g_curPlayer].weapon = RandRange(WEAPON_SINGLE, WEAPON_WAR_PLASMA);
        g_save.players[g_curPlayer].rank = RANK_ENSIGN;
        g_save.players[g_curPlayer].gameSpeedSetting = 200;
    }

    // ---- drop into the new level's respawn state ----
    TimerReset1();
    TimerReset2();
    g_state = STATE_RESPAWN;
    g_playerStallTime = g_time;
    EmptyViewChangeHook();
}

// Level-complete hook; currently a no-op (the original leaves it empty).
void OnLevelComplete()
{
}

// Applies the per-mode setup after g_gameMode is chosen: initializes the player(s),
// starts levels for the two-player modes, and selects the HUD draw function.
void SetStateByMode()
{
    switch (g_gameMode) {
    case MODE_SINGLE:
        g_curPlayer = 0;
        InitPlayer(g_curPlayer);
        g_drawHudFn = DrawHud1P;
        break;

    case MODE_TWO_PLAYER:
        g_curPlayer = 1;
        InitPlayer(g_curPlayer);
        StartLevel();
        g_curPlayer = 0;
        InitPlayer(g_curPlayer);
        StartLevel();
        g_drawHudFn = DrawHud2P;
        break;

    case MODE_DUAL:
        g_curPlayer = 0;
        InitPlayer(0);
        InitPlayer(1);
        g_curPlayer = 0;
        StartLevel();
        g_drawHudFn = DrawHud2PCoop;
        break;

    case MODE_TEAM:
        g_curPlayer = 0;
        InitPlayer(0);
        InitPlayer(1);
        g_drawHudFn = DrawHud1P;
        break;

    case MODE_ACE_TOURNAMENT:
        g_drawHudFn = DrawHud1P;
        break;

    case MODE_TIME_TRIAL:
        g_curPlayer = 0;
        InitPlayer(g_curPlayer);
        g_drawHudFn = DrawHudTimed;
        break;
    }
}
