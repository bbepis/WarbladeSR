// Tests for src/profile/stats.c: the per-profile statistics getters/updaters and their text
// output. Profiles live packed in g_accBuf[slot]; each test sets one up directly with
// Reset (ResetAccount) + edits to g_acc + PackAccount, and reads it back with UnpackAccount.
#include <stdlib.h>
#include <sys/stat.h>
#include "support.h"

static Account s_got;

// ResetAccount with the RNG seeded (MakeRandomId loops forever on an all-zero xorshift state).
static void Reset(void)
{
    if (g_rngW == 0) {
        SeedRand(1234);
        g_rngT = 0x1111;
        g_rngW = 0x2222;
    }
    ResetAccount();
}

// The packed profile `slot`, unpacked into a copy.
static const Account *Load(int slot)
{
    UnpackAccount(slot);
    s_got = g_acc;
    ClearAccount();
    return &s_got;
}

// A brand-new profile (ResetAccount's defaults) in `slot`.
static void Fresh(int slot)
{
    Reset();
    PackAccount(slot);
}

#define TICKS_PER_SEC 10000000LL   // FILETIME units (100 ns)

// ---------------------------------------------------------------------------------------
// High scores
// ---------------------------------------------------------------------------------------

TEST(Stats_UpdateHighScore_keeps_the_best)
{
    Fresh(0);
    UpdateHighScore(0, 1000);
    CHECK_EQ_INT(Load(0)->highScore, 1000);
    UpdateHighScore(0, 999);
    CHECK_EQ_INT(Load(0)->highScore, 1000);
    UpdateHighScore(0, 5000000000LL);
    CHECK_EQ_INT(Load(0)->highScore, 5000000000LL);
}

TEST(Stats_UpdateHighScore_saves_the_profile_file)
{
    Fresh(2);
    mkdir(FakeUserPath("warblade"), 0755);   // made at start-up in the game
    const char *path = FakeUserPath("warblade\\profiles\\profile002.acc");
    CHECK(!FakeFileExists(path));
    UpdateHighScore(2, 777);
    CHECK(FakeFileExists(path));

    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    size_t n = fread(&g_accBuf[5], 1, sizeof(Account), f);
    fclose(f);
    CHECK_EQ_INT(n, sizeof(Account));
    CHECK_EQ_INT(Load(5)->highScore, 777);
}

TEST(Stats_ShowHighScore_formats_with_dot_groups)
{
    Reset();
    g_acc.highScore = 1234567;
    PackAccount(0);
    ShowHighScore(0);
    CHECK_STR(g_logBuf, "1.234.567");
    CHECK_EQ_INT(g_shownStat, 1234567);

    strcpy(g_logBuf, "untouched");
    ShowHighScore(-1);
    CHECK_STR(g_logBuf, "untouched");
}

TEST(Stats_UpdateMarathonScore_keeps_the_best_and_flags_the_milestone)
{
    Fresh(0);
    g_curPlayer = 1;
    UpdateMarathonScore(0, 199999999);
    CHECK_EQ_INT(Load(0)->level100HighScore, 199999999);
    CHECK_EQ_INT(g_save.players[1].highScoreMilestone, 0);
    UpdateMarathonScore(0, 150);
    CHECK_EQ_INT(Load(0)->level100HighScore, 199999999);
    UpdateMarathonScore(0, 200000000);
    CHECK_EQ_INT(Load(0)->level100HighScore, 200000000);
    CHECK_EQ_INT(g_save.players[1].highScoreMilestone, 1);
    CHECK_EQ_INT(g_save.players[0].highScoreMilestone, 0);
}

TEST(Stats_marathon_milestone_is_not_flagged_in_time_trial)
{
    Fresh(0);
    g_gameMode = MODE_TIME_TRIAL;
    UpdateMarathonScore(0, 300000000);
    CHECK_EQ_INT(Load(0)->level100HighScore, 300000000);
    CHECK_EQ_INT(g_save.players[0].highScoreMilestone, 0);
    ShowMarathonScore(0);
    CHECK_STR(g_logBuf, "300.000.000");
    CHECK_EQ_INT(g_save.players[0].highScoreMilestone, 0);
}

