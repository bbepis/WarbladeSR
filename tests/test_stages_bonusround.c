// Tests for src/game/stages/bonusround.c: the special-level test, banking the bonus score,
// and the bonus-wave results screen (tally, PERFECT award, combo, timeout).
#include "support.h"

#define P0 g_save.players[0]
#define P1 g_save.players[1]

static void BonusSetUp(void)
{
    BootGame();
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_state = STATE_PLAYING;
    g_scoreMul[0] = 1;
    g_scoreMul[1] = 1;
    g_frameDt = 1;
    g_time = 50000;
    g_bannerMsgTime = g_msgTime = g_alertTextTime = g_msgTimer = g_levelBannerTime = 0;
}

// The results screen showing, in single-player: `killed` of `total` aliens caught.
static void ResultsShowing(int killed, int total)
{
    BonusSetUp();
    g_bonusResultsTime = g_time + 10000;
    g_perfectCheckDelay = g_time + 100000;     // no PERFECT unless a test allows it
    P0.bonusKilled = killed;
    P0.totalEnemies = total;
    P0.bonusTally = 0;
    P0.bonusTallyTick = 0;
    P0.bonusTallyDelay = 1;
    P0.perfectDone = 0;
    P0.score = 0;
}

// ---- IsSpecialLevel ----

TEST(stages_bonus_IsSpecialLevel_for_bonus_race_and_boss_levels)
{
    BonusSetUp();
    g_curLevelData.type = LEVEL_BONUS_WAVE;
    CHECK_EQ_INT(IsSpecialLevel(0), 1);
    g_curLevelData.type = LEVEL_RACE;
    CHECK_EQ_INT(IsSpecialLevel(0), 1);
    g_curLevelData.type = LEVEL_BOSS;
    CHECK_EQ_INT(IsSpecialLevel(0), 1);
}

TEST(stages_bonus_IsSpecialLevel_not_for_normal_levels)
{
    BonusSetUp();
    g_curLevelData.type = 0;
    CHECK_EQ_INT(IsSpecialLevel(0), 0);
    g_curLevelData.type = 1;
    CHECK_EQ_INT(IsSpecialLevel(0), 0);
    g_curLevelData.type = 5;
    CHECK_EQ_INT(IsSpecialLevel(0), 0);
}

TEST(stages_bonus_IsSpecialLevel_never_in_time_trial)
{
    BonusSetUp();
    g_gameMode = MODE_TIME_TRIAL;
    g_curLevelData.type = LEVEL_BOSS;
    CHECK_EQ_INT(IsSpecialLevel(0), 0);
}

// ---- BankBonusScore ----

TEST(stages_bonus_BankBonusScore_keeps_the_best_round)
{
    BonusSetUp();
    g_curPlayer = 1;
    P1.bonusRoundScore = 500;
    P1.bonusHighScore = 100;
    BankBonusScore();
    CHECK_EQ_INT(P1.bonusHighScore, 500);
    P1.bonusRoundScore = 50;
    BankBonusScore();
    CHECK_EQ_INT(P1.bonusHighScore, 500);
}

TEST(stages_bonus_BankBonusScore_restores_the_saved_hyperspace)
{
    BonusSetUp();
    P0.savedHyperspaceOutTimer = 11;
    P0.savedHyperspaceMidTimer = 12;
    P0.savedHyperspaceInTimer = 13;
    P0.savedHyperspaceInDuration = 14;
    P0.savedHyperspaceFade = 15;
    P0.savedScrollSpeedY = 16;
    P0.savedStarSpeed = 17;
    P0.savedStarVelZ = 18;
    BankBonusScore();
    CHECK_EQ_INT(P0.hyperspaceOutTimer, 11);
    CHECK_EQ_INT(P0.hyperspaceMidTimer, 12);
    CHECK_EQ_INT(P0.hyperspaceInTimer, 13);
    CHECK_EQ_INT(P0.hyperspaceInDuration, 14);
    CHECK_NEAR(P0.hyperspaceFade, 15, 1e-6);
    CHECK_NEAR(P0.scrollSpeedY, 16, 1e-6);
    CHECK_NEAR(P0.starSpeed, 17, 1e-6);
    CHECK_NEAR(P0.starVelZ, 18, 1e-6);
}

