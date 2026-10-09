// Tests for src/game/player.c: InitPlayer/PlacePlayer, PlayerHit, movement and its bounds,
// firing (FirePlayer, the fire latch, autofire, mirror shots), rockets, the shield/scoop
// timers, death and respawn per game mode, UpdatePlayers and the demo AI (StateDemo).
#include "support.h"

#define PL0 g_save.players[0]
#define PL1 g_save.players[1]

// The game booted to the title, then set up as a single-player game in normal play, with
// nothing on screen and fixed time/randomness.
static void StartPlay(void)
{
    BootGame();
    SeedRand(1234);
    g_gameMode = MODE_SINGLE;
    g_playerUpdateFn = UpdatePlayer;
    g_autoplay = 0;
    g_curPlayer = 0;
    PL0.ship = 0;
    PL1.ship = 0;
    InitPlayer(0);
    g_state = STATE_PLAYING;
    g_time = 500000;
    g_frameDt = 1.0f;
    g_transitionLock = 0;
    g_bonusResultsTime = 0;
    memset(g_items, 0, sizeof g_items);
    memset(g_mapObjs, 0, sizeof g_mapObjs);
    memset(g_enemies, 0, sizeof g_enemies);
    memset(g_levelObj, 0, sizeof g_levelObj);
    FakeReleaseAllKeys();
}

static int ActiveShots(void)
{
    int n = 0;
    for (int i = 0; i < MAX_MAP_OBJS; i++)
        n += g_mapObjs[i].active != 0;
    return n;
}

static int Plays(AudioHandle sample)
{
    return FakePlayCount(FakeSampleName(sample));
}

typedef struct { unsigned x, y, z, w, t; } Rng;
static Rng SaveRng(void) { return (Rng){g_rngX, g_rngY, g_rngZ, g_rngW, g_rngT}; }
static void RestoreRng(Rng r) { g_rngX = r.x; g_rngY = r.y; g_rngZ = r.z; g_rngW = r.w; g_rngT = r.t; }

// One gameplay frame of player 0's update with `key` held (or none for -1).
static void UpdateWithKey(int key)
{
    FakeReleaseAllKeys();
    if (key >= 0)
        FakePressKey((enum EKeyboardLayout)key);
    UpdatePlayer();
}

// ---------------------------------------------------------------- InitPlayer / PlacePlayer

TEST(Play_InitPlayer_sets_start_of_game_defaults)
{
    StartPlay();
    // dirty everything InitPlayer must reset
    PL0.score = 777; PL0.lives = 1; PL0.armour = 1; PL0.bullets = 40; PL0.weapon = 7;
    PL0.money = 5000; PL0.moneyMax = 1; PL0.buffDuration = 3; PL0.autofireInterval = 7;
    PL0.rockets = 9; PL0.gems = 1; PL0.level = 30; PL0.x = 1; PL0.y = 1; PL0.bank = 0;
    PL0.speed = 0; PL0.bulletSpeedMult = 3; PL0.dead = 1; PL0.shieldTimer = 9; PL0.marks = 7;
    PL0.gemSeqA = 3; PL0.gemSeqB = 3; PL0.enemyHpBonusRoll = 0; PL0.shieldHitFlashSpeed = 0;
    PL0.autofire = 1; PL0.superAuto = 1; PL0.mirrorTime = 5; PL0.levelFinished = 1;
    g_scoreMul[0] = 5;
    InitPlayer(0);

    // ship 0's stats: minEnergy 26, cost 4, extraLives 12, baseArmour 123, gemBase 452
    CHECK_EQ_INT(PL0.lives, 26 + 12);
    CHECK_EQ_INT(PL0.armour, 123);
    CHECK_EQ_INT(PL0.gems, 452);
    CHECK_EQ_INT(PL0.score, 0);
    CHECK_EQ_INT(PL0.bullets, 8);
    CHECK_EQ_INT(PL0.weapon, WEAPON_SINGLE);
    CHECK_EQ_INT(PL0.money, 0);
    CHECK_EQ_INT(PL0.moneyMax, 99990);
    CHECK_EQ_INT(PL0.buffDuration, 20);
    CHECK_EQ_INT(PL0.autofireInterval, 100);
    CHECK_EQ_INT(PL0.autofire, 0);
    CHECK_EQ_INT(PL0.superAuto, 0);
    CHECK_EQ_INT(PL0.rockets, 0);
    CHECK_EQ_INT(PL0.level, 1);
    CHECK_EQ_INT(PL0.dead, 0);
    CHECK_EQ_INT(PL0.shieldTimer, 0);
    CHECK_EQ_INT(PL0.mirrorTime, 0);
    CHECK_EQ_INT(PL0.levelFinished, 0);
    CHECK_EQ_INT((int)PL0.marks, 0);
    CHECK_EQ_INT(PL0.gemSeqA, -1);
    CHECK_EQ_INT(PL0.gemSeqB, -1);
    CHECK_EQ_INT(PL0.enemyHpBonusRoll, 8);
    CHECK_NEAR(PL0.shieldHitFlashSpeed, 3.0, 1e-6);
    CHECK_NEAR(PL0.bulletSpeedMult, 1.0, 1e-6);
    CHECK_NEAR(PL0.x, 800 / 2 - 20, 1e-6);
    CHECK_NEAR(PL0.y, 550, 1e-6);
    CHECK_NEAR(PL0.bank, 5.0, 1e-6);
    CHECK_NEAR(PL0.speed, g_speedBase, 1e-6);
    CHECK_EQ_INT(g_scoreMul[0], 1);
    CHECK_EQ_INT(PL0.inputDevice, g_cfg.device0);
}

