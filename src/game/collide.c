// collide.c: Collisions: pixel masks, items, enemy shots and player shots, shield grabs.
#include <stdio.h>
#include "globals.h"
#include "game.h"

enum {
    GROUP_CLEAR_BONUS_SCORE  = 10000,  // bonus for clearing a whole patterned/escaper group
    CHAIN_ALT_FIRE_BASE_SCORE = 2500,  // alt-fire chain kill bonus, doubled per chained enemy
    PAIRED_KILL_POPUP_SCORE  = 1000,   // popup shown for an unchained paired-enemy kill (see PAIRED_KILL_SCORE)
    PAIRED_KILL_SCORE        = 2500,   // score actually awarded for an unchained paired-enemy kill
    SHIELD_FULL_DEBRIS_SCORE = 2500,   // bonus when a captured enemy has nowhere to go (both shields full)
};

// Tests the current player (g_curPlayer) against every active/alive item for a pixel-mask
// collision; on a hit the item is removed and its effect applied (Pickup). Accounts for
// mirror mode (which may flip the player's effective x for the test) and skips the check
// entirely while the player is dead or hyperspacing.
void ItemsVsPlayer()
{
    int i;
    int x1;
    int y1;
    int x2;
    int y2;
    int px1;
    int py1;
    int px2;
    int py2;

    if (g_save.players[g_curPlayer].dead == 0) {
        if (g_save.players[g_curPlayer].lives > g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy &&
            g_save.players[g_curPlayer].hyperspaceFade < 50.0) {
            if (g_save.players[g_curPlayer].mirrorTime) {
                if (Rand1ff() < 0x100)
                    px1 = (int)g_save.players[g_curPlayer].x;
                else
                    px1 = g_screenW - 40 - (int)g_save.players[g_curPlayer].x;
            } else {
                px1 = (int)g_save.players[g_curPlayer].x;
            }

            py1 = (int)g_save.players[g_curPlayer].y;
            px2 = px1 + 40;
            py2 = py1 + 27;

            for (i = 0; i < MAX_ITEMS; i++) {
                if (g_items[i].alive != 0 && g_items[i].active != 0) {
                    x1 = (int)g_items[i].x;
                    y1 = (int)g_items[i].y;
                    x2 = x1 + g_items[i].w;
                    y2 = y1 + g_items[i].h;
                    if (y2 > py1 && y1 < py2 && x2 > px1 && x1 < px2) {
                        if (MaskCollide(x1, y1, x2, y2, px1, py1, px2, py2,
                                           (unsigned char *)g_items[i].hma,
                                               (unsigned char *)g_save.players[g_curPlayer].hitMask,
                                           g_items[i].rect, g_save.players[g_curPlayer].box,
                                           g_items[i].hmaW, g_save.players[g_curPlayer].hitMaskParamA,
                                           g_items[i].hmaH, g_save.players[g_curPlayer].hitMaskParamB,
                                           g_cfg.collisionDetail)) {
                            g_items[i].alive = 0;
                            Pickup(g_items[i].type);
                        }
                    }
                }
            }
        }
    }
}

// Runs ItemsVsPlayer for both players in a randomized order (so neither player is
// consistently favored on simultaneous pickups), then leaves g_curPlayer at 0.
void ItemsVsBothPlayers()
{
    g_curPlayer = RandRange(0, 2);
    ItemsVsPlayer();
    g_curPlayer = 1 - g_curPlayer;
    ItemsVsPlayer();
    g_curPlayer = 0;
}

// Tests whether the rect (left, top, right, bottom), offset by (x, y), overlaps any box in
// obstruction set `type` (0 = g_boxesA, 1 = g_boxesB); box lists are terminated by an x0 of
// -1. Used to keep spawns/movement clear of level geometry.
bool BoxOverlap(int type, int x, int y, int left, int top, int right, int bottom)
{
    int i;
    int j;

    if (type == 0) {
        for (i = 0; i < 9; i++) {
            if (g_boxesA[i].x0 == -1)
                return false;
            if (right > g_boxesA[i].x0 + x && bottom > g_boxesA[i].y0 + y &&
                left < g_boxesA[i].x1 + x && top < g_boxesA[i].y1 + y)
                return true;
        }
        return false;
    }
    if (type == 1) {
        for (j = 0; j < 9; j++) {
            if (g_boxesB[j].x0 == -1)
                return false;
            if (right > g_boxesB[j].x0 + x && bottom > g_boxesB[j].y0 + y &&
                left < g_boxesB[j].x1 + x && top < g_boxesB[j].y1 + y)
                return true;
        }
        return false;
    }
    return false;
}

