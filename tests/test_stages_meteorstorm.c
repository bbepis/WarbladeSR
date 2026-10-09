// Tests for src/game/stages/meteorstorm.c: the meteor-storm race (countdown, speed meter,
// spawning and moving meteors, catching bonus meteors and gems, crashing, and the end-of-race
// reward tiers).
#include "support.h"

#define P0 g_save.players[0]

// Ship 0: gemBase 452, gemStep 8.
enum { GEM_BASE = 452, GEM_STEP = 8 };

static void StormSetUp(void)
{
    BootGame();
    SeedRand(777);
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_scoreMul[0] = 1;
    g_scoreMul[1] = 1;
    g_frameDt = 1;
    g_time = 50000;
    g_cfg.collisionDetail = 0;
    g_autoplay = 0;
    P0.ship = 0;
    P0.raceDistance = 1000;
    P0.starSpeed = 5;
    P0.money = 0;
    P0.score = 0;
    P0.bonusRoundScore = 0;
    P0.drunkModeTimer = 0;
    P0.drunkStreak = 0;
    InitMeteorStormLevel();
    g_state = STATE_BONUS_RACE;
    g_chargeMax = 15;
    g_bonusSpawnTarget = 0;
    g_bonusSpawnRampRate = 0;
}

// The race running (start time passed) with the boost at 5 of 15.
static void Racing(void)
{
    StormSetUp();
    g_raceStartTime = g_time - 1;
    g_raceCountdownStage = 0;
    g_boostCharge = 5;
    g_slowFrameCount = 0;
    g_fastFrameCount = 0;
}

// Finishes the race this frame: the boost (5 of 15) counts as a fast frame, so the speed
// percentage comes out as (1 - slow / (fast + 1)) * 100.
static void FinishRace(int slow, int fast, int drunk, int released)
{
    Racing();
    g_slowFrameCount = slow;
    g_fastFrameCount = fast;
    g_boostReleased = released;
    P0.drunkModeTimer = drunk ? 100 : 0;
    P0.drunkStreak = 3;
    P0.totalEnemies = 17;
    P0.killed = 2;
    g_raceDistanceLeft = 1;
    MeteorStormUpdate();
}

// Per-frame race score at boost 5 of 15: ((int)(5 / 15 * 50) + 1) * 10.
enum { FRAME_SCORE = 170 };

// ---- the intro gate ----

TEST(stages_storm_intro_gate_arms_the_meter_once)
{
    StormSetUp();
    g_bonusMeterStarted = 0;
    g_resultsScreenEndTime = g_time + 1;
    UpdateMeteorStormIntroGate();
    CHECK_EQ_INT(g_raceActive, 1);
    CHECK_EQ_INT(g_introGateScratch, 0);
    CHECK_NEAR(g_meterY, 550, 1e-4);
    CHECK_NEAR(g_meterV, 35, 1e-4);
    CHECK_EQ_INT(g_meterDirUp, 1);
    CHECK_EQ_INT(g_bonusMeterStarted, 1);
    g_meterY = 100;
    UpdateMeteorStormIntroGate();
    CHECK_NEAR(g_meterY, 100, 1e-4);
}

TEST(stages_storm_intro_gate_releases_the_lock_when_over)
{
    StormSetUp();
    g_resultsScreenEndTime = g_time;
    g_transitionLock = 1;
    g_raceActive = 1;
    UpdateMeteorStormIntroGate();
    CHECK_EQ_INT(g_raceActive, 0);
    CHECK_EQ_INT(g_introGateScratch, 1);
    CHECK_EQ_INT(g_transitionLock, 0);
}

// ---- InitMeteorStormLevel ----

TEST(stages_storm_init_sets_up_the_race)
{
    BootGame();
    g_curPlayer = 0;
    g_time = 1000;
    P0.raceDistance = 1234;
    P0.starSpeed = 6;
    for (int i = 0; i < MAX_BONUS_METEORS; i++)
        g_bonusMeteors[i].active = 1;
    g_bonusItemCount = 9;
    InitMeteorStormLevel();
    CHECK_NEAR(g_raceDistanceLeft, 1234, 1e-3);
    CHECK_EQ_INT(g_meterValue, 1234);
    CHECK_EQ_INT(g_raceStartTime, 5000);
    CHECK_EQ_INT(g_raceCountdownStage, 3);
    CHECK_NEAR(P0.savedRaceStarSpeed, 6, 1e-6);
    CHECK_NEAR(g_baseSpeedRamp, 1.5, 1e-6);
    CHECK_NEAR(g_boostCharge, 5, 1e-6);
    CHECK_EQ_INT(g_bonusItemCount, 0);
    for (int i = 0; i < MAX_BONUS_METEORS; i++)
        CHECK_EQ_INT(g_bonusMeteors[i].active, 0);
}