TEST(Stats_ShowMarathonScore_formats_and_flags_the_milestone)
{
    Reset();
    g_acc.level100HighScore = 199999999;
    PackAccount(0);
    ShowMarathonScore(0);
    CHECK_STR(g_logBuf, "199.999.999");
    CHECK_EQ_INT(g_shownStat, 199999999);
    CHECK_EQ_INT(g_save.players[0].highScoreMilestone, 0);

    Reset();
    g_acc.level100HighScore = 200000000;
    PackAccount(0);
    ShowMarathonScore(0);
    CHECK_EQ_INT(g_save.players[0].highScoreMilestone, 1);
}

TEST(Stats_MeteorStorm_score_keeps_the_best)
{
    Fresh(0);
    UpdateMeteorStormScore(0, 4500000);
    UpdateMeteorStormScore(0, 300);
    CHECK_EQ_INT(Load(0)->meteorstormHighScore, 4500000);
    ShowMeteorStormScore(0);
    CHECK_STR(g_logBuf, "4.500.000");
    UpdateMeteorStormScore(0, 4500001);
    CHECK_EQ_INT(Load(0)->meteorstormHighScore, 4500001);
}

TEST(Stats_TimeTrial_score_keeps_the_best)
{
    Fresh(0);
    UpdateTimeTrialScore(0, 8000000);
    UpdateTimeTrialScore(0, 7999999);
    CHECK_EQ_INT(Load(0)->timeTrialHighScore, 8000000);
    ShowTimeTrialScore(0);
    CHECK_STR(g_logBuf, "8.000.000");
    CHECK_EQ_INT(g_shownStat, 8000000);
    UpdateTimeTrialScore(0, 9000000);
    CHECK_EQ_INT(Load(0)->timeTrialHighScore, 9000000);
}

// ---------------------------------------------------------------------------------------
// Bonus levels
// ---------------------------------------------------------------------------------------

TEST(Stats_AddStats_accumulates_and_resets_session_counters)
{
    Fresh(0);
    g_perfectCount = 5;
    g_killCount = 7;
    AddStats(0, 2, 3);
    CHECK_EQ_INT(g_perfectCount, 0);
    CHECK_EQ_INT(g_killCount, 0);
    AddStats(0, 1, 4);
    CHECK_EQ_INT(Load(0)->perfectBonusLevels, 3);
    CHECK_EQ_INT(Load(0)->bonusLevelsPlayed, 7);

    ShowPerfectAttempts(0);
    CHECK_STR(g_logBuf, "7");
    CHECK_EQ_INT(g_shownStat, 7);
    ShowPerfectCount(0);
    CHECK_STR(g_logBuf, "3");
    CHECK_EQ_INT(g_shownStat, 3);
}

TEST(Stats_ShowRatio_is_a_truncated_percentage)
{
    Reset();
    g_acc.perfectBonusLevels = 2;
    g_acc.bonusLevelsPlayed = 3;
    PackAccount(0);
    ShowRatio(0);
    CHECK_STR(g_logBuf, "66 %");

    Fresh(1);
    ShowRatio(1);
    CHECK_STR(g_logBuf, "0 %");

    ShowRatio(-1);
    CHECK_STR(g_logBuf, "");
}

// ---------------------------------------------------------------------------------------
// Play time
// ---------------------------------------------------------------------------------------

static void PlayTimeIs(__int64 secs)
{
    Reset();
    g_acc.playTime = secs * TICKS_PER_SEC;
    PackAccount(0);
    ShowPlayTime(0);
}

