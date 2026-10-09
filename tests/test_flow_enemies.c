// test_flow_enemies.c: What happens during a level: enemies (src/game/enemies.c), the hurry-up
// mothership (hurryup.c), the warp malfunction and hyperspace (warp.c), enemy shots
// (levelobj.c), player shots and debris (mapobj.c), and the end-of-game tally (gameover.c).
// Each test boots the game with a synthetic level and starts a single-player game.
#include <stdlib.h>
#include "support.h"

// ---- helpers ----

static LevelRaw *NewLevel(int type, const char *name)
{
    LevelRaw *L = calloc(1, sizeof *L);
    L->type = type;
    L->name1[0] = (char)strlen(name);
    memcpy(L->name1 + 1, name, strlen(name));
    strcpy(L->gfx[0], "gfx\\aliens\\Alien1.bmp");
    L->count = 1;
    L->grp[0].count = 1;
    L->grp[0].sub[0].type = 1;
    L->grp[0].sub[0].hp = 10;
    return L;
}

static void PutLevel(const char *fmt, int n, LevelRaw *L)
{
    char name[64];
    snprintf(name, sizeof name, fmt, n);
    FakePacAdd(name, L, sizeof *L);
    free(L);
}

// Boots with a level 1 of `type`, starts a single-player game and clears the field: the
// tests then place their own enemies. g_frameDt is 1 and the RNG seeded.
static void Setup(int type)
{
    PutLevel("classic_level_%03d.lvd", 1, NewLevel(type, "ONE"));
    PutLevel("classic_level_%03d.lvd", 2, NewLevel(LEVEL_WAVE, "TWO"));
    PutLevel("malfunction_%02d.lvd", 1, NewLevel(LEVEL_BONUS_WAVE, "MAL"));
    BootGame();
    g_gameMode = MODE_SINGLE;
    NewGame(true);
    g_save.players[0].started = 1;
    StartLevel();
    ResetAllObjects();
    g_state = STATE_PLAYING;
    g_timerA = 0;
    g_frameDt = 1.0f;
    g_curPlayer = 0;
    g_save.players[0].freezeTimer = 0;
    g_save.players[0].totalEnemies = 10;
    g_save.players[0].killed = 0;
    g_save.players[0].escaped = 0;
    g_save.players[0].done = 0;
    SeedRand(1234);
}

// An enemy that never fires (a huge fire delay).
static Enemy *PutEnemy(int i, int type, float x, float y)
{
    Enemy *e = &g_enemies[0][i];
    e->active = 1;
    e->type = type;
    e->x = x;
    e->y = y;
    e->fireDelay = 1000000000;
    e->attackDelay = 0;
    e->forcedDir = -1;
    e->pairedEnemyIdx = -1;
    e->hp = 10;
    return e;
}

// ---- patterned aliens ----

TEST(Flow_patterned_alien_moves_by_its_velocity_and_acceleration)
{
    Setup(LEVEL_WAVE);
    g_curLevelData.obj[0][0].holdTime = 50;
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->velX = 2;
    e->velY = 1;
    e->accelX = 0.5f;
    e->accelY = -0.25f;
    e->patternTimer = 1;
    UpdateEnemies();
    CHECK_NEAR(e->x, 102, 1e-4);
    CHECK_NEAR(e->y, 51, 1e-4);
    CHECK_NEAR(e->velX, 2.5, 1e-4);
    CHECK_NEAR(e->velY, 0.75, 1e-4);
    CHECK_NEAR(e->patternTimer, 2, 1e-4);
    CHECK_EQ_INT(e->patternStep, 0);
    UpdateEnemies();
    CHECK_NEAR(e->x, 104.5, 1e-4);
    CHECK_NEAR(e->y, 51.75, 1e-4);
}

TEST(Flow_patterned_alien_waits_for_its_attack_stagger)
{
    Setup(LEVEL_WAVE);
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->velX = 2;
    e->attackStaggerTimer = 3;
    UpdateEnemies();
    CHECK_NEAR(e->x, 100, 0);
    CHECK_NEAR(e->attackStaggerTimer, 2, 1e-4);
    UpdateEnemies();
    UpdateEnemies();
    CHECK_NEAR(e->attackStaggerTimer, 0, 0);
    CHECK_NEAR(e->x, 100, 0);
    UpdateEnemies();
    CHECK_NEAR(e->x, 102, 1e-4);
}

TEST(Flow_aliens_freeze_during_get_ready_and_the_freeze_item)
{
    Setup(LEVEL_WAVE);
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->velX = 2;
    Enemy *h = PutEnemy(1, ENEMY_HOVER, 200, 50);
    h->hoverX = 100;
    h->hoverY = 50;
    g_timerA = 5;
    UpdateEnemies();
    CHECK_NEAR(e->x, 100, 0);
    CHECK_NEAR(h->x, 200, 0);
    g_timerA = 0;
    g_save.players[0].freezeTimer = 99999999;
    UpdateEnemies();
    CHECK_NEAR(e->x, 100, 0);
    CHECK_NEAR(h->x, 200, 0);
    g_save.players[0].freezeTimer = 0;
    UpdateEnemies();
    CHECK_NEAR(e->x, 102, 1e-4);
    CHECK_NEAR(h->x, 195, 1e-4);
}

TEST(Flow_patterned_alien_takes_the_next_path_step_after_its_hold_time)
{
    Setup(LEVEL_WAVE);
    g_curLevelData.obj[2][0].holdTime = 3;
    g_curLevelData.obj[2][1] = (LvObj){.pathX = 0.75f, .pathY = -1.5f, .cmd = 0, .holdTime = 50};
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->groupIndex = 2;
    e->patternTimer = 1;
    UpdateEnemies();
    UpdateEnemies();
    CHECK_EQ_INT(e->patternStep, 0);
    UpdateEnemies();            // patternTimer 4 > 3
    CHECK_EQ_INT(e->patternStep, 1);
    CHECK_NEAR(e->accelX, 0.75, 1e-6);
    CHECK_NEAR(e->accelY, -1.5, 1e-6);
    CHECK_NEAR(e->patternTimer, 1, 1e-6);
}

TEST(Flow_patterned_alien_path_is_mirrored_on_mirror_levels)
{
    Setup(LEVEL_WAVE);
    g_mirrorLevel = 1;
    g_curLevelData.obj[0][1] = (LvObj){.pathX = 0.75f, .holdTime = 50};
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->patternTimer = 5;
    UpdateEnemies();
    CHECK_NEAR(e->accelX, -0.75, 1e-6);
}

