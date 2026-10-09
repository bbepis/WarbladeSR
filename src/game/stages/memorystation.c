// memorystation.c: The memory-station bonus stage (card grid).
#include <stdio.h>
#include "globals.h"
#include "game.h"

// Score/money constants used by this stage's card rewards.
enum {
    MEMORY_STATION_BONUS = 25000, // awarded for clearing the whole grid
    SCORE_100 = 100,              // duplicate-letter pick / CARD_SCORE_100
    SCORE_500 = 500,              // non-milestone gem card
    SCORE_1000 = 1000,            // CARD_SCORE_1000
    SCORE_5000 = 5000,            // CARD_MARK_* (LETTER macro)
    SCORE_10000 = 10000,          // CARD_SCORE_10000
    MAXED_STAT_BONUS = 25000,     // consolation score once a capped stat (bullets/speed/buff time) is maxed
    EXTRA_COMPLETE_BONUS = 1000000, // all 5 EXTRA letters collected, lives and armour already maxed
    MONEY_50 = 50,
    MONEY_100 = 100,
    MONEY_200 = 200,
    MONEY_DOUBLER_CAP = 450000,   // money must be under this for the money-doubler card to work
    SECRET_BIRD_MONEY_MAX = 999990, // money set after completing the secret-bird secret
};

// Per-frame gate for the memory-station intro: true (g_memoryIntro set) while
// g_memoryIntroTimer hasn't elapsed yet.
void UpdateMemoryStationIntro()
{
    g_memoryIntro = 0;
    g_introGateScratch = 1;
    if (g_time < g_memoryIntroTimer) {
        g_memoryIntro = 1;
        g_introGateScratch = 0;
    }
}

// Ends the memory-station bonus stage once its deadline passes: awards the pending
// memory bonus (and, every 2 clears, grows the grid), marks the player done and returns
// to gameplay (state 2).
void FinishMemoryStation()
{
    if (g_gameMode == MODE_DUAL)
        g_curPlayer = g_bonusStagePlayer;
    g_memoryDone = 1;
    g_memoryStationExitFlag = 0;
    if (g_time > g_memoryStationDeadline) {
        g_memoryStationDeadline = 0;
        g_memoryDone = 0;
        g_memoryStationExitFlag = 1;

        if (g_memoryBonusPending != 0) {
            g_save.players[g_curPlayer].memoryGridUpgradeStreak++;
            if (g_save.players[g_curPlayer].memoryGridUpgradeStreak == 2) {
                g_save.players[g_curPlayer].memoryGridUpgradeStreak = 0;
                if (g_save.players[g_curPlayer].rows < 8) {
                    g_save.players[g_curPlayer].rows++;
                    g_save.players[g_curPlayer].cols++;
                }
            }

            // Award the memory-station bonus.
            g_save.players[g_curPlayer].memoryBonus += MEMORY_STATION_BONUS;
            ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer,
                              g_save.players[g_curPlayer].memoryBonus);
            g_memoryBonusPending = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            OnMemoryStationLevelEnd();

            // Mark the player(s) as done with this bonus stage.
            if (g_gameMode == MODE_DUAL) {
                g_save.players[0].done = 1;
                if (g_save.players[0].doneTime == 0)
                    g_save.players[0].doneTime = g_time + DONE_DELAY_MS;
                g_save.players[0].killed = g_save.players[0].totalEnemies;
                g_save.players[0].escaped = 0;
            } else {
                if (g_save.players[g_curPlayer].done == 0) {
                    g_save.players[g_curPlayer].done = 1;
                    if (g_save.players[g_curPlayer].doneTime == 0)
                        g_save.players[g_curPlayer].doneTime = g_time + DONE_DELAY_MS;
                }
                g_save.players[g_curPlayer].killed = g_save.players[g_curPlayer].totalEnemies;
                g_save.players[g_curPlayer].escaped = 0;
            }
            ResetObjectsKeep();
            g_save.players[0].energy = 0;
            g_save.players[1].energy = 0;
            g_save.players[2].energy = 0;
            g_save.players[3].energy = 0;
            SetHurryUpTimer();

            // Restore the hyperspace/scroll-speed state saved before entering the bonus stage.
            if (g_gameMode == MODE_DUAL) {
                RESTORE_HYPERSPACE(g_save.players[0], g_save.players[g_curPlayer])
            } else {
                RESTORE_HYPERSPACE(g_save.players[g_curPlayer], g_save.players[g_curPlayer])
            }

            CLEAR_DRAW_COUNTERS()
            g_state = STATE_PLAYING;
            EmptyViewChangeHook();
        }
    }
}

