// gemdrop.c: The Gem Drop bonus stage (falling gems).
#include <stdio.h>
#include "globals.h"
#include "game.h"

// Gem pickup score tiers (by type), lower during a normal drop and higher once a
// "super" gem drop is in progress.
enum {
    GEM_SCORE_LOW = 50000,
    GEM_SCORE_MID = 100000,
    GEM_SCORE_HIGH = 500000,
    SUPER_GEM_SCORE_LOW = 1000000,
    SUPER_GEM_SCORE_MID = 5000000,
    SUPER_GEM_SCORE_HIGH = 10000000,
};

// Gate for the Gem Drop stage intro: stays active until g_gemDropIntroTimer elapses.
void UpdateGemDropIntroGate()
{
    g_gemDropIntroActive = 0;
    g_introGateScratch = 1;
    if (g_time < g_gemDropIntroTimer) {
        g_gemDropIntroActive = 1;
        g_introGateScratch = 0;
    }
}

// Draws the "GEM DROP" / "SUPER GEM DROP" intro banner and its background effect
// (scanlines when particles are off, laser-flare sparks when particles are on) while a
// Gem Drop intro is active; `g_levelDist` sign selects the plain vs. super variant.
void DrawGemDropBanner()
{
    int n;
    int cy;
    int i;
    int j;

    n = 20;
    cy = (int)g_screenH / 2;
    if (g_gemDropIntroActive != 0 && g_levelDist > 0.0) {
        if (g_cfg.particlesOn == 0) {
            for (i = 0; i < n; i++) {
                DrawLine(100.0f, (float)(cy - n * 3 / 2 + i * 3),
                                    g_screenW - 100.0, (float)(cy - n * 3 / 2 + i * 3),
                                    (i * 3 + 50) / 255.0, 0, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 255, 40,
                               150.0f, 0, -1, RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 255, 40,
                               150.0f, 0, -1, 0 - RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
        }
        if (g_superGemDrop)
            DrawMenuText("S U P E R   G E M   D R O P", POS_CENTERED, cy - 22, 0);
        else
            DrawMenuText("G E M   D R O P", POS_CENTERED, cy - 22, 0);
        DrawMenuText("G E T   R E A D Y", POS_CENTERED, cy + 10, 0);
    }

    // ---- super gem-drop variant ----
    if (g_gemDropIntroActive != 0 && g_levelDist < 0.0) {
        if (g_cfg.particlesOn == 0) {
            n = 40;
            for (j = 0; j < n; j++) {
                DrawLine(100.0f, (float)(cy - n * 3 / 2 + j * 3),
                                    g_screenW - 100.0, (float)(cy - n * 3 / 2 + j * 3),
                                    (j * 3 + 50) / 255.0, 0, 0.5f, 1.0f);
            }
        } else if (g_time > g_nextSpark) {
            g_nextSpark = g_time + 8;
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 255, 40,
                               150.0f, 0, -1, RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 100, 3.0f, 0, 90.0f, 0, 0, 255, 0, 255, 40,
                               150.0f, 0, -1, 0 - RandFloat(0.02f, 0.03f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) - 101, 2.0f, 0, 90.0f, 0, 0, 255, 255, 0, 40,
                               80.0f, 0, -1, 0 - RandFloat(0.01f, 0.02f), 1, 0, &g_introGateScratch, 0);
            AddParticle(g_gfxFlareLaser, 0, (g_screenH >> 1) + 101, 2.0f, 0, 90.0f, 0, 0, 255, 255, 0, 40,
                               80.0f, 0, -1, RandFloat(0.01f, 0.02f), 1, 0, &g_introGateScratch, 0);
        }
        DrawMenuText("SUPER GEM DROP BONUS", POS_CENTERED, cy - 34, 0);
        DrawMenuText("1.000.000 POINTS", POS_CENTERED, cy - 6, 3);
        DrawMenuText(g_logBuf, POS_CENTERED, cy + 22, 1);
    }
}

// Per-frame render for the Gem Drop stage: background, sparks, the falling gems, ship HUD,
// score popups, the Gem Drop banner, particles, flash, warp ring, borders and HUD.
void RenderGemDropFrame()
{
    g_stateFn();
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawFallingGems();
    g_shipHudFn();
    DrawScorePopups();
    DrawGemDropBanner();
    DrawParticles();
    DrawFlash();
    if (g_state == STATE_PAUSED)
        DrawWarpRing();
    g_drawBordersFn();
    g_drawHudFn();
}

// Resets state and spawns the initial pool of falling gems (with random type/frame/anim timing)
// for the Gem Drop bonus level.
void InitGemDropLevel()
{
    int i;
    ResetObjectsKeep();
    g_save.players[0].energy = 0;
    g_save.players[1].energy = 0;
    g_save.players[2].energy = 0;
    g_save.players[3].energy = 0;
    g_levelDist = 2680.0f;
    g_maxFallingGems = 1.0f;
    g_pickupCount = 0;
    for (i = 0; i < MAX_FALLING_GEMS; i++) {
        g_fallingGems[i].active = 0;
        g_fallingGems[i].h = 0x33;
        g_fallingGems[i].w = 0x50;
        g_fallingGems[i].sy = 0;
        g_fallingGems[i].type = RandRange(0, 3) * 0x50;
        g_fallingGems[i].frame = RandRange(0, 11);
        g_fallingGems[i].animDelay = (float)RandRange(3, 6);
        g_fallingGems[i].animTimer = g_fallingGems[i].animDelay;
    }
}

// Bonus-level-end callback for the Gem Drop bonus level; no cleanup needed.
void OnGemDropLevelEnd()
{
}

// Draws the 10 falling meteor/gem pickups, clipped to the screen rect, and (via the
// r1..r4 / r34 union) records each one's current source rect in `type`,`sy`,`w`,`h` as
// its hit-test rectangle for GemDropCollide().
void DrawFallingGems()
{
    Rect16 src;
    int i;
    int offY;
    int offX;

    for (i = 0; i < MAX_FALLING_GEMS; i++) {
        if (g_fallingGems[i].active != 0) {
            while (1) {
                offY = 0;
                offX = 0;
                // r1..r4 alias the Rect16 r34 used by GemDropCollide() as the hitbox.
                g_fallingGems[i].r1 = g_fallingGems[i].type;
                g_fallingGems[i].r2 = g_fallingGems[i].sy;
                g_fallingGems[i].r3 = g_fallingGems[i].r1 + g_fallingGems[i].w;
                g_fallingGems[i].r4 = g_fallingGems[i].r2 + g_fallingGems[i].h;
                g_fallingGems[i].sy = g_fallingGems[i].frame * 0x33; // 0x33 = 51 px per row

                if (g_fallingGems[i].y > g_clipTop)
                    src.y1 = g_fallingGems[i].sy;
                else if (g_clipTop - (int)g_fallingGems[i].y >= g_fallingGems[i].h)
                    break;
                else {
                    offY = g_clipTop - (int)g_fallingGems[i].y;
                    src.y1 = g_fallingGems[i].sy + offY;
                }
                if (g_fallingGems[i].x > g_clipLeft)
                    src.x1 = g_fallingGems[i].type;
                else if (g_clipLeft - (int)g_fallingGems[i].x >= g_fallingGems[i].w)
                    break;
                else {
                    offX = g_clipLeft - (int)g_fallingGems[i].x;
                    src.x1 = g_fallingGems[i].type + offX;
                }
                src.x2 = g_fallingGems[i].w - offX + src.x1;
                if ((int)g_fallingGems[i].x + g_fallingGems[i].w - offX > g_clipRight) {
                    if (g_fallingGems[i].x > g_clipRight)
                        break;
                    else
                        src.x2 = src.x2 - ((int)g_fallingGems[i].x + g_fallingGems[i].w - offX - g_clipRight);
                }

                src.y2 = g_fallingGems[i].h - offY + src.y1;
                if ((int)g_fallingGems[i].y + (g_fallingGems[i].h - offY) > g_clipBottom) {
                    if (g_fallingGems[i].y > g_clipBottom)
                        break;
                    else
                        src.y2 = src.y2 -
                                 ((int)g_fallingGems[i].y + (g_fallingGems[i].h - offY) - g_clipBottom);
                }
                QueueBlit(g_fallingGems[i].x + offX, g_fallingGems[i].y + offY, g_gfxDiamondBig, &src);
                break;
            }
        }
    }
}

// Awards score for a gem pickup and shows the floating score popup.
#define AWARD(val)                                                \
    ADD_PLAYER_SCORE(g_save.players[p].score, p, val); \
    AddScorePopup(bx + 20, by + 20, val, 0);

// Checks player `p` against all active falling gems for a pixel-mask collision;
// on a hit, awards score based on the pickup's `type` and whether a super-gem drop is in
// progress (which swaps in the lower-tier score table). Accounts for the level's mirror
// effect by testing the player's mirrored x position when `mirrorTime` rolls it.
void GemDropCollide(int p)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int bx;
    int by;
    int bx2;
    int by2;
    int i;

    if (g_save.players[p].mirrorTime == 0)
        x1 = (int)g_save.players[p].x;
    else if (Rand7f() < 64)
        x1 = (int)g_save.players[p].x;
    else
        x1 = (int)((g_screenW - 40) - g_save.players[p].x);
    y1 = (int)g_save.players[p].y;
    x2 = x1 + 40;
    y2 = y1 + 27;

    for (i = 0; i < MAX_FALLING_GEMS; i++) {
        if (g_fallingGems[i].active != 0) {
            bx = (int)g_fallingGems[i].x;
            bx2 = bx + g_fallingGems[i].w;
            by = (int)g_fallingGems[i].y;
            by2 = by + g_fallingGems[i].h;
            if (bx < x2 && bx2 > x1 && by < y2 && by2 > y1) {
                if (MaskCollide(bx, by, bx2, by2, x1, y1, x2, y2,
                                   (unsigned char *)g_fallingGems[i].hma,
                                   (unsigned char *)g_save.players[p].hitMask,
                                   g_fallingGems[i].hitRect, g_save.players[p].box,
                                   g_fallingGems[i].hmaW, g_save.players[p].hitMaskParamA,
                                   g_fallingGems[i].hmaH, g_save.players[p].hitMaskParamB,
                                   g_cfg.collisionDetail)) {
                    g_pickupCount--;
                    g_fallingGems[i].active = 0;
                    SoundPlay(g_sfxJingles, RandRange(30000, 45000), 255,
                                      g_panTable[ClampX((g_fallingGems[i].w >> 1) + bx)],
                                      127, g_sndFlags);
                    SoundQueueAdd(g_sfxBonus, 50, 0);

                    if (!g_superGemDrop) {
                        if (g_fallingGems[i].type == 0) {
                            AWARD(GEM_SCORE_LOW)
                        }
                        if (g_fallingGems[i].type == 80) {
                            AWARD(GEM_SCORE_MID)
                        }
                        if (g_fallingGems[i].type == 160) {
                            AWARD(GEM_SCORE_HIGH)
                        }

                    } else {
                        if (g_fallingGems[i].type == 0) {
                            AWARD(SUPER_GEM_SCORE_LOW)
                        }
                        if (g_fallingGems[i].type == 80) {
                            AWARD(SUPER_GEM_SCORE_MID)
                        }
                        if (g_fallingGems[i].type == 160) {
                            AWARD(SUPER_GEM_SCORE_HIGH)
                        }
                    }
                }
            }
        }
    }
}