TEST(Flow_flash_step_makes_the_alien_flash)
{
    Setup(LEVEL_WAVE);
    g_curLevelData.obj[0][1] = (LvObj){.cmd = 2, .holdTime = 50};
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->patternTimer = 5;
    UpdateEnemies();
    CHECK_EQ_INT(e->flashActive, 1);
    CHECK_NEAR(e->flashTimer, 15, 0);
}

TEST(Flow_stop_step_halts_the_alien_mid_path)
{
    Setup(LEVEL_WAVE);
    g_curLevelData.obj[0][1] = (LvObj){.pathX = 1, .cmd = 1, .holdTime = 40};
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->velX = 3;
    e->velY = 2;
    e->patternTimer = 5;
    UpdateEnemies();
    CHECK_EQ_INT(e->type, ENEMY_PATTERNED);
    CHECK_NEAR(e->velX, 0, 0);
    CHECK_NEAR(e->velY, 0, 0);
    CHECK_NEAR(e->accelX, 0, 0);
    CHECK_NE_INT(e->forcedDir, -1);
    float x = e->x;
    UpdateEnemies();
    CHECK_NEAR(e->x, x, 0);
}

// An alien reaching the last step of its path (STOP_TURN with hold time 100).
static Enemy *EndOfPath(int levelType)
{
    g_curLevelData.type = levelType;
    g_curLevelData.obj[0][1] = (LvObj){.cmd = 1, .holdTime = 100};
    g_groupEnemyCount[0] = 4;
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->patternStep = 0;
    e->patternTimer = 5;
    UpdateEnemies();
    return e;
}

TEST(Flow_wave_aliens_hover_at_the_end_of_their_path)
{
    Setup(LEVEL_WAVE);
    Enemy *e = EndOfPath(LEVEL_WAVE);
    CHECK_EQ_INT(e->type, ENEMY_HOVER);
    CHECK_EQ_INT(e->active, 1);
    CHECK_EQ_INT(g_groupEnemyCount[0], 0);
    e = EndOfPath(LEVEL_WAVE_AIMED);
    CHECK_EQ_INT(e->type, ENEMY_HOVER);
}

TEST(Flow_race_aliens_escape_at_the_end_of_their_path)
{
    Setup(LEVEL_RACE);
    Enemy *e = EndOfPath(LEVEL_RACE);
    CHECK_EQ_INT(e->active, 0);
    CHECK_EQ_INT(g_save.players[0].escaped, 1);
}

TEST(Flow_bonus_wave_aliens_fly_off_at_the_end_of_their_path)
{
    Setup(LEVEL_BONUS_WAVE);
    Enemy *e = EndOfPath(LEVEL_BONUS_WAVE);
    CHECK_EQ_INT(e->type, ENEMY_ESCAPER);
    CHECK_EQ_INT(e->active, 1);
    CHECK(e->descendAccelY >= 0.2f && e->descendAccelY < 0.4f);
    CHECK_NEAR(e->descendVelY, 0, 0);
}

TEST(Flow_escape_step_removes_the_alien_and_counts_an_escape)
{
    Setup(LEVEL_WAVE);
    g_curLevelData.obj[0][1] = (LvObj){.cmd = 6, .holdTime = 50};
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->patternTimer = 5;
    UpdateEnemies();
    CHECK_EQ_INT(e->active, 0);
    CHECK_EQ_INT(g_save.players[0].escaped, 1);
}

// ---- hovering and diving ----

TEST(Flow_hovering_alien_eases_toward_its_formation_slot)
{
    Setup(LEVEL_WAVE);
    Enemy *e = PutEnemy(0, ENEMY_HOVER, 200, 150);
    e->hoverX = 100;
    e->hoverY = 50;
    UpdateEnemies();
    CHECK_NEAR(e->x, 195, 1e-4);
    CHECK_NEAR(e->y, 145, 1e-4);
    UpdateEnemies();
    CHECK_NEAR(e->x, 190.25, 1e-4);
    CHECK_EQ_INT(e->type, ENEMY_HOVER);
}

TEST(Flow_hovering_alien_follows_the_formation_sway)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].enemySwayX = 2;
    g_save.players[0].enemySwayVelX = 0.5f;
    g_save.players[0].enemySwayAccel = 0.25f;
    g_save.players[0].enemySwayMax = 9;
    g_save.players[0].enemySwayMin = -9;
    g_save.players[0].enemySwayY = 3;
    Enemy *e = PutEnemy(0, ENEMY_HOVER, 100, 50);
    e->hoverX = 100;
    e->hoverY = 50;
    UpdateEnemies();
    CHECK_NEAR(g_save.players[0].enemySwayX, 2.5, 1e-5);
    CHECK_NEAR(g_save.players[0].enemySwayVelX, 0.75, 1e-5);
    CHECK_NEAR(e->offsetX, 2.5 - 16, 1e-5);
    CHECK_NEAR(e->offsetY, 3, 1e-5);
    g_save.players[0].enemySwayX = 8.9f;
    g_save.players[0].enemySwayVelX = 1;
    UpdateEnemies();
    CHECK_NEAR(g_save.players[0].enemySwayX, 9, 1e-5);
    CHECK_NEAR(g_save.players[0].enemySwayVelX, -1.25, 1e-5);
}

TEST(Flow_hovering_alien_pushed_off_screen_turns_into_a_flyby)
{
    Setup(LEVEL_WAVE);
    Enemy *e = PutEnemy(0, ENEMY_HOVER, 1000, 50);
    e->hoverX = 1000;
    e->hoverY = 50;
    UpdateEnemies();
    CHECK_EQ_INT(e->type, ENEMY_FLYBY);
    CHECK(e->y >= -100 && e->y <= -50);
    CHECK(e->x >= 0 && e->x < 800);
}

static void OnePattern(void)
{
    memset(&g_patterns[0], 0, sizeof g_patterns[0]);
    g_patterns[0].unused5 = 512;        // start velocity (2, -1)
    g_patterns[0].unused6 = -256;
    g_patterns[0].entries[0] = (PatternPt){.x = 128, .y = 64, .tParam = 3};
    g_patterns[0].entries[1] = (PatternPt){.x = -256, .y = 0, .tParam = 2};
    g_patterns[0].entries[2] = (PatternPt){.tParam = 0};
    g_patterns[0].count = 3;
    g_patternCount = 1;
}

