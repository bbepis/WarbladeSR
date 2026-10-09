// Tests for src/game/items.c: Pickup() of every item type and its effect on the player, and
// the item spawners (coins, gems, money bags, bursts, bonus rolls) and per-frame item motion.
#include "support.h"

#define PL0 g_save.players[0]
#define PL1 g_save.players[1]

// Ship 0: minEnergy 26, cost 4, extraLives 12, maxEnergy 20, baseArmour 123, armourStep 5,
// maxArmourBonus 10, gemBase 452, gemStep 8.
enum { LIVES0 = 38, MAX_LIVES = 46, ARMOUR0 = 123, MAX_ARMOUR = 133 };

static void StartPlay(void)
{
    BootGame();
    SeedRand(4321);
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
    g_bonusResultsTime = 0;
    g_inRandom = 0;
    memset(g_items, 0, sizeof g_items);
    memset(g_mapObjs, 0, sizeof g_mapObjs);
    memset(g_enemies, 0, sizeof g_enemies);
    memset(g_levelObj, 0, sizeof g_levelObj);
    g_soundQueueCount = 0;
    g_soundQueueNext = 0;
    FakeReleaseAllKeys();
}

static int Plays(AudioHandle sample)
{
    return FakePlayCount(FakeSampleName(sample));
}

static bool Queued(AudioHandle sample)
{
    for (int i = 0; i < g_soundQueueCount; i++)
        if (g_soundQueue[i].sample == sample)
            return true;
    return false;
}

static int CountItems(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_ITEMS; i++)
        n += g_items[i].alive && g_items[i].type == type;
    return n;
}

static int AliveItems(void)
{
    int n = 0;
    for (int i = 0; i < MAX_ITEMS; i++)
        n += g_items[i].alive != 0;
    return n;
}

typedef struct { unsigned x, y, z, w, t; } Rng;
static Rng SaveRng(void) { return (Rng){g_rngX, g_rngY, g_rngZ, g_rngW, g_rngT}; }
static void RestoreRng(Rng r) { g_rngX = r.x; g_rngY = r.y; g_rngZ = r.z; g_rngW = r.w; g_rngT = r.t; }

static void AddEnemy(int p, int i, int type, float x, float y)
{
    Enemy *e = &g_enemies[p][i];
    e->active = 1;
    e->type = type;
    e->x = x;
    e->y = y;
    e->hp = 10;
    e->pairedEnemyIdx = -1;
}

// ---------------------------------------------------------------- simple power-ups

TEST(Play_Pickup_counts_every_pickup)
{
    StartPlay();
    Pickup(ITEM_FREEZE);
    Pickup(ITEM_DRUNK);
    CHECK_EQ_INT(PL0.pickupCount, 2);
}

TEST(Play_Pickup_freeze_freezes_for_10_seconds)
{
    StartPlay();
    Pickup(ITEM_FREEZE);
    CHECK_EQ_INT(PL0.freezeTimer, g_time + 10000);
    CHECK_EQ_INT(PL1.freezeTimer, 0);
    CHECK(Queued(g_sfxFreeze));
}

TEST(Play_Pickup_freeze_in_dual_mode_freezes_both_players)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    g_curPlayer = 1;
    Pickup(ITEM_FREEZE);
    CHECK_EQ_INT(PL0.freezeTimer, g_time + 10000);
    CHECK_EQ_INT(PL1.freezeTimer, g_time + 10000);
}

TEST(Play_Pickup_times2_and_times5_set_the_score_multiplier)
{
    StartPlay();
    PL0.buffDuration = 20;
    Pickup(ITEM_TIMES5);
    CHECK_EQ_INT(g_scoreMul[0], 5);
    CHECK_EQ_INT(PL0.scoreMult5Timer, g_time + 20000);
    CHECK(Queued(g_sfxTimes5));
    Pickup(ITEM_TIMES2);
    CHECK_EQ_INT(g_scoreMul[0], 2);
    CHECK_EQ_INT(PL0.scoreMult2Timer, g_time + 20000);
    CHECK_EQ_INT(PL0.scoreMult5Timer, 0);
    Pickup(ITEM_TIMES5);
    CHECK_EQ_INT(PL0.scoreMult2Timer, 0);
}

TEST(Play_Pickup_extra_bullet_adds_one_up_to_50)
{
    StartPlay();
    Pickup(ITEM_EXTRA_BULLET);
    CHECK_EQ_INT(PL0.bullets, 9);
    CHECK_STR(g_alertMsg, "EXTRA BULLET");
    CHECK_EQ_INT(g_msgTimer, g_time + 1000);
    PL0.bullets = 49;
    Pickup(ITEM_EXTRA_BULLET);
    CHECK_EQ_INT(PL0.bullets, 50);
    CHECK_EQ_INT(PL0.score, 0);
    Pickup(ITEM_EXTRA_BULLET);
    CHECK_EQ_INT(PL0.bullets, 50);
    CHECK_EQ_INT(PL0.score, 25000);
}

TEST(Play_Pickup_extra_speed_is_capped)
{
    StartPlay();
    float cap = g_speedStep * g_maxSpeedMul + g_speedBase;
    PL0.speed = g_speedBase;
    Pickup(ITEM_EXTRA_SPEED);
    CHECK_NEAR(PL0.speed, g_speedBase + g_speedStep, 1e-4);
    CHECK_EQ_INT(PL0.score, 0);
    PL0.speed = cap - g_speedStep;
    Pickup(ITEM_EXTRA_SPEED);
    CHECK_NEAR(PL0.speed, cap, 1e-4);
    CHECK_EQ_INT(PL0.score, 0);
    Pickup(ITEM_EXTRA_SPEED);
    CHECK_NEAR(PL0.speed, cap, 1e-4);
    CHECK_EQ_INT(PL0.score, 25000);
    CHECK_STR(g_alertMsg, "EXTRA SPEED");
}

TEST(Play_Pickup_shield_lasts_the_buff_duration)
{
    StartPlay();
    PL0.buffDuration = 23;
    Pickup(ITEM_SHIELD);
    CHECK_EQ_INT(PL0.shieldTimer, g_time + 23000);
    CHECK_STR(g_alertMsg, "SHIELD");
    CHECK(Queued(g_sfxShield));
    CHECK_EQ_INT(Plays(g_sfxShieldHum), 1);
}

TEST(Play_Pickup_mirror_and_drunk_modes_last_the_buff_duration)
{
    StartPlay();
    PL0.buffDuration = 21;
    PL0.x = 333;
    Pickup(ITEM_MIRROR);
    CHECK_EQ_INT(PL0.mirrorTime, g_time + 21000);
    CHECK_NEAR(PL0.mirrorX, 333, 1e-6);
    CHECK_STR(g_alertMsg, "MIRROR MODE");
    Pickup(ITEM_DRUNK);
    CHECK_EQ_INT(PL0.drunkModeTimer, g_time + 21000);
    CHECK_STR(g_alertMsg, "DRUNK MODE");
    CHECK_EQ_INT(g_msgTimer, g_time + 2000);
}

TEST(Play_Pickup_scoop_arms_the_scoop_trail)
{
    StartPlay();
    g_scoopRange = 30;
    Pickup(ITEM_SCOOP);
    CHECK_EQ_INT(PL0.scoopTimer, g_time + 20000);
    CHECK_NEAR(g_scoopRange, 0, 1e-6);
    for (int i = 0; i < MAX_SCOOP; i++) {
        CHECK_EQ_INT(g_scoop[i].spawnDelay, i + 5);
        CHECK_EQ_INT(g_scoop[i].timer, 2);
    }
    CHECK_STR(g_alertMsg, "SCOOP");
}

TEST(Play_Pickup_extra_time_adds_5_seconds_up_to_the_max)
{
    StartPlay();
    Pickup(ITEM_EXTRA_TIME);
    CHECK_EQ_INT(PL0.buffDuration, 25);
    PL0.buffDuration = g_timeMax - 1;
    Pickup(ITEM_EXTRA_TIME);
    CHECK_EQ_INT(PL0.buffDuration, g_timeMax + 4);
    CHECK_EQ_INT(PL0.score, 0);
    Pickup(ITEM_EXTRA_TIME);
    CHECK_EQ_INT(PL0.buffDuration, g_timeMax);
    CHECK_EQ_INT(PL0.score, 25000);
}

