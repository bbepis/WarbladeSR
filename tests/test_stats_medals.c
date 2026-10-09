// Tests for src/profile/stats.c: medals (AwardMedal and its planet-rank milestones, the
// ratio/all-levels medal checks, medal order) and the profile resets around them.
#include "support.h"

static Account s_got;

// ResetAccount with the RNG seeded (MakeRandomId loops forever on an all-zero xorshift state).
static void Reset(void)
{
    if (g_rngW == 0) {
        SeedRand(4321);
        g_rngT = 0x3333;
        g_rngW = 0x4444;
    }
    ResetAccount();
}

static const Account *Load(int slot)
{
    UnpackAccount(slot);
    s_got = g_acc;
    ClearAccount();
    return &s_got;
}

static void Fresh(int slot)
{
    Reset();
    PackAccount(slot);
}

// Sound queue: distinct fake handles for the samples AwardMedal queues.
static void EnableSounds(void)
{
    g_soundEnabled = 1;
    g_cfg.sfxOn = 1;
    g_sfxNew = 101;
    g_sfxPlanet = 102;
    g_sfxRank = 103;
    g_sfxAvailable = 104;
    g_soundQueueCount = 0;
    g_soundQueueNext = 0;
}

// The queued samples must be `samples` in order, each followed by `delays[i]` ms (every fake
// sample is 1000 ms long).
static void CheckQueue(int n, const AudioHandle *samples, const int *delays)
{
    CHECK_EQ_INT(g_soundQueueCount, n);
    for (int i = 0; i < n; i++) {
        CHECK_EQ_INT(g_soundQueue[i].sample, samples[i]);
        unsigned next = i + 1 < n ? g_soundQueue[i + 1].time : g_soundQueueNext;
        CHECK_EQ_INT(next - g_soundQueue[i].time - 1000, delays[i]);
    }
}

// ---------------------------------------------------------------------------------------
// Award order
// ---------------------------------------------------------------------------------------

