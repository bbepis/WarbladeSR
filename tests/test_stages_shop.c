// Tests for src/game/stages/shop.c: the between-levels shop (buying items, the caps, the
// money checks, the selection keys, leaving), plus its small helpers.
#include "support.h"

#define P0 g_save.players[0]
#define P1 g_save.players[1]

// Ship 0 (g_shipStats0): minEnergy 26, cost 4, maxEnergy 20, baseArmour 123, armourStep 5,
// maxArmourBonus 10.
enum { LIVES_CAP = 26 + 20, ARMOUR_CAP = 123 + 10 };

// Boots to the title screen, then puts player 0 in a fully slid-in single-player shop.
static void ShopOpen(int money)
{
    BootGame();
    SeedRand(1234);
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_shopCurPlayer = 0;
    P0.level = 5;
    P0.money = money;
    P0.ship = 0;
    g_state = STATE_SHOP;
    g_shopTransition = 500;
    g_shopClosing = 0;
    g_offX = 0;
    g_offY = 0;
    g_inputCooldown = 0;
    g_transitionLock = 0;
    g_shopFireEdge = 1;
    g_shopSelItem = 0;
}

static void ShopFrame(void)
{
    Shop();
}

// Selects shop row `item` (1 = EXTRA SPEED ... 21 = CLEAR SHIELDS) and presses fire once.
static void Buy(int item)
{
    g_shopSelItem = item;
    FakePressKey((enum EKeyboardLayout)g_cfg.fire[0]);
    ShopFrame();
    FakeReleaseKey((enum EKeyboardLayout)g_cfg.fire[0]);
    ShopFrame();
}

static void Tap(int key)
{
    FakePressKey((enum EKeyboardLayout)key);
    ShopFrame();
    FakeReleaseKey((enum EKeyboardLayout)key);
    ShopFrame();
}

static int Buzzes(void) { return FakePlayCount("buzzer.wav"); }
static int Buzzes2(void) { return FakePlayCount("buzzer2"); }
static int Cash(void) { return FakePlayCount("cash"); }

static float MaxSpeed(void) { return g_speedStep * g_maxSpeedMul + g_speedBase; }

// A refused purchase: the "already have it" buzzer, no money taken, no visit counted.
static void CheckRefused(int moneyBefore)
{
    CHECK_EQ_INT(P0.money, moneyBefore);
    CHECK_EQ_INT(Buzzes2(), 1);
    CHECK_EQ_INT(Cash(), 0);
    CHECK_EQ_INT(P0.shopVisits, 0);
}

// A successful purchase: price taken, a visit counted, the cash register rung.
static void CheckBought(int moneyBefore, int price)
{
    CHECK_EQ_INT(P0.money, moneyBefore - price);
    CHECK_EQ_INT(Cash(), 1);
    CHECK_EQ_INT(P0.shopVisits, 1);
    CHECK_EQ_INT(Buzzes2(), 0);
}

// ---- prices and effects, item by item ----

TEST(stages_shop_extra_speed_adds_one_step_for_50)
{
    ShopOpen(10000);
    P0.speed = g_speedBase;
    Buy(1);
    CHECK_NEAR(P0.speed, g_speedBase + g_speedStep, 1e-4);
    CheckBought(10000, 50);
}

TEST(stages_shop_extra_speed_clamps_to_the_top_speed)
{
    ShopOpen(10000);
    P0.speed = MaxSpeed() - g_speedStep / 2;
    Buy(1);
    CHECK_NEAR(P0.speed, MaxSpeed(), 1e-4);
    CheckBought(10000, 50);
}

TEST(stages_shop_extra_speed_refused_at_top_speed)
{
    ShopOpen(10000);
    P0.speed = MaxSpeed();
    Buy(1);
    CHECK_NEAR(P0.speed, MaxSpeed(), 1e-4);
    CheckRefused(10000);
}

TEST(stages_shop_extra_bullet_adds_one_for_75)
{
    ShopOpen(10000);
    P0.bullets = 8;
    Buy(2);
    CHECK_EQ_INT(P0.bullets, 9);
    CheckBought(10000, 75);
}

TEST(stages_shop_extra_bullet_refused_at_50_bullets)
{
    ShopOpen(10000);
    P0.bullets = 49;
    Buy(2);
    CHECK_EQ_INT(P0.bullets, 50);
    Buy(2);
    CHECK_EQ_INT(P0.bullets, 50);
    CHECK_EQ_INT(P0.money, 10000 - 75);
    CHECK_EQ_INT(Buzzes2(), 1);
    CHECK_EQ_INT(Cash(), 1);
    CHECK_EQ_INT(P0.shopVisits, 1);
}

