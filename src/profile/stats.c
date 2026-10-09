// stats.c: Profile statistics, ranks and medals, secrets, the profile window, stat unlocks.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// "no medal-sequence progress yet" sentinel for Account::medalStep.
enum {
    MEDAL_STEP_RESET      = -1,
    MEDAL_RATIO_MIN_PLAYED = 500,  // CheckRatioMedal thresholds
    MEDAL_RATIO_MIN_PCT    = 75
};
// Baseline bonus-level count perfectBonusLevels/bonusLevelsPlayed are rescaled to.
#define RATIO_RESCALE_BASE 450.0
// Sentinel for "no best time recorded" on FormatBestTime/UpdateBestTime's larger (ms-scale)
// time value; unrelated to (and a different magnitude from) NO_TIME_RECORDED.
#define NO_BEST_TIME 9999999999

// Money cost of unlocking the next planet rank.
enum { PLANET_RANK_MONEY_COST = 500000 };

// Score bonus for a "planet rank available" milestone or for finding every secret in one game.
static const __int64 MILESTONE_SCORE_BONUS = 250000000;

// Money the profile's money is capped/reset to on a full-completion milestone; also the
// "exact money" medal target.
enum { MAX_MONEY = 999990 };

// Money reward amounts for the stat-threshold unlocks in ApplyStatUnlocks().
enum {
    MONEY_REWARD_500   = 500,
    MONEY_REWARD_1000  = 1000,
    MONEY_REWARD_2000  = 2000,
    MONEY_REWARD_5000  = 5000,
    MONEY_REWARD_25000 = 25000,
};


// Formats the profile's all-time game high score into g_logBuf (and g_shownStat).
void ShowHighScore(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_shownStat = g_acc.highScore;
        Int64ToStrGrouped(g_acc.highScore, g_logBuf);
        ClearAccount();
    }
}

// Raises the profile's high score to v if v is higher, then saves the account.
void UpdateHighScore(int profile, __int64 v)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if (v > g_acc.highScore)
            g_acc.highScore = v;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Formats the level-100 (marathon) high score into g_logBuf/g_shownStat, and flags the
// current player's highScoreMilestone once it is at least 200,000,000.
void ShowMarathonScore(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_shownStat = g_acc.level100HighScore;
        Int64ToStrGrouped(g_acc.level100HighScore, g_logBuf);
        ClearAccount();
        if (g_gameMode != MODE_TIME_TRIAL && g_shownStat >= MARATHON_MILESTONE_SCORE)
            g_save.players[g_curPlayer].highScoreMilestone = 1;
    }
}

// Raises the profile's marathon high score to v if higher, saves it, and flags the
// current player's highScoreMilestone once v is at least 200,000,000.
void UpdateMarathonScore(int profile, __int64 v)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if (v > g_acc.level100HighScore)
            g_acc.level100HighScore = v;
        PackAccount(profile);
        SaveAccount(profile);
        if (g_gameMode != MODE_TIME_TRIAL && v >= MARATHON_MILESTONE_SCORE)
            g_save.players[g_curPlayer].highScoreMilestone = 1;
    }
}

// Formats the profile's Meteorstorm (team mode) high score into g_logBuf.
void ShowMeteorStormScore(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        Int64ToStrGrouped(g_acc.meteorstormHighScore, g_logBuf);
        ClearAccount();
    }
}

// Raises the profile's Meteorstorm high score to v if higher, then saves the account.
void UpdateMeteorStormScore(int profile, __int64 v)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if (v > g_acc.meteorstormHighScore)
            g_acc.meteorstormHighScore = v;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Formats the profile's Time Trial high score into g_logBuf/g_shownStat.
void ShowTimeTrialScore(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_shownStat = g_acc.timeTrialHighScore;
        Int64ToStrGrouped(g_acc.timeTrialHighScore, g_logBuf);
        ClearAccount();
    }
}

// Raises the profile's Time Trial high score to v if higher, then saves the account.
void UpdateTimeTrialScore(int profile, __int64 v)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if (v > g_acc.timeTrialHighScore)
            g_acc.timeTrialHighScore = v;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Formats the profile's total bonus levels played into g_logBuf/g_shownStat.
void ShowPerfectAttempts(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_shownStat = g_acc.bonusLevelsPlayed;
        sprintf(g_logBuf, "%d", g_acc.bonusLevelsPlayed);
        ClearAccount();
    }
}

// Formats the profile's perfect bonus level count into g_logBuf/g_shownStat.
void ShowPerfectCount(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_shownStat = g_acc.perfectBonusLevels;
        sprintf(g_logBuf, "%d", g_acc.perfectBonusLevels);
        ClearAccount();
    }
}

// Adds a perfect bonus levels and b bonus levels played to the profile, and resets the
// session's perfect/kill counters.
void AddStats(int profile, int a, int b)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_acc.perfectBonusLevels += a;
        g_acc.bonusLevelsPlayed += b;
        g_perfectCount = g_killCount = 0;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Formats the profile's perfect-bonus-level percentage into g_logBuf.
void ShowRatio(int profile)
{
    g_logBuf[0] = 0;
    if (profile != -1) {
        UnpackAccount(profile);
        if (g_acc.bonusLevelsPlayed) {
            sprintf(g_logBuf, "%d %%",
                    (int)((double)g_acc.perfectBonusLevels / g_acc.bonusLevelsPlayed * 100.0));
        } else {
            sprintf(g_logBuf, "%d %%", 0);
        }
        ClearAccount();
    }
}

// Formats the profile's total play time (the FILETIME-valued g_acc.playTime) into g_logBuf as
// days/hours/minutes/seconds, showing only the coarsest non-zero units.
void ShowPlayTime(int profile)
{
    SysDate sy;
    int m;
    int d;
    int h;
    int mi;
    int s;

    g_logBuf[0] = 0;
    if (profile != -1) {
        UnpackAccount(profile);
        SysFileTimeToDate(g_acc.playTime, &sy);
        m = (sy.month - 1) * 30;
        d = sy.day + m - 1;
        h = sy.hour;
        mi = sy.minute;
        s = sy.second;

        if (d > 0)
            sprintf(g_logBuf, "%1dD,%2dH,%2dM,%2dSEC", d, h, mi, s);
        else if (h > 0)
            sprintf(g_logBuf, "%2dH, %2dM, %2dSEC", h, mi, s);
        else if (mi > 0)
            sprintf(g_logBuf, "%2d M, %2d SEC", mi, s);
        else
            sprintf(g_logBuf, "%2d SECONDS", s);
        ClearAccount();
    }
}

