// test_flow_game.c: Game flow (src/game/gameflow.c, levelstart.c): starting a game from the
// title, the get-ready intro, level complete -> next level, pause, game over, and the level
// start itself (spawning a level's enemies). The levels are synthesized from LevelRaw and
// added to the fake archive before the real start-up packs them.
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
    return L;
}

// Adds a sub (one alien) to group `g`, creating the group if needed.
static LvRawSub *AddSub(LevelRaw *L, int g, int xOffset, int yOffset, int hp)
{
    if (L->count < g + 1)
        L->count = g + 1;
    LvRawSub *s = &L->grp[g].sub[L->grp[g].count++];
    s->xOffset = xOffset;
    s->yOffset = yOffset;
    s->type = 1;
    s->hp = hp;
    return s;
}

static void PutClassic(int n, LevelRaw *L)
{
    char name[64];
    snprintf(name, sizeof name, "classic_level_%03d.lvd", n);
    FakePacAdd(name, L, sizeof *L);
    free(L);
}

// A wave level: one group of `n` aliens spawning at (0, 100) that fly a two-step path and
// then hold their formation slots.
static LevelRaw *WaveLevel(const char *name, int n)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, name);
    for (int i = 0; i < n; i++)
        AddSub(L, 0, -50 + 50 * i, 100, 10);
    L->grp[0].spawnY = 100;
    L->objCount[0] = 2;
    L->obj[0][0].holdTime = 10;
    L->obj[0][1].cmd = 1;
    L->obj[0][1].holdTime = 100;
    return L;
}

static bool NoWindow(void) { return !AnyWindowActive(); }
static bool Playing(void) { return g_state == STATE_PLAYING; }
static bool OnTitle(void) { return g_state == STATE_TITLE; }
static bool OnLevel2(void) { return g_save.players[0].level == 2; }

// Boots with the levels already added and closes the start-up message window.
static void BootToTitle(void)
{
    BootGame();
    RunFrames(2);
    TapKey(K_VK_ESCAPE, 2);
    CHECK(RunFramesUntil(NoWindow, 200));
}

static void PressF1(void)
{
    FakePressKey(K_VK_F1);
    RunFrames(1);
    FakeReleaseKey(K_VK_F1);
}

// Two wave levels, booted, a single-player game started and playing level 1.
static void PlayLevel1(void)
{
    PutClassic(1, WaveLevel("ALPHA", 2));
    PutClassic(2, WaveLevel("BETA", 3));
    BootToTitle();
    PressF1();
    CHECK(RunFramesUntil(Playing, 400));
}

static int MinEnergy(int p)
{
    return g_shipDefs[g_save.players[p].ship]->minEnergy;
}

// ---- end to end ----

TEST(Flow_F1_on_the_title_starts_a_single_player_game)
{
    PutClassic(1, WaveLevel("ALPHA", 2));
    BootToTitle();
    CHECK_EQ_INT(g_state, STATE_TITLE);
    int played = g_cfg.gamesPlayed;
    g_gameMode = MODE_DUAL;
    PressF1();
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK_EQ_INT(g_gameMode, MODE_SINGLE);
    CHECK_EQ_INT(g_curPlayer, 0);
    CHECK_EQ_INT(g_save.players[0].level, 1);
    CHECK_EQ_INT(g_cfg.gamesPlayed, played + 1);
    CHECK(g_drawHudFn == DrawHud1P);
    CHECK(g_playerUpdateFn == UpdatePlayer);
}

TEST(Flow_get_ready_shows_the_level_then_plays_it_after_2_seconds)
{
    PutClassic(1, WaveLevel("ALPHA", 2));
    BootToTitle();
    PressF1();
    RunFrames(2);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK_STR(g_getReadyText, "G E T   R E A D Y");
    CHECK_STR(g_levelBannerText, "LEVEL 1");
    CHECK_STR(g_levelName, "ALPHA");
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 2);
    // 2 s from the level start at 16 ms a frame
    RunFrames(110);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK(RunFramesUntil(Playing, 30));
}

TEST(Flow_the_first_level_spawns_its_aliens)
{
    PlayLevel1();
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 2);
    CHECK_EQ_INT(g_save.players[0].killed, 0);
    for (int i = 0; i < 2; i++) {
        Enemy *e = &g_enemies[0][i];
        CHECK_EQ_INT(e->active, 1);
        CHECK_EQ_INT(e->type, ENEMY_PATTERNED);
        CHECK_NEAR(e->x, 400 - 16, 0.01);
        CHECK_NEAR(e->y, 100 - 16, 0.01);
        CHECK_NEAR(e->hp, 10, 0.01);
        CHECK_NEAR(e->hoverX, 400 - 50 + 50 * i, 0.01);
    }
    CHECK_EQ_INT(g_enemies[0][2].active, 0);
}

TEST(Flow_aliens_fly_their_path_into_formation)
{
    PlayLevel1();
    // the path's second step (STOP_TURN, hold 100) turns them into hovering aliens
    for (int f = 0; f < 40 && g_enemies[0][0].type == ENEMY_PATTERNED; f++)
        RunFrames(1);
    CHECK_EQ_INT(g_enemies[0][0].type, ENEMY_HOVER);
    CHECK_EQ_INT(g_enemies[0][1].type, ENEMY_HOVER);
}

TEST(Flow_clearing_a_wave_level_starts_the_next_level_after_3_seconds)
{
    PlayLevel1();
    for (int i = 0; i < 2; i++) {
        g_enemies[0][i].active = 0;
        CreditKill(0);
    }
    CHECK_EQ_INT(g_save.players[0].done, 1);
    RunFrames(180);
    CHECK_EQ_INT(g_save.players[0].level, 1);
    CHECK(RunFramesUntil(OnLevel2, 20));
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK(RunFramesUntil(Playing, 400));
    CHECK_STR(g_levelBannerText, "LEVEL 2");
    CHECK_STR(g_levelName, "BETA");
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 3);
    CHECK_EQ_INT(g_save.players[0].killed, 0);
    CHECK_EQ_INT(g_save.players[0].done, 0);
}

static bool OneKilled(void) { return g_save.players[0].killed == 1; }

