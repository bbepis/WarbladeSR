// Tests for src/profile/stats.c: ApplyStatUnlocks (the start-of-game bonuses a profile's
// statistics unlock) and ProfileWindow (the statistics window and its unlock toggles).
#include "support.h"

static ShipDef s_ship = {.baseArmour = 2, .armourStep = 1, .maxArmourBonus = 3};

// ResetAccount with the RNG seeded (MakeRandomId loops forever on an all-zero xorshift state).
static void Reset(void)
{
    if (g_rngW == 0) {
        SeedRand(999);
        g_rngT = 0x5555;
        g_rngW = 0x6666;
    }
    ResetAccount();
}

static Account s_got;
static const Account *Load(int slot)
{
    UnpackAccount(slot);
    s_got = g_acc;
    ClearAccount();
    return &s_got;
}

#define P g_save.players[0]

// Profile 0 for a game in `mode`; the test then edits g_acc and calls Apply().
static void UnlockSetUp(int mode)
{
    g_profileIndex = 0;
    g_gameMode = mode;
    g_shipDefs[0] = &s_ship;
    g_speedBase = 2.0f;
    g_speedStep = 0.5f;
    g_maxSpeedMul = 10.0f;
    g_timeMax = 40;
    g_time = 10000;
    Reset();
}

// Packs g_acc as profile 0, clears player 0, and applies the unlocks.
static void Apply(void)
{
    PackAccount(0);
    memset(&P, 0, sizeof P);
    P.buffDuration = 7;
    ApplyStatUnlocks();
}

static void ApplyHighScore(__int64 score)
{
    UnlockSetUp(MODE_SINGLE);
    g_acc.highScore = score;
    Apply();
}

// ---------------------------------------------------------------------------------------
// ApplyStatUnlocks
// ---------------------------------------------------------------------------------------

TEST(Stats_ApplyStatUnlocks_needs_a_profile_in_single_or_time_trial)
{
    ApplyHighScore(5000000);
    CHECK_EQ_INT(P.bullets, 10);

    UnlockSetUp(MODE_SINGLE);
    g_acc.highScore = 5000000;
    g_profileIndex = -1;
    Apply();
    CHECK_EQ_INT(P.bullets, 0);

    UnlockSetUp(MODE_DUAL);
    g_acc.highScore = 5000000;
    Apply();
    CHECK_EQ_INT(P.bullets, 0);

    UnlockSetUp(MODE_SINGLE);
    g_acc.highScore = 5000000;
    g_playerUpdateFn = StateDemo;
    Apply();
    CHECK_EQ_INT(P.bullets, 0);
}

TEST(Stats_ApplyStatUnlocks_high_score_ladder)
{
    ApplyHighScore(4999999);
    CHECK_EQ_INT(P.bullets, 0);

    ApplyHighScore(5000000);
    CHECK_EQ_INT(P.bullets, 10);
    CHECK_NEAR(P.speed, 0, 0);

    ApplyHighScore(7500000);
    CHECK_NEAR(P.speed, 3.5, 1e-6);
    CHECK_EQ_INT(P.autofire, 0);

    ApplyHighScore(10000000);
    CHECK_EQ_INT(P.autofire, 1);
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);

    ApplyHighScore(20000000);
    CHECK_EQ_INT(P.weapon, WEAPON_DOUBLE);
    CHECK_EQ_INT(P.armour, 0);

    ApplyHighScore(50000000);
    CHECK_EQ_INT(P.armour, 3);
    CHECK_EQ_INT(P.money, 0);

    ApplyHighScore(100000000);
    CHECK_EQ_INT(P.money, 500);

    ApplyHighScore(250000000);
    CHECK_EQ_INT(P.money, 1000);
    CHECK_EQ_INT(P.armour, 3);

    ApplyHighScore(500000000);
    CHECK_EQ_INT(P.armour, 5);
    CHECK_EQ_INT(P.weapon, WEAPON_DOUBLE);

    ApplyHighScore(1000000000);
    CHECK_EQ_INT(P.weapon, WEAPON_TRIPLE);
    CHECK_EQ_INT(P.bullets, 10);
    CHECK_EQ_INT(P.autofire, 1);
    CHECK_EQ_INT(P.money, 1000);
}