TEST(Play_InitPlayer_uses_the_ships_stats)
{
    StartPlay();
    PL0.ship = 1;   // {45, 2, 6, 10, 34, 16, 32, 895, 43}
    InitPlayer(0);
    CHECK_EQ_INT(PL0.lives, 45 + 6);
    CHECK_EQ_INT(PL0.armour, 34);
    CHECK_EQ_INT(PL0.gems, 895);
}

TEST(Play_InitPlayer_time_trial_starts_with_weapon_9_and_10_bullets)
{
    StartPlay();
    g_gameMode = MODE_TIME_TRIAL;
    InitPlayer(0);
    CHECK_EQ_INT(PL0.weapon, 9);
    CHECK_EQ_INT(PL0.bullets, 10);
}

TEST(Play_InitPlayer_demo_rolls_level_then_weapon)
{
    StartPlay();
    g_playerUpdateFn = StateDemo;
    for (int round = 0; round < 40; round++) {
        Rng r = SaveRng();
        int level = RandRange(1, 8);
        int weapon = RandRange(0, 3);
        RestoreRng(r);
        InitPlayer(0);
        CHECK_EQ_INT(PL0.level, level);
        CHECK_EQ_INT(PL0.weapon, weapon);
    }
}

TEST(Play_PlacePlayer_puts_dual_players_at_thirds)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    PlacePlayer(0);
    PlacePlayer(1);
    CHECK_NEAR(PL0.x, 800 / 6 * 2 - 20, 1e-6);
    CHECK_NEAR(PL1.x, 800 / 6 * 4 - 20, 1e-6);
    CHECK_NEAR(PL1.y, 550, 1e-6);
    CHECK(PL0.gfx == g_gfxFighter1);
    CHECK(PL1.gfx == g_gfxFighter2);
}

TEST(Play_PlayerHasAllMarks_needs_all_six)
{
    StartPlay();
    PL0.marks = MARK_1 | MARK_2 | MARK_3 | MARK_4 | MARK_5;
    CHECK_EQ_INT(PlayerHasAllMarks(0), 0);
    PL0.marks = MARK_1 | MARK_2 | MARK_3 | MARK_4 | MARK_5 | MARK_6;
    CHECK_EQ_INT(PlayerHasAllMarks(0), 1);
}

TEST(Play_ResetPlayerTimers_clears_both_players_shield_and_scoop)
{
    StartPlay();
    PL0.shieldTimer = PL1.shieldTimer = 9999;
    PL0.scoopTimer = PL1.scoopTimer = 9999;
    ResetPlayerTimers();
    CHECK_EQ_INT(PL0.shieldTimer, 0);
    CHECK_EQ_INT(PL1.shieldTimer, 0);
    CHECK_EQ_INT(PL0.scoopTimer, 0);
    CHECK_EQ_INT(PL1.scoopTimer, 0);
}