TEST(Flow_shooting_the_last_alien_scores_and_completes_the_level)
{
    LevelRaw *L = WaveLevel("ALPHA", 1);
    L->w[0] = 150;
    PutClassic(1, L);
    BootToTitle();
    PressF1();
    CHECK(RunFramesUntil(Playing, 400));
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 1);
    // hold the alien in a formation slot right above the ship
    Enemy *e = &g_enemies[0][0];
    e->type = ENEMY_HOVER;
    e->hoverX = e->x = g_save.players[0].x - 10;
    e->hoverY = e->y = 300;
    e->attackDelay = 0;
    CHECK_EQ_INT(e->score, 150);
    enum EKeyboardLayout fire = (enum EKeyboardLayout)g_cfg.fire[0];
    for (int i = 0; i < 300 && !OneKilled(); i++)
        TapKey(fire, 2);
    CHECK(OneKilled());
    CHECK_EQ_INT(e->active, 0);
    CHECK_EQ_INT(g_save.players[0].score, 150);
    CHECK(g_save.players[0].hits > 0);
    CHECK(g_save.players[0].shots >= g_save.players[0].hits);
    CHECK_EQ_INT(g_save.players[0].done, 1);
    CHECK(g_save.players[0].doneTime > g_time && g_save.players[0].doneTime <= g_time + 3000);
}

TEST(Flow_P_pauses_and_resumes_the_game)
{
    PlayLevel1();
    TapKey(K_VK_P, 2);
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    CHECK_EQ_INT(g_savedState, STATE_PLAYING);
    CHECK_EQ_INT(g_pauseCount, 1);
    TapKey(K_VK_P, 2);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(g_pauseCount, 1);
}

TEST(Flow_aliens_stand_still_while_paused)
{
    PlayLevel1();
    TapKey(K_VK_P, 1);
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    float x = g_enemies[0][0].x, y = g_enemies[0][0].y;
    RunFrames(30);
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    CHECK_NEAR(g_enemies[0][0].x, x, 0);
    CHECK_NEAR(g_enemies[0][0].y, y, 0);
}

TEST(Flow_escape_in_game_opens_the_quit_dialog_paused)
{
    PlayLevel1();
    FakePressKey(K_VK_ESCAPE);
    RunFrames(1);
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    CHECK_EQ_INT(g_quitGameWinOpen, 1);
    CHECK(AnyWindowActive());
    FakeReleaseKey(K_VK_ESCAPE);
    RunFrames(2);
    TapKey(K_VK_ESCAPE, 1);
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(g_quitGameWinOpen, 0);
}

TEST(Flow_running_out_of_lives_ends_the_game_and_returns_to_the_title)
{
    PlayLevel1();
    g_save.players[0].lives = MinEnergy(0);
    g_state = STATE_RESPAWN;     // as after losing a ship
    RunFrames(1);
    CHECK_EQ_INT(g_state, STATE_HISCORE_TABLE);
    CHECK_EQ_INT(g_tallyStep, 1);
    CHECK(RunFramesUntil(OnTitle, 700));
}

TEST(Flow_respawn_with_lives_left_is_not_game_over)
{
    PlayLevel1();
    g_save.players[0].lives = MinEnergy(0) + 1;
    g_state = STATE_RESPAWN;
    RunFrames(1);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
}

// ---- IsGameOver ----

static void BootAndNewGame(int mode)
{
    PutClassic(1, WaveLevel("ALPHA", 2));
    PutClassic(2, WaveLevel("BETA", 3));
    BootGame();
    g_gameMode = mode;
    NewGame(true);
}

TEST(Flow_IsGameOver_single_player_at_the_ships_minimum_energy)
{
    BootAndNewGame(MODE_SINGLE);
    g_save.players[0].lives = MinEnergy(0) + 1;
    CHECK_EQ_INT(IsGameOver(), 0);
    g_save.players[0].lives = MinEnergy(0);
    CHECK_EQ_INT(IsGameOver(), 1);
    g_gameMode = MODE_TIME_TRIAL;
    CHECK_EQ_INT(IsGameOver(), 1);
    g_save.players[0].lives = MinEnergy(0) + 1;
    CHECK_EQ_INT(IsGameOver(), 0);
}

TEST(Flow_IsGameOver_two_players_switches_to_the_one_still_alive)
{
    BootAndNewGame(MODE_TWO_PLAYER);
    CHECK_EQ_INT(g_curPlayer, 0);
    g_save.players[0].lives = MinEnergy(0);
    g_save.players[1].lives = MinEnergy(1) + 5;
    g_state = STATE_PLAYING;
    CHECK_EQ_INT(IsGameOver(), 0);
    CHECK_EQ_INT(g_curPlayer, 1);
    CHECK_EQ_INT(g_state, STATE_GET_READY);
    g_save.players[1].lives = MinEnergy(1);
    CHECK_EQ_INT(IsGameOver(), 1);
}

TEST(Flow_IsGameOver_dual_needs_both_players_out)
{
    BootAndNewGame(MODE_DUAL);
    g_save.players[0].lives = MinEnergy(0);
    g_save.players[1].lives = MinEnergy(1) + 1;
    CHECK_EQ_INT(IsGameOver(), 0);
    CHECK_EQ_INT(g_curPlayer, 0);
    g_save.players[1].lives = MinEnergy(1);
    CHECK_EQ_INT(IsGameOver(), 1);
    g_gameMode = MODE_TEAM;
    CHECK_EQ_INT(IsGameOver(), 0);
}

// ---- kill / escape credit ----

TEST(Flow_CreditKill_finishes_the_level_when_every_enemy_is_accounted_for)
{
    BootAndNewGame(MODE_SINGLE);
    g_time = 50000;
    g_save.players[0].totalEnemies = 3;
    g_save.players[0].escaped = 1;
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[0].killed, 1);
    CHECK_EQ_INT(g_save.players[0].done, 0);
    CHECK_EQ_INT(g_save.players[0].doneTime, 0);
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[0].killed, 2);
    CHECK_EQ_INT(g_save.players[0].done, 1);
    CHECK_EQ_INT(g_save.players[0].doneTime, 53000);
    g_time = 51000;
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[0].doneTime, 53000);
}

TEST(Flow_CreditKill_perfect_wave_clear_adds_10_rockets_up_to_50)
{
    BootAndNewGame(MODE_SINGLE);
    g_time = 50000;
    g_isWaveLevel = 1;
    g_isBossLevel = 0;
    g_save.players[0].totalEnemies = 1;
    g_save.players[0].rockets = 45;
    __int64 score = g_save.players[0].score;
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[0].rockets, 50);
    CHECK_STR(g_alertMsg, "10 ROCKETS ADDED");
    CHECK_EQ_INT(g_msgTimer, 51500);
    CHECK_EQ_INT(g_save.players[0].score, score);
    g_save.players[0].rockets = 12;
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[0].rockets, 22);
}