TEST(Flow_hovering_alien_dives_along_an_attack_pattern)
{
    Setup(LEVEL_WAVE);
    OnePattern();
    g_diffEnemyFireChance = 0;
    Enemy *e = PutEnemy(0, ENEMY_HOVER, 100, 50);
    e->hoverX = 100;
    e->hoverY = 50;
    e->attackDelay = 2;     // RandRange(0, 2) == 1: a 50% chance per frame
    for (int f = 0; f < 50 && e->type == ENEMY_HOVER; f++)
        UpdateEnemies();
    CHECK_EQ_INT(e->type, ENEMY_DIVING);
    CHECK_EQ_INT(e->patternId, 0);
    CHECK_NEAR(e->velX, 2, 1e-6);
    CHECK_NEAR(e->velY, -1, 1e-6);
    CHECK_NEAR(e->accelX, 0.5, 1e-6);
    CHECK_NEAR(e->accelY, 0.25, 1e-6);
    CHECK_EQ_INT(e->patternStep, 0);
}

TEST(Flow_diving_alien_returns_to_its_slot_at_the_end_of_the_pattern)
{
    Setup(LEVEL_WAVE);
    OnePattern();
    Enemy *e = PutEnemy(0, ENEMY_DIVING, 100, 50);
    e->patternId = 0;
    e->velX = 2;
    e->accelX = 0.5f;
    e->patternTimer = 1;
    e->offsetX = 10;
    e->offsetY = 4;
    UpdateEnemies();
    CHECK_NEAR(e->x, 102, 1e-4);
    CHECK_NEAR(e->velX, 2.5, 1e-4);
    UpdateEnemies();
    CHECK_EQ_INT(e->patternStep, 0);
    UpdateEnemies();        // timer 4 > 3: step 1
    CHECK_EQ_INT(e->patternStep, 1);
    CHECK_NEAR(e->accelX, -1, 1e-6);
    CHECK_NEAR(e->accelY, 0, 1e-6);
    UpdateEnemies();
    CHECK_EQ_INT(e->type, ENEMY_DIVING);
    UpdateEnemies();        // timer 3 > 2: step 2, whose tParam 0 ends the dive
    CHECK_EQ_INT(e->type, ENEMY_HOVER);
    CHECK_EQ_INT(e->patternStep, 2);
}

TEST(Flow_diving_alien_escapes_on_an_escape_step)
{
    Setup(LEVEL_WAVE);
    OnePattern();
    g_patterns[0].entries[1].type = 6;
    Enemy *e = PutEnemy(0, ENEMY_DIVING, 100, 50);
    e->patternTimer = 3;
    UpdateEnemies();
    CHECK_EQ_INT(e->active, 0);
    CHECK_EQ_INT(g_save.players[0].escaped, 1);
}

// ---- flyby and escaper ----

TEST(Flow_flyby_alien_wraps_back_to_the_top)
{
    Setup(LEVEL_WAVE);
    Enemy *e = PutEnemy(0, ENEMY_FLYBY, 300, 600 + 15 - 2);
    e->velY = 3;
    e->zigzagTimer = 100;
    UpdateEnemies();
    CHECK_NEAR(e->y, -50, 0);
    CHECK_EQ_INT(e->active, 1);
    CHECK_EQ_INT(g_save.players[0].escaped, 0);
}

TEST(Flow_flyby_alien_escapes_during_hyperspace)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].hyperspaceFade = 0.5f;
    g_save.players[0].scrollSpeedY = 0;
    Enemy *e = PutEnemy(0, ENEMY_FLYBY, 300, 614);
    e->velY = 3;
    e->zigzagTimer = 100;
    UpdateEnemies();
    CHECK_EQ_INT(e->active, 0);
    CHECK_EQ_INT(g_save.players[0].escaped, 1);
}

TEST(Flow_escaper_climbs_off_the_top_and_escapes)
{
    Setup(LEVEL_BONUS_WAVE);
    Enemy *e = PutEnemy(0, ENEMY_ESCAPER, 300, 100);
    e->descendVelY = 2;
    e->descendAccelY = 0.5f;
    e->turnTimer2 = 100;
    UpdateEnemies();
    CHECK_NEAR(e->y, 98, 1e-4);
    CHECK_NEAR(e->descendVelY, 2.5, 1e-4);
    CHECK_NEAR(e->descendAccelY, 0.5 / 1.05, 1e-5);
    e->y = g_clipTop - 100 - 32 + 1;
    UpdateEnemies();
    CHECK_EQ_INT(e->active, 0);
    CHECK_EQ_INT(g_save.players[0].escaped, 1);
}

TEST(Flow_aliens_scroll_away_in_hyperspace)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].hyperspaceFade = 0.5f;
    g_save.players[0].scrollSpeedY = 4;
    Enemy *e = PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    e->attackStaggerTimer = 1000;
    UpdateEnemies();
    CHECK_NEAR(e->y, 58, 1e-4);
    e->y = 695;
    UpdateEnemies();
    CHECK_EQ_INT(e->active, 0);
}

TEST(Flow_idle_counter_counts_frames_where_no_alien_acts)
{
    Setup(LEVEL_WAVE);
    g_levelIdleCounter = 0;
    g_enemyActedThisFrame = 0;
    UpdateEnemies();
    UpdateEnemies();
    CHECK_EQ_INT(g_levelIdleCounter, 2);
    PutEnemy(0, ENEMY_PATTERNED, 100, 50);
    UpdateEnemies();
    CHECK_EQ_INT(g_levelIdleCounter, 3);   // the acting frame resets it on the next one
    UpdateEnemies();
    CHECK_EQ_INT(g_levelIdleCounter, 0);
    g_enemies[0][0].active = 0;
    UpdateEnemies();
    g_timerA = 5;
    UpdateEnemies();
    CHECK_EQ_INT(g_levelIdleCounter, 0);
}

// ---- money sucker, elite guard ----

TEST(Flow_SpawnMoneySucker_comes_for_a_rich_player)
{
    Setup(LEVEL_WAVE);
    g_frameDt = 100;            // the per-frame chance becomes certain
    g_save.players[0].money = 2000000000;
    g_time = 300000;
    g_moneySuckerCooldown = 0;
    g_diffHpBonusB = 3;
    SpawnMoneySucker();
    Enemy *e = &g_enemies[0][0];
    CHECK_EQ_INT(e->active, 1);
    CHECK_EQ_INT(e->type, ENEMY_MONEY_SUCKER);
    CHECK_EQ_INT(e->score, 50000);
    CHECK_NEAR(e->hp, g_moneySuckerBaseHp + 6, 0);
    CHECK(e->x == -70 || e->x == 800 + 70);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 11);
    CHECK_EQ_INT(g_moneySuckerCooldown, 300000 + 120000);
    g_moneySuckerCooldown = 0;
    SpawnMoneySucker();         // one at a time
    CHECK_EQ_INT(g_enemies[0][1].active, 0);
}

