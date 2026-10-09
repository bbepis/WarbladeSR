// Tests for src/game/input.c: each player's key bindings drive their controls (and only
// theirs), devices other than the keyboard ignore the keys, the autoplay AI's choices, and
// the auto-targeting finders it uses.
#include "support.h"

#define PL0 g_save.players[0]
#define PL1 g_save.players[1]

static void StartPlay(void)
{
    BootGame();
    SeedRand(55);
    g_gameMode = MODE_SINGLE;
    g_playerUpdateFn = UpdatePlayer;
    g_autoplay = 0;
    g_autoplayCanFire = 0;
    g_curPlayer = 0;
    PL0.ship = PL1.ship = 0;
    InitPlayer(0);
    InitPlayer(1);
    PL0.inputDevice = PL1.inputDevice = DEVICE_KEYBOARD;
    g_cfg.device[0] = g_cfg.device[1] = DEVICE_KEYBOARD;
    PL0.x = 300;
    PL0.y = 550;
    g_state = STATE_PLAYING;
    g_time = 500000;
    g_rankMsgActive = 0;
    memset(g_items, 0, sizeof g_items);
    memset(g_enemies, 0, sizeof g_enemies);
    memset(g_fallingGems, 0, sizeof g_fallingGems[0] * MAX_FALLING_GEMS);
    memset(g_bonusMeteors, 0, sizeof g_bonusMeteors);
    FakeReleaseAllKeys();
}

typedef struct { unsigned x, y, z, w, t; } Rng;
static Rng SaveRng(void) { return (Rng){g_rngX, g_rngY, g_rngZ, g_rngW, g_rngT}; }
static void RestoreRng(Rng r) { g_rngX = r.x; g_rngY = r.y; g_rngZ = r.z; g_rngW = r.w; g_rngT = r.t; }

enum { A_LEFT, A_RIGHT, A_UP, A_DOWN, A_FIRE, A_ROCKET, A_PAUSE, A_PROFILE, A_COUNT };

static int Action(int a, int p)
{
    switch (a) {
    case A_LEFT: return InputLeft(p);
    case A_RIGHT: return InputRight(p);
    case A_UP: return InputUp(p);
    case A_DOWN: return InputDown(p);
    case A_FIRE: return InputFire(p);
    case A_ROCKET: return InputRocket(p);
    case A_PAUSE: return InputPause(p);
    default: return InputProfile(p);
    }
}

static int *Binding(int a)
{
    switch (a) {
    case A_LEFT: return g_cfg.left;
    case A_RIGHT: return g_cfg.right;
    case A_UP: return g_cfg.up;
    case A_DOWN: return g_cfg.down;
    case A_FIRE: return g_cfg.fire;
    case A_ROCKET: return g_cfg.rocket;
    case A_PAUSE: return g_cfg.pause;
    default: return g_cfg.profile;
    }
}

static void Bind(int p, const int keys[A_COUNT])
{
    for (int a = 0; a < A_COUNT; a++)
        Binding(a)[p] = keys[a];
}

// ---------------------------------------------------------------- keyboard bindings

TEST(Play_Input_each_action_reads_only_its_own_key)
{
    StartPlay();
    const int p0[A_COUNT] = {K_VK_J, K_VK_I, K_VK_K, K_VK_M, K_VK_SPACE, K_VK_N, K_VK_P, K_VK_TAB};
    const int p1[A_COUNT] = {K_VK_A, K_VK_D, K_VK_W, K_VK_S, K_VK_Q, K_VK_E, K_VK_F1, K_VK_F2};
    Bind(0, p0);
    Bind(1, p1);
    for (int p = 0; p < 2; p++) {
        const int *keys = p ? p1 : p0;
        for (int a = 0; a < A_COUNT; a++) {
            FakeReleaseAllKeys();
            FakePressKey((enum EKeyboardLayout)keys[a]);
            for (int b = 0; b < A_COUNT; b++) {
                CHECK_MSG(Action(b, p) == (a == b), "player %d key of action %d -> action %d", p, a, b);
                CHECK_MSG(Action(b, 1 - p) == 0, "player %d key of action %d -> other player's %d", p, a, b);
            }
        }
    }
    FakeReleaseAllKeys();
    for (int a = 0; a < A_COUNT; a++)
        CHECK_EQ_INT(Action(a, 0), 0);
}

