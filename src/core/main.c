// main.c: Program entry: GameMain (startup, main loop, shutdown), one-time game setup, the
// window message hook, the crash report.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "globals.h"
#include "game.h"
#include "sdlhelp.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <zlib.h>
static char webAccountName[30];
static int webAccountStatus;
static int webGameReady;
static int webLoginPending = -1;
static int webLoginCreate;
static int webAuthQueued;
static int webAuthBusy;
// The legacy login flag also tracks the welcome sound; keep web UI state separate.
static int webLoginVisible;
static int webSignOutQueued;
static int webGuestQueued;
static int webGuestMode;
static char webLoginUsername[33];
static char webLoginPassword[257];
static char webAuthError[193];
void WebSaveSettings(void);

// Count completed game frames, including rendering/pacing; a separate browser
// animation loop would report display refresh even when this game runs slowly.
EM_JS(void, WebPerformanceFrame, (int phase, int target), {
    if (Module.onPerformanceFrame) Module.onPerformanceFrame(phase, target);
});

EM_ASYNC_JS(int, WebAuthenticate, (const char *username, const char *password, int create), {
    try {
        return await Module.authenticateGame(UTF8ToString(username), UTF8ToString(password), !!create) ? 1 : 0;
    } catch (error) {
        Module.authError = error.message || 'Could not connect. Please try again.';
        return 0;
    }
});
EM_JS(void, WebReadIdentity, (char *name, int capacity, char *error, int errorCapacity), {
    stringToUTF8(Module.accountName || '', name, capacity);
    stringToUTF8(Module.authError || 'Could not sign in. Please try again.', error, errorCapacity);
});
EM_JS(int, WebHasRestoredIdentity, (), {
    return typeof Module.accountName === 'string' && Module.accountName.length > 0 ? 1 : 0;
});
EM_ASYNC_JS(void, WebSignOut, (), {
    await Module.signOutGame();
});

EMSCRIPTEN_KEEPALIVE void WebOpenLogin(int create)
{
    if (!webAuthBusy)
        webLoginPending = !!create;
}

EMSCRIPTEN_KEEPALIVE int WebGameReady(void)
{
    return webGameReady;
}

EMSCRIPTEN_KEEPALIVE int WebLoginReady(void)
{
    return webLoginVisible && !webAuthBusy && AnyWindowHasEdit();
}

// Real browser inputs cover the native fields on touch devices. Focus must happen
// directly from the user's tap so mobile browsers can summon their keyboard.
static Window *WebLoginWindow(void)
{
    if (!WebLoginReady()) return NULL;
    for (int w = 0; w < MAX_WINDOWS; w++) {
        Window *window = &g_windows[w];
        if (window->active && window->visible && window->nH == 1)
            return window;
    }
    return NULL;
}

EM_JS(void, WebSyncLoginInputs, (int visible, int x, int y, int width,
                              int focus, int screenW, int screenH,
                              const char *name, const char *password), {
    if (Module.syncLoginInputs) Module.syncLoginInputs(visible ? {
        x, y, width, focus, screenW, screenH,
        name: UTF8ToString(name), password: UTF8ToString(password)
    } : null);
});

static void WebUpdateLoginInputs(void)
{
    Window *window = WebLoginWindow();
    if (!window) {
        WebSyncLoginInputs(0, 0, 0, 0, 0, 0, 0, NULL, NULL);
        return;
    }
    int x = window->x == POS_CENTERED ? (g_screenW - window->w) / 2 : window->x;
    int y = window->y == POS_CENTERED ? (g_screenH - window->h) / 2 : window->y;
    WebSyncLoginInputs(1, x + window->edits[0].x + (int)window->slideX,
                      y + window->edits[0].y, window->w - window->edits[0].x - 24,
                      window->firstH, g_screenW, g_screenH,
                      window->edits[0].buf, window->edits[1].buf);
}

EMSCRIPTEN_KEEPALIVE int WebSetLoginField(int field, const char *text)
{
    Window *window = WebLoginWindow();
    if (!window || field < 0 || field > 1 || !text) return 0;
    size_t length = strlen(text);
    if (length > (size_t)window->edits[field].len) return 0;
    memset(window->edits[field].buf, 0, sizeof(window->edits[field].buf));
    memcpy(window->edits[field].buf, text, length);
    window->edits[field].cursor = (int)length;
    window->firstH = field;
    for (int e = 0; e < 2; e++) window->edits[e].focused = e == field;
    return 1;
}