TEST(Stats_ApplyStatUnlocks_never_downgrades_the_weapon)
{
    UnlockSetUp(MODE_SINGLE);
    g_acc.highScore = 1000000000;
    PackAccount(0);
    memset(&P, 0, sizeof P);
    P.weapon = WEAPON_QUAD;
    ApplyStatUnlocks();
    CHECK_EQ_INT(P.weapon, WEAPON_QUAD);

    UnlockSetUp(MODE_SINGLE);
    g_acc.highScore = 20000000;
    PackAccount(0);
    memset(&P, 0, sizeof P);
    P.weapon = WEAPON_TRIPLE;
    ApplyStatUnlocks();
    CHECK_EQ_INT(P.weapon, WEAPON_TRIPLE);
}

static void ApplyTimeTrial(__int64 score)
{
    UnlockSetUp(MODE_TIME_TRIAL);
    g_acc.timeTrialHighScore = score;
    g_acc.highScore = 1000000000;
    g_scoreMul[0] = 1;
    g_scoopRange = 5;
    Apply();
}

TEST(Stats_ApplyStatUnlocks_time_trial_ladder)
{
    ApplyTimeTrial(4999999);
    CHECK_EQ_INT(g_scoreMul[0], 1);
    CHECK_EQ_INT(P.bullets, 0);     // no high-score unlocks in Time Trial
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);

    ApplyTimeTrial(5000000);
    CHECK_EQ_INT(P.scoreMult2Timer, 17000);
    CHECK_EQ_INT(P.scoreMult5Timer, 0);
    CHECK_EQ_INT(g_scoreMul[0], 2);
    CHECK_EQ_INT(P.scoopTimer, 0);

    ApplyTimeTrial(6000000);
    CHECK_EQ_INT(P.scoopTimer, 17000);
    CHECK_NEAR(g_scoopRange, 0, 0);
    CHECK_EQ_INT(g_scoop[0].spawnDelay, 5);
    CHECK_EQ_INT(g_scoop[14].spawnDelay, 19);
    CHECK_EQ_INT(g_scoop[14].timer, 2);
    CHECK_EQ_INT(g_scoreMul[0], 2);

    ApplyTimeTrial(7000000);
    CHECK_EQ_INT(P.scoreMult5Timer, 17000);
    CHECK_EQ_INT(P.scoreMult2Timer, 0);
    CHECK_EQ_INT(g_scoreMul[0], 5);
    CHECK_EQ_INT(P.autofire, 0);

    ApplyTimeTrial(8000000);
    CHECK_EQ_INT(P.autofire, 1);
    CHECK_NEAR(P.speed, 0, 0);

    ApplyTimeTrial(9000000);
    CHECK_NEAR(P.speed, 3.5, 1e-6);
    CHECK_EQ_INT(P.buffDuration, 30);

    ApplyTimeTrial(10000000);
    CHECK_NEAR(P.speed, 5.0, 1e-6);
    CHECK_EQ_INT(P.superAuto, 0);

    ApplyTimeTrial(15000000);
    CHECK_EQ_INT(P.superAuto, 1);
    CHECK_EQ_INT(P.autofireInterval, 25);
    CHECK_NEAR(P.speed, 5.0, 1e-6);
    CHECK_EQ_INT(g_timeTrialDeadline, 0);

    ApplyTimeTrial(17000000);
    CHECK_NEAR(P.speed, 7.0, 1e-6);
    CHECK_EQ_INT(g_timeTrialDeadline, 0);

    ApplyTimeTrial(20000000);
    CHECK_EQ_INT(g_timeTrialDeadline, 251000);
}

TEST(Stats_ApplyStatUnlocks_time_trial_ladder_not_in_single)
{
    UnlockSetUp(MODE_SINGLE);
    g_acc.timeTrialHighScore = 20000000;
    g_scoreMul[0] = 1;
    Apply();
    CHECK_EQ_INT(g_scoreMul[0], 1);
    CHECK_EQ_INT(P.scoreMult2Timer, 0);
    CHECK_EQ_INT(g_timeTrialDeadline, 0);
}