TEST(Play_Input_default_bindings)
{
    StartPlay();
    CHECK_EQ_INT(g_cfg.left[0], K_VK_LEFT);
    CHECK_EQ_INT(g_cfg.right[0], K_VK_RIGHT);
    CHECK_EQ_INT(g_cfg.up[0], K_VK_UP);
    CHECK_EQ_INT(g_cfg.down[0], K_VK_DOWN);
    CHECK_EQ_INT(g_cfg.fire[0], K_VK_L_CONTROL);
    CHECK_EQ_INT(g_cfg.rocket[0], K_VK_L_SHIFT);
    CHECK_EQ_INT(g_cfg.pause[0], K_VK_P);
    CHECK_EQ_INT(g_cfg.profile[0], K_VK_TAB);
    // player 2: the numeric keypad and the right-hand modifiers
    CHECK_EQ_INT(g_cfg.left[1], K_VK_NUM4);
    CHECK_EQ_INT(g_cfg.right[1], K_VK_NUM6);
    CHECK_EQ_INT(g_cfg.up[1], K_VK_NUM8);
    CHECK_EQ_INT(g_cfg.down[1], K_VK_NUM2);
    CHECK_EQ_INT(g_cfg.fire[1], K_VK_R_CONTROL);
    CHECK_EQ_INT(g_cfg.rocket[1], K_VK_R_SHIFT);
    FakePressKey(K_VK_LEFT);
    CHECK_EQ_INT(InputLeft(0), 1);
    FakeReleaseKey(K_VK_LEFT);
    FakePressKey(K_VK_RIGHT);
    CHECK_EQ_INT(InputRight(0), 1);
    CHECK_EQ_INT(InputLeft(0), 0);
}

TEST(Play_InputLeft_keyboard_moves_at_full_speed)
{
    StartPlay();
    g_joystickSpeedMul = 0.25f;
    FakePressKey((enum EKeyboardLayout)g_cfg.left[0]);
    InputLeft(0);
    CHECK_NEAR(g_joystickSpeedMul, 1.0, 1e-6);
}

TEST(Play_Input_a_joystick_player_ignores_the_keyboard)
{
    StartPlay();
    PL0.inputDevice = DEVICE_JOYSTICK1;
    PL1.inputDevice = DEVICE_JOYSTICK2;
    for (int a = A_FIRE; a < A_COUNT; a++) {
        for (int p = 0; p < 2; p++) {
            FakeReleaseAllKeys();
            FakePressKey((enum EKeyboardLayout)Binding(a)[p]);
            CHECK_EQ_INT(Action(a, p), 0);
        }
    }
}

TEST(Play_Input_an_unknown_device_reads_nothing)
{
    StartPlay();
    PL0.inputDevice = 7;
    for (int a = 0; a < A_COUNT; a++) {
        FakeReleaseAllKeys();
        FakePressKey((enum EKeyboardLayout)Binding(a)[0]);
        CHECK_EQ_INT(Action(a, 0), 0);
    }
}

TEST(Play_InputMenuFire_uses_the_configured_device)
{
    StartPlay();
    FakePressKey((enum EKeyboardLayout)g_cfg.fire[1]);
    CHECK_EQ_INT(InputMenuFire(1), 1);
    CHECK_EQ_INT(InputMenuFire(0), 0);
    PL1.inputDevice = DEVICE_JOYSTICK1;   // the in-game device doesn't matter here
    CHECK_EQ_INT(InputMenuFire(1), 1);
    g_cfg.device[1] = DEVICE_JOYSTICK2;
    CHECK_EQ_INT(InputMenuFire(1), 0);
}

TEST(Play_GetFlagMask_has_no_buttons_without_a_joystick)
{
    StartPlay();
    CHECK_EQ_INT(GetFlagMask(0), 0);
    CHECK_EQ_INT(GetFlagMask(1), 0);
}

// ---------------------------------------------------------------- gamepads and touch controls