TEST(Flow_SpawnMoneySucker_waits_for_its_cooldown)
{
    Setup(LEVEL_WAVE);
    g_frameDt = 100;
    g_save.players[0].money = 2000000000;
    g_time = 300000;
    g_moneySuckerCooldown = 300000;
    SpawnMoneySucker();
    CHECK_EQ_INT(g_enemies[0][0].active, 0);
    g_time = 300001;
    SpawnMoneySucker();
    CHECK_EQ_INT(g_enemies[0][0].active, 1);
}

TEST(Flow_SpawnMoneySucker_not_for_poor_players_or_on_boss_levels)
{
    Setup(LEVEL_WAVE);
    g_frameDt = 100;
    g_time = 300000;
    g_save.players[0].money = 750;
    // Make every random gate pass if the 750 threshold is accidentally inclusive.
    g_rngX = g_rngY = g_rngZ = g_rngW = g_rngT = 0;
    SpawnMoneySucker();
    CHECK_EQ_INT(g_enemies[0][0].active, 0);
    g_save.players[0].money = 2000000000;
    g_curLevelData.type = LEVEL_BOSS;
    SpawnMoneySucker();
    CHECK_EQ_INT(g_enemies[0][0].active, 0);
    g_curLevelData.type = LEVEL_WAVE;
    g_save.players[0].done = 1;
    SpawnMoneySucker();
    CHECK_EQ_INT(g_enemies[0][0].active, 0);
}

static int TryElite(int tries)
{
    for (int i = 0; i < tries; i++) {
        SpawnEliteFlyby();
        if (g_enemies[0][0].active)
            return 1;
    }
    return 0;
}

TEST(Flow_SpawnEliteFlyby_appears_after_level_15)
{
    Setup(LEVEL_WAVE);
    g_frameDt = 5000;
    g_save.players[0].level = 26;
    g_lastEliteSpawnLevel = 16;
    g_diffHpBonusB = 2;
    CHECK(TryElite(5000));
    Enemy *e = &g_enemies[0][0];
    CHECK_EQ_INT(e->type, ENEMY_GUARD);
    CHECK_EQ_INT(e->score, 10000000);
    CHECK_NEAR(e->hp, 20 + g_eliteHpBonus, 0);
    CHECK_EQ_INT(g_lastEliteSpawnLevel, 26);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 11);
}

TEST(Flow_SpawnEliteFlyby_not_before_level_16_or_within_10_levels)
{
    Setup(LEVEL_WAVE);
    g_frameDt = 5000;
    g_save.players[0].level = 15;
    g_lastEliteSpawnLevel = 0;
    CHECK(!TryElite(5000));
    g_save.players[0].level = 25;
    g_lastEliteSpawnLevel = 16;
    CHECK(!TryElite(5000));
    g_lastEliteSpawnLevel = 15;
    g_gameMode = MODE_TIME_TRIAL;
    CHECK(!TryElite(5000));
}

TEST(Flow_NearPlayerFireBoost_cuts_values_near_the_player)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].x = 100;
    int seen[4] = {0};
    for (int i = 0; i < 2000; i++) {
        int v = NearPlayerFireBoost(105, 400);   // d 5: 400 or 100
        CHECK_MSG(v == 400 || v == 100, "d 5 gave %d", v);
        seen[v == 100]++;
        v = NearPlayerFireBoost(80, 400);        // d 20: 400 or 200
        CHECK_MSG(v == 400 || v == 200, "d 20 gave %d", v);
        seen[2] += v == 200;
        v = NearPlayerFireBoost(140, 400);       // d 40: 400 or 300
        CHECK_MSG(v == 400 || v == 300, "d 40 gave %d", v);
        seen[3] += v == 300;
        CHECK_EQ_INT(NearPlayerFireBoost(150, 400), 400);
    }
    CHECK(seen[1] > 0 && seen[0] > 0 && seen[2] > 0 && seen[3] > 0);

    // Out-of-range enemies must leave both the value and the shared random stream alone.
    SeedRand(1234);
    const unsigned before[] = {g_rngX, g_rngY, g_rngZ, g_rngW, g_rngT};
    CHECK_EQ_INT(NearPlayerFireBoost(150, 400), 400);
    const unsigned after[] = {g_rngX, g_rngY, g_rngZ, g_rngW, g_rngT};
    CHECK_MEM(after, before, sizeof before);
}

// ---- hurry up ----

TEST(Flow_SetHurryUpTimer_arms_the_current_players_deadline)
{
    Setup(LEVEL_WAVE);
    g_time = 10000;
    g_bonusDuration = 30000;
    SetHurryUpTimer();
    CHECK_EQ_INT(g_save.players[0].time, 40000);
    CHECK_EQ_INT(g_save.players[0].effectDuration, 30000);
    CHECK_EQ_INT(g_lastEventTime, 40000);
    CHECK_EQ_INT(GetHurryUpTimer(), 40000);
    g_gameMode = MODE_DUAL;
    g_curPlayer = 1;
    g_save.players[1].time = 5;
    g_time = 20000;
    SetHurryUpTimer();
    CHECK_EQ_INT(g_save.players[0].time, 50000);
    CHECK_EQ_INT(g_save.players[1].time, 5);
    CHECK_EQ_INT(GetHurryUpTimer(), 50000);
}

TEST(Flow_HurryUp_sends_a_mothership_after_the_deadline)
{
    Setup(LEVEL_WAVE);
    g_time = 10000;
    g_save.players[0].time = 10000;
    g_hurryUpInterval = 100;
    g_guardCount = 1;
    g_diffHpBonusA = 4;
    PutEnemy(0, ENEMY_HOVER, 0, 0);
    HurryUp();
    CHECK_EQ_INT(g_enemies[0][1].active, 0);
    g_time = 10001;
    HurryUp();
    Enemy *e = &g_enemies[0][1];
    CHECK_EQ_INT(e->active, 1);
    CHECK_EQ_INT(e->type, ENEMY_MOTHERSHIP);
    CHECK_NEAR(e->y, 20, 0);
    CHECK_EQ_INT(e->score, 2500);
    CHECK_NEAR(e->hp, 4 + g_diffEnemyHpBonus, 0);
    CHECK(e->gfxA == g_gfxMothership);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 11);
    CHECK_EQ_INT(g_save.players[0].hurryupComboCount, 1);
    CHECK_STR(g_alertMsg, "H U R R Y   U P");
    CHECK_EQ_INT(g_msgTimer, 11001);
    CHECK_EQ_INT(g_save.players[0].time, 20001);
    CHECK_EQ_INT(g_hurryUpInterval, 92);
    CHECK_EQ_INT(g_guardCount, 2);
    CHECK_EQ_INT(g_enemies[0][2].active, 0);
}