// Tests every active level object (enemy shot) against the current player (g_curPlayer):
// first against captured-enemy shields on the left/right (which absorb and destroy the
// shot), then a pixel-mask test against the ship itself. A hit either eats one point of
// armour buffer (brief invulnerability shield flashes on) or, once armour is at minimum,
// kills the player (explosion, respawn timer, stat updates). `hit` limits the visible
// effect to once per frame even though the loop can match multiple objects.
void BulletsVsPlayer()
{
    int i;
    float x;
    float y;
    int bx1;
    int by1;
    int bx2;
    int by2;
    int px1;
    int py1;
    int px2;
    int py2;
    int pl;
    bool hit;

    pl = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        pl = 0;
    hit = false;

    if (g_save.players[g_curPlayer].dead == 0
        && g_save.players[g_curPlayer].shieldTimer == 0
        && !g_cheatGodMode
        && g_save.players[g_curPlayer].lives > g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy) {
        if (g_save.players[g_curPlayer].mirrorTime != 0) {
            if (Rand1ff() < 0x100)
                x = g_save.players[g_curPlayer].x;
            else
                x = (float)(g_screenW - 40) - g_save.players[g_curPlayer].x;
        } else {
            x = g_save.players[g_curPlayer].x;
        }
        y = g_save.players[g_curPlayer].y;
        px1 = (int)x;
        py1 = (int)y;
        px2 = px1 + 40;
        py2 = py1 + 27;

        if (g_profileCollisions)
            g_perfTimerStart = SysPerfCounter();

        for (i = 0; i < MAX_LEVEL_OBJS; i++) {
            if (g_levelObj[i].active) {
                bx1 = (int)g_levelObj[i].x + g_levelObj[i].hitOffsetX;
                by1 = (int)g_levelObj[i].y + g_levelObj[i].hitOffsetY;
                bx2 = bx1 + g_levelObj[i].w;
                by2 = by1 + g_levelObj[i].h;
                if (g_save.players[g_curPlayer].shieldL
                    && g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldLIdx].settled
                    && by2 > py1 && bx1 < px2 - 36 && bx2 > px1 - 36 && by1 < py2) {
                    g_levelObj[i].active = 0;
                    if (g_autoplay && RandRange(0, 1000) < 982)
                        return;
                    if (!hit) {
                        DropAlienGfxAge(g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldLIdx].gfxA);
                        g_save.players[g_curPlayer].shieldL = 0;
                        g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldLIdx].active = 0;
                        g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldLIdx].settled = 0;
                        SoundPlay(g_sfxExplo1, RandRange(35000, 44100), 255, g_panTable[ClampX((int)x)],
                            RandRange(216, 190), g_sndFlags);
                        SpawnExplosion(x - 32.0, y, 32, 32, 150, 3, 2, 255, 255, 0, 255, 255, 0);
                        hit = true;
                    }
                }

                if (g_save.players[g_curPlayer].shieldR
                    && g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldRIdx].settled
                    && by2 > py1 && bx1 < px2 + 36 && bx2 > px1 + 36 && by1 < py2) {
                    g_levelObj[i].active = 0;
                    if (g_autoplay && RandRange(0, 1000) < 982)
                        return;
                    if (!hit) {
                        DropAlienGfxAge(g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldRIdx].gfxA);
                        g_save.players[g_curPlayer].shieldR = 0;
                        g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldRIdx].active = 0;
                        g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldRIdx].settled = 0;
                        SoundPlay(g_sfxExplo1, RandRange(35000, 44100), 255, g_panTable[ClampX((int)x)],
                            RandRange(216, 190), g_sndFlags);
                        SpawnExplosion(x + 32.0, y, 32, 32, 150, 3, 2, 255, 255, 0, 255, 255, 0);
                        hit = true;
                    }
                }

                if (g_levelObj[i].active && by2 > py1 && bx1 < px2 && bx2 > px1 && by1 < py2) {
                    if (MaskCollide(bx1, by1, bx2, by2, px1, py1, px2, py2,
                                       (unsigned char *)g_levelObj[i].hma,
                                           (unsigned char *)g_save.players[g_curPlayer].hitMask,
                                       g_levelObj[i].rect, g_save.players[g_curPlayer].box,
                                       g_levelObj[i].hmaW, g_save.players[g_curPlayer].hitMaskParamA,
                                       g_levelObj[i].hmaH, g_save.players[g_curPlayer].hitMaskParamB,
                                       g_cfg.collisionDetail)) {
                        if (g_save.players[g_curPlayer].shieldL == 0 &&
                            g_save.players[g_curPlayer].shieldR == 0) {
                            if (g_save.players[g_curPlayer].armour <
                                g_shipDefs[g_save.players[g_curPlayer].ship]->baseArmour +
                                g_shipDefs[g_save.players[g_curPlayer].ship]->armourStep) {
                                g_levelObj[i].active = 0;
                                if (g_autoplay && RandRange(0, 1000) < 982)
                                    return;

                                if (!hit) {
                                    SoundPlay(g_sfxExplo4, -1, 255, 0.0f, RandRange(191, 255), g_sndFlags);
                                    if (g_curPlayer == 0)
                                        SpawnBigExplosion(x, y, 32, 32, 1, 2, 255, 200, 50);
                                    if (g_curPlayer == 1)
                                        SpawnBigExplosion(x, y, 32, 32, 1, 2, 50, 100, 255);
                                    SoundPlay(g_sfxExplo4, -1, 255, 0.0f, RandRange(191, 255), g_sndFlags);
                                    hit = true;
                                    if (g_curPlayer == 0)
                                        SpawnHugeExplosion(x, y, 32, 32, 3, 2, 255, 200, 50, 2);
                                    if (g_curPlayer == 1)
                                        SpawnHugeExplosion(x, y, 32, 32, 3, 2, 50, 100, 255, 2);
                                    SpawnSlots(x + 20.0, y + 13.0, 750, 255, 255, 255);

                                    g_save.players[g_curPlayer].deaths++;
                                    g_save.players[g_curPlayer].dead = 1;
                                    g_save.players[g_curPlayer].respawnTime = g_time + 3000;
                                    g_save.players[g_curPlayer].drunkModeTimer = 0;
                                    g_save.players[g_curPlayer].scoopTimer = 0;
                                    g_save.players[g_curPlayer].mirrorTime = 0;
                                    g_save.players[g_curPlayer].writeOnlyMirrorFlag = 0;
                                    g_chanShieldHum = SoundStop(g_chanShieldHum);
                                    g_chanScopeHum = SoundStop(g_chanScopeHum);
                                    g_deathsCount += 2;
                                }
                            } else {
                                g_levelObj[i].active = 0;
                                if (!hit) {
                                    g_viewTransitionFlag = 2;
                                    g_stateFn = SetViewHud;
                                    EmptyViewChangeHook();
                                    g_drawBordersFn = DrawBorders;
                                    SpawnSlots(x + 20.0, y + 13.0, 500, 255, 0, 0);

                                    g_save.players[g_curPlayer].shieldTimer =
                                        g_save.players[g_curPlayer].buffDuration / 5 * 1000 + g_time;
                                    g_save.players[g_curPlayer].armour -=
                                        g_shipDefs[g_save.players[g_curPlayer].ship]->armourStep;
                                    g_save.players[g_curPlayer].collisionsTaken++;
                                    SoundPlay(g_sfxExplo2, -1, 255, g_panTable[ClampX((int)x)],
                                        RandRange(191, 255), g_sndFlags);
                                    g_chanShieldHum = SoundPlayChannel(g_chanShieldHum, g_sfxShieldHum, -1,
                                        180, 0.0f, 223);
                                    hit = true;
                                }
                            }
                        } else {
                            g_levelObj[i].active = 0;
                            if (g_autoplay && RandRange(0, 1000) < 982)
                                return;

                            if (!hit) {
                                hit = true;
                                if (g_save.players[g_curPlayer].shieldL) {
                                    DropAlienGfxAge(
                                        g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldLIdx].gfxA);
                                    g_save.players[g_curPlayer].shieldL = 0;
                                    g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldLIdx].active = 0;
                                    g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldLIdx].settled =
                                        0;

                                    SoundPlay(g_sfxExplo1, RandRange(35000, 44100), 255,
                                        g_panTable[ClampX((int)x)], RandRange(191, 255), g_sndFlags);
                                    SpawnExplosion(x - 32.0, y, 32, 32, 150, 3, 2, 255, 255, 0, 255, 255, 0);
                                } else {
                                    DropAlienGfxAge(
                                        g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldRIdx].gfxA);
                                    g_save.players[g_curPlayer].shieldR = 0;
                                    g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldRIdx].active = 0;
                                    g_enemies[g_curPlayer][g_save.players[g_curPlayer].shieldRIdx].settled =
                                        0;
                                    SoundPlay(g_sfxExplo1, RandRange(35000, 44100), 255,
                                        g_panTable[ClampX((int)x)], RandRange(191, 255), g_sndFlags);
                                    SpawnExplosion(x + 32.0, y, 32, 32, 150, 3, 2, 255, 255, 0, 255, 255, 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// Runs BulletsVsPlayer for both players in a randomized order, then leaves g_curPlayer at 0.
void BulletsVsBothPlayers()
{
    g_curPlayer = RandRange(0, 2);
    BulletsVsPlayer();
    g_curPlayer = 1 - g_curPlayer;
    BulletsVsPlayer();
    g_curPlayer = 0;
}

// Returns 1 if the current player's (or player 0's, in versus mode) enemy wave has no
// active enemies left, else 0.
int IsEnemyWaveCleared()
{
    int i;
    int count;
    int p;
    p = g_curPlayer;
    if (g_gameMode == MODE_DUAL)
        p = 0;
    count = 0;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[p][i].active == 1)
            count++;
    }
    if (count == 0)
        return 1;
    else
        return 0;
}

// Returns true if either player has an active "hurry up" enemy (type 12) alive.
bool IsHurryUpEnemyActive()
{
    bool r;
    int p;
    int i;
    r = false;
    for (p = 0; p < 2; p++) {
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (g_enemies[p][i].active != 0 && g_enemies[p][i].type == ENEMY_MONEY_SHIP)
                return true;
        }
    }
    return r;
}

// Halves a laser shot's damage and, once spent, deactivates it plus its chained link1/link2
// segments (each refunding its energy cost to its owner); while the laser survives, it plays
// the "current" hum sound instead. A non-laser shot just deactivates itself and releases the
// enemy it had locked. `pan` is the current-sound's stereo position, the one thing that
// differs between this macro's three call sites.
#define LASER_DECAY_OR_UNLOCK(pan)                                                              \
    if (g_mapObjs[i].laser != 0) {                                                             \
        g_mapObjs[i].dmg /= 2.0;                                                                \
        if (g_mapObjs[i].dmg <= 0.0) {                                                          \
            g_mapObjs[i].active = 0;                                                            \
            g_save.players[g_mapObjs[i].player].energy -= g_mapObjs[i].cost;                    \
            if (g_save.players[g_mapObjs[i].player].energy < 0)                                 \
                g_save.players[g_mapObjs[i].player].energy = 0;                                 \
            if (g_mapObjs[i].laser != 0) {                                                      \
                if (g_mapObjs[i].link1 != -1) {                                                 \
                    g_mapObjs[g_mapObjs[i].link1].active = 0;                                   \
                    g_save.players[g_mapObjs[g_mapObjs[i].link1].player].energy -=              \
                        g_mapObjs[g_mapObjs[i].link1].cost;                                     \
                    if (g_save.players[g_mapObjs[g_mapObjs[i].link1].player].energy < 0)        \
                        g_save.players[g_mapObjs[g_mapObjs[i].link1].player].energy = 0;        \
                }                                                                                \
                if (g_mapObjs[i].link2 != -1) {                                                 \
                    g_mapObjs[g_mapObjs[i].link2].active = 0;                                   \
                    g_save.players[g_mapObjs[g_mapObjs[i].link2].player].energy -=              \
                        g_mapObjs[g_mapObjs[i].link2].cost;                                     \
                    if (g_save.players[g_mapObjs[g_mapObjs[i].link2].player].energy < 0)        \
                        g_save.players[g_mapObjs[g_mapObjs[i].link2].player].energy = 0;        \
                }                                                                                \
            }                                                                                    \
        } else {                                                                                 \
            if (g_sndTCurrent < g_time) {                                                       \
                g_sndTCurrent = RandRange(60, 300) + g_time;                                    \
                SoundPlay(g_sfxCurrent, RandRange(10000, 20100), 100, pan, 127, g_sndFlags);     \
            }                                                                                    \
        }                                                                                        \
    } else {                                                                                     \
        g_mapObjs[i].active = 0;                                                                \
        g_enemies[p][g_mapObjs[i].enemy].locked = 0;                                            \
        g_save.players[g_mapObjs[i].player].energy -= g_mapObjs[i].cost;                        \
        if (g_save.players[g_mapObjs[i].player].energy < 0)                                     \
            g_save.players[g_mapObjs[i].player].energy = 0;                                     \
    }