TEST(Play_Pickup_extra_bullet_speed_adds_a_tenth_up_to_the_max)
{
    StartPlay();
    Pickup(ITEM_EXTRA_BULLET_SPEED);
    CHECK_NEAR(PL0.bulletSpeedMult, 1.1, 1e-5);
    CHECK_STR(g_alertMsg, "EXTRA BULLET SPEED");
    PL0.bulletSpeedMult = g_bulletSpeedMax;
    Pickup(ITEM_EXTRA_BULLET_SPEED);
    CHECK_NEAR(PL0.bulletSpeedMult, g_bulletSpeedMax, 1e-6);
    CHECK_EQ_INT(PL0.score, 25000);
}

// ---------------------------------------------------------------- weapons

TEST(Play_Pickup_weapons_switch_to_that_weapon)
{
    StartPlay();
    Pickup(ITEM_WEAPON_DOUBLE);
    CHECK_EQ_INT(PL0.weapon, WEAPON_DOUBLE);
    CHECK_STR(g_alertMsg, "DOUBLE SHOT");
    Pickup(ITEM_WEAPON_TRIPLE);
    CHECK_EQ_INT(PL0.weapon, WEAPON_TRIPLE);
    CHECK_STR(g_alertMsg, "TRIPLE SHOT");
    Pickup(ITEM_WEAPON_QUAD);
    CHECK_EQ_INT(PL0.weapon, WEAPON_QUAD);
    CHECK_STR(g_alertMsg, "QUAD SHOT");
    Pickup(ITEM_WEAPON_SINGLE);
    CHECK_EQ_INT(PL0.weapon, WEAPON_SINGLE);
    CHECK_STR(g_alertMsg, "SINGLE SHOT");
    CHECK_EQ_INT(PL0.bullets, 8);
}

TEST(Play_Pickup_the_weapon_you_have_gives_an_extra_bullet)
{
    StartPlay();
    PL0.weapon = WEAPON_TRIPLE;
    Pickup(ITEM_WEAPON_TRIPLE);
    CHECK_EQ_INT(PL0.weapon, WEAPON_TRIPLE);
    CHECK_EQ_INT(PL0.bullets, 9);
    CHECK_STR(g_alertMsg, "EXTRA BULLET");
    PL0.weapon = WEAPON_QUAD;
    Pickup(ITEM_WEAPON_QUAD);
    CHECK_EQ_INT(PL0.bullets, 10);
    PL0.weapon = WEAPON_SINGLE;
    Pickup(ITEM_WEAPON_SINGLE);
    CHECK_EQ_INT(PL0.bullets, 11);
    PL0.weapon = WEAPON_DOUBLE;
    PL0.bullets = 50;
    Pickup(ITEM_WEAPON_DOUBLE);
    CHECK_EQ_INT(PL0.bullets, 50);
    CHECK_EQ_INT(PL0.score, 25000);
}

TEST(Play_Pickup_single_shot_laughs_at_a_big_weapon)
{
    StartPlay();
    PL0.weapon = WEAPON_LASER;
    Pickup(ITEM_WEAPON_SINGLE);
    CHECK_EQ_INT(PL0.weapon, WEAPON_SINGLE);
    CHECK_EQ_INT(Plays(g_sfxHahaha), 2);
    PL0.weapon = WEAPON_QUAD;
    Pickup(ITEM_WEAPON_SINGLE);
    CHECK_EQ_INT(Plays(g_sfxHahaha), 3);
    PL0.weapon = WEAPON_DOUBLE;
    Pickup(ITEM_WEAPON_SINGLE);
    CHECK_EQ_INT(Plays(g_sfxHahaha), 3);
}

TEST(Play_Pickup_autofire_turns_it_on_then_scores)
{
    StartPlay();
    PL0.autofireTimer = 77;
    Pickup(ITEM_AUTOFIRE);
    CHECK_EQ_INT(PL0.autofire, 1);
    CHECK_EQ_INT(PL0.autofireTimer, g_time);
    CHECK_STR(g_alertMsg, "AUTO FIRE");
    CHECK_EQ_INT(PL0.score, 0);
    Pickup(ITEM_AUTOFIRE);
    CHECK_EQ_INT(PL0.score, 25000);
}

TEST(Play_Pickup_autofire_leaves_super_autofire_alone)
{
    StartPlay();
    PL0.superAuto = 1;
    PL0.autofire = 0;
    PL0.autofireTimer = 77;
    Pickup(ITEM_AUTOFIRE);
    CHECK_EQ_INT(PL0.autofire, 0);
    CHECK_EQ_INT(PL0.autofireTimer, 77);
    CHECK_EQ_INT(PL0.score, 25000);
}

// ---------------------------------------------------------------- armour, lives, warp

TEST(Play_Pickup_armour_adds_a_step_up_to_the_max)
{
    StartPlay();
    Pickup(ITEM_ARMOUR);
    CHECK_EQ_INT(PL0.armour, ARMOUR0 + 5);
    CHECK_STR(g_alertMsg, "ARMOUR");
    CHECK_EQ_INT(g_msgTimer, g_time + 1000);
    PL0.armour = MAX_ARMOUR - 2;
    Pickup(ITEM_ARMOUR);
    CHECK_EQ_INT(PL0.armour, MAX_ARMOUR);
    CHECK_EQ_INT(PL0.score, 0);
    Pickup(ITEM_ARMOUR);
    CHECK_EQ_INT(PL0.armour, MAX_ARMOUR);
    CHECK_EQ_INT(PL0.score, 25000);
}

TEST(Play_Pickup_extra_life_adds_a_life_then_armour_then_score)
{
    StartPlay();
    Pickup(ITEM_EXTRA_LIFE);
    CHECK_EQ_INT(PL0.lives, LIVES0 + 4);
    CHECK_STR(g_alertMsg, "EXTRA LIFE");
    CHECK(Queued(g_sfxExtraLife));
    PL0.lives = MAX_LIVES - 1;
    Pickup(ITEM_EXTRA_LIFE);
    CHECK_EQ_INT(PL0.lives, MAX_LIVES);
    CHECK_EQ_INT(PL0.armour, ARMOUR0);
    Pickup(ITEM_EXTRA_LIFE);
    CHECK_EQ_INT(PL0.lives, MAX_LIVES);
    CHECK_EQ_INT(PL0.armour, ARMOUR0 + 5);
    CHECK_EQ_INT(PL0.score, 0);
    PL0.armour = MAX_ARMOUR;
    Pickup(ITEM_EXTRA_LIFE);
    CHECK_EQ_INT(PL0.score, 1000000);
}

TEST(Play_Pickup_warp_finishes_the_level_and_toughens_enemies)
{
    StartPlay();
    Pickup(ITEM_WARP);
    CHECK_EQ_INT(PL0.levelFinished, 1);
    CHECK_EQ_INT(PL0.levelWarpPending, 1);
    CHECK_NEAR(PL0.shieldHitFlashSpeed, 3.5, 1e-6);
    CHECK_EQ_INT(PL0.enemyHpBonusRoll, 10);
    CHECK_STR(g_alertMsg, "WARP");
    PL0.shieldHitFlashSpeed = 7.8f;
    PL0.enemyHpBonusRoll = 74;
    Pickup(ITEM_WARP);
    CHECK_NEAR(PL0.shieldHitFlashSpeed, 8.0, 1e-6);
    CHECK_EQ_INT(PL0.enemyHpBonusRoll, 75);
    CHECK_EQ_INT(PL1.levelFinished, 0);
}

TEST(Play_Pickup_warp_in_dual_mode_warps_both_players)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    PL1.enemyHpBonusRoll = 74;
    Pickup(ITEM_WARP);
    CHECK_EQ_INT(PL0.levelFinished, 1);
    CHECK_EQ_INT(PL1.levelFinished, 1);
    CHECK_EQ_INT(PL1.levelWarpPending, 1);
    CHECK_EQ_INT(PL0.enemyHpBonusRoll, 10);
    CHECK_EQ_INT(PL1.enemyHpBonusRoll, 75);
    CHECK_NEAR(PL1.shieldHitFlashSpeed, 3.5, 1e-6);
}