// Adds (a - b - c) 100ns ticks to the profile's total play time, clamped at zero.
void AddPlayTime(int profile, __int64 a, __int64 b, __int64 c)
{
    if (profile != -1) {
        g_profilePlayTimeAdded = 1;
        UnpackAccount(profile);
        g_acc.playTime += a - b - c;
        if (g_acc.playTime < 0)
            g_acc.playTime = 0;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Sets the profile's unlocked rank directly and saves the account.
void SetStat(int profile, int v)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_acc.unlockedRank = v;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Returns the profile's unlocked rank, or 20 if the profile is invalid.
int GetStat(int profile)
{
    int r = 20;
    if (profile != -1) {
        UnpackAccount(profile);
        r = g_acc.unlockedRank;
        ClearAccount();
    }
    return r;
}

// Returns the profile's completion rank (0-3), or 0 if the profile is invalid.
int GetRank(int slot)
{
    int r = 0;
    if (slot != -1) {
        UnpackAccount(slot);
        r = g_acc.completionRank;
        ClearAccount();
    }
    return r;
}

// Advances the profile's completion rank by one, capped at 3, and saves the account.
void IncrementRank(int slot)
{
    if (slot != -1) {
        UnpackAccount(slot);
        g_acc.completionRank++;
        if (g_acc.completionRank > 3)
            g_acc.completionRank = 3;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Returns the medal bit awarded at position idx (0-5) in the profile's medal-award order.
int GetMedalOrder(int slot, int idx)
{
    int r = 0;
    if (slot != -1) {
        UnpackAccount(slot);
        if (idx == 0)
            r = g_acc.medalOrder[0];
        if (idx == 1)
            r = g_acc.medalOrder[1];
        if (idx == 2)
            r = g_acc.medalOrder[2];
        if (idx == 3)
            r = g_acc.medalOrder[3];
        if (idx == 4)
            r = g_acc.medalOrder[4];
        if (idx == 5)
            r = g_acc.medalOrder[5];
        ClearAccount();
    }
    return r;
}

// Awards `medal` to the profile if not already held, tracking the order medals were
// earned. Also drives the secret 1-2-4-8-16-32 medal-sequence state machine
// (g_acc.medalStep) that unlocks a planet rank, and resets ratio/medal progress once all
// 6 planet medals (mask 0x3f) are collected at completion rank 3.
void AwardMedal(int slot, int medal)
{
    if (slot != -1) {
        UnpackAccount(slot);
        if (g_acc.completionRank == 2) {
            switch (g_acc.medalStep) {
            case MEDAL_STEP_RESET:
                if (medal == MEDAL_DRUNK_FINISH)
                    g_acc.medalStep = MEDAL_DRUNK_FINISH;
                else
                    g_acc.medalStep = MEDAL_STEP_RESET;
                break;

            case MEDAL_DRUNK_FINISH:
                if (medal == MEDAL_SPEED_STREAK)
                    g_acc.medalStep = MEDAL_SPEED_STREAK;
                else
                    g_acc.medalStep = MEDAL_STEP_RESET;
                break;

            case MEDAL_SPEED_STREAK:
                if (medal == MEDAL_BONUS_RATIO)
                    g_acc.medalStep = MEDAL_BONUS_RATIO;
                else
                    g_acc.medalStep = MEDAL_STEP_RESET;
                break;

            case MEDAL_BONUS_RATIO:
                if (medal == MEDAL_ALL_LEVELS)
                    g_acc.medalStep = MEDAL_ALL_LEVELS;
                else
                    g_acc.medalStep = MEDAL_STEP_RESET;
                break;

            case MEDAL_ALL_LEVELS:
                if (medal == MEDAL_OVERALL)
                    g_acc.medalStep = MEDAL_OVERALL;
                else
                    g_acc.medalStep = MEDAL_STEP_RESET;
                break;

            case MEDAL_OVERALL:
                if (medal == MEDAL_EXACT_MONEY) {
                    if (g_acc.completionRank < 3) {
                        int i;
                        float ratio;

                        g_acc.completionRank++;
                        medal = 0;
                        g_acc.medals = 0;
                        medal = 0;
                        g_acc.medals = 0;
                        for (i = 0; i < g_numLevels; i++)
                            g_acc.levelDone[i] = 0;
                        g_acc.secretsInOneGame = 0;
                        CLEAR_MEDAL_ORDER(g_acc)
                        g_acc.medalStep = MEDAL_STEP_RESET;

                        if (g_acc.bonusLevelsPlayed > RATIO_RESCALE_BASE) {
                            ratio = g_acc.bonusLevelsPlayed / RATIO_RESCALE_BASE;
                            if (ratio == 0.0)
                                ratio = 1.0;
                            g_acc.perfectBonusLevels = g_acc.perfectBonusLevels / ratio;
                            g_acc.bonusLevelsPlayed = 450;
                            if (g_acc.perfectBonusLevels < 0)
                                g_acc.perfectBonusLevels = 0;
                        }

                        if (g_save.players[g_curPlayer].money > PLANET_RANK_MONEY_COST)
                            g_save.players[g_curPlayer].money -= PLANET_RANK_MONEY_COST;
                        sprintf(g_alertMsg, "**** *  PLANET RANKS NOW AVAILABLE  * ****");
                        SoundQueueAdd(g_sfxPlanet, 40, 1);
                        SoundQueueAdd(g_sfxRank, 40, 1);
                        SoundQueueAdd(g_sfxAvailable, 40, 1);

                        ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer, MILESTONE_SCORE_BONUS);
                        AddScorePopup(g_screenW >> 1, g_screenH >> 1, MILESTONE_SCORE_BONUS, 1);
                        g_msgColor = 8;
                        g_msgTimer = g_time + 7000;
                    }
                } else
                    g_acc.medalStep = MEDAL_STEP_RESET;
                break;
            }
        }

        int allMedalsMask;
        int j;
        float bonusRatio;
        int newRank;

        if (medal != 0 && (g_acc.medals & medal) != medal) {
            g_acc.medals = g_acc.medals | medal;
            if (g_acc.medalOrder[6] == 0)
                g_acc.medalOrder[0] = medal;
            if (g_acc.medalOrder[6] == 1)
                g_acc.medalOrder[1] = medal;
            if (g_acc.medalOrder[6] == 2)
                g_acc.medalOrder[2] = medal;
            if (g_acc.medalOrder[6] == 3)
                g_acc.medalOrder[3] = medal;
            if (g_acc.medalOrder[6] == 4)
                g_acc.medalOrder[4] = medal;
            if (g_acc.medalOrder[6] == 5)
                g_acc.medalOrder[5] = medal;
            if (g_acc.medalOrder[6] < 6)
                g_acc.medalOrder[6]++;
        }

        if (g_acc.completionRank == 3 && (allMedalsMask = g_acc.medals & MEDALS_ALL) == MEDALS_ALL &&
            g_acc.unlockedRank < MAX_RANK) {
            medal = 0;
            g_acc.medals = 0;
            for (j = 0; j < g_numLevels; j++)
                g_acc.levelDone[j] = 0;
            g_acc.secretsInOneGame = 0;
            CLEAR_MEDAL_ORDER(g_acc)
            g_acc.medalStep = MEDAL_STEP_RESET;

            bonusRatio = g_acc.bonusLevelsPlayed / RATIO_RESCALE_BASE;
            if (bonusRatio == 0.0)
                bonusRatio = 1.0;
            g_acc.perfectBonusLevels = g_acc.perfectBonusLevels / bonusRatio;
            g_acc.bonusLevelsPlayed = 450;
            if (g_acc.perfectBonusLevels < 0)
                g_acc.perfectBonusLevels = 0;

            if (g_save.players[g_curPlayer].money > PLANET_RANK_MONEY_COST)
                g_save.players[g_curPlayer].money -= PLANET_RANK_MONEY_COST;
            ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer, MILESTONE_SCORE_BONUS);
            AddScorePopup(g_screenW >> 1, g_screenH >> 1, MILESTONE_SCORE_BONUS, 1);
            sprintf(g_alertMsg, "NEW PLANET RANK AVAILABLE");
            SoundQueueAdd(g_sfxNew, 40, 1);
            SoundQueueAdd(g_sfxPlanet, 40, 1);
            SoundQueueAdd(g_sfxRank, 200, 1);
            SoundQueueAdd(g_sfxAvailable, 40, 1);

            if (g_gameMode == MODE_DUAL) {
                if (g_curPlayer == 0)
                    g_msgColor = 1;
                else
                    g_msgColor = 4;
            } else
                g_msgColor = 1;
            g_msgTimer = g_time + 5000;

            newRank = g_acc.unlockedRank + 1;
            if (newRank > MAX_RANK)
                newRank = MAX_RANK;
            g_acc.unlockedRank = newRank;
        }
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Returns the profile's medal bitmask, or 0 if the profile is invalid.
int GetMedals(int slot)
{
    int r = 0;
    if (slot != -1) {
        UnpackAccount(slot);
        r = g_acc.medals;
        ClearAccount();
    }
    return r;
}

// Clears the profile's recorded medal-award order and saves the account.
void ClearMedalOrder(int slot)
{
    if (slot != -1) {
        UnpackAccount(slot);
        CLEAR_MEDAL_ORDER(g_acc)
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Clears the medals in `mask` from the profile's medal bitmask and resets medalStep.
void ClearMedalsMask(int slot, int mask)
{
    int inv;
    if (slot != -1) {
        UnpackAccount(slot);
        inv = mask ^ MEDALS_ALL;
        g_acc.medals = g_acc.medals & inv;
        g_acc.medalStep = MEDAL_STEP_RESET;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Clears every level's "secret found" flag for the profile.
void ClearLevelsDone(int slot)
{
    int i;
    if (slot != -1) {
        UnpackAccount(slot);
        for (i = 0; i < g_numLevels; i++)
            g_acc.levelDone[i] = 0;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Resets the profile's best level-100 time to zero.
void ClearRecordStat(int slot)
{
    if (slot != -1) {
        UnpackAccount(slot);
        // NOTE: zeroes only the low 32 bits via an (int*) cast, then the high half
        // separately, instead of assigning g_acc.level100HighScore = 0 directly.
        *(int *)&g_acc.level100HighScore = 0;
        g_acc.level100HighScoreHi = 0;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Rescales perfectBonusLevels/bonusLevelsPlayed down to a 450-game baseline (used when
// the ratio-medal threshold changes), then saves the account.
void RescaleRatioStat(int slot)
{
    float f;
    if (slot != -1) {
        UnpackAccount(slot);
        f = g_acc.bonusLevelsPlayed / RATIO_RESCALE_BASE;
        if (f == 0.0)
            f = 1.0;
        g_acc.perfectBonusLevels = g_acc.perfectBonusLevels / f;
        g_acc.bonusLevelsPlayed = 450;
        if (g_acc.perfectBonusLevels < 0)
            g_acc.perfectBonusLevels = 0;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// True if the profile holds all 6 planet medals (bits 0-5).
bool HasAllMedals(int slot)
{
    bool r = false;
    int m;
    if ((m = GetMedals(slot) & MEDALS_ALL) == MEDALS_ALL)
        r = true;
    return r;
}

// Awards the perfect-ratio medal (bit 2) once bonusLevelsPlayed >= 500 and the perfect
// percentage is >= 75%.
void CheckRatioMedal(int slot)
{
    int pct;
    if (slot != -1) {
        UnpackAccount(slot);
        if (g_acc.bonusLevelsPlayed != 0)
            pct = (double)g_acc.perfectBonusLevels / g_acc.bonusLevelsPlayed * 100.0;
        else
            pct = 0;
        if (g_acc.bonusLevelsPlayed >= MEDAL_RATIO_MIN_PLAYED && pct >= MEDAL_RATIO_MIN_PCT) {
            if ((GetMedals(g_profileIndex) & MEDAL_BONUS_RATIO) == 0)
                AwardMedal(slot, MEDAL_BONUS_RATIO);
        } else
            ClearAccount();
    }
}

// Awards the all-levels medal (bit 3) once every level's secret has been found.
void CheckAllLevelsMedal(int slot)
{
    int n;
    int i;
    if (slot != -1) {
        UnpackAccount(slot);
        n = 0;
        for (i = 0; i < g_numLevels; i++) {
            if (g_acc.levelDone[i] != 0)
                n++;
        }
        if (n == g_numLevels) {
            if ((GetMedals(g_profileIndex) & MEDAL_ALL_LEVELS) == 0)
                AwardMedal(slot, MEDAL_ALL_LEVELS);
        } else
            ClearAccount();
    }
}

// Formats the profile's fastest level-clear time into g_logBuf/g_shownStat, and raises
// g_timeMax (extra bonus time) once that time drops under 2 or 1 seconds.
void FormatBestTime(int slot)
{
    SysDate sy;
    int mn;
    int sc;
    int ms;
    if (slot != -1) {
        g_shownStat = NO_BEST_TIME;
        UnpackAccount(slot);

        if (g_acc.bestLevelTime < NO_BEST_TIME) {
            SysFileTimeToDate(g_acc.bestLevelTime, &sy);
            mn = sy.minute;
            sc = sy.second;
            ms = sy.milliseconds;

            g_shownStat = (__int64)(mn * 1000 * 60) + sc * 1000 + ms;
            if (mn < 44) {
                if (mn > 0)
                    sprintf(g_logBuf, "%d M, %d.%02d SEC", mn, sc, ms);
                else
                    sprintf(g_logBuf, "%d.%02d SEC", sc, ms);
            } else
                sprintf(g_logBuf, "--.--- SEC");
            g_shownStat = sc * 1000 + ms;
            ClearAccount();

            if (g_gameMode != MODE_TIME_TRIAL) {
                if (g_shownStat <= 2000)
                    g_timeMax = 60;
                if (g_shownStat <= 1000)
                    g_timeMax = 90;
            }
        } else
            sprintf(g_logBuf, "--.--- SEC");
    }
}

// Lowers the profile's best level-clear time to t if t is a valid, faster time.
void UpdateBestTime(int slot, __int64 t)
{
    if (slot != -1) {
        UnpackAccount(slot);
        if (t > 0 && t < NO_BEST_TIME && g_acc.bestLevelTime > t) {
            g_acc.bestLevelTime = t;
        }
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Adds n to the profile's shots-fired counter.
void AddScoreStat(int slot, int n)
{
    if (slot != -1) {
        UnpackAccount(slot);
        g_acc.shotsFired = g_acc.shotsFired + n;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Adds amount to the profile's shots-hit counter.
void AddHitsStat(int id, int amount)
{
    if (id != -1) {
        UnpackAccount(id);
        g_acc.shotsHit = g_acc.shotsHit + amount;
        PackAccount(id);
        SaveAccount(id);
    }
}

// Formats the profile's fastest Meteorstorm clear time into g_logBuf/g_shownStat.
void ShowFastestMeteorstorm(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        SysDate sy;
        int m;
        int s;
        int ms;
        SysFileTimeToDate(g_acc.bestMeteorstormTime, &sy);

        m = sy.minute;
        s = sy.second;
        ms = sy.milliseconds;

        g_shownStat = (__int64)(m * 1000 * 60) + s * 1000 + ms;
        if (m < 15) {
            if (m > 0)
                sprintf(g_logBuf, "%d M, %d.%02d SEC", m, s, ms);
            else
                sprintf(g_logBuf, "%d.%02d SEC", s, ms);
        } else {
            sprintf(g_logBuf, "--.--- SEC");
        }
        ClearAccount();
    }
}

// Lowers the profile's best Meteorstorm time to t if t is faster.
void UpdateFastestMeteorStorm(int id, __int64 t)
{
    if (id != -1) {
        UnpackAccount(id);
        if (g_acc.bestMeteorstormTime > t)
            g_acc.bestMeteorstormTime = t;
        PackAccount(id);
        SaveAccount(id);
    }
}

// Formats how many level secrets the profile has found (out of g_numLevels) into g_logBuf.
void ShowSecretsFound(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        int count = 0;
        for (int i = 0; i < g_numLevels; i++) {
            if (g_acc.levelDone[i])
                count++;
        }
        sprintf(g_logBuf, "%d OUT OF %d", count, g_numLevels);
        g_shownStat = count;
        ClearAccount();
    }
}

// Formats the profile's best secrets-found-in-one-game count into g_logBuf/g_shownStat.
void ShowSecretsInOneGame(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        g_shownStat = g_acc.secretsInOneGame;
        sprintf(g_logBuf, "%lld", (__int64)g_acc.secretsInOneGame);
        ClearAccount();
    }
}

// Marks the profile's campaign as completed and saves the account.
void SetGameCompleted(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        g_acc.gameCompleted = true;
        PackAccount(id);
        SaveAccount(id);
    }
}

// True if the profile's campaign is completed.
bool IsGameCompleted(int id)
{
    bool done = false;
    if (id != -1) {
        UnpackAccount(id);
        if (g_acc.gameCompleted)
            done = true;
        ClearAccount();
    }
    return done;
}

// Marks `level`'s secret as found for the profile (first time only), updates the current
// player's secret count for this game, and, on finding every secret in one game for the
// first time, awards a score bonus.
void MarkSecretFound(int id, int level)
{
    if (id != -1) {
        int count = 0;
        bool bRecord = false;
        bool bFirst = false;
        UnpackAccount(id);

        for (int i = 0; i < g_numLevels; i++) {
            if (g_save.players[g_curPlayer].secretFlags[i] != 0)
                count++;
        }
        if (!g_acc.levelDone[level - 1]) {
            g_acc.levelDone[level - 1] = true;
            SoundQueueAdd(g_sampleSecret, 50, 0);
            bFirst = true;
        }
        g_save.players[g_curPlayer].secretCount = count;
        if (count > g_acc.secretsInOneGame) {
            g_acc.secretsInOneGame = count;
            bRecord = true;
        }
        if (g_acc.secretsInOneGame > g_numLevels)
            g_acc.secretsInOneGame = 0;

        if (g_acc.secretsInOneGame == g_numLevels && bFirst && bRecord) {
            ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer, MILESTONE_SCORE_BONUS);
            AddScorePopup(g_screenW >> 1, g_screenH >> 1, MILESTONE_SCORE_BONUS, 1);
            g_msgColor = 8;
            g_msgTimer = g_time + 7000;
        }
        PackAccount(id);
        SaveAccount(id);
        CheckAllLevelsMedal(id);
    }
}

// True if the profile has already found `level`'s secret.
bool IsSecretFound(int id, int level)
{
    bool done = false;
    if (id != -1) {
        UnpackAccount(id);
        done = g_acc.levelDone[level - 1];
        ClearAccount();
    }
    return done;
}

// Formats the profile's highest level reached into g_logBuf/g_shownStat.
void ShowHighestLevelReached(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        g_shownStat = g_acc.highestLevel;
        sprintf(g_logBuf, "%lld", (__int64)g_acc.highestLevel);
        ClearAccount();
    }
}

// Raises the profile's highest level reached to `value` if higher.
void UpdateHighestLevel(int id, int value)
{
    if (id != -1) {
        UnpackAccount(id);
        if (value > g_acc.highestLevel)
            g_acc.highestLevel = value;
        PackAccount(id);
        SaveAccount(id);
    }
}

// Formats the profile's total levels played into g_logBuf/g_shownStat, and (outside
// Time Trial) unlocks autofire, turret-tracking reduction, the gem counter, blue money
// and the score multiplier, and disables two bonus weights, as thresholds are crossed.
void ShowTotalLevelsPlayed(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        g_shownStat = g_acc.totalLevelsPlayed;
        sprintf(g_logBuf, "%lld", (__int64)g_acc.totalLevelsPlayed);
        ClearAccount();

        if (g_gameMode != MODE_TIME_TRIAL) {
            if (g_shownStat >= 1000)
                g_save.players[g_curPlayer].autofireUnlocked = 1;
            if (g_shownStat >= 2500)
                g_save.players[g_curPlayer].turretTrackingReduction = 25;
            if (g_shownStat >= 5000)
                g_save.players[g_curPlayer].gemCounterUnlocked = 1;
            if (g_shownStat >= 10000)
                g_bonusWeight[12] = 0;
            if (g_shownStat >= 15000)
                g_bonusWeight[13] = 0;
            if (g_shownStat >= 20000)
                g_save.players[g_curPlayer].blueMoneyUnlocked = 1;
            if (g_shownStat >= 25000)
                g_bonusWeight[14] = 0;
            if (g_shownStat >= 35000)
                g_save.players[g_curPlayer].multiplierUnlocked = 1;
        }
    }
}

// Adds `amount` to the profile's total levels played.
void AddLevelsPlayed(int id, int amount)
{
    if (id != -1) {
        UnpackAccount(id);
        g_acc.totalLevelsPlayed = g_acc.totalLevelsPlayed + amount;
        PackAccount(id);
        SaveAccount(id);
    }
}

// Formats the profile's total games played into g_logBuf/g_shownStat.
void ShowTotalGamesPlayed(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        g_shownStat = g_acc.gamesPlayed;
        sprintf(g_logBuf, "%lld", (__int64)g_acc.gamesPlayed);
        ClearAccount();
    }
}

// Adds the pending new-game count to the profile's games-played total, then clears it.
void IncrementGamesPlayed(int id)
{
    if (id != -1) {
        UnpackAccount(id);
        g_acc.gamesPlayed = g_acc.gamesPlayed + g_newGamePending;
        g_newGamePending = 0;
        PackAccount(id);
        SaveAccount(id);
    }
}

// Formats the profile's best hit percentage recorded after level 25 into
// g_logBuf/g_shownStat.
void FormatHitPctAbove25(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_shownStat = g_acc.bestHitPctAbove25;
        sprintf(g_logBuf, "%d %%", g_acc.bestHitPctAbove25);
        ClearAccount();
    }
}

// Raises the profile's best post-level-25 hit percentage to `value` if higher.
void UpdateHitPctAbove25(int profile, int value)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if (value > g_acc.bestHitPctAbove25)
            g_acc.bestHitPctAbove25 = value;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Formats the profile's lifetime shot-hit percentage into g_logBuf.
void FormatTotalHitPct(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if (g_acc.shotsFired != 0) {
            sprintf(g_logBuf, "%d %%", (int)((double)g_acc.shotsHit / g_acc.shotsFired * 100.0));
        } else {
            sprintf(g_logBuf, "%d %%", 0);
        }
        ClearAccount();
    }
}

// Formats the profile's highest money held into g_logBuf/g_shownStat; a read-only
// profile masks amounts over 99999 as "???".
void FormatHighestMoney(int profile)
{
    if (profile != -1) {
        UnpackAccount(profile);
        g_shownStat = (__int64)g_acc.highestMoney;
        if (g_profileReadOnly && g_shownStat > 99999)
            sprintf(g_logBuf, "???");
        else
            sprintf(g_logBuf, "%lld", (__int64)g_acc.highestMoney);
        ClearAccount();
    }
}

// Raises the profile's highest money held to `value` if higher.
void UpdateHighestMoney(int profile, int value)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if ((double)value > g_acc.highestMoney)
            g_acc.highestMoney = value;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Formats the profile's highest rank name into g_logBuf/g_shownRank, clearing a stale
// highestRank that is above the currently-unlocked rank. A read-only profile masks
// near-max ranks as "???".
void FormatHighestRank(int profile)
{
    int n;
    if (profile != -1) {
        n = GetStat(profile);
        UnpackAccount(profile);
        if (g_acc.highestRank > n) {
            g_acc.highestRank = 0;
            PackAccount(profile);
            SaveAccount(profile);
        }

        g_shownRank = g_acc.highestRank;
        if (g_profileReadOnly && g_shownRank >= RANK_GOD)
            sprintf(g_logBuf, "???");
        else
            sprintf(g_logBuf, "%s", g_rankNames[g_acc.highestRank]);
        ClearAccount();
    }
}

// Raises the profile's highest rank reached to `value` if higher.
void UpdateHighestRank(int profile, int value)
{
    if (profile != -1) {
        UnpackAccount(profile);
        if (value > g_acc.highestRank)
            g_acc.highestRank = value;
        PackAccount(profile);
        SaveAccount(profile);
    }
}

// Returns the medal icon's Y offset in the medals sprite sheet for medal bit `n`.
int MedalIconOffsetY(int n)
{
    int r = 0;
    if (n == MEDAL_DRUNK_FINISH)
        r = 0;
    if (n == MEDAL_SPEED_STREAK)
        r = 0x40;
    if (n == MEDAL_BONUS_RATIO)
        r = 0x80;
    if (n == MEDAL_ALL_LEVELS)
        r = 0xc0;
    if (n == MEDAL_OVERALL)
        r = 0x100;
    if (n == MEDAL_EXACT_MONEY)
        r = 0x140;
    return r;
}

// One row of a stat-threshold unlock ladder: draws the toggle on/off depending on whether
// g_shownStat has reached `threshold`, and once one is missed `flag` stays off so every
// higher toggle in the ladder is also drawn (but not selectable) as not-yet-earned.
#define UNLOCK_TOGGLE_GE(threshold, xExpr, yExpr, label)          \
    if (g_shownStat >= (threshold)) {                             \
        WinAddToggle((xExpr), (yExpr), g_curWin, 1, (label), flag); \
    } else {                                                       \
        WinAddToggle((xExpr), (yExpr), g_curWin, 0, (label), flag); \
        flag = 0;                                                   \
    }

// Same as UNLOCK_TOGGLE_GE, but the ladder counts down (a *faster* time is the unlock).
#define UNLOCK_TOGGLE_LE(threshold, xExpr, yExpr, label)          \
    if (g_shownStat <= (threshold)) {                              \
        WinAddToggle((xExpr), (yExpr), g_curWin, 1, (label), flag); \
    } else {                                                        \
        WinAddToggle((xExpr), (yExpr), g_curWin, 0, (label), flag);  \
        flag = 0;                                                    \
    }

// A one-off (not-yet-earned rows aren't drawn at all) threshold toggle: only counts toward
// g_toggleOffCount when missed.
#define UNLOCK_TOGGLE_GE_OFF(threshold, xExpr, yExpr, label)      \
    if (g_shownStat >= (threshold)) {                              \
        WinAddToggle((xExpr), (yExpr), g_curWin, 1, (label), flag); \
    } else {                                                        \
        g_toggleOffCount++;                                          \
    }

// Builds the profile-statistics window: score/time/secret stats, the ship-unlock toggle
// rows (one per money/score/level threshold), and, once the profile is fully ranked
// (shownRank 32), the planet-rank tour graphic, or otherwise the earned medal icons and
// remaining planet-rank progress icons.
void ProfileWindow(bool noButtons)
{
    int h = 366;
    int w = 550;
    int y;
    int x = 427;
    int medalRowYAdjust = 0;
    int rankRowYAdjust = 0;
    bool flag;
    int spacing;
    int x0;
    int y0;
    int row;
    int rankIcon;
    int rankIdx;
    int sz;

    int medalIdx;
    bool any;
    int cx;
    int srcY;
    int medal;
    int n;
    int size;
    int my;
    int k;
    int mx;

    // ---- setup: exact-money medal check, then WinOpen sized for the sections below ----
    if (g_save.players[g_curPlayer].money == MAX_MONEY) {
        if ((GetMedals(g_profileIndex) & MEDAL_EXACT_MONEY) == 0)
            AwardMedal(g_profileIndex, MEDAL_EXACT_MONEY);
    }

    // grow the window to fit the medal/rank-icon sections drawn near the end
    if (!g_profileReadOnly) {
        FormatHighestRank(g_profileIndex);
        if (g_shownRank == MAX_RANK) {
            h += 200;
        } else {
            if (g_shownRank > RANK_GOD) {
                h += 50;
                rankRowYAdjust += 60;
            }
            if (GetMedals(g_profileIndex) > 0) {
                h += 75;
                medalRowYAdjust += 75;
                rankRowYAdjust += 75;
            }
        }
    }

    g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
    g_profileWin = g_curWin;
    WinAddText(30, 20, g_curWin, "USER:", 8);
    GetProfileName(g_profileIndex);
    WinAddText(70, 20, g_curWin, g_logBuf, 0xc);
    if (GetProfileEasyFlag(g_profileIndex))
        WinAddText(w - 130, 20, g_curWin, "EASY PROFILE", 10);
    WinAddText(30, 40, g_curWin, "HIGHEST POINTS SCORED", 8);
    WinAddText(30, 50, g_curWin, "          IN THE GAME :", 8);
    ShowHighScore(g_profileIndex);
    WinAddText(220, 50, g_curWin, g_logBuf, 0xc);
    g_toggleOnCount = 0;
    g_toggleOffCount = 0;
    y = 47;
    flag = !g_profileReadOnly;

    // Score-threshold unlock ladder: once a lower threshold is missed, `flag` stays off so
    // every higher toggle is also drawn (but not selectable) as not-yet-earned.
    UNLOCK_TOGGLE_GE(5000000, x, y, "5.000.000 : START WITH 10 BULLETS")
    UNLOCK_TOGGLE_GE(7500000, g_nextX + 1, y, "7.500.000 : START WITH SPEED * 3")
    UNLOCK_TOGGLE_GE(10000000, g_nextX + 1, y, "10.000.000 : START WITH AUTOFIRE")

    UNLOCK_TOGGLE_GE(20000000, g_nextX + 1, y, "20.000.000 : START WITH DOUBLE SHOT")
    UNLOCK_TOGGLE_GE(50000000, g_nextX + 1, y, "50.000.000 : START WITH 1 ARMOUR")

    UNLOCK_TOGGLE_GE(100000000, g_nextX + 1, y, "100.000.000 : START WITH $500")
    UNLOCK_TOGGLE_GE(250000000, g_nextX + 1, y, "250.000.000 : START WITH $1000")
    UNLOCK_TOGGLE_GE(500000000, g_nextX + 1, y, "500.000.000 : START WITH 2 ARMOUR")
    UNLOCK_TOGGLE_GE(1000000000, g_nextX + 1, y, "1.000.000.000 : START WITH TRIPLE SHOT")

    WinAddText(30, 60, g_curWin, "       IN METEORSTORM :", 8);
    ShowMeteorStormScore(g_profileIndex);
    WinAddText(220, 60, g_curWin, g_logBuf, 12);
    WinAddText(30, 70, g_curWin, "        IN TIME TRIAL :", 8);
    ShowTimeTrialScore(g_profileIndex);
    WinAddText(220, 70, g_curWin, g_logBuf, 12);
    flag = !g_profileReadOnly;

    // Meteorstorm score unlock ladder, same missed-threshold behavior as above.
    UNLOCK_TOGGLE_GE(5000000, x, 0x43, "5.000.000 : START WITH MULTIPLY 2")
    UNLOCK_TOGGLE_GE(6000000, g_nextX + 1, 0x43, "6.000.000 : START WITH SCOOP")
    UNLOCK_TOGGLE_GE(7000000, g_nextX + 1, 0x43, "7.000.000 : START WITH MULTIPLY 5")
    UNLOCK_TOGGLE_GE(8000000, g_nextX + 1, 0x43, "8.000.000 : START WITH AUTOFIRE")

    UNLOCK_TOGGLE_GE(9000000, g_nextX + 1, 0x43, "9.000.000 : START WITH SPEED * 3")
    UNLOCK_TOGGLE_GE(10000000, g_nextX + 1, 0x43, "10.000.000 : START WITH SPEED * 5")
    UNLOCK_TOGGLE_GE(15000000, g_nextX + 1, 0x43, "15.000.000 : START WITH SUPER AUTOFIRE")
    UNLOCK_TOGGLE_GE(17000000, g_nextX + 1, 0x43, "17.000.000 : START WITH MAX SPEED")

    UNLOCK_TOGGLE_GE(20000000, g_nextX + 1, 0x43, "20.000.000 : GET 1 MORE MINUTE TO PLAY WITH")

    // ---- marathon (level 100) score ----
    WinAddText(30, 80, g_curWin, "         AT LEVEL 100 :", 8);
    ShowMarathonScore(g_profileIndex);
    WinAddText(220, 80, g_curWin, g_logBuf, 12);
    if (g_shownStat > MARATHON_MILESTONE_SCORE) {
        WinAddToggle(x, 0x4d, g_curWin, 1, ">200.000.000 : START WITH SECRET COUNTER ON", flag);
    } else {
        g_toggleOffCount++;
    }

    // ---- bonus-level stats ----
    WinAddText(30, 96, g_curWin, "BONUS LEVELS PLAYED        :", 8);
    ShowPerfectAttempts(g_profileIndex);
    WinAddText(260, 96, g_curWin, g_logBuf, 12);
    WinAddText(30, 105, g_curWin, "PERFECT BONUS LEVELS       :", 8);
    ShowPerfectCount(g_profileIndex);
    WinAddText(260, 105, g_curWin, g_logBuf, 12);
    WinAddText(30, 114, g_curWin, "PERFECT BONUS LEVELS RATIO :", 8);
    ShowRatio(g_profileIndex);
    WinAddText(260, 114, g_curWin, g_logBuf, 12);

    // ---- play time and best-level-time toggles ----
    WinAddText(30, 131, g_curWin, "          TOTAL GAME TIME :", 8);
    ShowPlayTime(g_profileIndex);
    WinAddText(250, 130, g_curWin, g_logBuf, 12);

    WinAddText(30, 141, g_curWin, "FASTEST CLEARING OF LEVEL :", 8);
    FormatBestTime(g_profileIndex);
    WinAddText(250, 140, g_curWin, g_logBuf, 12);
    y = 0x8a;
    flag = !g_profileReadOnly;
    UNLOCK_TOGGLE_LE(2000, x, y, "<= 2 SECOND : MAX EXTRA TIME IS 60 SECONDS")
    UNLOCK_TOGGLE_LE(1000, g_nextX + 1, y, "<= 1 SECOND : MAX EXTRA TIME IS 90 SECONDS")

    // ---- fastest Meteorstorm and secrets found ----
    WinAddText(30, 150, g_curWin, "      FASTEST METEORSTORM :", 8);
    ShowFastestMeteorstorm(g_profileIndex);
    WinAddText(250, 150, g_curWin, g_logBuf, 12);

    WinAddText(30, 170, g_curWin, "SECRETS FOUND IN ONE GAME :", 8);
    ShowSecretsInOneGame(g_profileIndex);
    WinAddText(250, 170, g_curWin, g_logBuf, 12);
    WinAddText(30, 180, g_curWin, "            SECRETS FOUND :", 8);
    ShowSecretsFound(g_profileIndex);
    WinAddText(250, 180, g_curWin, g_logBuf, 12);
    y = 0xb1;
    flag = !g_profileReadOnly;
    if (g_shownStat > g_numLevels / 2) {
        if (g_shownStat == g_numLevels) {
            WinAddToggle(x, y, g_curWin, 1,
                         "FIND ALL SECRETS : START WITH 2 ARMOURS AND SUPER TRIPLE SHOT", flag);
        } else {
            WinAddToggle(x, y, g_curWin, 0,
                         "FIND ALL SECRETS : START WITH 2 ARMOURS AND SUPER TRIPLE SHOT", flag);
            flag = 0;
            g_toggleOffCount++;
        }
    } else {
        g_toggleOffCount++;
    }

    // ---- levels reached/played ----
    WinAddText(30, 200, g_curWin, "HIGHEST LEVEL REACHED :", 8);
    ShowHighestLevelReached(g_profileIndex);
    WinAddText(220, 200, g_curWin, g_logBuf, 12);
    WinAddText(30, 210, g_curWin, "   TOTAL LEVEL PLAYED :", 8);
    ShowTotalLevelsPlayed(g_profileIndex);
    WinAddText(220, 210, g_curWin, g_logBuf, 12);
    y = 0xcf;
    flag = !g_profileReadOnly;

    // Total-money unlock ladder, same missed-threshold behavior as above.
    UNLOCK_TOGGLE_GE(1000, x, y, "1.000 : AUTOFIRE WILL LAST THROUGH SHOP")
    UNLOCK_TOGGLE_GE(2500, g_nextX + 1, y, "2.500 : START WITH MISSILE STEALTH")
    UNLOCK_TOGGLE_GE(5000, g_nextX + 1, y, "5.000 : START WITH GEM COUNTER ON")
    UNLOCK_TOGGLE_GE(10000, g_nextX + 1, y, "10.000 : START WITH SINGLE SHOT BONUS OFF")

    UNLOCK_TOGGLE_GE(15000, g_nextX + 1, y, "15.000 : START WITH DOUBLE SHOT BONUS OFF")
    UNLOCK_TOGGLE_GE(20000, g_nextX + 1, y, "20.000 : START WITH ONLY BLUE COINS ON")
    UNLOCK_TOGGLE_GE(25000, g_nextX + 1, y, "25.000 : START WITH TRIPLE SHOT BONUS OFF")
    UNLOCK_TOGGLE_GE(35000, g_nextX + 1, y, "35.000 : START WITH MULTIPLY IN METEORSTORM")

    UNLOCK_TOGGLE_GE(50000, g_nextX + 1, y, "50.000 : START WITH QUAD SHOT")
    UNLOCK_TOGGLE_GE(75000, g_nextX + 1, y, "75.000 : START WITH EXTRA BULLET SPEED")
    UNLOCK_TOGGLE_GE(100000, g_nextX + 1, y, "100.000 : START WITH GOOD SPEED, BULLETS, TIME AND 5000 CASH")

    // ---- games played and hit percentage ----
    WinAddText(30, 220, g_curWin, "   TOTAL GAMES PLAYED :", 8);
    ShowTotalGamesPlayed(g_profileIndex);
    WinAddText(220, 220, g_curWin, g_logBuf, 12);
    flag = !g_profileReadOnly;

    WinAddText(30, 235, g_curWin, "HIGHEST HIT % ABOVE LEVEL 25:", 8);
    FormatHitPctAbove25(g_profileIndex);
    WinAddText(268, 235, g_curWin, g_logBuf, 12);
    y = 0xe9;
    UNLOCK_TOGGLE_GE_OFF(70, x, y, ">70% : ENABLE NEW ITEM IN SHOP")
    UNLOCK_TOGGLE_GE_OFF(80, g_nextX + 1, y, ">80% : ENABLE NEW ITEM IN SHOP")
    UNLOCK_TOGGLE_GE_OFF(90, g_nextX + 1, y, ">90% : ENABLE NEW ITEM IN SHOP")

    WinAddText(30, 245, g_curWin, "       TOTAL HIT PERCENTAGE :", 8);
    FormatTotalHitPct(g_profileIndex);
    WinAddText(268, 245, g_curWin, g_logBuf, 12);

    WinAddText(30, 265, g_curWin, " HIGHEST AMOUNT OF MONEY :", 8);
    FormatHighestMoney(g_profileIndex);
    WinAddText(240, 265, g_curWin, g_logBuf, 12);

    WinAddText(30, 280, g_curWin, "    HIGHEST RANK REACHED :", 8);
    FormatHighestRank(g_profileIndex);
    WinAddText(240, 280, g_curWin, g_logBuf, 12);

    // ---- rank-planet tour (fully ranked), or earned medals + remaining planet icons ----
    if (!g_profileReadOnly) {
        FormatHighestRank(g_profileIndex);
        if (g_shownRank == MAX_RANK) {
            spacing = 10;
            x0 = 30;
            y0 = h - 265;
            row = 0;

            // Two columns of 11 rank icons each, with small logo badges stacked on a few
            // specific ranks (the "you've earned N logos" milestones).
            for (rankIcon = 0; rankIcon < 22; rankIcon++) {
                WinAddItemA(x0, row * spacing + y0,
                            42, 9, 0, g_rankSprY[rankIcon], 64, 13, g_curWin, g_gfxRanks);
                if (rankIcon == 5) {
                    WinAddItemA(x0 + 5, row * spacing + y0, 10, 9, 0, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 6) {
                    WinAddItemA(x0 + 5, row * spacing + y0, 10, 9, 0, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 0, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 7) {
                    WinAddItemA(x0 + 5, row * spacing + y0, 10, 9, 0, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 0, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 27, row * spacing + y0,
                                10, 9, 0, 160, 16, 15, g_curWin, g_gfxLogos);
                }

                if (rankIcon == 8) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 16, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 9) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 16, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 16, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 10) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 16, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 16, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 27, row * spacing + y0,
                                10, 9, 16, 160, 16, 15, g_curWin, g_gfxLogos);
                }

                if (rankIcon == 11) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 12) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 13) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 27, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                }

                if (rankIcon == 18) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 19) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                }
                if (rankIcon == 20) {
                    WinAddItemA(x0 + 5, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 16, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                    WinAddItemA(x0 + 27, row * spacing + y0,
                                10, 9, 32, 160, 16, 15, g_curWin, g_gfxLogos);
                }

                row++;
                if (rankIcon == 10) {
                    x0 = w - 72;
                    y0 = h - 265;
                    row = 0;
                }
            }
            x0 = 30;
            y0 = h - 151;
            row = 0;

            // Final row of the remaining ranks (22-32), then the planet-tour graphic itself.
            for (rankIdx = 22; rankIdx < 33; rankIdx++) {
                WinAddItemA(row * 45 + x0, y0, 40, 8, 0, g_rankSprY[rankIdx], 64, 13, g_curWin, g_gfxRanks);
                row++;
            }

            WinAddItemA((w >> 1) - 64, h - 275, 128, 128, 0, 0, 128, 128, g_curWin, g_gfxRankPlanets);
            sz = 48;
            WinAddItemA((w >> 1) - 117, h - 265, sz, sz, 0, 128, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) + 85, h - 265, sz, sz, 0, 256, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) - 176, h - 250, sz, sz, 0, 384, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) + 144, h - 250, sz, sz, 0, 512, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) - 117, h - 230, sz, sz, 0, 640, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) + 85, h - 230, sz, sz, 0, 768, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) - 172, h - 210, sz, sz, 0, 896, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) + 140, h - 210, sz, sz, 0, 1024, 128, 128, g_curWin, g_gfxRankPlanets);
            WinAddItemA((w >> 1) + 70, h - 197, sz, sz, 0, 1152, 128, 128, g_curWin, g_gfxRankPlanets);
            x0 = 58;
            y0 = h - 136;
            row = 0;

            // Row of the 7 earned medal icons beneath the planet tour.
            for (medalIdx = 0; medalIdx < 7; medalIdx++) {
                WinAddItemA(row * 75 + x0, y0, 64, 70, medalIdx * 64, 0, 64, 70, g_curWin, g_gfxMedals);
                row++;
            }
        } else {
            any = 0;
            cx = 40;
            srcY = 0;
            medal = 0;
            if (GetRank(g_profileIndex) == 2)
                srcY = 70;

            // Draw each earned medal in the order it was awarded (medal = 0 is a dead store,
            // immediately overwritten by GetMedalOrder).
            medal = MEDAL_DRUNK_FINISH;
            medal = GetMedalOrder(g_profileIndex, 0);
            if (GetMedals(g_profileIndex) & medal) {
                WinAddItemB(cx, h - 55 - medalRowYAdjust,
                            MedalIconOffsetY(medal), srcY, 0x40, 0x46, "?", g_curWin);
                cx += 80;
                any = 1;
            }
            medal = MEDAL_SPEED_STREAK;
            medal = GetMedalOrder(g_profileIndex, 1);
            if (GetMedals(g_profileIndex) & medal) {
                WinAddItemB(cx, h - 55 - medalRowYAdjust,
                            MedalIconOffsetY(medal), srcY, 0x40, 0x46, "?", g_curWin);
                cx += 80;
                any = 1;
            }

            medal = MEDAL_BONUS_RATIO;
            medal = GetMedalOrder(g_profileIndex, 2);
            if (GetMedals(g_profileIndex) & medal) {
                WinAddItemB(cx, h - 55 - medalRowYAdjust,
                            MedalIconOffsetY(medal), srcY, 0x40, 0x46, "?", g_curWin);
                cx += 80;
                any = 1;
            }

            medal = MEDAL_ALL_LEVELS;
            medal = GetMedalOrder(g_profileIndex, 3);
            if (GetMedals(g_profileIndex) & medal) {
                WinAddItemB(cx, h - 55 - medalRowYAdjust,
                            MedalIconOffsetY(medal), srcY, 0x40, 0x46, "?", g_curWin);
                cx += 80;
                any = 1;
            }
            medal = MEDAL_OVERALL;
            medal = GetMedalOrder(g_profileIndex, 4);
            if (GetMedals(g_profileIndex) & medal) {
                WinAddItemB(cx, h - 55 - medalRowYAdjust,
                            MedalIconOffsetY(medal), srcY, 0x40, 0x46, "?", g_curWin);
                cx += 80;
                any = 1;
            }
            medal = MEDAL_EXACT_MONEY;
            medal = GetMedalOrder(g_profileIndex, 5);
            if (GetMedals(g_profileIndex) & medal) {
                WinAddItemB(cx, h - 55 - medalRowYAdjust,
                            MedalIconOffsetY(medal), srcY, 0x40, 0x46, "?", g_curWin);
                cx += 80;
                any = 1;
            }

            // Sixth planet medal implicitly awards the "found all secrets" medal too.
            if (g_toggleOffCount == 0) {
                if ((GetMedals(g_profileIndex) & MEDAL_OVERALL) == 0) {
                    AwardMedal(g_profileIndex, MEDAL_OVERALL);
                    any = 1;
                }
            }

            // Beyond RANK_GOD, also show a stack of remaining planet icons (one per rank above it).
            if (g_shownRank > RANK_GOD) {
                n = g_shownRank - RANK_GOD;
                size = 52;
                my = h - 55 - rankRowYAdjust;
                for (k = 0; k < n; k++) {
                    mx = w - (w / 2 - n * size / 2) - size;
                    WinAddItemA(mx - k * size, my,
                                size, size, 0, (9 - k) * 128, 128, 128, g_curWin, g_gfxRankPlanets);
                }
            }
        }
    }

    // ---- buttons ----
    if (noButtons) {
        WinAddText(POS_CENTERED, h - 50, g_curWin, "PRESS TAB TO CLOSE WINDOW", 8);
    } else {
        if (!g_newGameOnClose && g_state == STATE_TITLE && !g_profileReadOnly) {
#ifdef __EMSCRIPTEN__
            if (WebCanPlay() && !WebIsGuest() && ProfileValid(g_profileIndex))
                WinAddMenuItem(25, h - 75, g_curWin, 9003, "CONTINUE GAME", 5);
            WinAddMenuItem(25, h - 55, g_curWin, 0x106, WebIsGuest() ? "SIGN IN" : "SIGN OUT", 5);
            WinAddMenuItem(150, h - 55, g_curWin, 0x108, "PLAYLIST", 5);
#else
            WinAddMenuItem(25, h - 55, g_curWin, 0x106, "CLOSE PROFILE", 5);
            if (g_profileIndex == g_cfg.profileSel) {
                WinAddMenuItem(150, h - 55, g_curWin, 0x107, "CLEAR AS DEFAULT", 5);
                WinAddMenuItem(298, h - 55, g_curWin, 0x108, "PLAYLIST", 5);
            } else {
                WinAddMenuItem(150, h - 55, g_curWin, 0x107, "SET AS DEFAULT", 5);
                WinAddMenuItem(283, h - 55, g_curWin, 0x108, "PLAYLIST", 5);
            }
#endif
            WinAddMenuItem(25, h - 35, g_curWin, 0x109, "RESET", 5);
            WinAddMenuItem(82, h - 35, g_curWin, 0x10a, "BACKUP", 5);
            WinAddMenuItem(147, h - 35, g_curWin, 0x10b, "RESTORE", 5);
#ifndef __EMSCRIPTEN__
            WinAddMenuItem(220, h - 35, g_curWin, 0x10c, "CHANGE NAME", 5);
            WinAddMenuItem(325, h - 35, g_curWin, 0x10d, "CHANGE PASSWORD", 5);
#endif
        }
        WinAddMenuItem(w - 80, h - 35, g_curWin, 0xfc, "CLOSE", 5);
        WinSetSelected(g_curWin, 0xfc);
    }
}
#undef UNLOCK_TOGGLE_GE
#undef UNLOCK_TOGGLE_LE
#undef UNLOCK_TOGGLE_GE_OFF