TEST(Play_Input_each_pad_bit_reads_only_its_own_action_and_player)
{
    StartPlay();
    const unsigned bits[A_COUNT] = {PAD_LEFT, PAD_RIGHT, PAD_UP, PAD_DOWN,
                                   PAD_FIRE, PAD_ROCKET, PAD_PAUSE, PAD_PROFILE};
    for (int p = 0; p < 2; p++) {
        for (int a = 0; a < A_COUNT; a++) {
            g_fake.padBits[0] = g_fake.padBits[1] = 0;
            g_fake.padBits[p] = bits[a];
            for (int b = 0; b < A_COUNT; b++) {
                CHECK_EQ_INT(Action(b, p), a == b);
                CHECK_EQ_INT(Action(b, 1 - p), 0);
            }
        }
    }
}

TEST(Play_Input_touch_and_keyboard_combine_without_controlling_player_two)
{
    StartPlay();
    SysSetVirtualPad(PAD_LEFT | PAD_FIRE);
    FakePressKey((enum EKeyboardLayout)g_cfg.rocket[0]);
    CHECK_EQ_INT(InputLeft(0), 1);
    CHECK_EQ_INT(InputFire(0), 1);
    CHECK_EQ_INT(InputRocket(0), 1);
    CHECK_EQ_INT(InputLeft(1), 0);
    CHECK_EQ_INT(InputFire(1), 0);
    CHECK_EQ_INT(InputMenuFire(0), 1);
    SysSetVirtualPad(0);
    CHECK_EQ_INT(InputLeft(0), 0);
    CHECK_EQ_INT(InputFire(0), 0);
    CHECK_EQ_INT(InputRocket(0), 1);
}

TEST(Play_Input_legacy_and_unknown_devices_ignore_pad_bits)
{
    StartPlay();
    const int devices[] = {DEVICE_JOYSTICK1, DEVICE_JOYSTICK2, 7};
    g_joyCenter = 0x7fff;
    g_joyDead = 1000;
    g_fake.padBits[0] = ~0u;
    SysSetVirtualPad(~0u);
    for (unsigned i = 0; i < sizeof devices / sizeof devices[0]; i++) {
        PL0.inputDevice = devices[i];
        g_cfg.device[0] = devices[i];
        for (int a = 0; a < A_COUNT; a++)
            CHECK_EQ_INT(Action(a, 0), 0);
        CHECK_EQ_INT(InputMenuFire(0), 0);
    }
}

// ---------------------------------------------------------------- autoplay

TEST(Play_InputFire_in_autoplay_shows_the_banner)
{
    StartPlay();
    g_autoplay = 1;
    g_msgColor = 0;
    InputFire(0);
    CHECK_STR(g_alertMsg, "A U T O   P L A Y   G A M E");
    CHECK_EQ_INT(g_msgColor, 2);
    CHECK_EQ_INT(g_msgTimer, g_time + 5000);
}

TEST(Play_InputFire_in_autoplay_shoots_at_a_target)
{
    StartPlay();
    g_autoplay = 1;
    g_enemies[0][3].active = 1;
    g_enemies[0][3].type = ENEMY_HOVER;
    g_enemies[0][3].x = 400;
    g_enemies[0][3].y = 200;
    int fired = 0;
    for (int round = 0; round < 200; round++) {
        Rng r = SaveRng();
        // no item: target item y -1 < 200, so 70% to fire
        int expect = RandRange(0, 100) < 70;
        RestoreRng(r);
        int got = InputFire(0);
        CHECK_EQ_INT(got, expect);
        fired += got;
    }
    CHECK(fired > 100 && fired < 180);
    // no target at all: never fires
    g_enemies[0][3].active = 0;
    for (int round = 0; round < 50; round++)
        CHECK_EQ_INT(InputFire(0), 0);
}

TEST(Play_InputFire_in_autoplay_always_fires_in_the_memory_station)
{
    StartPlay();
    g_autoplay = 1;
    g_state = STATE_MEMORY_STATION;
    for (int round = 0; round < 50; round++)
        CHECK_EQ_INT(InputFire(0), 1);
}

TEST(Play_InputRocket_in_autoplay_fires_4_percent_of_the_time)
{
    StartPlay();
    g_autoplay = 1;
    FakePressKey((enum EKeyboardLayout)g_cfg.rocket[0]);
    int fired = 0;
    for (int round = 0; round < 500; round++) {
        Rng r = SaveRng();
        int expect = RandRange(0, 100) < 4;
        RestoreRng(r);
        int got = InputRocket(0);
        CHECK_EQ_INT(got, expect);
        fired += got;
    }
    CHECK(fired > 0 && fired < 60);
}