// ---------------------------------------------------------------- EXTRA / ARTXE letters

TEST(Play_Pickup_letters_in_order_spell_EXTRA)
{
    StartPlay();
    PL0.armour = ARMOUR0;
    Pickup(ITEM_LETTER_E);
    CHECK_EQ_INT(PL0.extraLetterE, 1);
    CHECK_EQ_INT(PL0.extraProgress, 'E');
    Pickup(ITEM_LETTER_X);
    CHECK_EQ_INT(PL0.extraProgress, 'X');
    Pickup(ITEM_LETTER_T);
    CHECK_EQ_INT(PL0.extraProgress, 'T');
    Pickup(ITEM_LETTER_R);
    CHECK_EQ_INT(PL0.extraProgress, 'R');
    CHECK_EQ_INT(PL0.lives, LIVES0);
    Pickup(ITEM_LETTER_A);
    // EXTRA fills lives and armour; the five letters then also complete (COMPLETE), which
    // finds both full and pays the 1,000,000 instead
    CHECK_EQ_INT(PL0.lives, MAX_LIVES);
    CHECK_EQ_INT(PL0.armour, MAX_ARMOUR);
    CHECK_STR(g_alertMsg, "*** E X T R A ***");
    CHECK_EQ_INT(g_msgTimer, g_time + 3000);
    CHECK_EQ_INT(PL0.score, 1000000);
    CHECK_EQ_INT(PL0.extraLetterE, 0);
    CHECK_EQ_INT(PL0.extraLetterA, 0);
    CHECK_EQ_INT(PL0.extraProgress, ' ');
}

TEST(Play_Pickup_super_EXTRA_when_already_full)
{
    StartPlay();
    PL0.lives = MAX_LIVES;
    PL0.armour = MAX_ARMOUR;
    Pickup(ITEM_LETTER_E);
    Pickup(ITEM_LETTER_X);
    Pickup(ITEM_LETTER_T);
    Pickup(ITEM_LETTER_R);
    Pickup(ITEM_LETTER_A);
    CHECK_STR(g_alertMsg, "*** S U P E R   E X T R A ***");
    CHECK_EQ_INT(PL0.score, 5000000 + 1000000);
    CHECK_EQ_INT(g_completePopupYOffset, 0x32);
}

TEST(Play_Pickup_letters_backwards_spell_ARTXE)
{
    StartPlay();
    Pickup(ITEM_LETTER_A);
    CHECK_EQ_INT(PL0.artxeProgress, 'A');
    Pickup(ITEM_LETTER_R);
    CHECK_EQ_INT(PL0.artxeProgress, 'R');
    Pickup(ITEM_LETTER_T);
    CHECK_EQ_INT(PL0.artxeProgress, 'T');
    Pickup(ITEM_LETTER_X);
    CHECK_EQ_INT(PL0.artxeProgress, 'X');
    Pickup(ITEM_LETTER_E);
    CHECK_EQ_INT(PL0.lives, MAX_LIVES);
    CHECK_EQ_INT(PL0.armour, MAX_ARMOUR);
    CHECK_STR(g_alertMsg, "*** A R T X E ***");
    CHECK_EQ_INT(PL0.score, 1000000);
}

TEST(Play_Pickup_super_ARTXE_when_already_full)
{
    StartPlay();
    PL0.lives = MAX_LIVES;
    PL0.armour = MAX_ARMOUR;
    Pickup(ITEM_LETTER_A);
    Pickup(ITEM_LETTER_R);
    Pickup(ITEM_LETTER_T);
    Pickup(ITEM_LETTER_X);
    Pickup(ITEM_LETTER_E);
    CHECK_STR(g_alertMsg, "*** S U P E R   A R T X E ***");
    CHECK_EQ_INT(PL0.score, 5000000 + 1000000);
}

TEST(Play_Pickup_all_five_letters_out_of_order_give_a_life)
{
    StartPlay();
    Pickup(ITEM_LETTER_X);
    Pickup(ITEM_LETTER_E);
    Pickup(ITEM_LETTER_T);
    Pickup(ITEM_LETTER_R);
    CHECK_EQ_INT(PL0.lives, LIVES0);
    Pickup(ITEM_LETTER_A);
    CHECK_EQ_INT(PL0.lives, LIVES0 + 4);
    CHECK_EQ_INT(PL0.armour, ARMOUR0);
    CHECK_EQ_INT(PL0.score, 0);
    CHECK_EQ_INT(PL0.extraLetterE + PL0.extraLetterX + PL0.extraLetterT + PL0.extraLetterR +
                 PL0.extraLetterA, 0);
    CHECK_EQ_INT(Plays(g_sfxFanfare), 1);
}

TEST(Play_Pickup_complete_letters_with_full_lives_give_armour)
{
    StartPlay();
    PL0.lives = MAX_LIVES;
    Pickup(ITEM_LETTER_A);
    Pickup(ITEM_LETTER_E);
    Pickup(ITEM_LETTER_X);
    Pickup(ITEM_LETTER_R);
    Pickup(ITEM_LETTER_T);
    CHECK_EQ_INT(PL0.lives, MAX_LIVES);
    CHECK_EQ_INT(PL0.armour, ARMOUR0 + 5);
    CHECK_STR(g_alertMsg, "ARMOUR");
}

TEST(Play_Pickup_a_letter_twice_scores_100)
{
    StartPlay();
    Pickup(ITEM_LETTER_T);
    CHECK_EQ_INT(PL0.score, 0);
    Pickup(ITEM_LETTER_T);
    CHECK_EQ_INT(PL0.score, 100);
    g_scoreMul[0] = 5;
    Pickup(ITEM_LETTER_T);
    CHECK_EQ_INT(PL0.score, 600);
    CHECK(Queued(g_sfxVoiceLetterT));
}

TEST(Play_Pickup_a_wrong_letter_breaks_the_word)
{
    StartPlay();
    Pickup(ITEM_LETTER_E);
    Pickup(ITEM_LETTER_X);
    Pickup(ITEM_LETTER_R);
    CHECK_EQ_INT(PL0.extraProgress, ' ');
    Pickup(ITEM_LETTER_T);
    CHECK_EQ_INT(PL0.extraProgress, ' ');
}

// ---------------------------------------------------------------- money

TEST(Play_Pickup_money_bags_add_their_value)
{
    StartPlay();
    Pickup(ITEM_MONEY_SMALL);
    CHECK_EQ_INT(PL0.money, 10);
    Pickup(ITEM_MONEY_MEDIUM);
    CHECK_EQ_INT(PL0.money, 60);
    Pickup(ITEM_MONEY_LARGE);
    CHECK_EQ_INT(PL0.money, 160);
    Pickup(ITEM_MONEY_BLUE);
    CHECK_EQ_INT(PL0.money, 360);
    CHECK_STR(g_alertMsg, "MONEY");
    CHECK_EQ_INT(g_moneyMax >= 360, 1);
    CHECK_EQ_INT(Plays(g_sfxCoin), 4);
}

TEST(Play_Pickup_money_over_the_wallet_size_becomes_score)
{
    StartPlay();
    PL0.money = PL0.moneyMax - 30;
    Pickup(ITEM_MONEY_MEDIUM);
    CHECK_EQ_INT(PL0.money, PL0.moneyMax);
    CHECK_EQ_INT(PL0.score, 500);
    g_scoreMul[0] = 2;
    Pickup(ITEM_MONEY_BLUE);
    CHECK_EQ_INT(PL0.score, 500 + 4000);
}

TEST(Play_Pickup_money_raises_the_session_money_record)
{
    StartPlay();
    g_moneyMax = 1000;
    PL0.money = 995;
    Pickup(ITEM_MONEY_SMALL);
    CHECK_EQ_INT(g_moneyMax, 1005);
    PL0.money = 0;
    Pickup(ITEM_MONEY_SMALL);
    CHECK_EQ_INT(g_moneyMax, 1005);
}

