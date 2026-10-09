// Tests for src/game/stages/memorystation.c: the memory-station card game (dealing the grid,
// turning cards, matching pairs, each card's reward, the bonus for clearing the grid, the
// countdown and the timeout).
#include "support.h"

#define P0 g_save.players[0]

// Ship 0: minEnergy 26 + maxEnergy 20 lives, cost 4; armour 123 + 10, step 5; gems 452 + 8n.
enum { LIVES_CAP = 46, ARMOUR_CAP = 133, GEM_BASE = 452, GEM_STEP = 8 };

static void MemSetUp(void)
{
    BootGame();
    SeedRand(2468);
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_scoreMul[0] = 1;
    g_scoreMul[1] = 1;
    g_frameDt = 1;
    g_time = 50000;
    g_autoplay = 0;
    g_mouseDown = 0;
    g_rightButton = 0;
    g_buttonsOn = 0;
    g_inputCooldown = 0;
    P0.ship = 0;
    P0.rows = 4;
    P0.cols = 4;
    P0.score = 0;
    P0.tries = 0;
    g_state = STATE_MEMORY_STATION;
    g_soundQueueCount = 0;
    g_soundQueueNext = 0;
}

// A 4 x 4 grid, every card face down: cards (0,0) and (1,0) are a pair of `type`, the others
// are all different (no other pairs).
static void Grid(int type)
{
    MemSetUp();
    InitGrid();
    int filler = 2, n = 0;
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 4; y++) {
            g_cards[x][y].active = 1;
            g_cards[x][y].open = 1;
            if ((x == 0 || x == 1) && y == 0) {
                g_cards[x][y].type = type;
                continue;
            }
            while (filler == type || filler == CARD_EXTRA_TIME || filler == CARD_LOSE_TIME ||
                   filler == CARD_SECRET_BIRD || (filler >= 20 && filler <= 23))
                filler++;
            g_cards[x][y].type = filler++;
            n++;
        }
    g_memStageDeadline = g_time + 100000;
    g_memCountdownStage = 11;
    g_memPendingCardType = -1;
    g_memPickResolveTime = 0;
    g_memoryBonusPending = 0;
}

// Moves the cursor to card (x, y) and presses fire for one update.
static void Pick(int x, int y)
{
    g_memSelRow = x;
    g_memSelCol = y;
    FakePressKey((enum EKeyboardLayout)g_cfg.fire[0]);
    MemoryBonusUpdate();
    FakeReleaseKey((enum EKeyboardLayout)g_cfg.fire[0]);
}

// Lets the pending pick resolve.
static void Settle(void)
{
    g_time += 1000;
    MemoryBonusUpdate();
}

// Turns the pair (0,0)/(1,0) and lets it resolve.
static void MatchPair(void)
{
    Pick(0, 0);
    Pick(1, 0);
    Settle();
}

static bool Queued(AudioHandle s)
{
    for (int i = 0; i < g_soundQueueCount; i++)
        if (g_soundQueue[i].sample == s)
            return true;
    return false;
}

// ---- dealing ----

TEST(stages_memory_InitGrid_deals_a_centred_grid_face_down)
{
    MemSetUp();
    P0.rows = 5;
    P0.cols = 3;
    InitGrid();
    CHECK_EQ_INT(g_gridX, 400 - 5 * 32);
    CHECK_EQ_INT(g_gridY, 300 - 3 * 32);
    for (int x = 0; x < 5; x++)
        for (int y = 0; y < 3; y++) {
            CHECK_EQ_INT(g_cards[x][y].active, 1);
            CHECK_EQ_INT(g_cards[x][y].open, 1);
            CHECK_EQ_INT(g_cards[x][y].x, g_gridX + x * 64);
            CHECK_EQ_INT(g_cards[x][y].y, g_gridY + y * 64);
            bool inPool = false;
            for (int k = 0; k < 35; k++)
                inPool |= g_cards[x][y].type == g_bonusRoundTypePool[k];
            CHECK_MSG(inPool, "card %d,%d: type %d", x, y, g_cards[x][y].type);
        }
    CHECK_EQ_INT(AllTypesUnique(), 0);     // at least one pair to find
}

TEST(stages_memory_InitGrid_always_deals_a_pair)
{
    MemSetUp();
    P0.rows = 2;
    P0.cols = 2;
    for (int k = 0; k < 50; k++) {
        InitGrid();
        CHECK_EQ_INT(AllTypesUnique(), 0);
    }
}

TEST(stages_memory_InitGrid_resets_the_round)
{
    MemSetUp();
    P0.buffDuration = 20;
    P0.secretBirdCounter = 33;
    g_memSelRow = 3;
    g_memSelCol = 2;
    g_memPendingCardType = 7;
    g_memPickResolveTime = 99;
    g_memoryBonusPending = 1;
    g_memoryDone = 1;
    g_rocketRepeatTimer = 5;
    g_popups[0].active = 1;
    InitGrid();
    CHECK_EQ_INT(g_memSelRow, 0);
    CHECK_EQ_INT(g_memSelCol, 0);
    CHECK_EQ_INT(g_memPendingCardType, -1);
    CHECK_EQ_INT(g_memPickResolveTime, 0);
    CHECK_EQ_INT(g_memoryBonusPending, 0);
    CHECK_EQ_INT(g_memoryDone, 0);
    CHECK_EQ_INT(g_rocketRepeatTimer, 0);
    CHECK_EQ_INT(g_popups[0].active, 0);
    CHECK_EQ_INT(g_memorySecretBirdSnapshot, 33);
    CHECK_EQ_INT(g_memStageDeadline, g_time + 30000);     // 1.5 s per second of buff time
}