TEST(Flow_CreditKill_perfect_clear_with_full_rockets_scores_50000)
{
    BootAndNewGame(MODE_SINGLE);
    g_isWaveLevel = 1;
    g_isBossLevel = 0;
    g_scoreMul[0] = 2;
    g_save.players[0].totalEnemies = 1;
    g_save.players[0].rockets = 50;
    g_save.players[0].score = 1000;
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[0].rockets, 50);
    CHECK_EQ_INT(g_save.players[0].score, 1000 + 2 * 50000);
}

TEST(Flow_CreditKill_no_perfect_bonus_on_boss_levels)
{
    BootAndNewGame(MODE_SINGLE);
    g_isWaveLevel = 0;
    g_isBossLevel = 1;
    g_save.players[0].totalEnemies = 1;
    g_save.players[0].rockets = 5;
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[0].done, 1);
    CHECK_EQ_INT(g_save.players[0].rockets, 5);
}

TEST(Flow_CreditKill_in_dual_mode_counts_an_escape_for_the_other_player)
{
    BootAndNewGame(MODE_DUAL);
    g_save.players[0].totalEnemies = 5;
    g_save.players[1].totalEnemies = 5;
    CreditKill(1);
    CHECK_EQ_INT(g_save.players[1].killed, 1);
    CHECK_EQ_INT(g_save.players[0].escaped, 1);
    CHECK_EQ_INT(g_save.players[1].escaped, 0);
    CreditKill(0);
    CHECK_EQ_INT(g_save.players[1].escaped, 1);
}

TEST(Flow_CreditEscape_counts_for_both_players_in_dual_mode)
{
    BootAndNewGame(MODE_SINGLE);
    g_time = 20000;
    g_save.players[0].totalEnemies = 2;
    CreditEscape(0);
    CHECK_EQ_INT(g_save.players[0].escaped, 1);
    CHECK_EQ_INT(g_save.players[1].escaped, 0);
    CHECK_EQ_INT(g_save.players[0].done, 0);
    CreditEscape(0);
    CHECK_EQ_INT(g_save.players[0].done, 1);
    CHECK_EQ_INT(g_save.players[0].doneTime, 23000);
    g_gameMode = MODE_DUAL;
    CreditEscape(0);
    CHECK_EQ_INT(g_save.players[0].escaped, 3);
    CHECK_EQ_INT(g_save.players[1].escaped, 1);
}

TEST(Flow_LevelStallWatchdog_recounts_kills_after_600_idle_frames)
{
    BootAndNewGame(MODE_SINGLE);
    g_time = 30000;
    g_playerStallTime = g_time;
    g_save.players[0].totalEnemies = 10;
    g_save.players[0].escaped = 3;
    g_save.players[0].killed = 4;
    g_levelIdleCounter = 600;
    LevelStallWatchdog();
    CHECK_EQ_INT(g_save.players[0].killed, 4);
    CHECK_EQ_INT(g_save.players[0].done, 0);
    g_levelIdleCounter = 601;
    LevelStallWatchdog();
    CHECK_EQ_INT(g_levelIdleCounter, 0);
    CHECK_EQ_INT(g_save.players[0].killed, 7);
    CHECK_EQ_INT(g_save.players[0].done, 1);
    CHECK_EQ_INT(g_save.players[0].doneTime, 33000);
}

TEST(Flow_LevelStallWatchdog_finishes_a_level_stalled_for_45_seconds)
{
    BootAndNewGame(MODE_SINGLE);
    g_save.players[0].totalEnemies = 10;
    g_playerStallTime = 10000;
    g_save.players[0].levelTransitioning = 1;
    g_time = 55000;
    LevelStallWatchdog();
    CHECK_EQ_INT(g_save.players[0].done, 0);
    g_time = 55001;
    LevelStallWatchdog();
    CHECK_EQ_INT(g_save.players[0].done, 1);
    CHECK_EQ_INT(g_save.players[0].levelTransitioning, 0);
    CHECK_EQ_INT(g_save.players[0].doneTime, 58001);
    CHECK_EQ_INT(g_playerStallTime, 55001);
}

// ---- pause ----

TEST(Flow_PauseGame_saves_the_state_once)
{
    BootAndNewGame(MODE_SINGLE);
    g_state = STATE_PLAYING;
    PauseGame();
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    CHECK_EQ_INT(g_savedState, STATE_PLAYING);
    CHECK_EQ_INT(g_pauseCount, 1);
    CHECK_EQ_INT(g_fx[MAX_FX - 1].active, 1);
    CHECK(g_fx[0].gfx == g_gfxPause);
    CHECK_NEAR(g_fx[0].alpha, 255, 0);
    CHECK_NEAR(g_fx[1].alpha, 230, 0);
    PauseGame();
    CHECK_EQ_INT(g_savedState, STATE_PLAYING);
    CHECK_EQ_INT(g_pauseCount, 1);
}

TEST(Flow_ResumeGame_restores_the_state_and_counts_the_paused_time)
{
    BootAndNewGame(MODE_SINGLE);
    g_state = STATE_SHOP_GATE;
    g_fake.fileTime = 1000000000LL;
    PauseGame();
    g_fake.fileTime += 70000000LL;          // 7 s in 100 ns units
    g_pausedDuration = 5;
    ResumeGame();
    CHECK_EQ_INT(g_state, STATE_SHOP_GATE);
    CHECK_EQ_INT(g_pausedDuration, 5 + 70000000LL);
    CHECK_EQ_INT(g_timeD, 0);
    CHECK_EQ_INT(g_timeE, 0);
}

TEST(Flow_ResumeGame_removes_star_items_and_laser_shots)
{
    BootAndNewGame(MODE_SINGLE);
    g_items[3].alive = 1;
    g_items[3].type = ITEM_STAR;
    g_items[4].alive = 1;
    g_items[4].type = ITEM_STAR - 1;
    g_mapObjs[5].active = 1;
    g_mapObjs[5].laser = 1;
    g_mapObjs[6].active = 1;
    g_state = STATE_PLAYING;
    PauseGame();
    ResumeGame();
    CHECK_EQ_INT(g_items[3].alive, 0);
    CHECK_EQ_INT(g_items[4].alive, 1);
    CHECK_EQ_INT(g_mapObjs[5].active, 0);
    CHECK_EQ_INT(g_mapObjs[6].active, 1);
}

