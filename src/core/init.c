// init.c: Start-up and shutdown: the window, loading all game data, init failure, resource
// teardown.
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "game.h"
#include "sdlhelp.h"


// Creates the game window and its 800x600 back buffer. Called once at startup.
// Returns false if no window could be created.
//
// The configured renderer is tried first; if SDL can't create it (not built in, or no driver
// for it on this machine), the setting goes back to auto and SDL picks one.

// SDL_HINT_RENDER_DRIVER value per RendererChoice.
static const char *const s_rendererHints[RENDERER_COUNT] = {
    0, 0, "direct3d", "direct3d11", "direct3d12", "opengl", "vulkan"
};

// The choice a stored g_cfg.renderer byte stands for: RENDERER_AUTO unless it names a driver.
int RendererChoiceOf(int value)
{
    return value > RENDERER_AUTO && value < RENDERER_COUNT ? value : RENDERER_AUTO;
}

// Passes the vsync and interpolation settings to the engine; they apply from the next frame.
void ApplyFrameSettings(void)
{
    SysSetVSync(g_cfg.vsyncOff != 1);
    SysSetInterpolation(g_cfg.interpolation);
}

// ALT + V (title screen and in game) and the settings-page button: toggles vsync, applies
// it and saves the settings. Returns the hotkey banner text.
const char *ToggleVSync(void)
{
    g_cfg.vsyncOff = g_cfg.vsyncOff == 1 ? 0 : 1;
    ApplyFrameSettings();
    MergeSettings(g_profileIndex);
    WriteSettings();
    return g_cfg.vsyncOff == 1 ? "VSYNC : OFF" : "VSYNC : ON";
}

// The S key (title screen and in game) and the settings-page button: cycles interpolation
// auto -> on -> off, applies it and saves the settings. Returns the hotkey banner text; auto
// also says what it chose.
const char *CycleInterpolation(void)
{
    g_cfg.interpolation = (unsigned char)(g_cfg.interpolation + 1);
    if (g_cfg.interpolation >= SYS_INTERP_COUNT)
        g_cfg.interpolation = SYS_INTERP_AUTO;
    ApplyFrameSettings();
    MergeSettings(g_profileIndex);
    WriteSettings();
    if (g_cfg.interpolation == SYS_INTERP_ON)
        return "INTERPOLATION : ON";
    if (g_cfg.interpolation == SYS_INTERP_OFF)
        return "INTERPOLATION : OFF";
    return SysInterpolating() ? "INTERPOLATION : AUTO - ON" : "INTERPOLATION : AUTO - OFF";
}

bool InitWindow(bool windowed)
{
    int renderer = RendererChoiceOf(g_cfg.renderer);

    ApplyFrameSettings();

    g_screenW = 800;
    g_screenH = 600;
    g_screenHInit = 600;
    g_worldZoomInit = 1.0f;
    g_worldZoom = g_worldZoomInit;

    g_cfg.windowed = windowed;
    if (renderer != RENDERER_AUTO &&
        !SysCreateWindow(g_screenW, g_screenH, windowed, "Warblade SR 2.0", s_rendererHints[renderer])) {
        sprintf(g_logBuf, "ERROR :  Could not create the %s renderer, using auto\n",
                s_rendererHints[renderer]);
        LogPrint(g_logBuf);
        renderer = RENDERER_AUTO;
        g_cfg.renderer = RENDERER_AUTO;
        WriteSettings();
    }
    if (renderer == RENDERER_AUTO &&
        !SysCreateWindow(g_screenW, g_screenH, windowed, "Warblade SR 2.0", 0)) {
        LogPrint("ERROR :  Could not open window\n");
        return false;
    }
    g_windowRenderer = (unsigned char)renderer;
    sprintf(g_logBuf, "Renderer : %s\n", SysRendererName());
    LogPrint(g_logBuf);
    g_offsetX = (float)((g_screenW - 800) / 2 < 0 ? 0 : (g_screenW - 800) / 2);
    g_offsetY = (float)((g_screenH - 600) / 2 < 0 ? 0 : (g_screenH - 600) / 2);
    g_offsetXi = (int)g_offsetX;
    g_offsetYi = (int)g_offsetY;
    return true;
}

// Resets the clip rect and creates the window with the configured windowed setting.
int InitWindowCfg()
{
    g_clipLeft = 0;
    g_clipTop = 0;
    g_clipRight = g_screenW;
    g_clipBottom = g_screenH;
    return InitWindow(g_cfg.windowed);
}