TEST(stages_bonus_BankBonusScore_dual_banks_both_players)
{
    BonusSetUp();
    g_gameMode = MODE_DUAL;
    P0.bonusRoundScore = 300;
    P0.bonusHighScore = 200;
    P1.bonusRoundScore = 700;
    P1.bonusHighScore = 100;
    P1.energy = 9;
    g_vsTurnPlayer = 1;
    P1.savedStarSpeed = 42;
    P0.savedStarSpeed = 7;
    BankBonusScore();
    CHECK_EQ_INT(P0.bonusHighScore, 300);
    CHECK_EQ_INT(P1.bonusHighScore, 700);
    CHECK_EQ_INT(P1.energy, 0);
    CHECK_NEAR(P0.starSpeed, 42, 1e-6);     // from the turn player's saved state
}

// ---- the results screen ----

TEST(stages_bonus_results_timeout_without_perfect_pays_the_base_bonus)
{
    BonusSetUp();
    g_bonusResultsTime = g_time - 1;
    P0.bonusKilled = 3;
    P0.totalEnemies = 5;
    P0.bonusRoundPoints = 777777;
    P0.perfectStreak = 4;
    P0.trackKillsFlag = 1;
    P0.bonusResultsInitDone = 1;
    g_perfectAwardedP0 = 0;
    g_comboLevel = 3;
    g_comboStep = 2;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusRoundPoints, 10000);
    CHECK_EQ_INT(P0.perfectStreak, 0);
    CHECK_EQ_INT(g_comboLevel, 0);
    CHECK_EQ_INT(g_comboStep, 0);
    CHECK_EQ_INT(g_bonusResultsTime, 0);
    CHECK_EQ_INT(P0.trackKillsFlag, 0);
    CHECK_EQ_INT(P0.bonusResultsInitDone, 0);
}

TEST(stages_bonus_results_timeout_after_perfect_keeps_the_combo)
{
    BonusSetUp();
    g_bonusResultsTime = g_time - 1;
    P0.bonusKilled = 3;
    P0.totalEnemies = 5;
    P0.bonusRoundPoints = 777777;
    P0.perfectStreak = 4;
    g_perfectAwardedP0 = 1;
    g_comboLevel = 3;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusRoundPoints, 777777);
    CHECK_EQ_INT(P0.perfectStreak, 4);
    CHECK_EQ_INT(g_comboLevel, 3);
    CHECK_EQ_INT(g_bonusResultsTime, 0);
}

TEST(stages_bonus_results_timeout_with_every_alien_keeps_the_combo)
{
    BonusSetUp();
    g_bonusResultsTime = g_time - 1;
    P0.bonusKilled = 5;
    P0.totalEnemies = 5;
    P0.bonusRoundPoints = 777777;
    g_perfectAwardedP0 = 0;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusRoundPoints, 777777);
}

TEST(stages_bonus_results_do_not_time_out_while_paused)
{
    BonusSetUp();
    g_state = STATE_PAUSED;
    g_bonusResultsTime = g_time - 1;
    P0.bonusKilled = 3;
    P0.totalEnemies = 5;
    P0.bonusRoundPoints = 777777;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusRoundPoints, 777777);
    CHECK_NE_INT(g_bonusResultsTime, 0);
}

TEST(stages_bonus_tally_counts_one_alien_every_fourth_frame)
{
    ResultsShowing(2, 10);
    int bells = FakePlayCount("bell3");
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusTally, 1);
    CHECK_EQ_INT(P0.score, 500);
    CHECK_EQ_INT(FakePlayCount("bell3"), bells + 1);
    CHECK_EQ_INT(g_bonusResultsTime, g_time + 4000);
    CHECK_EQ_INT(g_perfectCheckDelay, g_time + 1000);
    for (int i = 0; i < 3; i++)
        UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusTally, 1);
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusTally, 2);
    CHECK_EQ_INT(P0.score, 1000);
    for (int i = 0; i < 20; i++)
        UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusTally, 2);     // stops at the aliens killed
    CHECK_EQ_INT(P0.score, 1000);
    CHECK_STR(g_logBuf, " 2 X 500 = 1000 POINTS");
}

TEST(stages_bonus_tally_uses_the_score_multiplier)
{
    ResultsShowing(2, 10);
    g_scoreMul[0] = 3;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.score, 1500);
}

TEST(stages_bonus_results_clamp_killed_to_the_wave_size)
{
    ResultsShowing(12, 10);
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusKilled, 10);
}