TEST(stages_memory_InitGrid_caps_the_time_at_5_minutes)
{
    MemSetUp();
    P0.buffDuration = 250;
    InitGrid();
    CHECK_EQ_INT(g_memStageDeadline, g_time + 300000);
}

// ---- AllTypesUnique / CountTypePairs ----

TEST(stages_memory_AllTypesUnique_looks_at_face_down_cards)
{
    Grid(CARD_SCORE_100);
    CHECK_EQ_INT(AllTypesUnique(), 0);
    g_cards[1][0].open = 0;     // face up: no hidden pair left
    CHECK_EQ_INT(AllTypesUnique(), 1);
    g_cards[1][0].open = 1;
    g_cards[1][0].active = 0;   // taken
    CHECK_EQ_INT(AllTypesUnique(), 1);
}

TEST(stages_memory_AllTypesUnique_ignores_extra_time_cards)
{
    Grid(CARD_EXTRA_TIME);
    CHECK_EQ_INT(AllTypesUnique(), 1);
}

TEST(stages_memory_CountTypePairs_counts_remaining_pairs)
{
    Grid(CARD_SCORE_100);
    CHECK_EQ_INT(CountTypePairs(), 1);
    g_cards[2][0].type = CARD_SCORE_100;    // three of a kind: still one pair
    CHECK_EQ_INT(CountTypePairs(), 1);
    g_cards[3][0].type = CARD_SCORE_100;
    CHECK_EQ_INT(CountTypePairs(), 2);
    g_cards[0][1].type = g_cards[1][1].type = CARD_EXTRA_TIME;
    CHECK_EQ_INT(CountTypePairs(), 2);      // extra-time cards don't count
    g_cards[3][0].active = 0;
    CHECK_EQ_INT(CountTypePairs(), 1);
}

// ---- turning cards ----

TEST(stages_memory_first_pick_turns_the_card)
{
    Grid(CARD_SCORE_100);
    Pick(2, 3);
    CHECK_EQ_INT(g_cards[2][3].open, 0);
    CHECK_EQ_INT(g_cards[0][0].open, 1);
    CHECK_EQ_INT(P0.tries, 0);
    CHECK_EQ_INT(g_memPendingCardType, -1);
    CHECK_EQ_INT(g_memPickResolveTime, 0);
}

TEST(stages_memory_matching_pair_is_taken)
{
    Grid(CARD_SCORE_1000);
    // Leave another same-type pair face down; matching the open pair must not consume it.
    g_cards[2][0].type = g_cards[3][0].type = CARD_SCORE_1000;
    Pick(0, 0);
    Pick(1, 0);
    CHECK_EQ_INT(P0.tries, 1);
    CHECK_EQ_INT(g_memPendingCardType, CARD_SCORE_1000);
    CHECK_EQ_INT(g_memPickResolveTime, g_time + 150);
    CHECK_EQ_INT(FakePlayCount("foundit"), 1);
    g_time += 150;
    MemoryBonusUpdate();        // not yet
    CHECK_EQ_INT(g_cards[0][0].active, 1);
    g_time += 1;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_cards[0][0].active, 0);
    CHECK_EQ_INT(g_cards[1][0].active, 0);
    CHECK_EQ_INT(g_cards[2][0].active, 1);
    CHECK_EQ_INT(g_cards[3][0].active, 1);
    CHECK_EQ_INT(g_memPendingCardType, -1);
    CHECK_EQ_INT(P0.score, 1000);
}

TEST(stages_memory_mismatch_turns_both_back)
{
    Grid(CARD_SCORE_1000);
    Pick(0, 0);
    Pick(2, 2);
    CHECK_EQ_INT(P0.tries, 1);
    CHECK_EQ_INT(g_memPendingCardType, -1);
    CHECK_EQ_INT(g_memPickResolveTime, g_time + 450);
    CHECK_EQ_INT(g_cards[2][2].open, 0);
    g_time += 450;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_cards[2][2].open, 0);
    g_time += 1;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_cards[0][0].open, 1);
    CHECK_EQ_INT(g_cards[2][2].open, 1);
    CHECK_EQ_INT(g_cards[0][0].active, 1);
    CHECK_EQ_INT(P0.score, 0);
}

TEST(stages_memory_picking_the_turned_card_again_does_nothing)
{
    Grid(CARD_SCORE_1000);
    Pick(0, 0);
    Pick(0, 0);
    CHECK_EQ_INT(P0.tries, 0);
    CHECK_EQ_INT(g_memPickResolveTime, 0);
}

