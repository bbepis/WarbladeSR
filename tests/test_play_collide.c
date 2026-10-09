// Tests for src/game/collide.c: items vs the player (pickups), enemy shots vs the player
// (armour, death, captured shields, god mode), player shots vs enemies (damage, kills,
// score, lasers, bosses), shield (tractor beam) grabbing, and the small helpers. The fake
// engine loads no hit masks, so MaskCollide falls back to the bounding boxes.
#include "support.h"

#define PL0 g_save.players[0]
#define PL1 g_save.players[1]

enum { ARMOUR0 = 123, ARMOUR_STEP = 5 };   // ship 0

static void StartPlay(void)
{
    BootGame();
    SeedRand(999);
    g_gameMode = MODE_SINGLE;
    g_playerUpdateFn = UpdatePlayer;
    g_autoplay = 0;
    g_cheatGodMode = false;
    g_curPlayer = 0;
    PL0.ship = 0;
    PL1.ship = 0;
    InitPlayer(0);
    PL0.x = 300;
    PL0.y = 550;
    g_state = STATE_PLAYING;
    g_time = 500000;
    g_frameDt = 1.0f;
    memset(g_items, 0, sizeof g_items);
    memset(g_mapObjs, 0, sizeof g_mapObjs);
    memset(g_enemies, 0, sizeof g_enemies);
    memset(g_levelObj, 0, sizeof g_levelObj);
    FakeReleaseAllKeys();
}

typedef struct { unsigned x, y, z, w, t; } Rng;
static Rng SaveRng(void) { return (Rng){g_rngX, g_rngY, g_rngZ, g_rngW, g_rngT}; }
static void RestoreRng(Rng r) { g_rngX = r.x; g_rngY = r.y; g_rngZ = r.z; g_rngW = r.w; g_rngT = r.t; }

static int Plays(AudioHandle sample)
{
    return FakePlayCount(FakeSampleName(sample));
}

static Bonus *AddItem(int i, int type, float x, float y, int w, int h)
{
    Bonus *b = &g_items[i];
    b->alive = 1;
    b->active = 1;
    b->type = type;
    b->x = x;
    b->y = y;
    b->w = w;
    b->h = h;
    return b;
}

static LevelObj *AddBullet(int i, float x, float y, int w, int h)
{
    LevelObj *o = &g_levelObj[i];
    o->active = 1;
    o->x = x;
    o->y = y;
    o->w = w;
    o->h = h;
    return o;
}

static Enemy *AddEnemy(int p, int i, int type, float x, float y)
{
    Enemy *e = &g_enemies[p][i];
    e->active = 1;
    e->type = type;
    e->x = x;
    e->y = y;
    e->hp = 10;
    e->maxHp = 10;
    e->pairedEnemyIdx = -1;
    e->hitRect.x1 = 0;
    e->hitRect.y1 = 0;
    e->hitRect.x2 = 32;
    e->hitRect.y2 = 32;
    return e;
}

static MapObj *AddShot(int i, float x, float y, float dmg)
{
    MapObj *s = &g_mapObjs[i];
    s->active = 1;
    s->player = 0;
    s->x = x;
    s->y = y;
    s->w = 4;
    s->stepY = 10;
    s->dmg = dmg;
    s->cost = 1;
    s->link1 = s->link2 = -1;
    s->enemy = 149;
    return s;
}

// ---------------------------------------------------------------- items vs the player

TEST(Play_ItemsVsPlayer_picks_up_an_overlapping_item)
{
    StartPlay();
    AddItem(5, ITEM_ARMOUR, 310, 540, 20, 20);
    ItemsVsPlayer();
    CHECK_EQ_INT(g_items[5].alive, 0);
    CHECK_EQ_INT(PL0.armour, ARMOUR0 + ARMOUR_STEP);
    CHECK_EQ_INT(PL0.pickupCount, 1);
}

TEST(Play_ItemsVsPlayer_uses_a_40x27_box)
{
    StartPlay();
    // just touching each side of the ship's 40x27 box doesn't count...
    AddItem(0, ITEM_ARMOUR, 280, 550, 20, 20);   // left: x2 == 300
    AddItem(1, ITEM_ARMOUR, 340, 550, 20, 20);   // right: x1 == 340
    AddItem(2, ITEM_ARMOUR, 300, 530, 20, 20);   // above: y2 == 550
    AddItem(3, ITEM_ARMOUR, 300, 577, 20, 20);   // below: y1 == 577
    ItemsVsPlayer();
    for (int i = 0; i < 4; i++)
        CHECK_EQ_INT(g_items[i].alive, 1);
    CHECK_EQ_INT(PL0.pickupCount, 0);
    // ...one pixel further in does
    g_items[0].x = 281;
    g_items[1].x = 339;
    g_items[2].y = 531;
    g_items[3].y = 576;
    ItemsVsPlayer();
    for (int i = 0; i < 4; i++)
        CHECK_EQ_INT(g_items[i].alive, 0);
    CHECK_EQ_INT(PL0.pickupCount, 4);
}

