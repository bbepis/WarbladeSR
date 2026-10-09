// hiscore.c: Hiscores: the table file, qualifying, name entry, highlights, the hiscore table
// state, best score.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"
#include "game.h"
#include <zlib.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// warblade_132.his, zlib-packed: the six tables and each table's #1 run's level records, as the
// original kept them in memory (0xa1558 bytes from 0xc39380). The game keeps them in
// g_hiscoreMagic and g_replayRecs; Compress/DecompressHiscores copy them to and from this.
typedef struct HiscoreFile {
    HiscoreData tables;             // +0x0
    char pad_30d4[0x2084];          // +0x30d4
    LevelRec replayRecs[5][4000];   // +0x5158
} HiscoreFile;
C_ASSERT(offsetof(HiscoreFile, replayRecs) == 0x5158);
C_ASSERT(sizeof(HiscoreFile) == 0xa1558);

// The older layout DecompressHiscores tries first: the tables, then 960000 bytes it drops.
typedef struct HiscoreFileOld {
    HiscoreData tables;             // +0x0
    char rest[0xed6d8 - 0x30d4];    // +0x30d4
} HiscoreFileOld;
C_ASSERT(sizeof(HiscoreFileOld) == 0xed6d8);

enum {
    SIZEOF_HISCORES     = sizeof(HiscoreFile),
    SIZEOF_HISCORES_OLD = sizeof(HiscoreFileOld)
};

static HiscoreFile s_hiscoreFile;   // unpacking buffers
static HiscoreFileOld s_hiscoreFileOld;



// Loads the #1 hiscore for the current difficulty (or the time-trial table, in that
// mode) into g_bestScore, for the "beat the best score" HUD comparison.
void LoadBestScore()
{
    DecompressHiscores();
    switch (g_cfg.difficulty) {
    case DIFF_EASY:
        g_bestScore = *(__int64 *)&g_hiscoreMagic.table[0][0].score;
        break;
    case DIFF_NORMAL:
        g_bestScore = *(__int64 *)&g_hiscoreMagic.table1[0].score;
        break;
    case DIFF_HARD:
        g_bestScore = *(__int64 *)&g_hiscoreMagic.table2[0].score;
        break;
    case DIFF_ACE:
        g_bestScore = *(__int64 *)&g_hiscoreMagic.table3[0].score;
        break;
    }
    if (g_gameMode == MODE_TIME_TRIAL)
        g_bestScore = *(__int64 *)&g_hiscoreMagic.table5[0].score;
    ClearHiscores();
}

// Raises g_bestScore to either player's current score if it now exceeds it.
void UpdateBestScore()
{
    if (g_save.players[0].score > g_bestScore)
        g_bestScore = g_save.players[0].score;
    if (g_save.players[1].score > g_bestScore)
        g_bestScore = g_save.players[1].score;
}

// Transitions to the hiscore/tally screen (STATE_HISCORE_TABLE): snapshots the current level's stats
// into g_levelRecs, submits profile stats, resets per-run counters, and starts the
// hiscore-screen music and star field.
void ShowHiscoreTable()
{
    int idx;

#ifdef __EMSCRIPTEN__
    // The browser build saves at every shop (AutoSaveProfile). This table ends a run (game over,
    // retiring, a time trial's time up), and an ended single-player run must not be continued
    // from its last shop, which the original, deleting a save when loading it, never allowed.
    // Quitting a game (back to the title, no table) keeps the save.
    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE)
        DeleteProfile(g_profileIndex);