TEST(Stats_ShowPlayTime_shows_only_the_coarsest_units)
{
    PlayTimeIs(86400 + 2 * 3600 + 3 * 60 + 4);
    CHECK_STR(g_logBuf, "1D, 2H, 3M, 4SEC");
    PlayTimeIs(12 * 3600 + 34 * 60 + 56);
    CHECK_STR(g_logBuf, "12H, 34M, 56SEC");
    PlayTimeIs(3600 + 2 * 60 + 3);
    CHECK_STR(g_logBuf, " 1H,  2M,  3SEC");
    PlayTimeIs(5 * 60 + 7);
    CHECK_STR(g_logBuf, " 5 M,  7 SEC");
    PlayTimeIs(9);
    CHECK_STR(g_logBuf, " 9 SECONDS");
    PlayTimeIs(0);
    CHECK_STR(g_logBuf, " 0 SECONDS");

    ShowPlayTime(-1);
    CHECK_STR(g_logBuf, "");
}

// The day count is (month - 1) * 30 + day - 1 of the FILETIME date: every month counts as
// 30 days, so 31 days of play show as 30 (current behaviour).
TEST(Stats_ShowPlayTime_counts_months_as_30_days)
{
    PlayTimeIs(31 * 86400);
    CHECK_STR(g_logBuf, "30D, 0H, 0M, 0SEC");
    PlayTimeIs(30 * 86400);
    CHECK_STR(g_logBuf, "30D, 0H, 0M, 0SEC");
}

TEST(Stats_AddPlayTime_adds_the_net_time_clamped_at_zero)
{
    Fresh(0);
    CHECK(!g_profilePlayTimeAdded);
    AddPlayTime(0, 100, 30, 20);
    CHECK(g_profilePlayTimeAdded);
    CHECK_EQ_INT(Load(0)->playTime, 50);
    AddPlayTime(0, 10, 100, 0);
    CHECK_EQ_INT(Load(0)->playTime, 0);
}

// ---------------------------------------------------------------------------------------
// Ranks
// ---------------------------------------------------------------------------------------

TEST(Stats_SetStat_and_GetStat_are_the_unlocked_rank)
{
    Fresh(0);
    CHECK_EQ_INT(GetStat(0), RANK_GRANDMASTER_3);
    SetStat(0, 25);
    CHECK_EQ_INT(GetStat(0), 25);
    CHECK_EQ_INT(Load(0)->unlockedRank, 25);
    CHECK_EQ_INT(GetStat(-1), 20);
}

TEST(Stats_IncrementRank_caps_at_3)
{
    Fresh(0);
    CHECK_EQ_INT(GetRank(0), 0);
    IncrementRank(0);
    CHECK_EQ_INT(GetRank(0), 1);
    IncrementRank(0);
    IncrementRank(0);
    CHECK_EQ_INT(GetRank(0), 3);
    IncrementRank(0);
    CHECK_EQ_INT(GetRank(0), 3);
    CHECK_EQ_INT(GetRank(-1), 0);
}

TEST(Stats_FormatHighestRank_shows_the_rank_name)
{
    Reset();
    g_acc.highestRank = RANK_ADMIRAL_1_1;
    PackAccount(0);
    FormatHighestRank(0);
    CHECK_STR(g_logBuf, "ADMIRAL 1 BRONZE STAR");
    CHECK_EQ_INT(g_shownRank, RANK_ADMIRAL_1_1);
}

TEST(Stats_FormatHighestRank_drops_a_rank_above_the_unlocked_one)
{
    Reset();
    g_acc.highestRank = 25;
    g_acc.unlockedRank = 20;
    PackAccount(0);
    FormatHighestRank(0);
    CHECK_STR(g_logBuf, "ENSIGN");
    CHECK_EQ_INT(g_shownRank, 0);
    CHECK_EQ_INT(Load(0)->highestRank, 0);

    Reset();
    g_acc.highestRank = 20;
    g_acc.unlockedRank = 20;
    PackAccount(0);
    FormatHighestRank(0);
    CHECK_STR(g_logBuf, "WARBLADE GRANDMASTER 3 GOLD STARS");
    CHECK_EQ_INT(Load(0)->highestRank, 20);
}