#undef AWARD

// Per-frame update for the Gem Drop stage: spawns new gems up to the current cap
// (g_maxFallingGems, which ramps up over time) with a type picked by random tier, animates and moves them, respawning replacements as they fall
// off-screen, and ends the level once g_levelDist counts down to zero.
void GemDropUpdate()
{
    int i;
    int r;

    g_maxFallingGems = g_maxFallingGems + g_maxFallingGemsInc;

    // Spawn a new gem into a free slot if we're below the current cap.
    if (g_pickupCount < (int)g_maxFallingGems && g_levelDist > 0.0) {
        for (i = 0; i < MAX_FALLING_GEMS; i++) {
            if (g_fallingGems[i].active == 0) {
                g_fallingGems[i].active = 1;
                r = RandRange(0, 100);
                if (r <= 50)
                    g_fallingGems[i].type = 0;
                if (r > 50 && r < 85)
                    g_fallingGems[i].type = 0x50;
                if (r >= 85)
                    g_fallingGems[i].type = 0xa0;

                g_fallingGems[i].frame = RandRange(0, 11);
                g_fallingGems[i].animDelay = (float)RandRange(1, 4);
                g_fallingGems[i].animTimer = g_fallingGems[i].animDelay;
                g_fallingGems[i].y = -60.0f;
                g_fallingGems[i].vy = RandFloat(7.0f, 14.0f);
                g_fallingGems[i].x = RandFloat(70.0f, (float)(g_screenW - 150));
                g_fallingGems[i].hma = g_hmaDiamondBig;
                g_fallingGems[i].hmaW = g_diamondBigGfxW;
                g_fallingGems[i].hmaH = g_diamondBigGfxH;
                g_pickupCount++;
                break;
            }
        }
    }

    // Animate, move, and respawn each active gem that fell off the bottom.
    for (i = 0; i < MAX_FALLING_GEMS; i++) {
        if (g_fallingGems[i].active != 0) {
            g_fallingGems[i].animTimer -= 1.0;
            if (g_fallingGems[i].animTimer < 0.0) {
                g_fallingGems[i].animTimer = g_fallingGems[i].animDelay;
                g_fallingGems[i].frame++;
                if (g_fallingGems[i].frame > 10)
                    g_fallingGems[i].frame = 0;
            }
            g_fallingGems[i].y += g_fallingGems[i].vy * g_frameDt;

            if (g_fallingGems[i].y > g_screenH + g_fallingGems[i].h + 5) {
                g_fallingGems[i].active = 0;
                g_pickupCount--;

                if (g_pickupCount < (int)g_maxFallingGems) {
                    for (int j = 0; j < MAX_FALLING_GEMS; j++) {
                        if (g_fallingGems[j].active == 0) {
                            g_fallingGems[j].active = 1;
                            r = RandRange(0, 100);
                            g_fallingGems[j].type = 0x50;
                            if (r <= 50)
                                g_fallingGems[j].type = 0;
                            if (r > 50 && r < 85)
                                g_fallingGems[j].type = 0x50;
                            if (r >= 85)
                                g_fallingGems[j].type = 0xa0;

                            g_fallingGems[j].frame = RandRange(0, 11);
                            g_fallingGems[j].animDelay = (float)RandRange(1, 4);
                            g_fallingGems[j].animTimer = g_fallingGems[j].animDelay;
                            g_fallingGems[j].y = -60.0f;
                            g_fallingGems[j].vy = RandFloat(6.0f, 10.0f);
                            g_fallingGems[j].x = RandFloat(70.0f, (float)(g_screenW - 150));
                            g_pickupCount++;
                            break;
                        }
                    }
                }
            }
        }
    }
    g_levelDist -= 1.0f * g_frameDt;

    // Level distance ran out: end the Gem Drop level.
    if (g_levelDist < 0.0) {
        g_flashOverlayActive = 1;
        g_fadeStep = 0;
        g_fadeColorSet = 1;
        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
        g_transitionLock = 1;
        OnGemDropLevelEnd();
        if (g_save.players[g_curPlayer].done == 0) {
            g_save.players[g_curPlayer].done = 1;
            if (g_save.players[g_curPlayer].doneTime == 0)
                g_save.players[g_curPlayer].doneTime = g_time + DONE_DELAY_MS;
        }

        g_save.players[g_curPlayer].killed = g_save.players[g_curPlayer].totalEnemies;
        g_save.players[g_curPlayer].escaped = 0;
        g_save.players[0].energy = 0;
        g_save.players[1].energy = 0;
        g_save.players[2].energy = 0;
        g_save.players[3].energy = 0;
        ResetObjectsKeep();
        SetHurryUpTimer();
        g_superGemDrop = 0;
        g_state = STATE_PLAYING;
    }
}