TEST(Stats_AwardMedal_records_the_award_order_once_per_medal)
{
    Fresh(0);
    AwardMedal(0, MEDAL_ALL_LEVELS);
    AwardMedal(0, MEDAL_SPEED_STREAK);
    AwardMedal(0, MEDAL_ALL_LEVELS);
    AwardMedal(0, MEDAL_EXACT_MONEY);
    AwardMedal(0, 0);
    CHECK_EQ_INT(GetMedals(0), MEDAL_ALL_LEVELS | MEDAL_SPEED_STREAK | MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(GetMedalOrder(0, 0), MEDAL_ALL_LEVELS);
    CHECK_EQ_INT(GetMedalOrder(0, 1), MEDAL_SPEED_STREAK);
    CHECK_EQ_INT(GetMedalOrder(0, 2), MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(GetMedalOrder(0, 3), 0);
    CHECK_EQ_INT(Load(0)->medalOrder[6], 3);
    CHECK_EQ_INT(GetMedalOrder(-1, 0), 0);
    CHECK_EQ_INT(GetMedals(-1), 0);
}

TEST(Stats_AwardMedal_orders_all_six_medals)
{
    static const int order[6] = {MEDAL_EXACT_MONEY, MEDAL_OVERALL, MEDAL_DRUNK_FINISH,
                                 MEDAL_BONUS_RATIO, MEDAL_SPEED_STREAK, MEDAL_ALL_LEVELS};
    Fresh(0);
    for (int i = 0; i < 6; i++)
        AwardMedal(0, order[i]);
    for (int i = 0; i < 6; i++)
        CHECK_EQ_INT(GetMedalOrder(0, i), order[i]);
    CHECK_EQ_INT(GetMedalOrder(0, 6), 0);
    CHECK_EQ_INT(Load(0)->medalOrder[6], 6);
    CHECK_EQ_INT(GetMedals(0), MEDALS_ALL);
    // at completion rank 0 nothing else happens
    CHECK_EQ_INT(Load(0)->completionRank, 0);
    CHECK_EQ_INT(Load(0)->unlockedRank, RANK_GRANDMASTER_3);
}

TEST(Stats_HasAllMedals_needs_the_six_planet_medals)
{
    Reset();
    g_acc.medals = 0x1f;
    PackAccount(0);
    CHECK(!HasAllMedals(0));
    Reset();
    g_acc.medals = 0x3e;
    PackAccount(0);
    CHECK(!HasAllMedals(0));
    Reset();
    g_acc.medals = 0x3f;
    PackAccount(0);
    CHECK(HasAllMedals(0));
    Reset();
    g_acc.medals = 0x7f;
    PackAccount(0);
    CHECK(HasAllMedals(0));
    CHECK(!HasAllMedals(-1));
}

TEST(Stats_ClearMedalOrder_keeps_the_medals)
{
    Fresh(0);
    AwardMedal(0, MEDAL_OVERALL);
    AwardMedal(0, MEDAL_DRUNK_FINISH);
    ClearMedalOrder(0);
    CHECK_EQ_INT(GetMedalOrder(0, 0), 0);
    CHECK_EQ_INT(GetMedalOrder(0, 1), 0);
    CHECK_EQ_INT(Load(0)->medalOrder[6], 0);
    CHECK_EQ_INT(GetMedals(0), MEDAL_OVERALL | MEDAL_DRUNK_FINISH);
}

TEST(Stats_ClearMedalsMask_clears_only_those_medals)
{
    Reset();
    g_acc.medals = MEDALS_ALL;
    g_acc.medalStep = MEDAL_ALL_LEVELS;
    PackAccount(0);
    ClearMedalsMask(0, MEDAL_DRUNK_FINISH | MEDAL_BONUS_RATIO);
    CHECK_EQ_INT(GetMedals(0), 0x3a);
    CHECK_EQ_INT(Load(0)->medalStep, -1);
}

TEST(Stats_ClearLevelsDone_clears_the_game_levels)
{
    Reset();
    g_acc.levelDone[0] = 1;
    g_acc.levelDone[29] = 1;
    g_acc.levelDone[30] = 1;
    PackAccount(0);
    ClearLevelsDone(0);
    CHECK_EQ_INT(Load(0)->levelDone[0], 0);
    CHECK_EQ_INT(Load(0)->levelDone[29], 0);
    CHECK_EQ_INT(Load(0)->levelDone[30], 1);
}

TEST(Stats_ClearRecordStat_zeroes_the_marathon_score)
{
    Reset();
    g_acc.level100HighScore = 5000000123LL;
    g_acc.highScore = 77;
    PackAccount(0);
    ClearRecordStat(0);
    CHECK_EQ_INT(Load(0)->level100HighScore, 0);
    CHECK_EQ_INT(Load(0)->highScore, 77);
}

TEST(Stats_RescaleRatioStat_rescales_to_450_played)
{
    Reset();
    g_acc.bonusLevelsPlayed = 900;
    g_acc.perfectBonusLevels = 600;
    PackAccount(0);
    RescaleRatioStat(0);
    CHECK_EQ_INT(Load(0)->bonusLevelsPlayed, 450);
    CHECK_EQ_INT(Load(0)->perfectBonusLevels, 300);
}

// With nothing played the ratio counts as 1, but bonusLevelsPlayed still becomes 450
// (current behaviour).
TEST(Stats_RescaleRatioStat_with_nothing_played_sets_450)
{
    Reset();
    g_acc.perfectBonusLevels = 5;
    PackAccount(0);
    RescaleRatioStat(0);
    CHECK_EQ_INT(Load(0)->bonusLevelsPlayed, 450);
    CHECK_EQ_INT(Load(0)->perfectBonusLevels, 5);
}

// ---------------------------------------------------------------------------------------
// Medal checks
// ---------------------------------------------------------------------------------------

static int RatioMedalFor(int played, int perfect)
{
    g_profileIndex = 0;
    Reset();
    g_acc.bonusLevelsPlayed = played;
    g_acc.perfectBonusLevels = perfect;
    PackAccount(0);
    CheckRatioMedal(0);
    return GetMedals(0);
}

TEST(Stats_CheckRatioMedal_needs_500_played_at_75_percent)
{
    CHECK_EQ_INT(RatioMedalFor(500, 375), MEDAL_BONUS_RATIO);
    CHECK_EQ_INT(RatioMedalFor(499, 499), 0);
    CHECK_EQ_INT(RatioMedalFor(500, 374), 0);
    CHECK_EQ_INT(RatioMedalFor(1000, 1000), MEDAL_BONUS_RATIO);
    CHECK_EQ_INT(GetMedalOrder(0, 0), MEDAL_BONUS_RATIO);
}

static int AllLevelsMedalFor(int done)
{
    g_profileIndex = 0;
    Reset();
    for (int i = 0; i < done; i++)
        g_acc.levelDone[i] = 1;
    PackAccount(0);
    CheckAllLevelsMedal(0);
    return GetMedals(0);
}

TEST(Stats_CheckAllLevelsMedal_needs_every_level)
{
    CHECK_EQ_INT(AllLevelsMedalFor(29), 0);
    CHECK_EQ_INT(AllLevelsMedalFor(30), MEDAL_ALL_LEVELS);
    g_numLevels = 31;
    CHECK_EQ_INT(AllLevelsMedalFor(30), 0);
}

// ---------------------------------------------------------------------------------------
// The secret medal sequence at completion rank 2
// ---------------------------------------------------------------------------------------

static const int kSequence[6] = {MEDAL_DRUNK_FINISH, MEDAL_SPEED_STREAK, MEDAL_BONUS_RATIO,
                                 MEDAL_ALL_LEVELS, MEDAL_OVERALL, MEDAL_EXACT_MONEY};

static void RankTwoProfile(int played, int perfect, int money)
{
    EnableSounds();
    g_scoreMul[0] = 1;
    g_time = 1000;
    Reset();
    g_acc.completionRank = 2;
    g_acc.bonusLevelsPlayed = played;
    g_acc.perfectBonusLevels = perfect;
    g_acc.secretsInOneGame = 5;
    for (int i = 0; i < 30; i++)
        g_acc.levelDone[i] = 1;
    PackAccount(0);
    g_save.players[0].money = money;
}

TEST(Stats_medal_sequence_steps_through_the_six_medals)
{
    RankTwoProfile(0, 0, 0);
    for (int i = 0; i < 5; i++) {
        AwardMedal(0, kSequence[i]);
        CHECK_EQ_INT(Load(0)->medalStep, kSequence[i]);
    }
    CHECK_EQ_INT(GetMedals(0), 0x1f);
    CHECK_EQ_INT(Load(0)->completionRank, 2);
    CHECK_EQ_INT(g_soundQueueCount, 0);
}

TEST(Stats_medal_sequence_unlocks_planet_ranks)
{
    RankTwoProfile(900, 600, 600000);
    for (int i = 0; i < 6; i++)
        AwardMedal(0, kSequence[i]);
    const Account *a = Load(0);
    CHECK_EQ_INT(a->completionRank, 3);
    CHECK_EQ_INT(a->medals, 0);
    CHECK_EQ_INT(a->medalStep, -1);
    for (int i = 0; i < 7; i++)
        CHECK_EQ_INT(a->medalOrder[i], 0);
    CHECK_EQ_INT(a->levelDone[0], 0);
    CHECK_EQ_INT(a->levelDone[29], 0);
    CHECK_EQ_INT(a->secretsInOneGame, 0);
    CHECK_EQ_INT(a->bonusLevelsPlayed, 450);
    CHECK_EQ_INT(a->perfectBonusLevels, 300);
    CHECK_EQ_INT(g_save.players[0].money, 100000);
    CHECK_EQ_INT(g_save.players[0].score, 250000000);
    CHECK_STR(g_alertMsg, "**** *  PLANET RANKS NOW AVAILABLE  * ****");
    CHECK_EQ_INT(g_msgColor, 8);
    CHECK_EQ_INT(g_msgTimer, 8000);
    CHECK_EQ_INT(g_popups[0].active, 1);
    CHECK_EQ_INT(g_popups[0].value, 250000000);
    static const AudioHandle s[] = {102, 103, 104};
    static const int d[] = {40, 40, 40};
    CheckQueue(3, s, d);
}

TEST(Stats_medal_sequence_keeps_small_ratio_and_money)
{
    RankTwoProfile(449, 300, 500000);
    for (int i = 0; i < 6; i++)
        AwardMedal(0, kSequence[i]);
    CHECK_EQ_INT(Load(0)->completionRank, 3);
    CHECK_EQ_INT(Load(0)->bonusLevelsPlayed, 449);
    CHECK_EQ_INT(Load(0)->perfectBonusLevels, 300);
    CHECK_EQ_INT(g_save.players[0].money, 500000);
}

TEST(Stats_medal_sequence_restarts_on_a_wrong_medal)
{
    RankTwoProfile(0, 0, 0);
    AwardMedal(0, MEDAL_DRUNK_FINISH);
    AwardMedal(0, MEDAL_SPEED_STREAK);
    AwardMedal(0, MEDAL_ALL_LEVELS);
    CHECK_EQ_INT(Load(0)->medalStep, -1);
    AwardMedal(0, MEDAL_BONUS_RATIO);
    AwardMedal(0, MEDAL_ALL_LEVELS);
    AwardMedal(0, MEDAL_OVERALL);
    AwardMedal(0, MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(Load(0)->completionRank, 2);
    CHECK_EQ_INT(GetMedals(0), MEDALS_ALL);
    CHECK_EQ_INT(g_soundQueueCount, 0);
}

TEST(Stats_medal_sequence_each_step_needs_the_right_medal)
{
    // a wrong medal at any step sends the sequence back to the start
    for (int wrongAt = 1; wrongAt < 6; wrongAt++) {
        RankTwoProfile(0, 0, 0);
        for (int i = 0; i < wrongAt; i++)
            AwardMedal(0, kSequence[i]);
        CHECK_EQ_INT(Load(0)->medalStep, kSequence[wrongAt - 1]);
        AwardMedal(0, MEDAL_DRUNK_FINISH);
        CHECK_EQ_INT(Load(0)->medalStep, -1);
        CHECK_EQ_INT(Load(0)->completionRank, 2);
    }
}

TEST(Stats_medal_sequence_only_runs_at_completion_rank_2)
{
    Reset();
    g_acc.completionRank = 1;
    PackAccount(0);
    for (int i = 0; i < 6; i++)
        AwardMedal(0, kSequence[i]);
    CHECK_EQ_INT(Load(0)->medalStep, -1);
    CHECK_EQ_INT(Load(0)->completionRank, 1);
    CHECK_EQ_INT(GetMedals(0), MEDALS_ALL);
}

// ---------------------------------------------------------------------------------------
// All six medals at completion rank 3: the next planet rank
// ---------------------------------------------------------------------------------------

static void RankThreeProfile(int unlocked)
{
    EnableSounds();
    g_scoreMul[0] = g_scoreMul[1] = 1;
    g_time = 2000;
    Reset();
    g_acc.completionRank = 3;
    g_acc.unlockedRank = unlocked;
    g_acc.medals = 0x1f;
    g_acc.medalOrder[0] = MEDAL_DRUNK_FINISH;
    g_acc.medalOrder[6] = 5;
    g_acc.medalStep = MEDAL_ALL_LEVELS;
    g_acc.bonusLevelsPlayed = 900;
    g_acc.perfectBonusLevels = 600;
    g_acc.secretsInOneGame = 4;
    g_acc.levelDone[0] = g_acc.levelDone[29] = 1;
    PackAccount(0);
    g_save.players[0].money = g_save.players[1].money = 700000;
}

TEST(Stats_all_medals_at_rank_3_unlock_the_next_planet_rank)
{
    RankThreeProfile(RANK_GOD);
    AwardMedal(0, MEDAL_EXACT_MONEY);
    const Account *a = Load(0);
    CHECK_EQ_INT(a->unlockedRank, RANK_GOD_PLUTO);
    CHECK_EQ_INT(a->medals, 0);
    CHECK_EQ_INT(a->medalOrder[0], 0);
    CHECK_EQ_INT(a->medalOrder[6], 0);
    CHECK_EQ_INT(a->medalStep, -1);
    CHECK_EQ_INT(a->levelDone[0], 0);
    CHECK_EQ_INT(a->levelDone[29], 0);
    CHECK_EQ_INT(a->secretsInOneGame, 0);
    CHECK_EQ_INT(a->bonusLevelsPlayed, 450);
    CHECK_EQ_INT(a->perfectBonusLevels, 300);
    CHECK_EQ_INT(a->completionRank, 3);
    CHECK_EQ_INT(g_save.players[0].money, 200000);
    CHECK_EQ_INT(g_save.players[0].score, 250000000);
    CHECK_EQ_INT(g_popups[0].value, 250000000);
    CHECK_STR(g_alertMsg, "NEW PLANET RANK AVAILABLE");
    CHECK_EQ_INT(g_msgColor, 1);
    CHECK_EQ_INT(g_msgTimer, 7000);
    static const AudioHandle s[] = {101, 102, 103, 104};
    static const int d[] = {40, 40, 200, 40};
    CheckQueue(4, s, d);
}

TEST(Stats_planet_rank_message_uses_the_dual_mode_player_colour)
{
    RankThreeProfile(25);
    g_gameMode = MODE_DUAL;
    g_curPlayer = 1;
    AwardMedal(0, MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(Load(0)->unlockedRank, 26);
    CHECK_EQ_INT(g_msgColor, 4);
    CHECK_EQ_INT(g_save.players[1].money, 200000);
    CHECK_EQ_INT(g_save.players[0].money, 700000);
    CHECK_EQ_INT(g_save.players[1].score, 250000000);

    RankThreeProfile(25);
    g_curPlayer = 0;
    AwardMedal(0, MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(g_msgColor, 1);
}

TEST(Stats_planet_ranks_stop_at_the_last_one)
{
    RankThreeProfile(MAX_RANK - 1);
    AwardMedal(0, MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(Load(0)->unlockedRank, MAX_RANK);

    RankThreeProfile(MAX_RANK);
    AwardMedal(0, MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(Load(0)->unlockedRank, MAX_RANK);
    CHECK_EQ_INT(GetMedals(0), MEDALS_ALL);
    CHECK_EQ_INT(g_save.players[0].money, 700000);
    CHECK_EQ_INT(g_soundQueueCount, 0);
}

TEST(Stats_planet_rank_needs_completion_rank_3)
{
    RankThreeProfile(RANK_GOD);
    UnpackAccount(0);
    g_acc.completionRank = 1;
    PackAccount(0);
    AwardMedal(0, MEDAL_EXACT_MONEY);
    CHECK_EQ_INT(Load(0)->unlockedRank, RANK_GOD);
    CHECK_EQ_INT(GetMedals(0), MEDALS_ALL);
}

TEST(Stats_MedalIconOffsetY_is_one_row_per_medal)
{
    CHECK_EQ_INT(MedalIconOffsetY(MEDAL_DRUNK_FINISH), 0);
    CHECK_EQ_INT(MedalIconOffsetY(MEDAL_SPEED_STREAK), 0x40);
    CHECK_EQ_INT(MedalIconOffsetY(MEDAL_BONUS_RATIO), 0x80);
    CHECK_EQ_INT(MedalIconOffsetY(MEDAL_ALL_LEVELS), 0xc0);
    CHECK_EQ_INT(MedalIconOffsetY(MEDAL_OVERALL), 0x100);
    CHECK_EQ_INT(MedalIconOffsetY(MEDAL_EXACT_MONEY), 0x140);
    CHECK_EQ_INT(MedalIconOffsetY(0x40), 0);
}