TEST(stages_bonus_perfect_waits_for_the_check_delay)
{
    ResultsShowing(5, 5);
    P0.bonusTally = 5;
    P0.bonusRoundPoints = 25000;
    g_perfectCheckDelay = g_time;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.perfectDone, 0);
    CHECK_EQ_INT(g_perfectAwardedP0, 0);
    CHECK_EQ_INT(P0.score, 0);
}

TEST(stages_bonus_perfect_awards_the_round_points_once)
{
    ResultsShowing(5, 5);
    P0.bonusTally = 5;
    P0.bonusRoundPoints = 25000;
    P0.bonusRoundCount = 2;
    P0.perfectStreak = 0;
    g_perfectCount = 7;
    g_comboStep = 0;
    g_perfectCheckDelay = g_time - 1;
    strcpy(g_perfect, "PERFECT BONUS 99.999.999  POINTS");   // a previous, longer award
    int alright = FakePlayCount("alright");
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_perfectAwardedP0, 1);
    CHECK_EQ_INT(P0.perfectDone, 1);
    CHECK_EQ_INT(P0.score, 25000);
    CHECK_NEAR(P0.bonusRoundCount, 3, 1e-6);
    CHECK_EQ_INT(P0.perfectStreak, 1);
    CHECK_EQ_INT(g_perfectCount, 8);
    CHECK_EQ_INT(g_bonusResultsTime, g_time + 4000);
    CHECK_EQ_INT(FakePlayCount("alright"), alright + 1);
    CHECK_STR(g_perfect, "PERFECT BONUS     25.000  POINTS");
    CHECK_EQ_INT(P0.bonusRoundPoints, 25000);   // no combo step: unchanged
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.score, 25000);
    CHECK_EQ_INT(P0.perfectStreak, 1);
}

TEST(stages_bonus_perfect_reprices_the_next_round_from_the_combo)
{
    ResultsShowing(5, 5);
    P0.bonusTally = 5;
    P0.bonusRoundPoints = 25000;
    g_comboLevel = 3;
    g_comboStep = 2;
    g_perfectCheckDelay = g_time - 1;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.score, 25000);
    CHECK_EQ_INT(g_comboLevel, 5);
    CHECK_EQ_INT(g_comboStep, 0);
    CHECK_EQ_INT(P0.bonusRoundPoints, 500 * 1000);
}

TEST(stages_bonus_perfect_combo_tops_out_at_level_9)
{
    ResultsShowing(5, 5);
    P0.bonusTally = 5;
    P0.bonusRoundPoints = 25000;
    g_comboLevel = 8;
    g_comboStep = 3;
    g_perfectCheckDelay = g_time - 1;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_comboLevel, 9);
    CHECK_EQ_INT(P0.bonusRoundPoints, 10000 * 1000);
}

TEST(stages_bonus_perfect_above_the_combo_threshold_keeps_its_price)
{
    ResultsShowing(5, 5);
    P0.bonusTally = 5;
    P0.bonusRoundPoints = 10000000;
    g_comboLevel = 3;
    g_comboStep = 2;
    g_perfectCheckDelay = g_time - 1;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.score, 10000000);
    CHECK_EQ_INT(P0.bonusRoundPoints, 10000000);
    CHECK_EQ_INT(g_comboLevel, 3);
    CHECK_EQ_INT(g_comboStep, 2);
    CHECK_STR(g_perfect, "PERFECT BONUS 10.000.000  POINTS");
}

TEST(stages_bonus_second_perfect_in_a_row_marks_secret_22)
{
    ResultsShowing(5, 5);
    g_profileIndex = 0;
    P0.bonusTally = 5;
    P0.bonusRoundPoints = 25000;
    P0.perfectStreak = 1;
    P0.secretFound22 = 0;
    g_perfectCheckDelay = g_time - 1;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.perfectStreak, 2);
    CHECK_EQ_INT(P0.secretFound22, 1);
    CHECK(IsSecretFound(0, 22));
}

TEST(stages_bonus_first_perfect_marks_no_secret)
{
    ResultsShowing(5, 5);
    g_profileIndex = 0;
    P0.bonusTally = 5;
    P0.bonusRoundPoints = 25000;
    P0.perfectStreak = 0;
    P0.secretFound22 = 0;
    g_perfectCheckDelay = g_time - 1;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.secretFound22, 0);
    CHECK(!IsSecretFound(0, 22));
}