// Applies every profile stat-threshold unlock (start-of-game bonuses, weapon tiers,
// shop items, etc.) to the current player, for a fresh game or Time Trial run.
void ApplyStatUnlocks()
{
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo) {
        if (IsGameCompleted(g_profileIndex))
            g_save.players[g_curPlayer].moneyMax = MAX_MONEY;

        // ---- score-threshold unlocks (non-Time-Trial) ----
        if (g_gameMode != MODE_TIME_TRIAL) {
            ShowHighScore(g_profileIndex);
            if (g_shownStat >= 5000000)
                g_save.players[g_curPlayer].bullets = 10;
            if (g_shownStat >= 7500000)
                g_save.players[g_curPlayer].speed = g_speedStep * 3.0f + g_speedBase;
            if (g_shownStat >= 10000000)
                g_save.players[g_curPlayer].autofire = 1;
            if (g_shownStat >= 20000000 && g_save.players[g_curPlayer].weapon < WEAPON_DOUBLE)
                g_save.players[g_curPlayer].weapon = WEAPON_DOUBLE;

            if (g_shownStat >= 50000000) {
                g_save.players[g_curPlayer].armour =
                    g_shipDefs[g_save.players[g_curPlayer].ship]->baseArmour +
                    g_shipDefs[g_save.players[g_curPlayer].ship]->armourStep;
            }
            if (g_shownStat >= 100000000)
                g_save.players[g_curPlayer].money = MONEY_REWARD_500;
            if (g_shownStat >= 250000000)
                g_save.players[g_curPlayer].money = MONEY_REWARD_1000;
            if (g_shownStat >= 500000000) {
                g_save.players[g_curPlayer].armour =
                    g_shipDefs[g_save.players[g_curPlayer].ship]->baseArmour +
                    g_shipDefs[g_save.players[g_curPlayer].ship]->maxArmourBonus;
            }
            if (g_shownStat >= 1000000000 && g_save.players[g_curPlayer].weapon < WEAPON_TRIPLE)
                g_save.players[g_curPlayer].weapon = WEAPON_TRIPLE;
        }

        // ---- Time-Trial score-threshold unlocks ----
        ShowMeteorStormScore(g_profileIndex);
        if (g_gameMode == MODE_TIME_TRIAL) {
            ShowTimeTrialScore(g_profileIndex);
            if (g_shownStat >= 5000000) {
                g_save.players[g_curPlayer].scoreMult2Timer =
                    g_save.players[g_curPlayer].buffDuration * 1000 + g_time;
                g_save.players[g_curPlayer].scoreMult5Timer = 0;
                g_scoreMul[g_curPlayer] = 2;
            }
            if (g_shownStat >= 6000000) {
                g_save.players[g_curPlayer].scoopTimer =
                    g_save.players[g_curPlayer].buffDuration * 1000 + g_time;
                for (int i = 0; i < 15; i++) {
                    g_scoop[i].pos = 0;
                    g_scoop[i].spawnDelay = i + 5;
                    g_scoop[i].timer = 2;
                }
                g_scoopRange = 0;
            }

            if (g_shownStat >= 7000000) {
                g_save.players[g_curPlayer].scoreMult5Timer =
                    g_save.players[g_curPlayer].buffDuration * 1000 + g_time;
                g_save.players[g_curPlayer].scoreMult2Timer = 0;
                g_scoreMul[g_curPlayer] = 5;
            }
            if (g_shownStat >= 8000000)
                g_save.players[g_curPlayer].autofire = 1;

            if (g_shownStat >= 9000000) {
                g_save.players[g_curPlayer].speed = g_speedStep * 3.0f + g_speedBase;
                g_save.players[g_curPlayer].buffDuration = 30;
            }
            if (g_shownStat >= 10000000)
                g_save.players[g_curPlayer].speed = g_speedStep * 6.0f + g_speedBase;
            if (g_shownStat >= 15000000) {
                g_save.players[g_curPlayer].superAuto = 1;
                g_save.players[g_curPlayer].autofireInterval = 25;
                g_save.players[g_curPlayer].autofire = 1;
            }
            if (g_shownStat >= 17000000)
                g_save.players[g_curPlayer].speed = g_speedStep * g_maxSpeedMul + g_speedBase;
            if (g_shownStat >= 20000000)
                g_timeTrialDeadline = g_time + 241000;
        }

        // ---- secrets-found bonus: found every secret ----
        ShowPerfectCount(g_profileIndex);
        ShowRatio(g_profileIndex);
        ShowPlayTime(g_profileIndex);
        FormatBestTime(g_profileIndex);
        ShowFastestMeteorstorm(g_profileIndex);
        ShowSecretsFound(g_profileIndex);

        if (g_gameMode != MODE_TIME_TRIAL && g_shownStat == g_numLevels) {
            if (g_save.players[g_curPlayer].armour <
                g_shipDefs[g_save.players[g_curPlayer].ship]->baseArmour +
                    g_shipDefs[g_save.players[g_curPlayer].ship]->maxArmourBonus) {
                g_save.players[g_curPlayer].armour =
                    g_shipDefs[g_save.players[g_curPlayer].ship]->baseArmour +
                    g_shipDefs[g_save.players[g_curPlayer].ship]->maxArmourBonus;
            } else
                g_save.players[g_curPlayer].money = MONEY_REWARD_2000;
            if (g_save.players[g_curPlayer].weapon < WEAPON_SUPER_TRIPLE)
                g_save.players[g_curPlayer].weapon = WEAPON_SUPER_TRIPLE;
        }

        // ---- hit-percentage shop-item unlocks ----
        FormatHitPctAbove25(g_profileIndex);
        if (g_gameMode != MODE_TIME_TRIAL) {
            if (g_shownStat >= 70)
                g_shopItems = 84;
            if (g_shownStat >= 80)
                g_shopItems = 85;
            if (g_shownStat >= 90)
                g_shopItems = 86;
        }

        // ---- marathon (level 100) milestone ----
        ShowMarathonScore(g_profileIndex);
        if (g_gameMode != MODE_TIME_TRIAL && g_shownStat >= MARATHON_MILESTONE_SCORE)
            g_save.players[g_curPlayer].highScoreMilestone = 1;

        // ---- levels-played and total-money unlocks ----
        ShowHighestLevelReached(g_profileIndex);
        ShowTotalLevelsPlayed(g_profileIndex);
        if (g_gameMode != MODE_TIME_TRIAL) {
            if (g_shownStat >= 50000 && g_save.players[g_curPlayer].weapon < WEAPON_QUAD)
                g_save.players[g_curPlayer].weapon = WEAPON_QUAD;
            if (g_shownStat >= 75000 && g_save.players[g_curPlayer].bulletSpeedMult < 1.25f)
                g_save.players[g_curPlayer].bulletSpeedMult = 1.25f;
            if (g_shownStat >= 100000) {
                g_save.players[g_curPlayer].speed = g_maxSpeedMul / 2.0f * g_speedStep + g_speedBase;
                g_save.players[g_curPlayer].bullets = 25;
                g_save.players[g_curPlayer].buffDuration = g_timeMax / 2;
                g_save.players[g_curPlayer].money = MONEY_REWARD_5000;
            }
        }

        // ---- max-rank unlock ----
        if (g_gameMode != MODE_TIME_TRIAL) {
            FormatHighestRank(g_profileIndex);
            if (g_shownRank == MAX_RANK) {
                g_save.players[g_curPlayer].buffDuration = g_timeMax;
                g_save.players[g_curPlayer].money = MONEY_REWARD_25000;
                g_save.players[g_curPlayer].bulletSpeedMult = 1.5f;
                g_save.players[g_curPlayer].weapon = WEAPON_WAR_PLASMA;
                g_save.players[g_curPlayer].alienLock = 1;
                g_save.players[g_curPlayer].superAuto = 1;
                g_save.players[g_curPlayer].autofireInterval = 25;
                g_save.players[g_curPlayer].autofire = 1;
            }
        }

        ShowTotalGamesPlayed(g_profileIndex);
    }
}

// Always-true stub for a stat-snapshot related callback.
int StatSnapshotStub()
{
    return 1;
}