static void ApplySecrets(int mode, int found, int armour)
{
    UnlockSetUp(mode);
    for (int i = 0; i < found; i++)
        g_acc.levelDone[i] = 1;
    PackAccount(0);
    memset(&P, 0, sizeof P);
    P.armour = armour;
    ApplyStatUnlocks();
}

TEST(Stats_ApplyStatUnlocks_all_secrets_give_armour_and_super_triple)
{
    ApplySecrets(MODE_SINGLE, 30, 0);
    CHECK_EQ_INT(P.armour, 5);
    CHECK_EQ_INT(P.money, 0);
    CHECK_EQ_INT(P.weapon, WEAPON_SUPER_TRIPLE);

    ApplySecrets(MODE_SINGLE, 30, 5);
    CHECK_EQ_INT(P.armour, 5);
    CHECK_EQ_INT(P.money, 2000);

    ApplySecrets(MODE_SINGLE, 29, 0);
    CHECK_EQ_INT(P.armour, 0);
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);

    ApplySecrets(MODE_TIME_TRIAL, 30, 0);
    CHECK_EQ_INT(P.armour, 0);
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);
}

static void ApplyHitPct(int mode, int pct)
{
    UnlockSetUp(mode);
    g_shopItems = 83;
    g_acc.bestHitPctAbove25 = pct;
    Apply();
}

TEST(Stats_ApplyStatUnlocks_hit_percentage_adds_shop_items)
{
    ApplyHitPct(MODE_SINGLE, 69);
    CHECK_EQ_INT(g_shopItems, 83);
    ApplyHitPct(MODE_SINGLE, 70);
    CHECK_EQ_INT(g_shopItems, 84);
    ApplyHitPct(MODE_SINGLE, 80);
    CHECK_EQ_INT(g_shopItems, 85);
    ApplyHitPct(MODE_SINGLE, 90);
    CHECK_EQ_INT(g_shopItems, 86);
    ApplyHitPct(MODE_TIME_TRIAL, 90);
    CHECK_EQ_INT(g_shopItems, 83);
}

TEST(Stats_ApplyStatUnlocks_marathon_milestone)
{
    UnlockSetUp(MODE_SINGLE);
    g_acc.level100HighScore = 200000000;
    Apply();
    CHECK_EQ_INT(P.highScoreMilestone, 1);

    UnlockSetUp(MODE_TIME_TRIAL);
    g_acc.level100HighScore = 200000000;
    Apply();
    CHECK_EQ_INT(P.highScoreMilestone, 0);
}

static void ApplyLevels(int mode, int levels)
{
    UnlockSetUp(mode);
    g_acc.totalLevelsPlayed = levels;
    Apply();
}

TEST(Stats_ApplyStatUnlocks_levels_played_ladder)
{
    ApplyLevels(MODE_SINGLE, 49999);
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);
    CHECK_EQ_INT(P.multiplierUnlocked, 1);

    ApplyLevels(MODE_SINGLE, 50000);
    CHECK_EQ_INT(P.weapon, WEAPON_QUAD);
    CHECK_NEAR(P.bulletSpeedMult, 0, 0);

    ApplyLevels(MODE_SINGLE, 75000);
    CHECK_NEAR(P.bulletSpeedMult, 1.25, 1e-6);
    CHECK_EQ_INT(P.bullets, 0);

    ApplyLevels(MODE_SINGLE, 100000);
    CHECK_NEAR(P.speed, 4.5, 1e-6);
    CHECK_EQ_INT(P.bullets, 25);
    CHECK_EQ_INT(P.buffDuration, 20);
    CHECK_EQ_INT(P.money, 5000);

    ApplyLevels(MODE_TIME_TRIAL, 100000);
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);
    CHECK_EQ_INT(P.bullets, 0);
    CHECK_EQ_INT(P.multiplierUnlocked, 0);
}

TEST(Stats_ApplyStatUnlocks_keeps_a_faster_bullet_speed)
{
    UnlockSetUp(MODE_SINGLE);
    g_acc.totalLevelsPlayed = 75000;
    PackAccount(0);
    memset(&P, 0, sizeof P);
    P.bulletSpeedMult = 1.4f;
    ApplyStatUnlocks();
    CHECK_NEAR(P.bulletSpeedMult, 1.4, 1e-6);
}

