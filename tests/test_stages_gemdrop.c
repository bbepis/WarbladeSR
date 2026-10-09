// Tests for src/game/stages/gemdrop.c: the Gem Drop bonus stage (falling gems: spawning,
// moving, catching, scoring, and the end of the stage).
#include "support.h"

#define P0 g_save.players[0]

static void GemSetUp(void)
{
    BootGame();
    SeedRand(4321);
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_scoreMul[0] = 1;
    g_scoreMul[1] = 1;
    g_frameDt = 1;
    g_time = 50000;
    g_cfg.collisionDetail = 0;      // box collisions only
    g_superGemDrop = 0;
    InitGemDropLevel();
    g_state = STATE_GEM_DROP;
}

// Puts an active gem of `type` (0, 80, 160) at (x, y) in slot `i`.
static void PlaceGem(int i, int type, float x, float y)
{
    g_fallingGems[i].active = 1;
    g_fallingGems[i].type = type;
    g_fallingGems[i].x = x;
    g_fallingGems[i].y = y;
    g_fallingGems[i].w = 0x50;
    g_fallingGems[i].h = 0x33;
    g_fallingGems[i].vy = 10;
    g_pickupCount++;
}

static int ActiveGems(void)
{
    int n = 0;
    for (int i = 0; i < MAX_FALLING_GEMS; i++)
        n += g_fallingGems[i].active != 0;
    return n;
}

// ---- the intro gate ----

TEST(stages_gem_intro_gate_is_active_until_its_timer)
{
    GemSetUp();
    g_gemDropIntroTimer = g_time + 1;
    UpdateGemDropIntroGate();
    CHECK_EQ_INT(g_gemDropIntroActive, 1);
    CHECK_EQ_INT(g_introGateScratch, 0);
    g_gemDropIntroTimer = g_time;
    UpdateGemDropIntroGate();
    CHECK_EQ_INT(g_gemDropIntroActive, 0);
    CHECK_EQ_INT(g_introGateScratch, 1);
}

// ---- InitGemDropLevel ----

TEST(stages_gem_init_resets_the_stage)
{
    BootGame();
    SeedRand(99);
    for (int i = 0; i < MAX_FALLING_GEMS; i++)
        g_fallingGems[i].active = 1;
    g_pickupCount = 7;
    g_maxFallingGems = 5;
    g_save.players[0].energy = 3;
    g_save.players[3].energy = 3;
    InitGemDropLevel();
    CHECK_NEAR(g_levelDist, 2680, 1e-3);
    CHECK_NEAR(g_maxFallingGems, 1, 1e-6);
    CHECK_EQ_INT(g_pickupCount, 0);
    CHECK_EQ_INT(g_save.players[0].energy, 0);
    CHECK_EQ_INT(g_save.players[3].energy, 0);
    for (int i = 0; i < MAX_FALLING_GEMS; i++) {
        CHECK_EQ_INT(g_fallingGems[i].active, 0);
        CHECK_EQ_INT(g_fallingGems[i].h, 0x33);
        CHECK_EQ_INT(g_fallingGems[i].w, 0x50);
        CHECK_EQ_INT(g_fallingGems[i].sy, 0);
        CHECK(g_fallingGems[i].type == 0 || g_fallingGems[i].type == 0x50 ||
              g_fallingGems[i].type == 0xa0);
        CHECK(g_fallingGems[i].frame >= 0 && g_fallingGems[i].frame <= 10);
        CHECK(g_fallingGems[i].animDelay >= 3 && g_fallingGems[i].animDelay <= 5);
        CHECK_NEAR(g_fallingGems[i].animTimer, g_fallingGems[i].animDelay, 1e-6);
    }
}

// ---- GemDropUpdate ----

TEST(stages_gem_update_spawns_a_gem_above_the_screen)
{
    GemSetUp();
    g_maxFallingGemsInc = 0;
    GemDropUpdate();
    CHECK_EQ_INT(g_pickupCount, 1);
    CHECK_EQ_INT(ActiveGems(), 1);
    FallingSprite *g = &g_fallingGems[0];
    CHECK_EQ_INT(g->active, 1);
    CHECK(g->vy >= 7 && g->vy <= 14);
    CHECK_NEAR(g->y, -60 + g->vy, 1e-3);       // spawned at -60, then moved once
    CHECK(g->x >= 70 && g->x <= 650);
    CHECK(g->hma == g_hmaDiamondBig);
    CHECK_EQ_INT(g->hmaW, g_diamondBigGfxW);
    CHECK(g->animDelay >= 1 && g->animDelay <= 3);
    // At the cap (1 gem): no more.
    GemDropUpdate();
    CHECK_EQ_INT(g_pickupCount, 1);
    CHECK_EQ_INT(ActiveGems(), 1);
}