TEST(Play_ItemsVsPlayer_ignores_inactive_items)
{
    StartPlay();
    AddItem(0, ITEM_ARMOUR, 310, 540, 20, 20)->active = 0;
    AddItem(1, ITEM_ARMOUR, 310, 540, 20, 20)->alive = 0;
    ItemsVsPlayer();
    CHECK_EQ_INT(g_items[0].alive, 1);
    CHECK_EQ_INT(PL0.pickupCount, 0);
}

TEST(Play_ItemsVsPlayer_not_while_dead_out_of_lives_or_in_hyperspace)
{
    StartPlay();
    AddItem(0, ITEM_ARMOUR, 310, 540, 20, 20);
    PL0.dead = 1;
    ItemsVsPlayer();
    CHECK_EQ_INT(g_items[0].alive, 1);
    PL0.dead = 0;
    PL0.lives = 26;
    ItemsVsPlayer();
    CHECK_EQ_INT(g_items[0].alive, 1);
    PL0.lives = 30;
    PL0.hyperspaceFade = 50;
    ItemsVsPlayer();
    CHECK_EQ_INT(g_items[0].alive, 1);
    PL0.hyperspaceFade = 49;
    ItemsVsPlayer();
    CHECK_EQ_INT(g_items[0].alive, 0);
}

TEST(Play_ItemsVsPlayer_in_mirror_mode_either_ship_collects)
{
    StartPlay();
    PL0.mirrorTime = g_time + 10000;
    // the mirrored ship is at (800 - 40) - 300 = 460
    AddItem(0, ITEM_EXTRA_TIME, 470, 540, 20, 20);
    AddItem(1, ITEM_EXTRA_SPEED, 310, 540, 20, 20);
    int frames = 0;
    while ((g_items[0].alive || g_items[1].alive) && frames < 50) {
        ItemsVsPlayer();
        frames++;
    }
    CHECK_EQ_INT(g_items[0].alive, 0);
    CHECK_EQ_INT(g_items[1].alive, 0);
    CHECK(frames > 1);
}

TEST(Play_ItemsVsBothPlayers_checks_both_players_in_either_order)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    PL0.x = 100;
    PL1.x = 500;
    PL1.y = 550;
    int seen[2] = {0};
    for (int round = 0; round < 40; round++) {
        AddItem(0, ITEM_EXTRA_LIFE, 510, 540, 20, 20);
        int lives = PL1.lives = 30;
        Rng r = SaveRng();
        int first = RandRange(0, 2);
        RestoreRng(r);
        ItemsVsBothPlayers();
        CHECK_EQ_INT(g_curPlayer, 0);
        CHECK_EQ_INT(g_items[0].alive, 0);
        CHECK_EQ_INT(PL1.lives, lives + 4);
        seen[first]++;
    }
    CHECK(seen[0] > 0 && seen[1] > 0);
}

TEST(Play_ItemsVsBothPlayers_randomizes_simultaneous_pickups)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    PL1.x = PL0.x;
    int expected[2] = {0};
    for (int round = 0; round < 10; round++) {
        AddItem(0, ITEM_EXTRA_TIME, 310, 540, 20, 20);
        Rng r = SaveRng();
        int first = RandRange(0, 2);
        RestoreRng(r);
        ItemsVsBothPlayers();
        CHECK_EQ_INT(g_items[0].alive, 0);
        expected[first]++;
        CHECK_EQ_INT(PL0.pickupCount, expected[0]);
        CHECK_EQ_INT(PL1.pickupCount, expected[1]);
    }
}

// ---------------------------------------------------------------- enemy shots vs the player

TEST(Play_BulletsVsPlayer_kills_a_ship_without_spare_armour)
{
    StartPlay();
    PL0.drunkModeTimer = g_time + 5000;
    PL0.scoopTimer = g_time + 5000;
    PL0.mirrorTime = 0;
    int deaths = g_deathsCount;
    AddBullet(3, 310, 555, 8, 8);
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[3].active, 0);
    CHECK_EQ_INT(PL0.dead, 1);
    CHECK_EQ_INT(PL0.deaths, 1);
    CHECK_EQ_INT(PL0.respawnTime, g_time + 3000);
    CHECK_EQ_INT(PL0.drunkModeTimer, 0);
    CHECK_EQ_INT(PL0.scoopTimer, 0);
    CHECK_EQ_INT(g_deathsCount, deaths + 2);
    CHECK_EQ_INT(PL0.armour, ARMOUR0);
    CHECK(Plays(g_sfxExplo4) >= 2);
}

TEST(Play_BulletsVsPlayer_armour_absorbs_a_hit)
{
    StartPlay();
    PL0.armour = ARMOUR0 + ARMOUR_STEP;
    PL0.buffDuration = 20;
    AddBullet(0, 310, 555, 8, 8);
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 0);
    CHECK_EQ_INT(PL0.dead, 0);
    CHECK_EQ_INT(PL0.armour, ARMOUR0);
    CHECK_EQ_INT(PL0.collisionsTaken, 1);
    CHECK_EQ_INT(PL0.shieldTimer, g_time + 20 / 5 * 1000);
    CHECK_EQ_INT(Plays(g_sfxExplo2), 1);
}

TEST(Play_BulletsVsPlayer_only_one_hit_per_frame)
{
    StartPlay();
    PL0.armour = ARMOUR0 + 2 * ARMOUR_STEP;
    AddBullet(0, 310, 555, 8, 8);
    AddBullet(1, 320, 555, 8, 8);
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 0);
    CHECK_EQ_INT(g_levelObj[1].active, 0);
    CHECK_EQ_INT(PL0.armour, ARMOUR0 + ARMOUR_STEP);
    CHECK_EQ_INT(PL0.collisionsTaken, 1);
}