TEST(Play_InputDown_in_autoplay_is_a_coin_flip)
{
    StartPlay();
    g_autoplay = 1;
    int down = 0;
    for (int round = 0; round < 200; round++) {
        Rng r = SaveRng();
        int expect = RandRange(0, 100) < 50;
        RestoreRng(r);
        int got = InputDown(0);
        CHECK_EQ_INT(got, expect);
        down += got;
    }
    CHECK(down > 50 && down < 150);
}

TEST(Play_InputUp_in_autoplay_holds_for_the_ai_timer)
{
    StartPlay();
    g_autoplay = 1;
    g_aiTimer = 2;
    CHECK_EQ_INT(InputUp(0), 1);
    CHECK_EQ_INT(g_aiTimer, 1);
    CHECK_EQ_INT(InputUp(0), 1);
    CHECK_EQ_INT(g_aiTimer, 0);
    for (int round = 0; round < 50 && g_aiTimer == 0; round++) {
        Rng r = SaveRng();
        int roll = RandRange(0, 100);
        RestoreRng(r);
        CHECK_EQ_INT(InputUp(0), roll < 2);
    }
}

TEST(Play_InputLeft_and_Right_in_autoplay_chase_the_lowest_enemy)
{
    StartPlay();
    g_autoplay = 1;
    g_enemies[0][3].active = 1;
    g_enemies[0][3].type = ENEMY_HOVER;
    g_enemies[0][3].x = 200;
    g_enemies[0][3].y = 300;
    g_enemies[0][4].active = 1;
    g_enemies[0][4].type = ENEMY_HOVER;
    g_enemies[0][4].x = 500;
    g_enemies[0][4].y = 100;
    CHECK_EQ_INT(InputLeft(0), 1);
    CHECK_EQ_INT(InputRight(0), 0);
    g_enemies[0][4].y = 400;
    CHECK_EQ_INT(InputLeft(0), 0);
    CHECK_EQ_INT(InputRight(0), 1);
    g_enemies[0][3].active = g_enemies[0][4].active = 0;
    CHECK_EQ_INT(InputLeft(0), 0);
    CHECK_EQ_INT(InputRight(0), 0);
}

TEST(Play_InputLeft_in_autoplay_prefers_a_lower_item)
{
    StartPlay();
    g_autoplay = 1;
    g_enemies[0][3].active = 1;
    g_enemies[0][3].type = ENEMY_HOVER;
    g_enemies[0][3].x = 500;
    g_enemies[0][3].y = 100;
    g_items[0].alive = g_items[0].active = 1;
    g_items[0].x = 150;
    g_items[0].y = 300;
    CHECK_EQ_INT(InputLeft(0), 1);
    CHECK_EQ_INT(InputRight(0), 1);   // right also sees the enemy to the right
    g_items[0].y = 50;   // higher than the enemy: chase the enemy
    CHECK_EQ_INT(InputLeft(0), 0);
}

TEST(Play_InputLeft_in_the_gem_drop_chases_the_lowest_gem)
{
    StartPlay();
    g_autoplay = 1;
    g_state = STATE_GEM_DROP;
    g_fallingGems[0].active = 1;
    g_fallingGems[0].x = 100; g_fallingGems[0].w = 20; g_fallingGems[0].y = 200;
    CHECK_EQ_INT(InputLeft(0), 1);
    CHECK_EQ_INT(InputRight(0), 0);
    g_fallingGems[0].x = 400;
    CHECK_EQ_INT(InputLeft(0), 0);
    CHECK_EQ_INT(InputRight(0), 1);
    g_fallingGems[0].active = 0;
    CHECK_EQ_INT(InputLeft(0), 0);
    CHECK_EQ_INT(InputRight(0), 0);
}

TEST(Play_InputLeft_in_the_race_holds_for_the_ai_timer)
{
    StartPlay();
    g_autoplay = 1;
    g_state = STATE_BONUS_RACE;
    g_aiLeft = 2;
    CHECK_EQ_INT(InputLeft(0), 1);
    CHECK_EQ_INT(g_aiLeft, 1);
    g_aiRight = 1;
    CHECK_EQ_INT(InputRight(0), 1);
    CHECK_EQ_INT(g_aiRight, 0);
}