TEST(stages_gem_update_raises_the_cap_over_time)
{
    GemSetUp();
    g_maxFallingGemsInc = 0.5f;
    GemDropUpdate();    // cap 1.5: one gem
    CHECK_EQ_INT(g_pickupCount, 1);
    CHECK_NEAR(g_maxFallingGems, 1.5, 1e-6);
    GemDropUpdate();    // cap 2.0: a second
    CHECK_EQ_INT(g_pickupCount, 2);
    CHECK_EQ_INT(ActiveGems(), 2);
}

TEST(stages_gem_update_spawns_the_three_gem_kinds_by_weight)
{
    GemSetUp();
    int count[3] = {0, 0, 0}, n = 4000;
    for (int k = 0; k < n; k++) {
        g_fallingGems[0].active = 0;
        g_pickupCount = 0;
        g_maxFallingGems = 1;
        g_maxFallingGemsInc = 0;
        g_levelDist = 1000;
        GemDropUpdate();
        int t = g_fallingGems[0].type;
        CHECK(t == 0 || t == 0x50 || t == 0xa0);
        CHECK(g_fallingGems[0].x >= 70 && g_fallingGems[0].x <= 650);
        count[t / 0x50]++;
    }
    // RandRange(0, 100): 0..50 small, 51..84 medium, 85..99 big.
    CHECK_NEAR(count[0] / (double)n, 0.51, 0.03);
    CHECK_NEAR(count[1] / (double)n, 0.34, 0.03);
    CHECK_NEAR(count[2] / (double)n, 0.15, 0.03);
}

TEST(stages_gem_update_moves_and_animates_gems)
{
    GemSetUp();
    g_maxFallingGems = 0;
    g_maxFallingGemsInc = 0;
    PlaceGem(3, 0, 200, 100);
    g_fallingGems[3].vy = 8;
    g_fallingGems[3].frame = 10;
    g_fallingGems[3].animDelay = 1;
    g_fallingGems[3].animTimer = 0.5f;
    g_frameDt = 2;
    GemDropUpdate();
    CHECK_NEAR(g_fallingGems[3].y, 116, 1e-4);
    CHECK_NEAR(g_fallingGems[3].animTimer, 1, 1e-6);   // went below 0: reloaded
    CHECK_EQ_INT(g_fallingGems[3].frame, 0);            // wrapped past 10
    GemDropUpdate();
    CHECK_NEAR(g_fallingGems[3].animTimer, 0, 1e-6);
    CHECK_EQ_INT(g_fallingGems[3].frame, 0);
    GemDropUpdate();
    CHECK_EQ_INT(g_fallingGems[3].frame, 1);
}

TEST(stages_gem_update_replaces_a_gem_that_fell_off)
{
    GemSetUp();
    g_maxFallingGems = 1;
    g_maxFallingGemsInc = 0;
    PlaceGem(4, 0, 200, 600 + 0x33 + 5 - 5);     // one 10px step from falling off
    GemDropUpdate();
    // Slot 4 dropped out, a replacement went into the first free slot.
    CHECK_EQ_INT(g_fallingGems[4].active, 0);
    CHECK_EQ_INT(g_fallingGems[0].active, 1);
    CHECK_EQ_INT(g_pickupCount, 1);
    CHECK_NEAR(g_fallingGems[0].y, -60, 1e-4);
    CHECK(g_fallingGems[0].vy >= 6 && g_fallingGems[0].vy <= 10);
}