TEST(Play_BulletsVsPlayer_misses_outside_the_ship_box)
{
    StartPlay();
    AddBullet(0, 292, 555, 8, 8);   // bx2 == 300
    AddBullet(1, 340, 555, 8, 8);   // bx1 == 340
    AddBullet(2, 310, 542, 8, 8);   // by2 == 550
    AddBullet(3, 310, 577, 8, 8);   // by1 == 577
    AddBullet(4, 290, 555, 8, 8);
    g_levelObj[4].hitOffsetX = 40;   // the hit box is offset onto the ship
    BulletsVsPlayer();
    for (int i = 0; i < 4; i++)
        CHECK_EQ_INT(g_levelObj[i].active, 1);
    CHECK_EQ_INT(g_levelObj[4].active, 0);
    CHECK_EQ_INT(PL0.dead, 1);
}

TEST(Play_BulletsVsPlayer_shield_god_mode_and_death_protect)
{
    StartPlay();
    AddBullet(0, 310, 555, 8, 8);
    PL0.shieldTimer = g_time + 1000;
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 1);
    PL0.shieldTimer = 0;
    g_cheatGodMode = true;
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 1);
    g_cheatGodMode = false;
    PL0.dead = 1;
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 1);
    PL0.dead = 0;
    PL0.lives = 26;
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 1);
    PL0.lives = 27;
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 0);
    CHECK_EQ_INT(PL0.dead, 1);
}

TEST(Play_BulletsVsPlayer_left_captured_enemy_shields_the_ship)
{
    StartPlay();
    PL0.shieldL = 1;
    PL0.shieldLIdx = 4;
    AddEnemy(0, 4, ENEMY_CAPTURED, 264, 550)->settled = 1;
    AddBullet(0, 270, 555, 8, 8);   // left of the ship, inside the shield's zone
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 0);
    CHECK_EQ_INT(PL0.shieldL, 0);
    CHECK_EQ_INT(g_enemies[0][4].active, 0);
    CHECK_EQ_INT(g_enemies[0][4].settled, 0);
    CHECK_EQ_INT(PL0.dead, 0);
    CHECK_EQ_INT(Plays(g_sfxExplo1), 1);
}

TEST(Play_BulletsVsPlayer_right_captured_enemy_shields_the_ship)
{
    StartPlay();
    PL0.shieldR = 1;
    PL0.shieldRIdx = 6;
    AddEnemy(0, 6, ENEMY_CAPTURED, 336, 550)->settled = 1;
    AddBullet(0, 270, 555, 8, 8);   // on the (empty) left side: misses
    AddBullet(1, 350, 555, 8, 8);
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 1);
    CHECK_EQ_INT(g_levelObj[1].active, 0);
    CHECK_EQ_INT(PL0.shieldR, 0);
    CHECK_EQ_INT(g_enemies[0][6].active, 0);
    CHECK_EQ_INT(PL0.dead, 0);
}

TEST(Play_BulletsVsPlayer_an_unsettled_shield_does_not_block)
{
    StartPlay();
    PL0.shieldL = 1;
    PL0.shieldLIdx = 4;
    AddEnemy(0, 4, ENEMY_CAPTURED, 264, 550)->settled = 0;
    AddBullet(0, 270, 555, 8, 8);
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 1);
    CHECK_EQ_INT(PL0.shieldL, 1);
}

TEST(Play_BulletsVsPlayer_a_hit_on_the_ship_costs_a_shield_instead_of_a_life)
{
    StartPlay();
    PL0.shieldL = 1;
    PL0.shieldLIdx = 4;
    AddEnemy(0, 4, ENEMY_CAPTURED, 264, 550)->settled = 1;
    PL0.shieldR = 1;
    PL0.shieldRIdx = 5;
    AddEnemy(0, 5, ENEMY_CAPTURED, 336, 550)->settled = 1;
    AddBullet(0, 316, 555, 8, 8);   // straight on the ship
    BulletsVsPlayer();
    CHECK_EQ_INT(g_levelObj[0].active, 0);
    CHECK_EQ_INT(PL0.dead, 0);
    CHECK_EQ_INT(PL0.armour, ARMOUR0);
    CHECK_EQ_INT(PL0.shieldL, 0);   // the left one goes first
    CHECK_EQ_INT(PL0.shieldR, 1);
    AddBullet(0, 316, 555, 8, 8);
    BulletsVsPlayer();
    CHECK_EQ_INT(PL0.shieldR, 0);
    CHECK_EQ_INT(g_enemies[0][5].active, 0);
    CHECK_EQ_INT(PL0.dead, 0);
}