TEST(Flow_pausing_moves_running_timers_forward)
{
    PlayLevel1();
    g_save.players[0].freezeTimer = g_time + 100000;
    g_save.players[0].shieldTimer = 0;
    g_msgTimer = g_time + 100000;
    TapKey(K_VK_P, 1);
    CHECK_EQ_INT(g_state, STATE_PAUSED);
    unsigned freeze = g_save.players[0].freezeTimer, msg = g_msgTimer;
    unsigned t0 = g_time;
    RunFrames(10);
    CHECK_EQ_INT(g_save.players[0].freezeTimer, freeze + (g_time - t0));
    CHECK_EQ_INT(g_msgTimer, msg + (g_time - t0));
    CHECK_EQ_INT(g_save.players[0].shieldTimer, 0);
}

// ---- NewGame / SetStateByMode ----

TEST(Flow_NewGame_resets_the_run_and_drops_into_respawn)
{
    BootAndNewGame(MODE_SINGLE);
    g_levelRecs[0].score = 99;
    g_save.players[0].done = 1;
    g_save.players[0].levelFinished = 1;
    g_perfectCount = 3;
    g_time = 40000;
    NewGame(true);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK_EQ_INT(g_levelRecs[0].score, 0);
    CHECK_EQ_INT(g_save.players[0].done, 0);
    CHECK_EQ_INT(g_save.players[0].levelFinished, 0);
    CHECK_EQ_INT(g_perfectCount, 0);
    CHECK_EQ_INT(g_timerA, 42000);
    CHECK_EQ_INT(g_transitionLockUntil, 40000 + 500);
    CHECK_EQ_INT(g_timeMax, 45);
    CHECK_EQ_INT(g_shopItems, 0x53);
    CHECK_EQ_INT(g_save.players[0].level, 1);
}

TEST(Flow_NewGame_sets_the_item_weights)
{
    BootAndNewGame(MODE_SINGLE);
    CHECK_EQ_INT(g_bonusWeight[ITEM_EXTRA_LIFE], 10);
    CHECK_EQ_INT(g_bonusWeight[ITEM_RANDOM_BONUS], 111);
    CHECK_EQ_INT(g_bonusWeight[ITEM_WARP], 0);
    CHECK_EQ_INT(g_bonusWeight[ITEM_MONEY_SMALL], 300);
    CHECK_EQ_INT(g_bonusWeight[ITEM_SCOOP], 140);
    CHECK_EQ_INT(g_bonusWeight[ITEM_DRUNK], 40);
    CHECK_EQ_INT(g_bonusWeight[ITEM_EXTRA_BULLET_SPEED], 0);
}

TEST(Flow_NewGame_time_trial_keeps_only_its_own_items_and_runs_faster)
{
    PutClassic(1, WaveLevel("ALPHA", 2));
    FakePacAdd("timetrial_01.lvd", g_fake.keys, 1);
    BootGame();
    g_gameMode = MODE_TIME_TRIAL;
    g_cfg.difficulty = DIFF_HARD;
    NewGame(true);
    CHECK_EQ_INT(g_bonusWeight[ITEM_EXTRA_LIFE], 0);
    CHECK_EQ_INT(g_bonusWeight[ITEM_MONEY_SMALL], 0);
    CHECK_EQ_INT(g_bonusWeight[ITEM_WEAPON_DOUBLE], 0);
    CHECK_EQ_INT(g_bonusWeight[ITEM_DRUNK], 10);
    CHECK_EQ_INT(g_bonusWeight[ITEM_EXTRA_BULLET_SPEED], 15);
    CHECK_NEAR(g_gameSpeedMul, 7.0 / 6.0, 1e-6);
    CHECK_EQ_INT(g_bgIndex, 5);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_NORMAL);
    CHECK_EQ_INT(g_savedDifficultyTT, DIFF_HARD);
    CHECK(g_drawHudFn == DrawHudTimed);
}

TEST(Flow_NewGame_autoplay_gets_a_maxed_out_loadout)
{
    PutClassic(1, WaveLevel("ALPHA", 2));
    BootGame();
    g_gameMode = MODE_SINGLE;
    g_autoplay = true;
    NewGame(true);
    ShipDef *d = g_shipDefs[g_save.players[0].ship];
    CHECK_EQ_INT(g_save.players[0].lives, d->minEnergy + d->cost * 10);
    CHECK_EQ_INT(g_save.players[0].money, 10000);
    CHECK_EQ_INT(g_save.players[0].bullets, 50);
    CHECK_EQ_INT(g_save.players[0].armour, d->baseArmour + d->maxArmourBonus);
    CHECK_EQ_INT(g_save.players[0].gameSpeedSetting, 200);
}

TEST(Flow_SetStateByMode_picks_the_hud_of_each_mode)
{
    BootAndNewGame(MODE_SINGLE);
    CHECK(g_drawHudFn == DrawHud1P);
    g_gameMode = MODE_TWO_PLAYER;
    SetStateByMode();
    CHECK(g_drawHudFn == DrawHud2P);
    CHECK_EQ_INT(g_curPlayer, 0);
    g_gameMode = MODE_DUAL;
    SetStateByMode();
    CHECK(g_drawHudFn == DrawHud2PCoop);
    g_gameMode = MODE_TEAM;
    SetStateByMode();
    CHECK(g_drawHudFn == DrawHud1P);
    g_gameMode = MODE_TIME_TRIAL;
    SetStateByMode();
    CHECK(g_drawHudFn == DrawHudTimed);
}

TEST(Flow_NewGame_two_players_both_start_their_level)
{
    BootAndNewGame(MODE_TWO_PLAYER);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 2);
    CHECK_EQ_INT(g_save.players[1].totalEnemies, 2);
    CHECK_EQ_INT(g_enemies[1][0].active, 1);
    CHECK_EQ_INT(g_enemies[1][1].active, 1);
    CHECK_EQ_INT(g_curPlayer, 0);
}

// ---- StartNextLevel / SwitchPlayer ----