// ---- the countdown ----

TEST(stages_storm_countdown_three_two_one)
{
    StormSetUp();
    g_raceStartTime = g_time + 2500;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_raceCountdownStage, 3);
    g_time = g_raceStartTime - 1999;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_raceCountdownStage, 2);
    CHECK(g_soundQueueCount > 0 && g_soundQueue[g_soundQueueCount - 1].sample == g_sfxVoiceThree);
    g_time = g_raceStartTime - 1001;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_raceCountdownStage, 2);
    g_time = g_raceStartTime - 999;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_raceCountdownStage, 1);
    CHECK(g_soundQueue[g_soundQueueCount - 1].sample == g_sfxVoiceTwo);
    g_time = g_raceStartTime - 10;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_raceCountdownStage, 1);
    g_time = g_raceStartTime - 9;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_raceCountdownStage, 0);
    CHECK(g_soundQueue[g_soundQueueCount - 1].sample == g_sfxVoiceOne);
}

TEST(stages_storm_nothing_moves_before_the_start)
{
    StormSetUp();
    g_raceStartTime = g_time + 3000;
    g_speedPct = 50;
    g_slowFrameCount = 3;
    g_fastFrameCount = 4;
    g_boostReleased = 1;
    MeteorStormUpdate();
    CHECK_NEAR(g_raceDistanceLeft, 1000, 1e-3);
    CHECK_EQ_INT(P0.score, 0);
    CHECK_NEAR(g_speedPct, 0, 1e-6);
    CHECK_EQ_INT(g_slowFrameCount, 0);
    CHECK_EQ_INT(g_fastFrameCount, 0);
    CHECK_EQ_INT(g_boostReleased, 0);
    CHECK_NEAR(g_baseSpeedRamp, 1.5, 1e-6);
}

// ---- the race ----

TEST(stages_storm_race_scores_and_covers_distance_each_frame)
{
    Racing();
    g_frameDt = 2;
    MeteorStormUpdate();
    CHECK_EQ_INT(P0.score, FRAME_SCORE);
    CHECK_EQ_INT(P0.bonusRoundScore, FRAME_SCORE);
    CHECK_NEAR(g_raceDistanceLeft, 1000 - (5 / 2.0 + 3) * 2, 1e-3);
    CHECK_EQ_INT(g_meterValue, 989);
    CHECK_NEAR(g_baseSpeedRamp, 1.5 + 2 * 0.0012, 1e-5);
    CHECK_NEAR(P0.starSpeed, 5 + (g_baseSpeedRamp + 5) * 2, 1e-4);
}

TEST(stages_storm_race_score_grows_with_the_boost)
{
    Racing();
    g_boostCharge = 15;
    MeteorStormUpdate();
    CHECK_EQ_INT(P0.score, (50 + 1) * 10);
    g_scoreMul[0] = 2;
    P0.score = 0;
    P0.bonusRoundScore = 0;
    g_boostCharge = 0;
    MeteorStormUpdate();
    CHECK_EQ_INT(P0.score, 20);
    CHECK_EQ_INT(P0.bonusRoundScore, 20);
}

TEST(stages_storm_race_repairs_a_zero_score_multiplier)
{
    Racing();
    g_scoreMul[0] = 0;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_scoreMul[0], 1);
    CHECK_EQ_INT(P0.score, FRAME_SCORE);
}

TEST(stages_storm_speed_percentage_counts_fast_frames)
{
    Racing();
    g_slowFrameCount = 1;
    g_fastFrameCount = 3;
    MeteorStormUpdate();        // boost 5 >= 15 / 7 * 2: a fast frame
    CHECK_EQ_INT(g_fastFrameCount, 4);
    CHECK_EQ_INT(g_slowFrameCount, 1);
    CHECK_NEAR(g_speedPct, 75, 1e-3);
    g_boostCharge = 4;          // below 4.29: slow
    MeteorStormUpdate();
    CHECK_EQ_INT(g_slowFrameCount, 2);
    CHECK_NEAR(g_speedPct, 50, 1e-3);
    g_slowFrameCount = 9;
    MeteorStormUpdate();
    CHECK_NEAR(g_speedPct, 0, 1e-6);    // more slow than fast frames
}