TEST(Flow_HurryUp_interval_has_a_floor_of_40)
{
    Setup(LEVEL_WAVE);
    g_time = 10001;
    g_save.players[0].time = 0;
    g_hurryUpInterval = 44;
    HurryUp();
    CHECK_EQ_INT(g_hurryUpInterval, 40);
}

TEST(Flow_HurryUp_every_8th_brings_a_money_ship)
{
    Setup(LEVEL_WAVE);
    g_time = 10001;
    g_save.players[0].time = 0;
    g_save.players[0].hurryupComboCount = 7;
    HurryUp();
    CHECK_EQ_INT(g_enemies[0][0].type, ENEMY_MOTHERSHIP);
    Enemy *m = &g_enemies[0][1];
    CHECK_EQ_INT(m->active, 1);
    CHECK_EQ_INT(m->type, ENEMY_MONEY_SHIP);
    CHECK_EQ_INT(m->score, 25000);
    CHECK_NEAR(m->y, -110, 0);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 12);
    CHECK_EQ_INT(g_save.players[0].hurryupComboCount, 0);
}

TEST(Flow_HurryUp_not_on_boss_or_race_levels_or_when_done)
{
    Setup(LEVEL_WAVE);
    g_time = 10001;
    g_save.players[0].time = 0;
    g_curLevelData.type = LEVEL_BOSS;
    HurryUp();
    g_curLevelData.type = LEVEL_RACE;
    HurryUp();
    g_curLevelData.type = LEVEL_WAVE;
    g_save.players[0].done = 1;
    HurryUp();
    g_save.players[0].done = 0;
    g_gameMode = MODE_TIME_TRIAL;
    HurryUp();
    CHECK_EQ_INT(g_enemies[0][0].active, 0);
}

TEST(Flow_HurryUp_mothership_appears_during_play)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].time = g_time + 100;
    g_save.players[0].totalEnemies = 1;
    PutEnemy(0, ENEMY_HOVER, 400, 100)->hoverX = 400;
    g_enemies[0][0].hoverY = 100;
    RunFrames(3);
    CHECK_EQ_INT(g_enemies[0][1].active, 0);
    RunFrames(10);
    CHECK_EQ_INT(g_enemies[0][1].type, ENEMY_MOTHERSHIP);
}

// ---- warp malfunction / hyperspace ----

TEST(Flow_CenturyLevelFlag_is_always_set)
{
    CHECK_EQ_INT(CenturyLevelFlag(), 1);
}

static void ArmMalfunction(void)
{
    g_malfunctionTimer = 1;     // RandRange(0, 1) < 4: certain
    g_save.players[0].starSpeed = 121;
    g_save.players[0].shieldHitFlashSpeed = 2;     // RandRange(1, 2): one more wrapper
    g_save.players[0].enemyHpBonusRoll = 0;
    g_time = 90000;
}

TEST(Flow_WarpMalfunction_needs_warp_speed_and_malfunction_levels)
{
    Setup(LEVEL_WAVE);
    CHECK_EQ_INT(g_numMalfunction, 1);
    ArmMalfunction();
    g_save.players[0].starSpeed = 120;
    WarpMalfunction();
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    ArmMalfunction();
    g_numMalfunction = 0;
    WarpMalfunction();
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(g_warpMalfunctionCount, 0);
}

TEST(Flow_WarpMalfunction_drops_into_a_malfunction_level)
{
    Setup(LEVEL_WAVE);
    ArmMalfunction();
    PutEnemy(5, ENEMY_HOVER, 0, 0);
    PutEnemy(6, ENEMY_CAPTURED, 0, 0);
    g_diffHpBonusB = 0;
    WarpMalfunction();
    CHECK_EQ_INT(g_state, STATE_MALFUNCTION);
    CHECK_EQ_INT(g_warpMalfunctionCount, 1);
    CHECK_STR(g_warpMalfunctionMsg, "W A R P   M A L F U N C T I O N");
    CHECK_EQ_INT(g_alertTextTime, 92000);
    CHECK_EQ_INT(g_malfunctionAlarmTime, 90300);
    CHECK_EQ_INT(g_curLevelNum, 1);
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_BONUS_WAVE);
    CHECK_NEAR(g_save.players[0].starSpeed, 0, 0);
    CHECK_NEAR(g_save.players[0].hyperspaceInDuration, 100, 0);
    CHECK_EQ_INT(g_enemies[0][6].active, 1);
    CHECK_EQ_INT(g_enemies[0][6].type, ENEMY_CAPTURED);
    // RandRange(1, 2) + 1 wrappers in the first free slots
    int n = 2;
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 2);
    for (int i = 0; i < n; i++) {
        Enemy *e = &g_enemies[0][i];
        CHECK_EQ_INT(e->active, 1);
        CHECK_EQ_INT(e->type, ENEMY_WRAPPER);
        CHECK_EQ_INT(e->score, 5000);
        CHECK_NEAR(e->y, -110, 0);
        CHECK_NEAR(e->hp, 7, 0);
    }
    CHECK_EQ_INT(g_enemies[0][n].active, 0);
    CHECK_EQ_INT(g_save.players[0].killed, 0);
}

static void HyperspaceOut(void)
{
    g_save.players[0].hyperspaceInDuration = 3;
    g_save.players[0].hyperspaceOutTimer = 2;
    g_save.players[0].starSpeed = 5;
}

TEST(Flow_UpdateHyperspace_jumps_out_then_waits_twice_as_long)
{
    Setup(LEVEL_WAVE);
    HyperspaceOut();
    UpdateHyperspace();
    CHECK_EQ_INT(g_deathSeqActive, 1);
    CHECK_NEAR(g_save.players[0].starSpeed, 7.15, 1e-4);
    CHECK_NEAR(g_save.players[0].hyperspaceFade, 0.01, 1e-6);
    CHECK_NEAR(g_save.players[0].scrollSpeedY, 0.25, 1e-6);
    CHECK_NEAR(g_save.players[0].hyperspaceMidTimer, 0, 0);
    UpdateHyperspace();
    CHECK_NEAR(g_save.players[0].hyperspaceOutTimer, 0, 0);
    CHECK_NEAR(g_save.players[0].hyperspaceMidTimer, 6, 0);
}