TEST(Flow_StartNextLevel_advances_one_level_until_the_level_starts)
{
    BootAndNewGame(MODE_SINGLE);
    g_time = 60000;
    g_state = STATE_PLAYING;
    g_save.players[0].level = 4;
    g_save.players[0].done = 1;
    g_save.players[0].doneTime = 1;
    g_save.players[0].started = 1;
    g_pendingLevelsPlayed = 0;
    g_levelStartLatch = 0;
    StartNextLevel();
    CHECK_EQ_INT(g_save.players[0].level, 5);
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    CHECK_EQ_INT(g_timerA, 62000);
    CHECK_EQ_INT(g_save.players[0].done, 0);
    CHECK_EQ_INT(g_save.players[0].doneTime, 0);
    CHECK_EQ_INT(g_save.players[0].started, 0);
    CHECK_EQ_INT(g_pendingLevelsPlayed, 1);
    StartNextLevel();
    CHECK_EQ_INT(g_save.players[0].level, 5);
    CHECK_EQ_INT(g_pendingLevelsPlayed, 1);
}

TEST(Flow_StartNextLevel_time_trial_ignores_the_latch_and_waits_1s)
{
    BootAndNewGame(MODE_SINGLE);
    g_gameMode = MODE_TIME_TRIAL;
    g_time = 60000;
    g_save.players[0].level = 4;
    g_levelStartLatch = 1;
    StartNextLevel();
    CHECK_EQ_INT(g_save.players[0].level, 5);
    CHECK_EQ_INT(g_timerA, 61000);
}

TEST(Flow_StartNextLevel_shortens_the_malfunction_timer)
{
    BootAndNewGame(MODE_SINGLE);
    g_levelStartLatch = 0;
    g_malfunctionTimer = 10000;
    StartNextLevel();
    CHECK(g_malfunctionTimer <= 10000 - 100 && g_malfunctionTimer > 10000 - 1500);
    g_levelStartLatch = 0;
    g_malfunctionTimer = 2050;
    StartNextLevel();
    CHECK(g_malfunctionTimer >= 2300 && g_malfunctionTimer < 4000);
}

TEST(Flow_SwitchPlayer_resumes_the_other_players_level)
{
    BootAndNewGame(MODE_TWO_PLAYER);
    g_time = 70000;
    g_state = STATE_PLAYING;
    g_save.players[1].level = 2;
    g_save.players[1].done = 1;
    g_levelObj[3].active = 1;
    SwitchPlayer();
    CHECK_EQ_INT(g_curPlayer, 1);
    CHECK_EQ_INT(g_state, STATE_GET_READY);
    CHECK_EQ_INT(g_curLevelNum, 2);
    CHECK_MEM(g_curLevelData.name1, "\x04" "BETA", 5);
    CHECK_EQ_INT(g_save.players[1].done, 0);
    CHECK_EQ_INT(g_timerA, 71500);
    CHECK_EQ_INT(g_timerB, 75000);
    CHECK_EQ_INT(g_levelObj[3].active, 0);
    SwitchPlayer();
    CHECK_EQ_INT(g_curPlayer, 0);
}

TEST(Flow_SwitchPlayer_to_a_finished_player_warps_out)
{
    BootAndNewGame(MODE_TWO_PLAYER);
    g_state = STATE_PLAYING;
    g_save.players[1].levelFinished = 1;
    g_save.players[1].hyperspaceInDuration = 77;
    g_save.players[1].shieldL = 1;
    SwitchPlayer();
    CHECK_EQ_INT(g_curPlayer, 1);
    CHECK_EQ_INT(g_state, STATE_MALFUNCTION_DEATH);
    CHECK_NEAR(g_save.players[1].hyperspaceOutTimer, 77, 0);
    CHECK_EQ_INT(g_save.players[1].levelFinished, 0);
    CHECK_EQ_INT(g_save.players[1].shieldL, 0);
    CHECK_EQ_INT(g_deathSeqActive, 1);
}

TEST(Flow_SwitchPlayer_does_nothing_on_the_hiscore_table)
{
    BootAndNewGame(MODE_TWO_PLAYER);
    g_state = STATE_HISCORE_TABLE;
    SwitchPlayer();
    CHECK_EQ_INT(g_curPlayer, 0);
}

// ---- StartLevel ----

static void StartLevelN(int n)
{
    g_curPlayer = 0;
    g_save.players[0].level = n;
    StartLevel();
}

TEST(Flow_StartLevel_spawns_each_group_at_its_spawn_point)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "SPAWN");
    LvRawSub *s = AddSub(L, 0, -60, 120, 30);
    s->fireRateMin = 900;
    s->fireRateMax = 33;
    s->fireDelay = 700;
    s->pathId = 4;
    AddSub(L, 0, 60, 125, 31);
    L->grp[0] = (LvRawGrp){.spawnX = 100, .spawnY = 40, .spawnDelay = 7, .spawnStep = 5, .count = 2,
                           .velX = 256, .velY = 512, .groupId = 9, .sub = {L->grp[0].sub[0], L->grp[0].sub[1]}};
    AddSub(L, 1, 0, 0, 50);
    L->grp[1].spawnX = -200;
    L->objCount[0] = 1;
    L->obj[0][0].pathX = -64;
    L->obj[0][0].pathY = 128;
    L->aux[0].y1 = 1;
    PutClassic(3, L);
    BootAndNewGame(MODE_SINGLE);
    g_diffHpBonusA = 3;
    g_fireDelayBiasA = 0;
    g_fireDelayBiasB = 0;
    StartLevelN(3);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 3);
    CHECK_EQ_INT(g_save.players[0].primaryEnemyCount, 3);
    CHECK_EQ_INT(g_save.players[0].levelEnemyDataCount, 2);
    Enemy *e = &g_enemies[0][0];
    CHECK_EQ_INT(e->active, 1);
    CHECK_EQ_INT(e->type, ENEMY_PATTERNED);
    CHECK_EQ_INT(e->hazardType, 1);
    CHECK_NEAR(e->x, 100 - 16 + 400, 0.01);
    CHECK_NEAR(e->y, 40 - 16, 0.01);
    CHECK_NEAR(e->hoverX, -60 + 400, 0.01);
    CHECK_NEAR(e->hoverY, 120, 0.01);
    CHECK_NEAR(e->velX, 1.0, 1e-6);
    CHECK_NEAR(e->velY, 2.0, 1e-6);
    CHECK_NEAR(e->accelX, -0.25, 1e-6);
    CHECK_NEAR(e->accelY, 0.5, 1e-6);
    CHECK_NEAR(e->hp, 33, 0);
    CHECK_NEAR(e->maxHp, 33, 0);
    CHECK_EQ_INT(e->groupIndex, 0);
    CHECK_EQ_INT(e->bonusGroupIndex, 9);
    CHECK_EQ_INT(e->patternStep, 0);
    CHECK_EQ_INT(e->oscillateMul, 4);
    CHECK_NEAR(e->oscillateAccel, 700 / 512.0, 1e-6);
    CHECK_EQ_INT(e->animPingPong, 1);
    CHECK_EQ_INT(e->fireDelay, 900 > g_enemyFireRateMin ? 900 : g_enemyFireRateMin);
    CHECK_EQ_INT(e->fireDelayStep, 33);
    CHECK_EQ_INT(e->attackDelay, 700 > g_fireDelayMin ? 700 : g_fireDelayMin);
    CHECK_EQ_INT(e->attackDelayStep, 4);
    CHECK(e->gfxA == g_alienGfxCache[0].gfx1 && e->gfxA != NULL);
    CHECK_NEAR(g_enemies[0][1].hp, 34, 0);
    CHECK_NEAR(g_enemies[0][1].hoverX, 460, 0.01);
    CHECK_NEAR(g_enemies[0][2].x, -200 - 16 + 400, 0.01);
    CHECK_EQ_INT(g_enemies[0][2].groupIndex, 1);
    CHECK_EQ_INT(g_enemies[0][3].active, 0);
    CHECK_EQ_INT(g_groupEnemyCount[0], 2);
    CHECK_EQ_INT(g_groupEnemyCount[1], 1);
    CHECK_EQ_INT(g_groupKillCount[9], 2);
}