TEST(Play_BulletsVsBothPlayers_checks_both_players_in_either_order)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    PL0.x = 200;
    PL1.x = 500;
    PL1.y = 550;
    int seen[2] = {0};
    for (int round = 0; round < 30; round++) {
        PL0.shieldTimer = PL1.shieldTimer = 0;
        PL0.armour = PL1.armour = ARMOUR0 + ARMOUR_STEP;
        AddBullet(0, 210, 555, 8, 8);
        AddBullet(1, 510, 555, 8, 8);
        Rng r = SaveRng();
        int first = RandRange(0, 2);
        RestoreRng(r);
        BulletsVsBothPlayers();
        CHECK_EQ_INT(PL0.armour, ARMOUR0);
        CHECK_EQ_INT(PL1.armour, ARMOUR0);
        CHECK_EQ_INT(g_levelObj[0].active, 0);
        CHECK_EQ_INT(g_levelObj[1].active, 0);
        CHECK_EQ_INT(g_curPlayer, 0);
        seen[first]++;
    }
    CHECK(seen[0] > 0 && seen[1] > 0);
}

// ---------------------------------------------------------------- player shots vs enemies

TEST(Play_PlayerShotsHitEnemies_damages_an_enemy)
{
    StartPlay();
    AddEnemy(0, 2, ENEMY_DIVING, 290, 290)->hp = 10;
    g_enemies[0][2].score = 50;
    AddShot(0, 300, 300, 4);
    PL0.energy = 1;
    PlayerShotsHitEnemies();
    CHECK_NEAR(g_enemies[0][2].hp, 6, 1e-6);
    CHECK_EQ_INT(g_enemies[0][2].active, 1);
    CHECK_EQ_INT(g_enemies[0][2].altFrameCounter, 5);
    CHECK_EQ_INT(g_mapObjs[0].active, 0);
    CHECK_EQ_INT(PL0.energy, 0);   // the shot's slot is free again
    CHECK_EQ_INT(PL0.hits, 1);
    CHECK_EQ_INT(PL0.score, 0);
}

TEST(Play_PlayerShotsHitEnemies_kills_and_scores)
{
    StartPlay();
    AddEnemy(0, 2, ENEMY_DIVING, 290, 290)->hp = 3;
    g_enemies[0][2].score = 50;
    AddShot(0, 300, 300, 4);
    g_scoreMul[0] = 2;
    int hits = g_hits;
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_enemies[0][2].active, 0);
    CHECK_EQ_INT(g_mapObjs[0].active, 0);
    CHECK_EQ_INT(PL0.score, 100);
    CHECK_EQ_INT(PL0.killed, 1);
    CHECK_EQ_INT(PL0.hits, 1);
    CHECK_EQ_INT(g_hits, hits + 1);
}

TEST(Play_PlayerShotsHitEnemies_exact_damage_kills)
{
    StartPlay();
    AddEnemy(0, 2, ENEMY_DIVING, 290, 290)->hp = 4;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_enemies[0][2].active, 0);
}

TEST(Play_PlayerShotsHitEnemies_uses_the_enemy_hit_rect)
{
    StartPlay();
    Enemy *e = AddEnemy(0, 2, ENEMY_DIVING, 200, 200);
    e->hitRect.x1 = 8; e->hitRect.y1 = 4; e->hitRect.x2 = 16; e->hitRect.y2 = 16;
    // the hit box is x 208..224, y 204..220
    AddShot(0, 204, 210, 1);   // sx2 == 208: just left of it
    AddShot(1, 224, 210, 1);   // sx == 224: just right of it
    AddShot(2, 212, 194, 1);   // sy2 == 204: just above it
    AddShot(3, 212, 220, 1);   // sy == 220: just below it
    PlayerShotsHitEnemies();
    for (int i = 0; i < 4; i++)
        CHECK_EQ_INT(g_mapObjs[i].active, 1);
    CHECK_NEAR(e->hp, 10, 1e-6);
    AddShot(4, 205, 211, 1);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_mapObjs[4].active, 0);
    CHECK_NEAR(e->hp, 9, 1e-6);
    AddShot(5, 212, 196, 1);   // starts above the box, its 10 px reach into it
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_mapObjs[5].active, 0);
    CHECK_NEAR(e->hp, 8, 1e-6);
}

TEST(Play_PlayerShotsHitEnemies_hover_enemies_are_hit_where_drawn)
{
    StartPlay();
    Enemy *e = AddEnemy(0, 2, ENEMY_HOVER, 200, 200);
    e->offsetX = 100;
    e->offsetY = 50;
    AddShot(0, 210, 210, 1);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_mapObjs[0].active, 1);
    AddShot(1, 310, 260, 1);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_mapObjs[1].active, 0);
    CHECK_NEAR(e->hp, 9, 1e-6);
}

TEST(Play_PlayerShotsHitEnemies_skips_captured_debris_and_staggered_enemies)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_CAPTURED, 290, 290);
    AddEnemy(0, 2, ENEMY_DEBRIS, 290, 290);
    AddEnemy(0, 3, ENEMY_DIVING, 290, 290)->attackStaggerTimer = 1.0f;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_mapObjs[0].active, 1);
    g_enemies[0][3].attackStaggerTimer = 0.5f;
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_mapObjs[0].active, 0);
    CHECK_NEAR(g_enemies[0][3].hp, 6, 1e-6);
}

TEST(Play_PlayerShotsHitEnemies_one_shot_hits_one_enemy)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_DIVING, 290, 290);
    AddEnemy(0, 2, ENEMY_DIVING, 290, 290);
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_NEAR(g_enemies[0][1].hp, 6, 1e-6);
    CHECK_NEAR(g_enemies[0][2].hp, 10, 1e-6);
}