TEST(Flow_UpdateHyperspace_mid_phase_finishes_the_level)
{
    Setup(LEVEL_WAVE);
    g_time = 40000;
    g_save.players[0].hyperspaceInDuration = 3;
    g_save.players[0].hyperspaceMidTimer = 2;
    UpdateHyperspace();
    CHECK_EQ_INT(g_save.players[0].done, 0);
    int plays = FakePlayCount("");
    UpdateHyperspace();
    CHECK_NEAR(g_save.players[0].hyperspaceInTimer, 3, 0);
    CHECK_EQ_INT(g_save.players[0].done, 1);
    CHECK_EQ_INT(g_save.players[0].doneTime, 41500);
    CHECK(FakePlayCount("") > plays);
}

TEST(Flow_UpdateHyperspace_jump_in_goes_to_the_next_level)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].money = 10;
    g_save.players[0].level = 1;
    g_save.players[0].hyperspaceInTimer = 1;
    g_levelStartLatch = 0;
    PutEnemy(3, ENEMY_HOVER, 0, 0);
    PutEnemy(4, ENEMY_CAPTURED, 0, 0);
    g_deathSeqActive = 1;
    UpdateHyperspace();
    CHECK_EQ_INT(g_deathSeqActive, 0);
    CHECK_EQ_INT(g_save.players[0].killed, 10);
    CHECK_EQ_INT(g_enemies[0][3].active, 0);
    CHECK_EQ_INT(g_enemies[0][4].active, 1);
    CHECK_EQ_INT(g_save.players[0].level, 2);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK_NEAR(g_save.players[0].starSpeed, 5, 0);
    CHECK_NEAR(g_save.players[0].hyperspaceFade, 0, 0);
}

TEST(Flow_UpdateHyperspace_jump_in_with_money_goes_shopping)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].money = 50;
    g_save.players[0].level = 1;
    g_save.players[0].hyperspaceInTimer = 1;
    UpdateHyperspace();
    CHECK_EQ_INT(g_state, STATE_SHOP);
    CHECK_EQ_INT(g_save.players[0].level, 1);
    CHECK_NEAR(g_shopTransition, 500, 0);
}

TEST(Flow_UpdateHyperspace_level_warp_skips_four_levels)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].money = 0;
    g_save.players[0].level = 1;
    g_save.players[0].levelWarpPending = 1;
    g_save.players[0].hyperspaceInTimer = 1;
    UpdateHyperspace();
    CHECK_EQ_INT(g_save.players[0].levelWarpPending, 0);
    // four levels warped, then on to the next one
    CHECK_EQ_INT(g_save.players[0].level, 6);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
}

TEST(Flow_UpdateHyperspace_scrolls_the_stars)
{
    Setup(LEVEL_WAVE);
    g_cfg.numStars = 2;
    g_save.players[0].starSpeed = 3;
    g_save.players[0].starVelX = 0;
    g_save.players[0].starVelZ = 0;
    g_starY[0] = 10;
    g_starX[0] = (g_starXMin + g_starXMax) / 2;
    g_starZ[0] = (g_starZNear + g_starZFar) / 2;
    g_starY[1] = g_starYMax - 1;
    g_starX[1] = g_starX[0];
    g_starZ[1] = g_starZ[0];
    UpdateHyperspace();
    CHECK_NEAR(g_starY[0], 13, 1e-4);
    CHECK_NEAR(g_starY[1], g_starYMax + 2 - (g_starYMax - g_starYMin), 1e-3);
}

// ---- enemy shots (levelobj.c) ----

TEST(Flow_enemy_shot_falls_and_leaves_the_screen)
{
    Setup(LEVEL_WAVE);
    LevelObj *o = &g_levelObj[4];
    o->active = 1;
    o->type = LOBJ_SHOT;
    o->x = 100;
    o->y = 590;
    o->vx = 0.5f;
    o->vy = 6;
    o->turnDelay = 100;
    UpdateLevelObjects();
    CHECK_NEAR(o->x, 100.5, 1e-5);
    CHECK_NEAR(o->y, 596, 1e-5);
    CHECK_EQ_INT(o->active, 1);
    UpdateLevelObjects();
    CHECK_EQ_INT(o->active, 0);
}

TEST(Flow_enemy_shot_scrolls_in_hyperspace)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].hyperspaceFade = 0.5f;
    g_save.players[0].scrollSpeedY = 4;
    g_frameDt = 2;
    LevelObj *o = &g_levelObj[0];
    o->active = 1;
    o->type = LOBJ_SHOT;
    o->x = 100;
    o->y = 50;
    o->vx = 3;
    o->vy = 3;
    UpdateLevelObjects();
    CHECK_NEAR(o->x, 100, 0);
    CHECK_NEAR(o->y, 58, 1e-5);
}

TEST(Flow_guard_beam_fades_out)
{
    Setup(LEVEL_WAVE);
    LevelObj *o = &g_levelObj[0];
    o->active = 1;
    o->type = LOBJ_BEAM;
    o->f28 = 2.5f;
    o->y = 50;
    o->vy = 5;
    UpdateLevelObjects();
    CHECK_NEAR(o->f28, 1.5, 1e-5);
    CHECK_NEAR(o->y, 50, 0);
    CHECK_EQ_INT(o->active, 1);
    UpdateLevelObjects();
    CHECK_EQ_INT(o->active, 0);
}

TEST(Flow_mothership_rocket_explodes_when_its_fuse_runs_out)
{
    Setup(LEVEL_WAVE);
    LevelObj *o = &g_levelObj[0];
    o->active = 1;
    o->type = LOBJ_ROCKET;
    o->fuse = 1.5f;
    o->x = 300;
    o->y = 200;
    o->frame = 8;
    o->speed = 2;
    o->turnTimer = 1000;
    o->soundTimer = 1000;
    o->player = 0;
    UpdateLevelObjects();
    CHECK_EQ_INT(o->active, 1);
    CHECK_NEAR(o->x, 300 + g_dirVecX[8] * 2, 1e-4);
    CHECK_NEAR(o->y, 200 + g_dirVecY[8] * 2, 1e-4);
    int explosions = 0;
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
        explosions += g_explosions[i].active != 0;
    UpdateLevelObjects();
    CHECK_EQ_INT(o->active, 0);
    int after = 0;
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
        after += g_explosions[i].active != 0;
    CHECK_EQ_INT(after, explosions + 1);
}

// ---- player shots and debris (mapobj.c) ----