TEST(Flow_StartLevel_staggers_the_attacks_of_a_group)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "STAGGER");
    for (int i = 0; i < 3; i++)
        AddSub(L, 0, 0, 0, 1);
    L->grp[0].spawnDelay = 20;
    L->grp[0].spawnStep = 15;
    PutClassic(3, L);
    LevelRaw *B = NewLevel(LEVEL_BONUS_WAVE, "BONUS");
    for (int i = 0; i < 3; i++)
        AddSub(B, 0, 0, 0, 1);
    B->grp[0].spawnDelay = 20;
    B->grp[0].spawnStep = 15;
    PutClassic(4, B);
    BootAndNewGame(MODE_SINGLE);
    StartLevelN(3);
    CHECK_NEAR(g_enemies[0][0].attackStaggerTimer, 20, 0);
    CHECK_NEAR(g_enemies[0][1].attackStaggerTimer, 35, 0);
    CHECK_NEAR(g_enemies[0][2].attackStaggerTimer, 50, 0);
    StartLevelN(4);
    CHECK_NEAR(g_enemies[0][0].attackStaggerTimer, 20, 0);
    CHECK_NEAR(g_enemies[0][2].attackStaggerTimer, 20, 0);
}

TEST(Flow_StartLevel_mirrors_levels_100_to_199)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "MIRROR");
    AddSub(L, 0, -60, 120, 1);
    L->grp[0].spawnX = 100;
    L->grp[0].velX = 256;
    L->objCount[0] = 1;
    L->obj[0][0].pathX = -64;
    PutClassic(100, L);
    BootAndNewGame(MODE_SINGLE);
    StartLevelN(100);
    CHECK_EQ_INT(g_mirrorLevel, 1);
    Enemy *e = &g_enemies[0][0];
    CHECK_NEAR(e->x, 400 - (100 + 16), 0.01);
    CHECK_NEAR(e->hoverX, 400 + 60, 0.01);
    CHECK_NEAR(e->velX, -1.0, 1e-6);
    CHECK_NEAR(e->accelX, 0.25, 1e-6);
}

TEST(Flow_StartLevel_spawns_header_enemies_as_wrappers)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "HDR");
    AddSub(L, 0, 0, 0, 1);
    L->hdr[1] = (LvRawHdr){.count = 2, .type = 1, .hp = 30, .fireRateMin = 5000, .fireRateMax = 6};
    L->aux[1] = (LvRawQ){.x1 = 8, .y1 = 1};
    L->h[0] = 300000;
    PutClassic(3, L);
    BootAndNewGame(MODE_SINGLE);
    g_diffHpBonusB = 4;
    g_fireDelayBiasB = 0;
    StartLevelN(3);
    CHECK_EQ_INT(g_save.players[0].primaryEnemyCount, 1);
    CHECK_EQ_INT(g_save.players[0].secondaryEnemyCount, 2);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 3);
    for (int i = 1; i <= 2; i++) {
        Enemy *e = &g_enemies[0][i];
        CHECK_EQ_INT(e->active, 1);
        CHECK_EQ_INT(e->type, ENEMY_WRAPPER);
        CHECK_NEAR(e->y, -110, 0);
        CHECK(e->x >= 200 && e->x < 600);
        CHECK_NEAR(e->hp, 34, 0);
        CHECK_EQ_INT(e->fireDelay, 5000);
        CHECK_EQ_INT(e->fireDelayStep, 6);
        CHECK_EQ_INT(e->score, 200000);
        CHECK_EQ_INT(e->animFrameCount, 7);
        CHECK_EQ_INT(e->animPingPong, 1);
    }
}

TEST(Flow_StartLevel_caps_the_score_of_group_aliens)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "CAP");
    AddSub(L, 0, 0, 0, 1);
    AddSub(L, 0, 0, 0, 1)->type = 2;
    AddSub(L, 0, 0, 0, 1)->type = 3;
    strcpy(L->gfx[1], "gfx\\aliens\\Alien2.bmp");
    strcpy(L->gfx[2], "gfx\\aliens\\Alien3.bmp");
    L->w[0] = 70000;
    L->w[1] = -5;
    L->w[2] = 1234;
    PutClassic(3, L);
    BootAndNewGame(MODE_SINGLE);
    StartLevelN(3);
    CHECK_EQ_INT(g_enemies[0][0].score, 50000);
    CHECK_EQ_INT(g_enemies[0][1].score, 0);
    CHECK_EQ_INT(g_enemies[0][2].score, 1234);
    CHECK(g_enemies[0][1].gfxA == g_alienGfxCache[1].gfx1 && g_enemies[0][1].gfxA != NULL);
    CHECK(g_enemies[0][2].gfxA == g_alienGfxCache[2].gfx1 && g_enemies[0][2].gfxA != NULL);
    CHECK_EQ_INT(g_enemyDamageStage[0][1], 1);
    CHECK_EQ_INT(g_enemyDamageStage[0][2], 2);
}