TEST(stages_shop_less_speed_removes_one_step_for_150)
{
    ShopOpen(10000);
    P0.speed = g_speedBase + 2 * g_speedStep;
    Buy(4);
    CHECK_NEAR(P0.speed, g_speedBase + g_speedStep, 1e-4);
    CheckBought(10000, 150);
}

TEST(stages_shop_less_speed_refused_at_base_speed)
{
    ShopOpen(10000);
    P0.speed = g_speedBase;
    Buy(4);
    CHECK_NEAR(P0.speed, g_speedBase, 1e-4);
    CheckRefused(10000);
}

// Every weapon row: the weapon it equips and its price.
TEST(stages_shop_weapons_equip_and_cost_their_price)
{
    static const struct { int item, weapon, price; } rows[] = {
        {3, WEAPON_DOUBLE, 100},       {5, WEAPON_TRIPLE, 200},
        {6, WEAPON_QUAD, 300},         {8, WEAPON_SUPER_TRIPLE, 500},
        {10, WEAPON_PLASMA, 750},      {12, WEAPON_FIREBALLS, 990},
        {16, WEAPON_LASER, 2000},      {17, WEAPON_WAR_PLASMA, 3000},
    };
    ShopOpen(100000);
    int money = 100000;
    for (int i = 0; i < (int)(sizeof rows / sizeof rows[0]); i++) {
        P0.weapon = WEAPON_SINGLE;
        Buy(rows[i].item);
        CHECK_MSG(P0.weapon == rows[i].weapon, "item %d: weapon %d, expected %d", rows[i].item,
                  P0.weapon, rows[i].weapon);
        money -= rows[i].price;
        CHECK_MSG(P0.money == money, "item %d: money %d, expected %d", rows[i].item, P0.money,
                  money);
    }
    CHECK_EQ_INT(Cash(), 8);
    CHECK_EQ_INT(P0.shopVisits, 8);
}

TEST(stages_shop_weapon_already_equipped_is_refused)
{
    ShopOpen(10000);
    P0.weapon = WEAPON_QUAD;
    Buy(6);
    CHECK_EQ_INT(P0.weapon, WEAPON_QUAD);
    CheckRefused(10000);
}

TEST(stages_shop_auto_fire_unit_for_400)
{
    ShopOpen(10000);
    P0.autofire = 0;
    P0.superAuto = 0;
    Buy(7);
    CHECK_EQ_INT(P0.autofire, 1);
    CheckBought(10000, 400);
}

TEST(stages_shop_auto_fire_refused_when_owned)
{
    ShopOpen(10000);
    P0.autofire = 1;
    P0.superAuto = 0;
    Buy(7);
    CheckRefused(10000);
}

TEST(stages_shop_auto_fire_refused_with_super_autofire)
{
    ShopOpen(10000);
    P0.autofire = 0;
    P0.superAuto = 1;
    Buy(7);
    CHECK_EQ_INT(P0.autofire, 0);
    CheckRefused(10000);
}

TEST(stages_shop_ship_armour_adds_a_step_for_600)
{
    ShopOpen(10000);
    P0.armour = 123;
    int added = g_armourAddedCount;
    Buy(9);
    CHECK_EQ_INT(P0.armour, 128);
    CHECK_EQ_INT(g_armourAddedCount, added + 5);
    CheckBought(10000, 600);
}

TEST(stages_shop_ship_armour_clamps_to_the_ship_cap)
{
    ShopOpen(10000);
    P0.armour = ARMOUR_CAP - 2;
    Buy(9);
    CHECK_EQ_INT(P0.armour, ARMOUR_CAP);
    CheckBought(10000, 600);
}

TEST(stages_shop_ship_armour_refused_at_the_cap)
{
    ShopOpen(10000);
    P0.armour = ARMOUR_CAP;
    int added = g_armourAddedCount;
    Buy(9);
    CHECK_EQ_INT(P0.armour, ARMOUR_CAP);
    CHECK_EQ_INT(g_armourAddedCount, added);
    CheckRefused(10000);
}

TEST(stages_shop_extra_life_adds_the_ship_cost_for_800)
{
    ShopOpen(10000);
    P0.lives = 30;
    int gained = g_livesGainedCount;
    Buy(11);
    CHECK_EQ_INT(P0.lives, 34);
    CHECK_EQ_INT(g_livesGainedCount, gained + 10);
    CheckBought(10000, 800);
}

