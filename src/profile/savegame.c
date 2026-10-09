// savegame.c: Suspended games (profileNNN.svg; the format is in savefile.c), profile directories.
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"
#include "game.h"

// Writes the suspended-game save for profile: stamps the save signature/timestamps,
// converts running timers to elapsed-time deltas so they can be resumed later, and writes
// the game state to profile%03d.svg (SaveGameToFile).
void SaveProfile(int profile)
{
    if (profile > -1) {
        char savename[1024];

        sprintf(savename, "%s\\warblade\\profiles\\profile%03d.svg", SysUserFolder(), profile);

        // ---- stamp the save signature and elapsed play time ----
        g_save.sig[0] = 'S';
        g_save.sig[1] = 'D';
        g_save.sig[2] = 'Y';
        StampTimeC();
        g_timeStampA = g_timeA;
        g_timeStampB = g_timeMarkC;
        g_resumeTimeOffset = g_timeStampB - g_timeStampA - g_pausedDuration;
        g_savedDifficultySave = g_cfg.difficulty;

        // ---- convert running timers to elapsed-time deltas so they can resume later ----
        if (g_save.players[g_curPlayer].scoreMult2Timer != 0)
            g_save.players[g_curPlayer].scoreMult2Timer =
                g_save.players[g_curPlayer].scoreMult2Timer - g_time;
        if (g_save.players[g_curPlayer].scoreMult5Timer != 0)
            g_save.players[g_curPlayer].scoreMult5Timer =
                g_save.players[g_curPlayer].scoreMult5Timer - g_time;
        if (g_save.players[g_curPlayer].drunkModeTimer != 0)
            g_save.players[g_curPlayer].drunkModeTimer =
                g_save.players[g_curPlayer].drunkModeTimer - g_time;
        if (g_save.players[g_curPlayer].mirrorTime != 0)
            g_save.players[g_curPlayer].mirrorTime = g_save.players[g_curPlayer].mirrorTime - g_time;
        if (g_save.players[g_curPlayer].shieldTimer != 0)
            g_save.players[g_curPlayer].shieldTimer = g_save.players[g_curPlayer].shieldTimer - g_time;

        // ---- write the game state out ----
        g_saveVersion = g_saveMagic;
        g_save.saveId = MakeRandomId();
        SetProfileLastSaveId(profile, g_save.saveId);
        if (g_playerBroke || g_freshStart)
            DecProfileLives(profile);
        SaveGameToFile(savename);
    }
}

// The browser build's save (not in the original, which saves only with F1/F2 in the shop,
// ending the game): saves profile's suspended game at each shop visit while play goes on. It
// doesn't use up one of the profile's saves, and the game state SaveProfile changes for the file
// (timers made relative) is put back afterwards.
void AutoSaveProfile(int profile)
{
    static SaveData keep;
    unsigned char freshStart = g_freshStart;
    unsigned char broke = g_playerBroke;
    __int64 saveId;

    if (profile < 0)
        return;
    memcpy(&keep, &g_save, sizeof(keep));
    g_freshStart = 0;   // what makes SaveProfile charge a save
    g_playerBroke = 0;
    SaveProfile(profile);
    saveId = g_save.saveId;
    memcpy(&g_save, &keep, sizeof(keep));
    g_save.saveId = saveId;   // the id the profile now records for this save
    g_freshStart = freshStart;
    g_playerBroke = broke;
}

// Loads the suspended-game save for profile (LoadGameFromFile), discards the save file,
// restores each player's ship gfx/hit-mask pointers (dead zones in the file), and
// re-applies any active warp-portal hazard graphics. Converts saved timer deltas back to absolute times, recomputes the
// difficulty/fps setup, and routes into the shop screen to resume play.
// Sets `enemy`'s cached hazard graphics/hit-shot frame from alien-gfx cache slot `n` and
// bumps its LRU age (used when re-applying an active warp-portal hazard's graphics).
#define SET_HAZARD_GFX(enemy, n)                 \
    (enemy).gfxA = g_alienGfxCache[n].gfx1;       \
    (enemy).gfxB = g_alienGfxCache[n].gfx2;       \
    (enemy).shotFrame = g_alienGfxMem[n];         \
    (enemy).shotGfxW = g_hazard##n##GfxW;         \
    (enemy).shotGfxH = g_hazard##n##GfxH;         \
    BumpAlienGfxAge(g_alienGfxCache[n].gfx1);