// Browser text events preserve password case, punctuation and pasted text.
// This synchronous bridge only edits the visible login field; authentication
// remains queued on the game's main loop.
EMSCRIPTEN_KEEPALIVE int WebInsertLoginText(const char *text)
{
    if (!WebLoginReady() || !text)
        return 0;
    for (int w = 0; w < MAX_WINDOWS; w++) {
        Window *window = &g_windows[w];
        if (!window->active || !window->visible || window->nH < 0
            || window->firstH < 0 || window->firstH > window->nH)
            continue;
        int e = window->firstH;
        size_t length = strlen(text);
        int available = window->edits[e].len - window->edits[e].cursor;
        if (length > (size_t)available)
            return 0;
        memcpy(window->edits[e].buf + window->edits[e].cursor, text, length + 1);
        window->edits[e].cursor += (int)length;
        return 1;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE int WebQueueCredentials(const char *username, const char *password, int create)
{
    if (webAuthBusy || webAuthQueued || webAccountStatus == 1
        || !username || !password || strlen(username) > 32 || strlen(password) > 256)
        return 0;
    snprintf(webLoginUsername, sizeof(webLoginUsername), "%s", username);
    snprintf(webLoginPassword, sizeof(webLoginPassword), "%s", password);
    webLoginCreate = !!create;
    webAuthQueued = 1;
    return 1;
}

EMSCRIPTEN_KEEPALIVE int WebSubmitLogin(void)
{
    Window *window = WebLoginWindow();
    return window ? WebQueueCredentials(window->edits[0].buf,
                                        window->edits[1].buf, webLoginCreate) : 0;
}

EMSCRIPTEN_KEEPALIVE int WebPlayGuest(void)
{
    if (webAuthBusy || webAuthQueued || webAccountStatus == 1) return 0;
    webGuestQueued = 1;
    return 1;
}

EM_JS(int, WebBeginGuest, (), {
    return Module.beginGuestGame() ? 1 : 0;
});

EMSCRIPTEN_KEEPALIVE void WebQueueSignOut(void)
{
    if (webAccountStatus == 1)
        webSignOutQueued = 1;
}

static void WebShowLogin(int create)
{
    webLoginCreate = !!create;
    WinCloseAll();
    WinHideAll();
    g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 640, 310, WIN_MODE_SLIDING);
    WinAddText(POS_CENTERED, 20, g_curWin, create ? "CREATE PLAYER" : "PLAYER SIGN IN", 8);
    WinAddText(30, 50, g_curWin, "PLAYER NAME :", 8);
    WinAddEdit(150, 50, g_curWin, 32, 0, 7, 1);
    snprintf(g_windows[g_curWin].edits[0].buf, sizeof(g_windows[g_curWin].edits[0].buf), "%s", webLoginUsername);
    g_windows[g_curWin].edits[0].cursor = (int)strlen(webLoginUsername);
    WinAddText(30, 75, g_curWin, "PASSWORD :", 8);
    WinAddEdit(150, 75, g_curWin, 256, 1, 7, 0);
    WinAddText(30, 108, g_curWin, "PLAYER NAME: 3-32 LETTERS, NUMBERS, DOT, _ OR -", 7);
    WinAddText(30, 124, g_curWin, "PASSWORD: 8-256 CHARACTERS", 7);
    WinAddText(30, 140, g_curWin, "PROGRESS IS SAVED TO YOUR SERVER ACCOUNT", 7);
    if (webAuthError[0]) {
        char line[70];
        snprintf(line, sizeof(line), "%.68s", webAuthError);
        WinAddText(30, 162, g_curWin, line, 4);
        if (strlen(webAuthError) > 68) {
            snprintf(line, sizeof(line), "%.68s", webAuthError + 68);
            WinAddText(30, 178, g_curWin, line, 4);
        }
    }
    WinAddMenuItem(30, 210, g_curWin, 9000, create ? "CREATE PLAYER" : "SIGN IN", 5);
    WinAddMenuItem(340, 210, g_curWin, 9001, create ? "SIGN IN INSTEAD" : "CREATE PLAYER", 5);
    WinAddMenuItem(30, 242, g_curWin, 9002, "PLAY AS GUEST", 5);
    WinAddText(30, 274, g_curWin, "GUEST PROGRESS LASTS UNTIL YOU RELOAD", 7);
    WinSetSelected(g_curWin, 9000);
    g_loginWinOpen = 1;
    webLoginVisible = 1;
    g_clickWin = g_clickItem = -1;
}

static int WebActivateAccount(void)
{
    g_profileIndex = -1;
    g_profileCount = 0;
    LoadSettings();
    ScanProfiles();
    if (g_profileCount > 1) {
        webAccountStatus = -1;
        snprintf(webAuthError, sizeof(webAuthError), "Multiple saved profiles need migration. Progress has been preserved.");
    } else {
        if (!g_profileCount) {
            NextAccountReset();
            ResetAccount();
        } else {
            UnpackAccount(0);
        }
        WebNormalizeAccount();
        PackAccount(0);
        SaveAccount(0);
        g_selProfile = g_cfg.profileSel = 0;
        LoadHiscores();
        LoadBestScore();
        UnpackAccount(0);
        ActivateProfile(0);
        ClearAccount();
        webGuestMode = 0;
        webAccountStatus = 1;
        g_loginWinOpen = 0;
        webLoginVisible = 0;
        webAuthError[0] = 0;
        WinCloseAll();
        WebUpdateLoginInputs();
        g_clickWin = g_clickItem = -1;
        return 1;
    }
    return 0;
}

// Run asynchronous authentication only from the game's own main loop, never
// from a second exported async stack while its frame loop is suspended.
void WebProcessAccountQueue(void)
{
    WebUpdateLoginInputs();
    if (webGuestQueued) {
        webGuestQueued = 0;
        // Returning from sign-in keeps this visit's existing guest progress.
        if (!webGuestMode) {
            if (!WebBeginGuest()) return;
            webGuestMode = 1;
            webAccountStatus = 0;
            snprintf(webAccountName, sizeof(webAccountName), "GUEST");
            memset(webLoginUsername, 0, sizeof(webLoginUsername));
            memset(webLoginPassword, 0, sizeof(webLoginPassword));
            webAuthError[0] = 0;
            g_profileIndex = -1;
            g_profileCount = 0;
            NextAccountReset();
            ResetAccount();
            WebNormalizeAccount();
            PackAccount(0);
            SaveAccount(0);
            g_selProfile = g_cfg.profileSel = 0;
            LoadHiscores();
            LoadBestScore();
            UnpackAccount(0);
            ActivateProfile(0);
            ClearAccount();
        }
        webLoginPending = -1;
        webLoginVisible = 0;
        WinCloseAll();
        WebUpdateLoginInputs();
        g_clickWin = g_clickItem = -1;
        g_gameMode = MODE_SINGLE;
        g_hofMode = g_cfg.difficulty;
        NewGame(true);
        return;
    }
    if (webSignOutQueued) {
        webSignOutQueued = 0;
        WebSaveSettings();
        WebSignOut();
        return;
    }
    if (webLoginPending >= 0) {
        int create = webLoginPending;
        webLoginPending = -1;
        WebShowLogin(create);
    }
    if (!webAuthQueued || webAuthBusy)
        return;
    webAuthQueued = 0;
    webAuthBusy = 1;
    // Clear all visible password copies before yielding to the server.
    for (int w = 0; w < MAX_WINDOWS; w++)
        for (int e = 0; e <= g_windows[w].nH; e++)
            if (g_windows[w].edits[e].masked)
                memset(g_windows[w].edits[e].buf, 0, sizeof(g_windows[w].edits[e].buf));
    WinCloseAll();
    g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 420, 90, WIN_MODE_SLIDING);
    WinAddText(POS_CENTERED, 30, g_curWin, webLoginCreate ? "CREATING PLAYER..." : "SIGNING IN...", 8);
    WebUpdateLoginInputs();
    int ok = WebAuthenticate(webLoginUsername, webLoginPassword, webLoginCreate);
    memset(webLoginPassword, 0, sizeof(webLoginPassword));
    WebReadIdentity(webAccountName, sizeof(webAccountName), webAuthError, sizeof(webAuthError));
    webAuthBusy = 0;
    if (ok && webAccountName[0] && WebActivateAccount())
        return;
    WebShowLogin(webLoginCreate);
}