// Draws the memory-station minigame's intro text ("FIND A PAIR...") or, once the player has
// matched every pair, the completion text with the earned bonus, each with its own background
// spark/scanline effect.
void MemoryStationText()
{
    int count;
    int half;
    int i;
    int j;

    count = 50;
    half = (int)g_screenH / 2;
    if (g_memoryIntro != 0) {
        if (g_cfg.particlesOn == 0) {
            for (i = 0; i < count; i++) {
                DrawLine(100.0f, (float)(half - count * 3 / 2 + i * 3),
                                          g_screenW - 100.0, (float)(half - count * 3 / 2 + i * 3),
                                          (i * 3 + 50) / 255.0, 0.0f, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_sparkGfx, 0, (g_screenH >> 1) - 100, 2.0f, 0.0f, 90.0f, 0.0f,
                               0, 0, 0xff, 0, 0x32, 150.0f, 0.0f, -1, RandFloat(0.02f, 0.03f),
                               1, 0, &g_introGateScratch, 0);
            AddParticle(g_sparkGfx, 0, (g_screenH >> 1) + 100, 2.0f, 0.0f, 90.0f, 0.0f,
                               0, 0, 0xff, 0, 0x32, 150.0f, 0.0f, -1, 0 - RandFloat(0.02f, 0.03f),
                               1, 0, &g_introGateScratch, 0);
        }

        DrawMenuText("M E M O R Y S T A T I O N", POS_CENTERED, half - 42, 0);
        DrawMenuText("FIND A PAIR AND COLLECT THE BONUS", POS_CENTERED, half - 18, 1);
        DrawMenuText("BEFORE THE TIME RUNS OUT....", POS_CENTERED, half + 2, 1);
        DrawMenuText("G O O D   L U C K", POS_CENTERED, half + 33, 0);
    }

    if (g_memoryDone != 0) {
        g_buttonsOn = 0;
        HidePointer();
        if (g_cfg.particlesOn == 0) {
            for (j = 0; j < count; j++) {
                DrawLine(100.0f, (float)(half - count * 3 / 2 + j * 3),
                                          g_screenW - 100.0, (float)(half - count * 3 / 2 + j * 3),
                                          (j * 3 + 50) / 255.0, 0.0f, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_sparkGfx, 0, (g_screenH >> 1) - 100, 2.0f, 0.0f, 90.0f, 0.0f,
                               0, 0, 0xff, 0, 0x32, 150.0f, 0.0f, -1, RandFloat(0.02f, 0.03f),
                               1, 0, &g_memoryStationExitFlag, 0);
            AddParticle(g_sparkGfx, 0, (g_screenH >> 1) + 100, 2.0f, 0.0f, 90.0f, 0.0f,
                               0, 0, 0xff, 0, 0x32, 150.0f, 0.0f, -1, 0 - RandFloat(0.02f, 0.03f),
                               1, 0, &g_memoryStationExitFlag, 0);

            AddParticle(g_sparkGfx, 0, (g_screenH >> 1) - 101, 2.0f, 0.0f, 90.0f, 0.0f,
                               0, 0, 100, 0xff, 0x32, 70.0f, 0.0f, -1, 0 - RandFloat(0.01f, 0.02f),
                               1, 0, &g_memoryStationExitFlag, 0);
            AddParticle(g_sparkGfx, 0, (g_screenH >> 1) + 101, 2.0f, 0.0f, 90.0f, 0.0f,
                               0, 0, 100, 0xff, 0x32, 70.0f, 0.0f, -1, RandFloat(0.01f, 0.02f),
                               1, 0, &g_memoryStationExitFlag, 0);
        }

        DrawMenuText("C O N G R A T U L A T I O N S", POS_CENTERED, half - 40, 0);
        DrawMenuText("YOU FOUND ALL THE PAIRS", POS_CENTERED, half - 10, 1);
        sprintf(g_logBuf, "%lld POINTS BONUS", g_save.players[g_curPlayer].memoryBonus);
        DrawMenuText(g_logBuf, POS_CENTERED, half + 10, 1);
    }
}

// Per-frame render for the memory (card-matching) bonus stage: background, sparks,
// the card grid and cursor, particles, score popups, the bonus HUD, borders and HUD.
void RenderMemoryStationFrame()
{
    g_stateFn();
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    if (g_memoryIntro == 0 && g_memoryDone == 0) {
        DrawMemoryGrid();
        DrawMemoryCursor();
    }
    DrawParticles();
    DrawScorePopups();
    UpdateBonusResultsHud();
    MemoryStationText();
    if (g_state == STATE_PAUSED)
        DrawWarpRing();
    DrawFlash();
    g_drawBordersFn();
    g_drawHudFn();
}

// Returns 1 if no open card type (other than CARD_EXTRA_TIME) appears twice among the currently
// active memory-bonus cards, i.e. no unresolved matching pair remains face-up.
int AllTypesUnique()
{
    int result = 1;
    int typer[50];
    for (int i = 0; i < 50; i++) {
        typer[i] = 0;
    }
    for (int r = 0; r < g_save.players[g_curPlayer].rows; r++) {
        for (int c = 0; c < g_save.players[g_curPlayer].cols; c++) {
            if (g_cards[r][c].active != 0 && g_cards[r][c].open != 0) {
                typer[g_cards[r][c].type]++;
            }
        }
    }
    typer[CARD_EXTRA_TIME] = 0;
    for (int k = 0; k < 50; k++) {
        if (typer[k] >= 2) {
            result = 0;
        }
    }
    return result;
}

// Counts remaining matched pairs among the still-active memory-bonus cards (active count
// per type, halved and summed; type 11 excluded).
int CountTypePairs()
{
    int result = 0;
    int typer[50];
    for (int i = 0; i < 50; i++) {
        typer[i] = 0;
    }
    for (int r = 0; r < g_save.players[g_curPlayer].rows; r++) {
        for (int c = 0; c < g_save.players[g_curPlayer].cols; c++) {
            if (g_cards[r][c].active != 0) {
                typer[g_cards[r][c].type]++;
            }
        }
    }
    typer[CARD_EXTRA_TIME] = 0;
    for (int k = 0; k < 50; k++) {
        typer[k] = typer[k] >> 1;
        result = result + typer[k];
    }
    return result;
}

// Helper macros for MemoryBonusUpdate below, undef'd at the end of this section.

#define CUR g_save.players[g_curPlayer]

// Adds val (times the current score multiplier) to the score.
#define AWARD(val) ADD_PLAYER_SCORE(CUR.score, g_curPlayer, val);

// Marks the "all 5 letters collected" secret (number 28) found, once per profile.
#define VOICE28                                                                                 \
    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) { \
        CUR.secretFound28 = 1;                                                                           \
        MarkSecretFound(g_profileIndex, 28);                                                          \
    }

// Bails out of the bonus stage back to the HUD view (used when money is already maxed);
// same shape as the shared RETURN_TO_HUD_VIEW() macro (macros.h).
#define MONEY_FULL RETURN_TO_HUD_VIEW()

// Adds money; if it would exceed the cap, caps it instead and awards the value as score
// (via AWARD) with a score popup, otherwise bails out via MONEY_FULL.
#define MONEY_ADD(val)                                                                     \
    SoundQueueAdd(g_sfxMoney, 50, 0);                                                      \
    SoundPlay(g_sfxCoin, -1, 200, 0.0f, 127, g_sndFlags);                                  \
    CUR.money += val;                                                      \
    if (CUR.money > CUR.moneyMax) {                                                                  \
        CUR.money = CUR.moneyMax;                                                                    \
        AWARD(val)                                                                         \
        AddScorePopup(g_screenW / 2, g_screenH / 2, val, 0); \
        VOICE28                                                                                 \
    } else {                                                                                    \
        MONEY_FULL                                                                              \
    }                                                                                           \
    if (CUR.money > g_moneyMax)                                                                    \
        g_moneyMax = CUR.money;

// Sets one E/X/T/R/A marks bit and awards the base card score.
#define LETTER(bit)                                                                        \
    SoundPlay(g_sfxBell1, RandRange(22000, 32000), 255, g_pan, 127, g_sndFlags);  \
    CUR.marks = (short)CUR.marks | bit;                                                           \
    AWARD(SCORE_5000)

// Grants an armour pickup: adds a ship-defined armour step, shows the "ARMOUR" alert
// message (colored per-player in dual mode) and queues the pickup sound.
#define ARMOUR_ADD                                                                              \
    CUR.armour += STATS->armourStep;                                                                       \
    sprintf(g_alertMsg, "ARMOUR");                                                            \
    if (g_gameMode == MODE_DUAL) {                                                               \
        if (g_curPlayer == 0)                                                            \
            g_msgColor = 1;                                                                       \
        else                                                                                    \
            g_msgColor = 4;                                                                       \
    } else {                                                                                    \
        g_msgColor = 1;                                                                           \
    }                                                                                           \
    g_msgTimer = g_time + 1000;                                                            \
    SoundQueueAdd(g_sfxArmour, 50, 0);                                                      \
    g_armourAddedCount += 5;

// Sets one letter-of-EXTRA flag (awarding a score bonus if it was already set, i.e. a
// duplicate pick), and once all five are set: grants a life if under the cap, else armour
// if under its cap, else a score bonus, then clears all five flags and returns to the HUD.
#define PART(flag)                                                                  \
    if (CUR.flag != 0) {                                                                        \
        AWARD(SCORE_100)                                                                   \
    }                                                                                           \
    CUR.flag = 1;                                                                               \
    if (CUR.extraLetterE != 0 && CUR.extraLetterX != 0 && CUR.extraLetterT != 0 &&              \
        CUR.extraLetterR != 0 && CUR.extraLetterA != 0) {                                       \
        if (CUR.lives < STATS->minEnergy + STATS->maxEnergy) {                                              \
            CUR.lives += STATS->cost;                                                          \
            SoundPlay(g_sfxFanfare, -1, 255, 0.0f, 223, g_sndFlags);                          \
            g_livesGainedCount += 10;                                                                     \
        } else if (CUR.armour < STATS->baseArmour + STATS->maxArmourBonus) {                    \
            ARMOUR_ADD                                                                          \
        } else {                                                                                \
            AWARD(EXTRA_COMPLETE_BONUS)                                                               \
            AddScorePopup(g_screenW / 2, g_screenH / 2, EXTRA_COMPLETE_BONUS, 1); \
        }                                                                                       \
        if (CUR.lives > STATS->minEnergy + STATS->maxEnergy)                                                \
            CUR.lives = STATS->minEnergy + STATS->maxEnergy;                                                \
        CUR.extraLetterE = 0;                                                                           \
        CUR.extraLetterX = 0;                                                                           \
        CUR.extraLetterT = 0;                                                                           \
        CUR.extraLetterR = 0;                                                                           \
        CUR.extraLetterA = 0;                                                                           \
        g_viewTransitionFlag = 2;                                                                           \
        g_stateFn = SetViewHud;                                                                   \
        g_drawBordersFn = DrawBorders;                                                                   \
    }

#define SEL g_cards[g_memSelRow][g_memSelCol]

// If the currently selected card is of type `t`, schedules its pick to resolve after
// `delay` ms and runs the (optional) `dec` statement, e.g. rolling back a try count.
#define PICK(t, delay, dec)                                                                     \
    if (SEL.type == t) {                                                                        \
        g_memPendingCardType = SEL.type;                                                                    \
        g_memPickResolveTime = g_time + delay;                                                       \
        dec                                                                                     \
    }

// Edge-triggered cursor move: `flag` is a latch that starts (or resets to) nonzero once
// `fn` is released; on the frame `fn` first becomes held, the latch clears and, if `cond`
// holds, `op` (the row/col step) runs along with the move sound. Held-down repeats are
// ignored until `fn` is released and pressed again.
#define MOVE(fn, flag, cond, op)                                                                \
    if (fn(g_curPlayer) != 0) {                                                          \
        if (flag != 0) {                                                                        \
            flag = 0;                                                                           \
            if (cond) {                                                                         \
                op;                                                                             \
                SoundPlayNoFade(g_sfxOver, -1, 60, 0.0f, 127, g_sndFlags);                 \
            }                                                                                   \
        }                                                                                       \
    } else {                                                                                    \
        flag = 1;                                                                               \
    }

#define STATS g_shipDefs[CUR.ship]

// Plays the countdown voice sample `snd` and advances to stage `st - 1` once the deadline
// gap `diff` enters (lo, hi) at the current countdown stage `st` (skipped while a rocket
// boost / right-button input is held).
#define COUNTDOWN(lo, hi, st, snd)                                                              \
    if (diff > lo && diff < hi && g_memCountdownStage == st) {                                             \
        if (InputRocket(g_curPlayer) == 0 && g_rightButton == 0)                                \
            SoundQueueAdd(snd, 0, 0);                                                    \
        g_memCountdownStage = st - 1;                                                                      \
    }

// Per-frame update for the memory (card-matching) bonus stage: mouse hover/click on cards,
// the escape-to-quit dialog, the countdown voice cues, rocket-boost time-skips, deadline
// expiry (ends the stage and restores per-player state), resolving a matched pair's reward
// (money, score, extra life/armour, EXTRA letters, gem drop, speed/time buffs, the
// "secret bird" hidden bonus), and card selection/movement input (mouse or keyboard, or
// autoplay's random picks).
void MemoryBonusUpdate()
{
    int savedPlayer;
    int pickFound;
    int mouseMoved;
    int pickRow;
    int pickCol;
    int gemAddIter;
    bool gemMilestoneHit;
    int i;
    int j;
    int diff;
    int bonus;
    int matchCol;
    int matchRow;
    int openCol;
    int openRow;
    int scanCol;
    int scanRow;
    bool anyCardsLeft;
    int apCol;
    int apRow;

    savedPlayer = g_curPlayer;
    pickFound = 0;
    mouseMoved = 0;
    pickRow = -1;
    pickCol = -1;
    if (g_gameMode == MODE_DUAL)
        savedPlayer = 0;
    if (g_gameMode == MODE_DUAL)
        g_curPlayer = g_bonusStagePlayer;

    // NOTE: mouseMoved is initialised to 0 and never set before this check, so the branch
    // below is dead in this build; kept for the byte match.
    if (mouseMoved != 0) {
        g_buttonsOn = 1;
        g_idleFrames = 0;
    } else {
        g_idleFrames++;
        if (g_idleFrames > 200)
            g_buttonsOn = 0;
    }

    // mouse hover/click on cards
    if (g_buttonsOn != 0 && g_mouseClick != 0) {
        for (i = 0; i < CUR.cols; i++) {
            for (j = 0; j < CUR.rows; j++) {
                if (g_mouseX > g_cards[j][i].x && g_mouseX < g_cards[j][i].x + 64 &&
                    g_mouseY > g_cards[j][i].y && g_mouseY < g_cards[j][i].y + 64) {
                    g_memSelRow = j;
                    g_memSelCol = i;
                    if (g_memHoverRow != g_memSelRow || g_memHoverCol != g_memSelCol) {
                        g_memHoverRow = g_memSelRow;
                        g_memHoverCol = g_memSelCol;
                        SoundPlayNoFade(g_sfxOver, -1, 60, 0.0f, 127, g_sndFlags);
                    }
                }
            }
        }
    }

    // escape key: open the quit-game dialog (or reset to title from the demo)
    if (KeyDown(K_VK_ESCAPE) && g_inputCooldown <= 0 && EmptyEscGateCheck() == 0) {
        if (g_playerUpdateFn != StateDemo) {
            WinCloseAll();
            g_profileWinOpen = 0;
            g_buttonsOn = 0;
            HidePointer();
            g_quitGameWinOpen = 1;
            g_buttonsOn = 1;
            PauseGame();
            QuitGameDialog();
            return;
        } else {
            ResetToTitle();
            g_inputCooldown = 100;
            return;
        }
    }

    // countdown voice cues as the deadline approaches
    diff = g_memStageDeadline - g_time;
    COUNTDOWN(10000, 11000, 11, g_sfxVoiceTen)
    COUNTDOWN(9000, 10000, 10, g_sfxVoiceNine)
    COUNTDOWN(8000, 9000, 9, g_sfxVoiceEight)
    COUNTDOWN(7000, 8000, 8, g_sfxVoiceSeven)
    COUNTDOWN(6000, 7000, 7, g_sfxVoiceSix)
    COUNTDOWN(5000, 6000, 6, g_sfxVoiceFive)
    COUNTDOWN(4000, 5000, 5, g_sfxVoiceFour)
    COUNTDOWN(3000, 4000, 4, g_sfxVoiceThree)
    COUNTDOWN(2000, 3000, 3, g_sfxVoiceTwo)
    COUNTDOWN(1000, 2000, 2, g_sfxVoiceOne)
    if (diff > 0 && diff < 1000 && g_memCountdownStage == 1)
        g_memCountdownStage = 11;

    // rocket-boost / right-button input: skip 1000 ms off the deadline for a score bonus
    if ((InputRocket(g_curPlayer) != 0 || g_rightButton != 0) && g_rocketRepeatTimer < g_time) {
        g_rocketRepeatTimer = g_time + 25;
        if (g_memStageDeadline - g_time > 0) {
            g_memStageDeadline -= 1000;
            if (g_memStageDeadline - g_time < 0)
                g_memStageDeadline = g_time;
            bonus = RandRange(1, 11) * 100;
            CUR.score += bonus;
            AddScorePopup(g_screenW / 2 - 150 + RandRange(0, 300),
                                 g_screenH / 2 - 150 + RandRange(0, 300), bonus, 0);
            SoundPlay(g_sfxBell1, RandRange(10000, 41000), 255, g_pan, 127, g_sndFlags);
        }
    }

    // deadline expiry: end the stage and restore per-player state
    if (g_time > g_memStageDeadline) {
        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
        g_transitionLock = 1;
        OnMemoryStationLevelEnd();
        if (g_gameMode == MODE_DUAL) {
            if (g_save.players[0].done == 0) {
                g_save.players[0].done = 1;
                if (g_save.players[0].doneTime == 0)
                    g_save.players[0].doneTime = g_time + DONE_DELAY_MS;
            }
            g_save.players[0].killed = g_save.players[0].totalEnemies;
            g_save.players[0].escaped = 0;
        } else {
            if (CUR.done == 0) {
                CUR.done = 1;
                if (CUR.doneTime == 0)
                    CUR.doneTime = g_time + DONE_DELAY_MS;
            }
            CUR.killed = CUR.totalEnemies;
            CUR.escaped = 0;
        }

        ResetObjectsKeep();
        g_save.players[0].energy = 0;
        g_save.players[1].energy = 0;
        g_save.players[2].energy = 0;
        g_save.players[3].energy = 0;
        SetHurryUpTimer();

        // restore saved hyperspace/scroll state for the resuming player
        if (g_gameMode == MODE_DUAL) {
            RESTORE_HYPERSPACE(g_save.players[savedPlayer], g_save.players[g_bonusStagePlayer])
        } else {
            RESTORE_HYPERSPACE(CUR, CUR)
        }

        g_buttonsOn = 0;
        HidePointer();
        g_state = STATE_PLAYING;
        EmptyViewChangeHook();
    }

    // resolving a matched pair's reward, once its resolve delay has elapsed
    if (g_time > g_memPickResolveTime && g_memPickResolveTime != 0) {
        if (g_memPendingCardType != -1) {
            g_memPickResolveTime = 0;
            for (matchCol = 0; matchCol < CUR.cols; matchCol++) {
                for (matchRow = 0; matchRow < CUR.rows; matchRow++) {
                    if (g_cards[matchRow][matchCol].open == 0 &&
                        g_cards[matchRow][matchCol].type == g_memPendingCardType)
                        g_cards[matchRow][matchCol].active = 0;
                }
            }
            switch (g_memPendingCardType) {
                break;

            case CARD_LOSE_TIME:
                SoundPlay(g_sfxOops, -1, 255, 0.0f, 127, g_sndFlags);
                g_memStageDeadline -= 15000;
                g_memCountdownStage = 11;
                break;

            case CARD_SCORE_X2:
                CUR.scoreMult2Timer = CUR.buffDuration * 1000 + g_time;
                CUR.scoreMult5Timer = 0;
                g_scoreMul[g_curPlayer] = 2;
                SoundQueueAdd(g_sfxTimes2, 50, 0);
                break;

            case CARD_SCORE_X5:
                CUR.scoreMult5Timer = CUR.buffDuration * 1000 + g_time;
                CUR.scoreMult2Timer = 0;
                g_scoreMul[g_curPlayer] = 5;
                SoundQueueAdd(g_sfxTimes5, 50, 0);
                break;

            case CARD_SCORE_100:
                AWARD(SCORE_100)
                break;

            case CARD_SCORE_1000:
                AWARD(SCORE_1000)
                break;

            case CARD_SCORE_10000:
                AWARD(SCORE_10000)
                break;

            case CARD_MONEY_DOUBLER:
                if (CUR.money < MONEY_DOUBLER_CAP) {
                    SoundPlay(g_sfxChaching, -1, 255, 0.0f, 127, g_sndFlags);
                    CUR.money *= 2;
                    if (CUR.money > CUR.moneyMax) {
                        CUR.money = CUR.moneyMax;
                        CUR.score += CUR.moneyMax * 2 * g_scoreMul[g_curPlayer];
                        AddScorePopup(g_screenW / 2, g_screenH / 2, CUR.moneyMax * 2, 0);
                        VOICE28
                    } else {
                        MONEY_FULL
                    }
                    if (CUR.money > g_moneyMax)
                        g_moneyMax = CUR.money;
                } else {
                    SoundPlay(g_sfxChaching, RandRange(12000, 16000), 255, 0.0f, 127, g_sndFlags);
                    sprintf(g_alertMsg, "MONEY DOUBLER MALFUNCTION");
                    g_msgColor = 0;
                    g_msgTimer = g_time + 1000;
                }
                break;

            case CARD_MONEY_50:
                MONEY_ADD(MONEY_50)
                break;

            case CARD_MONEY_100:
                MONEY_ADD(MONEY_100)
                break;

            case CARD_MONEY_200:
                MONEY_ADD(MONEY_200)
                break;

            case CARD_EXTRA_TIME:
                SoundPlay(g_sfxBell1, 40000, 255, 0.0f, 191, g_sndFlags);
                g_memStageDeadline += 10000;
                g_memCountdownStage = 11;
                break;

            case CARD_EXTRA_LIFE:
                if (CUR.lives < STATS->minEnergy + STATS->maxEnergy) {
                    CUR.lives += STATS->cost;
                    SoundQueueAdd(g_sfxExtraLife, 50, 0);
                    g_livesGainedCount += 10;
                } else if (CUR.armour < STATS->baseArmour + STATS->maxArmourBonus) {
                    ARMOUR_ADD
                } else {
                    AWARD(EXTRA_COMPLETE_BONUS)
                    AddScorePopup(g_screenW / 2, g_screenH / 2, EXTRA_COMPLETE_BONUS, 1);
                }
                if (CUR.lives >= STATS->minEnergy + STATS->maxEnergy)
                    CUR.lives = STATS->minEnergy + STATS->maxEnergy;
                break;

            case CARD_MARK_6:
                LETTER(MARK_6)
                break;

            case CARD_MARK_5:
                LETTER(MARK_5)
                break;

            case CARD_MARK_4:
                LETTER(MARK_4)
                break;

            case CARD_MARK_3:
                LETTER(MARK_3)
                break;

            case CARD_MARK_2:
                LETTER(MARK_2)
                break;

            case CARD_MARK_1:
                LETTER(MARK_1)
                break;

            case CARD_EXTRA_BULLET:
                SoundQueueAdd(g_sfxExtraBullet, 50, 0);
                if (CUR.bullets < MAX_BULLETS) {
                    CUR.bullets++;
                } else {
                    AWARD(MAXED_STAT_BONUS)
                    AddScorePopup(g_screenW / 2, g_screenH / 2, MAXED_STAT_BONUS, 0);
                    VOICE28
                }
                break;

            case CARD_GEM_A:
            case CARD_GEM_B:
            case CARD_GEM_C:
            case CARD_GEM_D:
            case CARD_GEM_E:
            case CARD_GEM_F:
            case CARD_GEM_G:
                SoundPlay(g_sfxBell1, RandRange(22000, 32000), 255, g_pan, 127, g_sndFlags);
                gemMilestoneHit = 0;
                for (gemAddIter = 0; gemAddIter < 2; gemAddIter++) {
                    CUR.gems += STATS->gemStep;
                    if ((CUR.gems - STATS->gemBase) / STATS->gemStep % 100 == 0)
                        gemMilestoneHit = 1;
                }

                // every 100th gem is a gem-drop milestone; 1000 of them is the super variant
                if (gemMilestoneHit) {
                    SoundQueueAdd(g_sfxGemDrop, 50, 0);
                    sprintf(g_alertMsg, "G E M   D R O P");
                    if ((CUR.gems - STATS->gemBase) / STATS->gemStep >= 1000) {
                        g_superGemDrop = 1;
                        CUR.gems -= STATS->gemStep * 1000;
                        sprintf(g_alertMsg, "S U P E R   G E M   D R O P");
                        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                            CUR.secretFound30 = 1;
                            MarkSecretFound(g_profileIndex, 30);
                        }
                    }

                    // drop any held shield orbs before entering the Gem Drop level
                    if (CUR.alienLock == 0) {
                        if (CUR.shieldL != 0) {
                            DropAlienGfxAge(g_enemies[g_curPlayer][CUR.shieldLIdx].gfxA);
                            CUR.shieldL = 0;
                            g_enemies[savedPlayer][CUR.shieldLIdx].active = 0;
                            g_enemies[savedPlayer][CUR.shieldLIdx].settled = 0;
                        }
                        if (CUR.shieldR != 0) {
                            DropAlienGfxAge(g_enemies[g_curPlayer][CUR.shieldRIdx].gfxA);
                            CUR.shieldR = 0;
                            g_enemies[savedPlayer][CUR.shieldRIdx].active = 0;
                            g_enemies[savedPlayer][CUR.shieldRIdx].settled = 0;
                        }
                    }

                    // mark the secret and transition into the Gem Drop bonus level
                    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                        CUR.secretFound12 = 1;
                        MarkSecretFound(g_profileIndex, 12);
                    }
                    g_msgColor = 3;
                    g_flashOverlayActive = 1;
                    g_fadeStep = 0;
                    g_fadeColorSet = 1;
                    g_maxFallingGems = 1.0f;
                    InitGemDropLevel();
                    PlayGemDropMusic();
                    g_gemDropIntroTimer = g_time + 4000;
                    g_state = STATE_GEM_DROP;
                    g_buttonsOn = 0;
                    HidePointer();
                    return;
                }
                AWARD(SCORE_500)
                break;

            case CARD_LETTER_E:
                PART(extraLetterE)
                break;

            case CARD_LETTER_X:
                PART(extraLetterX)
                break;

            case CARD_LETTER_T:
                PART(extraLetterT)
                break;

            case CARD_LETTER_R:
                PART(extraLetterR)
                break;

            case CARD_LETTER_A:
                PART(extraLetterA)
                break;

            case CARD_EXTRA_SPEED:
                SoundQueueAdd(g_sfxExtraSpeed, 50, 0);
                CUR.speed += g_speedStep;
                if (CUR.speed > g_speedStep * g_maxSpeedMul + g_speedBase) {
                    CUR.speed = g_speedStep * g_maxSpeedMul + g_speedBase;
                    AWARD(MAXED_STAT_BONUS)
                    AddScorePopup(g_screenW / 2, g_screenH / 2, MAXED_STAT_BONUS, 0);
                    VOICE28
                }
                break;

            case CARD_EXTRA_BUFF_TIME:
                SoundQueueAdd(g_sfxExtraTime, 50, 0);
                if (CUR.buffDuration < g_timeMax) {
                    CUR.buffDuration += 5;
                } else {
                    CUR.buffDuration = g_timeMax;
                    AWARD(MAXED_STAT_BONUS)
                    AddScorePopup(g_screenW / 2, g_screenH / 2, MAXED_STAT_BONUS, 0);
                    VOICE28
                }
                break;

            case CARD_SECRET_BIRD:
                if (CUR.secretBirdHits < 10) {
                    g_secretBirdHitCount++;
                    CUR.secretBirdTick = 3;
                    CUR.secretBirdCounter--;
                    if (CUR.secretBirdCounter < 20)
                        CUR.secretBirdCounter = 20;
                    CUR.secretBirdHits++;
                    if (CUR.secretBirdHits >= 10) {
                        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                            CUR.secretFound23 = 1;
                            MarkSecretFound(g_profileIndex, 23);
                            SetGameCompleted(g_profileIndex);
                        }
                        CUR.secretBirdCounter = 0;
                        CUR.moneyMax = SECRET_BIRD_MONEY_MAX;
                        SoundPlay(g_sfxKanganang, -1, 255, 0.0f, 127, g_sndFlags);
                    } else {
                        SoundPlay(g_sfxGuit, -1, 255, 0.0f, 255, g_sndFlags);
                    }
                }
                break;
            }

            g_memPendingCardType = -1;
            if (AllTypesUnique() != 0) {
                if (g_memoryBonusPending == 0)
                    SoundPlay(g_sfxHarpGliss1, -1, 255, 0.0f, 255, g_sndFlags);
                g_memoryStationDeadline = g_time + 3000;
                g_memoryBonusPending = 1;
                if (g_secretBirdHitCount == 0 && CUR.secretBirdCounter != 0) {
                    CUR.secretBirdTick--;
                    if (CUR.secretBirdTick == 0) {
                        CUR.secretBirdTick = 3;
                        CUR.secretBirdCounter++;
                    }
                }
            }
        } else {
            for (openCol = 0; openCol < CUR.cols; openCol++) {
                for (openRow = 0; openRow < CUR.rows; openRow++) {
                    if (g_cards[openRow][openCol].open == 0)
                        g_cards[openRow][openCol].open = 1;
                }
            }
        }
    }

    // card selection/movement input: mouse or keyboard (or autoplay's random pick)
    if (g_time > g_memPickResolveTime && g_memPendingCardType == -1) {
        g_memPickResolveTime = 0;
        if (InputFire(g_curPlayer) != 0 || g_mouseDown != 0) {
            pickRow = -1;
            pickCol = -1;
            pickFound = 0;
            for (scanCol = 0; scanCol < CUR.cols; scanCol++) {
                for (scanRow = 0; scanRow < CUR.rows; scanRow++) {
                    if (g_cards[scanRow][scanCol].open == 0 && g_cards[scanRow][scanCol].active != 0) {
                        pickFound = 1;
                        pickRow = scanRow;
                        pickCol = scanCol;
                    }
                }
            }

            if (pickFound == 0) {
                if (SEL.active != 0) {
                    SEL.open = 0;
                    SoundPlayNoFade(g_sfxClickGeneric, 40000, 150, 0.0f, 127, g_sndFlags);
                    PICK(38, 450, )
                    PICK(11, 450, )
                    PICK(0, 150, )
                    PICK(1, 350, )
                }
            } else if (SEL.active != 0 && (pickRow != g_memSelRow || pickCol != g_memSelCol)) {
                if (g_autoplay == 0)
                    g_memPickResolveTime = g_time + 450;
                else
                    g_memPickResolveTime = g_time + 5;
                CUR.tries++;
                SEL.open = 0;
                SoundPlayNoFade(g_sfxClickGeneric, 40000, 150, 0.0f, 127, g_sndFlags);
                PICK(38, 450, CUR.tries--;)
                PICK(11, 450, CUR.tries--;)
                PICK(0, 150, CUR.tries--;)
                PICK(1, 350, CUR.tries--;)

                if (SEL.type == g_cards[pickRow][pickCol].type) {
                    g_memPendingCardType = SEL.type;
                    if (g_autoplay == 0)
                        g_memPickResolveTime = g_time + 150;
                    else
                        g_memPickResolveTime = g_time + 5;
                    SoundPlayNoFade(g_sfxFoundIt, -1, 170, 0.0f, 127, g_sndFlags);
                }
            }
        }

        if (g_autoplay == 0) {
            MOVE(InputLeft, g_keyLatch[0], g_memSelRow > 0, g_memSelRow--)
            MOVE(InputRight, g_keyLatch[K_VK_RIGHT], g_memSelRow < CUR.rows - 1, g_memSelRow++)
            MOVE(InputUp, g_keyLatch[K_VK_UP], g_memSelCol > 0, g_memSelCol--)
            MOVE(InputDown, g_keyLatch[K_VK_DOWN], g_memSelCol < CUR.cols - 1, g_memSelCol++)
        }
    }

    // autoplay: pick a random still-open, active card
    if (g_autoplay != 0) {
        anyCardsLeft = false;
        for (apCol = 0; apCol < CUR.cols; apCol++) {
            for (apRow = 0; apRow < CUR.rows; apRow++) {
                if (g_cards[apRow][apCol].active != 0)
                    anyCardsLeft = true;
            }
        }
        if (anyCardsLeft) {
            do {
                g_memSelRow = RandRange(0, CUR.rows);
                g_memSelCol = RandRange(0, CUR.cols);
            } while (SEL.open == 0 || SEL.active == 0);
        }
    }
}