TEST(Flow_SpawnDebris_fills_the_first_free_slot)
{
    Setup(LEVEL_WAVE);
    g_mapObjs[0].active = 1;
    g_save.players[0].energy = 0;
    g_save.players[0].shots = 0;
    int i = SpawnDebris(200, 500, 0, 2, 7.5f, 1, 2, 0);
    CHECK_EQ_INT(i, 1);
    MapObj *m = &g_mapObjs[1];
    CHECK_EQ_INT(m->active, 1);
    CHECK_EQ_INT(m->type, 2);
    CHECK_EQ_INT(m->player, 0);
    CHECK_EQ_INT(m->cost, 1);
    CHECK_NEAR(m->dmg, 7.5, 0);
    CHECK_NEAR(m->x, 200 + 20 - g_objW[2] / 2.0, 1e-4);
    CHECK_NEAR(m->y, 500, 0);
    CHECK_NEAR(m->vy, 2 * g_debrisVySpeedFactor[2], 1e-4);
    CHECK_NEAR(m->vx, 0, 0);
    CHECK_EQ_INT(m->link1, -1);
    CHECK_EQ_INT(g_save.players[0].energy, 1);
    CHECK_EQ_INT(g_save.players[0].shots, 1);
}

TEST(Flow_SpawnDebris_recoil_slows_the_shot)
{
    Setup(LEVEL_WAVE);
    SpawnDebris(200, 500, 0, 2, 1, 0, 2, 15);
    // push 15: speed factor 2 - 1.5
    CHECK_NEAR(g_mapObjs[0].vy, 2 * 0.5 * g_debrisVySpeedFactor[2], 1e-4);
    CHECK_EQ_INT(g_save.players[0].shots, 0);
}

TEST(Flow_SpawnDebris_chains_linked_pieces)
{
    Setup(LEVEL_WAVE);
    int i = SpawnDebris(100, 400, 0, 8, 1, 1, 1, 0);
    CHECK_EQ_INT(i, 0);
    CHECK_EQ_INT(g_mapObjs[0].link1, 1);
    CHECK_EQ_INT(g_mapObjs[0].link2, 2);
    CHECK_EQ_INT(g_mapObjs[1].type, g_debrisLinkType1[8]);
    CHECK_EQ_INT(g_mapObjs[2].type, g_debrisLinkType2[8]);
    CHECK_NEAR(g_mapObjs[1].vx, g_debrisVxFactor[g_debrisLinkType1[8]], 1e-4);
    CHECK_EQ_INT(g_save.players[0].energy, 3);
}

TEST(Flow_SpawnDebris_returns_minus_1_when_full)
{
    Setup(LEVEL_WAVE);
    for (int i = 0; i < MAX_MAP_OBJS; i++)
        g_mapObjs[i].active = 1;
    CHECK_EQ_INT(SpawnDebris(100, 400, 0, 2, 1, 1, 1, 0), -1);
    CHECK_EQ_INT(g_save.players[0].energy, 0);
}

TEST(Flow_UpdateMapObjects_moves_shots_and_refunds_them_off_screen)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].energy = 0;
    SpawnDebris(200, 500, 0, 2, 1, 1, 10, 0);
    MapObj *m = &g_mapObjs[0];
    float y = m->y;
    UpdateMapObjects();
    CHECK_NEAR(m->y, y + m->vy, 1e-4);
    CHECK_EQ_INT(m->active, 1);
    CHECK_EQ_INT(g_save.players[0].energy, 1);
    m->y = -50;
    m->vy = -1;
    UpdateMapObjects();
    CHECK_EQ_INT(m->active, 0);
    CHECK_EQ_INT(g_save.players[0].energy, 0);
}

TEST(Flow_UpdateMapObjects_cycles_the_shot_sprite_every_frame_of_animT)
{
    Setup(LEVEL_WAVE);
    SpawnDebris(200, 500, 0, 2, 1, 1, 0, 0);
    MapObj *m = &g_mapObjs[0];
    m->vy = 0;
    UpdateMapObjects();
    CHECK_EQ_INT(m->type, 2);
    UpdateMapObjects();
    CHECK_EQ_INT(m->type, g_debrisNextType[2]);
    CHECK_NEAR(m->animT, 1, 0);
}

TEST(Flow_UpdateMapObjects_ends_a_chain_at_type_minus_1)
{
    Setup(LEVEL_WAVE);
    MapObj *m = &g_mapObjs[0];
    m->active = 1;
    m->state = 0;
    m->laser = 1;
    m->type = 24;          // next: 50, then -1
    m->y = 300;
    m->link1 = -1;
    m->link2 = -1;
    UpdateMapObjects();
    CHECK_EQ_INT(m->type, 50);
    CHECK_EQ_INT(m->active, 1);
    UpdateMapObjects();
    CHECK_EQ_INT(m->active, 0);
}

TEST(Flow_homing_missile_explodes_when_its_life_runs_out)
{
    Setup(LEVEL_WAVE);
    PutEnemy(3, ENEMY_HOVER, 100, 100)->locked = 1;
    MapObj *m = &g_mapObjs[0];
    m->active = 1;
    m->state = MAPOBJ_STATE_HOMING;
    m->enemy = 3;
    m->life = 0.5f;
    m->x = 300;
    m->y = 300;
    UpdateMapObjects();
    CHECK_EQ_INT(m->active, 0);
    CHECK_EQ_INT(g_enemies[0][3].locked, 0);
}

// ---- end of game (gameover.c) ----

TEST(Flow_TallyStep_drains_cash_in_decreasing_chunks)
{
    memset(&g_bonusTally[0], 0, sizeof g_bonusTally[0]);
    g_bonusTally[0].cashRemaining = 1234567;
    g_bonusTally[0].maxCashFlag = -1;
    CHECK_EQ_INT(TallyStep(1), 1);
    CHECK_EQ_INT(g_bonusTally[0].cashRemaining, 234567);
    CHECK_EQ_INT(g_bonusTally[0].cash, 100000000);
    CHECK_EQ_INT(g_tallyDelay, 100);
    CHECK_EQ_INT(TallyStep(1), 1);
    CHECK_EQ_INT(g_bonusTally[0].cashRemaining, 134567);
    int steps = 2;
    while (g_bonusTally[0].cashRemaining > 0) {
        TallyStep(1);
        steps++;
    }
    CHECK_EQ_INT(steps, 1 + 2 + 3 + 4 + 5 + 6 + 7);
    CHECK_EQ_INT(g_tallyDelay, 300);
    CHECK_EQ_INT(g_bonusTally[0].cash, 123456700);
    CHECK_EQ_INT(g_bonusTally[0].total, 123456700);
    CHECK_EQ_INT(TallyStep(1), 0);
    CHECK_EQ_INT(g_tallyDelay, 500);
}

