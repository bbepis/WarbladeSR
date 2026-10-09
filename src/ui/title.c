// title.c: Title and attract: splash screens, the logo intro and its flashes, the menu prompt,
// returning to the title.
#include "globals.h"
#include "game.h"

// ShowLogoSplash/ShowTitleSplash: alpha ramp multiplier, growing (fade in) then shrinking
// (fade out) each frame.
#define SPLASH_FADE_IN_MUL  1.07f
#define SPLASH_FADE_OUT_MUL 0.94f


// Draws the title-screen blinking prompts: the time-trial "unavailable" notice, a restart-required
// warning (flickering between two colours), and "press fire/space to play" (device-dependent wording).
void DrawMenuPrompt()
{
    if (g_time - g_uiBlinkTime > g_blinkRate) {
        g_uiBlinkTime = g_time;
        g_uiBlink = g_uiBlink == 0;
    }
    if (AnyWindowActive())
        return;
    if (g_timeTrialLocked != 0 && g_timeTrialDeadline > g_time)
        DrawTinyText("TIME TRIAL IS UNAVAILABLE, PLEASE REGISTER FOR THE FULL VERSION",
                           POS_CENTERED, g_screenH - 0x34, 1);
    if (g_uiBlink != 0 && (g_restartNeeded ||
                            g_windowRenderer != RendererChoiceOf(g_cfg.renderer))) {
        if (RandRange(0, 100) < 50)
            DrawMenuText("GAME MUST BE RESTARTED FOR CHANGES TO TAKE EFFECT", POS_CENTERED, g_screenH - 0x38, 0);
        else
            DrawMenuText("GAME MUST BE RESTARTED FOR CHANGES TO TAKE EFFECT", POS_CENTERED, g_screenH - 0x38, 5);
    }
    if (g_uiBlink != 0) {
        if (g_cfg.device0 == 1 || g_cfg.device0 == 2)
            DrawMenuText("PRESS FIRE TO PLAY", POS_CENTERED, g_screenH - 0x2a, 5);
        else
            DrawMenuText("PRESS SPACE TO PLAY", POS_CENTERED, g_screenH - 0x2a, 5);
    }
}

// Shows the EMV Software logo splash: fades in, holds, fades out, skippable with space/esc/
// ctrl or a mouse click once the 1s minimum has elapsed.
void ShowLogoSplash()
{
    SoundStopAll();
    g_clipLeft = 0;
    g_clipRight = g_screenW;
    bool done = false;
    g_splash = LoadGraphic("emvsoftware.jpg", true, false);
    if (g_splash == 0) return;
    int unused = 1;
    g_time = SysMillis();
    if (g_time == 0) g_time = SysMillis();
    g_splashEnd = g_time + 5000;    // ms: total display time before fade-out is allowed to finish
    g_splashMinEnd = g_time + 1000; // ms: minimum time before a skip input is honored
    float blend = 0.001f;           // alpha ramp, grows geometrically (fade in), then shrinks (fade out)
    float mul = SPLASH_FADE_IN_MUL;

    do {
        g_time = SysMillis();
        if (g_time == 0) g_time = SysMillis();

        DrawRect(0.0f, 0.0f, (float)g_screenW, (float)g_screenH, 0.0f, 0.0f, 0.0f, 1.0f);
        ImgBlitAlphaRectFx(g_splash, 0.0f, 0.0f, 800.0f, 600.0f, 0, 0, 0.0f, 1.0f,
                                         blend > 1.0 ? 1.0f : blend, false, false, 0.0f, 0.0f);
        if (g_time > g_splashEnd && blend < 0.001f) return;

        blend = blend * mul;
        if (blend > 20.0 && g_time > g_splashEnd) {
            // switch from fade-in growth to fade-out decay once past the display time
            blend = 1.0f;
            mul = SPLASH_FADE_OUT_MUL;
        }

        g_mouseDown = MouseLeft();
        if ((KeyDown(K_VK_SPACE) || KeyDown(K_VK_ESCAPE) ||
             KeyDown(K_VK_L_CONTROL) || g_mouseDown != 0) &&
            g_time > g_splashMinEnd) {
            // drain the key(s) so the same press doesn't also skip the next screen
            WaitForKeyRelease(K_VK_SPACE);
            WaitForKeyRelease(K_VK_ESCAPE);
            WaitForKeyRelease(K_VK_L_CONTROL);
            return;
        }

        SysFlip();
        SysProcessEvents();
    } while (!done && !SysQuitRequested());
}