#endif

    SysSetMaxFps(60);
    g_introInit = 1;
    g_bgIndex = 1;

    // Snapshot this level's run stats into the level-records table.
    idx = g_save.players[g_curPlayer].level > MAX_LEVEL_RECS - 1 ? MAX_LEVEL_RECS - 1 : g_save.players[g_curPlayer].level;
    g_levelRecs[idx].score = g_save.players[g_curPlayer].score;
    g_levelRecs[idx].livesGainedByte = (g_livesGainedCount - 20) / 10;
    g_levelRecs[idx].shots = g_save.players[g_curPlayer].shots;
    g_levelRecs[idx].deathsByte = (g_deathsCount - 50) / 2;
    g_levelRecs[idx].armourAddedByte = (g_armourAddedCount - 10) / 5;
    g_levelRecs[idx].money = g_save.players[g_curPlayer].money;
    g_levelRecs[idx].verify0 = g_save.players[g_curPlayer].weaponAmmoPacked;
    g_levelRecs[idx].rank = g_save.players[g_curPlayer].rank;
    g_levelRecs[idx].verify4 = g_save.players[g_curPlayer].gemPickups;
    g_levelRecs[idx].verify3 = g_save.players[g_curPlayer].collisionsTaken;
    g_levelRecs[idx].verify5 = g_save.players[g_curPlayer].bombPickups;
    g_levelRecs[idx].verify1 = g_save.players[g_curPlayer].pickupCount;
    g_levelRecs[idx].verify2 = g_save.players[g_curPlayer].shopVisits;
    g_levelRecs[idx].verify6 = g_save.players[g_curPlayer].rocketsFired;
    g_levelRecs[idx].frameBucket = g_enemyFrameCounter / 70;
    g_armourAddedCount = 10;
    g_livesGainedCount = 20;

    // Drop any active shield-grab sounds and clear per-run counters.
    if (g_save.players[0].shieldL != 0) {
        DropAlienGfxAge(g_enemies[0][g_save.players[0].shieldLIdx].gfxA);
        g_save.players[0].shieldL = 0;
    }
    if (g_save.players[0].shieldR != 0) {
        DropAlienGfxAge(g_enemies[0][g_save.players[0].shieldRIdx].gfxA);
        g_save.players[0].shieldR = 0;
    }
    g_deathsCount = 50;
    if (g_save.players[1].shieldL != 0) {
        DropAlienGfxAge(g_enemies[1][g_save.players[1].shieldLIdx].gfxA);
        g_save.players[1].shieldL = 0;
    }
    if (g_save.players[1].shieldR != 0) {
        DropAlienGfxAge(g_enemies[1][g_save.players[1].shieldRIdx].gfxA);
        g_save.players[1].shieldR = 0;
    }
    if (g_gameMode == MODE_TIME_TRIAL)
        g_cfg.difficulty = g_savedDifficultyTT;
    g_cfg.fps = 60;
    DoNothing();

    // Reset the hiscore-rank/table scratch arrays for the tally screen.
    g_hsRank2[0] = -1;
    g_hsRank2[1] = -1;
    g_hsRank2[2] = -1;
    g_hsRank2[3] = -1;
    g_hsTable[0] = 100;
    g_hsTable[1] = 100;
    g_hsTable[2] = 100;
    g_hsTable[3] = 100;
    g_hsRank[0] = -1;
    g_hsRank[1] = -1;
    g_hsRank[2] = -1;
    g_hsRank[3] = -1;
    g_hsPlayer[0] = -1;
    g_hsPlayer[1] = -1;
    g_hsPlayer[2] = -1;
    g_hsPlayer[3] = -1;
    g_hsPlayer2[0] = -1;
    g_hsPlayer2[1] = -1;
    g_hsPlayer2[2] = -1;
    g_hsPlayer2[3] = -1;
    g_hsEntryIndex = -1;

    g_playerBroke = 1;
    ResetPlayerTimers();
    AudioStop();
    AudioStart();

    // Bank play time, then submit this run's stats/medals to the profile.
    if (g_playerUpdateFn != StateDemo) {
        StampTimeC();
        g_timeStampA = g_timeA;
        g_timeStampB = g_timeMarkC;
        if (!g_playTimeAdded) {
            g_cfg.playTime += g_timeStampB - g_timeStampA - g_pausedDuration;
            if (g_cfg.playTime < 0)
                g_cfg.playTime = 0;
            g_playTimeAdded = true;
        }
        if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo) {
            if (!g_profilePlayTimeAdded)
                AddPlayTime(g_profileIndex, g_timeStampB, g_timeStampA, g_pausedDuration);

            if (g_gameMode == MODE_SINGLE) {
                UpdateHighScore(g_profileIndex, g_save.players[g_curPlayer].score);
                UpdateMeteorStormScore(g_profileIndex, g_save.players[g_curPlayer].bonusHighScore);
                AddStats(g_profileIndex, g_perfectCount, (&g_perfectCount)[1]);
                CheckRatioMedal(g_profileIndex);
                UpdateBestTime(g_profileIndex, g_timerMin1);
                UpdateFastestMeteorStorm(g_profileIndex, g_timerMin2);
                UpdateHighestLevel(g_profileIndex, g_save.players[g_curPlayer].level);
                AddLevelsPlayed(g_profileIndex, g_pendingLevelsPlayed);
                g_pendingLevelsPlayed = 0;
                if (g_save.players[g_curPlayer].level > 1)
                    IncrementGamesPlayed(g_profileIndex);
                if (g_save.players[g_curPlayer].level > 25)
                    UpdateHitPctAbove25(g_profileIndex,
                        (int)((double)g_save.players[g_curPlayer].hits /
                              g_save.players[g_curPlayer].shots * 100.0));
                UpdateHighestRank(g_profileIndex, g_save.players[g_curPlayer].rank);
            }

            if (g_gameMode == MODE_TIME_TRIAL)
                UpdateTimeTrialScore(g_profileIndex, g_save.players[g_curPlayer].score);
            AddScoreStat(g_profileIndex, g_sessionScore);
            g_sessionScore = 0;
            AddHitsStat(g_profileIndex, g_hits);
            g_hits = 0;
            UpdateHighestMoney(g_profileIndex, g_moneyMax);
        }
    }

    // Set up the tally-screen scroll/star state and start its music.
    g_advanceScreenFlag = 0;
    InitPlayerStats(0, 0);
    InitPlayerStats(1, 1);
    g_screenTimerStart = g_time;
    g_screenDelayMs = 3000;
    g_tallyStep = 1;
    g_hiscoreScrollX = 1.0f;
    g_hiscoreScrollY = 20.0f;
    g_hiscoreEntryScale = 2.0f;
    g_hiscoreStarCount = RandRange(0, 20);
    g_hiscoreStarBrightness1 = RandRange(180, 250);
    g_hiscoreStarBrightness2 = RandRange(180, 250);
    g_hiscoreStarBrightness3 = RandRange(180, 250);
    PlayHiscoreMusic();

    g_menuIdleTimeout = g_time + 120000;
    g_transitionLockUntil = g_time + 500;
    g_transitionLock = 1;
    g_hiscoreEntryReset = 0;
    g_tipIndex = RandRange(0, 19);
    g_state = STATE_HISCORE_TABLE;
    InitStarRotation();
    g_promoRingActive = 0;
    g_hiscoreSkipGateActive = 0;
    EmptyShowHiscoreTableHook();
    g_timeTrialDeadline = -1;
}