TEST(stages_shop_extra_life_clamps_to_the_hangar)
{
    ShopOpen(10000);
    P0.lives = LIVES_CAP - 1;
    Buy(11);
    CHECK_EQ_INT(P0.lives, LIVES_CAP);
    CheckBought(10000, 800);
}

TEST(stages_shop_extra_life_refused_with_a_full_hangar)
{
    ShopOpen(10000);
    P0.lives = LIVES_CAP;
    Buy(11);
    CHECK_EQ_INT(P0.lives, LIVES_CAP);
    CheckRefused(10000);
}

TEST(stages_shop_game_secret_shows_a_secret_for_1000)
{
    ShopOpen(10000);
    g_secretPicCur = 3;
    g_secretPicPrev = 4;
    Buy(13);
    CHECK_EQ_INT(g_secretShown, 1);
    CHECK(g_gfxSecretScreen != NULL);
    CHECK_STR(FakeImageName(g_gfxSecretScreen), "secretscreen_new.tga");
    CHECK(g_gfxSecretPic != NULL);
    CHECK(g_secretPicPick >= 0 && g_secretPicPick < g_numLevels);
    CHECK(g_secretPicPick != 3 && g_secretPicPick != 4);
    CHECK_EQ_INT(g_secretPicCur, g_secretPicPick);
    CHECK_EQ_INT(g_secretPicPrev, 3);
    CHECK_STR(FakeImageName(g_gfxSecretPic), g_secretPics[g_secretPicPick]);
    CheckBought(10000, 1000);
}

TEST(stages_shop_game_secret_is_fixed_for_a_rank_2_profile)
{
    ShopOpen(10000);
    g_profileIndex = 0;
    UnpackAccount(0);
    g_acc.completionRank = 2;
    PackAccount(0);
    Buy(13);
    CHECK_EQ_INT(g_secretPicPick, 30);
    CHECK_STR(FakeImageName(g_gfxSecretPic), "secret_31.jpg");
    CHECK_EQ_INT(P0.money, 9000);
}

TEST(stages_shop_game_secret_prefers_unseen_secrets_of_a_profile)
{
    ShopOpen(10000);
    g_profileIndex = 0;
    // Every secret but #7 found and seen: the pick has to be #7.
    for (int i = 0; i < g_numLevels; i++) {
        P0.secretFlags[i] = 1;
        P0.secretSeen[i] = 1;
    }
    P0.secretFlags[7] = 0;
    P0.secretSeen[7] = 0;
    Buy(13);
    CHECK_EQ_INT(g_secretPicPick, 7);
    CHECK_EQ_INT(P0.secretSeen[7], 1);
}

TEST(stages_shop_rank_marker_sells_the_missing_marks_in_order)
{
    ShopOpen(100000);
    P0.marks = 0;
    Buy(14);
    CHECK_EQ_INT((short)P0.marks, MARK_6);
    CheckBought(100000, 1250);
    Buy(14);
    CHECK_EQ_INT((short)P0.marks, MARK_6 | MARK_5);
    Buy(14);
    CHECK_EQ_INT((short)P0.marks, MARK_6 | MARK_5 | MARK_4);
    Buy(14);
    CHECK_EQ_INT((short)P0.marks, MARK_6 | MARK_5 | MARK_4 | MARK_3);
    Buy(14);
    CHECK_EQ_INT((short)P0.marks, MARKS_ALL & ~MARK_1);
    Buy(14);
    CHECK_EQ_INT((short)P0.marks, MARKS_ALL);
    CHECK_EQ_INT(P0.money, 100000 - 6 * 1250);
}

TEST(stages_shop_rank_marker_fills_a_gap)
{
    ShopOpen(100000);
    P0.marks = MARKS_ALL & ~MARK_3;
    Buy(14);
    CHECK_EQ_INT((short)P0.marks, MARKS_ALL);
    CheckBought(100000, 1250);
}

TEST(stages_shop_rank_marker_with_all_marks_pays_a_bonus_once)
{
    ShopOpen(100000);
    P0.marks = MARKS_ALL;
    P0.score = 0;
    P0.maxRankReached = 0;
    P0.color = 0;
    Buy(14);
    CHECK_EQ_INT(P0.score, 1000000);
    CHECK_EQ_INT(g_marksBonusGiven, 1);
    CHECK_EQ_INT(P0.color, 1);
    CHECK_EQ_INT(P0.money, 100000 - 1250);
    CHECK_EQ_INT(P0.shopVisits, 0);     // the bonus path doesn't ring up a visit
    // A second time: refused.
    Buy(14);
    CHECK_EQ_INT(P0.score, 1000000);
    CHECK_EQ_INT(P0.money, 100000 - 1250);
    CHECK_EQ_INT(Buzzes2(), 1);
}