TEST(Play_PlayerShotsHitEnemies_a_laser_loses_half_its_power_per_hit)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_DIVING, 290, 100)->hp = 100;
    MapObj *s = AddShot(0, 300, 500, 8);
    s->laser = 1;   // a laser reaches from the top of the screen down to the ship
    PlayerShotsHitEnemies();
    CHECK_NEAR(g_enemies[0][1].hp, 92, 1e-6);
    CHECK_NEAR(s->dmg, 4, 1e-6);
    CHECK_EQ_INT(s->active, 1);
    CHECK_EQ_INT(g_cfg.collisionDetail, 3);   // restored after the laser test
}

TEST(Play_PlayerShotsHitEnemies_a_spent_laser_ends_with_its_links)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_DIVING, 290, 100)->hp = 1000;
    MapObj *s = AddShot(0, 300, 500, 0);
    s->laser = 1;
    s->link1 = 5;
    AddShot(5, 0, 0, 1)->cost = 1;
    g_mapObjs[5].y = -1000;
    PL0.energy = 2;
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(s->active, 0);
    CHECK_EQ_INT(g_mapObjs[5].active, 0);
    CHECK_EQ_INT(PL0.energy, 0);
}

TEST(Play_PlayerShotsHitEnemies_a_kill_unlocks_the_rockets_target)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_DIVING, 290, 290);
    AddEnemy(0, 7, ENEMY_DIVING, 600, 100)->locked = 1;
    MapObj *s = AddShot(0, 300, 300, 4);
    s->enemy = 7;
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_enemies[0][7].locked, 0);
}

TEST(Play_PlayerShotsHitEnemies_kills_speed_up_the_others_fire)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_DIVING, 290, 290)->hp = 1;
    Enemy *o = AddEnemy(0, 5, ENEMY_DIVING, 600, 100);
    o->attackDelay = 100; o->attackDelayStep = 3;
    o->fireDelay = 50; o->fireDelayStep = 2;
    g_fireDelayMin = 10;
    g_enemyFireRateMin = 10;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(o->attackDelay, 97);
    CHECK_EQ_INT(o->fireDelay, 48);
    o->attackDelay = 11;
    o->fireDelay = 11;
    AddEnemy(0, 1, ENEMY_DIVING, 290, 290)->hp = 1;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(o->attackDelay, 10);
    CHECK_EQ_INT(o->fireDelay, 10);
}

TEST(Play_PlayerShotsHitEnemies_paired_kill_bonus)
{
    StartPlay();
    Enemy *e = AddEnemy(0, 1, ENEMY_DIVING, 290, 290);
    e->hp = 1;
    e->score = 10;
    e->pairedEnemyIdx = 4;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(PL0.score, 2500 + 10);
    CHECK_EQ_INT(e->pairedEnemyIdx, -1);
}

TEST(Play_PlayerShotsHitEnemies_group_clear_bonus)
{
    StartPlay();
    g_groupEnemyCount[3] = 2;
    g_typeKillCombo[3] = 0;
    Enemy *a = AddEnemy(0, 1, ENEMY_PATTERNED, 290, 290);
    a->hp = 1; a->groupIndex = 3;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(PL0.score, 0);
    CHECK_EQ_INT(g_typeKillCombo[3], 1);
    a = AddEnemy(0, 2, ENEMY_ESCAPER, 290, 290);
    a->hp = 1; a->groupIndex = 3;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(PL0.score, 10000);
    CHECK_EQ_INT(g_typeKillCombo[3], 0);
}

TEST(Play_PlayerShotsHitEnemies_alt_fire_chain_kills_its_pair)
{
    StartPlay();
    Enemy *a = AddEnemy(0, 1, ENEMY_DIVING, 290, 290);
    a->hp = 1;
    a->altFireActive = 1;
    AddEnemy(0, 8, ENEMY_DIVING, 600, 100)->pairedEnemyIdx = 1;
    AddEnemy(0, 9, ENEMY_DIVING, 650, 100)->pairedEnemyIdx = 1;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_enemies[0][8].active, 0);
    CHECK_EQ_INT(g_enemies[0][9].active, 0);
    CHECK_EQ_INT(PL0.score, 2500 * 4);
    CHECK_EQ_INT(PL0.killed, 3);
}

TEST(Play_PlayerShotsHitEnemies_a_wrapper_drops_a_coin)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_WRAPPER, 290, 290)->hp = 1;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_enemies[0][1].active, 0);
    CHECK_EQ_INT(g_items[0].alive, 1);
    CHECK_NEAR(g_items[0].x, 290 + 22, 1e-4);
    CHECK_EQ_INT(PL0.killed, 1);
}

TEST(Play_PlayerShotsHitEnemies_a_mothership_drops_a_rank_gem)
{
    StartPlay();
    PL0.marks = MARKS_ALL & ~MARK_2;
    AddEnemy(0, 1, ENEMY_MOTHERSHIP, 280, 280)->hp = 1;
    AddShot(0, 300, 300, 4);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_enemies[0][1].active, 0);
    CHECK_EQ_INT(g_items[0].type, ITEM_RANK_GEM_2);
}