TEST(stages_storm_spawns_meteors_up_to_the_target)
{
    Racing();
    g_bonusSpawnTarget = 2;
    g_bonusSpawnRampRate = 0;
    g_bonusRareChance = 0;
    MeteorStormUpdate();
    CHECK_EQ_INT(g_bonusItemCount, 1);
    CHECK_EQ_INT(g_bonusMeteors[0].active, 1);
    MeteorStormUpdate();
    CHECK_EQ_INT(g_bonusItemCount, 2);
    CHECK_EQ_INT(g_bonusMeteors[1].active, 1);
    MeteorStormUpdate();
    CHECK_EQ_INT(g_bonusItemCount, 2);
    CHECK_EQ_INT(g_bonusMeteors[2].active, 0);
}

TEST(stages_storm_spawn_target_ramps_with_the_boost)
{
    Racing();
    g_bonusSpawnTarget = 1;
    g_bonusSpawnRampRate = 0.5f;
    g_boostCharge = 15;
    g_bonusItemCount = 100;     // no spawning
    MeteorStormUpdate();
    CHECK_NEAR(g_bonusSpawnTarget, 1 + 0.5 * (1 + 15 / 30.0), 1e-5);
}

TEST(stages_storm_meteors_fall_with_the_race_speed)
{
    Racing();
    g_bonusItemCount = 100;
    FallingHazard *m = &g_bonusMeteors[5];
    m->active = 1;
    m->type = METEOR_HAZARD;
    m->x = 100;
    m->y = 50;
    m->vx = 0.25f;
    m->vy = 2;
    MeteorStormUpdate();
    CHECK_NEAR(m->x, 100.25, 1e-4);
    CHECK_NEAR(m->y, 50 + 2 + 1.5 + 5, 1e-4);
}

TEST(stages_storm_meteor_that_falls_off_is_replaced)
{
    Racing();
    g_bonusRareChance = 0;
    g_bonusSpawnTarget = 1;
    g_bonusItemCount = 1;
    FallingHazard *m = &g_bonusMeteors[5];
    m->active = 1;
    m->type = METEOR_HAZARD;
    m->x = 100;
    m->y = 595;
    m->vy = 2;
    MeteorStormUpdate();
    CHECK_EQ_INT(m->active, 0);
    CHECK_EQ_INT(g_bonusMeteors[0].active, 1);
    CHECK_EQ_INT(g_bonusMeteors[0].type, METEOR_HAZARD);
    CHECK_EQ_INT(g_bonusItemCount, 1);
}

TEST(stages_storm_bonus_meteor_animates_through_its_frames)
{
    Racing();
    g_bonusItemCount = 100;
    FallingHazard *m = &g_bonusMeteors[2];
    m->active = 1;
    m->type = METEOR_MONEY;
    m->y = 0;
    m->sy = 333;
    m->animTimer = 0.5f;
    MeteorStormUpdate();
    CHECK_EQ_INT(m->sy, 0);         // 333 + 37 = 370 wraps
    CHECK_NEAR(m->animTimer, 5, 1e-6);
    FallingHazard *d = &g_bonusMeteors[3];
    d->active = 1;
    d->type = 2;
    d->y = 0;
    d->sy = 459;
    d->animTimer = 0.5f;
    MeteorStormUpdate();
    CHECK_EQ_INT(d->sy, 510);
    d->animTimer = 0.5f;
    MeteorStormUpdate();
    CHECK_EQ_INT(d->sy, 0);         // 561 wraps
}

TEST(stages_storm_bonus_meteor_leaves_at_the_bottom)
{
    Racing();
    g_bonusItemCount = 100;
    FallingHazard *m = &g_bonusMeteors[2];
    m->active = 1;
    m->type = METEOR_MONEY;
    m->y = 599;
    m->vy = 0;
    m->animTimer = 5;
    MeteorStormUpdate();
    CHECK_EQ_INT(m->active, 0);
}

// ---- the end of the race ----