TEST(stages_bonus_dual_tally_counts_both_players)
{
    BonusSetUp();
    g_gameMode = MODE_DUAL;
    g_bonusResultsTime = g_time + 10000;
    g_perfectCheckDelay = g_time + 100000;
    P0.bonusKilled = 3; P0.totalEnemies = 10; P0.bonusTally = 0; P0.bonusTallyTick = 0;
    P1.bonusKilled = 1; P1.totalEnemies = 10; P1.bonusTally = 0; P1.bonusTallyTick = 0;
    P0.score = P1.score = 0;
    for (int i = 0; i < 12; i++)
        UpdateBonusResultsHud();
    CHECK_EQ_INT(P0.bonusTally, 3);
    CHECK_EQ_INT(P1.bonusTally, 1);
    CHECK_EQ_INT(P0.score, 1500);
    CHECK_EQ_INT(P1.score, 500);
    CHECK_STR(g_logBuf, " 1 X 500 = 500 POINTS");
}

// Player two's PERFECT is judged against player one's wave size (as in the original).
TEST(stages_bonus_dual_player_two_perfect_uses_player_one_wave_size)
{
    BonusSetUp();
    g_gameMode = MODE_DUAL;
    g_bonusResultsTime = g_time + 10000;
    g_perfectCheckDelay = g_time - 1;
    P0.totalEnemies = 4; P0.bonusKilled = 0; P0.bonusTally = 0;
    P1.totalEnemies = 9; P1.bonusKilled = 4; P1.bonusTally = 4;
    P1.bonusRoundPoints = 25000;
    P1.perfectDone = 0;
    P1.score = 0;
    P0.bonusTallyTick = P1.bonusTallyTick = 3;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_perfectAwardedP1, 1);
    CHECK_EQ_INT(g_perfectAwardedP0, 0);
    CHECK_EQ_INT(P1.perfectDone, 1);
    CHECK_EQ_INT(P1.score, 25000);
}

// ---- the timed HUD messages ----

TEST(stages_bonus_messages_expire_after_their_time)
{
    BonusSetUp();
    g_bannerMsgTime = g_time - 1;
    g_msgTime = g_time - 1;
    g_alertTextTime = g_time - 1;
    g_msgTimer = g_time - 1;
    g_levelBannerTime = g_time - 1;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_bannerMsgTime, 0);
    CHECK_EQ_INT(g_msgTime, 0);
    CHECK_EQ_INT(g_alertTextTime, 0);
    CHECK_EQ_INT(g_msgTimer, 0);
    CHECK_EQ_INT(g_levelBannerTime, 0);
}

TEST(stages_bonus_messages_stay_until_their_time)
{
    BonusSetUp();
    g_bannerMsgTime = g_time;
    g_msgTime = g_time;
    g_alertTextTime = g_time;
    g_msgTimer = g_time;
    g_levelBannerTime = g_time;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_bannerMsgTime, g_time);
    CHECK_EQ_INT(g_msgTime, g_time);
    CHECK_EQ_INT(g_alertTextTime, g_time);
    CHECK_EQ_INT(g_msgTimer, g_time);
    CHECK_EQ_INT(g_levelBannerTime, g_time);
}

TEST(stages_bonus_warp_alert_blinks_every_70ms)
{
    BonusSetUp();
    g_alertTextTime = g_time + 1000;
    g_warpMsgBlinkTime = 0;
    g_warpMsgBlinkOn = 0;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_warpMsgBlinkOn, 1);
    CHECK_EQ_INT(g_warpMsgBlinkTime, g_time + 70);
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_warpMsgBlinkOn, 1);
    g_time += 71;
    UpdateBonusResultsHud();
    CHECK_EQ_INT(g_warpMsgBlinkOn, 0);
}

TEST(stages_bonus_results_flares_bounce_between_the_edges)
{
    ResultsShowing(0, 10);
    g_bonusFxX = 698;
    g_bonusFxVel = 5;
    UpdateBonusResultsHud();
    CHECK_NEAR(g_bonusFxX, 700, 1e-4);
    CHECK_NEAR(g_bonusFxVel, -5, 1e-4);
    g_bonusFxX = 102;
    UpdateBonusResultsHud();
    CHECK_NEAR(g_bonusFxX, 100, 1e-4);
    CHECK_NEAR(g_bonusFxVel, 5, 1e-4);
}