TEST(Play_PlayerShotsHitEnemies_damages_a_boss_by_a_tenth)
{
    StartPlay();
    Enemy *b = AddEnemy(0, 0, ENEMY_BOSS, 400, 100);
    b->hp = 200;
    b->score = 7;
    // the boss's box is x-128.. (16..240 x 16..112 inside it)
    AddShot(0, 400, 150, 30);
    PlayerShotsHitEnemies();
    CHECK_NEAR(b->hp, 197, 1e-4);
    CHECK_EQ_INT(g_mapObjs[0].active, 0);
    CHECK_EQ_INT(PL0.hits, 1);
    AddShot(0, 400, 150, 7);   // at least 1 damage
    PlayerShotsHitEnemies();
    CHECK_NEAR(b->hp, 196, 1e-4);
    AddShot(0, 400 - 128 + 10, 150, 30);   // outside the inner box
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_mapObjs[0].active, 1);
    CHECK_NEAR(b->hp, 196, 1e-4);
}

TEST(Play_PlayerShotsHitEnemies_killing_a_boss_pays_its_score_times_1000)
{
    StartPlay();
    Enemy *b = AddEnemy(0, 0, ENEMY_BOSS, 400, 100);
    b->hp = 2;
    b->score = 7;
    AddShot(0, 400, 150, 30);
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(b->active, 0);
    CHECK_EQ_INT(PL0.score, 7000);
    CHECK_EQ_INT(PL0.killed, 1);
    int coins = 0;
    for (int i = 0; i < MAX_ITEMS; i++)
        coins += g_items[i].alive;
    CHECK(coins >= 8);
}

TEST(Play_PlayerShotsHitEnemies_in_dual_mode_both_shoot_the_shared_wave)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    g_curPlayer = 1;
    AddEnemy(0, 1, ENEMY_DIVING, 290, 290)->hp = 1;
    g_enemies[0][1].score = 30;
    MapObj *s = AddShot(0, 300, 300, 4);
    s->player = 1;
    PlayerShotsHitEnemies();
    CHECK_EQ_INT(g_enemies[0][1].active, 0);
    CHECK_EQ_INT(PL1.score, 30);
    CHECK_EQ_INT(PL1.killed, 1);
    CHECK_EQ_INT(PL0.score, 0);
}

// ---------------------------------------------------------------- shield grabbing

TEST(Play_ShieldGrabEnemies_captures_into_the_left_then_right_slot)
{
    StartPlay();
    PL0.scoopTimer = g_time + 10000;
    AddEnemy(0, 3, ENEMY_HOVER, 300, 480);
    AddEnemy(0, 4, ENEMY_DIVING, 310, 490);
    ShieldGrabEnemies();
    CHECK_EQ_INT(g_grabZoneHit, 1);
    CHECK_EQ_INT(PL0.shieldL, 1);
    CHECK_EQ_INT(PL0.shieldLIdx, 3);
    CHECK_EQ_INT(g_enemies[0][3].type, ENEMY_CAPTURED);
    CHECK_EQ_INT(g_enemies[0][3].beamSide, 0);
    CHECK_EQ_INT(g_enemies[0][3].beamOffsetX, 0);
    CHECK_EQ_INT(PL0.shieldR, 1);
    CHECK_EQ_INT(PL0.shieldRIdx, 4);
    CHECK_EQ_INT(g_enemies[0][4].type, ENEMY_CAPTURED);
    CHECK_EQ_INT(g_enemies[0][4].beamSide, 1);
    CHECK_EQ_INT(g_enemies[0][4].beamOffsetX, 10);
    CHECK_EQ_INT(PL0.escaped, 2);
    CHECK_EQ_INT(Plays(g_sfxCapture), 2);
}

TEST(Play_ShieldGrabEnemies_with_both_slots_full_makes_debris)
{
    StartPlay();
    PL0.scoopTimer = g_time + 10000;
    PL0.shieldL = PL0.shieldR = 1;
    PL0.shieldLIdx = 10;
    PL0.shieldRIdx = 11;
    AddEnemy(0, 3, ENEMY_HOVER, 300, 480);
    ShieldGrabEnemies();
    CHECK_EQ_INT(g_enemies[0][3].type, ENEMY_DEBRIS);
    CHECK_EQ_INT(PL0.shieldLIdx, 10);
    CHECK_EQ_INT(PL0.shieldL, 1);
    CHECK_EQ_INT(PL0.shieldRIdx, 11);
    CHECK_EQ_INT(PL0.shieldR, 1);
    CHECK_EQ_INT(PL0.score, 2500);
    CHECK_EQ_INT(PL0.killed, 1);
    CHECK(g_enemies[0][3].debrisVelY >= -10 && g_enemies[0][3].debrisVelY < -6);
}

TEST(Play_ShieldGrabEnemies_in_a_bonus_wave_counts_kills)
{
    StartPlay();
    PL0.scoopTimer = g_time + 10000;
    PL0.trackKillsFlag = 1;
    PL0.totalEnemies = 50;
    AddEnemy(0, 3, ENEMY_HOVER, 300, 480);
    ShieldGrabEnemies();
    CHECK_EQ_INT(PL0.killed, 1);
    CHECK_EQ_INT(PL0.escaped, 0);
    CHECK_EQ_INT(PL0.bonusKilled, 1);
}