int WebLoginMode(void)
{
    return webLoginCreate;
}

// Apply the server identity to an already unpacked profile, including old backups.
void WebNormalizeAccount(void)
{
    snprintf(g_acc.name, sizeof(g_acc.name), "%s", webAccountName);
    memset(g_acc.password, 0, sizeof(g_acc.password));
    g_acc.settings.profileSel = 0;
}

EMSCRIPTEN_KEEPALIVE int WebAccountStatus(void)
{
    if (webAccountStatus != 1)
        return webAccountStatus;
    if (g_profileIndex != 0 || g_profileCount != 1 || !g_loggedIn || webLoginVisible)
        return -1;
    // Inspect a private copy; diagnostics must not disturb the game's g_acc scratch.
    Account account;
    uLongf length = sizeof(account);
    if (uncompress((unsigned char *)&account, &length,
                   (const unsigned char *)&g_accBuf[0], sizeof(account)) != Z_OK
        || length != sizeof(account) || strcmp(account.name, webAccountName))
        return -1;
    for (unsigned int i = 0; i < sizeof(account.password); i++)
        if (account.password[i])
            return -1;
    return 1;
}
EMSCRIPTEN_KEEPALIVE int WebIsGuest(void)
{
    return webGuestMode;
}

EMSCRIPTEN_KEEPALIVE int WebCanPlay(void)
{
    return WebAccountStatus() == 1 ||
           (webGuestMode && g_profileIndex == 0 && g_profileCount == 1 && g_loggedIn);
}

#endif


// The program's `main`: the CRT's mainCRTStartup calls it with (argc, argv, envp), which it
// ignores (the exe is a GUI program linked with /ENTRY:mainCRTStartup). The linker maps
// `main` to this name (x86 C names carry a leading underscore, x64 ones don't).
#if defined(_MSC_VER) && defined(_WIN64)
#pragma comment(linker, "/alternatename:main=GameMain")
#elif defined(_MSC_VER)
#pragma comment(linker, "/alternatename:_main=_GameMain")
#endif