TEST(Play_Pickup_money_doubler_doubles_below_450000)
{
    StartPlay();
    PL0.money = 300;
    Pickup(ITEM_MONEY_DOUBLER);
    CHECK_EQ_INT(PL0.money, 600);
    CHECK_STR(g_alertMsg, "MONEY DOUBLER");
    CHECK_EQ_INT(g_msgTimer, g_time + 2000);
    CHECK_EQ_INT(Plays(g_sfxChaching), 1);
}

TEST(Play_Pickup_money_doubler_malfunctions_from_450000)
{
    StartPlay();
    PL0.moneyMax = 2000000;
    PL0.money = 450000;
    Pickup(ITEM_MONEY_DOUBLER);
    CHECK_EQ_INT(PL0.money, 450000);
    CHECK_STR(g_alertMsg, "MONEY DOUBLER MALFUNCTION");
    PL0.money = 449999;
    Pickup(ITEM_MONEY_DOUBLER);
    CHECK_EQ_INT(PL0.money, 899998);
}

TEST(Play_Pickup_money_doubler_overflow_pays_double_the_wallet)
{
    StartPlay();
    PL0.money = 60000;
    Pickup(ITEM_MONEY_DOUBLER);
    CHECK_EQ_INT(PL0.money, 99990);
    CHECK_EQ_INT(PL0.score, 99990 * 2);
}

TEST(Play_Pickup_money_doubler_with_no_money_adds_time)
{
    StartPlay();
    PL0.money = 0;
    PL0.buffDuration = 10;
    Pickup(ITEM_MONEY_DOUBLER);
    CHECK_EQ_INT(PL0.buffDuration, 40);
    Pickup(ITEM_MONEY_DOUBLER);
    CHECK_EQ_INT(PL0.buffDuration, g_timeMax);
    PL0.money = 5;
    PL0.buffDuration = 10;
    Pickup(ITEM_MONEY_DOUBLER);
    CHECK_EQ_INT(PL0.buffDuration, 10);
}

// ---------------------------------------------------------------- sucker traps

TEST(Play_Pickup_sucker_penalizes_like_a_hit)
{
    StartPlay();
    PL0.bullets = 8; PL0.weapon = 2; PL0.speed = g_speedBase + g_speedStep; PL0.buffDuration = 30;
    g_bonusWeight[27] = 14; g_bonusWeight[33] = 15; g_bonusWeight[26] = 3;
    Pickup(ITEM_SUCKER_BLUE_MONEY);
    CHECK_EQ_INT(PL0.bullets, 7);
    CHECK_EQ_INT(PL0.weapon, 1);
    CHECK_NEAR(PL0.speed, g_speedBase, 1e-4);
    CHECK_EQ_INT(PL0.buffDuration, 25);
    CHECK_EQ_INT(PL0.blueMoneyActive, 1);
    CHECK_STR(g_alertMsg, "SUCKER");
    CHECK_EQ_INT(g_bonusWeight[27], 15);
    CHECK_EQ_INT(g_bonusWeight[33], 15);
    CHECK_EQ_INT(g_bonusWeight[26], 4);
}

TEST(Play_Pickup_sucker_floors)
{
    StartPlay();
    PL0.bullets = 4; PL0.weapon = 0; PL0.speed = g_speedBase; PL0.buffDuration = g_speedMin + 1;
    Pickup(ITEM_SUCKER_GEMS);
    CHECK_EQ_INT(PL0.bullets, 4);
    CHECK_EQ_INT(PL0.weapon, 0);
    CHECK_NEAR(PL0.speed, g_speedBase, 1e-6);
    CHECK_EQ_INT(PL0.buffDuration, g_speedMin);
    CHECK_EQ_INT(PL0.gemCounterCollected, 1);
    PL0.bullets = 5;
    PL0.weaponFloorAtOne = 1;
    PL0.weapon = 1;
    Pickup(ITEM_SUCKER_MULTIPLIER);
    CHECK_EQ_INT(PL0.bullets, 4);
    CHECK_EQ_INT(PL0.weapon, 1);
}

TEST(Play_Pickup_three_blue_money_suckers_unlock_blue_money)
{
    StartPlay();
    Pickup(ITEM_SUCKER_BLUE_MONEY);
    Pickup(ITEM_SUCKER_BLUE_MONEY);
    CHECK_EQ_INT(PL0.blueMoneyUnlocked, 0);
    CHECK_EQ_INT(PL0.blueMoneyPicks, 2);
    Pickup(ITEM_SUCKER_BLUE_MONEY);
    CHECK_EQ_INT(PL0.blueMoneyUnlocked, 1);
    CHECK_EQ_INT(PL0.blueMoneyPicks, 0);
    CHECK_STR(g_alertMsg, "** BLUE MONEY ACTIVATED **");
    CHECK_EQ_INT(g_msgTimer, g_time + 3000);
}

TEST(Play_Pickup_three_gem_suckers_unlock_the_gem_counter)
{
    StartPlay();
    Pickup(ITEM_SUCKER_GEMS);
    Pickup(ITEM_SUCKER_GEMS);
    Pickup(ITEM_SUCKER_GEMS);
    CHECK_EQ_INT(PL0.gemCounterUnlocked, 1);
    CHECK_STR(g_alertMsg, "** GEM COUNTER ACTIVATED **");
    CHECK_EQ_INT(g_bonusWeight[19] <= 0x2d, 1);
}

TEST(Play_Pickup_three_multiplier_suckers_unlock_the_multiplier)
{
    StartPlay();
    Pickup(ITEM_SUCKER_MULTIPLIER);
    Pickup(ITEM_SUCKER_MULTIPLIER);
    Pickup(ITEM_SUCKER_MULTIPLIER);
    CHECK_EQ_INT(PL0.multiplierUnlocked, 1);
    CHECK_STR(g_alertMsg, "** MULTIPLIER IN M.S. ENABLED **");
}

TEST(Play_Pickup_one_of_each_sucker_gives_super_triple_shot)
{
    StartPlay();
    PL0.weapon = 2;
    Pickup(ITEM_SUCKER_BLUE_MONEY);
    Pickup(ITEM_SUCKER_GEMS);
    CHECK_EQ_INT(PL0.weapon, 0);
    Pickup(ITEM_SUCKER_MULTIPLIER);
    CHECK_EQ_INT(PL0.weapon, WEAPON_SUPER_TRIPLE);
    CHECK_EQ_INT(PL0.bullets, 25);
    CHECK_NEAR(PL0.speed, g_speedStep * g_speedMax + g_speedBase, 1e-4);
    CHECK_EQ_INT(PL0.buffDuration, 30);
    CHECK_EQ_INT(PL0.blueMoneyActive, 0);
    CHECK_EQ_INT(PL0.gemCounterCollected, 0);
    CHECK_EQ_INT(PL0.msMultiplierActive, 0);
    CHECK_STR(g_alertMsg, "SUPER TRIPLE SHOT");
}

TEST(Play_Pickup_super_triple_keeps_a_better_weapon)
{
    StartPlay();
    PL0.weapon = WEAPON_LASER;
    PL0.bullets = 40;
    PL0.blueMoneyActive = 1;
    PL0.gemCounterCollected = 1;
    Pickup(ITEM_SUCKER_MULTIPLIER);
    CHECK_EQ_INT(PL0.weapon, WEAPON_LASER - 1);
    CHECK_EQ_INT(PL0.bullets, 39);
}

// ---------------------------------------------------------------- gems and rank gems

TEST(Play_Pickup_gem_adds_a_gem_and_1000_points)
{
    StartPlay();
    Pickup(ITEM_GEM);
    CHECK_EQ_INT(PL0.gems, 452 + 8);
    CHECK_EQ_INT(PL0.score, 1000);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    PL0.gems = 452 + 49 * 8;   // the 50th gem is nothing special
    Pickup(ITEM_GEM);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
}

TEST(Play_Pickup_every_100th_gem_starts_the_gem_drop)
{
    StartPlay();
    PL0.gems = 452 + 98 * 8;
    Pickup(ITEM_GEM);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    Pickup(ITEM_GEM);
    CHECK_EQ_INT(g_state, STATE_GEM_DROP);
    CHECK_STR(g_alertMsg, "G E M   D R O P");
    CHECK_EQ_INT(g_superGemDrop, 0);
}