// Shows the "LOADING DATA" screen, then loads every graphic/HMA used by the game
// (fonts, ships, flares, stars, UI, explosions, logos, etc.), seeds the starfield and
// builds the flash colour ramps. Called once from InitGame(). Returns 0 if any graphic
// fails to load, 1 on success.
int LoadGameData()
{
    int rnd;
    int i;
    int j;
    int k;
    float r;
    float g;
    float b;
    float dr;
    float dg;
    float db;
    int a;
    int c;
    int d;

    // Loading screen (shown twice, matching the original double flip).
    g_smallFont = LoadGraphic2("abcd_3.tga", true, true);
    if (g_smallFont == 0) return 0;
    SysSetMaxFps(60);
    SysSetClearColor(0, 0, 0, 1.0f);
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    SysFlip();
    SysSetMaxFps(60);
    SysSetClearColor(0, 0, 0, 1.0f);
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    SysFlip();
    SysSetClearColor(0, 0, 0, 1.0f);
    SysSetWorldView(g_worldViewX, g_worldViewY, g_worldViewRotation, g_worldZoom, true);
    DrawMenuText("WARBLADE SR VERSION 2.0", POS_CENTERED, 0x10e, 2);
    DrawMenuText("L O A D I N G   D A T A", POS_CENTERED, 0x136, 2);
    FlushBlit(0);
    FlushQuads(0);
    FlushStretchF();
    FlushStretchRot();
    FlushStretchI();
    FlushStretchRot2();
    FlushBlit2(0);
    FlipBuffer(0);

    // Ship graphics and HMAs.
    g_gfxFighter1 = LoadGraphic2("fighter1.tga", true, true);
    if (g_gfxFighter1 == 0) return 0;
    g_ship1Hma = LoadHma("fighter1", (int)ImgWidth(g_gfxFighter1), (int)ImgHeight(g_gfxFighter1));
    g_ship1GfxParamA = 440;
    g_ship1GfxParamB = 27;
    g_gfxFighter2 = LoadGraphic2("fighter2.tga", true, true);
    if (g_gfxFighter2 == 0) return 0;
    g_ship2Hma = LoadHma("fighter2", (int)ImgWidth(g_gfxFighter2), (int)ImgHeight(g_gfxFighter2));
    g_ship2GfxParamA = 440;
    g_ship2GfxParamB = 27;

    // Fonts and small misc graphics.
    g_tinyFont = LoadGraphic2("abcd_2.tga", true, true);
    if (g_tinyFont == 0) return 0;
    g_digitFont = LoadGraphic2("abcd_4.tga", true, true);
    if (g_digitFont == 0) return 0;
    g_fontGfx = LoadGraphic2("endfont.tga", true, true);
    if (g_fontGfx == 0) return 0;
    g_gfxSparks = LoadGraphic("sparks.tga", true, true);
    if (g_gfxSparks == 0) return 0;
    g_gfxLogos = LoadGraphic2("div.tga", true, true);
    if (g_gfxLogos == 0) return 0;

    // Flare graphics (numbered 1-34, skipping 22).
    g_gfxFlare1 = LoadGraphic2("flare1.tga", true, true);
    if (g_gfxFlare1 == 0) return 0;
    g_gfxFlare2 = LoadGraphic2("flare2.tga", true, true);
    if (g_gfxFlare2 == 0) return 0;
    g_gfxFlare3 = LoadGraphic2("flare3.tga", true, true);
    if (g_gfxFlare3 == 0) return 0;
    g_gfxFlare4 = LoadGraphic2("flare4.tga", true, true);
    if (g_gfxFlare4 == 0) return 0;
    g_gfxFlare5 = LoadGraphic2("flare5.tga", true, true);
    if (g_gfxFlare5 == 0) return 0;
    g_gfxFlare6 = LoadGraphic2("flare6.tga", true, true);
    if (g_gfxFlare6 == 0) return 0;
    g_gfxFlare7 = LoadGraphic2("flare7.tga", true, true);
    if (g_gfxFlare7 == 0) return 0;
    g_gfxFlare8 = LoadGraphic2("flare8.tga", true, true);
    if (g_gfxFlare8 == 0) return 0;

    g_gfxFlare9 = LoadGraphic2("flare9.tga", true, true);
    if (g_gfxFlare9 == 0) return 0;
    g_gfxFlare10 = LoadGraphic2("flare10.tga", true, true);
    if (g_gfxFlare10 == 0) return 0;
    g_gfxFlare11 = LoadGraphic2("flare11.tga", true, true);
    if (g_gfxFlare11 == 0) return 0;
    g_gfxFlare12 = LoadGraphic2("flare12.tga", true, true);
    if (g_gfxFlare12 == 0) return 0;
    g_gfxFlare13 = LoadGraphic2("flare13.tga", true, true);
    if (g_gfxFlare13 == 0) return 0;
    g_gfxFlare14 = LoadGraphic2("flare14.tga", true, true);
    if (g_gfxFlare14 == 0) return 0;
    g_gfxFlare15 = LoadGraphic2("flare15.tga", true, true);
    if (g_gfxFlare15 == 0) return 0;
    g_gfxFlare16 = LoadGraphic2("flare16.tga", true, true);
    if (g_gfxFlare16 == 0) return 0;

    g_gfxFlare17 = LoadGraphic2("flare17.tga", true, true);
    if (g_gfxFlare17 == 0) return 0;
    g_gfxFlare18 = LoadGraphic2("flare18.tga", true, true);
    if (g_gfxFlare18 == 0) return 0;
    g_gfxFlare19 = LoadGraphic2("flare19.tga", true, true);
    if (g_gfxFlare19 == 0) return 0;
    g_gfxFlare20 = LoadGraphic2("flare20.tga", true, true);
    if (g_gfxFlare20 == 0) return 0;
    g_gfxFlare21 = LoadGraphic2("flare21.tga", true, true);
    if (g_gfxFlare21 == 0) return 0;
    g_gfxFlare23 = LoadGraphic2("flare23.tga", true, true);
    if (g_gfxFlare23 == 0) return 0;
    g_gfxFlare24 = LoadGraphic2("flare24.tga", true, true);
    if (g_gfxFlare24 == 0) return 0;

    g_gfxFlare25 = LoadGraphic2("flare25.tga", true, true);
    if (g_gfxFlare25 == 0) return 0;
    g_gfxFlare26 = LoadGraphic2("flare26.tga", true, true);
    if (g_gfxFlare26 == 0) return 0;
    g_gfxFlare27 = LoadGraphic2("flare27.tga", true, true);
    if (g_gfxFlare27 == 0) return 0;
    g_gfxFlare28 = LoadGraphic2("flare28.tga", true, true);
    if (g_gfxFlare28 == 0) return 0;
    g_gfxFlare29 = LoadGraphic2("flare29.tga", true, true);
    if (g_gfxFlare29 == 0) return 0;
    g_gfxFlare30 = LoadGraphic2("flare30.tga", true, true);
    if (g_gfxFlare30 == 0) return 0;
    g_gfxFlare31 = LoadGraphic2("flare31.tga", true, true);
    if (g_gfxFlare31 == 0) return 0;
    g_gfxFlare32 = LoadGraphic2("flare32.tga", true, true);
    if (g_gfxFlare32 == 0) return 0;
    g_gfxFlare33 = LoadGraphic2("flare33.tga", true, true);
    if (g_gfxFlare33 == 0) return 0;
    g_gfxFlare34 = LoadGraphic2("flare34.tga", true, true);
    if (g_gfxFlare34 == 0) return 0;
    g_gfxFlarePlanet = LoadGraphic2("flareplanet.tga", true, true);
    if (g_gfxFlarePlanet == 0) return 0;

    // setAlphaMode(4): additive blend, used for glowing/flare-type sprites throughout.
    ImgSetAlphaMode(g_gfxFlare1, 4);
    ImgSetAlphaMode(g_gfxFlare2, 4);
    ImgSetAlphaMode(g_gfxFlare3, 4);
    ImgSetAlphaMode(g_gfxFlare4, 4);
    ImgSetAlphaMode(g_gfxFlare5, 4);
    ImgSetAlphaMode(g_gfxFlare6, 4);
    ImgSetAlphaMode(g_gfxFlare7, 4);
    ImgSetAlphaMode(g_gfxFlare8, 4);
    ImgSetAlphaMode(g_gfxFlare9, 4);
    ImgSetAlphaMode(g_gfxFlare10, 4);
    ImgSetAlphaMode(g_gfxFlare11, 4);
    ImgSetAlphaMode(g_gfxFlare12, 4);
    ImgSetAlphaMode(g_gfxFlare13, 4);
    ImgSetAlphaMode(g_gfxFlare14, 4);
    ImgSetAlphaMode(g_gfxFlare15, 4);
    ImgSetAlphaMode(g_gfxFlare16, 4);

    ImgSetAlphaMode(g_gfxFlare17, 4);
    ImgSetAlphaMode(g_gfxFlare18, 4);
    ImgSetAlphaMode(g_gfxFlare19, 4);
    ImgSetAlphaMode(g_gfxFlare20, 4);
    ImgSetAlphaMode(g_gfxFlare21, 4);
    ImgSetAlphaMode(g_gfxFlare23, 4);
    ImgSetAlphaMode(g_gfxFlare24, 4);
    ImgSetAlphaMode(g_gfxFlare25, 4);
    ImgSetAlphaMode(g_gfxFlare26, 4);
    ImgSetAlphaMode(g_gfxFlare27, 4);
    ImgSetAlphaMode(g_gfxFlare28, 4);
    ImgSetAlphaMode(g_gfxFlare29, 4);
    ImgSetAlphaMode(g_gfxFlare30, 4);
    ImgSetAlphaMode(g_gfxFlare31, 4);
    ImgSetAlphaMode(g_gfxFlare32, 4);
    ImgSetAlphaMode(g_gfxFlare33, 4);
    ImgSetAlphaMode(g_gfxFlare34, 4);

    // Flare graphic lookup table, indexed by flare id.
    g_gfxTable[0] = g_gfxFlare1;
    g_gfxTable[1] = g_gfxFlare2;
    g_gfxTable[2] = g_gfxFlare3;
    g_gfxTable[3] = g_gfxFlare4;
    g_gfxTable[4] = g_gfxFlare5;
    g_gfxTable[5] = g_gfxFlare6;
    g_gfxTable[6] = g_gfxFlare7;
    g_gfxTable[7] = g_gfxFlare8;
    g_gfxTable[8] = g_gfxFlare9;
    g_gfxTable[9] = g_gfxFlare10;
    g_gfxTable[10] = g_gfxFlare11;
    g_gfxTable[11] = g_gfxFlare12;
    g_gfxTable[12] = g_gfxFlare13;
    g_gfxTable[13] = g_gfxFlare14;
    g_gfxTable[14] = g_gfxFlare15;
    g_gfxTable[15] = g_gfxFlare16;

    g_gfxTable[16] = g_gfxFlare17;
    g_gfxTable[17] = g_gfxFlare18;
    g_gfxTable[18] = g_gfxFlare19;
    g_gfxTable[19] = g_gfxFlare20;
    g_gfxTable[20] = g_gfxFlare21;
    g_gfxTable[21] = g_gfxFlarePlanet;
    g_gfxTable[22] = g_gfxFlare23;
    g_gfxTable[23] = g_gfxFlare24;
    g_gfxTable[24] = g_gfxFlare25;
    g_gfxTable[25] = g_gfxFlare26;
    g_gfxTable[26] = g_gfxFlare27;
    g_gfxTable[27] = g_gfxFlare28;
    g_gfxTable[28] = g_gfxFlare29;
    g_gfxTable[29] = g_gfxFlare30;
    g_gfxTable[30] = g_gfxFlare31;
    g_gfxTable[31] = g_gfxFlare32;
    g_gfxTable[32] = g_gfxFlare33;
    g_gfxTable[33] = g_gfxFlare34;
    g_gfxTable[34] = g_gfxFlareSpark;

    // Star sprites and difficulty border graphics.
    g_gfxStar1 = LoadGraphic("star_1.tga", true, true);
    if (g_gfxStar1 == 0) return 0;
    g_starSprite = LoadGraphic("star_2.tga", true, true);
    if (g_starSprite == 0) return 0;
    g_gfxStar3 = LoadGraphic("star_3.tga", true, true);
    if (g_gfxStar3 == 0) return 0;
    ImgSetAlphaMode(g_gfxStar1, 4);
    ImgSetAlphaMode(g_starSprite, 4);
    ImgSetAlphaMode(g_gfxStar3, 4);

    // Difficulty border graphics, weapon/bonus-item graphics and HMAs.
    g_gfxBorderEasy = LoadGraphic("border_easy.jpg", true, true);
    if (g_gfxBorderEasy == 0) return 0;
    g_gfxBorderNormal = LoadGraphic("border.jpg", true, true);
    if (g_gfxBorderNormal == 0) return 0;
    g_gfxBorderHard = LoadGraphic("border_hard.jpg", true, true);
    if (g_gfxBorderHard == 0) return 0;
    g_gfxBorderAce = LoadGraphic("border_ace.jpg", true, true);
    if (g_gfxBorderAce == 0) return 0;
    g_gfxWeaponsBig = LoadGraphic2("weapons_big.tga", true, true);
    if (g_gfxWeaponsBig == 0) return 0;
    g_hmaWeaponsBig = LoadHma("weapons_big", (int)ImgWidth(g_gfxWeaponsBig),
                               (int)ImgHeight(g_gfxWeaponsBig));
    g_gfxWeaponsBigW = 672;
    g_gfxWeaponsBigH = 100;
    g_gfxBonus = LoadGraphic2("bonuses.tga", true, true);
    if (g_gfxBonus == 0) return 0;
    g_hmaBonuses = LoadHma("bonuses", (int)ImgWidth(g_gfxBonus), (int)ImgHeight(g_gfxBonus));
    g_bonusItemField34 = 200;
    g_bonusItemGfxH = 740;

    g_gfxDiamond = LoadGraphic2("diamant.tga", true, true);
    if (g_gfxDiamond == 0) return 0;
    g_hmaDiamond = LoadHma("diamant", (int)ImgWidth(g_gfxDiamond), (int)ImgHeight(g_gfxDiamond));
    g_diamondGfxW = 64;
    g_diamondGfxH = 143;
    g_gfxDiamondBig = LoadGraphic2("diamantbig.tga", true, true);
    if (g_gfxDiamondBig == 0) return 0;
    g_hmaDiamondBig = LoadHma("diamantbig", (int)ImgWidth(g_gfxDiamondBig),
                               (int)ImgHeight(g_gfxDiamondBig));
    g_diamondBigGfxW = 240;
    g_diamondBigGfxH = 561;
    g_gfxCoins = LoadGraphic2("marks.tga", true, true);
    if (g_gfxCoins == 0) return 0;
    g_hmaMarks = LoadHma("marks", (int)ImgWidth(g_gfxCoins), (int)ImgHeight(g_gfxCoins));
    g_coinGfxW = 200;
    g_coinGfxH = 140;

    // Logos and starfield backgrounds.
    g_gfxLogo3 = LoadGraphic2("newlogo3.tga", true, true);
    if (g_gfxLogo3 == 0) return 0;
    g_gfxLogo3Glow = LoadGraphic2("newlogo3_glow_bigfish.tga", true, true);
    if (g_gfxLogo3Glow == 0) return 0;
    ImgSetAlphaMode(g_gfxLogo3Glow, 4);
    g_gfxLogoBird = LoadGraphic2("warblade_logo_bird.tga", true, true);
    if (g_gfxLogoBird == 0) return 0;
    g_gfxLogoBirdFlare = LoadGraphic2("warblade_logo_bird_flare.tga", true, true);
    if (g_gfxLogoBirdFlare == 0) return 0;
    ImgSetAlphaMode(g_gfxLogoBirdFlare, 4);
    g_logoBirdFlareGfxW = 64;
    g_logoBirdFlareGfxH = 70;
    g_bg1 = LoadGraphic("stars1.jpg", true, true);
    if (g_bg1 == 0) return 0;
    g_bg2 = LoadGraphic("stars2.jpg", true, true);
    if (g_bg2 == 0) return 0;
    g_bg3 = LoadGraphic("stars3.jpg", true, true);
    if (g_bg3 == 0) return 0;
    g_bg4 = LoadGraphic("stars4.jpg", true, true);
    if (g_bg4 == 0) return 0;
    g_bg5 = LoadGraphic("stars5.jpg", true, true);
    if (g_bg5 == 0) return 0;

    // Seed the 3000-star parallax field: position/depth plus a size band picked from a
    // weighted percentage roll (rnd), and a sprite (mostly g_gfxStar1, rarely 2/3).
    rnd = 0;
    for (i = 0; i < NUM_STARS; i++) {
        g_stars[i].y = RandFloat(-550.0f, (float)g_screenH);
        g_stars[i].x = RandFloat(0, (float)g_screenW);
        g_stars[i].z = RandFloat(1.0f, 16.0f);
        g_starZ[i] = RandFloat(g_starZNear, g_starZFar);
        g_starX[i] = RandFloat(g_starXMin, g_starXMax);
        g_starY[i] = RandFloat(g_starYMin, g_starYMax);
        rnd = RandRange(0, 100);
        if (rnd < 50) g_starA[i] = RandFloat(1.0f, 8.0f);
        if (rnd >= 50 && rnd < 70) g_starA[i] = RandFloat(8.0f, 12.0f);
        if (rnd >= 70 && rnd < 90) g_starA[i] = RandFloat(12.0f, 16.0f);
        if (rnd >= 90 && rnd < 98) g_starA[i] = RandFloat(16.0f, 60.0f);
        if (rnd >= 98) g_starA[i] = RandFloat(60.0f, 100.0f);
        if (rnd >= 0 && rnd < 95) g_starGfx[i] = g_gfxStar1;
        if (rnd >= 95) {
            if (RandRange(0, 2) == 0)
                g_starGfx[i] = g_starSprite;
            else
                g_starGfx[i] = g_gfxStar3;
        }
    }

    for (j = 0; j < MAX_SPARKLE_FLASHES; j++) {
        g_sparkleFlashes[j].active = 0;
    }
    g_gfxPause = LoadGraphic2("pause4.tga", true, true);
    if (g_gfxPause == 0) return 0;
    ImgSetAlphaMode(g_gfxPause, 4);
    for (k = 0; k < MAX_FX; k++) {
        if (g_fx[k].active != 0) g_fx[k].gfx = g_gfxPause;
    }

    // Misc HUD and game-over/ranking graphics.
    g_gfxGameOver = LoadGraphic2("gameover.tga", true, true);
    if (g_gfxGameOver == 0) return 0;
    g_gfxSkull = LoadGraphic2("skalle.tga", true, true);
    if (g_gfxSkull == 0) return 0;
    ImgSetAlphaMode(g_gfxSkull, 4);
    g_gfxRanks = LoadGraphic2("ranks2.tga", true, true);
    if (g_gfxRanks == 0) return 0;
    g_gfxRankIcons = LoadGraphic2("ranks3.tga", true, true);
    if (g_gfxRankIcons == 0) return 0;

    g_gfxMothership = LoadGraphic2("mothership2.png", true, true);
    if (g_gfxMothership == 0) return 0;
    g_gfxMothershipMask = LoadGraphic2("mothership2_mask.png", true, true);
    if (g_gfxMothershipMask == 0) return 0;
    g_mothershipMaskW = 288;
    g_mothershipMaskH = 512;
    g_gfxBeam = LoadGraphic2("beam.tga", true, true);
    if (g_gfxBeam == 0) return 0;
    g_gfxBird = LoadGraphic2("bird.tga", true, true);
    if (g_gfxBird == 0) return 0;
    g_gfxSword = LoadGraphic2("sverd.tga", true, true);
    if (g_gfxSword == 0) return 0;
    g_gfxRankPlanets = LoadGraphic2("planeter.tga", true, true);
    if (g_gfxRankPlanets == 0) return 0;
    g_gfxShopBg = LoadGraphic2("butikk3.png", true, true);
    if (g_gfxShopBg == 0) return 0;

    // Explosion/particle/flare-streak effect graphics (additive blend).
    g_explGfx = LoadGraphic2("explo.tga", true, true);
    if (g_explGfx == 0) return 0;
    ImgSetAlphaMode(g_explGfx, 4);
    g_explGfx2 = LoadGraphic2("expl_small.tga", true, true);
    if (g_explGfx2 == 0) return 0;
    ImgSetAlphaMode(g_explGfx2, 4);
    g_gfxFlareSpark = LoadGraphic2("flare_2.tga", true, true);
    if (g_gfxFlareSpark == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareSpark, 4);
    g_gfxFlareAtmos = LoadGraphic2("flare_atmos.tga", true, true);
    if (g_gfxFlareAtmos == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareAtmos, 4);
    g_sparkGfx = LoadGraphic2("flare_line.tga", true, true);
    if (g_sparkGfx == 0) return 0;
    ImgSetAlphaMode(g_sparkGfx, 4);

    g_gfxFlareScoop = LoadGraphic2("flare_scoop.tga", true, true);
    if (g_gfxFlareScoop == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareScoop, 4);
    g_flashGfx = LoadGraphic2("flare_streak.tga", true, true);
    if (g_flashGfx == 0) return 0;
    ImgSetAlphaMode(g_flashGfx, 4);
    g_gfxFlareStreakBig = LoadGraphic2("flare_streak_big.tga", true, true);
    if (g_gfxFlareStreakBig == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareStreakBig, 4);
    g_gfxFlareStreakGuard = LoadGraphic2("flare_streak_guard.tga", true, true);
    if (g_gfxFlareStreakGuard == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareStreakGuard, 4);
    g_gfxFlareLaser = LoadGraphic2("flare_laser.tga", true, true);
    if (g_gfxFlareLaser == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareLaser, 4);

    g_gfxFlareBomb = LoadGraphic2("flarebomb.tga", true, true);
    if (g_gfxFlareBomb == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareBomb, 4);
    g_gfxFlareBomb2 = LoadGraphic2("flarebomb2.tga", true, true);
    if (g_gfxFlareBomb2 == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareBomb2, 4);
    g_gfxFlareBomb3 = LoadGraphic2("flarebomb3.tga", true, true);
    if (g_gfxFlareBomb3 == 0) return 0;
    ImgSetAlphaMode(g_gfxFlareBomb3, 4);

    // Logos, guard, meteor and prize (money ship/sucker) graphics and HMAs.
    g_gfxLogoFighter = LoadGraphic2("warblade_logo_fighter.tga", true, true);
    if (g_gfxLogoFighter == 0) return 0;
    g_gfxLogoFighterShadow = LoadGraphic2("warblade_logo_fightershadow.tga", true, true);
    if (g_gfxLogoFighterShadow == 0) return 0;
    g_gfxMemoryBlocks = LoadGraphic2("memoryblocks.tga", true, true);
    if (g_gfxMemoryBlocks == 0) return 0;
    g_gfxGuard = LoadGraphic2("guard.tga", true, true);
    if (g_gfxGuard == 0) return 0;
    g_gfxGuardMask = LoadGraphic2("guardmask.tga", true, true);
    if (g_gfxGuardMask == 0) return 0;
    g_hmaGuard = LoadHma("guard", (int)ImgWidth(g_gfxGuard), (int)ImgHeight(g_gfxGuard));
    g_guardWidth = (int)ImgWidth(g_gfxGuard);
    g_guardHeight = (int)ImgHeight(g_gfxGuard);
    g_gfxMedals = LoadGraphic2("medaljer.tga", true, true);
    if (g_gfxMedals == 0) return 0;
    g_bgGraphic = LoadGraphic2("meteors.png", true, true);
    if (g_bgGraphic == 0) return 0;
    g_hmaMeteors = LoadHma("meteors", (int)ImgWidth(g_bgGraphic), (int)ImgHeight(g_bgGraphic));
    g_meteorsGfxW = 624;
    g_meteorsGfxH = 717;
    g_gfxMeteorBonuses = LoadGraphic2("meteorbonuses.tga", true, true);
    if (g_gfxMeteorBonuses == 0) return 0;
    g_meteorBonusesGfxW = 384;
    g_meteorBonusesGfxH = 370;

    g_gfxMeteorMeter = LoadGraphic2("meteormeter2.tga", true, true);
    if (g_gfxMeteorMeter == 0) return 0;
    g_gfxMoneyShip = LoadGraphic2("moneyship.tga", true, true);
    if (g_gfxMoneyShip == 0) return 0;
    g_hmaMoneyShip = LoadHma("moneyship", (int)ImgWidth(g_gfxMoneyShip), (int)ImgHeight(g_gfxMoneyShip));
    g_gfxMoneyShipMask = LoadGraphic2("moneyship_mask.tga", true, true);
    if (g_gfxMoneyShipMask == 0) return 0;
    g_moneyShipGfxW = (int)ImgWidth(g_gfxMoneyShip);
    g_moneyShipGfxH = (int)ImgHeight(g_gfxMoneyShip);
    g_gfxMoneySucker = LoadGraphic2("moneysucker2.tga", true, true);
    if (g_gfxMoneySucker == 0) return 0;
    g_gfxMoneySuckerMask = LoadGraphic2("moneysucker2_mask.tga", true, true);
    if (g_gfxMoneySuckerMask == 0) return 0;
    g_hmaMoneySucker = LoadHma("moneysucker2", (int)ImgWidth(g_gfxMoneySucker),
                                (int)ImgHeight(g_gfxMoneySucker));
    g_moneySuckerWidth = (int)ImgWidth(g_gfxMoneySucker);
    g_moneySuckerHeight = (int)ImgHeight(g_gfxMoneySucker);

    g_numbersGfx = LoadGraphic2("numbers.tga", true, true);
    if (g_numbersGfx == 0) return 0;
    g_gfxRocket = LoadGraphic2("rocket.tga", true, true);
    if (g_gfxRocket == 0) return 0;
    g_hmaRocket = LoadHma("rocket", (int)ImgWidth(g_gfxRocket), (int)ImgHeight(g_gfxRocket));
    g_rocketGfxW = (int)ImgWidth(g_gfxRocket);
    g_rocketGfxH = (int)ImgHeight(g_gfxRocket);
    g_gfxShieldNew = LoadGraphic2("shield_new.tga", true, true);
    if (g_gfxShieldNew == 0) return 0;
    ImgSetAlphaMode(g_gfxShieldNew, 4);
    g_gfxLogoSword = LoadGraphic2("warblade_logo_sword.tga", true, true);
    if (g_gfxLogoSword == 0) return 0;
    g_gfxLogoSwordGlow = LoadGraphic2("warblade_logo_sword_glow.tga", true, true);
    if (g_gfxLogoSwordGlow == 0) return 0;
    g_gfxFighterFire2 = LoadGraphic2("figterfire2.tga", true, true);
    if (g_gfxFighterFire2 == 0) return 0;
    g_winGfx = LoadGraphic2("newscreen.png", true, true);

    // Build three 30-step fade-out colour ramps (white, cyan, orange) used for the
    // screen-flash effects: each step subtracts 1/30 from the starting colour.
    r = 1.0f;
    g = 1.0f;
    b = 1.0f;
    dr = 0.033333335f;
    dg = 0.033333335f;
    db = 0.033333335f;
    for (a = 0; a < 30; a++) {
        (&g_colR[0].step0)[a] = r;
        (&g_colG[0].step0)[a] = g;
        (&g_colB[0].step0)[a] = b;
        r -= dr;
        g -= dg;
        b -= db;
        if (r < 0.0) r = 0;
        if (g < 0.0) g = 0;
        if (b < 0.0) b = 0;
    }

    r = 0;
    g = 1.0f;
    b = 1.0f;
    dr = 0;
    dg = 0.033333335f;
    db = 0.033333335f;
    for (c = 0; c < 30; c++) {
        (&g_colR[1].step0)[c] = r;
        (&g_colG[1].step0)[c] = g;
        (&g_colB[1].step0)[c] = b;
        r -= dr;
        g -= dg;
        b -= db;
        if (r < 0.0) r = 0;
        if (g < 0.0) g = 0;
        if (b < 0.0) b = 0;
    }

    r = 1.0f;
    g = 0.5f;
    b = 0;
    dr = 0.033333335f;
    dg = 0.016666668f;
    db = 0;
    for (d = 0; d < 30; d++) {
        (&g_colR[2].step0)[d] = r;
        (&g_colG[2].step0)[d] = g;
        (&g_colB[2].step0)[d] = b;
        r -= dr;
        g -= dg;
        b -= db;
        if (r < 0.0) r = 0;
        if (g < 0.0) g = 0;
        if (b < 0.0) b = 0;
    }
    return 1;
}