#undef CUR
#undef AWARD
#undef VOICE28
#undef MONEY_FULL
#undef MONEY_ADD
#undef LETTER
#undef ARMOUR_ADD
#undef PART
#undef SEL
#undef PICK
#undef MOVE
#undef STATS
#undef COUNTDOWN

// Builds the card grid for the memory bonus level: lays out and opens every card, then randomly
// assigns weighted card types (retrying the whole assignment until at least one matching pair
// exists on the board), with a small chance to force in the special "secret bird" card. Also resets
// the round's timers, selection state and popups, and hands control to the HUD view.
void InitGrid()
{
    int y;
    int x;
    int total;
    int r;
    int k;
    int i;
    int birdRow;
    int birdCol;
    bool placed;
    int n;
    int secs;
    int j;

    g_rocketRepeatTimer = 0;
    g_gridX = (g_screenW >> 1) - g_save.players[g_curPlayer].rows * 64 / 2;
    g_gridY = (g_screenH >> 1) - g_save.players[g_curPlayer].cols * 64 / 2;

    for (y = 0; y < g_save.players[g_curPlayer].cols; y++) {
        for (x = 0; x < g_save.players[g_curPlayer].rows; x++) {
            g_cards[x][y].active = 1;
            g_cards[x][y].open = 1;
            g_cards[x][y].x = x * 64 + g_gridX;
            g_cards[x][y].y = y * 64 + g_gridY;
            g_cards[x][y].unusedF08 = 0;
            g_cards[x][y].type = 0;
        }
    }

    g_memorySecretBirdSnapshot = g_save.players[g_curPlayer].secretBirdCounter;
    g_secretBirdHitCount = 0;
    total = 0;
    r = 0;
    k = 0;
    for (i = 0; i < 35; i++)
        total = total + g_weights[i];
    birdRow = 0;
    birdCol = 0;
    placed = false;

    do {
        // Assign every card a weighted-random type: pick a bucket [0,total) and walk the weight
        // table to find which type it falls into.
        for (n = 0; n < 2000; n++) {
            birdRow = RandRange(0, g_save.players[g_curPlayer].rows);
            r = RandRange(0, total);
            k = 0;
            while (r > g_weights[k]) {
                r = r - g_weights[k];
                k++;
                birdCol = RandRange(0, g_save.players[g_curPlayer].cols);
            }
            g_cards[RandRange(0, g_save.players[g_curPlayer].rows)]
                         [RandRange(0, g_save.players[g_curPlayer].cols)].type = g_bonusRoundTypePool[k];
        }
        // Small (~0.7%) chance to also drop the special secret bird card, once per attempt.
        if (RandRange(1, 1000) < 7 && !placed) {
            g_cards[birdRow][birdCol].type = 1;
            placed = true;
        }
    } while (AllTypesUnique());

    g_memSelValid = 0;
    g_memSelRow = 0;
    g_memSelCol = 0;
    g_memPendingCardType = -1;
    g_memCursorFrame = 0;
    g_memPickResolveTime = 0;
    g_memoryBonusPending = 0;
    g_memoryDone = 0;
    secs = (int)(g_save.players[g_curPlayer].buffDuration * 1.5);
    if (secs > 300)
        secs = 300;  // cap the round timer at 5 minutes
    g_memStageDeadline = secs * 1000 + g_time;

    EmptyViewChangeHook();
    g_viewTransitionFlag = 2;
    g_stateFn = SetViewHud;
    for (j = 0; j < MAX_POPUPS; j++)
        g_popups[j].active = 0;
}