TEST(stages_memory_no_pick_while_a_pick_resolves)
{
    Grid(CARD_SCORE_1000);
    Pick(0, 0);
    Pick(2, 2);
    Pick(3, 3);         // still showing the mismatch
    CHECK_EQ_INT(g_cards[3][3].open, 1);
    CHECK_EQ_INT(P0.tries, 1);
}

TEST(stages_memory_lose_time_card_works_alone)
{
    Grid(CARD_SCORE_1000);
    g_cards[3][3].type = CARD_LOSE_TIME;
    int deadline = g_memStageDeadline;
    g_memCountdownStage = 4;
    Pick(3, 3);
    CHECK_EQ_INT(g_memPendingCardType, CARD_LOSE_TIME);
    CHECK_EQ_INT(g_memPickResolveTime, g_time + 350);
    Settle();
    CHECK_EQ_INT(g_memStageDeadline, deadline - 15000);
    CHECK_EQ_INT(g_memCountdownStage, 11);
    CHECK_EQ_INT(g_cards[3][3].active, 0);
    CHECK_EQ_INT(FakePlayCount("oops"), 1);
}

TEST(stages_memory_extra_time_card_works_alone)
{
    Grid(CARD_SCORE_1000);
    g_cards[3][3].type = CARD_EXTRA_TIME;
    int deadline = g_memStageDeadline;
    Pick(3, 3);
    CHECK_EQ_INT(g_memPendingCardType, CARD_EXTRA_TIME);
    CHECK_EQ_INT(g_memPickResolveTime, g_time + 450);
    Settle();
    CHECK_EQ_INT(g_memStageDeadline, deadline + 10000);
    CHECK_EQ_INT(g_cards[3][3].active, 0);
}

TEST(stages_memory_single_card_as_second_pick_costs_no_try)
{
    Grid(CARD_SCORE_1000);
    g_cards[3][3].type = CARD_EXTRA_TIME;
    int deadline = g_memStageDeadline;
    Pick(0, 0);
    Pick(3, 3);
    CHECK_EQ_INT(P0.tries, 0);
    CHECK_EQ_INT(g_memPendingCardType, CARD_EXTRA_TIME);
    Settle();
    CHECK_EQ_INT(g_memStageDeadline, deadline + 10000);
}

// ---- moving the cursor ----

TEST(stages_memory_cursor_moves_one_card_per_press)
{
    Grid(CARD_SCORE_1000);
    g_memSelRow = 1;
    g_memSelCol = 1;
    MemoryBonusUpdate();        // nothing held: arms the key latches
    FakePressKey((enum EKeyboardLayout)g_cfg.right[0]);
    MemoryBonusUpdate();
    MemoryBonusUpdate();        // held: no repeat
    CHECK_EQ_INT(g_memSelRow, 2);
    FakeReleaseKey((enum EKeyboardLayout)g_cfg.right[0]);
    MemoryBonusUpdate();
    FakePressKey((enum EKeyboardLayout)g_cfg.down[0]);
    MemoryBonusUpdate();
    FakeReleaseKey((enum EKeyboardLayout)g_cfg.down[0]);
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memSelCol, 2);
    FakePressKey((enum EKeyboardLayout)g_cfg.left[0]);
    MemoryBonusUpdate();
    FakeReleaseKey((enum EKeyboardLayout)g_cfg.left[0]);
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memSelRow, 1);
    FakePressKey((enum EKeyboardLayout)g_cfg.up[0]);
    MemoryBonusUpdate();
    FakeReleaseKey((enum EKeyboardLayout)g_cfg.up[0]);
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memSelCol, 1);
    CHECK_EQ_INT(FakePlayCount("over"), 4);
}

TEST(stages_memory_cursor_stays_on_the_grid)
{
    Grid(CARD_SCORE_1000);
    int keys[4] = {g_cfg.left[0], g_cfg.up[0], g_cfg.right[0], g_cfg.down[0]};
    g_memSelRow = 0;
    g_memSelCol = 0;
    MemoryBonusUpdate();
    for (int k = 0; k < 2; k++) {
        FakePressKey((enum EKeyboardLayout)keys[k]);
        MemoryBonusUpdate();
        FakeReleaseKey((enum EKeyboardLayout)keys[k]);
        MemoryBonusUpdate();
    }
    CHECK_EQ_INT(g_memSelRow, 0);
    CHECK_EQ_INT(g_memSelCol, 0);
    g_memSelRow = 3;
    g_memSelCol = 3;
    for (int k = 2; k < 4; k++) {
        FakePressKey((enum EKeyboardLayout)keys[k]);
        MemoryBonusUpdate();
        FakeReleaseKey((enum EKeyboardLayout)keys[k]);
        MemoryBonusUpdate();
    }
    CHECK_EQ_INT(g_memSelRow, 3);
    CHECK_EQ_INT(g_memSelCol, 3);
}