TEST(stages_shop_rank_marker_bonus_keeps_colour_after_max_rank)
{
    ShopOpen(100000);
    P0.marks = MARKS_ALL;
    P0.maxRankReached = 1;
    P0.color = 0;
    Buy(14);
    CHECK_EQ_INT(P0.color, 0);
    CHECK_EQ_INT(g_marksBonusGiven, 1);
}

TEST(stages_shop_extra_time_adds_5_for_1500)
{
    ShopOpen(10000);
    P0.buffDuration = 20;
    Buy(15);
    CHECK_EQ_INT(P0.buffDuration, 25);
    CheckBought(10000, 1500);
}

TEST(stages_shop_extra_time_refused_at_the_maximum)
{
    ShopOpen(10000);
    P0.buffDuration = g_timeMax;
    Buy(15);
    CHECK_EQ_INT(P0.buffDuration, g_timeMax);
    CheckRefused(10000);
}

TEST(stages_shop_rocket_pack_adds_10_for_5000)
{
    ShopOpen(100000);
    P0.rockets = 0;
    Buy(18);
    CHECK_EQ_INT(P0.rockets, 10);
    CheckBought(100000, 5000);
}

TEST(stages_shop_rocket_pack_tops_up_to_50)
{
    ShopOpen(100000);
    P0.rockets = 45;
    Buy(18);
    CHECK_EQ_INT(P0.rockets, 50);
    CheckBought(100000, 5000);
}

TEST(stages_shop_rocket_pack_refused_at_50)
{
    ShopOpen(100000);
    P0.rockets = 50;
    Buy(18);
    CHECK_EQ_INT(P0.rockets, 50);
    CheckRefused(100000);
}

TEST(stages_shop_alien_lock_for_15000_once)
{
    ShopOpen(100000);
    P0.alienLock = 0;
    Buy(19);
    CHECK_EQ_INT(P0.alienLock, 1);
    CheckBought(100000, 15000);
    Buy(19);
    CHECK_EQ_INT(P0.money, 100000 - 15000);
    CHECK_EQ_INT(Buzzes2(), 1);
}

TEST(stages_shop_super_autofire_for_30000)
{
    ShopOpen(100000);
    P0.superAuto = 0;
    P0.autofire = 0;
    P0.autofireInterval = 100;
    Buy(20);
    CHECK_EQ_INT(P0.superAuto, 1);
    CHECK_EQ_INT(P0.autofire, 1);
    CHECK_EQ_INT(P0.autofireInterval, 25);
    CheckBought(100000, 30000);
}

TEST(stages_shop_super_autofire_refused_when_owned)
{
    ShopOpen(100000);
    P0.superAuto = 1;
    Buy(20);
    CheckRefused(100000);
}

TEST(stages_shop_clear_shields_needs_a_profile)
{
    ShopOpen(600000);
    P0.secretBirdCounter = 7;
    Buy(21);
    CHECK_EQ_INT(P0.money, 600000);
    CHECK_EQ_INT(P0.secretBirdCounter, 7);
    CHECK_EQ_INT(Cash(), 0);
}

TEST(stages_shop_clear_shields_resets_the_profile_progress)
{
    ShopOpen(600000);
    g_profileIndex = 0;
    UnpackAccount(0);
    g_acc.completionRank = 1;
    PackAccount(0);
    for (int i = 0; i < g_numLevels; i++)
        P0.secretFlags[i] = 1;
    P0.secretBirdCounter = 7;
    P0.secretBirdTick = 9;
    P0.secretBirdHits = 4;
    Buy(21);
    CHECK_EQ_INT(P0.money, 100000);
    CHECK_EQ_INT(P0.secretBirdCounter, 20);
    CHECK_EQ_INT(P0.secretBirdTick, 3);
    CHECK_EQ_INT(P0.secretBirdHits, 0);
    CHECK_EQ_INT(P0.secretFlags[0], 0);
    CHECK_EQ_INT(P0.secretFlags[24], 0);
    CHECK_EQ_INT(P0.secretFlags[25], 1);
    CHECK_EQ_INT(GetRank(0), 2);       // rank 1 is promoted
    CHECK(IsSecretFound(0, 0x1a));
    CHECK_EQ_INT(Cash(), 1);
}

// ---- money checks ----