// Gate that blocks the hiscore-rank popup from being dismissed early: stays active until
// g_rankLockUntil passes, then shows the rank message and lets InputFire clear it early.
void UpdateHiscoreSkipGate()
{
    g_hiscoreSkipGateActive = 0;
    g_introGateScratch = 1;
    if (g_time < g_rankLockUntil) {
        g_hiscoreSkipGateActive = 1;
        g_introGateScratch = 0;
    }
    if (g_time >= g_rankPopupMinTime) {
        g_rankMsgActive = 1;
        if (InputFire(g_promoPlayer)) {
            g_rankLockUntil = g_time;
            g_rankPopupMinTime = g_time;
            g_hiscoreSkipGateActive = 0;
            g_rankMsgActive = 0;
            g_promoSpecialRank = 0;
            g_promoRingActive = 0;
        }
    }
}

// Checks whether player `p`'s current score (and, in mode 6, bonus score) qualifies for a
// hiscore table slot. Sets g_hsTable/g_hsRank/g_hsPlayer (and the "money" table's g_hsRank2/
// g_hsPlayer2) on a hit. Returns nonzero if any table was qualified for.
// Checks `tbl` (the current-difficulty score table) for a qualifying slot for player `p`'s
// score, records it, snapshots the level-records replay table on a new #1, and returns.
#define CHECK_HISCORE_TABLE(tbl, hof, replayIdx)               \
    for (i = 0; i < MAX_HISCORES; i++) {                       \
        if (g_save.players[p].score >= (tbl)[i].score) {       \
            g_hsTable[p] = (hof);                               \
            g_hsRank[p] = i;                                     \
            g_hsPlayer[p] = p;                                    \
            if (i == 0) {                                         \
                DecompressHiscores();                              \
                for (int j = 0; j < MAX_LEVEL_RECS; j++)           \
                    g_replayRecs[replayIdx][j] = g_levelRecs[j];   \
                CompressHiscores();                                 \
            }                                                       \
            return 1;                                                \
        }                                                             \
    }

int CheckHiscore(int p)
{
    int i;
    int found;

    g_nameLen = 0;
    DecompressHiscores();
    found = 0;

    // "Money" hiscore table (bonus score), checked regardless of game mode.
    if (g_save.players[p].bonusHighScore > 0) {
        for (i = 0; i < MAX_HISCORES; i++) {
            if (g_save.players[p].bonusHighScore >= g_hiscoreMagic.table[4][i].score) {
                g_hsRank2[p] = i;
                g_hsPlayer2[p] = p;
                found = 1;
                break;
            }
        }
    }

    // Time-trial mode has its own hiscore table.
    if (g_gameMode == MODE_TIME_TRIAL) {
        if (g_save.players[p].score > 0) {
            for (i = 0; i < MAX_HISCORES; i++) {
                if (g_save.players[p].score >= g_hiscoreMagic.table5[i].score) {
                    g_hsRank[p] = i;
                    g_hsPlayer[p] = p;
                    if (i == 0) {
                        DecompressHiscores();
                        for (int j = 0; j < MAX_LEVEL_RECS; j++)
                            g_replayRecs[4][j] = g_levelRecs[j];
                        CompressHiscores();
                    }
                    return 1;
                }
            }
        }
        return 0;
    }

    // Otherwise check the score table for the current difficulty.
    if (g_save.players[p].score > 0) {
        switch (g_cfg.difficulty) {
        case DIFF_EASY:
            CHECK_HISCORE_TABLE(g_hiscoreMagic.table[0], HOF_EASY, 0)
            break;

        case DIFF_NORMAL:
            CHECK_HISCORE_TABLE(g_hiscoreMagic.table[1], HOF_NORMAL, 1)
            break;

        case DIFF_HARD:
            CHECK_HISCORE_TABLE(g_hiscoreMagic.table[2], HOF_HARD, 2)
            break;

        case DIFF_ACE:
            CHECK_HISCORE_TABLE(g_hiscoreMagic.table[3], HOF_ACE, 3)
            break;
        }
    }
    return found;
}
#undef CHECK_HISCORE_TABLE