TEST(stages_memory_mouse_selects_the_card_under_it)
{
    Grid(CARD_SCORE_1000);
    g_buttonsOn = 1;
    g_idleFrames = 0;
    g_mouseClick = 1;
    g_mouseX = g_cards[2][3].x + 10;
    g_mouseY = g_cards[2][3].y + 63;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memSelRow, 2);
    CHECK_EQ_INT(g_memSelCol, 3);
    g_mouseX = g_cards[1][1].x;      // on the border: not inside a card
    g_mouseY = g_cards[1][1].y + 5;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memSelRow, 2);
}

// ---- the rewards ----

TEST(stages_memory_score_cards)
{
    Grid(CARD_SCORE_100);
    MatchPair();
    CHECK_EQ_INT(P0.score, 100);
    Grid(CARD_SCORE_10000);
    g_scoreMul[0] = 2;
    MatchPair();
    CHECK_EQ_INT(P0.score, 20000);
}

TEST(stages_memory_times_two_card)
{
    Grid(CARD_SCORE_X2);
    P0.buffDuration = 20;
    P0.scoreMult5Timer = 7;
    MatchPair();
    CHECK_EQ_INT(g_scoreMul[0], 2);
    CHECK_EQ_INT(P0.scoreMult2Timer, g_time + 20000);
    CHECK_EQ_INT(P0.scoreMult5Timer, 0);
}

TEST(stages_memory_times_five_card)
{
    Grid(CARD_SCORE_X5);
    P0.buffDuration = 25;
    P0.scoreMult2Timer = 7;
    MatchPair();
    CHECK_EQ_INT(g_scoreMul[0], 5);
    CHECK_EQ_INT(P0.scoreMult5Timer, g_time + 25000);
    CHECK_EQ_INT(P0.scoreMult2Timer, 0);
}

TEST(stages_memory_money_cards)
{
    static const int cards[3] = {CARD_MONEY_50, CARD_MONEY_100, CARD_MONEY_200};
    static const int money[3] = {50, 100, 200};
    for (int k = 0; k < 3; k++) {
        Grid(cards[k]);
        P0.money = 1000;
        P0.moneyMax = 99990;
        MatchPair();
        CHECK_EQ_INT(P0.money, 1000 + money[k]);
        CHECK_EQ_INT(P0.score, 0);
    }
    CHECK_EQ_INT(FakePlayCount("coin"), 3);
}

TEST(stages_memory_money_card_over_the_cap_pays_score)
{
    Grid(CARD_MONEY_200);
    P0.money = 99900;
    P0.moneyMax = 99990;
    g_moneyMax = 0;
    MatchPair();
    CHECK_EQ_INT(P0.money, 99990);
    CHECK_EQ_INT(P0.score, 200);
    CHECK_EQ_INT(g_moneyMax, 99990);
}

TEST(stages_memory_money_doubler)
{
    Grid(CARD_MONEY_DOUBLER);
    P0.money = 3000;
    P0.moneyMax = 99990;
    MatchPair();
    CHECK_EQ_INT(P0.money, 6000);
    CHECK_EQ_INT(P0.score, 0);
}

TEST(stages_memory_money_doubler_over_the_cap_pays_score)
{
    Grid(CARD_MONEY_DOUBLER);
    P0.money = 60000;
    P0.moneyMax = 99990;
    MatchPair();
    CHECK_EQ_INT(P0.money, 99990);
    CHECK_EQ_INT(P0.score, 99990 * 2);
}

TEST(stages_memory_money_doubler_malfunctions_when_rich)
{
    Grid(CARD_MONEY_DOUBLER);
    P0.money = 450000;
    P0.moneyMax = 999990;
    MatchPair();
    CHECK_EQ_INT(P0.money, 450000);
    CHECK_STR(g_alertMsg, "MONEY DOUBLER MALFUNCTION");
    CHECK_EQ_INT(g_msgTimer, g_time + 1000);
}

TEST(stages_memory_extra_life_card)
{
    Grid(CARD_EXTRA_LIFE);
    P0.lives = 30;
    int gained = g_livesGainedCount;
    MatchPair();
    CHECK_EQ_INT(P0.lives, 34);
    CHECK_EQ_INT(g_livesGainedCount, gained + 10);
    CHECK(Queued(g_sfxExtraLife));
}

TEST(stages_memory_extra_life_card_clamps_to_the_hangar)
{
    Grid(CARD_EXTRA_LIFE);
    P0.lives = LIVES_CAP - 1;
    MatchPair();
    CHECK_EQ_INT(P0.lives, LIVES_CAP);
}

TEST(stages_memory_extra_life_card_gives_armour_with_a_full_hangar)
{
    Grid(CARD_EXTRA_LIFE);
    P0.lives = LIVES_CAP;
    P0.armour = 123;
    int added = g_armourAddedCount;
    MatchPair();
    CHECK_EQ_INT(P0.lives, LIVES_CAP);
    CHECK_EQ_INT(P0.armour, 128);
    CHECK_EQ_INT(g_armourAddedCount, added + 5);
    CHECK_STR(g_alertMsg, "ARMOUR");
    CHECK_EQ_INT(g_msgColor, 1);
}