TEST(Play_InputMenuFire_in_autoplay_needs_permission)
{
    StartPlay();
    g_autoplay = 1;
    for (int round = 0; round < 30; round++)
        CHECK_EQ_INT(InputMenuFire(0), 0);
    g_autoplayCanFire = 1;
    int fired = 0;
    for (int round = 0; round < 100; round++) {
        Rng r = SaveRng();
        int expect = RandRange(0, 100) < 90;
        RestoreRng(r);
        int got = InputMenuFire(0);
        CHECK_EQ_INT(got, expect);
        fired += got;
    }
    CHECK(fired > 70 && fired < 100);
}

// ---------------------------------------------------------------- target finders

TEST(Play_FindTargetItem_picks_the_lowest_item_above_the_ship)
{
    StartPlay();
    g_targetItemY = 123;
    FindTargetItem();
    CHECK_NEAR(g_targetItemX, -1, 1e-6);
    CHECK_NEAR(g_targetItemY, -1, 1e-6);
    // candidates (ship y 550: items must be above 582)
    g_items[0] = (Bonus){.alive = 1, .active = 1, .x = 100, .y = 300};
    g_items[1] = (Bonus){.alive = 1, .active = 1, .x = 200, .y = 400};
    g_items[2] = (Bonus){.alive = 1, .active = 0, .x = 300, .y = 500};   // inactive
    g_items[3] = (Bonus){.alive = 0, .active = 1, .x = 300, .y = 500};   // dead
    g_items[4] = (Bonus){.alive = 1, .active = 1, .x = 70, .y = 500};    // at the left margin
    g_items[5] = (Bonus){.alive = 1, .active = 1, .x = 730, .y = 500};   // at the right margin
    g_items[6] = (Bonus){.alive = 1, .active = 1, .x = 300, .y = 582};   // below the ship
    FindTargetItem();
    CHECK_NEAR(g_targetItemX, 200, 1e-6);
    CHECK_NEAR(g_targetItemY, 400, 1e-6);
    g_items[7] = (Bonus){.alive = 1, .active = 1, .x = 71, .y = 581};
    // Later entries are higher on screen; selection must not depend on slot order.
    g_items[8] = (Bonus){.alive = 1, .active = 1, .x = 250, .y = 200};
    FindTargetItem();
    CHECK_NEAR(g_targetItemX, 71, 1e-6);
    CHECK_NEAR(g_targetItemY, 581, 1e-6);
}

TEST(Play_FindTargetObj_picks_the_lowest_enemy)
{
    StartPlay();
    g_targetObjY = 123;
    FindTargetObj();
    CHECK_NEAR(g_targetObjX, -1, 1e-6);
    CHECK_NEAR(g_targetObjY, 123, 1e-6);   // left alone
    Enemy *e = g_enemies[0];
    e[0].active = 1; e[0].type = ENEMY_HOVER; e[0].x = 100; e[0].y = 100;
    e[1].active = 1; e[1].type = ENEMY_DIVING; e[1].x = 200; e[1].y = 300;
    e[2].active = 1; e[2].type = ENEMY_CAPTURED; e[2].x = 300; e[2].y = 400;
    e[3].active = 2; e[3].type = ENEMY_HOVER; e[3].x = 300; e[3].y = 400;
    e[4].active = 1; e[4].type = ENEMY_HOVER; e[4].x = 300; e[4].y = 500;   // too low
    e[5].active = 1; e[5].type = ENEMY_HOVER; e[5].x = 70; e[5].y = 400;
    e[6].active = 1; e[6].type = ENEMY_HOVER; e[6].x = 730; e[6].y = 400;
    g_enemies[1][7].active = 1; g_enemies[1][7].x = 300; g_enemies[1][7].y = 450;  // player 2's
    FindTargetObj();
    CHECK_NEAR(g_targetObjX, 200, 1e-6);
    CHECK_NEAR(g_targetObjY, 300, 1e-6);
    e[8].active = 1; e[8].type = ENEMY_HOVER; e[8].x = 729; e[8].y = 499;
    FindTargetObj();
    CHECK_NEAR(g_targetObjX, 729, 1e-6);
    g_curPlayer = 1;
    FindTargetObj();
    CHECK_NEAR(g_targetObjX, 300, 1e-6);
    CHECK_NEAR(g_targetObjY, 450, 1e-6);
}