// Hook called while the hiscore table is shown; unused (no-op) in the retail game.
void EmptyShowHiscoreTableHook()
{
}

// Clears the "highlight" (just-entered) flag on every row of the money/bonus hiscore
// table (table4), if player `p` had qualified for a slot in it.
void ClearMoneyHiscoreHighlight(int p)
{
    int i;

    if (g_hsRank2[p] != -1) {
        for (i = 0; i < MAX_HISCORES; i++)
            g_hiscoreMagic.table4[i].highlight = 0;
    }
}

// Clears the "highlight" flag on every row of the time-trial hiscore table (table5),
// if player `p` had qualified for a slot in it.
void ClearHiscoreHighlightTT(int p)
{
    int i;

    if (g_hsRank[p] != -1) {
        for (i = 0; i < MAX_HISCORES; i++)
            g_hiscoreMagic.table5[i].highlight = 0;
    }
}

// Clears the "highlight" flag on every row of the difficulty-specific hiscore table
// (table[0..3]) that player `p` qualified for, per g_hsTable[p].
void ClearHiscoreHighlight(int p)
{
    int i;

    if (g_hsTable[p] != -1) {
        switch (g_hsTable[p]) {
        case HOF_EASY:
            for (i = 0; i < MAX_HISCORES; i++)
                g_hiscoreMagic.table[0][i].highlight = 0;
            break;
        case HOF_NORMAL:
            for (i = 0; i < MAX_HISCORES; i++)
                g_hiscoreMagic.table1[i].highlight = 0;
            break;
        case HOF_HARD:
            for (i = 0; i < MAX_HISCORES; i++)
                g_hiscoreMagic.table2[i].highlight = 0;
            break;
        case HOF_ACE:
            for (i = 0; i < MAX_HISCORES; i++)
                g_hiscoreMagic.table3[i].highlight = 0;
            break;
        }
    }
}

// Insert the entered name into hiscore table `tbl` at slot `slot`, taking the stats of player `pl`.
#define HISCORE_INSERT(tbl, slot, pl, scoreField)                                   \
    for (j = MAX_HISCORES - 2; j > (slot) - 1; j--)                                 \
        tbl[j + 1] = tbl[j];                                                        \
    for (k = 0; k < NAME_LEN; k++) {                                                \
        c = g_name[k];                                                       \
        if (c == '_')                                                               \
            c = ' ';                                                                \
        tbl[slot].name[k] = c;                                                      \
    }                                                                               \
    tbl[slot].score = g_save.players[pl].scoreField;                      \
    tbl[slot].level = g_save.players[pl].displayLevel;                                \
    tbl[slot].rank = g_save.players[pl].rank;                                \
    tbl[slot].power = g_save.players[pl].buffDuration;                               \
    tbl[slot].duration = g_timeStampB - g_timeStampA - g_pausedDuration;                                    \
    tbl[slot].hits = g_save.players[pl].hits;                                \
    tbl[slot].shots = g_save.players[pl].shots;                                \
    tbl[slot].highlight = 1;                                                          \
    tbl[slot].ownerStamp = g_save.players[pl].sessionPlayTime;                               \
    memset(&tbl[slot].date, 0, 16);                                              \
    SysUtcDate(&tbl[slot].date);

// Inserts the entered name into difficulty table `tbl` for the current-loop player `i`
// (HOF mode `hof`), if that player is the one currently entering their name.
#define ENTER_HISCORE_TABLE(tbl, hof)                          \
    g_hofMode = (hof);                                          \
    if (g_hsPlayer[i] == g_hsEntryIndex) {                       \
        HISCORE_INSERT(tbl, g_hsRank[i], g_hsPlayer[i], score)   \
        g_hsTable[i] = 100;                                       \
    }