TEST(stages_storm_finish_ends_the_stage)
{
    FinishRace(50, 99, 0, 0);
    CHECK_EQ_INT(g_state, STATE_METEOR_STORM);
    CHECK_NEAR(g_raceDistanceLeft, 0, 1e-6);
    CHECK_EQ_INT(P0.raceDistance, 1000 + 134);
    CHECK_EQ_INT(P0.done, 1);
    CHECK_EQ_INT(P0.doneTime, g_time + 3000);
    CHECK_EQ_INT(P0.killed, 17);
    CHECK_EQ_INT(g_transitionLock, 1);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 3000);
    CHECK_EQ_INT(g_drunk, 0);
}

TEST(stages_storm_finish_below_90_percent_pays_the_extra_bonus)
{
    FinishRace(50, 99, 0, 0);    // 50 %
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 1000000);
    CHECK_EQ_INT(P0.bonusRoundScore, FRAME_SCORE + 1000000);
    CHECK_EQ_INT(P0.money, 0);
    CHECK_EQ_INT(P0.drunkStreak, 0);
}

// NOTE: the results screen announces "1.000.000 POINTS AND 1.000 CREDITS" for this tier, but
// SUPER_SCORE awards 2.000.000 points.
TEST(stages_storm_finish_at_90_percent_pays_the_super_bonus)
{
    FinishRace(10, 99, 0, 0);    // 90 %
    CHECK_NEAR(g_speedPct, 90, 1e-3);
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 2000000);
    CHECK_EQ_INT(P0.money, 1000);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 3000);
}

TEST(stages_storm_finish_at_99_percent_pays_the_extreme_bonus)
{
    FinishRace(1, 99, 0, 0);     // 99 %
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 5000000);
    CHECK_EQ_INT(P0.money, 5000);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 8000);
    CHECK_EQ_INT(P0.drunkStreak, 4);
}

TEST(stages_storm_finish_at_100_percent_pays_the_mega_bonus)
{
    FinishRace(0, 99, 0, 0);
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 10000000);
    CHECK_EQ_INT(P0.money, 25000);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 10000);
}

TEST(stages_storm_finish_at_100_percent_after_releasing_the_boost_is_extreme)
{
    FinishRace(0, 99, 0, 1);
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 5000000);
    CHECK_EQ_INT(P0.money, 5000);
}

TEST(stages_storm_drunk_finish_below_90_percent)
{
    FinishRace(50, 99, 1, 0);
    CHECK_EQ_INT(g_drunk, 1);
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 2000000);
    CHECK_EQ_INT(P0.money, 0);
}

TEST(stages_storm_drunk_finish_at_90_percent)
{
    FinishRace(10, 99, 1, 0);
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 5000000);
    CHECK_EQ_INT(P0.money, 5000);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 8000);
}

TEST(stages_storm_drunk_finish_at_99_percent)
{
    FinishRace(1, 99, 1, 0);
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 10000000);
    CHECK_EQ_INT(P0.money, 10000);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 8000);
}

TEST(stages_storm_drunk_finish_at_100_percent)
{
    FinishRace(0, 99, 1, 0);
    CHECK_EQ_INT(P0.score, FRAME_SCORE + 20000000);
    CHECK_EQ_INT(P0.money, 50000);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 10000);
}

TEST(stages_storm_finish_money_is_capped)
{
    Racing();
    P0.money = 90000;
    P0.moneyMax = 99990;
    g_moneyMax = 0;
    g_raceDistanceLeft = 1;
    MeteorStormUpdate();         // 100 %: +25000
    CHECK_EQ_INT(P0.money, 99990);
    CHECK_EQ_INT(g_moneyMax, 99990);
}

TEST(stages_storm_fifth_fast_finish_in_a_row_wins_a_medal)
{
    StormSetUp();
    g_profileIndex = 0;
    CHECK_EQ_INT(GetMedals(0) & MEDAL_SPEED_STREAK, 0);
    for (int k = 0; k < 4; k++) {
        FinishRace(0, 99, 0, 0);
        g_profileIndex = 0;
        P0.drunkStreak = k + 1;
    }
    CHECK_EQ_INT(GetMedals(0) & MEDAL_SPEED_STREAK, 0);
    Racing();
    g_profileIndex = 0;
    P0.drunkStreak = 4;
    g_raceDistanceLeft = 1;
    MeteorStormUpdate();
    CHECK_EQ_INT(P0.drunkStreak, 5);
    CHECK_NE_INT(GetMedals(0) & MEDAL_SPEED_STREAK, 0);
}