// Replacing a fallen gem must not rewind the movement loop.
TEST(stages_gem_update_respawn_moves_each_existing_gem_once)
{
    GemSetUp();
    g_maxFallingGems = 3;
    g_maxFallingGemsInc = 0;
    PlaceGem(1, 0, 100, 100);
    g_fallingGems[1].vy = 5;
    g_fallingGems[1].animDelay = g_fallingGems[1].animTimer = 50;
    PlaceGem(4, 0, 200, 650);
    PlaceGem(6, 0, 300, 100);
    g_fallingGems[6].vy = 5;
    g_fallingGems[6].animDelay = g_fallingGems[6].animTimer = 50;
    GemDropUpdate();
    CHECK_EQ_INT(g_fallingGems[0].active, 1);
    CHECK_NEAR(g_fallingGems[0].y, -60, 1e-4);
    CHECK_NEAR(g_fallingGems[1].animTimer, 49, 1e-4);
    CHECK_NEAR(g_fallingGems[6].animTimer, 49, 1e-4);
    CHECK_NEAR(g_fallingGems[1].y, 105, 1e-4);     // moved once
    CHECK_NEAR(g_fallingGems[6].y, 105, 1e-4);     // moved once
}

TEST(stages_gem_update_keeps_a_gem_on_the_bottom_edge)
{
    GemSetUp();
    g_maxFallingGems = 1;
    g_maxFallingGemsInc = 0;
    PlaceGem(4, 0, 200, 600 + 0x33 + 5 - 10);    // lands exactly on the limit
    GemDropUpdate();
    CHECK_EQ_INT(g_fallingGems[4].active, 1);
    CHECK_EQ_INT(g_pickupCount, 1);
}

TEST(stages_gem_update_counts_down_the_stage)
{
    GemSetUp();
    g_maxFallingGemsInc = 0;
    g_frameDt = 3;
    GemDropUpdate();
    CHECK_NEAR(g_levelDist, 2677, 1e-3);
    CHECK_EQ_INT(g_state, STATE_GEM_DROP);
}

TEST(stages_gem_update_ends_the_stage_when_the_distance_runs_out)
{
    GemSetUp();
    g_maxFallingGemsInc = 0;
    g_levelDist = 0.5f;
    g_superGemDrop = 1;
    P0.done = 0;
    P0.doneTime = 0;
    P0.totalEnemies = 17;
    P0.killed = 3;
    P0.escaped = 4;
    g_transitionLock = 0;
    GemDropUpdate();
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(g_superGemDrop, 0);
    CHECK_EQ_INT(P0.done, 1);
    CHECK_EQ_INT(P0.doneTime, g_time + 3000);
    CHECK_EQ_INT(P0.killed, 17);
    CHECK_EQ_INT(P0.escaped, 0);
    CHECK_EQ_INT(g_transitionLock, 1);
    CHECK_EQ_INT(g_transitionLockUntil, g_time + 500);
    CHECK_EQ_INT(g_flashOverlayActive, 1);
}

TEST(stages_gem_update_keeps_an_earlier_done_time)
{
    GemSetUp();
    g_levelDist = 0.5f;
    P0.done = 0;
    P0.doneTime = 1234;
    GemDropUpdate();
    CHECK_EQ_INT(P0.done, 1);
    CHECK_EQ_INT(P0.doneTime, 1234);
}

TEST(stages_gem_update_spawns_nothing_after_the_stage)
{
    GemSetUp();
    g_maxFallingGemsInc = 0;
    g_levelDist = 0;
    GemDropUpdate();
    CHECK_EQ_INT(g_pickupCount, 0);
    CHECK_EQ_INT(ActiveGems(), 0);
}

// ---- GemDropCollide ----

static long long CatchScore(int type, int super)
{
    GemSetUp();
    g_superGemDrop = super;
    P0.x = 200;
    P0.y = 400;
    P0.mirrorTime = 0;
    P0.score = 0;
    PlaceGem(2, type, 180, 390);
    int jingles = FakePlayCount("jingles");
    GemDropCollide(0);
    CHECK_EQ_INT(g_fallingGems[2].active, 0);
    CHECK_EQ_INT(g_pickupCount, 0);
    CHECK_EQ_INT(FakePlayCount("jingles"), jingles + 1);
    return P0.score;
}

TEST(stages_gem_catching_a_small_gem_scores_50000)
{
    CHECK_EQ_INT(CatchScore(0, 0), 50000);
}

TEST(stages_gem_catching_a_medium_gem_scores_100000)
{
    CHECK_EQ_INT(CatchScore(0x50, 0), 100000);
}