// Per-frame driver for the "NEW HISCORE" name entry screen: reads keyboard input into
// g_name (or auto-fills it from the player's profile name), and on RETURN/auto-confirm
// inserts the name into every hiscore table the player qualified for (HISCORE_INSERT),
// writes the hiscore file, then either re-enters this screen for the next qualifying
// player or returns to the title screen.
void EnterHiscore()
{
    char cursortxt[256];
    int j;
    int k;
    int key;
    int c;

    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo) {
        GetProfileName(g_profileIndex);
        for (int i = 0; i < NAME_LEN; i++)
            g_name[i] = g_logBuf[i];
        g_name[NAME_LEN] = 0;
        g_autoConfirmName = 1;
        g_inputCooldown = 0;
    } else {
        if (RandRange(0, 100) < 2 && g_state != STATE_PAUSED) {
            SpawnFirework();
            SoundPlay(g_sfxExplo3, -1, RandRange(30, 100), g_panTable[ClampX(400)], 127, g_sndFlags);
        }
    }

    if (KeyDown(K_VK_BACK) == true && g_inputCooldown < 1) {
        if (g_keyLatch[K_VK_BACK] != 0) {
            if (g_nameLen > 0) {
                g_nameLen--;
                g_name[g_nameLen] = '_';
                PlayClick();
                g_inputCooldown = 8;
            }
            g_keyLatch[K_VK_BACK] = 0;
        }
    } else {
        g_keyLatch[K_VK_BACK] = 1;
    }

    key = 0;
    GetPressedKeyName();
    if (g_pressedKey != -1) {
        if (g_pressedKey == 4 && g_nameLen == 0) {
        } else if (g_keyLatch[g_pressedKey] != 0) {
            key = KeyToChar(g_pressedKey);
            g_keyLatch[g_pressedKey] = 0;
            PlayClick();
        }
    } else {
        for (int i = 0; i < 256; i++)
            g_keyLatch[i] = 1;
    }

    if ((KeyDown(K_VK_RETURN) == true && g_inputCooldown < 1) || g_autoConfirmName) {
        g_hsEntryIndex++;
        g_inputCooldown = 100;
        g_autoConfirmName = 0;
        PlayClick();
        g_lastActivityTime = g_time;
        g_attractScreen = ATTRACT_HALL_OF_FAME;
        g_resetFlag = 1;
        g_idleTimeoutMs = 25000;
        g_hofMode = g_cfg.difficulty;
        if (g_gameMode == MODE_TIME_TRIAL)
            g_hofMode = HOF_TIME_TRIAL;
        g_transitionLockUntil = g_time + 500;
        g_transitionLock = 1;

        if (g_hiscoreInsertGate == 0) {
            DecompressHiscores();

            for (int i = 0; i < 4; i++) {
                if (g_hsRank2[i] != -1) {
                    g_hofMode = HOF_METEORSTORM;
                    if (g_hsPlayer2[i] == g_hsEntryIndex) {
                        HISCORE_INSERT(g_hiscoreMagic.table4, g_hsRank2[i], g_hsPlayer2[i], bonusHighScore)
                        g_hsRank2[i] = -1;
                    }
                }
            }

            if (g_gameMode == MODE_TIME_TRIAL) {
                g_hofMode = HOF_TIME_TRIAL;
                for (int i = 0; i < 4; i++) {
                    if (g_hsRank[i] != -1) {
                        HISCORE_INSERT(g_hiscoreMagic.table5, g_hsRank[i], g_hsPlayer[i], score)
                        g_hsRank[i] = -1;
                    }
                }
            } else {
                for (int i = 0; i < 4; i++) {
                    switch (g_hsTable[i]) {
                    case DIFF_EASY:
                        ENTER_HISCORE_TABLE(g_hiscoreMagic.table[0], HOF_EASY)
                        break;

                    case DIFF_NORMAL:
                        ENTER_HISCORE_TABLE(g_hiscoreMagic.table1, HOF_NORMAL)
                        break;

                    case DIFF_HARD:
                        ENTER_HISCORE_TABLE(g_hiscoreMagic.table2, HOF_HARD)
                        break;

                    case DIFF_ACE:
                        ENTER_HISCORE_TABLE(g_hiscoreMagic.table3, HOF_ACE)
                        break;
                    }
                }
            }

            CompressHiscores();
            WriteHiscoreFile();

            switch (g_gameMode) {
            case MODE_SINGLE:
                g_curPlayer = -1;
                break;

            case MODE_TWO_PLAYER:
                if (g_curPlayer == 0)
                    g_curPlayer = 1;
                else
                    g_curPlayer = -1;
                break;

            case MODE_DUAL:
                if (g_curPlayer == 0)
                    g_curPlayer = 1;
                else
                    g_curPlayer = -1;
                break;

            case MODE_TEAM:
                if (g_curPlayer == 0)
                    g_curPlayer = 1;
                else
                    g_curPlayer = -1;
                break;

            case MODE_UNUSED_4:
                g_curPlayer = -1;
                break;

            case MODE_ACE_TOURNAMENT:
                g_curPlayer = -1;
                break;

            case MODE_TIME_TRIAL:
                g_curPlayer = -1;
                break;
            }

            if (g_curPlayer != -1 && CheckHiscore(g_curPlayer) != 0) {
                for (int i = 0; i < NAME_LEN; i++)
                    g_name[i] = '_';
                g_name[NAME_LEN] = 0;
                g_nameLen = 0;
                cursortxt[g_nameLen] = '#';
                ClearHiscores();
                key = 0;
                g_inputCooldown = 100;
                g_state = STATE_ENTER_HISCORE;
                g_transitionLockUntil = g_time + 500;
                g_transitionLock = 1;

                if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
                    g_playerUpdateFn != StateDemo) {
                    if (!g_profilePlayTimeAdded)
                        AddPlayTime(g_profileIndex, g_timeStampB, g_timeStampA, g_pausedDuration);
                    if (g_gameMode == MODE_SINGLE)
                        UpdateHighScore(g_profileIndex, g_save.players[g_curPlayer].score);
                    UpdateMeteorStormScore(g_profileIndex, g_save.players[g_curPlayer].bonusHighScore);
                    if (g_gameMode == MODE_TIME_TRIAL)
                        UpdateTimeTrialScore(g_profileIndex, g_save.players[g_curPlayer].score);
                    AddStats(g_profileIndex, g_perfectCount, (&g_perfectCount)[1]);
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
                        if (g_save.players[g_curPlayer].level > 1)
                            IncrementGamesPlayed(g_profileIndex);
                        if (g_save.players[g_curPlayer].level > 25)
                            UpdateHitPctAbove25(g_profileIndex,
                                (int)((double)g_save.players[g_curPlayer].hits /
                                    g_save.players[g_curPlayer].shots * 100.0));
                        UpdateHighestRank(g_profileIndex, g_save.players[g_curPlayer].rank);
                    }

                    UpdateHighestMoney(g_profileIndex, g_moneyMax);
                }

            } else {
                key = 0;
                g_inputCooldown = 100;
                g_menuIdleTimeout = g_time + 120000;
                g_inputCooldown = 100;
                InitStarRotation();
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HALL_OF_FAME;
                g_idleTimeoutMs = 60000;
                g_resetFlag = 1;
                g_transitionLockUntil = g_time + 500;
                g_transitionLock = 1;
                g_state = STATE_TITLE;
                ClearPlayers();
                g_hiscoreEntryReset = 0;
            }
        }
    }

    if (KeyDown(K_VK_ESCAPE) == true && g_inputCooldown <= 0 && EmptyEscGateCheck() == 0) {
        ResetToTitle();
        g_inputCooldown = 100;
    } else {
        DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
        DrawBackground();
        g_fnPtr();
        if (g_flag)
            UpdateSparks();
        DrawMenuText("****                       ****", POS_CENTERED, 0xe6, 6);
        DrawMenuText("     N E W   H I S C O R E     ", POS_CENTERED, g_curY, 0);
        g_textCursorY = g_textCursorY + 0x14;
        if (g_curPlayer == 0)
            DrawMenuText("ENTER YOUR NAME PLAYER ONE", POS_CENTERED, g_textAutoY, 1);
        if (g_curPlayer == 1)
            DrawMenuText("ENTER YOUR NAME PLAYER TWO", POS_CENTERED, g_textAutoY, 1);
        if (g_curPlayer == 2)
            DrawMenuText("ENTER YOUR NAME PLAYER THREE", POS_CENTERED, g_textAutoY, 1);
        if (g_curPlayer == 3)
            DrawMenuText("ENTER YOUR NAME PLAYER FOUR", POS_CENTERED, g_textAutoY, 1);
        g_textCursorY = g_textCursorY + 0x14;
        DrawMenuText(g_name, POS_CENTERED, g_textAutoY, 5);

        if (key != 0 && g_nameLen < NAME_LEN - 1) {
            g_name[g_nameLen] = key;
            g_nameLen++;
        }

        for (k = 0; k < NAME_LEN; k++)
            cursortxt[k] = ' ';
        cursortxt[NAME_LEN] = 0;
        cursortxt[g_nameLen] = '#';
        if (g_cursorBlinkOn != 0)
            DrawMenuText(cursortxt, POS_CENTERED, g_curY, 0);
        if (g_time - g_cursorBlinkTime > g_cursorBlinkRate) {
            g_cursorBlinkTime = g_time;
            g_cursorBlinkOn = g_cursorBlinkOn == 0;
        }
        if (g_time - g_uiBlinkTime > g_blinkRate) {
            g_uiBlinkTime = g_time;
            g_uiBlink = g_uiBlink == 0;
        }
    }
}