TEST(Stats_FormatHighestRank_masks_god_ranks_on_read_only_profiles)
{
    Reset();
    g_acc.highestRank = RANK_GOD;
    g_acc.unlockedRank = MAX_RANK;
    PackAccount(0);
    g_profileReadOnly = 1;
    FormatHighestRank(0);
    CHECK_STR(g_logBuf, "???");
    CHECK_EQ_INT(g_shownRank, RANK_GOD);

    Reset();
    g_acc.highestRank = RANK_CHAMPION;
    g_acc.unlockedRank = MAX_RANK;
    PackAccount(1);
    FormatHighestRank(1);
    CHECK_STR(g_logBuf, "WARBLADE CHAMPION");

    g_profileReadOnly = 0;
    FormatHighestRank(0);
    CHECK_STR(g_logBuf, "WARBLADE GOD");
}

TEST(Stats_UpdateHighestRank_keeps_the_best)
{
    Fresh(0);
    UpdateHighestRank(0, 7);
    UpdateHighestRank(0, 3);
    CHECK_EQ_INT(Load(0)->highestRank, 7);
    UpdateHighestRank(0, 8);
    CHECK_EQ_INT(Load(0)->highestRank, 8);
}

// ---------------------------------------------------------------------------------------
// Level times
// ---------------------------------------------------------------------------------------

static void BestTimeIs(__int64 ticks)
{
    Reset();
    g_acc.bestLevelTime = ticks;
    PackAccount(0);
    FormatBestTime(0);
}

TEST(Stats_FormatBestTime_formats_seconds_and_minutes)
{
    g_timeMax = 30;
    BestTimeIs(25000000);           // 2.5 s
    CHECK_STR(g_logBuf, "2.500 SEC");
    CHECK_EQ_INT(g_shownStat, 2500);
    CHECK_EQ_INT(g_timeMax, 30);

    BestTimeIs(9999999998LL);        // 16 min 39.999 s, just under the sentinel
    CHECK_STR(g_logBuf, "16 M, 39.999 SEC");
}

TEST(Stats_FormatBestTime_without_a_time_shows_dashes)
{
    g_timeMax = 30;
    BestTimeIs(9999999999LL);
    CHECK_STR(g_logBuf, "--.--- SEC");
    CHECK_EQ_INT(g_shownStat, 9999999999LL);
    CHECK_EQ_INT(g_timeMax, 30);

    strcpy(g_logBuf, "x");
    g_shownStat = 5;
    FormatBestTime(-1);
    CHECK_STR(g_logBuf, "x");
    CHECK_EQ_INT(g_shownStat, 5);
}

// ResetAccount's "no time" sentinel (NO_TIME_RECORDED, 999999999) is below FormatBestTime's
// (9999999999), so a new profile shows 99.9999999 s as a time (current behaviour).
TEST(Stats_FormatBestTime_shows_a_new_profiles_sentinel_as_a_time)
{
    Fresh(0);
    FormatBestTime(0);
    CHECK_STR(g_logBuf, "1 M, 39.999 SEC");
    CHECK_EQ_INT(g_shownStat, 39999);
}

TEST(Stats_FormatBestTime_raises_max_extra_time_for_fast_clears)
{
    g_timeMax = 30;
    BestTimeIs(20000000);           // 2.0 s
    CHECK_STR(g_logBuf, "2.00 SEC");
    CHECK_EQ_INT(g_timeMax, 60);
    BestTimeIs(15000000);           // 1.5 s
    CHECK_STR(g_logBuf, "1.500 SEC");
    CHECK_EQ_INT(g_shownStat, 1500);
    CHECK_EQ_INT(g_timeMax, 60);
    BestTimeIs(10000000);           // 1.0 s
    CHECK_EQ_INT(g_timeMax, 90);
    g_timeMax = 30;
    BestTimeIs(8000000);            // 0.8 s
    CHECK_STR(g_logBuf, "0.800 SEC");
    CHECK_EQ_INT(g_timeMax, 90);
}