TEST(Play_Pickup_the_1000th_gem_is_a_super_gem_drop)
{
    StartPlay();
    PL0.gems = 452 + 999 * 8;
    Pickup(ITEM_GEM);
    CHECK_EQ_INT(g_state, STATE_GEM_DROP);
    CHECK_EQ_INT(g_superGemDrop, 1);
    CHECK_EQ_INT(PL0.gems, 452 + 8);
    CHECK_STR(g_alertMsg, "S U P E R   G E M   D R O P");
}

TEST(Play_Pickup_rank_gems_set_their_mark)
{
    StartPlay();
    Pickup(ITEM_RANK_GEM_3);
    CHECK_EQ_INT((short)PL0.marks, MARK_3);
    CHECK_EQ_INT(PL0.score, 5000);
    CHECK_EQ_INT(PL0.gemPickups, 1);
    Pickup(ITEM_RANK_GEM_1);
    CHECK_EQ_INT(PL0.bulkLevelsCooldown, 0);   // not after a 2
    Pickup(ITEM_RANK_GEM_2);
    Pickup(ITEM_RANK_GEM_4);
    Pickup(ITEM_RANK_GEM_5);
    CHECK_EQ_INT(PlayerHasAllMarks(0), 0);
    Pickup(ITEM_RANK_GEM_6);
    CHECK_EQ_INT((short)PL0.marks, MARKS_ALL);
    CHECK_EQ_INT(PlayerHasAllMarks(0), 1);
    CHECK_EQ_INT(PL0.gemPickups, 6);
    CHECK(Queued(g_sfxRankMarker));
}

TEST(Play_Pickup_rank_gems_1_to_6_in_order_give_super_autofire)
{
    StartPlay();
    Pickup(ITEM_RANK_GEM_1);
    CHECK_EQ_INT(PL0.gemSeqB, 1);
    Pickup(ITEM_RANK_GEM_2);
    CHECK_EQ_INT(PL0.gemSeqB, 2);
    Pickup(ITEM_RANK_GEM_3);
    CHECK_EQ_INT(PL0.gemSeqB, 4);
    Pickup(ITEM_RANK_GEM_4);
    CHECK_EQ_INT(PL0.gemSeqB, 8);
    Pickup(ITEM_RANK_GEM_5);
    CHECK_EQ_INT(PL0.gemSeqB, 0x10);
    CHECK_EQ_INT(PL0.superAuto, 0);
    Pickup(ITEM_RANK_GEM_6);
    CHECK_EQ_INT(PL0.autofire, 1);
    CHECK_EQ_INT(PL0.superAuto, 1);
    CHECK_EQ_INT(PL0.autofireInterval, 0x19);
    CHECK_STR(g_alertMsg, "*** SUPER AUTO FIRE ON ***");
    CHECK_EQ_INT(PL0.gemSeqB, -1);
}

TEST(Play_Pickup_rank_gems_out_of_order_break_the_sequence)
{
    StartPlay();
    Pickup(ITEM_RANK_GEM_1);
    Pickup(ITEM_RANK_GEM_2);
    Pickup(ITEM_RANK_GEM_4);
    CHECK_EQ_INT(PL0.gemSeqB, -1);
    Pickup(ITEM_RANK_GEM_5);
    Pickup(ITEM_RANK_GEM_6);
    CHECK_EQ_INT(PL0.superAuto, 0);
    Pickup(ITEM_RANK_GEM_1);
    CHECK(strcmp(g_alertMsg, "*** 500 LEVELS ADDED ***") != 0);
    CHECK_EQ_INT(PL0.bulkLevelsCooldown, 0);
}

TEST(Play_Pickup_rank_gems_6_to_1_add_500_levels)
{
    StartPlay();
    Pickup(ITEM_RANK_GEM_6);
    CHECK_EQ_INT(PL0.gemSeqA, 0x20);
    Pickup(ITEM_RANK_GEM_5);
    CHECK_EQ_INT(PL0.gemSeqA, 0x10);
    Pickup(ITEM_RANK_GEM_4);
    CHECK_EQ_INT(PL0.gemSeqA, 8);
    Pickup(ITEM_RANK_GEM_3);
    CHECK_EQ_INT(PL0.gemSeqA, 4);
    Pickup(ITEM_RANK_GEM_2);
    CHECK_EQ_INT(PL0.gemSeqA, 2);
    Pickup(ITEM_RANK_GEM_1);
    CHECK_STR(g_alertMsg, "*** 500 LEVELS ADDED ***");
    CHECK_EQ_INT(PL0.bulkLevelsCooldown, 50);
    CHECK_EQ_INT(PL0.gemSeqA, -1);
}

TEST(Play_Pickup_500_levels_waits_for_its_cooldown)
{
    StartPlay();
    PL0.bulkLevelsCooldown = 3;
    PL0.gemSeqA = 2;
    Pickup(ITEM_RANK_GEM_1);
    CHECK_EQ_INT(PL0.bulkLevelsCooldown, 3);
    CHECK(strcmp(g_alertMsg, "*** 500 LEVELS ADDED ***") != 0);
}

TEST(Play_Pickup_bad_gem_clears_the_marks_and_costs_rank)
{
    StartPlay();
    PL0.marks = MARK_1 | MARK_2;
    PL0.gemSeqA = 4;
    PL0.gemSeqB = 4;
    PL0.rank = 10;
    Rng r = SaveRng();
    RandRange(9000, 0x2774);   // the bell's pitch
    int loss = RandRange(2, 5);
    RestoreRng(r);
    Pickup(ITEM_BAD_GEM);
    CHECK_EQ_INT((short)PL0.marks, 0);
    CHECK_EQ_INT(PL0.gemSeqA, -1);
    CHECK_EQ_INT(PL0.gemSeqB, -1);
    CHECK_EQ_INT(PL0.rank, 10 - loss);
    PL0.rank = 1;
    Pickup(ITEM_BAD_GEM);
    CHECK_EQ_INT(PL0.rank, 0);
    CHECK(Queued(g_sfxOhNo));
}

// ---------------------------------------------------------------- bombs

TEST(Play_Pickup_gem_bomb_destroys_enemies_and_drops_gems)
{
    StartPlay();
    AddEnemy(0, 3, ENEMY_HOVER, 200, 100);
    g_enemies[0][3].offsetX = 10;
    g_enemies[0][3].offsetY = 20;
    AddEnemy(0, 4, ENEMY_DIVING, 300, 150);
    AddEnemy(0, 5, ENEMY_MOTHERSHIP, 400, 50);
    AddEnemy(0, 6, ENEMY_GUARD, 400, 50);
    AddEnemy(0, 7, ENEMY_HOVER, 400, 50);
    g_enemies[0][7].attackStaggerTimer = 2;
    AddEnemy(0, 8, ENEMY_CAPTURED, 400, 50);
    Pickup(ITEM_GEM_BOMB);
    CHECK_EQ_INT(g_enemies[0][3].active, 0);
    CHECK_NEAR(g_enemies[0][3].hp, 0, 1e-6);
    CHECK_EQ_INT(g_enemies[0][4].active, 0);
    CHECK_EQ_INT(g_enemies[0][5].active, 1);
    CHECK_EQ_INT(g_enemies[0][6].active, 1);
    CHECK_EQ_INT(g_enemies[0][7].active, 1);
    CHECK_EQ_INT(g_enemies[0][8].active, 1);
    CHECK_EQ_INT(PL0.killed, 2);
    CHECK_EQ_INT(PL0.bombPickups, 1);
    CHECK_EQ_INT(CountItems(ITEM_GEM), 2);
    // a hovering enemy's gem drops where it's drawn (x + offsetX)
    CHECK_NEAR(g_items[0].x, 200 + 10 + 9, 1e-4);
    CHECK_NEAR(g_items[0].y, 100 + 20 + 2, 1e-4);
    CHECK_NEAR(g_items[1].x, 300 + 9, 1e-4);
    CHECK_STR(g_alertMsg, "GEM BOMB");
    CHECK_EQ_INT(Plays(g_sfxGemBomb), 1);
}