// ---------------------------------------------------------------- PlayerHit

TEST(Play_PlayerHit_steps_every_upgrade_down)
{
    StartPlay();
    PL0.weapon = 3; PL0.bullets = 10; PL0.speed = g_speedBase + 2 * g_speedStep;
    PL0.buffDuration = 30; PL0.bulletSpeedMult = 1.6f;
    PL0.autofire = 1; PL0.superAuto = 1; PL0.alienLock = 1; PL0.autofireInterval = 25;
    PlayerHit(0);
    CHECK_EQ_INT(PL0.weapon, 2);
    CHECK_EQ_INT(PL0.bullets, 9);
    CHECK_NEAR(PL0.speed, g_speedBase + g_speedStep, 1e-4);
    CHECK_EQ_INT(PL0.buffDuration, 25);
    CHECK_NEAR(PL0.bulletSpeedMult, 1.3, 1e-4);
    CHECK_EQ_INT(PL0.autofire, 0);
    CHECK_EQ_INT(PL0.superAuto, 0);
    CHECK_EQ_INT(PL0.alienLock, 0);
    CHECK_EQ_INT(PL0.autofireInterval, 100);
}

TEST(Play_PlayerHit_stops_at_the_minimums)
{
    StartPlay();
    PL0.weapon = 0; PL0.bullets = 5; PL0.speed = g_speedBase + 0.1f;
    PL0.buffDuration = g_speedMin + 2; PL0.bulletSpeedMult = 1.0f;
    PlayerHit(0);
    CHECK_EQ_INT(PL0.weapon, 0);
    CHECK_EQ_INT(PL0.bullets, 5);
    CHECK_NEAR(PL0.speed, g_speedBase, 1e-6);
    CHECK_EQ_INT(PL0.buffDuration, g_speedMin);
    CHECK_NEAR(PL0.bulletSpeedMult, 1.0, 1e-6);
    PL0.bullets = 6;
    PlayerHit(0);
    CHECK_EQ_INT(PL0.bullets, 5);
}

TEST(Play_PlayerHit_weapon_floor_at_one_keeps_double_shot)
{
    StartPlay();
    PL0.weaponFloorAtOne = 1;
    PL0.weapon = 2;
    PlayerHit(0);
    CHECK_EQ_INT(PL0.weapon, 1);
    PlayerHit(0);
    CHECK_EQ_INT(PL0.weapon, 1);
}

TEST(Play_PlayerHit_in_time_trial_only_drops_autofire)
{
    StartPlay();
    g_gameMode = MODE_TIME_TRIAL;
    PL0.weapon = 9; PL0.bullets = 10; PL0.autofire = 1; PL0.superAuto = 1;
    PlayerHit(0);
    CHECK_EQ_INT(PL0.autofire, 0);
    CHECK_EQ_INT(PL0.superAuto, 1);
    CHECK_EQ_INT(PL0.weapon, 9);
    CHECK_EQ_INT(PL0.bullets, 10);
}

// ---------------------------------------------------------------- movement

TEST(Play_UpdatePlayer_left_key_moves_and_banks_left)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 4; PL0.bank = 5;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.x, 376, 1e-4);
    CHECK_NEAR(PL0.bank, 4.5, 1e-4);
}

TEST(Play_UpdatePlayer_right_key_moves_and_banks_right)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 4; PL0.bank = 5;
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.x, 384, 1e-4);
    CHECK_NEAR(PL0.bank, 5.5, 1e-4);
}

TEST(Play_UpdatePlayer_left_wins_when_both_are_held)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 4;
    FakePressKey((enum EKeyboardLayout)g_cfg.left[0]);
    FakePressKey((enum EKeyboardLayout)g_cfg.right[0]);
    UpdatePlayer();
    CHECK_NEAR(PL0.x, 376, 1e-4);
}

TEST(Play_UpdatePlayer_movement_scales_with_frame_time)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 4;
    g_frameDt = 2.5f;
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.x, 390, 1e-4);
}

TEST(Play_UpdatePlayer_speed_is_capped_at_14)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 20;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.x, 366, 1e-4);
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.x, 380, 1e-4);
}