TEST(stages_shop_unaffordable_item_buzzes_and_takes_nothing)
{
    ShopOpen(90);
    P0.weapon = WEAPON_SINGLE;
    Buy(3);     // DOUBLE SHOT, 100
    CHECK_EQ_INT(P0.weapon, WEAPON_SINGLE);
    CHECK_EQ_INT(P0.money, 90);
    CHECK_EQ_INT(Buzzes(), 1);
    CHECK_EQ_INT(Cash(), 0);
    CHECK_EQ_INT(g_shopClosing, 0);     // 90 still buys something
}

TEST(stages_shop_unaffordable_with_under_50_closes_the_shop)
{
    ShopOpen(40);
    P0.speed = g_speedBase;
    Buy(1);
    CHECK_NEAR(P0.speed, g_speedBase, 1e-4);
    CHECK_EQ_INT(P0.money, 40);
    CHECK_EQ_INT(g_shopClosing, 1);
    CHECK_EQ_INT(g_playerBroke, 1);
}

TEST(stages_shop_spending_below_50_closes_the_shop)
{
    ShopOpen(149);
    P0.weapon = WEAPON_SINGLE;
    Buy(3);
    CHECK_EQ_INT(P0.money, 49);
    CHECK_EQ_INT(g_shopClosing, 1);
    CHECK_EQ_INT(g_playerBroke, 1);
    CHECK_NEAR(g_shopSlideVelY, 4 * 1.05, 1e-5);     // 4, then one closing frame
    CHECK(FakePlayCount("slide") >= 1);
}

TEST(stages_shop_keeping_exactly_50_keeps_it_open)
{
    ShopOpen(150);
    P0.weapon = WEAPON_SINGLE;
    Buy(3);
    CHECK_EQ_INT(P0.money, 50);
    CHECK_EQ_INT(g_shopClosing, 0);
}

TEST(stages_shop_buys_only_on_the_fire_edge)
{
    ShopOpen(10000);
    P0.bullets = 8;
    g_shopSelItem = 2;
    FakePressKey((enum EKeyboardLayout)g_cfg.fire[0]);
    for (int i = 0; i < 5; i++)
        ShopFrame();
    CHECK_EQ_INT(P0.bullets, 9);
    CHECK_EQ_INT(P0.money, 10000 - 75);
}

TEST(stages_shop_ignores_input_during_the_cooldown)
{
    ShopOpen(10000);
    P0.bullets = 8;
    g_inputCooldown = 5;
    Buy(2);
    CHECK_EQ_INT(P0.bullets, 8);
    CHECK_EQ_INT(P0.money, 10000);
}

TEST(stages_shop_ignores_fire_until_slid_in)
{
    ShopOpen(10000);
    P0.bullets = 8;
    g_shopTransition = 400;
    g_gfxSecretScreen = NULL;
    Buy(2);
    CHECK_EQ_INT(P0.bullets, 8);
}

// ---- leaving ----

TEST(stages_shop_fire_on_exit_row_closes_the_shop)
{
    ShopOpen(10000);
    Buy(0);
    CHECK_EQ_INT(g_shopClosing, 1);
    CHECK_EQ_INT(g_playerBroke, 1);
    CHECK_EQ_INT(P0.money, 10000);
}

TEST(stages_shop_escape_goes_to_exit_row_then_leaves)
{
    ShopOpen(10000);
    g_shopSelItem = 5;
    Tap(K_VK_ESCAPE);
    CHECK_EQ_INT(g_shopSelItem, 0);
    CHECK_EQ_INT(g_shopClosing, 0);
    Tap(K_VK_ESCAPE);
    CHECK_EQ_INT(g_shopClosing, 1);
}

TEST(stages_shop_closing_slides_up_and_starts_the_next_level)
{
    ShopOpen(10000);
    Buy(0);
    P0.levelTransitioning = 1;
    for (int i = 0; i < 300 && g_state == STATE_SHOP; i++)
        ShopFrame();
    CHECK_EQ_INT(g_state, STATE_RESPAWN);     // StartNextLevel()
    CHECK_EQ_INT(g_shopClosing, 0);
    CHECK_EQ_INT(P0.levelTransitioning, 0);
}

TEST(stages_shop_closing_with_all_marks_promotes_a_rank)
{
    ShopOpen(10000);
    Buy(0);
    P0.marks = MARKS_ALL;
    P0.rank = 3;
    P0.bestRank = 3;
    P0.score = 0;
    for (int i = 0; i < 300 && g_state == STATE_SHOP; i++)
        ShopFrame();
    CHECK_EQ_INT(g_state, STATE_SHOP_GATE);
    CHECK_EQ_INT(P0.rank, 4);
    CHECK_EQ_INT(P0.bestRank, 4);
    CHECK_EQ_INT((short)P0.marks, 0);
    CHECK_EQ_INT(P0.score, 1000000);
}