// Bonus-level-end callback for the memory-station level; no cleanup needed.
void OnMemoryStationLevelEnd()
{
}

// Draws the memory-station card grid: face-up or coloured-back tiles, plus the time
// left, pairs left/tries HUD text, and a countdown "bing" that speeds up as time runs out.
void DrawMemoryGrid()
{
    Rect16 src;
    int j;
    int i;
    int now;
    int secs;
    int flag;

    for (i = 0; i < g_save.players[g_curPlayer].cols; i++) {
        for (j = 0; j < g_save.players[g_curPlayer].rows; j++) {
            if (g_cards[j][i].active != 0) {
                if (g_cards[j][i].open != 0) {
                    src.y1 = 0;
                    src.y2 = src.y1 + 0x40;
                    src.x1 = 0;
                    src.x2 = src.x1 + 0x40;
                } else {
                    src.y1 = g_cards[j][i].type / 4 * 64;
                    src.y2 = src.y1 + 0x40;
                    src.x1 = g_cards[j][i].type % 4 * 64;
                    src.x2 = src.x1 + 0x40;
                }
                if (g_cards[j][i].type != 0 || g_cards[j][i].open != 0) {
                    while (1) {
                        QueueBlit(g_cards[j][i].x, g_cards[j][i].y, g_gfxMemoryBlocks, &src);
                        break;
                    }
                }
            }
        }
    }

    now = g_time;
    secs = (g_memStageDeadline - now) / 1000;
    if (secs < 0)
        secs = 0;
    if (secs > 500)
        secs = 0;
    if (g_gameMode == MODE_DUAL) {
        if (g_curPlayer == 0)
            DrawMenuText("PLAYER ONE", POS_CENTERED, g_gridY - 0x1f, 1);
        if (g_curPlayer == 1)
            DrawMenuText("PLAYER TWO", POS_CENTERED, g_gridY - 0x1f, 4);
    }

    sprintf(g_logBuf, "TIME LEFT :%d", secs);
    if (secs <= 10)
        DrawMenuText(g_logBuf, POS_CENTERED, g_gridY - 0x10, 0);
    else
        DrawMenuText(g_logBuf, POS_CENTERED, g_gridY - 0x10, 1);
    flag = 1;
    sprintf(g_logBuf, "PAIR LEFT :%d   TRIES :%d", CountTypePairs(), g_save.players[g_curPlayer].tries);
    if (g_curPlayer == 0)
        DrawMenuText(g_logBuf, POS_CENTERED, g_gridY + g_save.players[g_curPlayer].cols * 64 + 6, 1);
    if (g_curPlayer == 1)
        DrawMenuText(g_logBuf, POS_CENTERED, g_gridY + g_save.players[g_curPlayer].cols * 64 + 6, 4);
    DrawTinyText("PRESS SECOND FIREBUTTON OR RIGHT MOUSEBUTTON TO KILL TIME", POS_CENTERED,
                 g_gridY + g_save.players[g_curPlayer].cols * 64 + 0x19, 2);

    if (now > g_pairsTickTime) {
        if (secs <= 5)
            SoundPlay(g_sfxBing, -1, 0xff, 0.0f, 0x7f, g_sndFlags);
        if (secs > 5 && secs <= 10)
            SoundPlay(g_sfxBing, -1, 0xaf, 0.0f, 0x7f, g_sndFlags);
        if (secs > 10)
            SoundPlay(g_sfxBing, -1, 0x5a, 0.0f, 0x7f, g_sndFlags);
        g_pairsTickTime = g_time + 1000;
    }
}