// Shows the game's title splash image; same fade/skip behavior as ShowLogoSplash().
void ShowTitleSplash()
{
    SoundStopAll();
    g_clipLeft = 0;
    g_clipRight = g_screenW;
    bool done = false;
    g_splash = LoadGraphic("splashscreen.jpg", true, false);
    if (g_splash == 0) return;
    int unused = 1;
    g_time = SysMillis();
    if (g_time == 0) g_time = SysMillis();
    g_splashEnd = g_time + 5000;
    g_splashMinEnd = g_time + 1000;
    float blend = 0.001f;
    float mul = SPLASH_FADE_IN_MUL;

    do {
        g_time = SysMillis();
        if (g_time == 0) g_time = SysMillis();

        DrawRect(0.0f, 0.0f, (float)g_screenW, (float)g_screenH, 0.0f, 0.0f, 0.0f, 1.0f);
        ImgBlitAlphaRectFx(g_splash, 0.0f, 0.0f, 800.0f, 600.0f, 0, 0, 0.0f, 1.0f,
                                         blend > 1.0 ? 1.0f : blend, false, false, 0.0f, 0.0f);
        if (g_time > g_splashEnd && blend < 0.001f) return;

        blend = blend * mul;
        if (blend > 20.0 && g_time > g_splashEnd) {
            blend = 1.0f;
            mul = SPLASH_FADE_OUT_MUL;
        }

        g_mouseDown = MouseLeft();
        if ((KeyDown(K_VK_SPACE) || KeyDown(K_VK_ESCAPE) ||
             KeyDown(K_VK_L_CONTROL) || g_mouseDown != 0) &&
            g_time > g_splashMinEnd) {
            WaitForKeyRelease(K_VK_SPACE);
            WaitForKeyRelease(K_VK_ESCAPE);
            WaitForKeyRelease(K_VK_L_CONTROL);
            return;
        }

        SysFlip();
        SysProcessEvents();
    } while (!done && !SysQuitRequested());
}

// Always-ready stub for a screen's "is ready" callback.
int SplashReadyStub()
{
    return 1;
}

// No-op stub for a screen's optional callback.
void EmptySplashStub()
{
}

// Deactivates all logo-flash particles.
void ClearFlashes()
{
    int i;
    for (i = 0; i < MAX_LOGO_FLASHES; i++)
        g_logoFlashes[i].active = 0;
}

// Spawns a new logo-flash particle at (x, y) in the first free slot, with randomized
// color, size, life, fade and outward velocity (angle/speed picked at random).
void AddLogoFlash(int x, int y)
{
    int i;
    float speed;
    for (i = 0; i < MAX_LOGO_FLASHES; i++) {
        if (!g_logoFlashes[i].active) {
            g_logoFlashes[i].active = 1;
            g_logoFlashes[i].x = x;
            g_logoFlashes[i].y = y;
            g_logoFlashes[i].alpha = RandFloat(100.0f, 150.0f);
            g_logoFlashes[i].fade = RandFloat(2.0f, 5.0f);
            g_logoFlashes[i].r = 0xff;
            g_logoFlashes[i].g = RandRange(0, 100);
            g_logoFlashes[i].b = RandRange(0, 100);
            g_logoFlashes[i].life = RandFloat(100.0f, 200.0f);
            g_logoFlashes[i].size = RandFloat(50.0f, 200.0f);
            g_logoFlashes[i].angle = RandRange(0, 359);
            speed = RandFloat(2.0f, 12.0f);
            g_logoFlashes[i].vx = g_cosDeg[g_logoFlashes[i].angle] * speed;
            g_logoFlashes[i].vy = 0.0f - g_sinDeg[g_logoFlashes[i].angle] * speed;
            return;
        }
    }
}

// Advances every active logo-flash particle: ages it out, fades its alpha, and moves
// it by its velocity.
void UpdateLogoFlashes()
{
    int i;
    for (i = 0; i < MAX_LOGO_FLASHES; i++) {
        if (g_logoFlashes[i].active) {
            g_logoFlashes[i].life -= 1.0;
            if (g_logoFlashes[i].life < 0.0)
                g_logoFlashes[i].active = 0;
            g_logoFlashes[i].alpha -= g_logoFlashes[i].fade;
            if (g_logoFlashes[i].alpha < 0.0)
                g_logoFlashes[i].alpha = 0;
            g_logoFlashes[i].x += g_logoFlashes[i].vx;
            g_logoFlashes[i].y += g_logoFlashes[i].vy;
        }
    }
}