TEST(stages_shop_closing_with_all_marks_at_the_unlocked_rank_only_pays)
{
    ShopOpen(10000);
    Buy(0);
    P0.marks = MARKS_ALL;
    P0.rank = 20;           // GetStat() without a profile: 20
    P0.score = 0;
    for (int i = 0; i < 300 && g_state == STATE_SHOP; i++)
        ShopFrame();
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK_EQ_INT(P0.rank, 20);
    CHECK_EQ_INT((short)P0.marks, 0);
    CHECK_EQ_INT(P0.score, 1000000);
}

TEST(stages_shop_dual_hands_the_shop_to_player_two)
{
    ShopOpen(10000);
    g_gameMode = MODE_DUAL;
    P1.money = 500;
    P1.ship = 0;
    P1.lives = 30;      // > minEnergy 26
    P1.level = 5;
    Buy(0);
    g_shopSelItem = 4;      // where player one left the cursor
    for (int i = 0; i < 300 && g_shopCurPlayer == 0; i++)
        ShopFrame();
    CHECK_EQ_INT(g_shopCurPlayer, 1);
    CHECK_EQ_INT(g_state, STATE_SHOP);
    CHECK_EQ_INT(g_shopSelItem, 0);
    CHECK_NEAR(g_offY, -600, 1e-3);
    CHECK_NEAR(g_shopSlideVelY, 40, 1e-3);
}

TEST(stages_shop_dual_skips_a_broke_player_two)
{
    ShopOpen(10000);
    g_gameMode = MODE_DUAL;
    P1.money = 49;
    P1.ship = 0;
    P1.lives = 30;
    Buy(0);
    for (int i = 0; i < 300 && g_state == STATE_SHOP; i++)
        ShopFrame();
    CHECK_EQ_INT(g_shopCurPlayer, 0);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
}

// ---- moving the selection ----

TEST(stages_shop_up_moves_only_to_affordable_rows)
{
    ShopOpen(80);
    Tap(g_cfg.up[0]);
    CHECK_EQ_INT(g_shopSelItem, 1);
    CHECK_STR(FakeImageName(g_gfxShopItemPic), "shop_speed.jpg");
    Tap(g_cfg.up[0]);
    CHECK_EQ_INT(g_shopSelItem, 2);
    Tap(g_cfg.up[0]);       // DOUBLE SHOT costs 100
    CHECK_EQ_INT(g_shopSelItem, 2);
    CHECK_EQ_INT(FakePlayCount("rollover"), 2);
}

TEST(stages_shop_up_stops_at_the_last_unlocked_row)
{
    ShopOpen(1000000);
    g_shopItems = 0x53;
    g_shopSelItem = 0x53 - 0x42;
    Tap(g_cfg.up[0]);
    CHECK_EQ_INT(g_shopSelItem, 0x53 - 0x42);
}

TEST(stages_shop_down_moves_back_to_the_exit_row)
{
    ShopOpen(1000);
    g_shopSelItem = 2;
    Tap(g_cfg.down[0]);
    CHECK_EQ_INT(g_shopSelItem, 1);
    Tap(g_cfg.down[0]);
    CHECK_EQ_INT(g_shopSelItem, 0);
    Tap(g_cfg.down[0]);
    CHECK_EQ_INT(g_shopSelItem, 0);
}

TEST(stages_shop_mouse_hover_selects_the_row_under_it)
{
    ShopOpen(1000);
    g_shopItems = 0x53;     // 18 rows: 380 / 18 px each
    g_buttonsOn = 1;
    g_mouseX = 500;
    g_mouseY = (int)(502 - 380.0 / 18 * 3.5);
    ShopFrame();
    CHECK_EQ_INT(g_shopSelItem, 3);
    CHECK_EQ_INT(g_shopHoverItem, 3);
    CHECK_STR(FakeImageName(g_gfxShopItemPic), "shop_doubleshot.jpg");
}

TEST(stages_shop_mouse_hover_on_unaffordable_row_selects_exit)
{
    ShopOpen(90);
    g_shopItems = 0x53;
    g_buttonsOn = 1;
    g_mouseX = 500;
    g_mouseY = (int)(502 - 380.0 / 18 * 3.5);
    ShopFrame();
    CHECK_EQ_INT(g_shopSelItem, 0);
}

// ---- which items are unlocked ----

static int ItemsAfterOneFrame(long long stat, int bonus, int mode)
{
    ShopOpen(1000);
    g_shopItems = 0x53;
    g_shownStat = stat;
    g_bonusFlag = bonus;
    g_gameMode = mode;
    ShopFrame();
    return g_shopItems;
}