// The whole game: one-time startup (sound/graphics tables, level/menu init), the main loop
// (input -> GameFrame -> render -> present), and shutdown stats on exit. Returns 1 normally;
// 0 if an early init step fails.
int GameMain()
{
    int i;
    bool b;

    // NOTE: dead stores kept for the byte match; `i`/`b` are immediately overwritten before
    // first use (likely leftover sizeof()/constant probing from the original source).
    i = 1;
    i = 2;
    i = 1;
    i = 4;
    i = 4;
    i = 8;
    i = 8;
    i = 8;
    i = 4;
    i = 0x68;
    i = 0xA1558;
    i = 0x20;
    i = 0x638;
    b = true;
    i = b;

    // Zero the sound-slot table and the per-difficulty sample cache.
    for (int j = 0; j < MAX_ALIEN_GFX_SLOTS; j++) {
        g_alienGfxSlots[j].src = 0;
        for (i = 0; i < NUM_HAZARD_GFX; i++) {
            g_alienGfxSlots[j].gfx[i] = 0;
            g_alienGfxSlots[j].gfx2[i] = 0;
            g_alienGfxSlots[j].hma[i] = 0;
            g_alienGfxSlots[j].loaded[i] = 0;
            g_alienGfxSlots[j].key[i] = 0;
            g_alienGfxSlots[j].name1[i][0] = 0;
            g_alienGfxSlots[j].name2[i][0] = 0;
            g_alienGfxSlots[j].name3[i][0] = 0;
        }
    }
    for (int k = 0; k < 4; k++) {
        for (int l = 0; l < 150; l++) {
            g_samples[k][l] = 0;
        }
    }

    InitStaticGlobals();
    SysInit();
    int a1 = 1;  // NOTE: a1-a4 are unused after assignment; kept for the byte match.
    int a2 = 1;
    int a3 = 1;
    int a4 = 0;
    g_state = STATE_TITLE;
    g_items[140].alive = 5;
    MakeGameDir();
    LogInit();
    LogPrint("Warblade SR 2.0, Copyright 1999-2009 Edgar M Vigdal\r\n");

    HidePointer();
    LogPrint("Hide mouse curosr is passed...\r\n");
    SeedRand(MouseX() * MouseY() * SysMillis());
    ResetAllObjects();
    InitTablePtrs();
    DeleteSetPro();
    g_viewTransitionFlag = 2;

    // Hook up the per-mode draw/update function pointers.
    g_stateFn = SetViewHud;
    g_drawHudFn = DrawHud1P;
    LoadSettings();
    g_drawBordersFn = DrawBorders;
    if (g_cfg.bulletIntensity == 1) {
        g_drawLevelObjectsFn = DrawLevelObjectsBright;
    } else {
        g_drawLevelObjectsFn = DrawLevelObjectsNormal;
    }
    g_playerUpdateFn = UpdatePlayer;
    g_itemsVsPlayerFn = ItemsVsPlayer;
    g_bulletsVsPlayerFn = BulletsVsPlayer;
    g_grabEnemyFn = ShieldGrabEnemies;
    g_shipHudFn = ShipHud;

    // ---- resource archive, game/sound init, cheat detection, trig tables, scratch pools, starfield ----
    PacAddArchive(SysAppPath("data\\warblade.pac"));
    if (!GameInit()) {
        return 0;
    }
    LogPrint("InitApplication is passed...\r\n");
    if (!InitGame()) {
        return 0;
    }
    SysSetFocusCallback(OnFocusChange);
    InitSound();
    LoadPlaylist();
    g_playlistCount = ((ParsePlaylist() - 1 < 0 ? 0 : ParsePlaylist() - 1) > 9999) ? 9999
                             : (ParsePlaylist() - 1 < 0 ? 0 : ParsePlaylist() - 1);
    InitTrigTables();

    // Scratch memory pools, then random starfield init.
    g_scratchPoolA = malloc(0x1cb98);
    g_scratchPoolB = malloc(0x1b000);
    g_scratchPoolC = malloc(0x1b000);
    g_scratchPoolD = malloc(0x1b000);
    g_scratchPoolE = malloc(0x1b000);
    int rnd = 0;
    for (i = 0; i < NUM_STARS; i++) {
        g_stars[i].y = RandFloat(-550.0f, (float)g_screenH);
        g_stars[i].x = RandFloat(0, (float)g_screenW);
        g_stars[i].z = RandFloat(1.0f, 16.0f);
        g_starZ[i] = RandFloat(g_starZNear, g_starZFar);
        g_starX[i] = RandFloat(g_starXMin, g_starXMax);
        g_starY[i] = RandFloat(g_starYMin, g_starYMax);

        rnd = RandRange(0, 100);  // per-star speed band, weighted toward slower stars
        if (rnd < 50) {
            g_starA[i] = RandFloat(1.0f, 8.0f);
        }
        if (rnd >= 50 && rnd < 70) {
            g_starA[i] = RandFloat(8.0f, 12.0f);
        }
        if (rnd >= 70 && rnd < 90) {
            g_starA[i] = RandFloat(12.0f, 16.0f);
        }
        if (rnd >= 90 && rnd < 98) {
            g_starA[i] = RandFloat(16.0f, 60.0f);
        }
        if (rnd >= 98) {
            g_starA[i] = RandFloat(60.0f, 100.0f);
        }

        if (rnd >= 0 && rnd < 98) {
            g_starGfx[i] = g_gfxStar1;
        }
        if (rnd >= 98) {
            if (RandRange(0, 2) == 0) {
                g_starGfx[i] = g_starSprite;
            } else {
                g_starGfx[i] = g_gfxStar3;
            }
        }
    }

    // Players, per-frame effect tables, and another round of object-array resets.
    for (i = 0; i < NUM_PLAYERS; i++) {
        InitPlayer(i);
    }
    for (i = 0; i < 512; i++) {
        g_perfectColorTable[i] = RandRange(0, 7);
    }
    ClearSlots();
    for (i = 0; i < 100; i++) {
        g_grabZoneWidthTable[i] = i * 0.5 + 4.0;
    }

    for (i = 0; i < MAX_MAP_OBJS; i++) {
        g_mapObjs[i].x = 0;
        g_mapObjs[i].y = 0;
        g_mapObjs[i].active = 0;
    }
    for (i = 0; i < MAX_EXPLOSIONS; i++) {
        g_explosions[i].active = 0;
        g_explosions[i].x = 0;
        g_explosions[i].y = 0;
        g_explosions[i].type = 0;
        g_explosions[i].frame = 0;
        g_explosions[i].delay = 0;
        g_explosions[i].angle = 0;
        g_explosions[i].angleVel = 0;
    }
    for (i = 0; i < MAX_EXPLOSION_PARTICLES; i++) {
        g_explosionParticles[i].active = 0;
    }

    InitFrameRectDefaults(0);
    InitFrameRectDefaults(1);
    InitFrameRectDefaults(2);
    InitFrameRectDefaults(3);
    InitFrameRectDefaults(4);
    InitFrameRectDefaults(5);

    // Bonus-item volume table, scaled from each sprite's pixel area.
    float maxv = 0;
    for (i = 0; i < NUM_METEOR_VARIANTS; i++) {
        g_bonusGfxArea[i] = g_bonusGfxW[i] * g_bonusGfxH[i];
        if (g_bonusGfxArea[i] > (int)maxv) {
            maxv = (float)g_bonusGfxArea[i];
        }
    }
    if (maxv == 0.0) {
        maxv = 1.0f;
    }
    int v;
    float scale = 800.0 / maxv;
    for (i = 0; i < NUM_METEOR_VARIANTS; i++) {
        v = (int)(g_bonusGfxArea[i] * scale);
        if (v > 255) {
            v = 255;
        }
        g_bonusVolume[i] = v;
    }

    // Trig lookup tables and difficulty/joystick defaults.
    float ang = 0;
    for (i = 0; i < 360; i++) {
        g_cosDeg[i] = Sin2((float)(ang * 0.017453292384743690));
        g_sinDeg[i] = Cos2((float)(ang * 0.017453292384743690));
        ang = ang + 1.0;
    }
    BuildRampTables();
    g_gameMode = MODE_SINGLE;
    g_cfg.difficulty = DIFF_NORMAL;
    LogPrint("Setting Variables is passed...\r\n");
    g_joyCount = 0;
    g_joy0 = 0;
    if (JoyEnable(0)) {
        g_joy0 = 1;
        g_joyCount++;
    }
    g_joy1 = 0;
    if (JoyEnable(1)) {
        g_joy1 = 1;
        g_joyCount++;
    }
    LogPrint("Init joysticks is passed...\r\n");

    // First screen clear/flip, world view, and level/splash preload.
    if (g_cfg.bgStars != 0) {
        g_fnPtr = DrawStarsStill;
    } else {
        g_fnPtr = DrawStarsGlow;
    }
    SetViewHud();
    LogPrint("Erase all screens is passed...\r\n");
    g_attractScreen = ATTRACT_HALL_OF_FAME;
    g_idleTimeoutMs = ATTRACT_SCREEN_MS;
    g_lastActivityTime = g_time - 10000;
    g_state = STATE_TITLE;
    ClearPlayers();
    g_titleResetPending = 1;
    DoNothing();
    LogPrint("Calc time span is passed...\r\n");
    ClipCursorOn();
    SysDisableScreenSaver();

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
    CheckTimeTrialAvailable();
    BufferAllLevels();
    LogPrint("Preoading of all level data is passed...\r\n");
#ifndef __EMSCRIPTEN__
    // The close box works from here on: the splashes end early and the main loop never runs.
    if (!SysQuitRequested())
        ShowLogoSplash();
    LogPrint("EMV Software splash is passed...\r\n");
    if (!SysQuitRequested())
        ShowTitleSplash();
    LogPrint("Game splash is passed...\r\n");

#else
    // The web loading view already shows the original title artwork. Avoid a
    // second pair of hidden splashes before the native account screen.
#endif

    // Menu/intro state, then the login/profile prompts shown before the title screen.
    g_mouseDown = 0;
    InitMenu();
    InitStarRotation();
    g_introInit = 1;
    DoNothing();
    g_frameFunc = IntroFrame;
    CLEAR_DRAW_COUNTERS();

    if (g_cfg.profileSel >= 10) {
        g_cfg.profileSel = -1;
    }
    g_loginWinOpen = 0;
    g_loggedIn = 0;
#ifdef __EMSCRIPTEN__
    if (WebHasRestoredIdentity()) {
        WebReadIdentity(webAccountName, sizeof(webAccountName), webAuthError, sizeof(webAuthError));
        if (!WebActivateAccount())
            WebOpenLogin(0);
    } else {
        WebOpenLogin(0);
    }
#else
    // ---- default-account login window (a profile is selected and remembered) ----
    if (g_cfg.profileSel != -1) {
        GetProfileName(g_cfg.profileSel);
        g_selProfile = g_cfg.profileSel;
        WinHideAll();
        g_curWin = WinOpen(POS_CENTERED, 0x19a, 0x1c2, 0x6e, WIN_MODE_SLIDING);  // "use profile / enter password" login window
        WinAddText(0x1e, 0x14, g_curWin, "USE PROFILE : ", 0xc);
        WinAddText(0x96, 0x14, g_curWin, g_logBuf, 0xc);
        WinAddText(0x1e, 0x28, g_curWin, "PASSWORD :", 0xc);
        WinAddEdit(0x78, 0x28, g_curWin, 0xf, 1, 3, 1);
        WinAddMenuItem(0x18, 0x4b, g_curWin, 0xfd, "OK", 5);
        WinSetSelected(g_curWin, 0xfd);
        WinAddMenuItem(0x168, 0x4b, g_curWin, 0x9f6, "CANCEL", 6);
        g_loginWinOpen = 1;
    }
#endif
    LogPrint("Default account? is passed...\r\n");

    // ---- "no valid user profiles found" error window ----
    if (g_noProfilesError && g_profileCount == 0) {
        WinHideAll();
        g_curWin = WinOpen(POS_CENTERED, 0x1c2, 0x1c2, 0x46, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 0x14, g_curWin, "ERROR : NO VALID USER PROFILES FOUND!!", 0xc);
        WinAddText(POS_CENTERED, 0x28, g_curWin, "PRESS ESCAPE TO CONTINUE", 0xc);
    }

    // Hardware presets prompt (hold shift at startup).
    if (KeyDown(K_VK_L_SHIFT) == true || KeyDown(K_VK_R_SHIFT) == true) {
        g_presets = 1;
    }
    if (g_presets) {
        g_curWin = WinOpen(POS_CENTERED, 0xdc, 0xfa, 0xa0, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 0x14, g_curWin, "PRESETS", 5);
        WinAddMenuItem(POS_CENTERED, 0x28, g_curWin, 4000, "    VERY OLD PC   ", 5);
        WinAddMenuItem(POS_CENTERED, 0x3c, g_curWin, 4001, "      OLD  PC     ", 5);
        WinAddMenuItem(POS_CENTERED, 0x50, g_curWin, 4002, "     NORMAL PC    ", 5);
        WinAddMenuItem(POS_CENTERED, 100, g_curWin, 4003, " VERY POWERFUL PC ", 5);
        WinAddMenuItem(POS_CENTERED, 0x7d, g_curWin, 4999, "  CLOSE  ", 6);
        WinSetSelected(g_curWin, 4999);
    }

    // Per-loop state before the main loop.
    g_musicRestartTime = g_time + 1500;
    g_menuIdleTimeout = g_time + MENU_IDLE_MS;
    g_bgIndex = 1;
    ClearParticles();
    g_cfg.fps = 60;
    DoNothing();
    g_saveMagic = 12345;
#ifdef __EMSCRIPTEN__
    webGameReady = 1;
#endif

    // Main loop: input, GameFrame, render/present, repeat until the window quits.
    while (!SysQuitRequested()) {
        if (g_autoplay) {
            g_frameDt = 3.0f;
        } else {
            g_frameDt = g_gameSpeedMul;
        }
        if (MouseLeft()) {
            g_mouseDown = 1;
            g_mouseClick = 0;
        } else {
            g_buttonHitLatch = 0;
            g_gadgetHitLatch = 0;
            g_mouseOverTextItem = 0;
            g_mouseDown = 0;
            g_mouseClick = 1;
            g_mouseClickHandled = 0;
            g_mouseFlag = 0;
        }
        g_rightButton = MouseRight();
        GameFrame();

        // Render the frame: game world, HUD windows, promo/steal overlays, present.
        if (g_skipRender == 0 && g_state != STATE_UNUSED_19) {
            g_frameFunc();
            if (g_showFps) {
                DrawFps();
            }
            FlushBlit(0);
            FlushQuads(0);
            FlushStretchF();
            FlushStretchRot();
            FlushStretchI();
            FlushStretchRot2();
            FlushBlit2(0);
            if (g_promoRingActive) {
                DrawStretch(g_gfxRankPlanets, g_screenW / 2 - 0x80, 0x70, 0x100, 0x100,
                            0, g_promoRingIndex * 128, 0x80, 0x80);
            }
            if (AnyWindowActive()) {
                WinUpdateAll();
            }
            if (g_promoSpecialRank && !AnyWindowActive()) {
                DrawRankPromoBanner();
            }
            FlushBlit(0);
            FlushQuads(0);

            if (g_soundStealCooldown > g_time) {
                ImgSetBlitColor(g_gfxFlare11, RandFloat(0, 1.0f), RandFloat(0, 1.0f),
                                RandFloat(0, 1.0f), 0.7f);
                ImgBlitAlphaRectFx(g_gfxFlare11, 0, 0, ImgWidth(g_gfxFlare11), ImgHeight(g_gfxFlare11),
                                   0x59 - (short)(ImgWidth(g_gfxFlare11) / 2.0),
                                   0x13 - (short)(ImgHeight(g_gfxFlare11) / 2.0),
                                   RandFloat(0, 360.0f), RandFloat(0.6f, 1.2f),
                                   1.0f, false, false, 0, 0);
                ImgSetBlitColor(g_gfxFlare11, 1.0f, 1.0f, 1.0f, 1.0f);
                ImgBlitAlphaRect(g_gfxLogos, 96.0f, 284.0f, 124.0f, 312.0f, 0x4b, 5, 0, 0);
            }

            if (g_state == STATE_PAUSED && g_promoRingActive) {
                DrawWarpRing();
                FlushStretchRot();
            }
            if (g_buttonsOn != 0) {
                g_lastActivityTime = g_time;
                g_idleTimeoutMs = 3000;
                Blit(g_mouseX, g_mouseY, 0, g_gfxLogos, 0, 0x16, 0x28, 0x2a);
                FlushBlit(0);
            }
            FlipBuffer(0);
#ifdef __EMSCRIPTEN__
            WebPerformanceFrame(g_state == STATE_TITLE || g_state == STATE_HISCORE_TABLE ? 0 :
                                g_state == STATE_PAUSED ? 2 : 1, g_cfg.fps);
#endif
        }
        if (g_soundEnabled != 0) {
            AudioUpdate();
        }
        SysProcessEvents();
    }

    // Shutdown: tear down and log the alien gfx/hitmask allocation counters.
    Shutdown();
    sprintf(g_logBuf, "# of alien gfx allocated : %d \n", g_gfxLoaded);
    LogPrint(g_logBuf);
    sprintf(g_logBuf, "# of alien gfx released : %d \n", g_alienGfxFreedA);
    LogPrint(g_logBuf);
    sprintf(g_logBuf, "# of alien hit gfx allocated : %d \n", g_gfx2Loaded);
    LogPrint(g_logBuf);
    sprintf(g_logBuf, "# of alien hit gfx released : %d \n", g_alienGfxFreedB);
    LogPrint(g_logBuf);
    sprintf(g_logBuf, "# of alien hitmasks allocated : %d \n", g_hmaLoaded);
    LogPrint(g_logBuf);
    sprintf(g_logBuf, "# of alien hitmasks released : %d \n", g_alienGfxFreedM);
    LogPrint(g_logBuf);
    LogPrint("\n\n");
    sprintf(g_logBuf, "# of alien gfx released at exit: %d \n", g_freedA);
    LogPrint(g_logBuf);
    sprintf(g_logBuf, "# of alien hit gfx released at exit : %d \n", g_freedB);
    LogPrint(g_logBuf);
    sprintf(g_logBuf, "# of alien hitmasks released at exit: %d \n", g_freedC);
    LogPrint(g_logBuf);
    return 1;
}