TEST(Play_Pickup_gem_bomb_on_a_money_sucker_drops_five_gems)
{
    StartPlay();
    AddEnemy(0, 0, ENEMY_MONEY_SUCKER, 200, 100);
    Pickup(ITEM_GEM_BOMB);
    CHECK_EQ_INT(g_enemies[0][0].active, 0);
    CHECK_EQ_INT(CountItems(ITEM_GEM), 5);
}

TEST(Play_Pickup_gem_bomb_on_a_wrapper_drops_a_coin)
{
    StartPlay();
    AddEnemy(0, 0, ENEMY_WRAPPER, 200, 100);
    Pickup(ITEM_GEM_BOMB);
    CHECK_EQ_INT(g_enemies[0][0].active, 0);
    CHECK_EQ_INT(CountItems(ITEM_GEM), 0);
    CHECK_EQ_INT(AliveItems(), 1);
    CHECK_NEAR(g_items[0].x, 222, 1e-4);
}

TEST(Play_Pickup_gem_bomb_in_a_bonus_wave_counts_every_enemy)
{
    StartPlay();
    PL0.trackKillsFlag = 1;
    PL0.totalEnemies = 40;
    AddEnemy(0, 0, ENEMY_HOVER, 200, 100);
    Pickup(ITEM_GEM_BOMB);
    CHECK_EQ_INT(PL0.bonusKilled, 40);
}

TEST(Play_Pickup_money_bomb_drops_money_instead_of_gems)
{
    StartPlay();
    AddEnemy(0, 3, ENEMY_HOVER, 200, 100);
    AddEnemy(0, 4, ENEMY_MOTHERSHIP, 400, 50);
    Pickup(ITEM_MONEY_BOMB);
    CHECK_EQ_INT(g_enemies[0][3].active, 0);
    CHECK_EQ_INT(g_enemies[0][4].active, 1);
    CHECK_EQ_INT(CountItems(ITEM_GEM), 0);
    CHECK_EQ_INT(AliveItems(), 1);
    CHECK(g_items[0].type >= ITEM_MONEY_SMALL && g_items[0].type <= ITEM_MONEY_BLUE);
    CHECK_NEAR(g_items[0].x, 204, 1e-4);
    CHECK_NEAR(g_items[0].y, 103, 1e-4);
    CHECK_EQ_INT(PL0.bombPickups, 1);
    CHECK_EQ_INT(PL0.killed, 1);
    CHECK_STR(g_alertMsg, "MONEY BOMB");
    CHECK_EQ_INT(Plays(g_sfxMoneyBomb), 1);
}

TEST(Play_Pickup_bomb_in_dual_mode_hits_the_shared_enemies)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    g_curPlayer = 1;
    AddEnemy(0, 3, ENEMY_HOVER, 200, 100);
    AddEnemy(1, 3, ENEMY_HOVER, 200, 100);
    Pickup(ITEM_GEM_BOMB);
    CHECK_EQ_INT(g_enemies[0][3].active, 0);
    CHECK_EQ_INT(g_enemies[1][3].active, 1);
    CHECK_EQ_INT(PL1.killed, 1);
    CHECK_EQ_INT(PL1.bombPickups, 1);
}

// ---------------------------------------------------------------- random bonus, bonus stages

TEST(Play_Pickup_random_bonus_applies_a_rolled_item)
{
    StartPlay();
    for (int k = 0; k < NUM_BONUS_WEIGHTS; k++)
        g_bonusWeight[k] = 0;
    g_bonusWeight[ITEM_ARMOUR] = 5;   // g_itemTypePool[k] == k
    Pickup(ITEM_RANDOM_BONUS);
    CHECK_EQ_INT(PL0.armour, ARMOUR0 + 5);
    CHECK_EQ_INT(PL0.pickupCount, 2);
    CHECK_EQ_INT(g_inRandom, 0);
}

TEST(Play_Pickup_random_bonus_never_rolls_itself)
{
    StartPlay();
    for (int k = 0; k < NUM_BONUS_WEIGHTS; k++)
        g_bonusWeight[k] = 0;
    g_bonusWeight[ITEM_RANDOM_BONUS] = 10000;
    g_bonusWeight[ITEM_EXTRA_TIME] = 10;
    Pickup(ITEM_RANDOM_BONUS);
    CHECK_EQ_INT(PL0.buffDuration, 25);
    CHECK_EQ_INT(PL0.pickupCount, 2);
}

TEST(Play_Pickup_random_bonus_does_nothing_when_nested)
{
    StartPlay();
    g_inRandom = 1;
    Pickup(ITEM_RANDOM_BONUS);
    CHECK_EQ_INT(PL0.pickupCount, 1);
    CHECK_EQ_INT(g_inRandom, 1);
}

TEST(Play_Pickup_memory_station_saves_the_ship_and_enters_the_stage)
{
    StartPlay();
    PL0.hyperspaceFade = 7;
    PL0.starSpeed = 9;
    PL0.tries = 3;
    PL0.shieldL = 1;
    PL0.shieldLIdx = 4;
    AddEnemy(0, 4, ENEMY_CAPTURED, 300, 500);
    Pickup(ITEM_MEMORY_STATION);
    CHECK_EQ_INT(g_state, STATE_MEMORY_STATION);
    CHECK_NEAR(PL0.savedHyperspaceFade, 7, 1e-6);
    CHECK_NEAR(PL0.hyperspaceFade, 0, 1e-6);
    CHECK_NEAR(PL0.savedStarSpeed, 9, 1e-6);
    CHECK_NEAR(PL0.starSpeed, 5, 1e-6);
    CHECK_EQ_INT(PL0.tries, 0);
    CHECK_EQ_INT(PL0.shieldL, 0);
    CHECK_EQ_INT(g_enemies[0][4].active, 0);
}

TEST(Play_Pickup_memory_station_waits_for_the_bonus_results)
{
    StartPlay();
    g_bonusResultsTime = 1;
    Pickup(ITEM_MEMORY_STATION);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
}

TEST(Play_Pickup_memory_station_in_the_demo_returns_to_the_title)
{
    StartPlay();
    g_playerUpdateFn = StateDemo;
    Pickup(ITEM_MEMORY_STATION);
    CHECK_EQ_INT(g_state, STATE_TITLE);
}

TEST(Play_Pickup_meteor_storm_clears_items_and_starts_the_race)
{
    StartPlay();
    g_items[10].alive = 1;
    g_items[10].type = ITEM_SHIELD;
    PL0.mirrorTime = g_time + 1000;
    g_transitionLock = 1;
    Pickup(ITEM_METEOR_STORM);
    CHECK_EQ_INT(g_state, STATE_BONUS_RACE);
    CHECK_EQ_INT(g_items[10].alive, 0);
    CHECK_EQ_INT(PL0.mirrorTime, 0);
    CHECK_EQ_INT(PL0.bonusRoundEnded, 1);
    CHECK_EQ_INT(PL0.bonusRoundScore, 0);
    CHECK_EQ_INT(g_transitionLock, 0);
}

TEST(Play_Pickup_meteor_storm_drops_a_multiplier_once_unlocked)
{
    StartPlay();
    PL0.multiplierUnlocked = 1;
    Pickup(ITEM_METEOR_STORM);
    CHECK_EQ_INT(CountItems(ITEM_TIMES2) + CountItems(ITEM_TIMES5), 1);
    CHECK_NEAR(g_items[0].y, -300, 1e-6);
}

// ---------------------------------------------------------------- spawning

TEST(Play_SpawnItem_drops_a_coin_in_the_first_free_slot)
{
    StartPlay();
    g_items[0].alive = 1;
    g_items[0].type = ITEM_STAR;
    for (int round = 0; round < 30; round++) {
        g_items[1].alive = 0;
        Rng r = SaveRng();
        int n = RandRange(0, 6);
        RestoreRng(r);
        SpawnItem(100, 50, 0);
        CHECK_EQ_INT(g_items[1].alive, 1);
        CHECK_EQ_INT(g_items[1].active, 1);
        CHECK_NEAR(g_items[1].x, 122, 1e-6);
        CHECK_NEAR(g_items[1].y, 50, 1e-6);
        CHECK_EQ_INT(g_items[1].type, g_coinTypeTable[n]);
        CHECK_NEAR(g_items[1].vy, g_coinVySpeedTable[n], 1e-6);
        CHECK_EQ_INT(g_items[1].w, 20);
        CHECK_EQ_INT(g_items[2].alive, 0);
    }
}