// Draws every active logo-flash particle as a rotated glow image.
void DrawFlashes()
{
    int i;
    float angle;
    for (i = 0; i < MAX_LOGO_FLASHES; i++) {
        if (g_logoFlashes[i].active) {
            angle = 0.0 - g_logoFlashes[i].angle;
            if (angle < 0.0)
                angle = angle + 360.0;
            DrawImage(g_flashGfx,
                             g_logoFlashes[i].x - g_logoFlashes[i].size,
                             g_logoFlashes[i].y - g_logoFlashes[i].size,
                             g_logoFlashes[i].x + g_logoFlashes[i].size,
                             g_logoFlashes[i].y + g_logoFlashes[i].size,
                             g_logoFlashes[i].r, g_logoFlashes[i].g, g_logoFlashes[i].b,
                             (int)g_logoFlashes[i].alpha, 0, angle);
        }
    }
}

// Advances and draws one frame of the title-screen logo animation: applies a pending
// game-mode change, shows the news/update popup windows once per session, and on the
// first call resets the animation state. Steps the sequence through its stages (center
// bird -> left wing -> right wing -> wordmark -> flash -> banner -> final starburst),
// each gated on the previous stage completing, then draws the menu prompt and buttons.
void IntroFrame()
{
    float angle;

    if (g_pendingGameMode != -1) {
        g_gameMode = g_pendingGameMode;
        g_pendingGameMode = -1;
    }

    g_promoSpecialRank = 0;
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();

    g_introBlockedByWindow = 0;
    if (AnyWindowActive()) {
        g_introBlockedByWindow = 1;
        g_lastActivityTime = g_time;
        g_idleTimeoutMs = ATTRACT_SCREEN_MS;
    }

    if (g_introInit && !g_introBlockedByWindow) {
        ClearFlashes();
        g_introStageCenter = 1;
        g_introStageLeftWing = 0;
        g_introStageRightWing = 0;
        g_introStageWordmark = 0;
        g_introStageBanner = 0;
        g_introStageFlash = 0;
        g_introStageFinal = 0;
        g_centerScale = 1.0f;
        g_convergeFlash = 0.0f;
        g_bannerShrink = 600;
        g_bannerShrinkSpeed = 0.5f;

        g_leftWingX = 600.0f;
        g_leftWingY = 1300.0f;
        g_leftWingSize = 20.0f;
        g_leftWingAngleBase = 50.0f;
        g_leftWingAngleDecay = 110.0f;
        g_rightWingX = -600.0f;
        g_rightWingY = 1300.0f;
        g_rightWingSize = 20.0f;
        g_rightWingAngleBase = 310.0f;
        g_rightWingAngleDecay = 110.0f;
        g_wordmarkOffsetX = 0.0f;
        g_wordmarkShrinkW = 5500.0f;
        g_wordmarkShrinkH = 5500.0f;

        g_introInit = 0;
        DoNothing();
        g_starScale = 1.0f;
        g_starAlpha = 50.0f;
        g_starAlphaStep = RandFloat(1.0f, 4.0f) / 1.5;
        SoundPlay(g_sfxZoom, 54000, 200, 0.0f, 0xff, g_sndFlags);
        g_introSoundPlayed[0] = 1;
        g_introSoundPlayed[1] = 1;
        g_introSoundPlayed[2] = 1;
        g_introSoundPlayed[3] = 1;
        g_introSoundPlayed[4] = 1;
        g_introSoundPlayed[5] = 1;
        g_introSoundPlayed[6] = 1;
        g_introSoundPlayed[7] = 1;
        g_introSoundPlayed[8] = 1;
    }

    // ---- banner shrink and streaks ----
    if (g_introStageBanner) {
        g_bannerShrink -= (int)g_bannerShrinkSpeed;
        g_bannerShrinkSpeed *= 1.05f;
        if (g_bannerShrink < 0) {
            g_bannerShrink = 0;
            g_bannerShrinkSpeed = 0.0f;
            if (g_introSoundPlayed[3]) {
                SoundPlay(g_sfxThumpBig, -1, 0xff, 0.0f, 0x7f, g_sndFlags);
                g_introSoundPlayed[3] = 0;
            }

            g_streakX = 0.0f;
            g_streakAlpha = 0.0f;
            g_streakAlphaStep = 6.375f;
            g_streakActive = 1;
            g_streakColorR = 255;
            g_streakColorG = 0;
            g_streakColorB = 0;
            g_streakSpinSpeed0 = RandFloat(-10.0f, 10.0f);
            g_streakSpinSpeed1 = RandFloat(-10.0f, 10.0f);
            g_streakSpinSpeed2 = RandFloat(-10.0f, 10.0f);
        }
        if (g_bannerShrink < 500 && g_introSoundPlayed[8]) {
            SoundPlay(g_sfxComing, 32000, 0xff, 0.0f, 0x7f, g_sndFlags);
            g_introSoundPlayed[8] = 0;
        }
    }

    g_bannerY = 50 - g_bannerShrink;
    g_bannerCenterY = g_bannerY + 105;
    g_streakY = g_bannerCenterY - 40.0;
    g_bannerY = g_bannerY + 200;
    if (g_introStageBanner)
        Blit((g_screenW - 693) / 2, g_bannerY - 200, 0, g_gfxLogo3, 0, 0, 693, 110);

    if (g_introStageBanner) {
        QueueStretchF(g_gfxLogo3Glow,
                             (float)(g_bannerCenterX - g_bannerHalfW),
                             (float)(g_bannerCenterY - g_bannerHalfH),
                             (float)(g_bannerCenterX + g_bannerHalfW),
                             (float)(g_bannerCenterY + g_bannerHalfH),
                             g_streakColorR, 0, 0, (int)g_starPulseAlpha, 0);

        if (g_streakActive) {
            DrawImage(g_gfxFlare5, g_streakX - g_streakHalfSize, g_streakY - g_streakHalfSize,
                             g_streakX + g_streakHalfSize, g_streakY + g_streakHalfSize,
                             g_streakColorR, 0, 0, (int)g_streakAlpha, 0, g_angle0);
            DrawImage(g_gfxFlare8, g_streakX - g_streakHalfSize * 1.4f, g_streakY - g_streakHalfSize * 1.4f,
                             g_streakX + g_streakHalfSize * 1.4f, g_streakY + g_streakHalfSize * 1.4f,
                             g_streakColorR / 2, 0, 0, (int)(g_streakAlpha / 4.0), 0, g_angle1);
            DrawImage(g_gfxFlare8, g_streakX - g_streakHalfSize * 1.8f, g_streakY - g_streakHalfSize * 1.8f,
                             g_streakX + g_streakHalfSize * 1.8f, g_streakY + g_streakHalfSize * 1.8f,
                             g_streakColorR / 4, 0, 0, (int)(g_streakAlpha / 8.0), 0, g_angle2);

            g_angle0 += g_streakSpinSpeed0;
            if (g_angle0 > 360.0)
                g_angle0 -= 360.0;
            if (g_angle0 < 0.0)
                g_angle0 += 360.0;
            g_angle1 += g_streakSpinSpeed1;
            if (g_angle1 > 360.0)
                g_angle1 -= 360.0;
            if (g_angle1 < 0.0)
                g_angle1 += 360.0;
            g_angle2 += g_streakSpinSpeed2;
            if (g_angle2 > 360.0)
                g_angle2 -= 360.0;
            if (g_angle2 < 0.0)
                g_angle2 += 360.0;

            g_streakX += 10.0;
            g_streakAlpha += g_streakAlphaStep;
            if (g_streakAlpha > 255.0) {
                g_streakAlpha = 255.0f;
                g_streakAlphaStep = -6.375f;
            }
            if (g_streakAlpha < 0.0) {
                g_streakAlpha = 0.0f;
                g_streakAlphaStep = 0.0f;
            }
            if (g_streakX > g_screenW)
                g_streakActive = 0;
        }

        if (RandRange(0, 900) == 1 && !g_streakActive) {
            g_streakX = 0.0f;
            g_streakAlpha = 0.0f;
            g_streakAlphaStep = 6.375f;
            g_streakActive = 1;
            g_streakColorR = 255;
            g_streakColorG = 0;
            g_streakColorB = 0;
            g_streakSpinSpeed0 = RandFloat(-10.0f, 10.0f);
            g_streakSpinSpeed1 = RandFloat(-10.0f, 10.0f);
            g_streakSpinSpeed2 = RandFloat(-10.0f, 10.0f);
        }
    }

    // ---- final starburst ----
    if (g_introStageFinal) {
        DrawImage(g_gfxFlareAtmos,
                         g_logoAnchorX - (int)g_starScale - 20,
                         g_logoAnchorY + g_starYOffset - (int)g_starScale - 20,
                         g_logoAnchorX + (int)g_starScale + 20,
                         g_logoAnchorY + g_starYOffset + (int)g_starScale + 20,
                         0xff, 0, 0, 0x80, 0, g_starRayAngle1);
        DrawImage(g_gfxStar1,
                         g_logoAnchorX - (int)g_starScale - 40,
                         g_logoAnchorY + g_starYOffset - (int)g_starScale - 40,
                         g_logoAnchorX + (int)g_starScale + 40,
                         g_logoAnchorY + g_starYOffset + (int)g_starScale + 20,
                         0xff, 0, 0, 0x80, 0, g_starRayAngle2);
        DrawImage(g_gfxStar3,
                         g_logoAnchorX - (int)g_starScale + 20,
                         g_logoAnchorY + g_starYOffset - (int)g_starScale + 20,
                         g_logoAnchorX + (int)g_starScale - 20,
                         g_logoAnchorY + g_starYOffset + (int)g_starScale - 20,
                         0xff, 0, 0, 0x80, 0, g_starRayAngle3);

        DrawImage(g_starSprite,
                         g_logoAnchorX - (int)g_starScale, g_logoAnchorY + g_starYOffset - (int)g_starScale,
                         g_logoAnchorX + (int)g_starScale, g_logoAnchorY + g_starYOffset + (int)g_starScale,
                         0xff, 0, 0, 0x80, 0, g_starRayAngle4);
        DrawImage(g_gfxFlare10,
                         g_logoAnchorX - (int)g_starScale - 60,
                         g_logoAnchorY + g_starYOffset - (int)g_starScale - 60,
                         g_logoAnchorX + (int)g_starScale + 60,
                         g_logoAnchorY + g_starYOffset + (int)g_starScale + 60,
                         0xff, 0, 0, 0x80, 0, g_starRayAngle5);

        g_starRayAngle1 += 1.2f;
        if (g_starRayAngle1 > 360.0)
            g_starRayAngle1 -= 360.0;
        g_starRayAngle2 += 2.0;
        if (g_starRayAngle2 > 360.0)
            g_starRayAngle2 -= 360.0;
        g_starRayAngle3 += 3.2f;
        if (g_starRayAngle3 > 360.0)
            g_starRayAngle3 -= 360.0;
        g_starRayAngle4 -= 4.5;
        if (g_starRayAngle4 < 0.0)
            g_starRayAngle4 += 360.0;
        g_starRayAngle5 += 5.2f;
        if (g_starRayAngle5 > 360.0)
            g_starRayAngle5 -= 360.0;

        DrawImage(g_gfxLogoBirdFlare, g_logoAnchorX - 230, g_logoAnchorY - 225,
                         g_logoAnchorX + 230, g_logoAnchorY + 235,
                         0xff, 0, 0, (int)g_starAlpha, 0, 0.0f);

        g_starAlpha += g_starAlphaStep;
        if (g_starAlpha > 255.0) {
            g_starAlpha = 255.0f;
            g_starAlphaStep = -g_starAlphaStep;
        }
        if (g_starAlpha < 50.0) {
            g_starAlpha = 50.0f;
            g_starAlphaStep = -g_starAlphaStep;
        }
        if (RandRange(0, 200) == 2) {
            g_starAlphaStep = RandFloat(1.0f, 4.0f) / 1.5;
            g_starAlphaStep = -g_starAlphaStep;
        }
        if (RandRange(0, 100) < 30)
            AddLogoFlash(g_logoAnchorX, g_logoAnchorY);
        UpdateLogoFlashes();
        DrawFlashes();
    }

    if (g_introStageLeftWing) {
        angle = g_leftWingAngleBase + g_leftWingAngleDecay;
        if (angle > 360.0)
            angle -= 360.0;
        if (angle < 0.0)
            angle += 360.0;
        QueueStretchRot2(g_gfxLogoSwordGlow,
                                g_logoAnchorX + g_leftWingX - g_leftWingSize + g_convergeFlash - 34.0 - 57.0,
                                g_logoAnchorY + g_leftWingY - g_leftWingSize + g_convergeFlash - 57.0,
                                g_logoAnchorX + g_leftWingX + g_leftWingSize - g_convergeFlash - 34.0 + 57.0,
                                g_logoAnchorY + g_leftWingY + g_leftWingSize - g_convergeFlash + 57.0,
                                0xff, 0x40, 0,
                                (int)(g_starAlpha * 1.2f > 255.0 ? 255.0 : g_starAlpha * 1.2f), 0, angle);
        QueueStretchRot2(g_gfxLogoSword,
                                g_logoAnchorX + g_leftWingX - g_leftWingSize + g_convergeFlash - 34.0,
                                g_logoAnchorY + g_leftWingY - g_leftWingSize + g_convergeFlash,
                                g_logoAnchorX + g_leftWingX + g_leftWingSize - g_convergeFlash - 34.0,
                                g_logoAnchorY + g_leftWingY + g_leftWingSize - g_convergeFlash,
                                0xff, 0xff, 0xff, 0xff, 0, angle);

        if (g_leftWingSize > 180.0) {
            g_introStageRightWing = 1;
            if (g_introSoundPlayed[7]) {
                SoundPlay(g_sfxWhip, 32000, 200, 0.0f, 0x7f, g_sndFlags);
                g_introSoundPlayed[7] = 0;
            }
        }
        if (g_leftWingSize > 160.0 && g_introSoundPlayed[1]) {
            SoundPlay(g_sfxSword, 34000, 200, 0.0f, 0x7f, g_sndFlags);
            g_introSoundPlayed[1] = 0;
        }

        if (g_leftWingSize < 230.0) {
            g_leftWingSize *= 1.1f;
            g_leftWingX *= 0.87f;
            g_leftWingY *= 0.8f;
            g_leftWingAngleDecay *= 0.85f;
        } else {
            g_leftWingSize = 230.0f;
            g_leftWingY = 5.0f;
            g_leftWingX = 18.0f;
            g_leftWingAngleDecay = 0.0f;
        }
    }

    if (g_introStageRightWing) {
        angle = g_rightWingAngleBase - g_rightWingAngleDecay;
        if (angle > 360.0)
            angle -= 360.0;
        if (angle < 0.0)
            angle += 360.0;
        QueueStretchRot2(g_gfxLogoSwordGlow,
                                g_logoAnchorX + g_rightWingX - g_rightWingSize + g_convergeFlash
                                    + 34.0 - 57.0,
                                g_logoAnchorY + g_rightWingY - g_rightWingSize + g_convergeFlash - 57.0,
                                g_logoAnchorX + g_rightWingX + g_rightWingSize - g_convergeFlash
                                    + 34.0 + 57.0,
                                g_logoAnchorY + g_rightWingY + g_rightWingSize - g_convergeFlash + 57.0,
                                0xff, 0x40, 0,
                                (int)(g_starAlpha * 1.2f > 255.0 ? 255.0 : g_starAlpha * 1.2f), 0, angle);
        QueueStretchRot2(g_gfxLogoSword,
                                g_logoAnchorX + g_rightWingX - g_rightWingSize + g_convergeFlash + 34.0,
                                g_logoAnchorY + g_rightWingY - g_rightWingSize + g_convergeFlash,
                                g_logoAnchorX + g_rightWingX + g_rightWingSize - g_convergeFlash + 34.0,
                                g_logoAnchorY + g_rightWingY + g_rightWingSize - g_convergeFlash,
                                0xff, 0xff, 0xff, 0xff, 0, angle);

        if (g_rightWingSize > 188.0) {
            g_introStageWordmark = 1;
            if (g_introSoundPlayed[5]) {
                SoundPlay(g_sfxSwoosh, -1, 0xff, 0.0f, 0x7f, g_sndFlags);
                g_introSoundPlayed[5] = 0;
            }
        }
        if (g_rightWingSize > 160.0 && g_introSoundPlayed[2]) {
            SoundPlay(g_sfxSword, 34000, 200, 0.0f, 0x7f, g_sndFlags);
            g_introSoundPlayed[2] = 0;
        }

        if (g_rightWingSize < 230.0) {
            g_rightWingSize *= 1.1f;
            g_rightWingX *= 0.87f;
            g_rightWingY *= 0.8f;
            g_rightWingAngleDecay *= 0.85f;
        } else {
            g_rightWingSize = 230.0f;
            g_rightWingY = 5.0f;
            g_rightWingX = -18.0f;
            g_rightWingAngleDecay = 0.0f;
        }
    }
    if (g_introStageCenter) {
        QueueStretchRot2(g_gfxLogoBird,
                                g_logoAnchorX - g_centerScale + g_convergeFlash,
                                g_logoAnchorY - g_centerScale + g_convergeFlash,
                                g_logoAnchorX + g_centerScale - g_convergeFlash,
                                g_logoAnchorY + g_centerScale - g_convergeFlash,
                                0xff, 0xff, 0xff, 0xff, 0, 0.0f);

        g_centerScale *= 1.1f;
        if (g_centerScale > 180.0) {
            g_introStageLeftWing = 1;
            g_centerScale = 180.0f;
            if (g_introSoundPlayed[4]) {
                SoundPlay(g_sfxThump, 24000, 0xff, 0.0f, 0x7f, g_sndFlags);
                g_introSoundPlayed[4] = 0;
            }
            if (g_introSoundPlayed[6]) {
                SoundPlay(g_sfxWhip, 32000, 200, 0.0f, 0x7f, g_sndFlags);
                g_introSoundPlayed[6] = 0;
            }
        }
    }

    if (g_introStageWordmark) {
        QueueStretchRot2(g_gfxLogoFighterShadow,
                                g_logoAnchorX + g_wordmarkOffsetX - g_wordmarkShrinkH - 84.0 + 15.0,
                                g_logoAnchorY + g_wordmarkShrinkW - g_wordmarkShrinkH - 84.0 - 50.0 + 15.0,
                                g_logoAnchorX + g_wordmarkOffsetX + g_wordmarkShrinkH + 84.0 + 15.0,
                                g_logoAnchorY + g_wordmarkShrinkW + g_wordmarkShrinkH + 84.0 - 50.0 + 15.0,
                                0xff, 0xff, 0xff, 0x8c, 0, 0.0f);
        QueueStretchRot2(g_gfxLogoFighterShadow,
                                g_logoAnchorX + g_wordmarkOffsetX - g_wordmarkShrinkH - 75.0 + 8.0,
                                g_logoAnchorY + g_wordmarkShrinkW - g_wordmarkShrinkH - 75.0 - 50.0 + 8.0,
                                g_logoAnchorX + g_wordmarkOffsetX + g_wordmarkShrinkH + 75.0 + 8.0,
                                g_logoAnchorY + g_wordmarkShrinkW + g_wordmarkShrinkH + 75.0 - 50.0 + 8.0,
                                0xff, 0xff, 0xff, 200, 0, 0.0f);
        QueueStretchRot2(g_gfxLogoFighter,
                                g_logoAnchorX + g_wordmarkOffsetX - g_wordmarkShrinkH - 84.0,
                                g_logoAnchorY + g_wordmarkShrinkW - g_wordmarkShrinkH - 84.0 - 50.0,
                                g_logoAnchorX + g_wordmarkOffsetX + g_wordmarkShrinkH + 84.0,
                                g_logoAnchorY + g_wordmarkShrinkW + g_wordmarkShrinkH + 84.0 - 50.0,
                                0xff, 0xff, 0xff, 0xff, 0, 0.0f);

        g_starScale *= 1.2f;
        if (g_starScale > 300.0)
            g_starScale = 300.0f;
        if (g_wordmarkShrinkW > 0.0) {
            g_wordmarkShrinkW *= 0.8f;
            g_wordmarkShrinkH *= 0.8f;
            if (g_wordmarkShrinkW < 5.0) {
                g_convergeFlash = 100.0f;
                g_introStageFlash = 1;
                g_wordmarkShrinkW = 0.0f;
                g_wordmarkOffsetX = 0.0f;
                g_wordmarkShrinkH = 0.0f;
                if (g_introSoundPlayed[0]) {
                    SoundPlay(g_sfxMetal, -1, 0xff, 0.0f, 0x7f, g_sndFlags);
                    g_introSoundPlayed[0] = 0;
                }
            }
            if (g_wordmarkShrinkW < 10.0)
                g_introStageBanner = 1;
        }
    }

    if (g_introStageFlash) {
        g_convergeFlash *= 0.6f;
        if (g_convergeFlash < 1.0) {
            g_convergeFlash = 0.0f;
            g_introStageFlash = 0;
            g_introStageFinal = 1;
        }
    }
    g_starPulseAlpha += g_starPulseAlphaStep;
    if (g_starPulseAlpha >= 255.0) {
        g_starPulseAlpha = 255.0f;
        g_starPulseAlphaStep = 0 - g_starPulseAlphaStep;
    }
    if (g_starPulseAlpha < 150.0) {
        g_starPulseAlpha = 150.0f;
        g_starPulseAlphaStep = 0 - g_starPulseAlphaStep;
    }
    DrawMenuPrompt();
    if (g_debug != 0)
        DrawTextBox(100, 100, 500, 500, " ");
    HidePageButtons(1);
    DrawButtons(0);
}