TEST(Play_UpdatePlayer_keeps_the_ship_inside_64_to_screen_minus_104)
{
    StartPlay();
    PL0.speed = 4;
    PL0.x = 66;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.x, 64, 1e-4);
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.x, 64, 1e-4);
    PL0.x = 694;
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.x, 800 - 104, 1e-4);
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.x, 800 - 104, 1e-4);
}

TEST(Play_UpdatePlayer_bank_is_limited_to_0_and_10)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 1;
    PL0.bank = 0.2f;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.bank, 0, 1e-6);
    PL0.bank = 10.6f;
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.bank, 10, 1e-6);
    PL0.bank = 10.4f;
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.bank, 10.9, 1e-4);
}

TEST(Play_UpdatePlayer_bank_returns_toward_5_when_idle)
{
    StartPlay();
    PL0.bank = 7;
    UpdateWithKey(-1);
    CHECK_NEAR(PL0.bank, 6.5, 1e-4);
    PL0.bank = 3;
    UpdateWithKey(-1);
    CHECK_NEAR(PL0.bank, 3.5, 1e-4);
    PL0.bank = 5;
    UpdateWithKey(-1);
    CHECK_NEAR(PL0.bank, 5, 1e-4);
}

TEST(Play_UpdatePlayer_drunk_mode_reverses_controls_until_it_expires)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 4;
    PL0.drunkModeTimer = g_time + 1000;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.x, 384, 1e-4);
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.x, 380, 1e-4);
    g_time += 1001;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_EQ_INT(PL0.drunkModeTimer, 0);
    CHECK_NEAR(PL0.x, 376, 1e-4);
}

TEST(Play_UpdatePlayer_mirror_mode_moves_the_mirror_position)
{
    StartPlay();
    PL0.x = 380; PL0.speed = 4;
    PL0.mirrorTime = g_time + 5000;
    PL0.mirrorX = 300;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.mirrorX, 296, 1e-4);
    CHECK_NEAR(PL0.x, 296, 1e-4);
    PL0.mirrorX = 694;
    UpdateWithKey(g_cfg.right[0]);
    CHECK_NEAR(PL0.mirrorX, 696, 1e-4);
    CHECK_NEAR(PL0.x, 696, 1e-4);
    PL0.mirrorX = 66;
    UpdateWithKey(g_cfg.left[0]);
    CHECK_NEAR(PL0.mirrorX, 64, 1e-4);
}

// ---------------------------------------------------------------- firing

TEST(Play_UpdatePlayer_fire_key_shoots_once_per_press)
{
    StartPlay();
    PL0.keyLatchFire = 1;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 1);
    CHECK_EQ_INT(g_mapObjs[0].player, 0);
    CHECK_EQ_INT(g_mapObjs[0].type, g_shotType[WEAPON_SINGLE]);
    CHECK_EQ_INT(PL0.shots, 1);
    CHECK_EQ_INT(PL0.energy, 1);
    CHECK_EQ_INT(PL0.keyLatchFire, 0);
    // held: no new shot
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 1);
    // released, then pressed again
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.keyLatchFire, 1);
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 2);
    CHECK_EQ_INT(PL0.shots, 2);
}

TEST(Play_UpdatePlayer_does_not_fire_when_dead_or_out_of_lives)
{
    StartPlay();
    PL0.keyLatchFire = 1;
    PL0.dead = 1;
    PL0.respawnTime = g_time + 10000;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 0);
    PL0.dead = 0;
    PL0.keyLatchFire = 1;
    PL0.lives = 26;   // == minEnergy: no lives left
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 0);
    PL0.keyLatchFire = 1;
    PL0.lives = 27;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 1);
}

TEST(Play_UpdatePlayer_does_not_fire_during_a_transition_or_in_the_shop)
{
    StartPlay();
    PL0.keyLatchFire = 1;
    g_transitionLock = 1;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 0);
    g_transitionLock = 0;
    g_state = STATE_SHOP;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 0);
    g_state = STATE_PLAYING;
    g_bonusResultsTime = 1;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 0);
    g_bonusResultsTime = 0;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 1);
}