TEST(stages_memory_extra_life_card_pays_a_million_when_maxed)
{
    Grid(CARD_EXTRA_LIFE);
    P0.lives = LIVES_CAP;
    P0.armour = ARMOUR_CAP;
    MatchPair();
    CHECK_EQ_INT(P0.armour, ARMOUR_CAP);
    CHECK_EQ_INT(P0.score, 1000000);
}

TEST(stages_memory_mark_cards_set_their_mark)
{
    static const int cards[6] = {CARD_MARK_6, CARD_MARK_5, CARD_MARK_4,
                                 CARD_MARK_3, CARD_MARK_2, CARD_MARK_1};
    static const int bits[6] = {MARK_6, MARK_5, MARK_4, MARK_3, MARK_2, MARK_1};
    for (int k = 0; k < 6; k++) {
        Grid(cards[k]);
        P0.marks = 0;
        MatchPair();
        CHECK_MSG((short)P0.marks == bits[k], "card %d: marks %d", cards[k], (short)P0.marks);
        CHECK_EQ_INT(P0.score, 5000);
    }
}

TEST(stages_memory_extra_bullet_card)
{
    Grid(CARD_EXTRA_BULLET);
    P0.bullets = 12;
    MatchPair();
    CHECK_EQ_INT(P0.bullets, 13);
    CHECK_EQ_INT(P0.score, 0);
}

TEST(stages_memory_extra_bullet_card_when_full_pays_score)
{
    Grid(CARD_EXTRA_BULLET);
    P0.bullets = 50;
    MatchPair();
    CHECK_EQ_INT(P0.bullets, 50);
    CHECK_EQ_INT(P0.score, 25000);
}

TEST(stages_memory_extra_speed_card)
{
    Grid(CARD_EXTRA_SPEED);
    P0.speed = g_speedBase;
    MatchPair();
    CHECK_NEAR(P0.speed, g_speedBase + g_speedStep, 1e-4);
    CHECK_EQ_INT(P0.score, 0);
}

TEST(stages_memory_extra_speed_card_at_top_speed_pays_score)
{
    Grid(CARD_EXTRA_SPEED);
    float top = g_speedStep * g_maxSpeedMul + g_speedBase;
    P0.speed = top;
    MatchPair();
    CHECK_NEAR(P0.speed, top, 1e-4);
    CHECK_EQ_INT(P0.score, 25000);
}

TEST(stages_memory_extra_buff_time_card)
{
    Grid(CARD_EXTRA_BUFF_TIME);
    P0.buffDuration = 20;
    MatchPair();
    CHECK_EQ_INT(P0.buffDuration, 25);
    CHECK_EQ_INT(P0.score, 0);
}

TEST(stages_memory_extra_buff_time_card_at_maximum_pays_score)
{
    Grid(CARD_EXTRA_BUFF_TIME);
    P0.buffDuration = g_timeMax + 3;
    MatchPair();
    CHECK_EQ_INT(P0.buffDuration, g_timeMax);
    CHECK_EQ_INT(P0.score, 25000);
}

TEST(stages_memory_gem_card_adds_two_gems)
{
    Grid(CARD_GEM_C);
    P0.gems = GEM_BASE + GEM_STEP * 10;
    MatchPair();
    CHECK_EQ_INT(P0.gems, GEM_BASE + GEM_STEP * 12);
    CHECK_EQ_INT(P0.score, 500);
    CHECK_EQ_INT(g_state, STATE_MEMORY_STATION);
}

TEST(stages_memory_hundredth_gem_starts_a_gem_drop)
{
    Grid(CARD_GEM_A);
    P0.gems = GEM_BASE + GEM_STEP * 98;
    g_superGemDrop = 0;
    MatchPair();
    CHECK_EQ_INT(g_state, STATE_GEM_DROP);
    CHECK_EQ_INT(P0.gems, GEM_BASE + GEM_STEP * 100);
    CHECK_EQ_INT(P0.score, 0);
    CHECK_EQ_INT(g_superGemDrop, 0);
    CHECK_STR(g_alertMsg, "G E M   D R O P");
    CHECK_EQ_INT(g_gemDropIntroTimer, g_time + 4000);
}

TEST(stages_memory_thousandth_gem_starts_a_super_gem_drop)
{
    Grid(CARD_GEM_G);
    P0.gems = GEM_BASE + GEM_STEP * 998;
    MatchPair();
    CHECK_EQ_INT(g_state, STATE_GEM_DROP);
    CHECK_EQ_INT(g_superGemDrop, 1);
    CHECK_EQ_INT(P0.gems, GEM_BASE);
    CHECK_STR(g_alertMsg, "S U P E R   G E M   D R O P");
}

TEST(stages_memory_letter_card_sets_its_letter)
{
    Grid(CARD_LETTER_R);
    P0.extraLetterE = P0.extraLetterX = P0.extraLetterT = P0.extraLetterR = P0.extraLetterA = 0;
    MatchPair();
    CHECK_EQ_INT(P0.extraLetterR, 1);
    CHECK_EQ_INT(P0.extraLetterE + P0.extraLetterX + P0.extraLetterT + P0.extraLetterA, 0);
    CHECK_EQ_INT(P0.score, 0);
}