TEST(Flow_TallyStep_then_rank_perfects_and_hit_percent)
{
    memset(&g_bonusTally[0], 0, sizeof g_bonusTally[0]);
    g_bonusTally[0].maxCashFlag = 1;
    g_bonusTally[0].rankRemaining = 2;
    g_bonusTally[0].perfectsRemaining = 3;
    g_bonusTally[0].hitPercentTarget = 4;
    int steps = 0;
    while (TallyStep(1))
        steps++;
    CHECK_EQ_INT(steps, 1 + 2 + 3 + 4);
    CHECK_EQ_INT(g_bonusTally[0].maxCashBonus, 50000000);
    CHECK_EQ_INT(g_bonusTally[0].rank, 2);
    CHECK_EQ_INT(g_bonusTally[0].rankBonus, 20000 + 30000);
    CHECK_EQ_INT(g_bonusTally[0].perfects, 3);
    CHECK_EQ_INT(g_bonusTally[0].perfectsBonus, 300000);
    CHECK_EQ_INT(g_bonusTally[0].hitPercent, 4);
    CHECK_EQ_INT(g_bonusTally[0].hitBonus, 4000);
    CHECK_EQ_INT(g_bonusTally[0].total, 50000000 + 50000 + 300000 + 4000);
}

TEST(Flow_TallyStep_advances_every_slot)
{
    memset(g_bonusTally, 0, 2 * sizeof g_bonusTally[0]);
    g_bonusTally[0].maxCashFlag = -1;
    g_bonusTally[1].maxCashFlag = -1;
    g_bonusTally[1].hitPercentTarget = 2;
    CHECK_EQ_INT(TallyStep(1), 0);
    CHECK_EQ_INT(TallyStep(2), 1);
    CHECK_EQ_INT(g_bonusTally[1].hitPercent, 1);
    CHECK_EQ_INT(g_tallyDelay, 75);
    CHECK_EQ_INT(TallyStep(2), 1);
    CHECK_EQ_INT(g_tallyDelay, 300);
}

TEST(Flow_InitPlayerStats_starts_the_tally_from_the_players_run)
{
    Setup(LEVEL_WAVE);
    g_time = 77000;
    g_save.players[1].money = 4321;
    g_save.players[1].rank = 5;
    g_save.players[1].bonusRoundCount = 2;
    g_save.players[1].hits = 30;
    g_save.players[1].shots = 40;
    g_bonusTally[1].total = 99;
    InitPlayerStats(1, 1);
    CHECK_EQ_INT(g_bonusTally[1].cashRemaining, 4321);
    CHECK_EQ_INT(g_bonusTally[1].rankRemaining, 5);
    CHECK_EQ_INT(g_bonusTally[1].perfectsRemaining, 2);
    CHECK_EQ_INT(g_bonusTally[1].hitPercentTarget, 75);
    CHECK_EQ_INT(g_bonusTally[1].maxCashFlag, -1);
    CHECK_EQ_INT(g_bonusTally[1].total, 0);
    CHECK_EQ_INT(g_tallyTime, 77000);
    g_save.players[1].hits = 50;
    g_save.players[1].shots = 0;
    InitPlayerStats(1, 1);
    CHECK_EQ_INT(g_save.players[1].shots, 1);
    CHECK_EQ_INT(g_bonusTally[1].hitPercentTarget, 100);
}

TEST(Flow_InitLayers_puts_the_skull_in_front)
{
    Setup(LEVEL_WAVE);
    InitLayers();
    CHECK(g_fx[0].gfx == g_gfxSkull);
    CHECK(g_fx[1].gfx == g_gfxGameOver);
    CHECK_NEAR(g_fx[0].alpha, 255, 0);
    CHECK_NEAR(g_fx[1].alpha, 230, 0);
    CHECK_NEAR(g_fx[2].alpha, 205, 0);
    CHECK_NEAR(g_fx[0].speed, 20, 1e-5);
    CHECK_NEAR(g_fx[1].speed, 20 / 1.5, 1e-4);
    CHECK_EQ_INT(g_fx[0].rInit, 255);
    CHECK_EQ_INT(g_fx[1].gInit, 255);
    CHECK_NEAR(g_fx[9].x, 400, 0);
    CHECK_EQ_INT(g_fx[9].active, 1);
}

TEST(Flow_PostRoundIdleTimeout_resumes_play_after_the_delay)
{
    g_state = STATE_POST_ROUND_IDLE;
    g_screenTimerStart = 1000;
    g_screenDelayMs = 3000;
    g_time = 4000;
    PostRoundIdleTimeout();
    CHECK_EQ_INT(g_state, STATE_POST_ROUND_IDLE);
    g_time = 4001;
    PostRoundIdleTimeout();
    CHECK_EQ_INT(g_state, STATE_PLAYING);
}

static bool NotOnTally(void) { return g_state != STATE_HISCORE_TABLE; }

TEST(Flow_game_over_tally_adds_the_bonus_to_the_score)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].score = 500;
    g_save.players[0].money = 3;
    g_save.players[0].rank = 0;
    g_save.players[0].bonusRoundCount = 0;
    g_save.players[0].hits = 0;
    g_save.players[0].lives = g_shipDefs[g_save.players[0].ship]->minEnergy;
    g_state = STATE_RESPAWN;
    RunFrames(1);
    CHECK_EQ_INT(g_state, STATE_HISCORE_TABLE);
    CHECK_EQ_INT(g_bonusTally[0].cashRemaining, 3);
    CHECK(RunFramesUntil(NotOnTally, 700));
    CHECK_EQ_INT(g_save.players[0].score, 500 + 300);
}

TEST(Flow_game_over_screens_advance_on_their_timeouts)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].lives = g_shipDefs[g_save.players[0].ship]->minEnergy;
    g_state = STATE_RESPAWN;
    RunFrames(1);
    CHECK_EQ_INT(g_tallyStep, 1);
    CHECK_EQ_INT(g_screenDelayMs, 3000);
    // the first screen lasts 3 s, then the bonus screen 5 s
    RunFrames(150);
    CHECK_EQ_INT(g_tallyStep, 1);
    RunFrames(40);
    CHECK_EQ_INT(g_tallyStep, 2);
    CHECK_EQ_INT(g_screenDelayMs, 5000);
    CHECK(g_frameFunc == GameOverScreen);
}

TEST(Flow_space_skips_the_game_over_tally)
{
    Setup(LEVEL_WAVE);
    g_save.players[0].money = 123;
    g_save.players[0].lives = g_shipDefs[g_save.players[0].ship]->minEnergy;
    g_state = STATE_RESPAWN;
    RunFrames(40);              // past the 500 ms transition lock
    CHECK_EQ_INT(g_tallyStep, 1);
    TapKey(K_VK_SPACE, 1);
    CHECK_EQ_INT(g_bonusTally[0].cashRemaining, 0);
    CHECK_EQ_INT(g_bonusTally[0].total, 12300);
    CHECK_EQ_INT(g_tallyStep, 2);
}