TEST(Play_FirePlayer_keeps_at_most_bullets_shots_in_flight)
{
    StartPlay();
    PL0.bullets = 8;
    for (int i = 0; i < 12; i++)
        FirePlayer(PL0.bullets, 1);
    CHECK_EQ_INT(ActiveShots(), 8);
    CHECK_EQ_INT(PL0.shots, 8);
    CHECK_EQ_INT(PL0.energy, 8);
}

TEST(Play_FirePlayer_shoots_the_weapons_shot_type_and_damage)
{
    StartPlay();
    PL0.weapon = WEAPON_WAR_PLASMA;
    PL0.x = 300; PL0.y = 550;
    FirePlayer(PL0.bullets, 1);
    // the first slot is the main shot; WAR.I.PLASMA (type 19) links two more pieces
    CHECK_EQ_INT(g_mapObjs[0].type, 19);
    CHECK_NEAR(g_mapObjs[0].dmg, 15, 1e-6);
    CHECK_EQ_INT(ActiveShots(), 3);
    CHECK_EQ_INT(g_mapObjs[0].link1, 1);
    CHECK_EQ_INT(g_mapObjs[0].link2, 2);
    CHECK_EQ_INT(g_mapObjs[1].type, 20);
    CHECK_EQ_INT(g_mapObjs[2].type, 21);
}

TEST(Play_FirePlayer_plays_the_weapon_sound_at_most_every_50ms)
{
    StartPlay();
    PL0.weapon = WEAPON_DOUBLE;
    PL0.bullets = 50;
    FirePlayer(PL0.bullets, 1);
    CHECK_EQ_INT(Plays(g_sampleHandle[WEAPON_DOUBLE]), 1);
    CHECK_EQ_INT(PL0.nextShotSnd, g_time + 50);
    FirePlayer(PL0.bullets, 1);
    CHECK_EQ_INT(Plays(g_sampleHandle[WEAPON_DOUBLE]), 1);
    g_time += 51;
    FirePlayer(PL0.bullets, 1);
    CHECK_EQ_INT(Plays(g_sampleHandle[WEAPON_DOUBLE]), 2);
}

TEST(Play_FirePlayer_captured_shields_fire_side_shots)
{
    StartPlay();
    PL0.x = 300;
    PL0.shieldL = 1; PL0.shieldLIdx = 3; g_enemies[0][3].settled = 1;
    PL0.shieldR = 1; PL0.shieldRIdx = 4; g_enemies[0][4].settled = 1;
    FirePlayer(PL0.bullets, 1);
    // single shot (type 0 -> side type g_shotType[16]) at x-36 and x+36
    CHECK_EQ_INT(ActiveShots(), 3);
    CHECK_EQ_INT(PL0.shots, 1);   // side shots don't count
    float x0 = g_mapObjs[0].x;
    CHECK_EQ_INT(g_mapObjs[1].type, g_shotType[16]);
    CHECK_EQ_INT(g_mapObjs[2].type, g_shotType[16]);
    CHECK(g_mapObjs[1].x < x0 && g_mapObjs[2].x > x0);
    // an unsettled shield doesn't shoot
    memset(g_mapObjs, 0, sizeof g_mapObjs);
    g_enemies[0][4].settled = 0;
    FirePlayer(100, 1);
    CHECK_EQ_INT(ActiveShots(), 2);
}

TEST(Play_UpdatePlayer_autofire_repeats_at_its_interval_while_fire_is_held)
{
    StartPlay();
    PL0.bullets = 50;
    PL0.autofire = 1;
    PL0.autofireInterval = 100;
    PL0.autofireTimer = 0;
    PL0.keyLatchFire = 0;   // fire already held
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 1);
    CHECK_EQ_INT(PL0.autofireTimer, g_time + 100);
    g_time += 100;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 1);
    g_time += 1;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 2);
    // without autofire, holding does nothing
    PL0.autofire = 0;
    g_time += 1000;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 2);
}