TEST(Play_ShieldGrabEnemies_beam_height_is_90_above_the_ship)
{
    StartPlay();
    PL0.scoopTimer = g_time + 10000;
    // top = 550 - 2 * 45 = 460; an enemy is 32 tall
    AddEnemy(0, 3, ENEMY_HOVER, 300, 428);   // y2 == 460
    AddEnemy(0, 4, ENEMY_HOVER, 300, 550);   // y == ship y
    ShieldGrabEnemies();
    CHECK_EQ_INT(g_grabZoneHit, 0);
    CHECK_EQ_INT(PL0.shieldL, 0);
    g_enemies[0][3].y = 429;
    ShieldGrabEnemies();
    CHECK_EQ_INT(g_grabZoneHit, 1);
    CHECK_EQ_INT(PL0.shieldLIdx, 3);
}

TEST(Play_ShieldGrabEnemies_beam_widens_with_distance)
{
    StartPlay();
    PL0.scoopTimer = g_time + 10000;
    // ship centre 320; at y 480 (70 above) the beam is +-39 wide: 281..359
    AddEnemy(0, 3, ENEMY_HOVER, 249, 480);   // x2 == 281
    AddEnemy(0, 4, ENEMY_HOVER, 359, 480);   // x == 359
    ShieldGrabEnemies();
    CHECK_EQ_INT(g_grabZoneHit, 1);
    CHECK_EQ_INT(PL0.shieldL, 0);
    g_enemies[0][4].x = 358;
    ShieldGrabEnemies();
    CHECK_EQ_INT(PL0.shieldLIdx, 4);
    g_enemies[0][3].x = 250;
    ShieldGrabEnemies();
    CHECK_EQ_INT(PL0.shieldR, 1);
    CHECK_EQ_INT(PL0.shieldRIdx, 3);
}

TEST(Play_ShieldGrabEnemies_needs_the_scoop_and_skips_wrappers)
{
    StartPlay();
    AddEnemy(0, 3, ENEMY_HOVER, 300, 480);
    ShieldGrabEnemies();
    CHECK_EQ_INT(PL0.shieldL, 0);
    CHECK_EQ_INT(g_grabZoneHit, 0);
    PL0.scoopTimer = g_time + 10000;
    g_enemies[0][3].type = ENEMY_WRAPPER;
    ShieldGrabEnemies();
    CHECK_EQ_INT(PL0.shieldL, 0);
    g_enemies[0][3].type = ENEMY_HOVER;
    PL0.dead = 1;
    ShieldGrabEnemies();
    CHECK_EQ_INT(PL0.shieldL, 0);
    PL0.dead = 0;
    ShieldGrabEnemies();
    CHECK_EQ_INT(PL0.shieldL, 1);
}

TEST(Play_ShieldGrabEnemiesBothPlayers_checks_both_players_in_either_order)
{
    StartPlay();
    g_gameMode = MODE_DUAL;
    InitPlayer(1);
    PL0.x = 100;
    PL1.x = 500;
    PL1.y = 550;
    int seen[2] = {0};
    for (int round = 0; round < 30; round++) {
        memset(g_enemies, 0, sizeof g_enemies);
        PL0.shieldL = PL1.shieldL = 0;
        PL0.scoopTimer = PL1.scoopTimer = g_time + 10000;
        AddEnemy(0, 3, ENEMY_HOVER, 100, 480);
        AddEnemy(0, 4, ENEMY_HOVER, 500, 480);
        Rng r = SaveRng();
        int first = RandRange(0, 2);
        RestoreRng(r);
        ShieldGrabEnemiesBothPlayers();
        CHECK_EQ_INT(PL1.shieldL, 1);
        CHECK_EQ_INT(PL1.shieldLIdx, 4);
        CHECK_EQ_INT(g_enemies[0][4].ownerPlayer, 1);
        CHECK_EQ_INT(PL0.shieldL, 1);
        CHECK_EQ_INT(PL0.shieldLIdx, 3);
        CHECK_EQ_INT(g_enemies[0][3].ownerPlayer, 0);
        CHECK_EQ_INT(g_curPlayer, 0);
        seen[first]++;
    }
    CHECK(seen[0] > 0 && seen[1] > 0);
}

// ---------------------------------------------------------------- helpers

TEST(Play_IsEnemyWaveCleared_counts_active_enemies)
{
    StartPlay();
    CHECK_EQ_INT(IsEnemyWaveCleared(), 1);
    AddEnemy(0, 149, ENEMY_HOVER, 0, 0);
    CHECK_EQ_INT(IsEnemyWaveCleared(), 0);
    g_enemies[0][149].active = 2;
    CHECK_EQ_INT(IsEnemyWaveCleared(), 1);
    AddEnemy(1, 0, ENEMY_HOVER, 0, 0);
    g_curPlayer = 1;
    CHECK_EQ_INT(IsEnemyWaveCleared(), 0);
    g_gameMode = MODE_DUAL;
    CHECK_EQ_INT(IsEnemyWaveCleared(), 1);
}