static void ApplyRank(int mode, int rank)
{
    UnlockSetUp(mode);
    g_acc.highestRank = rank;
    g_acc.unlockedRank = MAX_RANK;
    Apply();
}

TEST(Stats_ApplyStatUnlocks_max_rank_gives_the_war_plasma)
{
    ApplyRank(MODE_SINGLE, MAX_RANK);
    CHECK_EQ_INT(P.buffDuration, 40);
    CHECK_EQ_INT(P.money, 25000);
    CHECK_NEAR(P.bulletSpeedMult, 1.5, 1e-6);
    CHECK_EQ_INT(P.weapon, WEAPON_WAR_PLASMA);
    CHECK_EQ_INT(P.alienLock, 1);
    CHECK_EQ_INT(P.superAuto, 1);
    CHECK_EQ_INT(P.autofireInterval, 25);
    CHECK_EQ_INT(P.autofire, 1);

    ApplyRank(MODE_SINGLE, MAX_RANK - 1);
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);
    CHECK_EQ_INT(P.money, 0);

    ApplyRank(MODE_TIME_TRIAL, MAX_RANK);
    CHECK_EQ_INT(P.weapon, WEAPON_SINGLE);
    CHECK_EQ_INT(P.alienLock, 0);
}

TEST(Stats_ApplyStatUnlocks_completed_game_raises_the_money_cap)
{
    UnlockSetUp(MODE_TIME_TRIAL);
    g_acc.gameCompleted = true;
    Apply();
    CHECK_EQ_INT(P.moneyMax, 999990);

    UnlockSetUp(MODE_SINGLE);
    Apply();
    CHECK_EQ_INT(P.moneyMax, 0);
}

// ---------------------------------------------------------------------------------------
// ProfileWindow
// ---------------------------------------------------------------------------------------

static Window *OpenProfile(bool noButtons)
{
    PackAccount(0);
    g_profileIndex = 0;
    ProfileWindow(noButtons);
    return &g_windows[g_profileWin];
}

static const ToggleItem *FindToggle(const Window *w, const char *prefix)
{
    for (int i = 0; i <= w->nG; i++)
        if (!strncmp(w->toggles[i].text, prefix, strlen(prefix)))
            return &w->toggles[i];
    TestFail(__FILE__, __LINE__, "no toggle \"%s\"", prefix);
    return &w->toggles[0];
}

static const TextItem *TextAt(const Window *w, int x, int y)
{
    for (int i = 0; i <= w->nE; i++)
        if (w->texts[i].x == x && w->texts[i].y == y)
            return &w->texts[i];
    TestFail(__FILE__, __LINE__, "no text at %d,%d", x, y);
    return &w->texts[0];
}

static const MenuItem *FindMenuItem(const Window *w, int id)
{
    for (int i = 0; i <= w->nF; i++)
        if (w->menuItems[i].id == id)
            return &w->menuItems[i];
    return NULL;
}

static const char *MenuText(const Window *w, int id)
{
    const MenuItem *m = FindMenuItem(w, id);
    return m ? m->text : "(none)";
}

// A profile with every unlock toggle of the window earned.
static void EverythingUnlocked(void)
{
    g_acc.highScore = 1000000000;
    g_acc.timeTrialHighScore = 20000000;
    g_acc.level100HighScore = 200000001;
    g_acc.bestLevelTime = 5000000;          // 0.5 s
    for (int i = 0; i < 30; i++)
        g_acc.levelDone[i] = 1;
    g_acc.totalLevelsPlayed = 100000;
    g_acc.bestHitPctAbove25 = 90;
}