TEST(Stats_FormatBestTime_leaves_max_extra_time_alone_in_time_trial)
{
    g_gameMode = MODE_TIME_TRIAL;
    g_timeMax = 30;
    BestTimeIs(8000000);
    CHECK_STR(g_logBuf, "0.800 SEC");
    CHECK_EQ_INT(g_timeMax, 30);
}

// The milliseconds are printed with %02d (3.045 s shows as "3.45"), and g_shownStat, the
// value the unlocks compare, drops the minutes: 1 min 0.5 s counts as 0.5 s (current
// behaviour).
TEST(Stats_FormatBestTime_quirks_milliseconds_and_minutes)
{
    g_timeMax = 30;
    BestTimeIs((2 * 60 * 1000 + 3045) * 10000LL);
    CHECK_STR(g_logBuf, "2 M, 3.45 SEC");
    CHECK_EQ_INT(g_shownStat, 3045);
    CHECK_EQ_INT(g_timeMax, 30);

    BestTimeIs((60 * 1000 + 500) * 10000LL);
    CHECK_STR(g_logBuf, "1 M, 0.500 SEC");
    CHECK_EQ_INT(g_shownStat, 500);
    CHECK_EQ_INT(g_timeMax, 90);
}

TEST(Stats_UpdateBestTime_only_takes_faster_valid_times)
{
    Fresh(0);
    UpdateBestTime(0, 20000000);
    CHECK_EQ_INT(Load(0)->bestLevelTime, 20000000);
    UpdateBestTime(0, 30000000);
    CHECK_EQ_INT(Load(0)->bestLevelTime, 20000000);
    UpdateBestTime(0, 0);
    CHECK_EQ_INT(Load(0)->bestLevelTime, 20000000);
    UpdateBestTime(0, -5);
    CHECK_EQ_INT(Load(0)->bestLevelTime, 20000000);
    UpdateBestTime(0, 19999999);
    CHECK_EQ_INT(Load(0)->bestLevelTime, 19999999);

    Reset();
    g_acc.bestLevelTime = 99999999999LL;
    PackAccount(1);
    UpdateBestTime(1, 9999999999LL);
    CHECK_EQ_INT(Load(1)->bestLevelTime, 99999999999LL);
    UpdateBestTime(1, 9999999998LL);
    CHECK_EQ_INT(Load(1)->bestLevelTime, 9999999998LL);
}

static void MeteorTimeIs(__int64 ticks)
{
    Reset();
    g_acc.bestMeteorstormTime = ticks;
    PackAccount(0);
    ShowFastestMeteorstorm(0);
}

TEST(Stats_ShowFastestMeteorstorm_formats_the_time)
{
    MeteorTimeIs(75500000);         // 7.55 s
    CHECK_STR(g_logBuf, "7.550 SEC");
    CHECK_EQ_INT(g_shownStat, 7550);
    MeteorTimeIs(125250 * 10000LL); // 2 min 5.25 s
    CHECK_STR(g_logBuf, "2 M, 5.250 SEC");
    CHECK_EQ_INT(g_shownStat, 125250);
    MeteorTimeIs((14 * 60 * 1000 + 59999) * 10000LL);
    CHECK_STR(g_logBuf, "14 M, 59.999 SEC");
    MeteorTimeIs(15 * 60 * TICKS_PER_SEC);
    CHECK_STR(g_logBuf, "--.--- SEC");
    CHECK_EQ_INT(g_shownStat, 15 * 60 * 1000);
}

TEST(Stats_UpdateFastestMeteorStorm_keeps_the_fastest)
{
    Fresh(0);
    UpdateFastestMeteorStorm(0, 500000000);
    CHECK_EQ_INT(Load(0)->bestMeteorstormTime, 500000000);
    UpdateFastestMeteorStorm(0, 600000000);
    CHECK_EQ_INT(Load(0)->bestMeteorstormTime, 500000000);
    UpdateFastestMeteorStorm(0, 400000000);
    CHECK_EQ_INT(Load(0)->bestMeteorstormTime, 400000000);
}