// One-time game state setup called from GameMain: loads settings/profiles, decides
// windowed vs fullscreen against the desktop resolution, seeds default difficulty/tuning
// globals, and loads patterns, hiscores and the active profile. Always returns true.
bool GameInit()
{
    LoadSettings();
    g_noProfilesError = 0;
#ifndef __EMSCRIPTEN__
    ScanProfiles();

    if (g_cfg.profileSel != -1)
        g_windowed = GetProfileCfgFlag(g_cfg.profileSel);
#endif
    // Web profiles are scanned and activated together after the browser has
    // restored a server account, or after an explicit native sign-in.
    if (SysDesktopWidth() <= (int)g_screenW)  // desktop too narrow to run windowed
        g_windowed = 0;
    if (SysDesktopHeight() <= (int)g_screenH)  // desktop too short to run windowed
        g_windowed = 0;
    LogPrint("Main window is created...\r\n");
    LogPrint("Window is updated...\r\n");
    BuildSinCos();
    LogPrint("Pixel shader is initiated...\r\n");
    LogPrint("Cursor is killed...\r\n");

    // Level/warp/promo state reset for a fresh session.
    g_curLevelNum = 0;
    g_levelDataLoaded = 1;
    g_loadedLevel = -1;
    g_warpLevelR = -1;
    g_warpLevelL = -1;
    g_marksBonusGiven = 0;
    g_enemyAimAtPlayer = 0;
    g_fastEnemyBullets = 0;

    // Default difficulty/tuning globals.
    g_diffEnemyFireChance = 6;
    g_diffShotFuseBase = 230;
    g_diffShotFuseRange = 240;
    g_hurryUpInterval = 120;
    g_diffShotSpeedMin = 0.0f;
    g_diffShotSpeedMax = 0.0f;
    g_diffTurretTrackChance = 20.0f;
    g_diffBonusDropRoll = 17.0f;
    g_curPlayer = 0;
    g_shopCurPlayer = 0;
    g_promoPlayer = 0;
    g_bonusStagePlayer = 0;
    g_vsTurnPlayer = 0;
    g_defaultObjAlpha = 1.0f;
    g_enemyBulletSpeed = 4.0f;
    g_playerStartSpeed = 12.0f;

    g_diffEnemyTimerMul = 2.2f;
    g_diffEnemyHpBonus = 75;
    g_moneySuckerBaseHp = 500;
    g_diffHurryUpSpeedMax = 3.0f;
    g_eliteHpBonus = 1500;
    g_hurryUpHpBonus = 100;
    g_speedBase = 4.0f;
    g_speedStep = 0.75f;
    g_maxSpeedMul = 16.0f;
    g_speedMax = 10.0f;
    g_bonusDuration = 0;
    g_bonusSpawnRampRate = 0.0006f;
    g_bonusThresholdBase = 30000;
    g_bonusRareChance = 5;
    g_timeMax = 45;
    g_speedMin = 15;
    g_lastEliteSpawnLevel = 0;
    g_fireDelayMin = 200;
    g_enemyFireRateMin = 200;
    g_fireDelayBiasA = 0;
    g_fireDelayBiasB = 0;

    // Combo/score counters.
    g_comboLevel = 0;
    g_comboStep = 0;
    g_hits = 0;
    g_sessionScore = 0;
    g_perfectCount = 0;
    g_killCount = 0;

    WinInit();
    LoadPatterns();
    CountMalfunctionLevels();
    LogPrint("Loading of attack patterns is passed...\r\n");
    LoadHiscores();
    LoadProfile();
    LogPrint("Statistics loaded is passed...\r\n");
    return true;
}