TEST(Play_UpdatePlayer_mirror_mode_fires_from_both_ships)
{
    StartPlay();
    PL0.x = 200;
    PL0.mirrorTime = g_time + 5000;
    PL0.keyLatchFire = 1;
    UpdateWithKey(g_cfg.fire[0]);
    CHECK_EQ_INT(ActiveShots(), 2);
    CHECK_EQ_INT(PL0.shots, 0);
    CHECK_NEAR(PL0.x, 200, 1e-4);
    // the second shot is from the mirrored x: (800 - 40) - 200 = 560
    CHECK_NEAR(g_mapObjs[1].x - g_mapObjs[0].x, 560 - 200, 1e-3);
}

// ---------------------------------------------------------------- rockets

static void AddEnemy(int p, int i, int type, float x, float y)
{
    Enemy *e = &g_enemies[p][i];
    e->active = 1;
    e->type = type;
    e->x = x;
    e->y = y;
    e->pairedEnemyIdx = -1;
}

TEST(Play_UpdatePlayer_rocket_locks_an_enemy_and_uses_a_rocket)
{
    StartPlay();
    PL0.rockets = 3;
    PL0.keyLatchRocket = 1;
    PL0.x = 300; PL0.y = 550;
    AddEnemy(0, 5, ENEMY_HOVER, 200, 100);
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 2);
    CHECK_EQ_INT(PL0.rocketsFired, 1);
    CHECK_EQ_INT(PL0.keyLatchRocket, 0);
    CHECK_EQ_INT(g_mapObjs[0].active, 1);
    CHECK_EQ_INT(g_mapObjs[0].state, MAPOBJ_STATE_HOMING);
    CHECK_EQ_INT(g_mapObjs[0].enemy, 5);
    CHECK_EQ_INT(g_mapObjs[0].player, 0);
    CHECK_NEAR(g_mapObjs[0].dmg, 200, 1e-6);
    CHECK_NEAR(g_mapObjs[0].life, 300, 1e-6);
    CHECK_NEAR(g_mapObjs[0].x, 309, 1e-4);
    CHECK_NEAR(g_mapObjs[0].y, 542, 1e-4);
    CHECK_EQ_INT(g_enemies[0][5].locked, 1);
    CHECK_EQ_INT(g_transitionLock, 1);
    CHECK_EQ_INT(g_transitionLockUntil, g_time + 500);
    CHECK_EQ_INT(Plays(g_sfxRocket), 1);
    // held: no second rocket
    g_transitionLock = 0;
    AddEnemy(0, 6, ENEMY_HOVER, 200, 100);
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 2);
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.keyLatchRocket, 1);
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 1);
    CHECK_EQ_INT(g_mapObjs[1].enemy, 6);
}

TEST(Play_UpdatePlayer_rocket_skips_ineligible_enemies)
{
    StartPlay();
    PL0.rockets = 3;
    PL0.keyLatchRocket = 1;
    AddEnemy(0, 0, ENEMY_CAPTURED, 200, 100);
    AddEnemy(0, 1, ENEMY_HOVER, 200, 100);
    g_enemies[0][1].locked = 1;
    AddEnemy(0, 2, ENEMY_DEBRIS, 200, 100);
    AddEnemy(0, 3, ENEMY_HOVER, 200, 100);
    g_enemies[0][3].attackStaggerTimer = 1.0f;
    AddEnemy(0, 4, ENEMY_HOVER, 200, 100);
    g_enemies[0][4].active = 0;
    AddEnemy(0, 7, ENEMY_DIVING, 200, 100);
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 2);
    CHECK_EQ_INT(g_mapObjs[0].enemy, 7);
}

TEST(Play_UpdatePlayer_rocket_needs_a_target_and_rockets)
{
    StartPlay();
    PL0.rockets = 3;
    PL0.keyLatchRocket = 1;
    UpdateWithKey(g_cfg.rocket[0]);   // no enemy
    CHECK_EQ_INT(PL0.rockets, 3);
    CHECK_EQ_INT(ActiveShots(), 0);
    AddEnemy(0, 1, ENEMY_HOVER, 200, 100);
    PL0.rockets = 0;
    UpdateWithKey(-1);
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(ActiveShots(), 0);
    CHECK_EQ_INT(PL0.rockets, 0);
}