TEST(Play_IsHurryUpEnemyActive_finds_a_money_ship_of_either_player)
{
    StartPlay();
    AddEnemy(0, 1, ENEMY_MOTHERSHIP, 0, 0);
    CHECK_EQ_INT(IsHurryUpEnemyActive(), 0);
    AddEnemy(1, 149, ENEMY_MONEY_SHIP, 0, 0);
    CHECK_EQ_INT(IsHurryUpEnemyActive(), 1);
    g_enemies[1][149].active = 0;
    CHECK_EQ_INT(IsHurryUpEnemyActive(), 0);
}

TEST(Play_BoxOverlap_tests_the_rect_against_the_box_list)
{
    StartPlay();
    g_boxesA[0] = (Box){100, 100, 200, 150};
    g_boxesA[1] = (Box){-1, 0, 0, 0};
    CHECK(BoxOverlap(0, 0, 0, 150, 120, 160, 130));
    CHECK(!BoxOverlap(0, 0, 0, 200, 120, 210, 130));   // touching the right edge
    CHECK(!BoxOverlap(0, 0, 0, 90, 120, 100, 130));    // touching the left edge
    CHECK(BoxOverlap(0, 0, 0, 90, 120, 101, 130));
    CHECK(BoxOverlap(0, 50, 0, 210, 120, 220, 130));   // boxes shifted by x
    CHECK(!BoxOverlap(0, 0, 50, 150, 100, 160, 110));  // shifted down, rect above it
    g_boxesA[0].x0 = -1;
    CHECK(!BoxOverlap(0, 0, 0, 150, 120, 160, 130));
}

TEST(Play_BoxOverlap_set_1_uses_its_own_shifted_boxes_and_terminator)
{
    StartPlay();
    g_boxesA[0] = (Box){100, 100, 200, 150};
    g_boxesA[1] = (Box){-1, 0, 0, 0};
    g_boxesB[0] = (Box){0, 0, 10, 10};
    g_boxesB[1] = (Box){-1, 0, 0, 0};
    CHECK(BoxOverlap(1, 150, 120, 155, 123, 160, 130));
    CHECK(!BoxOverlap(1, 0, 0, 150, 120, 160, 130));
    g_boxesB[0].x0 = -1;
    CHECK(!BoxOverlap(1, 150, 120, 155, 123, 160, 130));
    CHECK(!BoxOverlap(2, 0, 0, 150, 120, 160, 130));
}

TEST(Play_MaskCollide_finds_overlapping_set_pixels)
{
    StartPlay();
    // two 8x8 masks; A has a pixel at (6,6), B at (1,1)
    unsigned char a[64] = {0}, b[64] = {0};
    a[6 * 8 + 6] = 1;
    b[1 * 8 + 1] = 1;
    Rect16 box = {{{0, 0, 8, 8}}};
    // B placed at (5,5) relative to A: A(6,6) meets B(1,1)
    CHECK_EQ_INT(MaskCollide(0, 0, 8, 8, 5, 5, 13, 13, a, b, box, box, 8, 8, 8, 8, 3), 1);
    // at (4,5) they miss
    CHECK_EQ_INT(MaskCollide(0, 0, 8, 8, 4, 5, 12, 13, a, b, box, box, 8, 8, 8, 8, 3), 0);
    // disabled or without a mask: always a hit
    CHECK_EQ_INT(MaskCollide(0, 0, 8, 8, 4, 5, 12, 13, a, b, box, box, 8, 8, 8, 8, 0), 1);
    CHECK_EQ_INT(MaskCollide(0, 0, 8, 8, 4, 5, 12, 13, NULL, b, box, box, 8, 8, 8, 8, 3), 1);
    // B to the upper left of A: B(1,1) at A(-4,-4)... A(6,6) meets B(1,1) when B is at (5,5)
    b[1 * 8 + 1] = 0;
    b[6 * 8 + 6] = 1;
    a[6 * 8 + 6] = 0;
    a[1 * 8 + 1] = 1;
    CHECK_EQ_INT(MaskCollide(5, 5, 13, 13, 0, 0, 8, 8, a, b, box, box, 8, 8, 8, 8, 3), 1);
    CHECK_EQ_INT(MaskCollide(5, 4, 13, 12, 0, 0, 8, 8, a, b, box, box, 8, 8, 8, 8, 3), 0);
}

TEST(Play_MaskCollide2_finds_overlapping_set_pixels)
{
    StartPlay();
    unsigned char a[64] = {0}, b[64] = {0};
    a[6 * 8 + 6] = 1;
    b[1 * 8 + 1] = 1;
    CHECK_EQ_INT(MaskCollide2(0, 0, 8, 8, 5, 5, 13, 13, a, b, 0, 0, 8, 8, 0, 0, 8, 8, 8, 8, 8, 8, 3), 1);
    CHECK_EQ_INT(MaskCollide2(0, 0, 8, 8, 5, 4, 13, 12, a, b, 0, 0, 8, 8, 0, 0, 8, 8, 8, 8, 8, 8, 3), 0);
    CHECK_EQ_INT(MaskCollide2(5, 5, 13, 13, 0, 0, 8, 8, b, a, 0, 0, 8, 8, 0, 0, 8, 8, 8, 8, 8, 8, 3), 1);
    CHECK_EQ_INT(MaskCollide2(0, 0, 8, 8, 5, 4, 13, 12, a, b, 0, 0, 8, 8, 0, 0, 8, 8, 8, 8, 8, 8, 0), 1);
}