// ---------------------------------------------------------------------------------------
// Shots, levels, games, money
// ---------------------------------------------------------------------------------------

TEST(Stats_shot_counters_and_FormatTotalHitPct)
{
    Fresh(0);
    FormatTotalHitPct(0);
    CHECK_STR(g_logBuf, "0 %");
    AddScoreStat(0, 40);
    AddScoreStat(0, 40);
    AddHitsStat(0, 30);
    CHECK_EQ_INT(Load(0)->shotsFired, 80);
    CHECK_EQ_INT(Load(0)->shotsHit, 30);
    FormatTotalHitPct(0);
    CHECK_STR(g_logBuf, "37 %");
}

TEST(Stats_HitPctAbove25_keeps_the_best)
{
    Fresh(0);
    UpdateHitPctAbove25(0, 72);
    UpdateHitPctAbove25(0, 60);
    FormatHitPctAbove25(0);
    CHECK_STR(g_logBuf, "72 %");
    CHECK_EQ_INT(g_shownStat, 72);
    UpdateHitPctAbove25(0, 73);
    CHECK_EQ_INT(Load(0)->bestHitPctAbove25, 73);
}

TEST(Stats_highest_level_keeps_the_best)
{
    Fresh(0);
    UpdateHighestLevel(0, 42);
    UpdateHighestLevel(0, 12);
    ShowHighestLevelReached(0);
    CHECK_STR(g_logBuf, "42");
    CHECK_EQ_INT(g_shownStat, 42);
    UpdateHighestLevel(0, 43);
    CHECK_EQ_INT(Load(0)->highestLevel, 43);
}

TEST(Stats_AddLevelsPlayed_accumulates)
{
    Fresh(0);
    AddLevelsPlayed(0, 12);
    AddLevelsPlayed(0, 30);
    CHECK_EQ_INT(Load(0)->totalLevelsPlayed, 42);
    ShowTotalLevelsPlayed(0);
    CHECK_STR(g_logBuf, "42");
    CHECK_EQ_INT(g_shownStat, 42);
}

static void LevelsPlayedIs(int n)
{
    memset(&g_save.players[g_curPlayer], 0, sizeof g_save.players[0]);
    g_bonusWeight[12] = g_bonusWeight[13] = g_bonusWeight[14] = 80;
    Reset();
    g_acc.totalLevelsPlayed = n;
    PackAccount(0);
    ShowTotalLevelsPlayed(0);
}

#define P0 g_save.players[g_curPlayer]

TEST(Stats_ShowTotalLevelsPlayed_unlocks_by_threshold)
{
    g_curPlayer = 1;
    LevelsPlayedIs(999);
    CHECK_EQ_INT(P0.autofireUnlocked, 0);

    LevelsPlayedIs(1000);
    CHECK_EQ_INT(P0.autofireUnlocked, 1);
    CHECK_EQ_INT(P0.turretTrackingReduction, 0);

    LevelsPlayedIs(2500);
    CHECK_EQ_INT(P0.turretTrackingReduction, 25);
    CHECK_EQ_INT(P0.gemCounterUnlocked, 0);

    LevelsPlayedIs(5000);
    CHECK_EQ_INT(P0.gemCounterUnlocked, 1);
    CHECK_EQ_INT(g_bonusWeight[12], 80);

    LevelsPlayedIs(10000);
    CHECK_EQ_INT(g_bonusWeight[12], 0);
    CHECK_EQ_INT(g_bonusWeight[13], 80);

    LevelsPlayedIs(15000);
    CHECK_EQ_INT(g_bonusWeight[13], 0);
    CHECK_EQ_INT(P0.blueMoneyUnlocked, 0);

    LevelsPlayedIs(20000);
    CHECK_EQ_INT(P0.blueMoneyUnlocked, 1);
    CHECK_EQ_INT(g_bonusWeight[14], 80);

    LevelsPlayedIs(25000);
    CHECK_EQ_INT(g_bonusWeight[14], 0);
    CHECK_EQ_INT(P0.multiplierUnlocked, 0);

    LevelsPlayedIs(35000);
    CHECK_EQ_INT(P0.multiplierUnlocked, 1);
    CHECK_EQ_INT(P0.autofireUnlocked, 1);
    CHECK_EQ_INT(g_save.players[0].autofireUnlocked, 0);
}