// Top-level game startup: opens the window, sets the app icon, loads all game data and
// surfaces, places the current player and starts the title music. Called once at launch.
// Returns 0 (via InitFail) on any failure, 1 on success.
int InitGame()
{
    if (g_skipLogoFlag != 0) {
        g_logoSplashTimer = 1000;
    }
    LogPrint("Init game is entered...\r\n");
    if (InitWindowCfg() == 0) {
        return InitFail("OpenScreen Failed");
    }
    SysSetIcon("warblade.ico");
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    LogPrint("Screens zeroed passed...\r\n");
    if (LoadGameData() == 0) {
        return 0;
    }
    LogPrint("Restore Surfaces passed...\r\n");

    g_save.players[g_curPlayer].placeSlot = 1;
    PlacePlayer(g_save.players[g_curPlayer].placeSlot);
    LogPrint("Init fighter passed...\r\n");

    g_songName = "title";
    g_musicMode = 0;
    g_musicRestartTime = g_time + 1500;
    LogPrint("Set volum passed...\r\n");
    g_lastFrameTick = g_time;
    g_state = STATE_TITLE; // main menu
    ClearPlayers();
    return 1;
}

// Logs `msg` and tears down the window/sound on a fatal init error. Always returns 0
// (so callers can `return InitFail(...)` directly).
int InitFail(const char *msg)
{
    LogPrint(msg);
    SoundShutdown();
    SysDestroyWindow();
    return 0;
}