// Tests every active player shot/laser (g_mapObjs) against every active enemy (g_enemies)
// for the current player (or player 0 in versus mode). Type-13 enemies (bonus targets) use a
// simple offset-box hit test; everything else uses a pixel-mask test against the enemy's hit
// rect (with a few per-type rect overrides). On a hit: applies damage (halving a laser's dmg
// and detaching it once spent, or consuming the shot outright), credits hits/score, and on a
// kill spawns the right explosion/items/score popup for that enemy type, handles chained
// "hurry up" (type 12) area kills, kill-combo bonuses, and paired-enemy bonuses. `kills`
// counts kills this call so enemy fire rates can be nudged up afterward.
void PlayerShotsHitEnemies()
{
    __int64 chainBonus;
    int i;
    int j;
    int c;
    int xp[10];
    int yp[10];
    float ex;
    float ey;
    float fxKill = 0;
    float fyKill = 0;
    float sx;
    float sy;
    float sx2;
    float sy2;
    float ew;
    float eh;

    int kills = 0;
    int p = g_curPlayer;
    int savedFlag = 0;
    float dmg;
    int n;
    int k;
    int m;
    __int64 killScore;

    if (g_gameMode == MODE_DUAL)
        p = 0;
    kills = 0;

    for (i = 0; i < MAX_MAP_OBJS; i++) {
        if (g_mapObjs[i].active != 0) {
            // ---- shot hitbox setup ----
            sx = g_mapObjs[i].x;
            sx2 = g_mapObjs[i].w + sx;
            sy = g_mapObjs[i].y;
            sy2 = g_mapObjs[i].stepY + sy;
            if (g_mapObjs[i].laser != 0) {
                sy = 0;
                sy2 = g_save.players[g_mapObjs[i].player].y;
            }

            for (j = 0; j < MAX_ENEMIES; j++) {
                if (g_enemies[p][j].active == 1 && g_enemies[p][j].type != ENEMY_CAPTURED &&
                    g_enemies[p][j].type != ENEMY_DEBRIS && g_enemies[p][j].attackStaggerTimer < 1.0 &&
                    g_mapObjs[i].active != 0) {
                    // ---- boss branch ----
                    if (g_enemies[p][j].type == ENEMY_BOSS) {
                        xp[0] = (sx + sx2) / 2.0 - (g_enemies[p][j].x - 128.0);
                        yp[0] = (sy + sy2) / 2.0 - g_enemies[p][j].y;
                        xp[1] = xp[0];
                        yp[1] = yp[0];
                        if (g_mapObjs[i].laser != 0 && g_enemies[p][j].y > 0.0 &&
                            g_enemies[p][j].y < g_screenH)
                            yp[0] = 60;

                        if (xp[0] > 16 && yp[0] > 16 && xp[0] < 240 && yp[0] < 112) {
                            if (g_cfg.particlesOn == 0)
                                SpawnSmall(g_enemies[p][j].x - 128.0 + xp[0], g_enemies[p][j].y + yp[0]);
                            else
                                AddParticle(g_gfxFlareBomb, (int)(g_enemies[p][j].x - 128.0) + xp[0],
                                          (int)g_enemies[p][j].y + yp[0], RandFloat(10.0f, 30.0f),
                                          RandFloat(10.8f, 15.6f), RandFloat(0.0f, 359.0f),
                                          RandFloat(0.0f, 40.0f) - 20.0, 0, RandRange(150, 255),
                                          RandRange(0, 150), RandRange(0, 55), 600,
                                          RandFloat(8.0f, 13.0f), 0.0f, 10, 0.0f, 0, 0, 0, 1);
                            if (g_sndTHit1 < g_time) {
                                g_sndTHit1 = RandRange(100, 300) + g_time;
                                SoundPlay(g_sfxHit1, RandRange(25000, 44100), 250,
                                                  g_panTable[ClampX((int)(g_enemies[p][j].x - 128.0) +
                                                      xp[0])],
                                                  127, g_sndFlags);
                            }

                            dmg = g_mapObjs[i].dmg / 10.0;
                            if (dmg <= 1.0)
                                dmg = 1.0;
                            g_enemies[p][j].hp -= dmg;
                            if (g_enemies[p][j].hp < 50.0) {
                                if (g_sndTHit2 < g_time) {
                                    g_sndTHit2 = RandRange(100, 300) + g_time;
                                    SoundPlay(g_sfxHit2, RandRange(25000, 44100), 200,
                                                      g_panTable[ClampX((int)(g_enemies[p][j].x - 128.0) +
                                                          xp[0])],
                                                      127, g_sndFlags);
                                }
                                SpawnSparks(RandRange(10, 20),
                                          g_enemies[p][j].x - 128.0 + RandRange(0, 224),
                                          g_enemies[p][j].y + 16.0 + RandRange(0, 96),
                                          1.0f, 6.0f, 2.0f, 3.0f);
                            }

                            if (g_enemies[p][j].hp <= 0.0) {
                                n = RandRange(8, 15);
                                for (k = 0; k < n; k++)
                                    SpawnItem(g_enemies[p][j].x - 128.0 + RandRange(64, 192),
                                              g_enemies[p][j].y + RandRange(-32, 32),
                                              g_save.players[g_mapObjs[i].player].color);
                                g_enemies[p][j].active = 0;

                                SpawnPowerupBurst((int)g_enemies[p][j].x, (int)g_enemies[p][j].y, 1, 1);
                                SpawnHugeExplosion(g_enemies[p][j].x - 128.0, g_enemies[p][j].y,
                                          256, 128, 3, 10, 255, 200, 255, 1);
                                SpawnHugeExplosion(g_enemies[p][j].x - 128.0, g_enemies[p][j].y - 32.0,
                                          256, 128, 0, 0, 255, 0, 0, 0);
                                SpawnSlots(g_enemies[p][j].x - 128.0 + 128.0, g_enemies[p][j].y + 64.0,
                                                  500, 100, 255, 100);
                                SpawnSparks(RandRange(50, 100),
                                          g_enemies[p][j].x - 128.0 + RandFloat(0.0f, 224.0f),
                                          g_enemies[p][j].y + 16.0 + RandFloat(0.0f, 96.0f),
                                          1.0f, 6.0f, 2.0f, 3.0f);
                                SpawnSparks(RandRange(50, 100),
                                          g_enemies[p][j].x - 128.0 + RandFloat(0.0f, 224.0f),
                                          g_enemies[p][j].y + 16.0 + RandFloat(0.0f, 96.0f),
                                          1.0f, 6.0f, 2.0f, 3.0f);

                                if (g_sndTExplo4 < g_time) {
                                    g_sndTExplo4 = RandRange(40, 200) + g_time;
                                    SoundPlay(g_sfxExplo4, RandRange(35000, 44100), 255, 0.0f,
                                                      RandRange(216, 190), g_sndFlags);
                                    SoundPlay(g_sfxExplo4, RandRange(30000, 40100), 255, 0.0f,
                                                      RandRange(216, 190), g_sndFlags);
                                    SoundPlay(g_sfxExplo4, RandRange(25000, 30100), 255, 0.0f,
                                                      RandRange(216, 190), g_sndFlags);
                                }

                                ADD_PLAYER_SCORE(g_save.players[g_mapObjs[i].player].score,
                                    g_mapObjs[i].player, g_enemies[p][j].score * 1000);
                                AddScorePopup(g_screenW >> 1, g_screenH >> 1,
                                                     g_enemies[p][j].score * 1000, 1);
                                CreditKill(g_mapObjs[i].player);
                                g_samples[p][j] = SoundStop(g_samples[p][j]);
                            }

                            LASER_DECAY_OR_UNLOCK(0.0f)

                            g_save.players[g_mapObjs[i].player].hits++;
                            g_hits++;
                        }
                    } else {
                        // ---- non-boss hitbox per enemy type ----
                        ex = g_enemies[p][j].x + g_enemies[p][j].hitRect.x1;
                        ey = g_enemies[p][j].y + g_enemies[p][j].hitRect.y1;
                        eh = g_enemies[p][j].hitRect.y2;
                        ew = g_enemies[p][j].hitRect.x2;
                        switch (g_enemies[p][j].type) {

                        case ENEMY_HOVER:
                            ex = g_enemies[p][j].x + g_enemies[p][j].offsetX + g_enemies[p][j].hitRect.x1;
                            ey = g_enemies[p][j].y + g_enemies[p][j].offsetY + g_enemies[p][j].hitRect.y1;
                            eh = g_enemies[p][j].hitRect.y2;
                            ew = g_enemies[p][j].hitRect.x2;
                            break;

                        case ENEMY_PATTERNED:
                        case ENEMY_DIVING:
                        case ENEMY_FLYBY:
                        case ENEMY_UNKNOWN_7:
                        case ENEMY_ESCAPER:
                            ex = g_enemies[p][j].x + g_enemies[p][j].hitRect.x1;
                            ey = g_enemies[p][j].y + g_enemies[p][j].hitRect.y1;
                            eh = g_enemies[p][j].hitRect.y2;
                            ew = g_enemies[p][j].hitRect.x2;
                            break;

                        case ENEMY_WRAPPER:
                            ex = g_enemies[p][j].x + g_enemies[p][j].hitRect.x1;
                            ey = g_enemies[p][j].y + g_enemies[p][j].hitRect.y1;
                            eh = g_enemies[p][j].hitRect.y2;
                            ew = g_enemies[p][j].hitRect.x2;
                            break;

                        case ENEMY_MONEY_SHIP:
                            ex = g_enemies[p][j].x + 14.0;
                            ey = g_enemies[p][j].y + 14.0;
                            eh = 100.0f;
                            ew = 100.0f;
                            break;

                        case ENEMY_MOTHERSHIP:
                            ex = g_enemies[p][j].x;
                            ey = g_enemies[p][j].y;
                            eh = 57.0f;
                            ew = 96.0f;
                            break;

                        case ENEMY_MONEY_SUCKER:
                            ex = g_enemies[p][j].x;
                            ey = g_enemies[p][j].y;
                            eh = 50.0f;
                            ew = 128.0f;
                            break;

                        case ENEMY_GUARD:
                            ex = g_enemies[p][j].x;
                            ey = g_enemies[p][j].y;
                            eh = 64.0f;
                            ew = 128.0f;
                            break;
                        }

                        // ---- mask test and damage ----
                        if (sx2 > ex && sx < ex + ew && sy2 > ey && sy < ey + eh) {
                            if (g_mapObjs[i].laser != 0) {
                                savedFlag = g_cfg.collisionDetail;
                                g_cfg.collisionDetail = 0;
                            }

                            if (MaskCollide((int)sx, (int)sy, (int)sx2, (int)sy2, (int)ex, (int)ey, (int)(ex +
                                ew), (int)(ey + eh),
                                          (unsigned char *)g_hmaWeaponsBig,
                                              (unsigned char *)g_enemies[p][j].shotFrame,
                                              g_mapObjs[i].srcRect,
                                          g_enemies[p][j].srcRect, g_gfxWeaponsBigW, g_enemies[p][j].shotGfxW,
                                              g_gfxWeaponsBigH,
                                          g_enemies[p][j].shotGfxH, g_cfg.collisionDetail)) {
                                if (g_mapObjs[i].laser != 0)
                                    g_cfg.collisionDetail = savedFlag;
                                g_enemies[p][j].hp -= g_mapObjs[i].dmg;

                                if (g_enemies[p][j].hp <= 0.0) {
                                    LASER_DECAY_OR_UNLOCK(0.0f)

                                    g_save.players[g_mapObjs[i].player].hits++;
                                    g_hits++;

                                    if (g_enemyDamageStage[p][j] != 0 && g_enemies[p][j].type != ENEMY_WRAPPER &&
                                        g_enemies[p][j].attackDelay != 0 &&
                                        g_mapObjs[i].state != MAPOBJ_STATE_HOMING) {
                                        AddParticle(g_gfxSpark, (int)g_enemies[p][j].x,
                                            (int)g_enemies[p][j].y + 16, 32.0f,
                                                  RandFloat(10.0f, 30.0f), RandFloat(0.0f, 359.0f),
                                                      RandFloat(10.0f, 60.0f),
                                                  0, 255, 128, 255, 1000, 15.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                                        g_enemies[p][j].hp = g_enemies[p][j].maxHp;
                                        if (g_enemyDamageStage[p][j] == 1) {
                                            g_enemies[p][j].hitFlashGfxA = g_alienGfxCache[1].gfx1;
                                            g_enemies[p][j].hitFlashGfxB = g_alienGfxCache[1].gfx2;
                                            g_enemies[p][j].gfxA = *(Image **)&g_alienGfxCache;
                                            g_enemies[p][j].gfxB = g_alienGfxCache[0].gfx2;
                                            g_enemyDamageStage[p][j] = 0;
                                        }

                                        if (g_enemyDamageStage[p][j] == 2) {
                                            g_enemies[p][j].hitFlashGfxA = g_alienGfxCache[2].gfx1;
                                            g_enemies[p][j].hitFlashGfxB = g_alienGfxCache[2].gfx2;
                                            g_enemies[p][j].gfxA = g_alienGfxCache[1].gfx1;
                                            g_enemies[p][j].gfxB = g_alienGfxCache[1].gfx2;
                                            g_enemyDamageStage[p][j] = 0;
                                        }

                                        if (g_enemyDamageStage[p][j] == 3) {
                                            g_enemies[p][j].hitFlashGfxA = g_alienGfxCache[3].gfx1;
                                            g_enemies[p][j].hitFlashGfxB = g_alienGfxCache[3].gfx2;
                                            g_enemies[p][j].gfxA = g_alienGfxCache[2].gfx1;
                                            g_enemies[p][j].gfxB = g_alienGfxCache[2].gfx2;
                                            g_enemyDamageStage[p][j] = 0;
                                        }

                                        if (g_enemyDamageStage[p][j] == 4) {
                                            g_enemies[p][j].hitFlashGfxA = g_alienGfxCache[4].gfx1;
                                            g_enemies[p][j].hitFlashGfxB = g_alienGfxCache[4].gfx2;
                                            g_enemies[p][j].gfxA = g_alienGfxCache[3].gfx1;
                                            g_enemies[p][j].gfxB = g_alienGfxCache[3].gfx2;
                                            g_enemyDamageStage[p][j] = 0;
                                        }

                                        if (g_enemyDamageStage[p][j] == 5) {
                                            g_enemies[p][j].hitFlashGfxA = g_alienGfxCache[5].gfx1;
                                            g_enemies[p][j].hitFlashGfxB = g_alienGfxCache[5].gfx2;
                                            g_enemies[p][j].gfxA = g_alienGfxCache[4].gfx1;
                                            g_enemies[p][j].gfxB = g_alienGfxCache[4].gfx2;
                                            g_enemyDamageStage[p][j] = 0;
                                        }

                                        g_enemies[p][j].hitFlashTimer = 100;
                                        break;
                                    }

                                    // ---- kill ----
                                    g_enemies[p][j].active = 0;

                                    // hurry-up (type 12) area kill: chain-kills every other enemy too
                                    if (g_enemies[p][j].fixedFireDelay != 0) {
                                        kills++;
                                        CreditKill(g_mapObjs[i].player);
                                        SpawnHugeExplosion(g_enemies[p][j].x + g_enemies[p][j].offsetX - 64.0,
                                                  g_enemies[p][j].y + g_enemies[p][j].offsetY - 64.0,
                                                  128, 128, 3, 0, 255, 0, 255, 1);
                                        for (m = 0; m < MAX_ENEMIES; m++) {
                                            if (g_enemies[p][m].active == 1 && g_enemies[p][m].type != ENEMY_CAPTURED &&
                                                g_enemies[p][m].type != ENEMY_MOTHERSHIP && g_enemies[p][m].type != ENEMY_DEBRIS &&
                                                g_enemies[p][m].attackStaggerTimer < 1.0) {
                                                if ((g_enemies[p][m].hp -= 100.0) <= 0.0) {
                                                    g_flashOverlayActive = 1;
                                                    g_fadeStep = 0;
                                                    *((int *)&g_fadeStep + 1) = 0;
                                                    g_enemies[p][m].active = 0;
                                                    g_enemies[p][m].forcedDir = -1;
                                                    CreditKill(g_mapObjs[i].player);
                                                    kills++;
                                                    if (g_save.players[p].trackKillsFlag != 0)
                                                        g_save.players[g_mapObjs[i].player].bonusKilled++;

                                                    if (g_enemies[p][m].type == ENEMY_WRAPPER) {
                                                        fxKill = g_enemies[p][m].x;
                                                        fyKill = g_enemies[p][m].y;
                                                        if (g_sndTExplo2 < g_time) {
                                                            g_sndTExplo2 = RandRange(30, 300) + g_time;
                                                            SoundPlay(g_sfxExplo2, -1, 200,
                                                                g_panTable[ClampX((int)fxKill)],
                                                                              RandRange(216, 190),
                                                                                  g_sndFlags);
                                                        }
                                                        SpawnBigExplosion(ex, ey, 64, 64, 0, 6, 0, 255, 0);
                                                        SpawnItem(fxKill, fyKill,
                                                            g_save.players[g_mapObjs[i].player].color);
                                                        SpawnSlots(ex + 32.0, ey + 32.0, 30, 255, 0, 255);
                                                    }

                                                    if (g_enemies[p][m].type == ENEMY_HOVER) {
                                                        fxKill = g_enemies[p][m].x + g_enemies[p][m].offsetX;
                                                        fyKill = g_enemies[p][m].y + g_enemies[p][m].offsetY;
                                                        if (g_sndTExplo1 < g_time) {
                                                            g_sndTExplo1 = RandRange(40, 200) + g_time;
                                                            SoundPlay(g_sfxExplo1, RandRange(35000, 44100),
                                                                210,
                                                                              g_panTable[ClampX((int)fxKill)],
                                                                              RandRange(216, 190),
                                                                                  g_sndFlags);
                                                        }
                                                        SpawnExplosion(fxKill, fyKill, 32, 32, 150, 0, 6, 0,
                                                            255, 0, 0, 255, 0);
                                                        SpawnGem(fxKill + 9.0, fyKill + 2.0);
                                                        SpawnSlots(fxKill + 16.0, fyKill + 16.0, 50, 255, 0,
                                                            255);

                                                    } else {
                                                        fxKill = g_enemies[p][m].x;
                                                        fyKill = g_enemies[p][m].y;
                                                        if (g_sndTExplo1 < g_time) {
                                                            g_sndTExplo1 = RandRange(40, 200) + g_time;
                                                            SoundPlay(g_sfxExplo1, RandRange(35000, 44100),
                                                                210,
                                                                              g_panTable[ClampX((int)fxKill)],
                                                                              RandRange(216, 190),
                                                                                  g_sndFlags);
                                                        }
                                                        SpawnExplosion(fxKill, fyKill, 32, 32, 150, 0, 6, 0,
                                                            255, 0, 0, 255, 0);
                                                        SpawnGem(fxKill + 9.0, fyKill + 2.0);
                                                        SpawnSlots(fxKill + 16.0, fyKill + 16.0, 50, 255, 0,
                                                            255);
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        // single kill: per-type death explosion/items/score
                                        fxKill = g_enemies[p][j].x;
                                        switch (g_enemies[p][j].type) {

                                        case ENEMY_HOVER:  // explosion/particle, drops a bonus roll, tracks kill combo
                                            fxKill += g_enemies[p][j].offsetX;
                                            SpawnExplosion(g_enemies[p][j].x + g_enemies[p][j].offsetX,
                                                      g_enemies[p][j].y + g_enemies[p][j].offsetY, 32, 32,
                                                      g_enemies[p][j].deathExplosionLife, 3,
                                                          g_enemies[p][j].deathExplosionGfx, 255, 255, 255,
                                                      g_enemies[p][j].deathExplosionR,
                                                          g_enemies[p][j].deathExplosionG,
                                                          g_enemies[p][j].deathExplosionB);
                                            AddParticle(g_gfxTable[g_enemies[p][j].deathExplosionGfx],
                                                      (int)(g_enemies[p][j].x + g_enemies[p][j].offsetX) + 16,
                                                      (int)(g_enemies[p][j].y + g_enemies[p][j].offsetY) + 16,
                                                          16.0f,
                                                      RandFloat(0.0f, 5.0f) + 20.0, RandFloat(0.0f, 359.0f),
                                                          0.0f, 0,
                                                      g_enemies[p][j].deathExplosionR,
                                                          g_enemies[p][j].deathExplosionG,
                                                          g_enemies[p][j].deathExplosionB,
                                                      600, 15.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);

                                            if (g_sndTExplo1 < g_time) {
                                                g_sndTExplo1 = RandRange(40, 200) + g_time;
                                                SoundPlay(g_sfxExplo1, RandRange(35000, 44100), 200,
                                                                  g_panTable[ClampX((int)fxKill)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }

                                            kills++;
                                            CreditKill(g_mapObjs[i].player);
                                            if (RandRange(1, (int)g_diffBonusDropRoll) < 4)
                                                SpawnBonus(ex, ey);
                                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                                g_save.players[g_mapObjs[i].player].bonusKilled++;
                                            break;

                                        case ENEMY_PATTERNED:  // group-kill and paired-enemy combo bonuses (falls through)
                                        case ENEMY_ESCAPER:
                                            if (g_save.players[g_curPlayer].shipDestroyedThisLevel == 0) {
                                                g_typeKillCombo[g_enemies[p][j].groupIndex]++;
                                                if (g_typeKillCombo[g_enemies[p][j].groupIndex] ==
                                                    g_groupEnemyCount[g_enemies[p][j].groupIndex]) {
                                                    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE &&
                                                        g_playerUpdateFn != StateDemo) {
                                                        g_save.players[g_curPlayer].secretFound17 = 1;
                                                        MarkSecretFound(g_profileIndex, 17);
                                                    }

                                                    g_typeKillCombo[g_enemies[p][j].groupIndex] = 0;
                                                    ADD_PLAYER_SCORE(g_save.players[g_mapObjs[i].player].score,
                                                        g_mapObjs[i].player, GROUP_CLEAR_BONUS_SCORE);

                                                    AddScorePopup((int)ex, (int)ey, GROUP_CLEAR_BONUS_SCORE, 0);
                                                    SoundQueueAdd(g_sfxBonus, 50, 0);
                                                }
                                            }

                                            if (g_save.players[g_curPlayer].shipDestroyedThisLevel != 0) {
                                                g_bonusKillCombo[g_enemies[p][j].bonusGroupIndex]++;
                                                if (g_bonusKillCombo[g_enemies[p][j].bonusGroupIndex] ==
                                                    g_groupKillCount[g_enemies[p][j].bonusGroupIndex]) {
                                                    g_bonusKillCombo[g_enemies[p][j].bonusGroupIndex] = 0;
                                                    ADD_PLAYER_SCORE(g_save.players[g_mapObjs[i].player].score,
                                                        g_mapObjs[i].player,
                                                        g_save.players[g_mapObjs[i].player].chainBonusValue);

                                                    AddScorePopup((int)ex, (int)ey,
                                                        g_save.players[g_mapObjs[i].player].chainBonusValue,
                                                        0);
                                                    g_save.players[g_mapObjs[i].player].chainBonusValue =
                                                        g_save.players[g_mapObjs[i].player].chainBonusValue *
                                                            2;
                                                }
                                            }

                                        case ENEMY_DIVING:
                                        case ENEMY_FLYBY:
                                        case ENEMY_UNKNOWN_7:
                                            if (g_enemies[p][j].altFireActive != 0) {
                                                chainBonus = CHAIN_ALT_FIRE_BASE_SCORE;
                                                g_enemies[p][j].altFireActive = 0;

                                                for (c = 0; c < 150; c++) {
                                                    if (g_enemies[p][c].active != 0 &&
                                                        g_enemies[p][c].pairedEnemyIdx == j) {
                                                        chainBonus = chainBonus * 2;
                                                        g_enemies[p][c].pairedEnemyIdx = -1;
                                                        g_enemies[p][c].active = 0;
                                                        SpawnExplosion(g_enemies[p][c].x, g_enemies[p][c].y,
                                                            32, 32,
                                                                  g_enemies[p][j].deathExplosionLife, 3,
                                                                      g_enemies[p][j].deathExplosionGfx,
                                                                  50, 255, 50, 50, 255, 50);
                                                        AddParticle(g_gfxTable[g_enemies[p][j]
                                                            .deathExplosionGfx],
                                                                  (int)g_enemies[p][c].x + 16,
                                                                      (int)g_enemies[p][c].y + 16, 16.0f,
                                                                  RandFloat(0.0f, 5.0f) + 20.0,
                                                                      RandFloat(0.0f, 359.0f), 0.0f, 0,
                                                                  80, 255, 80, 600, 15.0f, 0.0f, -1, 0.0f, 0,
                                                                      0, 0, 1);
                                                        kills++;
                                                        CreditKill(g_mapObjs[i].player);
                                                    }
                                                }

                                                if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE &&
                                                    g_playerUpdateFn != StateDemo) {
                                                    g_save.players[g_curPlayer].secretFound18 = 1;
                                                    MarkSecretFound(g_profileIndex, 18);
                                                }

                                                if (chainBonus > 50000)
                                                    chainBonus = 50000;
                                                if (chainBonus < 0)
                                                    chainBonus = 0;
                                                AddScorePopup((int)(ew / 2.0 + ex), (int)ey - 10, chainBonus,
                                                    0);

                                                ADD_PLAYER_SCORE(g_save.players[g_mapObjs[i].player].score,
                                                    g_mapObjs[i].player, chainBonus);

                                                SoundQueueAdd(g_sfxBonus, 50, 0);
                                            }

                                            if (g_enemies[p][j].pairedEnemyIdx > -1) {
                                                g_enemies[p][j].pairedEnemyIdx = -1;
                                                AddScorePopup((int)(ew / 2.0 + ex), (int)ey - 10,
                                                    PAIRED_KILL_POPUP_SCORE, 0);
                                                ADD_PLAYER_SCORE(g_save.players[g_mapObjs[i].player].score,
                                                    g_mapObjs[i].player, PAIRED_KILL_SCORE);

                                                SpawnExplosion(g_enemies[p][j].x, g_enemies[p][j].y, 32, 32,
                                                          g_enemies[p][j].deathExplosionLife, 3,
                                                              g_enemies[p][j].deathExplosionGfx,
                                                          80, 255, 80, 80, 255, 80);
                                                AddParticle(g_gfxTable[g_enemies[p][j].deathExplosionGfx],
                                                          (int)g_enemies[p][j].x + 16,
                                                              (int)g_enemies[p][j].y + 16, 16.0f,
                                                          RandFloat(0.0f, 5.0f) + 20.0,
                                                              RandFloat(0.0f, 359.0f), 0.0f, 0,
                                                          80, 255, 80, 600, 15.0f, 0.0f, -1, 0.0f, 0, 0, 0,
                                                              1);
                                                SoundQueueAdd(g_sfxBonus, 50, 0);

                                            } else {
                                                SpawnExplosion(g_enemies[p][j].x, g_enemies[p][j].y, 32, 32,
                                                          g_enemies[p][j].deathExplosionLife, 3,
                                                              g_enemies[p][j].deathExplosionGfx, 255, 255,
                                                              255,
                                                          g_enemies[p][j].deathExplosionR,
                                                              g_enemies[p][j].deathExplosionG,
                                                              g_enemies[p][j].deathExplosionB);
                                                AddParticle(g_gfxTable[g_enemies[p][j].deathExplosionGfx],
                                                          (int)g_enemies[p][j].x + 16,
                                                              (int)g_enemies[p][j].y + 16, 16.0f,
                                                          RandFloat(0.0f, 5.0f) + 20.0,
                                                              RandFloat(0.0f, 359.0f), 0.0f, 0,
                                                          g_enemies[p][j].deathExplosionR,
                                                              g_enemies[p][j].deathExplosionG,
                                                              g_enemies[p][j].deathExplosionB,
                                                          600, 15.0f, 0.0f, -1, 0.0f, 0, 0, 0, 1);
                                            }

                                            if (g_sndTExplo1 < g_time) {
                                                g_sndTExplo1 = RandRange(40, 200) + g_time;
                                                SoundPlay(g_sfxExplo1, RandRange(35000, 44100), 200,
                                                                  g_panTable[ClampX((int)fxKill)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }

                                            kills++;
                                            CreditKill(g_mapObjs[i].player);
                                            if (RandRange(1, (int)g_diffBonusDropRoll) < 4)
                                                SpawnBonus(ex, ey);
                                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                                g_save.players[g_mapObjs[i].player].bonusKilled++;
                                            break;

                                        case ENEMY_WRAPPER:
                                            if (g_sndTExplo2 < g_time) {
                                                g_sndTExplo2 = RandRange(40, 300) + g_time;
                                                SoundPlay(g_sfxExplo2, RandRange(35000, 44100), 200,
                                                                  g_panTable[ClampX((int)fxKill)],
                                                                      RandRange(216, 190), g_sndFlags);
                                                SoundPlay(g_sfxExplo2, RandRange(30000, 40100), 200,
                                                                  g_panTable[ClampX((int)fxKill)],
                                                                      RandRange(216, 190), g_sndFlags);
                                                SoundPlay(g_sfxExplo2, RandRange(25000, 30100), 200,
                                                                  g_panTable[ClampX((int)fxKill)],
                                                                      RandRange(216, 190), g_sndFlags);
                                                SoundPlay(g_sfxExplo2, RandRange(25000, 30100), 200,
                                                                  g_panTable[ClampX((int)fxKill)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }

                                            SpawnBigExplosion(ex, ey, 64, 64, 1,
                                                g_enemies[p][j].deathExplosionGfx, 255, 255, 255);
                                            AddParticle(g_gfxTable[g_enemies[p][j].deathExplosionGfx],
                                                (int)ex + 32, (int)ey + 32, 16.0f,
                                                      RandFloat(0.0f, 5.0f) + 30.0, RandFloat(0.0f, 359.0f),
                                                          0.0f, 0,
                                                      g_enemies[p][j].deathExplosionR,
                                                          g_enemies[p][j].deathExplosionG,
                                                          g_enemies[p][j].deathExplosionB,
                                                      600, 18.0f, 0.0f, 50, 0.0f, 0, 0, 0, 1);
                                            SpawnSlots(ex + 32.0, ey + 30.0, 100, 255, 255, 0);
                                            SpawnItem(ex, ey, g_save.players[g_mapObjs[i].player].color);
                                            CreditKill(g_mapObjs[i].player);
                                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                                g_save.players[g_mapObjs[i].player].bonusKilled++;
                                            break;

                                        case ENEMY_MONEY_SHIP:
                                            if (g_sndTExplo4 < g_time) {
                                                g_sndTExplo4 = RandRange(40, 200) + g_time;
                                                SoundPlay(g_sfxExplo4, RandRange(35000, 44100), 230,
                                                                  g_panTable[ClampX((int)ex)],
                                                                      RandRange(216, 190), g_sndFlags);
                                                SoundPlay(g_sfxExplo4, RandRange(30000, 40100), 200,
                                                                  g_panTable[ClampX((int)ex)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }

                                            SpawnHugeExplosion(ex, ey, 128, 128, 3, 0, 255, 200, 255, 0);
                                            AddParticle(g_gfxTable[0], (int)ex + 64, (int)ey + 64, 16.0f,
                                                      RandFloat(0.0f, 5.0f) + 35.0, RandFloat(0.0f, 359.0f),
                                                          0.0f, 0,
                                                      255, 200, 255, 600, 29.0f, 0.0f, 30, 0.0f, 0, 0, 0, 1);
                                            SpawnSlots(ex + 64.0, ey + 64.0, 100, 255, 0, 100);
                                            CreditKill(g_mapObjs[i].player);
                                            SpawnPowerupBurst((int)(ex + 64.0), (int)(ey + 64.0), 0, 0);

                                            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn !=
                                                StateDemo) {
                                                g_save.players[g_curPlayer].secretFound06 = 1;
                                                MarkSecretFound(g_profileIndex, 6);
                                            }
                                            g_samples[p][j] = SoundStop(g_samples[p][j]);
                                            if (!IsHurryUpEnemyActive())
                                                break;

                                        case ENEMY_MOTHERSHIP:
                                            if (g_sndTExplo4 < g_time) {
                                                g_sndTExplo4 = RandRange(40, 200) + g_time;
                                                SoundPlay(g_sfxExplo4, RandRange(35000, 44100), 230,
                                                                  g_panTable[ClampX((int)ex)],
                                                                      RandRange(216, 190), g_sndFlags);
                                                SoundPlay(g_sfxExplo4, RandRange(30000, 40100), 200,
                                                                  g_panTable[ClampX((int)ex)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }
                                            SpawnHugeExplosion(ex, ey, 96, 57, 3, 0, 255, 200, 200, 0);
                                            AddParticle(g_gfxTable[0], (int)ex + 48, (int)ey + 27, 16.0f,
                                                      RandFloat(0.0f, 5.0f) + 35.0, RandFloat(0.0f, 359.0f),
                                                          0.0f, 0,
                                                      255, 200, 200, 600, 29.0f, 0.0f, 30, 0.0f, 0, 0, 0, 1);
                                            SpawnSlots(ex + 48.0, ey + 28.5, 100, 100, 255, 100);
                                            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn !=
                                                StateDemo) {
                                                g_save.players[g_curPlayer].secretFound03 = 1;
                                                MarkSecretFound(g_profileIndex, 3);
                                            }
                                            SpawnWeaponItem((int)ex, (int)ey);
                                            CreditKill(g_mapObjs[i].player);
                                            g_samples[p][j] = SoundStop(g_samples[p][j]);
                                            switch (g_gameMode) {

                                            case MODE_SINGLE:
                                            case MODE_TWO_PLAYER:
                                                g_save.players[g_mapObjs[i].player].time =
                                                    g_time +
                                                        g_save.players[g_mapObjs[i].player].effectDuration;
                                                break;

                                            case MODE_DUAL:
                                                g_save.players[0].time = g_time +
                                                    g_save.players[0].effectDuration;
                                                break;

                                            case MODE_TEAM:
                                                g_lastEventTime = g_time + g_bonusDuration;
                                                break;
                                                break;
                                                break;

                                            case MODE_TIME_TRIAL:
                                                g_save.players[g_mapObjs[i].player].time =
                                                    g_time +
                                                        g_save.players[g_mapObjs[i].player].effectDuration;
                                            }
                                            break;

                                        case ENEMY_MONEY_SUCKER:
                                            if (g_sndTExplo5 < g_time) {
                                                g_sndTExplo5 = RandRange(40, 200) + g_time;
                                                SoundPlay(g_sfxExplo5, RandRange(35000, 44100), 200,
                                                                  g_panTable[ClampX((int)ex)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }
                                            if (g_sndTExplo2 < g_time) {
                                                g_sndTExplo2 = RandRange(40, 300) + g_time;
                                                SoundPlay(g_sfxExplo2, RandRange(30000, 40100), 200,
                                                                  g_panTable[ClampX((int)ex)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }
                                            SpawnHugeExplosion(ex, ey, 128, 51, 3, 0, 255, 200, 255, 0);
                                            AddParticle(g_gfxTable[0], (int)ex + 64, (int)ey + 25, 16.0f,
                                                      RandFloat(0.0f, 5.0f) + 35.0, RandFloat(0.0f, 359.0f),
                                                          0.0f, 0,
                                                      255, 200, 255, 600, 29.0f, 0.0f, 30, 0.0f, 0, 0, 0, 1);
                                            SpawnSlots(ex + 64.0, ey + 28.5, 300, 100, 255, 100);
                                            CreditKill(g_mapObjs[i].player);
                                            SpawnPowerupBurst((int)(ex + 64.0), (int)(ey + 28.0), 1, 0);
                                            SpawnItems();
                                            g_moneySuckerBaseHp += 20;
                                            break;

                                        case ENEMY_GUARD:
                                            if (g_sndTExplo4 < g_time) {
                                                g_sndTExplo4 = RandRange(40, 200) + g_time;
                                                SoundPlay(g_sfxExplo4, RandRange(35000, 44100), 230,
                                                                  g_panTable[ClampX((int)ex)], 223,
                                                                      g_sndFlags);
                                                SoundPlay(g_sfxExplo4, RandRange(30000, 40100), 200,
                                                                  g_panTable[ClampX((int)ex)],
                                                                      RandRange(216, 190), g_sndFlags);
                                            }
                                            AddScorePopup(g_screenW >> 1, g_screenH >> 1,
                                                g_enemies[p][j].score, 1);
                                            SpawnHugeExplosion(ex, ey, 128, 32, 3, 0, 255, 200, 255, 1);
                                            AddParticle(g_gfxTable[0], (int)ex + 64, (int)ey + 19, 16.0f,
                                                      RandFloat(0.0f, 5.0f) + 35.0, RandFloat(0.0f, 359.0f),
                                                          0.0f, 0,
                                                      255, 200, 255, 600, 29.0f, 0.0f, 30, 0.0f, 0, 0, 0, 1);
                                            SpawnSlots(ex + 48.0, ey + 19.5, 100, 255, 0, 100);
                                            CreditKill(g_mapObjs[i].player);
                                            g_eliteHpBonus += 250;
                                            g_samples[p][j] = SoundStop(g_samples[p][j]);
                                        }
                                    }

                                    // ---- flat kill score (enemy's own g_enemies[].score, all types) ----
                                    killScore = g_enemies[p][j].score * g_scoreMul[g_mapObjs[i].player];
                                    if (killScore > 0 && killScore < 100000000) {
                                        g_save.players[g_mapObjs[i].player].score =
                                            g_save.players[g_mapObjs[i].player].score + killScore;
                                    }
                                } else {
                                    // ---- non-kill hit: hit sound/particle, laser decay or shot consumption ----
                                    switch (g_enemies[p][j].type) {

                                    case ENEMY_WRAPPER:
                                    case ENEMY_MOTHERSHIP:
                                    case ENEMY_MONEY_SUCKER:
                                    case ENEMY_GUARD:
                                        kills++;
                                        AddParticle(g_gfxFlareBomb2, (int)(ew / 2.0 + ex), (int)(eh / 2.0 +
                                            ey), RandFloat(5.0f, 10.0f),
                                                  RandFloat(10.8f, 20.6f), RandFloat(0.0f, 359.0f), 0.0f, 0,
                                                  RandRange(150, 255), RandRange(0, 150), 0, 600,
                                                  RandFloat(5.0f, 10.0f), 0.0f, 30, 0.0f, 0, 0, 0, 0);

                                        if (RandRange(0, 100) < 50) {
                                            if (g_time > g_sndTHit3) {
                                                SoundPlay(g_sfxHit3, RandRange(20000, 50000),
                                                    g_rampB[ClampY((int)ey)],
                                                                  g_panTable[ClampX((int)ex)], 127,
                                                                      g_sndFlags);
                                                g_sndTHit3 = RandRange(100, 300) + g_time;
                                            }
                                        } else {
                                            if (g_time > g_sndTHit4) {
                                                SoundPlay(g_sfxHit4, RandRange(15000, 40000),
                                                    g_rampB[ClampY((int)ey)],
                                                                  g_panTable[ClampX((int)ex)], 127,
                                                                      g_sndFlags);
                                                g_sndTHit4 = RandRange(100, 300) + g_time;
                                            }
                                        }
                                    }

                                    g_enemies[p][j].altFrameCounter = 5;

                                    LASER_DECAY_OR_UNLOCK(RandFloat(-1.0f, 1.0f))

                                    g_save.players[g_mapObjs[i].player].hits++;
                                    g_hits++;
                                }
                            } else {
                                if (g_mapObjs[i].laser != 0)
                                    g_cfg.collisionDetail = savedFlag;
                            }
                        }
                    }
                }
            }
        }
    }

    // Killing enemies nudges the survivors' fire rate up slightly.
    if (kills != 0) {
        for (j = 0; j < MAX_ENEMIES; j++) {
            if (g_enemies[p][j].active == 1 && g_enemies[p][j].type != ENEMY_CAPTURED && g_enemies[p][j].attackDelay !=
                0) {
                g_enemies[p][j].attackDelay -= g_enemies[p][j].attackDelayStep * kills;
                if (g_enemies[p][j].attackDelay < g_fireDelayMin)
                    g_enemies[p][j].attackDelay = g_fireDelayMin;
                g_enemies[p][j].fireDelay -= g_enemies[p][j].fireDelayStep * kills;
                if (g_enemies[p][j].fireDelay < g_enemyFireRateMin)
                    g_enemies[p][j].fireDelay = g_enemyFireRateMin;
            }
        }
    }
}

#undef LASER_DECAY_OR_UNLOCK

// Tests every active, non-captured/non-debris/non-hurry-up enemy for the current player
// against the player's tractor-beam grab zone (active while scoopTimer is running). An enemy
// inside the vertical/horizontal grab band is captured onto the left shield slot if free,
// else the right, else (both slots full) it is destroyed for a score bonus instead.
void ShieldGrabEnemies()
{
    int i;
    int clamp;
    int set;
    int ex;
    int ey;
    int ex2;
    int ey2;
    int px;
    int top;
    int py;
    int left;
    int right;

    g_grabZoneHit = 0;
    if (g_save.players[g_curPlayer].dead == 0
        && g_save.players[g_curPlayer].lives > g_shipDefs[g_save.players[g_curPlayer].ship]->minEnergy
        && g_save.players[g_curPlayer].scoopTimer != 0) {
        if (g_gameMode == MODE_DUAL)
            set = 0;
        else
            set = g_curPlayer;
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (g_enemies[set][i].active == 1 && g_enemies[set][i].type != ENEMY_CAPTURED
                && g_enemies[set][i].type != ENEMY_DEBRIS && g_enemies[set][i].type != ENEMY_WRAPPER) {
                ex = (int)g_enemies[set][i].x;
                ey = (int)g_enemies[set][i].y;
                ex2 = (int)(g_enemies[set][i].x + 32.0);
                ey2 = (int)(g_enemies[set][i].y + 32.0);
                py = (int)g_save.players[g_curPlayer].y;
                top = py - (int)(g_scoopYScale * 45.0);

                if ((ey > top && ey < py) || (ey2 > top && ey2 < py)) {
                    if (g_save.players[g_curPlayer].mirrorTime == 0)
                        px = (int)(g_save.players[g_curPlayer].x + 20.0);
                    else if (Rand7f() < 64)
                        px = (int)(g_save.players[g_curPlayer].x + 20.0);
                    else
                        px = (int)((g_screenW - 40) - g_save.players[g_curPlayer].x + 20.0);
                    if ((clamp = py - ey) < 0)
                        clamp = 0;
                    if (clamp > 90)
                        clamp = 90;
                    left = px - (int)g_grabZoneWidthTable[clamp];
                    right = (int)g_grabZoneWidthTable[clamp] + px;
                    g_grabZoneHit = 1;

                    if ((ex > left && ex < right) || (ex2 > left && ex2 < right)) {
                        if (g_save.players[g_curPlayer].shieldL == 0) {
                            g_save.players[g_curPlayer].shieldL = 1;
                            g_save.players[g_curPlayer].shieldLIdx = i;
                            g_enemies[set][i].ownerPlayer = g_curPlayer;
                            g_enemies[set][i].animDelay = 3.0f;
                            g_enemies[set][i].beamSide = 0;
                            g_enemies[set][i].type = ENEMY_CAPTURED;
                            g_enemies[set][i].settled = 0;
                            g_enemies[set][i].beamOffsetX = (int)(ex - g_save.players[g_curPlayer].x);
                            g_enemies[set][i].y = (float)ey;

                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                CreditKill(g_curPlayer);
                            else
                                CreditEscape(g_curPlayer);
                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                g_save.players[g_curPlayer].bonusKilled++;
                            g_warpLevelL = g_loadedLevel;
                            BumpAlienGfxAge(g_enemies[set][i].gfxA);
                            SoundPlay(g_sfxCapture, 18000, 130, g_panTable[ClampX(ex)], 127, g_sndFlags);
                            if (g_save.players[g_curPlayer].scoopTimer - g_time < 5000)
                                SoundQueueAdd(g_sfxGotcha, 20, 1);
                        } else if (g_save.players[g_curPlayer].shieldR == 0) {
                            g_save.players[g_curPlayer].shieldR = 1;
                            g_save.players[g_curPlayer].shieldRIdx = i;
                            g_enemies[set][i].ownerPlayer = g_curPlayer;
                            g_enemies[set][i].animDelay = 3.0f;
                            g_enemies[set][i].beamSide = 1;
                            g_enemies[set][i].type = ENEMY_CAPTURED;
                            g_enemies[set][i].settled = 0;
                            g_enemies[set][i].beamOffsetX = (int)(ex - g_save.players[g_curPlayer].x);
                            g_enemies[set][i].y = (float)ey;

                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                CreditKill(g_curPlayer);
                            else
                                CreditEscape(g_curPlayer);
                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                g_save.players[g_curPlayer].bonusKilled++;
                            g_warpLevelR = g_loadedLevel;
                            BumpAlienGfxAge(g_enemies[set][i].gfxA);
                            SoundPlay(g_sfxCapture, 15000, 150, g_panTable[ClampX(ex)], 127, g_sndFlags);
                            if (g_save.players[g_curPlayer].scoopTimer - g_time < 5000)
                                SoundQueueAdd(g_sfxGotcha, 20, 1);
                        } else {
                            g_enemies[set][i].animDelay = 2.0f;
                            AddScorePopup(ex + RandRange(0, 15) - 20, ey, SHIELD_FULL_DEBRIS_SCORE,
                                0);
                            g_enemies[set][i].type = ENEMY_DEBRIS;
                            g_enemies[set][i].locked = 0;
                            g_enemies[set][i].debrisVelX = RandFloat(-4.0f, 4.0f);
                            g_enemies[set][i].debrisVelY = RandFloat(-10.0f, -6.0f);

                            ADD_PLAYER_SCORE(g_save.players[g_curPlayer].score, g_curPlayer,
                                SHIELD_FULL_DEBRIS_SCORE);

                            if (g_save.players[g_curPlayer].trackKillsFlag != 0)
                                g_save.players[g_curPlayer].bonusKilled++;
                            CreditKill(g_curPlayer);
                            if (g_time > g_sndTMouww) {
                                SoundPlay(g_sfxMouww, RandRange(15000, 22000), 130, g_panTable[ClampX(ex)],
                                    127, g_sndFlags);
                                g_sndTMouww = RandRange(30, 300) + g_time;
                            }
                            SoundQueueAdd(g_sfxBonus, 50, 0);
                            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                                g_save.players[g_curPlayer].secretFound08 = 1;
                                MarkSecretFound(g_profileIndex, 8);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Runs ShieldGrabEnemies for both players in a randomized order, then leaves g_curPlayer at 0.
void ShieldGrabEnemiesBothPlayers()
{
    g_curPlayer = RandRange(0, 2);
    ShieldGrabEnemies();
    g_curPlayer = 1 - g_curPlayer;
    ShieldGrabEnemies();
    g_curPlayer = 0;
}

// Pixel-mask collision test between two sprites' overlapping bounding boxes ((x1,y1)-(x2,y2)
// and (x3,y3)-(x4,y4)), sampling maskA/maskB (pitchA/pitchB wide, hA/hB tall) inside the
// overlap. Returns 1 on a hit or when enable is 0 or a mask pointer is null, 0 otherwise.
int MaskCollide(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
                       unsigned char *maskA, unsigned char *maskB,
                       Rect16 boxA, Rect16 boxB,
                       int pitchA, int pitchB, int hA, int hB, int enable)
{
    int i;
    int j;

    if (!enable || !maskA || !maskB)
        return 1;

    g_ovX1 = x1 > x3 ? x1 : x3;
    g_ovX2 = x2 < x4 ? x2 : x4;
    g_ovY1 = y1 > y3 ? y1 : y3;
    g_ovY2 = y2 < y4 ? y2 : y4;
    g_ovW = g_ovX2 - g_ovX1;
    g_ovH = g_ovY2 - g_ovY1;

    if (x1 > x3)
        g_offXA = 0;
    else
        g_offXA = x3 - x1;
    if (y1 > y3)
        g_offYA = 0;
    else
        g_offYA = y3 - y1;
    if (g_offXA == 0)
        g_offXB = x1 - x3;
    else
        g_offXB = 0;
    if (g_offYA == 0)
        g_offYB = y1 - y3;
    else
        g_offYB = 0;

    if (g_offYA + g_ovH > boxA.y2 - boxA.y1)
        return 0;
    if (g_offYB + g_ovH > boxB.y2 - boxB.y1)
        return 0;

    g_idxA = (boxA.y1 + g_offYA) * pitchA + boxA.x1 + g_offXA;
    g_idxB = (boxB.y1 + g_offYB) * pitchB + boxB.x1 + g_offXB;
    if ((g_ovH - 1) * pitchA + g_idxA + g_ovW - 1 > (hA - 1) * pitchA + g_idxA + pitchA - 1)
        return 0;
    if ((g_ovH - 1) * pitchB + g_idxB + g_ovW - 1 > (hB - 1) * pitchB + g_idxB + pitchB - 1)
        return 0;

    for (i = 0; i < g_ovH - 1; i++) {
        for (j = 0; j < g_ovW - 1; j++) {
            if (maskA[g_idxA + j] && maskB[g_idxB + j])
                return 1;
        }
        g_idxA += pitchA;
        g_idxB += pitchB;
    }
    return 0;
}

// Pixel-mask collision test like MaskCollide, but with each mask's own box passed
// separately (ax1..ay2 for maskA, bx1..by2 for maskB) rather than assuming both masks
// start at their sprite's origin. Returns 1 on a hit or when enable is 0 or a mask
// pointer is null, 0 otherwise.
int MaskCollide2(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
                        unsigned char *maskA, unsigned char *maskB,
                        int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2,
                        int pitchA, int pitchB, int hA, int hB, int enable)
{
    int xa;
    int w;
    int xb;
    int ya;
    int h;
    int yb;
    int idxA;
    int idxB;
    int i;
    int j;

    if (!enable)
        return 1;
    if (!maskA || !maskB)
        return 1;

    if (x3 >= x1) {
        xa = x3 - x1;
        w = x2 - x3;
        if (w > x2 - x1)
            w = x2 - x1;
        if (w > x4 - x3)
            w = x4 - x3;
        xb = 0;
    } else {
        xa = 0;
        w = x4 - x1;
        if (w > x2 - x1)
            w = x2 - x1;
        if (w > x4 - x3)
            w = x4 - x3;
        xb = x1 - x3;
    }
    if (y3 >= y1) {
        ya = y3 - y1;
        h = y2 - y3;
        if (h > y2 - y1)
            h = y2 - y1;
        if (h > y4 - y3)
            h = y4 - y3;
        yb = 0;
    } else {
        ya = 0;
        h = y4 - y1;
        if (h > y2 - y1)
            h = y2 - y1;
        if (h > y4 - y3)
            h = y4 - y3;
        yb = y1 - y3;
    }

    idxA = (ya + ay1) * pitchA + xa + ax1;
    idxB = (yb + by1) * pitchB + xb + bx1;
    if ((h - 1) * pitchA + idxA + w - 1 > pitchA * hA)
        return 0;
    if ((h - 1) * pitchB + idxB + w - 1 > pitchB * hB)
        return 0;
    if (idxB < 0)
        return 0;

    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            if (maskA[idxA + j] && maskB[idxB + j])
                return 1;
        }
        idxA += pitchA;
        idxB += pitchB;
    }
    return 0;
}