TEST(stages_storm_drunk_fast_finish_wins_a_medal_and_a_secret)
{
    Racing();
    g_profileIndex = 0;
    P0.drunkModeTimer = 100;
    P0.secretFound29 = 0;
    g_raceDistanceLeft = 1;
    MeteorStormUpdate();
    CHECK_NE_INT(GetMedals(0) & MEDAL_DRUNK_FINISH, 0);
    CHECK_EQ_INT(P0.secretFound29, 1);
    CHECK(IsSecretFound(0, 29));
}

TEST(stages_storm_sober_finish_wins_no_drunk_medal)
{
    Racing();
    g_profileIndex = 0;
    P0.drunkModeTimer = 0;
    g_raceDistanceLeft = 1;
    MeteorStormUpdate();
    CHECK_EQ_INT(GetMedals(0) & MEDAL_DRUNK_FINISH, 0);
    CHECK(!IsSecretFound(0, 29));
}

TEST(stages_storm_finish_with_a_multiplier_marks_secret_5)
{
    Racing();
    g_profileIndex = 0;
    g_scoreMul[0] = 2;
    g_raceDistanceLeft = 1;
    MeteorStormUpdate();
    CHECK_EQ_INT(P0.secretFound05, 1);
    CHECK(IsSecretFound(0, 5));
}

// ---- SpawnMeteor ----

TEST(stages_storm_spawn_a_rock)
{
    StormSetUp();
    g_bonusRareChance = 0;
    for (int k = 0; k < 200; k++) {
        SpawnMeteor(7);
        FallingHazard *m = &g_bonusMeteors[7];
        CHECK_EQ_INT(m->type, METEOR_HAZARD);
        CHECK(m->graphic == g_bgGraphic);
        CHECK(m->hma == g_hmaMeteors);
        CHECK_EQ_INT(m->vol, g_bonusVolume[7]);
        int v = -1;
        for (int j = 0; j < NUM_METEOR_VARIANTS; j++)
            if (g_meteorSrcX[j] == m->sx && g_bonusSrcY[j] == m->sy && g_bonusGfxW[j] == m->w &&
                g_bonusGfxH[j] == m->h)
                v = j;
        CHECK(v >= 0);
        CHECK_EQ_INT(m->x1_38, m->sx);
        CHECK_EQ_INT(m->y1_3c, m->sy);
        CHECK_EQ_INT(m->x2_40, m->sx + m->w);
        CHECK_EQ_INT(m->y2_44, m->sy + m->h);
        CHECK(m->x >= -30 && m->x <= 700);
        CHECK(m->y >= -700 && m->y <= -200);
        CHECK(m->vx >= -0.3f && m->vx <= 0.3f);
        CHECK(m->vy >= 1 && m->vy <= 4);
    }
}

TEST(stages_storm_spawn_rare_items_half_diamonds)
{
    StormSetUp();
    g_bonusRareChance = 100;
    int diamonds = 0, n = 2000;
    for (int k = 0; k < n; k++) {
        SpawnMeteor(0);
        FallingHazard *m = &g_bonusMeteors[0];
        if (m->type == 2) {
            diamonds++;
            CHECK(m->graphic == g_gfxDiamondBig);
            CHECK(m->sx == 0 || m->sx == 0x50 || m->sx == 0xa0);
            CHECK_EQ_INT(m->w, 0x50);
            CHECK_EQ_INT(m->h, 0x33);
        } else {
            CHECK_EQ_INT(m->type, METEOR_MONEY);
            CHECK(m->graphic == g_gfxMeteorBonuses);
            CHECK_EQ_INT(m->w, 0x40);
            CHECK_EQ_INT(m->h, 0x25);
        }
        CHECK_EQ_INT(m->sy, 0);
        CHECK_NEAR(m->animTimer, 5, 1e-6);
    }
    CHECK_NEAR(diamonds / (double)n, 0.5, 0.04);
}