#undef HISCORE_INSERT
#undef ENTER_HISCORE_TABLE

// zlib-compresses the in-memory hiscore tables (g_hiscoreMagic, g_replayRecs) into
// g_hiscoreBuf, then zeroes them (ClearHiscores) so they must be decompressed again before use.
void CompressHiscores()
{
    uLongf packed = SIZEOF_HISCORES;

    memset(&s_hiscoreFile, 0, sizeof(s_hiscoreFile));
    s_hiscoreFile.tables = g_hiscoreMagic;
    memcpy(s_hiscoreFile.replayRecs, g_replayRecs, sizeof(s_hiscoreFile.replayRecs));
    g_hiscoreSize = SIZEOF_HISCORES;
    compress((unsigned char *)g_hiscoreBuf, &packed, (const unsigned char *)&s_hiscoreFile,
        SIZEOF_HISCORES);
    g_hiscorePackedSize = (int)packed;
    ClearHiscores();
}

// Zeroes the in-memory hiscore tables (g_hiscoreMagic, g_replayRecs) and marks them as
// cleared/stale.
void ClearHiscores()
{
    memset(&g_hiscoreMagic, 0, sizeof(g_hiscoreMagic));
    memset(g_replayRecs, 0, sizeof(g_replayRecs));
    g_hiscoresCleared = 1;
}