TEST(Stats_ShowTotalLevelsPlayed_unlocks_nothing_in_time_trial)
{
    g_gameMode = MODE_TIME_TRIAL;
    LevelsPlayedIs(40000);
    CHECK_STR(g_logBuf, "40000");
    CHECK_EQ_INT(P0.autofireUnlocked, 0);
    CHECK_EQ_INT(P0.multiplierUnlocked, 0);
    CHECK_EQ_INT(g_bonusWeight[12], 80);
}

TEST(Stats_IncrementGamesPlayed_adds_the_pending_game)
{
    Fresh(0);
    g_newGamePending = 1;
    IncrementGamesPlayed(0);
    CHECK_EQ_INT(g_newGamePending, 0);
    CHECK_EQ_INT(Load(0)->gamesPlayed, 1);
    IncrementGamesPlayed(0);
    CHECK_EQ_INT(Load(0)->gamesPlayed, 1);
    g_newGamePending = 1;
    IncrementGamesPlayed(0);
    ShowTotalGamesPlayed(0);
    CHECK_STR(g_logBuf, "2");
    CHECK_EQ_INT(g_shownStat, 2);
}

TEST(Stats_highest_money_keeps_the_best)
{
    Fresh(0);
    UpdateHighestMoney(0, 123456);
    UpdateHighestMoney(0, 1000);
    FormatHighestMoney(0);
    CHECK_STR(g_logBuf, "123456");
    CHECK_EQ_INT(g_shownStat, 123456);
    UpdateHighestMoney(0, 123457);
    CHECK_NEAR(Load(0)->highestMoney, 123457.0, 0);
}

TEST(Stats_FormatHighestMoney_preserves_64_bit_amounts)
{
    Reset();
    g_acc.highestMoney = 7500000000.0;
    PackAccount(0);
    FormatHighestMoney(0);
    CHECK_STR(g_logBuf, "7500000000");
    CHECK_EQ_INT(g_shownStat, 7500000000LL);
}

TEST(Stats_FormatHighestMoney_masks_big_amounts_on_read_only_profiles)
{
    g_profileReadOnly = 1;
    Reset();
    g_acc.highestMoney = 100000;
    PackAccount(0);
    FormatHighestMoney(0);
    CHECK_STR(g_logBuf, "???");
    Reset();
    g_acc.highestMoney = 99999;
    PackAccount(0);
    FormatHighestMoney(0);
    CHECK_STR(g_logBuf, "99999");
}

// ---------------------------------------------------------------------------------------
// Secrets and completion
// ---------------------------------------------------------------------------------------

TEST(Stats_ShowSecretsFound_counts_found_levels)
{
    Reset();
    g_acc.levelDone[0] = 1;
    g_acc.levelDone[17] = 1;
    g_acc.levelDone[29] = 1;
    g_acc.levelDone[35] = 1;
    g_acc.secretsInOneGame = 7;
    PackAccount(0);
    ShowSecretsFound(0);
    CHECK_STR(g_logBuf, "3 OUT OF 30");
    CHECK_EQ_INT(g_shownStat, 3);
    g_numLevels = 40;
    ShowSecretsFound(0);
    CHECK_STR(g_logBuf, "4 OUT OF 40");
    ShowSecretsInOneGame(0);
    CHECK_STR(g_logBuf, "7");
    CHECK_EQ_INT(g_shownStat, 7);
}

TEST(Stats_SetGameCompleted_marks_the_profile)
{
    Fresh(0);
    CHECK(!IsGameCompleted(0));
    SetGameCompleted(0);
    CHECK(IsGameCompleted(0));
    CHECK(!IsGameCompleted(-1));
}