TEST(Play_FindTargetGem_picks_the_lowest_gem_centre)
{
    StartPlay();
    FindTargetGem();
    CHECK_NEAR(g_target847X, -1, 1e-6);
    FallingSprite *g = g_fallingGems;
    g[0].active = 1; g[0].x = 100; g[0].w = 20; g[0].y = 100;
    g[1].active = 1; g[1].x = 200; g[1].w = 21; g[1].y = 300;
    g[2].active = 1; g[2].x = 300; g[2].w = 20; g[2].y = 550;   // too low
    g[3].active = 1; g[3].x = 50; g[3].w = 40; g[3].y = 400;    // centre 70: at the margin
    g[4].active = 0; g[4].x = 300; g[4].w = 20; g[4].y = 450;
    g[9].active = 1; g[9].x = 700; g[9].w = 60; g[9].y = 450;   // centre 730
    FindTargetGem();
    CHECK_NEAR(g_target847X, 210, 1e-6);   // 200 + (21 >> 1)
    g[5].active = 1; g[5].x = 51; g[5].w = 40; g[5].y = 549;
    FindTargetGem();
    CHECK_NEAR(g_target847X, 71, 1e-6);
    g[5].active = 0;
    g[MAX_FALLING_GEMS - 1] = (FallingSprite){.active = 1, .x = 400, .w = 20, .y = 549};
    FindTargetGem();
    CHECK_NEAR(g_target847X, 410, 1e-6);
}

TEST(Play_FindTargetMeteor_picks_the_lowest_meteor_bottom)
{
    StartPlay();
    FindTargetMeteor();
    CHECK_NEAR(g_targetB49X, -1, 1e-6);
    FallingHazard *m = g_bonusMeteors;
    m[0].active = 1; m[0].x = 100; m[0].w = 30; m[0].y = 100; m[0].h = 40;
    m[1].active = 1; m[1].x = 200; m[1].w = 31; m[1].y = 250; m[1].h = 50;
    m[2].active = 1; m[2].x = 300; m[2].w = 20; m[2].y = 500; m[2].h = 50;   // bottom 550
    m[3].active = 1; m[3].x = 50; m[3].w = 40; m[3].y = 400; m[3].h = 10;    // centre 70
    m[4].active = 0; m[4].x = 300; m[4].w = 20; m[4].y = 450; m[4].h = 10;
    m[29].active = 1; m[29].x = 700; m[29].w = 60; m[29].y = 450; m[29].h = 10;   // centre 730
    FindTargetMeteor();
    CHECK_NEAR(g_targetB49X, 215.5, 1e-6);
    CHECK_NEAR(g_targetB49Y, 300, 1e-6);
}

TEST(Play_Input_legacy_axes_return_values_and_respect_player_and_deadzone)
{
    StartPlay();
    g_joyCenter = 0x7fff;
    g_joyDead = 1000;
    PL0.inputDevice = DEVICE_JOYSTICK1;
    PL1.inputDevice = DEVICE_JOYSTICK2;
    g_fake.joyX[0] = 1234;
    g_fake.joyY[1] = 60000;
    CHECK_EQ_INT(GetJoyX(0), 1234);
    CHECK_EQ_INT(GetJoyY(1), 60000);
    CHECK_EQ_INT(InputLeft(0), 1);
    CHECK_NEAR(g_joystickSpeedMul, (32767.0 - 1234) / 32767.0, 1e-6);
    CHECK_EQ_INT(InputRight(0), 0);
    CHECK_EQ_INT(InputLeft(1), 0);
    CHECK_EQ_INT(InputDown(1), 1);
    CHECK_EQ_INT(InputUp(1), 0);
    CHECK_EQ_INT(InputDown(0), 0);
    g_fake.joyX[0] = g_joyCenter - g_joyDead;
    g_fake.joyY[1] = g_joyCenter + g_joyDead;
    CHECK_EQ_INT(InputLeft(0), 0);
    CHECK_EQ_INT(InputDown(1), 0);
    g_fake.joyX[1] = 60000;
    g_fake.joyY[0] = 1234;
    CHECK_EQ_INT(InputRight(1), 1);
    CHECK_NEAR(g_joystickSpeedMul, (60000.0 - 32767) / 32767.0, 1e-6);
    CHECK_EQ_INT(InputUp(0), 1);
}