// Draws the blinking cursor over the currently selected card in the memory-station
// grid. g_memCursorFrame cycles 0..5 on a timer and picks a 64x64 tile from row 0x140 of
// the memory-blocks sheet; frames 0/1/5 share the left tile and 2/4 share the middle one,
// giving a back-and-forth blink through 3 tiles (left, middle, right, middle, left).
void DrawMemoryCursor()
{
    Rect16 src;

    if (g_memCursorFrame == 0) {
        src.y1 = 0x140;
        src.x1 = 0;
        src.y2 = src.y1 + 0x40;
        src.x2 = src.x1 + 0x40;
    }
    if (g_memCursorFrame == 1) {
        src.y1 = 0x140;
        src.x1 = 0;
        src.y2 = src.y1 + 0x40;
        src.x2 = src.x1 + 0x40;
    }
    if (g_memCursorFrame == 2) {
        src.y1 = 0x140;
        src.x1 = 0x40;
        src.y2 = src.y1 + 0x40;
        src.x2 = src.x1 + 0x40;
    }

    if (g_memCursorFrame == 3) {
        src.y1 = 0x140;
        src.x1 = 0x80;
        src.y2 = src.y1 + 0x40;
        src.x2 = src.x1 + 0x40;
    }
    if (g_memCursorFrame == 4) {
        src.y1 = 0x140;
        src.x1 = 0x40;
        src.y2 = src.y1 + 0x40;
        src.x2 = src.x1 + 0x40;
    }
    if (g_memCursorFrame == 5) {
        src.y1 = 0x140;
        src.x1 = 0;
        src.y2 = src.y1 + 0x40;
        src.x2 = src.x1 + 0x40;
    }

    if (g_time > g_gridAnimTime) {
        g_gridAnimTime = g_time + 0x32;
        g_memCursorFrame = g_memCursorFrame + 1;
        if (g_memCursorFrame > 5)
            g_memCursorFrame = 0;
    }
    while (1) {
        QueueBlit((float)(g_memSelRow * 64 + g_gridX), (float)(g_memSelCol * 64 + g_gridY),
                  g_gfxMemoryBlocks, &src);
        break;
    }
}