TEST(stages_memory_duplicate_letter_pays_100)
{
    Grid(CARD_LETTER_X);
    P0.extraLetterE = P0.extraLetterT = P0.extraLetterR = P0.extraLetterA = 0;
    P0.extraLetterX = 1;
    MatchPair();
    CHECK_EQ_INT(P0.extraLetterX, 1);
    CHECK_EQ_INT(P0.score, 100);
}

TEST(stages_memory_spelling_EXTRA_gives_a_life)
{
    static const int cards[5] = {CARD_LETTER_E, CARD_LETTER_X, CARD_LETTER_T, CARD_LETTER_R,
                                 CARD_LETTER_A};
    for (int missing = 0; missing < 5; missing++) {
        Grid(cards[missing]);
        P0.extraLetterE = missing != 0;
        P0.extraLetterX = missing != 1;
        P0.extraLetterT = missing != 2;
        P0.extraLetterR = missing != 3;
        P0.extraLetterA = missing != 4;
        P0.lives = 30;
        MatchPair();
        CHECK_MSG(P0.lives == 34, "missing letter %d: lives %d", missing, P0.lives);
        CHECK_EQ_INT(P0.extraLetterE + P0.extraLetterX + P0.extraLetterT + P0.extraLetterR +
                         P0.extraLetterA, 0);
        CHECK_EQ_INT(FakePlayCount("fanfare"), missing + 1);
    }
}

TEST(stages_memory_spelling_EXTRA_requires_A)
{
    Grid(CARD_LETTER_E);
    P0.extraLetterE = 0;
    P0.extraLetterX = P0.extraLetterT = P0.extraLetterR = 1;
    P0.extraLetterA = 0;
    P0.lives = 30;
    MatchPair();
    CHECK_EQ_INT(P0.lives, 30);
    CHECK_EQ_INT(P0.extraLetterE, 1);
    CHECK_EQ_INT(P0.extraLetterX, 1);
    CHECK_EQ_INT(P0.extraLetterT, 1);
    CHECK_EQ_INT(P0.extraLetterR, 1);
    CHECK_EQ_INT(P0.extraLetterA, 0);
    CHECK_EQ_INT(FakePlayCount("fanfare"), 0);
}

TEST(stages_memory_spelling_EXTRA_with_full_hangar_gives_armour_or_score)
{
    Grid(CARD_LETTER_A);
    P0.extraLetterE = P0.extraLetterX = P0.extraLetterT = P0.extraLetterR = 1;
    P0.extraLetterA = 0;
    P0.lives = LIVES_CAP;
    P0.armour = ARMOUR_CAP - 5;
    MatchPair();
    CHECK_EQ_INT(P0.armour, ARMOUR_CAP);
    CHECK_EQ_INT(P0.score, 0);
    Grid(CARD_LETTER_A);
    P0.extraLetterE = P0.extraLetterX = P0.extraLetterT = P0.extraLetterR = 1;
    P0.extraLetterA = 0;
    P0.lives = LIVES_CAP;
    P0.armour = ARMOUR_CAP;
    MatchPair();
    CHECK_EQ_INT(P0.score, 1000000);
    CHECK_EQ_INT(P0.extraLetterE, 0);
}

TEST(stages_memory_secret_bird_card_counts_hits)
{
    Grid(CARD_SCORE_1000);
    g_cards[3][3].type = CARD_SECRET_BIRD;
    P0.secretBirdHits = 3;
    P0.secretBirdCounter = 25;
    g_secretBirdHitCount = 0;
    Pick(3, 3);
    CHECK_EQ_INT(g_memPendingCardType, CARD_SECRET_BIRD);
    Settle();
    CHECK_EQ_INT(P0.secretBirdHits, 4);
    CHECK_EQ_INT(P0.secretBirdCounter, 24);
    CHECK_EQ_INT(P0.secretBirdTick, 3);
    CHECK_EQ_INT(g_secretBirdHitCount, 1);
    CHECK_EQ_INT(FakePlayCount("guit"), 1);
}

TEST(stages_memory_secret_bird_counter_floor_is_20)
{
    Grid(CARD_SCORE_1000);
    g_cards[3][3].type = CARD_SECRET_BIRD;
    P0.secretBirdHits = 3;
    P0.secretBirdCounter = 20;
    Pick(3, 3);
    Settle();
    CHECK_EQ_INT(P0.secretBirdCounter, 20);
}

TEST(stages_memory_tenth_secret_bird_raises_the_money_cap)
{
    Grid(CARD_SCORE_1000);
    g_cards[3][3].type = CARD_SECRET_BIRD;
    P0.secretBirdHits = 9;
    P0.secretBirdCounter = 25;
    P0.moneyMax = 99990;
    Pick(3, 3);
    Settle();
    CHECK_EQ_INT(P0.secretBirdHits, 10);
    CHECK_EQ_INT(P0.secretBirdCounter, 0);
    CHECK_EQ_INT(P0.moneyMax, 999990);
    CHECK_EQ_INT(FakePlayCount("kanganang"), 1);
}