TEST(Stats_ProfileWindow_shows_the_statistics)
{
    Reset();
    g_acc.highScore = 1234567;
    g_acc.meteorstormHighScore = 4500000;
    g_acc.timeTrialHighScore = 8000000;
    g_acc.bonusLevelsPlayed = 4;
    g_acc.perfectBonusLevels = 3;
    g_acc.highestLevel = 42;
    g_acc.gamesPlayed = 9;
    g_acc.highestRank = RANK_CAPTAIN;
    Window *w = OpenProfile(true);
    CHECK_EQ_INT(w->w, 550);
    CHECK_STR(TextAt(w, 220, 50)->text, "1.234.567");
    CHECK_STR(TextAt(w, 220, 60)->text, "4.500.000");
    CHECK_STR(TextAt(w, 220, 70)->text, "8.000.000");
    CHECK_STR(TextAt(w, 260, 96)->text, "4");
    CHECK_STR(TextAt(w, 260, 105)->text, "3");
    CHECK_STR(TextAt(w, 260, 114)->text, "75 %");
    CHECK_STR(TextAt(w, 220, 200)->text, "42");
    CHECK_STR(TextAt(w, 220, 220)->text, "9");
    CHECK_STR(TextAt(w, 240, 280)->text, "CAPTAIN");
    CHECK_STR(TextAt(w, POS_CENTERED, 366 - 50)->text, "PRESS TAB TO CLOSE WINDOW");
}

TEST(Stats_ProfileWindow_score_toggles_stop_at_the_first_missed_one)
{
    Reset();
    g_acc.highScore = 7500000;
    Window *w = OpenProfile(true);
    const ToggleItem *t = FindToggle(w, "5.000.000 : START WITH 10 BULLETS");
    CHECK_EQ_INT(t->value, 1);
    CHECK_EQ_INT(t->flag2, 1);
    t = FindToggle(w, "7.500.000 : START WITH SPEED * 3");
    CHECK_EQ_INT(t->value, 1);
    t = FindToggle(w, "10.000.000 : START WITH AUTOFIRE");
    CHECK_EQ_INT(t->value, 0);
    CHECK_EQ_INT(t->flag2, 1);
    t = FindToggle(w, "20.000.000 : START WITH DOUBLE SHOT");
    CHECK_EQ_INT(t->value, 0);
    CHECK_EQ_INT(t->flag2, 0);
    t = FindToggle(w, "1.000.000.000 : START WITH TRIPLE SHOT");
    CHECK_EQ_INT(t->value, 0);
}

TEST(Stats_ProfileWindow_toggles_are_not_selectable_when_read_only)
{
    Reset();
    g_acc.highScore = 7500000;
    g_profileReadOnly = 1;
    Window *w = OpenProfile(true);
    const ToggleItem *t = FindToggle(w, "5.000.000 : START WITH 10 BULLETS");
    CHECK_EQ_INT(t->value, 1);
    CHECK_EQ_INT(t->flag2, 0);
}

// The second ladder (drawn on the Meteorstorm line) compares the Time Trial score:
// ShowMeteorStormScore doesn't set g_shownStat, ShowTimeTrialScore after it does.
TEST(Stats_ProfileWindow_time_and_level_toggles)
{
    Reset();
    g_acc.bestLevelTime = 15000000;     // 1.5 s
    g_acc.totalLevelsPlayed = 2500;
    g_acc.meteorstormHighScore = 9000000;
    g_acc.timeTrialHighScore = 6000000;
    Window *w = OpenProfile(true);
    CHECK_EQ_INT(FindToggle(w, "<= 2 SECOND")->value, 1);
    CHECK_EQ_INT(FindToggle(w, "<= 1 SECOND")->value, 0);
    CHECK_EQ_INT(FindToggle(w, "<= 1 SECOND")->flag2, 1);
    CHECK_EQ_INT(FindToggle(w, "2.500 : START WITH MISSILE STEALTH")->value, 1);
    CHECK_EQ_INT(FindToggle(w, "5.000 : START WITH GEM COUNTER ON")->value, 0);
    CHECK_EQ_INT(FindToggle(w, "6.000.000 : START WITH SCOOP")->value, 1);
    CHECK_EQ_INT(FindToggle(w, "7.000.000 : START WITH MULTIPLY 5")->value, 0);
}

TEST(Stats_ProfileWindow_counts_every_unlock_toggle)
{
    Reset();
    EverythingUnlocked();
    OpenProfile(true);
    CHECK_EQ_INT(g_toggleOffCount, 0);
    CHECK_EQ_INT(g_toggleOnCount, 9 + 9 + 1 + 2 + 1 + 11 + 3);

    Reset();
    OpenProfile(true);
    CHECK_EQ_INT(g_toggleOnCount, 0);
    // the score, Meteorstorm, time and money ladders are all drawn, the one-offs counted
    CHECK_EQ_INT(g_toggleOffCount, 9 + 9 + 1 + 2 + 1 + 11 + 3);
}