void LoadSuspended(int profile)
{
    if (profile > -1) {
        char openname[1024];
        int idx;
        int idx2;

        if (ProfileValid(profile)) {
            sprintf(openname, "%s\\warblade\\profiles\\profile%03d.svg",
                          SysUserFolder(), profile);
            if (LoadGameFromFile(openname)) {
                ProfileHistPush(profile, g_save.saveId);
                DeleteProfile(profile);

                // ---- restore ship gfx/hit-mask pointers (not part of the save data) ----
                g_save.players[0].gfx = g_gfxFighter1;
                g_save.players[0].hitMask = g_ship1Hma;
                g_save.players[0].hitMaskParamA = g_ship1GfxParamA;
                g_save.players[0].hitMaskParamB = g_ship1GfxParamB;
                g_save.players[1].gfx = g_gfxFighter2;
                g_save.players[1].hitMask = g_ship2Hma;
                g_save.players[1].hitMaskParamA = g_ship2GfxParamA;
                g_save.players[1].hitMaskParamB = g_ship2GfxParamB;
                ResetObjectsKeep();

                // ---- re-apply any active warp-portal hazard graphics ----
                g_save.players[g_curPlayer].energy = 0;
                if (g_save.players[g_curPlayer].alienLock == 0) {
                    g_save.players[g_curPlayer].shieldL = 0;
                    g_save.players[g_curPlayer].shieldR = 0;
                } else {
                    // ---- left shield's portal ----
                    if (g_warpLevelL != -1 && g_save.players[g_curPlayer].shieldL != 0) {
                        g_curLevelNum = g_warpLevelL;
                        LoadLevelData();
                        idx = g_save.players[g_curPlayer].shieldLIdx;
                        switch (g_enemies[g_curPlayer][idx].hazardType) {
                        case 1:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 0)
                            break;

                        case 2:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 1)
                            break;

                        case 3:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 2)
                            break;

                        case 4:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 3)
                            break;

                        case 5:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 4)
                            break;

                        case 6:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx], 5)
                            break;
                        }
                        g_portalGfxW = HAZARD_GFX_W;
                        g_portalGfxH = HAZARD_GFX_H;
                    }

                    // ---- right shield's portal ----
                    if (g_warpLevelR != -1 && g_save.players[g_curPlayer].shieldR != 0) {
                        g_curLevelNum = g_warpLevelR;
                        LoadLevelData();
                        idx2 = g_save.players[g_curPlayer].shieldRIdx;
                        switch (g_enemies[g_curPlayer][idx2].hazardType) {
                        case 1:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 0)
                            break;

                        case 2:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 1)
                            break;

                        case 3:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 2)
                            break;

                        case 4:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 3)
                            break;

                        case 5:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 4)
                            break;

                        case 6:
                            SET_HAZARD_GFX(g_enemies[g_curPlayer][idx2], 5)
                            break;
                        }
                        g_portalGfxW2 = HAZARD_GFX_W;
                        g_portalGfxH2 = HAZARD_GFX_H;
                    }
                }

                // ---- rebase the elapsed-time clock past the suspended period ----
                g_save.players[g_curPlayer].freezeTimer = 0;
                EmptyPostTransitionHook();
                StampTimeA();
                if (g_resumeTimeOffset < 0)
                    g_resumeTimeOffset = 0;
                g_timeA = g_timeA - g_resumeTimeOffset;

                // ---- convert saved timer deltas back to absolute times ----
                if (g_save.players[g_curPlayer].scoreMult2Timer != 0) {
                    g_save.players[g_curPlayer].scoreMult2Timer =
                        g_save.players[g_curPlayer].scoreMult2Timer + g_time;
                    g_scoreMul[g_curPlayer] = 2;
                }
                if (g_save.players[g_curPlayer].scoreMult5Timer != 0) {
                    g_save.players[g_curPlayer].scoreMult5Timer =
                        g_save.players[g_curPlayer].scoreMult5Timer + g_time;
                    g_scoreMul[g_curPlayer] = 5;
                }
                if (g_save.players[g_curPlayer].drunkModeTimer != 0)
                    g_save.players[g_curPlayer].drunkModeTimer =
                        g_save.players[g_curPlayer].drunkModeTimer + g_time;
                if (g_save.players[g_curPlayer].mirrorTime != 0)
                    g_save.players[g_curPlayer].mirrorTime =
                        g_save.players[g_curPlayer].mirrorTime + g_time;
                if (g_save.players[g_curPlayer].shieldTimer != 0)
                    g_save.players[g_curPlayer].shieldTimer =
                        g_save.players[g_curPlayer].shieldTimer + g_time;

                // ---- recompute the difficulty/fps setup for the restored level ----
                g_bonusResultsTime = 0;
                g_cfg.difficulty = g_savedDifficultySave;
                if (g_cfg.difficulty == DIFF_EASY)
                    g_cfg.fps = 0x32;
                if (g_cfg.difficulty == DIFF_NORMAL)
                    g_cfg.fps = 0x3c;
                if (g_cfg.difficulty == DIFF_HARD)
                    g_cfg.fps = 0x46;
                if (g_cfg.difficulty == DIFF_ACE)
                    g_cfg.fps = 0x50;
                g_save.players[g_curPlayer].gameSpeedSetting = g_cfg.fps;
                SetupDifficulty();
                DoNothing();
                EmptyPostTransitionHook();

                // ---- route into the shop screen to resume play ----
                g_shopCurPlayer = 0;
                g_extraLifeGranted = 0;
                CheckProfileBonus();
                PlayShopMusic();
                if (g_save.players[g_shopCurPlayer].superAuto != 0)
                    g_save.players[g_shopCurPlayer].autofireInterval = 0x19;
                if (g_save.players[g_shopCurPlayer].autofireUnlocked == 0)
                    g_save.players[g_shopCurPlayer].autofire =
                        g_save.players[g_shopCurPlayer].superAuto;
                g_save.players[g_shopCurPlayer].autofireTimer = g_time;
                g_save.players[g_shopCurPlayer].nextShotSnd = g_time;
                if (g_save.players[g_shopCurPlayer].level > 100)
                    g_bonusWeight[36] = 0x3c;
                g_shopTransition = 500.0f;
                g_state = STATE_SHOP;
                g_buttonsOn = 0;
                HidePointer();

                // ---- reset the per-frame draw-call counters ----
                g_blitCount = 0;
                g_blit2Count = 0;
                g_quadCount = 0;
                g_stretchFCount = 0;
                g_stretchRotCount = 0;
                g_stretchRot2Count = 0;
            }
        }
    }
}
#undef SET_HAZARD_GFX

// Checks whether profile's save file exists and is a valid, current-version save
// (SaveFileValid: signature "SDY" and matching g_saveMagic). The game state is not touched.
bool ProfileValid(int profile)
{
    bool ok = false;
    if (profile > -1) {
        char openname[1024];

        sprintf(openname, "%s\\warblade\\profiles\\profile%03d.svg", SysUserFolder(), profile);
        ok = SaveFileValid(openname);
    }
    return ok;
}

// Deletes profile's suspended-game save file.
void DeleteProfile(int profile)
{
    char openname[512];
    if (profile > -1) {
        sprintf(openname, "%s\\warblade\\profiles\\profile%03d.svg", SysUserFolder(), profile);
        remove(openname);
    }
}

// Creates the profiles directory under the user's Warblade folder if it doesn't exist.
void MakeProfilesDir()
{
    char path[512];
    sprintf(path, "%s\\warblade\\profiles", SysUserFolder());
    if (!SysFileExists(path))
        SysMakeDir(path);
}