// The six bonus kinds by weight {50, 30, 10, 150, 80, 40} over RandRange(0, 359), walked
// with `r > weight` (so the first bucket gets one extra value and the last one two fewer).
TEST(stages_storm_spawn_bonus_kinds_by_weight)
{
    StormSetUp();
    g_bonusRareChance = 100;
    int count[6] = {0}, n = 0;
    while (n < 6000) {
        SpawnMeteor(0);
        if (g_bonusMeteors[0].type != METEOR_MONEY)
            continue;
        int k = g_bonusMeteors[0].sx / 64;
        CHECK(k >= 0 && k < 6 && g_bonusMeteors[0].sx % 64 == 0);
        count[k]++;
        n++;
    }
    static const double expect[6] = {51, 30, 10, 150, 80, 38};
    for (int k = 0; k < 6; k++)
        CHECK_MSG(fabs(count[k] / (double)n - expect[k] / 359) < 0.02, "kind %d: %d of %d", k,
                  count[k], n);
}

// ---- MeteorStormCollide ----

// Puts a bonus meteor of `type`/`sx` right on the ship (at 200, 400).
static FallingHazard *OnShip(int type, int sx)
{
    P0.x = 200;
    P0.y = 400;
    FallingHazard *m = &g_bonusMeteors[4];
    m->active = 1;
    m->type = type;
    m->sx = sx;
    m->w = 0x40;
    m->h = 0x25;
    m->x = 190;
    m->y = 390;
    return m;
}

TEST(stages_storm_hitting_a_rock_crashes_the_race)
{
    Racing();
    P0.drunkStreak = 3;
    P0.savedRaceStarSpeed = 9;
    P0.starSpeed = 30;
    g_raceDistanceLeft = 500;
    int thumps = FakePlayCount("thumpbig");
    OnShip(METEOR_HAZARD, 0);
    MeteorStormCollide();
    CHECK_NEAR(g_raceDistanceLeft, 1, 1e-6);
    CHECK_EQ_INT(g_state, STATE_METEOR_STORM);
    CHECK_EQ_INT(P0.drunkStreak, 0);
    CHECK_NEAR(P0.starSpeed, 9, 1e-6);
    CHECK_EQ_INT(g_resultsScreenEndTime, g_time + 3000);
    CHECK_EQ_INT(g_transitionLock, 1);
    CHECK_EQ_INT(g_meterDirUp, 0);
    CHECK_EQ_INT(FakePlayCount("thumpbig"), thumps + 1);
}

TEST(stages_storm_missing_a_rock_changes_nothing)
{
    Racing();
    g_raceDistanceLeft = 500;
    FallingHazard *m = OnShip(METEOR_HAZARD, 0);
    m->x = 240;     // just right of the ship (200..240)
    MeteorStormCollide();
    CHECK_NEAR(g_raceDistanceLeft, 500, 1e-6);
    CHECK_EQ_INT(g_state, STATE_BONUS_RACE);
    m->x = 239;
    m->y = 400 + 27;     // just below it (400..427)
    MeteorStormCollide();
    CHECK_EQ_INT(g_state, STATE_BONUS_RACE);
    m->y = 426;
    MeteorStormCollide();
    CHECK_EQ_INT(g_state, STATE_METEOR_STORM);
}

static void CheckMoneyMeteor(int sx, int money, int score)
{
    Racing();
    P0.money = 100;
    P0.moneyMax = 99990;
    int bings = FakePlayCount("bing");
    FallingHazard *m = OnShip(METEOR_MONEY, sx);
    MeteorStormCollide();
    CHECK_EQ_INT(P0.money, 100 + money);
    CHECK_EQ_INT(P0.score, score);
    CHECK_EQ_INT(P0.bonusRoundScore, score);
    CHECK_EQ_INT(m->active, 0);
    CHECK_EQ_INT(FakePlayCount("bing"), bings + 1);
}

TEST(stages_storm_money_meteor_50)
{
    CheckMoneyMeteor(0, 50, 0);
}

TEST(stages_storm_money_meteor_100)
{
    CheckMoneyMeteor(0x40, 100, 0);
}

TEST(stages_storm_money_meteor_250)
{
    CheckMoneyMeteor(0x80, 250, 0);
}

TEST(stages_storm_score_meteor_1000)
{
    CheckMoneyMeteor(0xc0, 0, 1000);
}

TEST(stages_storm_score_meteor_5000)
{
    CheckMoneyMeteor(0x100, 0, 5000);
}

TEST(stages_storm_score_meteor_10000)
{
    CheckMoneyMeteor(0x140, 0, 10000);
}