TEST(stages_shop_stat_below_70_keeps_the_item_list)
{
    CHECK_EQ_INT(ItemsAfterOneFrame(69, 0, MODE_SINGLE), 0x53);
}

TEST(stages_shop_stat_70_unlocks_tier_one)
{
    CHECK_EQ_INT(ItemsAfterOneFrame(70, 0, MODE_SINGLE), 0x54);
}

TEST(stages_shop_stat_80_unlocks_tier_two)
{
    CHECK_EQ_INT(ItemsAfterOneFrame(80, 0, MODE_SINGLE), 0x55);
}

TEST(stages_shop_stat_90_unlocks_tier_three)
{
    CHECK_EQ_INT(ItemsAfterOneFrame(90, 0, MODE_SINGLE), 0x56);
}

TEST(stages_shop_bonus_rank_unlocks_everything)
{
    CHECK_EQ_INT(ItemsAfterOneFrame(0, 1, MODE_SINGLE), 0x57);
}

TEST(stages_shop_time_trial_keeps_the_item_list)
{
    CHECK_EQ_INT(ItemsAfterOneFrame(95, 1, MODE_TIME_TRIAL), 0x53);
}

// ---- helpers ----

TEST(stages_shop_AddCash_counts_a_visit_and_rings)
{
    BootGame();
    g_shopCurPlayer = 1;
    P1.shopVisits = 2;
    AddCash();
    CHECK_EQ_INT(P1.shopVisits, 3);
    CHECK_EQ_INT(Cash(), 1);
}

TEST(stages_shop_LoadShopPic_loads_the_item_picture)
{
    BootGame();
    LoadShopPic(17);
    CHECK_STR(FakeImageName(g_gfxShopItemPic), "shop_rocketpack.jpg");
    CHECK_EQ_INT(g_shopPic, 17);
    Image *before = g_gfxShopItemPic;
    int freed = g_fake.imagesFreed;
    LoadShopPic(-1);
    CHECK(g_gfxShopItemPic == before);
    CHECK_EQ_INT(g_fake.imagesFreed, freed);
    LoadShopPic(0);
    CHECK_EQ_INT(g_fake.imagesFreed, freed + 1);
    CHECK_STR(FakeImageName(g_gfxShopItemPic), "shop_speed.jpg");
    FreeShopItemGfx();
    CHECK(g_gfxShopItemPic == NULL);
}

TEST(stages_shop_LoadSecretPic_loads_the_secret_picture)
{
    BootGame();
    CHECK_EQ_INT(LoadSecretPic(4), 1);
    CHECK_STR(FakeImageName(g_gfxSecretPic), "secret_05.jpg");
    CHECK_EQ_INT(g_secretPic, 4);
    FreeSecretPicGfx();
    CHECK(g_gfxSecretPic == NULL);
}

TEST(stages_shop_AddSparkleFlash_takes_the_first_free_slot)
{
    BootGame();
    for (int i = 0; i < MAX_SPARKLE_FLASHES; i++)
        g_sparkleFlashes[i].active = 0;
    g_sparkleFlashes[0].active = 1;
    AddSparkleFlash(g_gfxFlare10, 10, 20, 30, 1, 2, 3, 300, 40);
    CHECK_EQ_INT(g_sparkleFlashes[1].active, 1);
    CHECK_EQ_INT(g_sparkleFlashes[1].x, 10);
    CHECK_EQ_INT(g_sparkleFlashes[1].y, 20);
    CHECK_EQ_INT(g_sparkleFlashes[1].size, 30);
    CHECK_EQ_INT(g_sparkleFlashes[1].r, 1);
    CHECK_EQ_INT(g_sparkleFlashes[1].g, 2);
    CHECK_EQ_INT(g_sparkleFlashes[1].b, 3);
    CHECK_NEAR(g_sparkleFlashes[1].alpha, 300, 1e-6);
    CHECK_NEAR(g_sparkleFlashes[1].fade, 40, 1e-6);
    CHECK_EQ_INT(g_sparkleFlashes[2].active, 0);
}

TEST(stages_shop_UpdateSparkleFlashes_fades_then_frees)
{
    BootGame();
    for (int i = 0; i < MAX_SPARKLE_FLASHES; i++)
        g_sparkleFlashes[i].active = 0;
    AddSparkleFlash(g_gfxFlare10, 10, 20, 30, 1, 2, 3, 100, 40);
    UpdateSparkleFlashes();
    CHECK_NEAR(g_sparkleFlashes[0].alpha, 60, 1e-4);
    CHECK_EQ_INT(g_sparkleFlashes[0].active, 1);
    UpdateSparkleFlashes();
    CHECK_NEAR(g_sparkleFlashes[0].alpha, 20, 1e-4);
    CHECK_EQ_INT(g_sparkleFlashes[0].active, 1);
    UpdateSparkleFlashes();
    CHECK_EQ_INT(g_sparkleFlashes[0].active, 0);
}