TEST(Flow_StartLevel_fire_delays)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "FIRE");
    AddSub(L, 0, 0, 0, 1)->fireRateMin = 1;          // below the minimum: clamped
    AddSub(L, 0, 0, 0, 1)->fireDelay = 3500;         // the special value: x10
    AddSub(L, 0, 0, 0, 1);                           // no delay, no path: fixed
    LvRawSub *s = AddSub(L, 0, 0, 0, 1);
    s->fireDelay = 1;                                // below the minimum: clamped
    s->pathId = 2;
    AddSub(L, 0, 0, 0, 1)->pathId = 3;               // no delay but a path: not fixed
    PutClassic(3, L);
    BootAndNewGame(MODE_SINGLE);
    g_enemyFireRateMin = 200;
    g_fireDelayMin = 300;
    g_fireDelayBiasA = 0;
    g_fireDelayBiasB = 0;
    StartLevelN(3);
    CHECK_EQ_INT(g_enemies[0][0].fireDelay, 200);
    CHECK_EQ_INT(g_enemies[0][1].attackDelay, 35000);
    CHECK_EQ_INT(g_enemies[0][1].fixedFireDelay, 0);
    CHECK_EQ_INT(g_enemies[0][2].attackDelay, 0);
    CHECK_EQ_INT(g_enemies[0][2].fixedFireDelay, 1);
    CHECK_EQ_INT(g_enemies[0][3].attackDelay, 300);
    CHECK_EQ_INT(g_enemies[0][3].fixedFireDelay, 0);
    CHECK_EQ_INT(g_enemies[0][4].attackDelay, 300);
    CHECK_EQ_INT(g_enemies[0][4].fixedFireDelay, 0);
}

TEST(Flow_StartLevel_spawns_a_boss_with_its_guns)
{
    LevelRaw *L = NewLevel(LEVEL_BOSS, "BOSS");
    L->count = 3;
    L->grp[0].spawnX = 10;
    L->grp[0].spawnY = 50;
    L->grp[0].velX = 128;
    L->grp[1] = (LvRawGrp){.spawnX = -70, .spawnY = 20, .kind = 7};
    L->grp[2] = (LvRawGrp){.spawnX = 70, .spawnY = 30, .kind = 8};
    L->hdr[0] = (LvRawHdr){.hp = 500, .fireRateMin = 1000, .fireRateMax = 44};
    L->h[0] = 30000000;
    PutClassic(3, L);
    BootAndNewGame(MODE_SINGLE);
    g_diffHpBonusB = 2.5f;
    StartLevelN(3);
    CHECK_EQ_INT(g_isBossLevel, 1);
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 1);
    Enemy *e = &g_enemies[0][0];
    CHECK_EQ_INT(e->active, 1);
    CHECK_EQ_INT(e->type, ENEMY_BOSS);
    CHECK_NEAR(e->x, 10 - 16 + 400, 0.01);
    CHECK_NEAR(e->y, 50 - 16, 0.01);
    CHECK_NEAR(e->velX, 0.5, 1e-6);
    CHECK_NEAR(e->hp, 550, 0);
    CHECK_NEAR(e->maxHp, 550, 0);
    CHECK_EQ_INT(e->score, 20000000);
    CHECK_EQ_INT(e->fireDelay, 1000);
    CHECK_EQ_INT(e->fireDelayStep, 44);
    CHECK_EQ_INT(e->bossGunASlot[0], 1);
    CHECK_NEAR(e->bossGunAX[0], -70, 0);
    CHECK_NEAR(e->bossGunAY[0], 20 + 48, 0);
    CHECK_EQ_INT(e->bossGunBSlot[0], 1);
    CHECK_NEAR(e->bossGunBX[0], 70, 0);
    CHECK_EQ_INT(e->bossGunCSlot[0], 0);
    CHECK_EQ_INT(g_bossGunCountA, 1);
    CHECK_EQ_INT(g_bossGunActiveB, 1);
    CHECK_EQ_INT(g_bossGunActiveC, 0);
    CHECK_EQ_INT(g_enemies[0][1].active, 0);
}

TEST(Flow_StartLevel_sets_the_level_type_flags)
{
    PutClassic(3, NewLevel(LEVEL_RACE, "RACE"));
    PutClassic(4, NewLevel(LEVEL_BONUS_WAVE, "BONUS"));
    PutClassic(5, NewLevel(LEVEL_WAVE_AIMED, "AIMED"));
    PutClassic(6, NewLevel(LEVEL_WAVE, "WAVE"));
    BootAndNewGame(MODE_SINGLE);
    int kills = g_killCount;
    StartLevelN(3);
    CHECK_EQ_INT(g_isBossLevel, 1);
    CHECK_EQ_INT(g_isWaveLevel, 0);
    CHECK_EQ_INT(g_save.players[0].trackKillsFlag, 1);
    CHECK_EQ_INT(g_save.players[0].shipDestroyedThisLevel, 0);
    CHECK_EQ_INT(g_killCount, kills + 1);
    StartLevelN(4);
    CHECK_EQ_INT(g_isBossLevel, 0);
    CHECK_EQ_INT(g_isWaveLevel, 1);
    CHECK_EQ_INT(g_save.players[0].trackKillsFlag, 0);
    CHECK_EQ_INT(g_save.players[0].shipDestroyedThisLevel, 1);
    StartLevelN(5);
    CHECK_EQ_INT(g_isWaveLevel, 1);
    CHECK_EQ_INT(g_enemyAimAtPlayer, 1);
    CHECK_EQ_INT(g_save.players[0].shipDestroyedThisLevel, 0);
    StartLevelN(6);
    CHECK_EQ_INT(g_isWaveLevel, 1);
    CHECK_EQ_INT(g_enemyAimAtPlayer, 0);
}

TEST(Flow_StartLevel_picks_the_background_and_death_explosion_by_level)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "FX");
    AddSub(L, 0, 0, 0, 1);
    PutClassic(1, L);
    BootAndNewGame(MODE_SINGLE);
    StartLevelN(6);
    CHECK_EQ_INT(g_bgIndex, 1);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionGfx, 14);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionLife, 100);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionR, 255);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionG, 180);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionB, 0);
    StartLevelN(30);
    CHECK_EQ_INT(g_bgIndex, 2);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionGfx, 17);
    StartLevelN(60);
    CHECK_EQ_INT(g_bgIndex, 3);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionGfx, 25);
    StartLevelN(99);
    CHECK_EQ_INT(g_bgIndex, 4);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionGfx, 14);
    CHECK_EQ_INT(g_enemies[0][0].deathExplosionR, 200);
}