// Writes a crash report to the log: what went wrong (`reason`), the program/game state and
// both players' stats. Not called by anything yet; meant for an assert or fatal-error path.
void LogCrashReport(const char *reason)
{
    char sMsg[1024];
    char szTimeBuffer[100];
    time_t now;

    LogPrint("\r\n");
    LogPrint("\r\n");
    LogPrint("\r\n");
    LogPrint("## CRASH ## v2.0\r\n");
    LogPrint("Version : FULL VERSION\r\n");
    LogPrint("\r\n");
    sprintf(sMsg, "Program State: %d\r\n", g_state);
    LogPrint(sMsg);
    LogPrint("\r\n");
    switch (g_gameMode) {
    case MODE_SINGLE: LogPrint("Game mode : SINGLE PLAYER GAME\r\n"); break;
    case MODE_TWO_PLAYER: LogPrint("Game mode : TWO PLAYER GAME\r\n"); break;
    case MODE_DUAL: LogPrint("Game mode : DUAL PLAYER GAME\r\n"); break;
    case MODE_TEAM: LogPrint("Game mode : TEAM PLAYER GAME\r\n"); break;
    case MODE_ACE_TOURNAMENT: LogPrint("Game mode : ACE_TURNAMENT GAME\r\n"); break;
    case MODE_TIME_TRIAL: LogPrint("Game mode : TIME TRIAL GAME\r\n"); break;
    }
    LogPrint("\r\n");
    if (g_windowed)
        LogPrint("Game screen mode : WINDOWED\r\n");
    else
        LogPrint("Game screen mode : FULLSCREEN\r\n");
    LogPrint("\r\n");

    LogPrint("**** PLAYER ONE ****\r\n");
    sprintf(sMsg, " On level: %d\r\n", g_save.players[0].level);
    LogPrint(sMsg);
    sprintf(sMsg, "    Score: %lld\r\n", (long long)g_save.players[0].score);
    LogPrint(sMsg);
    sprintf(sMsg, "   Weapon: %d\r\n", g_save.players[0].weapon);
    LogPrint(sMsg);
    sprintf(sMsg, "     Cash: %d\r\n", g_save.players[0].money);
    LogPrint(sMsg);
    sprintf(sMsg, "Activ BoS: %d\r\n", g_save.players[0].energy);
    LogPrint(sMsg);
    sprintf(sMsg, "     xpos: %d\r\n", (int)g_save.players[0].x);
    LogPrint(sMsg);
    sprintf(sMsg, "     ypos: %d\r\n", (int)g_save.players[0].y);
    LogPrint(sMsg);
    LogPrint("\r\n");

    LogPrint("**** PLAYER TWO ****\r\n");
    sprintf(sMsg, " On level: %d\r\n", g_save.players[1].level);
    LogPrint(sMsg);
    sprintf(sMsg, "    Score: %lld\r\n", (long long)g_save.players[1].score);
    LogPrint(sMsg);
    sprintf(sMsg, "   Weapon: %d\r\n", g_save.players[1].weapon);
    LogPrint(sMsg);
    sprintf(sMsg, "     Cash: %d\r\n", g_save.players[1].money);
    LogPrint(sMsg);
    sprintf(sMsg, "Activ BoS: %d\r\n", g_save.players[1].energy);
    LogPrint(sMsg);
    sprintf(sMsg, "     xpos: %d\r\n", (int)g_save.players[1].x);
    LogPrint(sMsg);
    sprintf(sMsg, "     ypos: %d\r\n", (int)g_save.players[1].y);
    LogPrint(sMsg);

    if (g_profileIndex != -1)
        LogPrint("Profile is in use...\r\n");
    LogPrint("\r\n");
    LogPrint("\r\n");

    sprintf(sMsg, "Error: %s\r\n", reason);
    LogPrint(sMsg);
    time(&now);
    strftime(szTimeBuffer, sizeof(szTimeBuffer), "%m/%d/%Y %H:%M:%S", localtime(&now));
    sprintf(sMsg, "Error occurred at %s.\r\n", szTimeBuffer);
    LogPrint(sMsg);
}