TEST(stages_memory_secret_bird_after_ten_does_nothing)
{
    Grid(CARD_SCORE_1000);
    g_cards[3][3].type = CARD_SECRET_BIRD;
    P0.secretBirdHits = 10;
    P0.secretBirdCounter = 25;
    Pick(3, 3);
    Settle();
    CHECK_EQ_INT(P0.secretBirdHits, 10);
    CHECK_EQ_INT(P0.secretBirdCounter, 25);
}

// ---- clearing the grid ----

TEST(stages_memory_last_pair_sets_up_the_bonus)
{
    Grid(CARD_SCORE_100);
    MatchPair();
    CHECK_EQ_INT(g_memoryBonusPending, 1);
    CHECK_EQ_INT(g_memoryStationDeadline, g_time + 3000);
    CHECK_EQ_INT(FakePlayCount("harpgliss1"), 1);
}

TEST(stages_memory_pairs_left_means_no_bonus_yet)
{
    Grid(CARD_SCORE_100);
    g_cards[3][3].type = g_cards[2][3].type;    // another pair
    MatchPair();
    CHECK_EQ_INT(g_memoryBonusPending, 0);
}

TEST(stages_memory_FinishMemoryStation_waits_for_its_deadline)
{
    MemSetUp();
    g_memoryStationDeadline = g_time;
    g_memoryBonusPending = 1;
    FinishMemoryStation();
    CHECK_EQ_INT(g_memoryDone, 1);
    CHECK_EQ_INT(g_memoryStationExitFlag, 0);
    CHECK_EQ_INT(g_memoryBonusPending, 1);
    CHECK_EQ_INT(g_state, STATE_MEMORY_STATION);
}

TEST(stages_memory_FinishMemoryStation_pays_the_bonus_and_leaves)
{
    MemSetUp();
    g_memoryStationDeadline = g_time - 1;
    g_memoryBonusPending = 1;
    P0.memoryBonus = 25000;
    P0.memoryGridUpgradeStreak = 0;
    P0.done = 0;
    P0.doneTime = 0;
    P0.totalEnemies = 12;
    P0.savedStarSpeed = 21;
    FinishMemoryStation();
    CHECK_EQ_INT(P0.memoryBonus, 50000);
    CHECK_EQ_INT(P0.score, 50000);
    CHECK_EQ_INT(g_memoryBonusPending, 0);
    CHECK_EQ_INT(g_memoryDone, 0);
    CHECK_EQ_INT(g_memoryStationExitFlag, 1);
    CHECK_EQ_INT(g_memoryStationDeadline, 0);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(P0.done, 1);
    CHECK_EQ_INT(P0.doneTime, g_time + 3000);
    CHECK_EQ_INT(P0.killed, 12);
    CHECK_NEAR(P0.starSpeed, 21, 1e-6);
    CHECK_EQ_INT(P0.memoryGridUpgradeStreak, 1);
    CHECK_EQ_INT(P0.rows, 4);
}

TEST(stages_memory_every_second_clear_grows_the_grid)
{
    MemSetUp();
    g_memoryStationDeadline = g_time - 1;
    g_memoryBonusPending = 1;
    P0.memoryGridUpgradeStreak = 1;
    FinishMemoryStation();
    CHECK_EQ_INT(P0.memoryGridUpgradeStreak, 0);
    CHECK_EQ_INT(P0.rows, 5);
    CHECK_EQ_INT(P0.cols, 5);
}

TEST(stages_memory_grid_stops_growing_at_8)
{
    MemSetUp();
    g_memoryStationDeadline = g_time - 1;
    g_memoryBonusPending = 1;
    P0.memoryGridUpgradeStreak = 1;
    P0.rows = 8;
    P0.cols = 8;
    FinishMemoryStation();
    CHECK_EQ_INT(P0.rows, 8);
    CHECK_EQ_INT(P0.cols, 8);
}

TEST(stages_memory_FinishMemoryStation_without_bonus_stays)
{
    MemSetUp();
    g_memoryStationDeadline = g_time - 1;
    g_memoryBonusPending = 0;
    FinishMemoryStation();
    CHECK_EQ_INT(g_memoryStationExitFlag, 1);
    CHECK_EQ_INT(g_state, STATE_MEMORY_STATION);
    CHECK_EQ_INT(P0.score, 0);
}

// ---- time ----

TEST(stages_memory_timeout_ends_the_stage)
{
    Grid(CARD_SCORE_100);
    g_memStageDeadline = g_time - 1;
    P0.done = 0;
    P0.doneTime = 0;
    P0.totalEnemies = 8;
    P0.savedScrollSpeedY = 3;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(P0.done, 1);
    CHECK_EQ_INT(P0.doneTime, g_time + 3000);
    CHECK_EQ_INT(P0.killed, 8);
    CHECK_NEAR(P0.scrollSpeedY, 3, 1e-6);
    CHECK_EQ_INT(g_transitionLock, 1);
}