TEST(Flow_StartLevel_records_the_previous_level)
{
    BootAndNewGame(MODE_SINGLE);
    g_save.players[0].score = 123456;
    g_save.players[0].money = 777;
    g_save.players[0].shots = 42;
    g_save.players[0].rank = 3;
    g_deathsCount = 54;
    StartLevelN(2);
    CHECK_EQ_INT(g_levelRecs[2].score, 123456);
    CHECK_EQ_INT(g_levelRecs[2].money, 777);
    CHECK_EQ_INT(g_levelRecs[2].shots, 42);
    CHECK_EQ_INT(g_levelRecs[2].rank, 3);
    CHECK_EQ_INT(g_levelRecs[2].deathsByte, 2);
    CHECK_EQ_INT(g_deathsCount, 50);
    CHECK_EQ_INT(g_levelStartLatch, 0);
    CHECK_EQ_INT(g_loadedLevel, 2);
}

TEST(Flow_StartLevel_ramps_the_difficulty_every_100_levels)
{
    BootAndNewGame(MODE_SINGLE);
    g_diffHpBonusA = 0;
    g_diffHpBonusB = 0;
    g_fireDelayBiasA = -480;
    float speed = g_gameSpeedMul;
    StartLevelN(100);
    CHECK_NEAR(g_diffHpBonusA, 0, 0);
    StartLevelN(101);
    CHECK_NEAR(g_diffHpBonusA, 1, 0);
    CHECK_NEAR(g_diffHpBonusB, 5, 0);
    CHECK_EQ_INT(g_fireDelayBiasA, -500);
    CHECK_NEAR(g_gameSpeedMul, speed + 0.12f, 1e-5);
    CHECK_EQ_INT(g_save.players[0].levelMilestoneHandled, 1);
}

TEST(Flow_StartLevel_game_over_shows_the_hiscore_table)
{
    BootAndNewGame(MODE_SINGLE);
    g_save.players[0].lives = MinEnergy(0);
    StartLevelN(2);
    CHECK_EQ_INT(g_state, STATE_HISCORE_TABLE);
}

// ---- get ready ----

TEST(Flow_UpdateGetReadyRespawn_names_the_player_in_two_player_games)
{
    BootAndNewGame(MODE_TWO_PLAYER);
    g_time = 1000;
    g_curPlayer = 1;
    g_save.players[1].started = 1;
    g_timerA = 5000;
    g_introDone = 0;
    UpdateGetReadyRespawn();
    CHECK_STR(g_getReadyText, "GET READY PLAYER 2");
    CHECK_EQ_INT(g_introDone, 1);
    CHECK_EQ_INT(g_timerA, 3000);
    CHECK_EQ_INT(g_levelBannerTime, 3000);
}

TEST(Flow_UpdateGetReadyRespawn_starts_play_when_the_timer_runs_out)
{
    BootAndNewGame(MODE_SINGLE);
    g_timerA = 5000;
    g_introDone = 1;
    g_time = 5000;
    UpdateGetReadyRespawn();
    CHECK_EQ_INT(g_state, STATE_RESPAWN);
    g_time = 5001;
    UpdateGetReadyRespawn();
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(g_timerA, 0);
    CHECK_EQ_INT(g_introDone, 0);
}

TEST(Flow_UpdateGetReadyNewLevel_plays_after_timer_B)
{
    BootAndNewGame(MODE_SINGLE);
    g_state = STATE_GET_READY;
    g_save.players[0].started = 0;
    g_save.players[0].level = 2;
    g_time = 1000;
    g_timerA = 2000;
    g_timerB = 4000;
    g_introDone = 0;
    UpdateGetReadyNewLevel();
    CHECK_EQ_INT(g_save.players[0].started, 1);
    CHECK_STR(g_levelBannerText, "LEVEL 2");
    CHECK_STR(g_levelName, "BETA");
    CHECK_EQ_INT(g_save.players[0].totalEnemies, 3);
    CHECK_EQ_INT(g_introDone, 1);
    g_time = 3000;
    UpdateGetReadyNewLevel();
    CHECK_EQ_INT(g_timerA, 0);
    CHECK_EQ_INT(g_state, STATE_GET_READY);
    g_time = 4001;
    UpdateGetReadyNewLevel();
    CHECK_EQ_INT(g_state, STATE_PLAYING);
    CHECK_EQ_INT(g_timerB, 0);
}

// ---- misc ----

TEST(Flow_ApplyBgTint_clamps_to_the_three_tints)
{
    g_bgTint = -3;
    ApplyBgTint();
    CHECK_EQ_INT(g_bgTint, 0);
    g_bgTint = 7;
    ApplyBgTint();
    CHECK_EQ_INT(g_bgTint, 2);
    g_bgTint = 1;
    ApplyBgTint();
    CHECK_EQ_INT(g_bgTint, 1);
}

TEST(Flow_ResetObjectsKeep_keeps_captured_aliens)
{
    BootAndNewGame(MODE_SINGLE);
    g_enemies[0][3].active = 1;
    g_enemies[0][3].type = ENEMY_CAPTURED;
    g_enemies[0][4].active = 1;
    g_enemies[0][4].type = ENEMY_HOVER;
    g_items[2].alive = 1;
    g_mapObjs[1].active = 1;
    g_levelObj[1].active = 1;
    ResetObjectsKeep();
    CHECK_EQ_INT(g_enemies[0][3].active, 1);
    CHECK_EQ_INT(g_enemies[0][4].active, 0);
    CHECK_EQ_INT(g_items[2].alive, 0);
    CHECK_EQ_INT(g_mapObjs[1].active, 0);
    CHECK_EQ_INT(g_levelObj[1].active, 0);
    ResetAllObjects();
    CHECK_EQ_INT(g_enemies[0][3].active, 0);
}

TEST(Flow_ResetObjects_clears_the_dual_mode_field_of_player_1)
{
    BootAndNewGame(MODE_DUAL);
    g_curPlayer = 1;
    g_enemies[0][2].active = 1;
    g_enemies[1][2].active = 1;
    g_explosions[4].active = 1;
    ResetObjects();
    CHECK_EQ_INT(g_enemies[0][2].active, 0);
    CHECK_EQ_INT(g_enemies[1][2].active, 1);
    CHECK_EQ_INT(g_explosions[4].active, 0);
}