TEST(stages_storm_money_meteor_respects_the_money_cap)
{
    Racing();
    P0.money = 99980;
    P0.moneyMax = 99990;
    g_moneyMax = 5;
    OnShip(METEOR_MONEY, 0x80);
    MeteorStormCollide();
    CHECK_EQ_INT(P0.money, 99990);
    CHECK_EQ_INT(g_moneyMax, 99990);
}

static void CheckGem(int sx, int score)
{
    Racing();
    P0.gems = GEM_BASE + GEM_STEP * 46;     // past 50: no gem drop (only every 100)
    FallingHazard *m = OnShip(2, sx);
    MeteorStormCollide();
    CHECK_EQ_INT(P0.score, score);
    CHECK_EQ_INT(P0.bonusRoundScore, score);
    CHECK_EQ_INT(P0.gems, GEM_BASE + GEM_STEP * 51);
    CHECK_EQ_INT(m->active, 0);
    CHECK_EQ_INT(g_state, STATE_BONUS_RACE);
}

TEST(stages_storm_gem_small_scores_2500_and_five_gems)
{
    CheckGem(0, 2500);
}

TEST(stages_storm_gem_medium_scores_5000_and_five_gems)
{
    CheckGem(0x50, 5000);
}

TEST(stages_storm_gem_big_scores_10000_and_five_gems)
{
    CheckGem(0xa0, 10000);
}

TEST(stages_storm_hundredth_gem_starts_a_gem_drop)
{
    Racing();
    P0.gems = GEM_BASE + GEM_STEP * 97;
    P0.bonusRoundScore = 0;
    P0.bonusHighScore = 0;
    g_superGemDrop = 0;
    OnShip(2, 0x50);
    MeteorStormCollide();
    CHECK_EQ_INT(g_state, STATE_GEM_DROP);
    CHECK_EQ_INT(P0.gems, GEM_BASE + GEM_STEP * 102);
    CHECK_EQ_INT(g_superGemDrop, 0);
    CHECK_STR(g_alertMsg, "G E M   D R O P");
    CHECK_EQ_INT(g_gemDropIntroTimer, g_time + 4000);
    CHECK_NEAR(g_levelDist, 2680, 1e-3);    // InitGemDropLevel
    CHECK_EQ_INT(P0.bonusHighScore, 5000);  // BankBonusScore
}

TEST(stages_storm_thousandth_gem_starts_a_super_gem_drop)
{
    Racing();
    P0.gems = GEM_BASE + GEM_STEP * 998;
    OnShip(2, 0);
    MeteorStormCollide();
    CHECK_EQ_INT(g_state, STATE_GEM_DROP);
    CHECK_EQ_INT(g_superGemDrop, 1);
    CHECK_EQ_INT(P0.gems, GEM_BASE + GEM_STEP * 3);
    CHECK_STR(g_alertMsg, "S U P E R   G E M   D R O P");
    CHECK_NEAR(g_meterY, 550, 1e-4);
}

// ---- DrawSpeedMeter / DrawMeteorStorm ----

TEST(stages_storm_speed_meter_text)
{
    StormSetUp();
    g_speedPctCache = -1;
    g_speedPct = 75.9f;
    g_chargeMax = 0;
    DrawSpeedMeter();
    CHECK_STR(g_speedPctText, " 75%");
    CHECK_EQ_INT(g_speedPctCache, 75);
    CHECK_NEAR(g_chargeMax, 15, 1e-6);
}

TEST(stages_storm_results_screen_shows_the_speed)
{
    StormSetUp();
    g_raceActive = 1;
    g_raceDistanceLeft = 0;
    g_speedPct = 93;
    g_drunk = 0;
    DrawMeteorStorm();
    CHECK_STR(g_logBuf, "SPEED PERCENTAGE  93");
    CHECK_EQ_INT(g_meterShowUntil, g_time + 3000);
}

TEST(stages_storm_crash_screen_shows_the_meteor_bonus)
{
    StormSetUp();
    g_raceActive = 1;
    g_raceDistanceLeft = 1;
    g_speedPct = 40;
    DrawMeteorStorm();
    CHECK_STR(g_logBuf, "SPEED PERCENTAGE  40");
}

TEST(stages_storm_dual_follows_the_turn_player)
{
    StormSetUp();
    g_gameMode = MODE_DUAL;
    g_vsTurnPlayer = 1;
    g_curPlayer = 0;
    g_raceActive = 0;
    DrawMeteorStorm();
    CHECK_EQ_INT(g_curPlayer, 1);
}