TEST(stages_shop_CheckProfileBonus_without_profile)
{
    BootGame();
    g_bonusFlag = 1;
    g_hasLives = 1;
    CHECK_EQ_INT(CheckProfileBonus(), 1);
    CHECK_EQ_INT(g_bonusFlag, 0);
    CHECK_EQ_INT(g_lives, 0);
    CHECK_EQ_INT(g_hasLives, 0);
}

TEST(stages_shop_CheckProfileBonus_reads_profile_lives)
{
    BootGame();
    g_profileIndex = 0;
    g_gameMode = MODE_SINGLE;
    UnpackAccount(0);
    g_acc.lives = 3;
    PackAccount(0);
    g_shopCurPlayer = 0;
    P0.level = 10;
    CheckProfileBonus();
    CHECK_EQ_INT(g_lives, 3);
    CHECK_EQ_INT(g_hasLives, 1);
    CHECK_EQ_INT(g_extraLifeGranted, 0);
}

TEST(stages_shop_CheckProfileBonus_grants_a_life_every_250_levels_when_broke)
{
    BootGame();
    g_profileIndex = 0;
    g_gameMode = MODE_SINGLE;
    UnpackAccount(0);
    g_acc.lives = 2;
    PackAccount(0);
    g_shopCurPlayer = 0;
    P0.level = 249;
    g_playerBroke = 1;
    g_extraLifeGranted = 0;
    CheckProfileBonus();
    CHECK_EQ_INT(g_lives, 3);
    CHECK_EQ_INT(g_extraLifeGranted, 1);
    CHECK_EQ_INT(FakePlayCount("fanfare"), 1);
    CheckProfileBonus();    // latched: not twice
    CHECK_EQ_INT(g_lives, 3);
    P0.level = 250;
    CheckProfileBonus();
    CHECK_EQ_INT(g_extraLifeGranted, 0);
}

TEST(stages_shop_CheckProfileBonus_no_life_when_not_broke)
{
    BootGame();
    g_profileIndex = 0;
    g_gameMode = MODE_SINGLE;
    UnpackAccount(0);
    g_acc.lives = 2;
    PackAccount(0);
    g_shopCurPlayer = 0;
    P0.level = 499;
    g_playerBroke = 0;
    g_extraLifeGranted = 0;
    CheckProfileBonus();
    CHECK_EQ_INT(g_lives, 2);
}

TEST(stages_shop_CheckProfileBonus_flags_rank_with_all_medals)
{
    BootGame();
    g_profileIndex = 0;
    g_gameMode = MODE_SINGLE;
    UnpackAccount(0);
    g_acc.completionRank = 1;
    g_acc.medals = 0xffffffff;
    PackAccount(0);
    if (!HasAllMedals(0))
        TestFail(__FILE__, __LINE__, "couldn't give the profile all medals");
    CheckProfileBonus();
    CHECK_EQ_INT(g_bonusFlag, 1);
    UnpackAccount(0);
    g_acc.completionRank = 0;
    PackAccount(0);
    CheckProfileBonus();
    CHECK_EQ_INT(g_bonusFlag, 0);
}

TEST(stages_shop_DrawSavePrompt_alternates_the_two_messages)
{
    BootGame();
    g_lives = 3;
    g_blinkRate = 100;
    g_uiBlink = 0;
    g_saveMsgToggle = 0;
    g_uiBlinkTime = 0;
    g_saveMsgBlinkRate = 0;
    g_time = 1000;
    DrawSavePrompt();
    CHECK_EQ_INT(g_uiBlink, 1);
    CHECK_STR(g_logBuf, "PRESS F1 TO SAVE GAME AND EXIT TO WINDOWS      3 SAVES LEFT");
    CHECK_EQ_INT(g_saveMsgBlinkRate, 400);
    g_time += 401;
    DrawSavePrompt();
    CHECK_EQ_INT(g_uiBlink, 0);
    CHECK_EQ_INT(g_saveMsgBlinkRate, 100);
    g_time += 101;
    DrawSavePrompt();
    CHECK_STR(g_logBuf, "PRESS F2 TO SAVE GAME AND EXIT TO MENUSCREEN   3 SAVES LEFT");
}