// Decompresses the hiscore tables into g_hiscoreMagic (and g_replayRecs) for use. Tries the
// newer layout (g_hiscorePackedOld -> HiscoreFileOld) first and copies its tables/tag/count
// across field by field; falls back to the older packed buffer (g_hiscoreBuf) on failure, and
// clears everything if that also fails.
void DecompressHiscores()
{
    int res;
    int i;
    int res2;
    uLongf size = SIZEOF_HISCORES_OLD;
    const HiscoreData *old = &s_hiscoreFileOld.tables;

    g_hiscorePackedSize = SIZEOF_HISCORES_OLD;
    res = uncompress((unsigned char *)&s_hiscoreFileOld, &size,
        (const unsigned char *)g_hiscorePackedOld, g_hiscorePackedSize);
    g_hiscoreSize = (int)size;
    if (res == 0)
    {
        memset(&g_hiscoreMagic, 0, sizeof(g_hiscoreMagic));
        memset(g_replayRecs, 0, sizeof(g_replayRecs));
        for (i = 0; i < MAX_HISCORES; i++)
        {
            g_hiscoreMagic.table[0][i] = old->table[0][i];
            g_hiscoreMagic.table[1][i] = old->table[1][i];
            g_hiscoreMagic.table[2][i] = old->table[2][i];
            g_hiscoreMagic.table[3][i] = old->table[3][i];
            g_hiscoreMagic.table5[i] = old->table5[i];
            g_hiscoreMagic.table[4][i] = old->table[4][i];
        }

        g_hiscoreMagic.tag[0] = old->tag[0];
        g_hiscoreMagic.tag[1] = old->tag[1];
        g_hiscoreMagic.tag[2] = old->tag[2];
        g_hiscoreMagic.tag[3] = old->tag[3];
        g_hiscoreMagic.tag[4] = 0;
        g_hiscoreMagic.count = old->count;
    }
    else
    {
        size = SIZEOF_HISCORES;
        g_hiscorePackedSize = SIZEOF_HISCORES;
        memset(&s_hiscoreFile, 0, sizeof(s_hiscoreFile));
        res2 = uncompress((unsigned char *)&s_hiscoreFile, &size,
            (const unsigned char *)g_hiscoreBuf, g_hiscorePackedSize);
        g_hiscoreSize = (int)size;
        if (res2 != 0) {
            ClearHiscores();
        } else {
            // (a short file, such as the 0x30c8-byte one LoadHiscores accepts, leaves the rest zero)
            g_hiscoreMagic = s_hiscoreFile.tables;
            memcpy(g_replayRecs, s_hiscoreFile.replayRecs, sizeof(g_replayRecs));
        }
    }
}