TEST(stages_gem_catching_a_big_gem_scores_500000)
{
    CHECK_EQ_INT(CatchScore(0xa0, 0), 500000);
}

TEST(stages_gem_super_drop_small_gem_scores_a_million)
{
    CHECK_EQ_INT(CatchScore(0, 1), 1000000);
}

TEST(stages_gem_super_drop_medium_gem_scores_5_million)
{
    CHECK_EQ_INT(CatchScore(0x50, 1), 5000000);
}

TEST(stages_gem_super_drop_big_gem_scores_10_million)
{
    CHECK_EQ_INT(CatchScore(0xa0, 1), 10000000);
}

TEST(stages_gem_catch_uses_the_score_multiplier_and_shows_a_popup)
{
    GemSetUp();
    g_scoreMul[0] = 2;
    P0.x = 200;
    P0.y = 400;
    P0.mirrorTime = 0;
    P0.score = 0;
    for (int j = 0; j < MAX_POPUPS; j++)
        g_popups[j].active = 0;
    PlaceGem(2, 0x50, 180, 390);
    GemDropCollide(0);
    CHECK_EQ_INT(P0.score, 200000);
    int popups = 0;
    for (int j = 0; j < MAX_POPUPS; j++)
        popups += g_popups[j].active != 0;
    CHECK_EQ_INT(popups, 1);
}

TEST(stages_gem_catch_needs_an_overlap)
{
    GemSetUp();
    P0.x = 200;
    P0.y = 400;
    P0.mirrorTime = 0;
    P0.score = 0;
    // Ship box 200..240 x 400..427; gems touching it edge to edge don't count.
    PlaceGem(0, 0, 240, 400);           // right of it
    PlaceGem(1, 0, 200 - 0x50, 400);    // left of it
    PlaceGem(2, 0, 200, 427);           // below it
    PlaceGem(3, 0, 200, 400 - 0x33);    // above it
    GemDropCollide(0);
    CHECK_EQ_INT(ActiveGems(), 4);
    CHECK_EQ_INT(P0.score, 0);
    PlaceGem(4, 0, 239, 426);           // one pixel in
    GemDropCollide(0);
    CHECK_EQ_INT(ActiveGems(), 4);
    CHECK_EQ_INT(g_fallingGems[4].active, 0);
    CHECK_EQ_INT(P0.score, 50000);
}

TEST(stages_gem_catch_by_the_second_player)
{
    GemSetUp();
    g_scoreMul[1] = 1;
    g_save.players[1].x = 500;
    g_save.players[1].y = 400;
    g_save.players[1].mirrorTime = 0;
    g_save.players[1].score = 0;
    P0.score = 0;
    PlaceGem(0, 0xa0, 480, 390);
    GemDropCollide(1);
    CHECK_EQ_INT(g_save.players[1].score, 500000);
    CHECK_EQ_INT(P0.score, 0);
}

// With the mirror effect on, about half the time the ship counts as being at its mirrored
// position (screen width - 40 - x).
TEST(stages_gem_mirror_effect_sometimes_mirrors_the_ship)
{
    GemSetUp();
    P0.x = 100;
    P0.y = 400;
    P0.mirrorTime = 100;
    int hits = 0;
    for (int k = 0; k < 400; k++) {
        g_fallingGems[0].active = 0;
        g_pickupCount = 0;
        PlaceGem(0, 0, 800 - 40 - 100 + 39, 390);   // over the mirrored ship's right edge only
        GemDropCollide(0);
        hits += g_fallingGems[0].active == 0;
    }
    CHECK(hits > 140 && hits < 260);
}

// ---- DrawFallingGems ----

TEST(stages_gem_draw_records_the_hit_rect_and_frame_row)
{
    GemSetUp();
    PlaceGem(5, 0x50, 300, 200);
    g_fallingGems[5].frame = 3;
    g_fallingGems[5].sy = 51 * 2;
    DrawFallingGems();
    CHECK_EQ_INT(g_fallingGems[5].r1, 0x50);
    CHECK_EQ_INT(g_fallingGems[5].r2, 51 * 2);
    CHECK_EQ_INT(g_fallingGems[5].r3, 0x50 + 0x50);
    CHECK_EQ_INT(g_fallingGems[5].r4, 51 * 2 + 0x33);
    CHECK_EQ_INT(g_fallingGems[5].sy, 51 * 3);
}
