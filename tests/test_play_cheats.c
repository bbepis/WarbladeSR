// Tests for src/game/cheats.c: typing GALAGA during play toggles the cheats (letters within
// 2 s of each other), then the number keys 1-9 apply one cheat each to the current player.
#include "support.h"

#define PL0 g_save.players[0]

enum { LIVES0 = 38, MAX_LIVES = 46, MAX_ARMOUR = 133 };   // ship 0

static void StartPlay(void)
{
    BootGame();
    SeedRand(77);
    g_gameMode = MODE_SINGLE;
    g_playerUpdateFn = UpdatePlayer;
    g_autoplay = 0;
    g_curPlayer = 0;
    PL0.ship = 0;
    InitPlayer(0);
    g_state = STATE_PLAYING;
    g_time = 500000;
    g_frameDt = 1.0f;
    g_cheatsOn = false;
    g_cheatGodMode = false;
    memset(g_items, 0, sizeof g_items);
    memset(g_enemies, 0, sizeof g_enemies);
    for (int i = 0; i < MAX_WINDOWS; i++)
        g_windows[i].active = 0;
    FakeReleaseAllKeys();
}

static int Plays(AudioHandle sample)
{
    return FakePlayCount(FakeSampleName(sample));
}

// Presses and releases `key` (one CheatHotkeys poll each), then lets `gapMs` pass.
static void Tap(int key, unsigned gapMs)
{
    FakePressKey((enum EKeyboardLayout)key);
    CheatHotkeys();
    FakeReleaseKey((enum EKeyboardLayout)key);
    CheatHotkeys();
    g_time += gapMs;
}

static void Type(const char *letters)
{
    for (const char *c = letters; *c; c++)
        Tap(K_VK_A + (*c - 'A'), 100);
}

static void Digit(int d)
{
    Tap(K_VK_0 + d, 50);
}

static void EnableCheats(void)
{
    Type("GALAGA");
    CHECK(g_cheatsOn);
}

// ---------------------------------------------------------------- the code

TEST(Play_Cheats_typing_GALAGA_toggles_them)
{
    StartPlay();
    Type("GALAG");
    CHECK(!g_cheatsOn);
    unsigned t = g_time;
    Type("A");
    CHECK(g_cheatsOn);
    CHECK_STR(g_optionMsg, "CHEATS ENABLED : KEYS 1-9");
    CHECK_EQ_INT(g_msgTime, t + 2000);
    CHECK_EQ_INT(Plays(g_sfxFanfare), 1);
    g_cheatGodMode = true;
    Type("GALAGA");
    CHECK(!g_cheatsOn);
    CHECK(!g_cheatGodMode);
    CHECK_STR(g_optionMsg, "CHEATS DISABLED");
    CHECK_EQ_INT(Plays(g_sfxFanfare), 2);
}

TEST(Play_Cheats_letters_must_follow_within_2_seconds)
{
    StartPlay();
    Type("GAL");
    g_time += 2001 - 100;
    Type("AGA");
    CHECK(!g_cheatsOn);
    Type("GAL");
    g_time += 2000 - 100;
    Type("AGA");
    CHECK(g_cheatsOn);
}

TEST(Play_Cheats_a_wrong_letter_restarts_the_code)
{
    StartPlay();
    Type("GALXAGA");
    CHECK(!g_cheatsOn);
    Type("GALAGB");
    CHECK(!g_cheatsOn);
    Type("A");
    CHECK(!g_cheatsOn);
    // a G in the wrong place starts a new attempt right away
    Type("GAGALAGA");
    CHECK(g_cheatsOn);
}

TEST(Play_Cheats_a_held_key_counts_once)
{
    StartPlay();
    Type("G");
    FakePressKey(K_VK_A);   // held for three polls: still one A
    CheatHotkeys();
    CheatHotkeys();
    CheatHotkeys();
    FakeReleaseKey(K_VK_A);
    CheatHotkeys();
    Type("LAGA");
    CHECK(g_cheatsOn);
}

TEST(Play_Cheats_a_held_key_does_not_hide_the_next_one)
{
    StartPlay();
    Type("GALA");
    FakePressKey(K_VK_G);
    CheatHotkeys();
    FakePressKey(K_VK_A);   // G still held
    CheatHotkeys();
    CHECK(g_cheatsOn);
}

TEST(Play_Cheats_not_in_the_demo_autoplay_or_with_a_window_open)
{
    StartPlay();
    g_playerUpdateFn = StateDemo;
    Type("GALAGA");
    CHECK(!g_cheatsOn);
    g_playerUpdateFn = UpdatePlayer;
    g_autoplay = 1;
    Type("GALAGA");
    CHECK(!g_cheatsOn);
    g_autoplay = 0;
    g_windows[9].active = 1;
    Type("GALAGA");
    CHECK(!g_cheatsOn);
    g_windows[9].active = 0;
    Type("GALAGA");
    CHECK(g_cheatsOn);
}

TEST(Play_Cheats_Hotkeys_watches_for_the_code)
{
    StartPlay();
    for (const char *c = "GALAGA"; *c; c++) {
        FakePressKey((enum EKeyboardLayout)(K_VK_A + (*c - 'A')));
        Hotkeys();
        FakeReleaseAllKeys();
        Hotkeys();
        g_time += 100;
    }
    CHECK(g_cheatsOn);
}

// ---------------------------------------------------------------- the number keys

TEST(Play_Cheats_digits_do_nothing_while_cheats_are_off)
{
    StartPlay();
    for (int d = 1; d <= 9; d++)
        Digit(d);
    CHECK_EQ_INT(PL0.lives, LIVES0);
    CHECK_EQ_INT(PL0.money, 0);
    CHECK_EQ_INT(PL0.weapon, 0);
    CHECK_EQ_INT(PL0.levelFinished, 0);
    CHECK(!g_cheatGodMode);
    CHECK_EQ_INT(Plays(g_sfxTast), 0);
}