// Stamps the "WARX" tag onto the hiscore tables, compresses them, and writes them to
// warblade_132.his in the user's data folder.
void WriteHiscoreFile()
{
    int fd;
    int res;

    fd = 0;
    char nnn[5] = "WARX";
    char path[512];
    DecompressHiscores();
    g_hiscoreMagic.count = 0;
    g_hiscoreMagic.tag[0] = nnn[0];
    g_hiscoreMagic.tag[1] = nnn[1];
    g_hiscoreMagic.tag[2] = nnn[2];
    g_hiscoreMagic.tag[3] = nnn[3];
    g_hiscoreMagic.tag[4] = 0;
    CompressHiscores();

    _set_fmode(_O_BINARY);
    sprintf(path, "%s\\warblade\\warblade_132.his", SysUserFolder());
    fd = _open(path, _O_CREAT | _O_TRUNC | _O_RDWR, _S_IREAD | _S_IWRITE);
    if (fd != -1)
    {
        res = _write(fd, g_hiscoreBuf, g_hiscorePackedSize);
        if (res == -1)
        {
            g_fileWriteErrorFlag = 1;
            SysMessageBox("Warblade SR 2.0, Copyright 1999-2009 Edgar M Vigdal",
                          "Could not open/create WarBlade Hiscore file");
            g_fileWriteErrorFlag = 0;
        }
        _close(fd);
#ifdef __EMSCRIPTEN__
        // Hand the new candidate to the page immediately; it is merged by the server.
        EM_ASM({ if (Module.onGameScoreWritten) Module.onGameScoreWritten(); });
#endif
    }
}

// Clears one hiscore-table entry to a blank row (name/level/rank/power/hits/shots/highlight/
// duration/year/ownerStamp).
#define RESET_HISCORE_TABLE(entry) \
    sprintf((entry).name, " ");    \
    (entry).score = 0;             \
    (entry).level = 0;             \
    (entry).rank = 0;              \
    (entry).power = 0;             \
    (entry).hits = 0;              \
    (entry).shots = 0;             \
    (entry).highlight = 0;         \
    (entry).duration = 0;          \
    (entry).year = 0;              \
    (entry).ownerStamp = 0;

// table5 (time trial) has no level/rank/power fields.
#define RESET_HISCORE_TABLE5(entry) \
    sprintf((entry).name, " ");     \
    (entry).score = 0;              \
    (entry).hits = 0;               \
    (entry).shots = 0;              \
    (entry).highlight = 0;          \
    (entry).duration = 0;           \
    (entry).year = 0;               \
    (entry).ownerStamp = 0;

// Clears every row of all six hiscore tables (table[0..4] and table5) to blank/zero entries
// and resets the compressed-buffer copy.
void ResetHiscores()
{
    int i;
    for (i = 0; i < MAX_HISCORES; i++) {
        RESET_HISCORE_TABLE(g_hiscoreMagic.table[0][i])

        RESET_HISCORE_TABLE(g_hiscoreMagic.table[1][i])

        RESET_HISCORE_TABLE(g_hiscoreMagic.table[2][i])

        RESET_HISCORE_TABLE(g_hiscoreMagic.table[3][i])

        RESET_HISCORE_TABLE(g_hiscoreMagic.table[4][i])

        RESET_HISCORE_TABLE5(g_hiscoreMagic.table5[i])
    }
    g_hiscoreMagic.count = 0;
    CompressHiscores();
}
#undef RESET_HISCORE_TABLE
#undef RESET_HISCORE_TABLE5

// Loads warblade_132.his from the user's data folder into g_hiscoreBuf and decompresses it.
// Falls back to a fresh table set (ResetHiscores) if the file is missing, an unexpected size,
// or missing the "WARX" tag; a 0x30c8-sized decompression (older/short format) is accepted
// as-is with just the entry count reset.
void LoadHiscores()
{
    int fd = 0;
    int n;
    char nnn[5] = "WARX";
    char path[512];

    _set_fmode(_O_BINARY);
    sprintf(path, "%s\\warblade\\warblade_132.his", SysUserFolder());
    fd = _open(path, 0, 0);
    if (fd != -1) {
        n = _read(fd, g_hiscoreBuf, SIZEOF_HISCORES);
        _close(fd);
        DecompressHiscores();
        if (g_hiscoreSize == 0x30c8) {
            g_hiscoreMagic.count = 0;
        } else if (g_hiscoreSize != SIZEOF_HISCORES) {
            ResetHiscores();
            g_hiscoreMagic.count = 0;
        } else if (g_hiscoreMagic.tag[0] == nnn[0] && g_hiscoreMagic.tag[1] == nnn[1] &&
                   g_hiscoreMagic.tag[2] == nnn[2] && g_hiscoreMagic.tag[3] == nnn[3]) {
        } else {
            ResetHiscores();
            g_hiscoreMagic.count = 0;
        }
        ClearHiscores();
    } else {
        g_hiscoreMagic.count = 0;
        ResetHiscores();
    }
}