// Focus-change callback installed on the game window (SysSetFocusCallback). The
// screensaver/monitor-power/menu system commands are swallowed by the facade itself;
// this only pauses/resumes sound and the game loop when the window loses/regains focus.
void OnFocusChange(bool focused)
{
    if (focused) {
        HidePointer();
        SoundResume();
    } else {
        ShowPointer();
        if (g_state != STATE_PAUSED && g_state != STATE_TITLE && g_state != STATE_HISCORE_TABLE &&
            g_state != STATE_POST_ROUND_IDLE && g_state != STATE_ENTER_HISCORE && g_state != STATE_INPUT_CONFIG &&
            g_state != STATE_END_SEQUENCE && g_state != STATE_UNUSED_21)
            PauseGame();
        else
            SoundPause();
    }
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// The browser page calls this when it's hidden or closed (web/index.html): players close the
// tab rather than quit, so save the settings then, as the in-game hotkeys do.
EMSCRIPTEN_KEEPALIVE void WebSaveSettings(void)
{
    MergeSettings(g_profileIndex);
    WriteSettings();
}

// The page replaces the in-memory file with the server's transactionally merged board.
// Avoid reloading during name entry, when the game is still updating its local candidate.
EMSCRIPTEN_KEEPALIVE int WebReloadHiscores(void)
{
    if (g_state == STATE_ENTER_HISCORE)
        return 0;
    LoadHiscores();
    LoadBestScore();
    return 1;
}
#endif

#ifndef _MSC_VER
// Other compilers have no /alternatename: a plain main.
int main(void)
{
    return GameMain();
}
#endif