// Points each ship type's definition at its stats array.
void InitTablePtrs()
{
    g_shipDefs[0] = (ShipDef *)g_shipStats0;
    g_shipDefs[1] = (ShipDef *)g_shipStats1;
    g_shipDefs[2] = (ShipDef *)g_shipStats2;
    g_shipDefs[3] = (ShipDef *)g_shipStats3;
    g_shipDefs[4] = (ShipDef *)g_shipStats4;
    g_shipDefs[5] = (ShipDef *)g_shipStats5;
    g_shipDefs[6] = (ShipDef *)g_shipStats6;
    g_shipDefs[7] = (ShipDef *)g_shipStats7;
    g_shipDefs[8] = (ShipDef *)g_shipStats8;
    g_shipDefs[9] = (ShipDef *)g_shipStats9;
}

#define FREE_NULL(p) if (p) { free(p); p = 0; }

// Frees level, sound, ship and UI graphics resources, stops and restarts audio, and
// logs the exit. Called once on program shutdown.
void Shutdown()
{
    FreeLevelBufs();
    ReleaseAlienGfxCache();
    DeleteSetPro();
    FreeShopBgGfx();
    FreeShopItemGfx();
    FreeSecretPicGfx();
    FreeSecretScreenGfx();
    ResetF893();

    if (g_scratchPoolA) free(g_scratchPoolA);
    if (g_scratchPoolB) free(g_scratchPoolB);
    if (g_scratchPoolC) free(g_scratchPoolC);
    if (g_scratchPoolD) free(g_scratchPoolD);
    if (g_scratchPoolE) free(g_scratchPoolE);
    if (g_levelBufA) free(g_levelBufA);
    if (g_levelBufB) free(g_levelBufB);
    FREE_NULL(g_ship1Hma);
    FREE_NULL(g_ship2Hma);
    FREE_NULL(g_hmaWeaponsBig);

    if (g_alienGfxMem[0]) { free(g_alienGfxMem[0]); g_alienGfxMem[0] = 0; }
    FREE_NULL(g_alienGfxMem[1]);
    FREE_NULL(g_alienGfxMem[2]);
    FREE_NULL(g_alienGfxMem[3]);
    FREE_NULL(g_alienGfxMem[4]);
    FREE_NULL(g_alienGfxMem[5]);
    FREE_NULL(g_shutdownPtrA);
    FREE_NULL(g_shutdownPtrB);

    FREE_NULL(g_hmaBonuses);
    FREE_NULL(g_hmaDiamond);
    FREE_NULL(g_hmaDiamondBig);
    FREE_NULL(g_hmaRocket);
    FREE_NULL(g_hmaMarks);
    FREE_NULL(g_hmaMoneyShip);
    FREE_NULL(g_hmaGuard);
    FREE_NULL(g_hmaLogoBirdFlare);
    FREE_NULL(g_hmaMeteorBonuses);
    FREE_NULL(g_hmaMeteors);
    FREE_NULL(g_hmaWeaponsBig);

    FreeShopBgGfx();
    FreeShopItemGfx();
    FreeSecretPicGfx();
    FreeSecretScreenGfx();
    FreePlaylist();

    AudioStop();
    AudioStart();
    InitFail("\r\n*** EXITING WARBLADE ***\r\n");
}

#undef FREE_NULL