// Tears down the current run and returns to the title/attract screen: banks play time,
// resets counters and the title-intro stage machine, and starts the title music.
void ResetToTitle()
{
    // Reset the rank/promo popup state.
    SysSetMaxFps(FPS_NORMAL);
    g_bgIndex = 1;
    g_rankLockUntil = g_time;
    g_rankPopupMinTime = g_time;
    g_hiscoreSkipGateActive = 0;
    g_rankMsgActive = 0;
    g_promoSpecialRank = 0;
    g_promoRingActive = 0;

    // Clear the per-frame draw-call counters and restore the run's fps/difficulty.
    CLEAR_DRAW_COUNTERS()
    g_playerBroke = 1;
    g_introInit = 1;
    if (g_gameMode == MODE_TIME_TRIAL)
        g_cfg.difficulty = g_savedDifficultyTT;
    g_cfg.fps = FPS_NORMAL;
    DoNothing();

    // Bank the elapsed play time into the profile/session totals.
    if (g_playerUpdateFn != StateDemo) {
        StampTimeC();
        g_timeStampA = g_timeA;
        g_timeStampB = g_timeMarkC;
        if (!g_playTimeAdded) {
            g_cfg.playTime += g_timeStampB - g_timeStampA - g_pausedDuration;
            if (g_cfg.playTime < 0)
                g_cfg.playTime = 0;
            g_playTimeAdded = true;
        }
        if (!g_profilePlayTimeAdded)
            AddPlayTime(g_profileIndex, g_timeStampB, g_timeStampA, g_pausedDuration);
    }

    // Reset idle/attract-mode tracking and drop back to the title state.
    g_attractScreen = 0;
    g_idleTimeoutMs = 30000;
    g_lastActivityTime = g_time;
    g_titleResetPending = 0;
    g_state = STATE_TITLE;
    ClearPlayers();
    g_save.players[0].hyperspaceOutTimer = 0.0f;
    g_save.players[0].hyperspaceMidTimer = 0.0f;
    g_save.players[0].hyperspaceInTimer = 0.0f;
    g_save.players[0].hyperspaceInDuration = 100.0f;
    g_save.players[0].hyperspaceFade = 0.0f;
    InitStarRotation();
    g_viewTransitionFlag = 4;
    g_stateFn = SetViewHud;
    g_playerUpdateFn = UpdatePlayer;
    g_menuIdleTimeout = g_time + MENU_IDLE_MS;
    PlayTitleMusic();
    g_inputCooldown = 100;
    g_state = STATE_TITLE;
    g_hiscoreEntryReset = 0;
    g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
    g_transitionLock = 1;
    ClearFlashes();

    // Re-arm the title-screen wordmark/wing/banner intro stage machine.
    g_introStageCenter = 1;
    g_introStageLeftWing = 0;
    g_introStageRightWing = 0;
    g_introStageWordmark = 0;
    g_introStageBanner = 0;
    g_introStageFlash = 0;
    g_introStageFinal = 0;
    g_centerScale = 1.0f;
    g_convergeFlash = 0.0f;
    g_bannerShrink = 600;
    g_bannerShrinkSpeed = 0.5f;
    g_leftWingX = 600.0f;
    g_leftWingY = 1300.0f;
    g_leftWingSize = 20.0f;
    g_leftWingAngleBase = 50.0f;
    g_leftWingAngleDecay = 110.0f;

    g_rightWingX = -600.0f;
    g_rightWingY = 1300.0f;
    g_rightWingSize = 20.0f;
    g_rightWingAngleBase = 310.0f;
    g_rightWingAngleDecay = 110.0f;
    g_wordmarkOffsetX = 0.0f;
    g_wordmarkShrinkW = 5500.0f;
    g_wordmarkShrinkH = 5500.0f;
    g_introInit = 0;
    DoNothing();

    // Re-arm the title-screen star field and its zoom sound, and mark all intro voice
    // lines as already played so they don't fire again.
    g_starScale = 1.0f;
    g_starAlpha = 50.0f;
    g_starAlphaStep = RandFloat(1.0f, 4.0f) / 1.5;
    SoundPlay(g_sfxZoom, 54000, 200, 0.0f, 255, g_sndFlags);
    g_introSoundPlayed[0] = 1;
    g_introSoundPlayed[1] = 1;
    g_introSoundPlayed[2] = 1;
    g_introSoundPlayed[3] = 1;
    g_introSoundPlayed[4] = 1;
    g_introSoundPlayed[5] = 1;
    g_introSoundPlayed[6] = 1;
    g_introSoundPlayed[7] = 1;
    g_introSoundPlayed[8] = 1;
    CLEAR_DRAW_COUNTERS()
}