TEST(Play_UpdatePlayer_no_rockets_in_the_shop_or_when_dead)
{
    StartPlay();
    PL0.rockets = 3;
    AddEnemy(0, 1, ENEMY_HOVER, 200, 100);
    PL0.keyLatchRocket = 1;
    g_state = STATE_SHOP;
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 3);
    g_state = STATE_METEOR_STORM;
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 3);
    g_state = STATE_PLAYING;
    PL0.dead = 1;
    PL0.respawnTime = g_time + 10000;
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 3);
    PL0.dead = 0;
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(PL0.rockets, 2);
}

TEST(Play_UpdatePlayer_rocket_leaves_bosses_unlocked)
{
    StartPlay();
    PL0.rockets = 3;
    PL0.keyLatchRocket = 1;
    AddEnemy(0, 2, ENEMY_BOSS, 400, 100);
    UpdateWithKey(g_cfg.rocket[0]);
    CHECK_EQ_INT(g_mapObjs[0].enemy, 2);
    CHECK_EQ_INT(g_enemies[0][2].locked, 0);
    CHECK_EQ_INT(PL0.rockets, 2);
}

// ---------------------------------------------------------------- shield and scoop timers

TEST(Play_UpdatePlayer_ends_the_shield_and_scoop_when_their_time_is_up)
{
    StartPlay();
    PL0.shieldTimer = g_time + 10;
    PL0.scoopTimer = g_time + 10;
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.shieldTimer, g_time + 10);
    CHECK_EQ_INT(PL0.scoopTimer, g_time + 10);
    g_time += 10;
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.shieldTimer, g_time);
    g_time += 1;
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.shieldTimer, 0);
    CHECK_EQ_INT(PL0.scoopTimer, 0);
}

TEST(Play_UpdatePlayer_scoop_beam_grows_to_45)
{
    StartPlay();
    PL0.scoopTimer = g_time + 100000;
    g_scoopRange = 43;
    UpdateWithKey(-1);
    CHECK_NEAR(g_scoopRange, 44, 1e-6);
    UpdateWithKey(-1);
    CHECK_NEAR(g_scoopRange, 45, 1e-6);
    UpdateWithKey(-1);
    CHECK_NEAR(g_scoopRange, 45, 1e-6);
}

// ---------------------------------------------------------------- death and respawn

TEST(Play_UpdatePlayer_respawns_single_player_after_the_timer)
{
    StartPlay();
    PL0.dead = 1;
    PL0.respawnTime = g_time + 5;
    PL0.x = 100; PL0.y = 300;
    PL0.weapon = 2;
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.dead, 1);
    CHECK_EQ_INT(PL0.lives, 38);
    g_time += 6;
    g_bonusWeight[27] = g_bonusWeight[33] = g_bonusWeight[26] = g_bonusWeight[19] = 0;
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.dead, 0);
    CHECK_EQ_INT(PL0.lives, 38 - 4);
    CHECK_NEAR(PL0.x, 380, 1e-6);
    CHECK_NEAR(PL0.y, 550, 1e-6);
    CHECK_EQ_INT(PL0.weapon, 1);   // PlayerHit
    CHECK_EQ_INT(PL0.shieldTimer, g_time + 3000);
    CHECK_EQ_INT(g_bonusWeight[27], 10);
    CHECK_EQ_INT(g_bonusWeight[33], 8);
    CHECK_EQ_INT(g_bonusWeight[26], 25);
    CHECK_EQ_INT(g_bonusWeight[19], 35);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
}

TEST(Play_UpdatePlayer_losing_the_last_life_shows_the_hiscore_table)
{
    StartPlay();
    PL0.dead = 1;
    PL0.respawnTime = g_time - 1;
    PL0.lives = 26 + 4;
    g_playerBroke = 0;
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.lives, 26);
    CHECK_EQ_INT(PL0.shieldTimer, 0);
    CHECK_EQ_INT(g_playerBroke, 1);   // ShowHiscoreTable ran
}

TEST(Play_UpdatePlayer_respawn_shield_time_depends_on_the_mode)
{
    StartPlay();
    g_gameMode = MODE_TIME_TRIAL;
    PL0.dead = 1;
    PL0.respawnTime = g_time - 1;
    UpdateWithKey(-1);
    CHECK_EQ_INT(PL0.dead, 0);
    CHECK_EQ_INT(PL0.lives, 34);
    CHECK_EQ_INT(PL0.shieldTimer, g_time + 1500);
}