TEST(Stats_ProfileWindow_awards_the_overall_medal_for_every_unlock)
{
    Reset();
    EverythingUnlocked();
    OpenProfile(true);
    CHECK_EQ_INT(GetMedals(0), MEDAL_OVERALL);
}

TEST(Stats_ProfileWindow_overall_medal_needs_the_marathon_score)
{
    Reset();
    EverythingUnlocked();
    g_acc.level100HighScore = 200000000;
    OpenProfile(true);
    CHECK_EQ_INT(GetMedals(0), 0);
}

TEST(Stats_ProfileWindow_overall_medal_needs_the_hit_percentage)
{
    Reset();
    EverythingUnlocked();
    g_acc.bestHitPctAbove25 = 89;
    OpenProfile(true);
    CHECK_EQ_INT(GetMedals(0), 0);
}

TEST(Stats_ProfileWindow_overall_medal_needs_every_secret)
{
    Reset();
    EverythingUnlocked();
    g_acc.levelDone[29] = 0;
    Window *w = OpenProfile(true);
    CHECK_EQ_INT(FindToggle(w, "FIND ALL SECRETS")->value, 0);
    CHECK_EQ_INT(GetMedals(0), 0);

    Reset();
    EverythingUnlocked();
    for (int i = 0; i < 30; i++)
        g_acc.levelDone[i] = i < 15;
    OpenProfile(true);
    CHECK_EQ_INT(GetMedals(0), 0);
}

TEST(Stats_ProfileWindow_awards_the_exact_money_medal)
{
    Reset();
    g_save.players[0].money = 999990;
    OpenProfile(true);
    CHECK_EQ_INT(GetMedals(0), MEDAL_EXACT_MONEY);

    Reset();
    g_save.players[0].money = 999989;
    OpenProfile(true);
    CHECK_EQ_INT(GetMedals(0), 0);
}

TEST(Stats_ProfileWindow_grows_for_medals_and_ranks)
{
    Reset();
    CHECK_EQ_INT(OpenProfile(true)->h, 366);

    Reset();
    g_acc.medals = MEDAL_DRUNK_FINISH;
    CHECK_EQ_INT(OpenProfile(true)->h, 441);

    Reset();
    g_acc.highestRank = g_acc.unlockedRank = RANK_GOD;
    CHECK_EQ_INT(OpenProfile(true)->h, 366);

    Reset();
    g_acc.highestRank = g_acc.unlockedRank = RANK_GOD_URANUS;
    CHECK_EQ_INT(OpenProfile(true)->h, 416);

    Reset();
    g_acc.highestRank = g_acc.unlockedRank = RANK_GOD_URANUS;
    g_acc.medals = MEDAL_DRUNK_FINISH;
    CHECK_EQ_INT(OpenProfile(true)->h, 491);

    Reset();
    g_acc.highestRank = g_acc.unlockedRank = MAX_RANK;
    g_acc.medals = MEDAL_DRUNK_FINISH;
    CHECK_EQ_INT(OpenProfile(true)->h, 566);

    Reset();
    g_acc.medals = MEDAL_DRUNK_FINISH;
    g_profileReadOnly = 1;
    CHECK_EQ_INT(OpenProfile(true)->h, 366);
}

TEST(Stats_ProfileWindow_draws_the_medals_in_award_order)
{
    Reset();
    g_acc.medals = MEDAL_ALL_LEVELS | MEDAL_SPEED_STREAK;
    g_acc.medalOrder[0] = MEDAL_ALL_LEVELS;
    g_acc.medalOrder[1] = MEDAL_SPEED_STREAK;
    g_acc.medalOrder[6] = 2;
    Window *w = OpenProfile(true);
    CHECK_EQ_INT(w->nB, 1);
    CHECK_EQ_INT(w->buttons[0].srcX, 0xc0);
    CHECK_EQ_INT(w->buttons[0].x, 40);
    CHECK_EQ_INT(w->buttons[0].y, 441 - 55 - 75);
    CHECK_EQ_INT(w->buttons[0].srcY, 0);
    CHECK_EQ_INT(w->buttons[1].srcX, 0x40);
    CHECK_EQ_INT(w->buttons[1].x, 120);
}