TEST(Play_Cheats_1_adds_a_life_up_to_the_max)
{
    StartPlay();
    EnableCheats();
    Digit(1);
    CHECK_EQ_INT(PL0.lives, LIVES0 + 4);
    CHECK_STR(g_optionMsg, "CHEAT : EXTRA LIFE");
    CHECK_EQ_INT(Plays(g_sfxTast), 1);
    PL0.lives = MAX_LIVES - 2;
    Digit(1);
    CHECK_EQ_INT(PL0.lives, MAX_LIVES);
    Digit(1);
    CHECK_EQ_INT(PL0.lives, MAX_LIVES);
    CHECK_STR(g_optionMsg, "CHEAT : LIVES ARE FULL");
}

TEST(Play_Cheats_2_adds_1000_money_up_to_the_wallet)
{
    StartPlay();
    EnableCheats();
    g_moneyMax = 0;
    Digit(2);
    CHECK_EQ_INT(PL0.money, 1000);
    CHECK_EQ_INT(g_moneyMax, 1000);
    CHECK_STR(g_optionMsg, "CHEAT : MONEY");
    PL0.money = PL0.moneyMax - 10;
    Digit(2);
    CHECK_EQ_INT(PL0.money, PL0.moneyMax);
    CHECK_EQ_INT(g_moneyMax, PL0.moneyMax);
}

TEST(Play_Cheats_3_warps_once_per_level)
{
    StartPlay();
    EnableCheats();
    Digit(3);
    CHECK_EQ_INT(PL0.levelFinished, 1);
    CHECK_EQ_INT(PL0.levelWarpPending, 1);
    CHECK_EQ_INT(PL0.enemyHpBonusRoll, 10);
    CHECK_STR(g_optionMsg, "CHEAT : SKIP LEVEL");
    Digit(3);
    CHECK_EQ_INT(PL0.enemyHpBonusRoll, 10);
    CHECK_EQ_INT(Plays(g_sfxTast), 1);
}

TEST(Play_Cheats_4_gives_war_i_plasma_with_50_bullets)
{
    StartPlay();
    EnableCheats();
    Digit(4);
    CHECK_EQ_INT(PL0.weapon, WEAPON_WAR_PLASMA);
    CHECK_EQ_INT(PL0.bullets, 50);
    CHECK_STR(g_optionMsg, "CHEAT : WAR.I.PLASMA");
}

TEST(Play_Cheats_5_fills_the_armour)
{
    StartPlay();
    EnableCheats();
    Digit(5);
    CHECK_EQ_INT(PL0.armour, MAX_ARMOUR);
    CHECK_STR(g_optionMsg, "CHEAT : FULL ARMOUR");
}

TEST(Play_Cheats_6_gives_rockets_and_super_autofire)
{
    StartPlay();
    EnableCheats();
    Digit(6);
    CHECK_EQ_INT(PL0.rockets, 50);
    CHECK_EQ_INT(PL0.superAuto, 1);
    CHECK_EQ_INT(PL0.autofire, 1);
    CHECK_EQ_INT(PL0.autofireInterval, 0x19);
    CHECK_STR(g_optionMsg, "CHEAT : ROCKETS + SUPER AUTOFIRE");
}

TEST(Play_Cheats_7_gives_a_shield)
{
    StartPlay();
    EnableCheats();
    PL0.buffDuration = 22;
    Digit(7);
    CHECK_EQ_INT(PL0.shieldTimer, g_time - 50 + 22000);
    CHECK_STR(g_optionMsg, "CHEAT : SHIELD");
}

TEST(Play_Cheats_8_is_a_smart_bomb)
{
    StartPlay();
    EnableCheats();
    g_enemies[0][4].active = 1;
    g_enemies[0][4].type = ENEMY_HOVER;
    Digit(8);
    CHECK_EQ_INT(PL0.bombPickups, 1);
    CHECK_EQ_INT(g_enemies[0][4].active, 0);
    CHECK_STR(g_optionMsg, "CHEAT : SMART BOMB");
}

TEST(Play_Cheats_9_toggles_god_mode_even_while_dead)
{
    StartPlay();
    EnableCheats();
    Digit(9);
    CHECK(g_cheatGodMode);
    CHECK_STR(g_optionMsg, "CHEAT : GOD MODE ON");
    PL0.dead = 1;
    g_state = STATE_GET_READY;
    Digit(9);
    CHECK(!g_cheatGodMode);
    CHECK_STR(g_optionMsg, "CHEAT : GOD MODE OFF");
}

TEST(Play_Cheats_other_digits_need_normal_play_and_a_live_ship)
{
    StartPlay();
    EnableCheats();
    PL0.dead = 1;
    Digit(1);
    CHECK_EQ_INT(PL0.lives, LIVES0);
    PL0.dead = 0;
    g_state = STATE_GET_READY;
    Digit(1);
    CHECK_EQ_INT(PL0.lives, LIVES0);
    g_state = STATE_PLAYING;
    Digit(1);
    CHECK_EQ_INT(PL0.lives, LIVES0 + 4);
}

TEST(Play_Cheats_numpad_digits_work_too_once_per_press)
{
    StartPlay();
    EnableCheats();
    FakePressKey(K_VK_NUM2);
    CheatHotkeys();
    CheatHotkeys();
    CheatHotkeys();
    CHECK_EQ_INT(PL0.money, 1000);
    FakeReleaseKey(K_VK_NUM2);
    CheatHotkeys();
    Tap(K_VK_NUM2, 10);
    CHECK_EQ_INT(PL0.money, 2000);
}