TEST(Stats_MarkSecretFound_marks_the_level_and_counts_this_game)
{
    Fresh(0);
    g_profileIndex = 0;
    g_save.players[0].secretFlags[0] = 1;
    g_save.players[0].secretFlags[4] = 1;
    g_save.players[0].secretFlags[9] = 1;
    MarkSecretFound(0, 5);
    CHECK(IsSecretFound(0, 5));
    CHECK(!IsSecretFound(0, 4));
    CHECK(!IsSecretFound(0, 6));
    CHECK(!IsSecretFound(-1, 5));
    CHECK_EQ_INT(Load(0)->levelDone[4], 1);
    CHECK_EQ_INT(g_save.players[0].secretCount, 3);
    CHECK_EQ_INT(Load(0)->secretsInOneGame, 3);
    CHECK_EQ_INT(g_save.players[0].score, 0);
}

TEST(Stats_MarkSecretFound_keeps_the_best_game_count)
{
    Reset();
    g_acc.secretsInOneGame = 5;
    PackAccount(0);
    g_save.players[0].secretFlags[0] = 1;
    MarkSecretFound(0, 1);
    CHECK_EQ_INT(g_save.players[0].secretCount, 1);
    CHECK_EQ_INT(Load(0)->secretsInOneGame, 5);
}

TEST(Stats_MarkSecretFound_resets_an_impossible_game_count)
{
    g_numLevels = 3;
    Reset();
    g_acc.secretsInOneGame = 10;
    PackAccount(0);
    g_save.players[0].secretFlags[0] = 1;
    MarkSecretFound(0, 1);
    CHECK_EQ_INT(Load(0)->secretsInOneGame, 0);
}

TEST(Stats_MarkSecretFound_plays_the_jingle_only_the_first_time)
{
    g_soundEnabled = 1;
    g_cfg.sfxOn = 1;
    g_sampleSecret = 77;
    g_time = 1000;
    Fresh(0);
    MarkSecretFound(0, 2);
    CHECK_EQ_INT(g_soundQueueCount, 1);
    CHECK_EQ_INT(g_soundQueue[0].sample, 77);
    g_soundQueueCount = 0;
    g_soundQueueNext = 0;
    MarkSecretFound(0, 2);
    CHECK_EQ_INT(g_soundQueueCount, 0);
}

// Three levels; the first two secrets were found before, the third one now, and this game
// has found all three.
static void AllSecretsSetUp(bool thirdFoundBefore)
{
    g_numLevels = 3;
    g_profileIndex = 0;
    g_scoreMul[0] = 2;
    g_time = 5000;
    Reset();
    g_acc.levelDone[0] = g_acc.levelDone[1] = 1;
    g_acc.levelDone[2] = thirdFoundBefore;
    g_acc.secretsInOneGame = 2;
    PackAccount(0);
    for (int i = 0; i < 3; i++)
        g_save.players[0].secretFlags[i] = 1;
    MarkSecretFound(0, 3);
}

TEST(Stats_MarkSecretFound_rewards_all_secrets_in_one_game)
{
    AllSecretsSetUp(false);
    CHECK_EQ_INT(Load(0)->secretsInOneGame, 3);
    CHECK_EQ_INT(g_save.players[0].score, 500000000);
    CHECK_EQ_INT(g_msgColor, 8);
    CHECK_EQ_INT(g_msgTimer, 12000);
    CHECK_EQ_INT(g_popups[0].active, 1);
    CHECK_EQ_INT(g_popups[0].value, 250000000);
    // ...and CheckAllLevelsMedal awards the all-levels medal
    CHECK_EQ_INT(GetMedals(0), MEDAL_ALL_LEVELS);
}

TEST(Stats_MarkSecretFound_no_reward_when_the_last_secret_was_known)
{
    AllSecretsSetUp(true);
    CHECK_EQ_INT(Load(0)->secretsInOneGame, 3);
    CHECK_EQ_INT(g_save.players[0].score, 0);
    CHECK_EQ_INT(g_msgTimer, 0);
}