TEST(stages_memory_no_timeout_on_the_deadline)
{
    Grid(CARD_SCORE_100);
    g_memStageDeadline = g_time;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_state, STATE_MEMORY_STATION);
}

TEST(stages_memory_countdown_voices)
{
    Grid(CARD_SCORE_100);
    g_memCountdownStage = 11;
    g_memStageDeadline = g_time + 10500;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memCountdownStage, 10);
    CHECK(Queued(g_sfxVoiceTen));
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memCountdownStage, 10);
    g_memStageDeadline = g_time + 9500;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memCountdownStage, 9);
    g_memStageDeadline = g_time + 9000;      // the boundary: not yet eight
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memCountdownStage, 9);
    g_memStageDeadline = g_time + 8999;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memCountdownStage, 8);
    g_memCountdownStage = 4;
    g_soundQueueCount = 0;      // (a voice is only queued into an empty queue)
    g_soundQueueNext = 0;
    g_memStageDeadline = g_time + 3500;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memCountdownStage, 3);
    CHECK(Queued(g_sfxVoiceThree));
    g_memCountdownStage = 1;
    g_memStageDeadline = g_time + 500;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memCountdownStage, 11);
}

TEST(stages_memory_rocket_kills_time_for_score)
{
    Grid(CARD_SCORE_100);
    g_memStageDeadline = g_time + 5000;
    g_memCountdownStage = 0;
    g_rocketRepeatTimer = 0;
    g_rightButton = 1;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memStageDeadline, g_time + 4000);
    CHECK_EQ_INT(g_rocketRepeatTimer, g_time + 25);
    CHECK(P0.score >= 100 && P0.score <= 1000 && P0.score % 100 == 0);
    long long s = P0.score;
    MemoryBonusUpdate();        // repeat delay
    CHECK_EQ_INT(g_memStageDeadline, g_time + 4000);
    CHECK_EQ_INT(P0.score, s);
    g_time += 26;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memStageDeadline, g_time - 26 + 3000);
}

// The clamp meant to stop killing time at "now" (deadline - g_time < 0) never fires: both
// are unsigned. The deadline lands in the past and the stage ends in the same update.
TEST(stages_memory_rocket_can_push_the_deadline_into_the_past)
{
    Grid(CARD_SCORE_100);
    g_memStageDeadline = g_time + 300;
    g_memCountdownStage = 0;
    g_rocketRepeatTimer = 0;
    g_rightButton = 1;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memStageDeadline, g_time - 700);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
}

TEST(stages_memory_rocket_does_nothing_on_the_deadline)
{
    Grid(CARD_SCORE_100);
    g_memStageDeadline = g_time;
    g_memCountdownStage = 0;
    g_rocketRepeatTimer = 0;
    g_rightButton = 1;
    MemoryBonusUpdate();
    CHECK_EQ_INT(g_memStageDeadline, g_time);
    CHECK_EQ_INT(P0.score, 0);
}

// ---- drawing, the intro gate, the cursor ----

TEST(stages_memory_intro_gate)
{
    MemSetUp();
    g_memoryIntroTimer = g_time + 1;
    UpdateMemoryStationIntro();
    CHECK_EQ_INT(g_memoryIntro, 1);
    CHECK_EQ_INT(g_introGateScratch, 0);
    g_memoryIntroTimer = g_time;
    UpdateMemoryStationIntro();
    CHECK_EQ_INT(g_memoryIntro, 0);
    CHECK_EQ_INT(g_introGateScratch, 1);
}

TEST(stages_memory_grid_shows_pairs_and_tries)
{
    Grid(CARD_SCORE_100);
    P0.tries = 7;
    g_pairsTickTime = 0;
    g_memStageDeadline = g_time + 4500;
    DrawMemoryGrid();
    CHECK_STR(g_logBuf, "PAIR LEFT :1   TRIES :7");
    CHECK_EQ_INT(g_pairsTickTime, g_time + 1000);
    CHECK_EQ_INT(FakePlayCount("bing"), 1);
    DrawMemoryGrid();
    CHECK_EQ_INT(FakePlayCount("bing"), 1);
}

TEST(stages_memory_cursor_blinks_through_six_frames)
{
    MemSetUp();
    g_memCursorFrame = 4;
    g_gridAnimTime = 0;
    DrawMemoryCursor();
    CHECK_EQ_INT(g_memCursorFrame, 5);
    CHECK_EQ_INT(g_gridAnimTime, g_time + 50);
    DrawMemoryCursor();
    CHECK_EQ_INT(g_memCursorFrame, 5);
    g_time += 51;
    DrawMemoryCursor();
    CHECK_EQ_INT(g_memCursorFrame, 0);
}

TEST(stages_memory_congratulations_show_the_bonus)
{
    MemSetUp();
    g_memoryIntro = 0;
    g_memoryDone = 1;
    g_buttonsOn = 1;
    P0.memoryBonus = 7500000000LL;
    MemoryStationText();
    CHECK_STR(g_logBuf, "7500000000 POINTS BONUS");
    CHECK_EQ_INT(g_buttonsOn, 0);
}