TEST(Play_UpdatePlayer_two_player_respawn_hands_over_to_the_other_player)
{
    StartPlay();
    g_gameMode = MODE_TWO_PLAYER;
    InitPlayer(1);
    PL1.levelFinished = 1;   // player 2 then warps out instead of loading a level
    PL1.x = 100;
    PL0.dead = 1;
    PL0.respawnTime = g_time - 1;
    PL0.weapon = 2;
    UpdatePlayer();
    CHECK_EQ_INT(PL0.dead, 0);
    CHECK_EQ_INT(PL0.lives, 34);
    CHECK_EQ_INT(PL0.weapon, 1);
    CHECK_EQ_INT(g_curPlayer, 1);
    CHECK_EQ_INT(g_state, STATE_MALFUNCTION_DEATH);
    // the rest of the respawn applies to the player taking over
    CHECK_NEAR(PL1.x, 380, 1e-6);
    CHECK_EQ_INT(PL1.shieldTimer, g_time + 5000);
    CHECK_EQ_INT(PL0.shieldTimer, 0);
}

TEST(Play_UpdatePlayer_dual_mode_respawns_each_player_in_its_third)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    g_curPlayer = 1;
    PL1.dead = 1;
    PL1.respawnTime = g_time - 1;
    PL1.x = 100;
    UpdatePlayer();
    CHECK_EQ_INT(PL1.dead, 0);
    CHECK_NEAR(PL1.x, 800 / 6 * 4 - 20, 1e-6);
    CHECK_EQ_INT(PL1.lives, 34);
    CHECK_EQ_INT(PL1.shieldTimer, g_time + 3000);
    g_curPlayer = 0;
    PL0.dead = 1;
    PL0.respawnTime = g_time - 1;
    PL0.x = 600;
    UpdatePlayer();
    CHECK_NEAR(PL0.x, 800 / 6 * 2 - 20, 1e-6);
}

TEST(Play_UpdatePlayers_updates_player_2_then_player_1)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    PL0.shieldTimer = g_time - 1;
    PL1.shieldTimer = g_time - 1;
    g_curPlayer = 1;
    UpdatePlayers();
    CHECK_EQ_INT(PL0.shieldTimer, 0);
    CHECK_EQ_INT(PL1.shieldTimer, 0);
    CHECK_EQ_INT(g_curPlayer, 0);
}

TEST(Play_UpdatePlayers_in_a_race_only_updates_the_turn_player)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    g_state = STATE_BONUS_RACE;
    g_vsTurnPlayer = 1;
    PL0.shieldTimer = g_time - 1;
    PL1.shieldTimer = g_time - 1;
    UpdatePlayers();
    CHECK_EQ_INT(PL0.shieldTimer, g_time - 1);
    CHECK_EQ_INT(PL1.shieldTimer, 0);
    CHECK_EQ_INT(g_curPlayer, 1);
    g_vsTurnPlayer = 0;
    UpdatePlayers();
    CHECK_EQ_INT(PL0.shieldTimer, 0);
    CHECK_EQ_INT(g_curPlayer, 0);
}

// ---------------------------------------------------------------- the demo AI

TEST(Play_StateDemo_steers_away_from_the_edges)
{
    StartPlay();
    g_playerUpdateFn = StateDemo;
    PL0.speed = 4;
    PL0.x = 100;
    StateDemo();
    CHECK_EQ_INT(g_demoSteer, 90);
    CHECK_NEAR(PL0.x, 104, 1e-4);
    PL0.x = 700;
    StateDemo();
    CHECK_EQ_INT(g_demoSteer, 10);
    CHECK_NEAR(PL0.x, 696, 1e-4);
    CHECK_STR(g_alertMsg, "D E M O");
}

TEST(Play_StateDemo_any_key_returns_to_the_title)
{
    StartPlay();
    g_playerUpdateFn = StateDemo;
    g_inputCooldown = 0;
    FakePressKey(K_VK_SPACE);
    PL0.x = 100;
    StateDemo();
    CHECK_EQ_INT(g_inputCooldown, 100);
    CHECK_NEAR(PL0.x, 100, 1e-6);
    CHECK_EQ_INT(g_state, STATE_TITLE);
}