TEST(Play_SpawnItem_rare_can_drop_the_bad_gem)
{
    StartPlay();
    int bad = 0;
    for (int round = 0; round < 200; round++) {
        g_items[0].alive = 0;
        Rng r = SaveRng();
        int n = RandRange(0, 5) < 1 ? 7 : 6;
        int k = RandRange(0, n);
        RestoreRng(r);
        SpawnItem(100, 50, 1);
        CHECK_EQ_INT(g_items[0].type, g_coinTypeTable[k]);
        bad += g_items[0].type == ITEM_BAD_GEM;
    }
    CHECK(bad > 0);
}

TEST(Play_SpawnItem_does_nothing_when_all_slots_are_used)
{
    StartPlay();
    for (int i = 0; i < MAX_ITEMS; i++) {
        g_items[i].alive = 1;
        g_items[i].type = ITEM_STAR;
    }
    SpawnItem(100, 50, 0);
    SpawnGem(100, 50);
    for (int i = 0; i < MAX_ITEMS; i++)
        CHECK_EQ_INT(g_items[i].type, ITEM_STAR);
}

TEST(Play_SpawnWeaponItem_prefers_a_missing_mark)
{
    StartPlay();
    for (int round = 0; round < 10; round++) {
        g_items[0].alive = 0;
        PL0.marks = MARKS_ALL & ~MARK_3;
        SpawnWeaponItem(100, 50);
        CHECK_EQ_INT(g_items[0].type, ITEM_RANK_GEM_3);
        CHECK_NEAR(g_items[0].x, 122, 1e-6);
        g_items[0].alive = 0;
        PL0.marks = MARKS_ALL & ~MARK_6;
        SpawnWeaponItem(100, 50);
        CHECK_EQ_INT(g_items[0].type, ITEM_RANK_GEM_6);
        g_items[0].alive = 0;
        PL0.marks = MARKS_ALL & ~MARK_1;
        SpawnWeaponItem(100, 50);
        CHECK_EQ_INT(g_items[0].type, ITEM_RANK_GEM_1);
    }
    int seen2 = 0, seen5 = 0;
    PL0.marks = MARKS_ALL & ~(MARK_2 | MARK_5);
    for (int round = 0; round < 40; round++) {
        g_items[0].alive = 0;
        SpawnWeaponItem(100, 50);
        CHECK(g_items[0].type == ITEM_RANK_GEM_2 || g_items[0].type == ITEM_RANK_GEM_5);
        seen2 += g_items[0].type == ITEM_RANK_GEM_2;
        seen5 += g_items[0].type == ITEM_RANK_GEM_5;
    }
    CHECK(seen2 > 0 && seen5 > 0);
}

TEST(Play_SpawnWeaponItem_with_every_mark_drops_any)
{
    StartPlay();
    PL0.marks = MARKS_ALL;
    for (int round = 0; round < 20; round++) {
        g_items[0].alive = 0;
        Rng r = SaveRng();
        int k = RandRange(0, 6);
        RestoreRng(r);
        SpawnWeaponItem(100, 50);
        CHECK_EQ_INT(g_items[0].type, g_coinTypeTable[k]);
    }
}

TEST(Play_SpawnGem_drops_a_gem)
{
    StartPlay();
    int seen[4] = {0};
    for (int round = 0; round < 40; round++) {
        g_items[0].alive = 0;
        Rng r = SaveRng();
        int k = RandRange(0, 4);
        RestoreRng(r);
        SpawnGem(70, 80);
        CHECK_EQ_INT(g_items[0].type, ITEM_GEM);
        CHECK_EQ_INT(g_items[0].srcX, k * 16);
        CHECK_EQ_INT(g_items[0].w, 16);
        CHECK_EQ_INT(g_items[0].h, 13);
        CHECK_NEAR(g_items[0].x, 70, 1e-6);
        CHECK(g_items[0].vy >= 0.5f && g_items[0].vy < 1.2f);
        seen[k]++;
    }
    CHECK(seen[0] && seen[1] && seen[2] && seen[3]);
}

static int MoneyFor(int roll)
{
    if (roll <= 40) return ITEM_MONEY_SMALL;
    if (roll <= 67) return ITEM_MONEY_MEDIUM;
    if (roll <= 87) return ITEM_MONEY_LARGE;
    return ITEM_MONEY_BLUE;
}

TEST(Play_SpawnPowerup_rolls_40_27_20_13_percent_money)
{
    StartPlay();
    int edges = 0;
    for (int round = 0; round < 1500; round++) {
        g_items[0].alive = 0;
        Rng r = SaveRng();
        int roll = RandRange(1, 100);
        RestoreRng(r);
        SpawnPowerup(10, 20);
        CHECK_EQ_INT(g_items[0].type, MoneyFor(roll));
        CHECK_EQ_INT(g_items[0].srcX, g_itemBonusSrcX[MoneyFor(roll)]);
        edges += roll == 40 || roll == 41 || roll == 67 || roll == 68 || roll == 87 || roll == 88;
    }
    CHECK(edges >= 6);
}

TEST(Play_SpawnBonus_rolls_a_weighted_type)
{
    StartPlay();
    g_itemBonusSrcX[ITEM_SCOOP] = 123;
    g_itemBonusSrcX[ITEM_SCOOP + 1] = 456;
    for (int k = 0; k < NUM_BONUS_WEIGHTS; k++)
        g_bonusWeight[k] = 0;
    g_bonusWeight[ITEM_SCOOP] = 3;
    SpawnBonus(200, 100);
    CHECK_EQ_INT(g_items[0].alive, 1);
    CHECK_EQ_INT(g_items[0].type, ITEM_SCOOP);
    CHECK_EQ_INT(g_items[0].srcX, 123);
    CHECK(g_items[0].x >= 197 && g_items[0].x < 203);
    CHECK(g_items[0].vy >= 1.2f && g_items[0].vy < 1.8f);
    g_bonusWeight[ITEM_SCOOP] = 0;
    g_bonusWeight[ITEM_MIRROR] = 10;
    g_bonusWeight[ITEM_FREEZE] = 10;
    int got[2] = {0};
    for (int round = 0; round < 40; round++) {
        g_items[0].alive = 0;
        SpawnBonus(200, 100);
        CHECK(g_items[0].type == ITEM_MIRROR || g_items[0].type == ITEM_FREEZE);
        got[g_items[0].type == ITEM_MIRROR]++;
    }
    CHECK(got[0] > 0 && got[1] > 0);
}

TEST(Play_SpawnPowerupBurst_flings_30_to_44_bags)
{
    StartPlay();
    for (int round = 0; round < 5; round++) {
        memset(g_items, 0, sizeof g_items);
        Rng r = SaveRng();
        int count = RandRange(0, 15) + 30;
        RestoreRng(r);
        SpawnPowerupBurst(300, 200, 0, 0);
        CHECK_EQ_INT(AliveItems(), count);
        for (int i = 0; i < count; i++) {
            CHECK(g_items[i].type >= ITEM_MONEY_SMALL_BURST && g_items[i].type <= ITEM_MONEY_BLUE_BURST);
            CHECK_NEAR(g_items[i].timer, 10, 1e-6);
            CHECK_EQ_INT(g_items[i].fastFall, 0);
        }
    }
}

TEST(Play_SpawnPowerupBurst_blue_money_once_unlocked)
{
    StartPlay();
    PL0.blueMoneyUnlocked = 1;
    Rng r = SaveRng();
    int count = (int)((RandRange(0, 15) + 30) * 0.6f);
    RestoreRng(r);
    SpawnPowerupBurst(300, 200, 1, 1);
    CHECK_EQ_INT(AliveItems(), count);
    CHECK_EQ_INT(CountItems(ITEM_MONEY_BLUE_BURST), count);
    CHECK_EQ_INT(g_items[0].fastFall, 1);
}