TEST(Stats_ProfileWindow_draws_later_medals_after_the_first)
{
    Reset();
    static const int order[6] = {MEDAL_EXACT_MONEY, MEDAL_OVERALL, MEDAL_DRUNK_FINISH,
                                 MEDAL_BONUS_RATIO, MEDAL_SPEED_STREAK, MEDAL_ALL_LEVELS};
    g_acc.medals = MEDALS_ALL;
    for (int i = 0; i < 6; i++)
        g_acc.medalOrder[i] = order[i];
    g_acc.medalOrder[6] = 6;
    Window *w = OpenProfile(true);
    CHECK_EQ_INT(w->nB, 5);
    for (int i = 0; i < 6; i++) {
        CHECK_EQ_INT(w->buttons[i].srcX, MedalIconOffsetY(order[i]));
        CHECK_EQ_INT(w->buttons[i].x, 40 + 80 * i);
    }
}

TEST(Stats_ProfileWindow_medals_use_the_second_row_at_completion_rank_2)
{
    Reset();
    g_acc.completionRank = 2;
    g_acc.medals = MEDAL_BONUS_RATIO;
    g_acc.medalOrder[0] = MEDAL_BONUS_RATIO;
    g_acc.medalOrder[6] = 1;
    Window *w = OpenProfile(true);
    CHECK_EQ_INT(w->nB, 0);
    CHECK_EQ_INT(w->buttons[0].srcX, 0x80);
    CHECK_EQ_INT(w->buttons[0].srcY, 70);
}

TEST(Stats_ProfileWindow_shows_a_planet_per_god_rank)
{
    Reset();
    g_acc.highestRank = g_acc.unlockedRank = RANK_GOD_URANUS;
    Window *w = OpenProfile(true);
    CHECK_EQ_INT(w->nA, 2);     // 3 planets
    CHECK_EQ_INT(w->quadImages[0].srcY, 9 * 128);
    CHECK_EQ_INT(w->quadImages[2].srcY, 7 * 128);
    CHECK_EQ_INT(w->quadImages[0].destY, 416 - 55 - 60);
    CHECK_EQ_INT(w->quadImages[0].destX, 550 - (275 - 78) - 52);
    CHECK_EQ_INT(w->quadImages[1].destX, 550 - (275 - 78) - 104);

    Reset();
    g_acc.highestRank = g_acc.unlockedRank = RANK_GOD;
    w = OpenProfile(true);
    CHECK_EQ_INT(w->nA, -1);
}

TEST(Stats_ProfileWindow_title_buttons)
{
    g_state = STATE_TITLE;
    g_cfg.profileSel = 3;
    Reset();
    Window *w = OpenProfile(false);
    CHECK_STR(MenuText(w, 0x107), "SET AS DEFAULT");
    CHECK(FindMenuItem(w, 0x106) != NULL);
    CHECK(FindMenuItem(w, 0x10d) != NULL);
    CHECK(FindMenuItem(w, 0xfc) != NULL);
    CHECK_EQ_INT(w->selF, 0xfc);

    g_cfg.profileSel = 0;
    Reset();
    w = OpenProfile(false);
    CHECK_STR(MenuText(w, 0x107), "CLEAR AS DEFAULT");
}

TEST(Stats_ProfileWindow_only_close_outside_the_title)
{
    g_state = STATE_TITLE;
    g_profileReadOnly = 1;
    Reset();
    Window *w = OpenProfile(false);
    CHECK(FindMenuItem(w, 0x106) == NULL);
    CHECK(FindMenuItem(w, 0xfc) != NULL);

    g_profileReadOnly = 0;
    g_newGameOnClose = 1;
    Reset();
    w = OpenProfile(false);
    CHECK(FindMenuItem(w, 0x106) == NULL);
    CHECK_EQ_INT(w->nF, 0);
}