TEST(Play_SpawnItems_turns_tally_markers_into_money)
{
    StartPlay();
    g_items[0].alive = 1; g_items[0].type = ITEM_MONEY_MEDIUM_TALLY;
    g_items[1].alive = 1; g_items[1].type = ITEM_MONEY_BLUE_TALLY;
    g_items[2].alive = 1; g_items[2].type = ITEM_MONEY_SMALL_BURST; g_items[2].active = 0;
    g_items[3].alive = 1; g_items[3].type = ITEM_MONEY_LARGE; g_items[3].active = 0;
    g_items[4].alive = 1; g_items[4].type = ITEM_SHIELD; g_items[4].active = 0;
    g_items[5].alive = 0; g_items[5].type = ITEM_MONEY_SMALL_TALLY;
    SpawnItems();
    CHECK_EQ_INT(g_items[0].type, ITEM_MONEY_MEDIUM);
    CHECK_EQ_INT(g_items[0].active, 1);
    CHECK_EQ_INT(g_items[0].srcX, g_itemBonusSrcX[ITEM_MONEY_MEDIUM]);
    CHECK_EQ_INT(g_items[1].type, ITEM_MONEY_BLUE);
    CHECK_EQ_INT(g_items[2].active, 1);
    CHECK_EQ_INT(g_items[3].active, 1);
    CHECK_EQ_INT(g_items[4].active, 0);
    CHECK_EQ_INT(g_items[5].type, ITEM_MONEY_SMALL_TALLY);
}

// ---------------------------------------------------------------- item motion

TEST(Play_UpdateItems_items_fall_and_vanish_below_the_screen)
{
    StartPlay();
    g_items[0].alive = 1; g_items[0].active = 1; g_items[0].type = ITEM_SHIELD;
    g_items[0].x = 100; g_items[0].y = 100; g_items[0].vy = 2; g_items[0].vx = 0.5f;
    g_items[0].frameTimer = 5; g_items[0].frameDelay = 5;
    g_frameDt = 1.5f;
    UpdateItems();
    CHECK_NEAR(g_items[0].y, 103, 1e-4);
    CHECK_NEAR(g_items[0].x, 100.75, 1e-4);
    CHECK_EQ_INT(g_items[0].alive, 1);
    g_items[0].y = 629;
    g_frameDt = 1;
    UpdateItems();
    CHECK_EQ_INT(g_items[0].alive, 0);
}

TEST(Play_UpdateItems_animates_frames)
{
    StartPlay();
    g_items[0].alive = 1; g_items[0].type = ITEM_SHIELD;
    g_items[0].y = 100;
    g_items[0].frame = 2; g_items[0].frameCount = 3;
    g_items[0].frameTimer = 0.5f; g_items[0].frameDelay = 4;
    UpdateItems();
    CHECK_NEAR(g_items[0].frame, 0, 1e-6);   // 3 wraps to 0
    CHECK_NEAR(g_items[0].frameTimer, 4, 1e-6);
    UpdateItems();
    CHECK_NEAR(g_items[0].frame, 0, 1e-6);
    CHECK_NEAR(g_items[0].frameTimer, 3, 1e-6);
}

TEST(Play_UpdateItems_wraps_items_off_the_right_edge)
{
    StartPlay();
    g_items[0].alive = 1; g_items[0].type = ITEM_SHIELD;
    g_items[0].x = 899; g_items[0].y = 100; g_items[0].vx = 2;
    g_items[0].frameTimer = 9;
    UpdateItems();
    CHECK_NEAR(g_items[0].x, -100, 1e-6);
}

TEST(Play_UpdateItems_burst_bags_slow_down_then_fall)
{
    StartPlay();
    g_items[0].alive = 1; g_items[0].type = ITEM_MONEY_LARGE_BURST;
    g_items[0].x = 100; g_items[0].y = 100; g_items[0].vx = 10; g_items[0].vy = 0;
    g_items[0].timer = 1.5f; g_items[0].frameTimer = 9;
    UpdateItems();
    // moved by vx, dragged, then moved again by the generic step
    CHECK_NEAR(g_items[0].x, 100 + 10 + 10 / 1.02, 1e-3);
    CHECK_NEAR(g_items[0].vx, 10 / 1.02, 1e-4);
    CHECK_NEAR(g_items[0].timer, 0.5, 1e-6);
    CHECK_EQ_INT(g_items[0].type, ITEM_MONEY_LARGE_BURST);
    UpdateItems();
    CHECK_EQ_INT(g_items[0].type, ITEM_MONEY_LARGE);
    CHECK_NEAR(g_items[0].vx, 0, 1e-6);
    CHECK(g_items[0].vy >= 1.0f && g_items[0].vy < 2.0f);
}

TEST(Play_UpdateItems_fast_falling_bursts_drop_faster)
{
    StartPlay();
    g_items[0].alive = 1; g_items[0].type = ITEM_MONEY_BLUE_BURST;
    g_items[0].fastFall = 1;
    g_items[0].y = 100; g_items[0].timer = 0.5f; g_items[0].frameTimer = 9;
    Rng r = SaveRng();
    float vy = 1 + RandFloat(1.0f, 6.0f);
    RestoreRng(r);
    UpdateItems();
    CHECK_EQ_INT(g_items[0].type, ITEM_MONEY_BLUE);
    CHECK_NEAR(g_items[0].vy, vy, 1e-5);
}

TEST(Play_UpdateItems_down_pulls_gems_toward_the_ship)
{
    StartPlay();
    PL0.x = 300;   // ship centre 314
    g_items[0].alive = 1; g_items[0].type = ITEM_GEM;
    g_items[0].x = 200; g_items[0].y = 100; g_items[0].vy = 1; g_items[0].frameTimer = 9;
    g_items[1].alive = 1; g_items[1].type = ITEM_GEM;
    g_items[1].x = 400; g_items[1].y = 100; g_items[1].vy = 1; g_items[1].frameTimer = 9;
    UpdateItems();
    CHECK_NEAR(g_items[0].x, 200, 1e-6);
    // a gem moves by its vy twice a frame
    CHECK_NEAR(g_items[0].y, 102, 1e-6);
    FakePressKey((enum EKeyboardLayout)g_cfg.down[0]);
    UpdateItems();
    CHECK_NEAR(g_items[0].x, 201, 1e-6);
    CHECK_NEAR(g_items[1].x, 399, 1e-6);
}

TEST(Play_UpdateItems_during_hyperspace_items_scroll)
{
    StartPlay();
    PL0.hyperspaceFade = 10;
    PL0.scrollSpeedY = 7;
    g_items[0].alive = 1; g_items[0].type = ITEM_SHIELD;
    g_items[0].x = 100; g_items[0].y = 100; g_items[0].vy = 2; g_items[0].vx = 3;
    UpdateItems();
    CHECK_NEAR(g_items[0].y, 107, 1e-6);
    CHECK_NEAR(g_items[0].x, 100, 1e-6);
}

TEST(Play_UpdateItems_tally_money_flies_to_the_counter)
{
    StartPlay();
    g_targetX = 700;
    g_targetY = 20;
    g_items[0].alive = 1; g_items[0].type = ITEM_MONEY_SMALL_TALLY;
    g_items[0].x = 300; g_items[0].y = 420; g_items[0].vx = 20; g_items[0].frameTimer = 9;
    UpdateItems();
    CHECK_NEAR(g_items[0].x, 300 + 400 / 20.0, 1e-3);
    CHECK_NEAR(g_items[0].y, 420 - 400 / 20.0, 1e-3);
    CHECK_EQ_INT(g_items[0].alive, 1);
    g_items[0].x = 680; g_items[0].y = 45;
    UpdateItems();
    CHECK_EQ_INT(g_items[0].alive, 0);
}

TEST(Play_UpdateStarItems_wraps_stars)
{
    StartPlay();
    g_items[0].alive = 1; g_items[0].type = ITEM_STAR;
    g_items[0].x = 890; g_items[0].y = 50; g_items[0].vx = 5;
    g_frameDt = 2;
    UpdateStarItems();
    CHECK_NEAR(g_items[0].x, 900, 1e-6);
    UpdateStarItems();
    CHECK_NEAR(g_items[0].x, -100, 1e-6);
    CHECK(g_items[0].vx >= 1.5f && g_items[0].vx < 8.0f);
    CHECK(g_items[0].y >= 0 && g_items[0].y < 600);
}
