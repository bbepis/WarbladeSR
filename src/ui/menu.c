// menu.c: The menus: building the menu entries, buttons, MenuUpdate/MenuHandler, confirmation
// dialogs.
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "game.h"


// Popup window sizes and id bases used only in this file. The menu ids themselves (MenuId)
// are in constants.h.
enum {
    OK_POPUP_W             = 300,    // one-button "OK" confirmation popups (OPEN_OK_POPUP)
    OK_POPUP_H             = 70,
    LOGIN_WIN_W            = 450,    // "ENTER PASSWORD" / "NEW PROFILE" popups
    LOGIN_WIN_H            = 110,
    PROFILE_LIST_WIN_W     = 450,    // "USER PROFILES" list popup
    PROFILE_LIST_WIN_H     = 260,
    PASSWORD_LEN           = 15,     // matches Account::password[15]
    PROFILE_USE_ID_BASE    = 30,     // profile list "USE PROFILE" button id = slot + this
    PROFILE_VIEW_ID_BASE   = 3000,   // profile list "VIEW" button id = slot + this
    TIME_TRIAL_DURATION_MS = 181000,
    GOODBYE_WAIT_MS        = 2500,   // how long the goodbye jingle plays before quitting
};

// Copies a file, overwriting `to` (the original used Win32 CopyFileA with bFailIfExists = 0).
static bool CopyProfileFile(const char *from, const char *to)
{
    FILE *in = fopen(SysPath(from), "rb");
    if (!in)
        return false;
    FILE *out = fopen(SysPath(to), "wb");
    if (!out) {
        fclose(in);
        return false;
    }
    char buf[4096];
    size_t n;
    bool ok = true;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = false;
            break;
        }
    }
    if (ferror(in))
        ok = false;
    fclose(in);
    fclose(out);
    return ok;
}

// Opens the "quit current game?" confirmation window.
void QuitGameDialog()
{
#ifdef __EMSCRIPTEN__
    int h = 120;    // no QUIT TO WINDOWS in the browser
#else
    int h = 140;
#endif
    int w = 200;
    g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
    g_quitGameWin = g_curWin;
    WinAddText(POS_CENTERED, 20, g_curWin, "QUIT GAME?", 4);
    WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_QUIT_GAME_CONFIRM, "QUIT CURRENT GAME", 3);
    WinAddMenuItem(POS_CENTERED, 60, g_curWin, MENUID_RETIRE_CONFIRM, "RETIRE FROM  GAME", 3);
#ifdef __EMSCRIPTEN__
    WinAddMenuItem(POS_CENTERED, 80, g_curWin, MENUID_QUIT_GAME_CONTINUE, "  CONTINUE GAME  ", 3);
#else
    WinAddMenuItem(POS_CENTERED, 80, g_curWin, MENUID_QUIT_GAME_TO_WINDOWS, " QUIT TO WINDOWS ", 3);
    WinAddMenuItem(POS_CENTERED, 100, g_curWin, MENUID_QUIT_GAME_CONTINUE, "  CONTINUE GAME  ", 3);
#endif
    WinSetSelected(g_curWin, MENUID_QUIT_GAME_CONFIRM);
    WinCheckMenuItem(g_curWin, MENUID_QUIT_GAME_CONFIRM);
}

// Opens the "quit to Windows?" confirmation window; the "continue" option's wording
// differs while a game is already in progress (not STATE_TITLE).
void QuitToWindowsDialog()
{
    int h = 100;
    int w = 250;
    g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
    g_quitWinWin = g_curWin;
    WinAddText(POS_CENTERED, 20, g_curWin, "QUIT TO WINDOWS?", 4);
    WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_QUIT_TO_WINDOWS_YES, " YES! QUIT TO WINDOWS ", 3);
    if (g_state != STATE_TITLE)
        WinAddMenuItem(POS_CENTERED, 60, g_curWin, MENUID_QUIT_TO_WINDOWS_NO, "  NO!! CONTINUE GAME  ", 3);
    else
        WinAddMenuItem(POS_CENTERED, 60, g_curWin, MENUID_QUIT_TO_WINDOWS_NO, "         NO!!         ", 3);
    WinSetSelected(g_curWin, MENUID_QUIT_TO_WINDOWS_YES);
    WinCheckMenuItem(g_curWin, MENUID_QUIT_TO_WINDOWS_YES);
}

// Activate an already unpacked account after authentication.
void ActivateProfile(int slot)
{
    g_loggedIn = 1;
    g_profileIndex = slot;

    if (g_acc.settings.version == g_version) {
        g_hiscore = g_cfg.best;
        int savedC8 = g_cfg.alienBuffer;
        g_cfg = g_acc.settings;
        ApplyFrameSettings();

        if (g_joyCount == 0 && (g_cfg.device0 == DEVICE_JOYSTICK1 || g_cfg.device0 == DEVICE_JOYSTICK2)) {
            g_cfg.device0 = DEVICE_KEYBOARD;
            g_cfg.playerKeys[1][0] = 0;
            g_cfg.playerKeys[2][0] = 3;
            g_cfg.playerKeys[3][0] = 1;
            g_cfg.playerKeys[4][0] = 2;
            g_cfg.playerKeys[0][0] = 9;
            g_cfg.playerKeys[5][0] = 0x4d;
            g_cfg.playerKeys[0][0] = 4;
            g_cfg.playerKeys[5][0] = 0x32;
            g_cfg.menuKeys[0][0] = 0x28;
            g_cfg.menuKeys[1][0] = 0x17;
            g_cfg.playerKeys[6][0] = 0x30;
            g_cfg.joy[0][0] = 1;
            g_cfg.joy[1][0] = 2;
            g_cfg.joy[2][0] = 4;
            g_cfg.menuKeys[2][0] = 8;
            g_cfg.menuKeys[3][0] = 0x10;
        }
        FixDuplicateKeys(0);

        if (g_keysChanged != 0) {
            g_cfg.playerKeys[1][0] = 0;
            g_cfg.playerKeys[2][0] = 3;
            g_cfg.playerKeys[3][0] = 1;
            g_cfg.playerKeys[4][0] = 2;
            g_cfg.playerKeys[0][0] = 9;
            g_cfg.playerKeys[5][0] = 0x4d;
            g_cfg.playerKeys[0][0] = 4;
            g_cfg.playerKeys[5][0] = 0x32;
            g_cfg.menuKeys[0][0] = 0x28;
            g_cfg.menuKeys[1][0] = 0x17;
            g_cfg.playerKeys[6][0] = 0x30;
            g_cfg.joy[0][0] = 1;
            g_cfg.joy[1][0] = 2;
            g_cfg.joy[2][0] = 4;
            g_cfg.menuKeys[2][0] = 8;
            g_cfg.menuKeys[3][0] = 0x10;
        }
        FixDuplicateKeys(1);

        if (g_keysChanged != 0) {
            g_cfg.playerKeys[1][1] = 0x41;
            g_cfg.playerKeys[2][1] = 0x43;
            g_cfg.playerKeys[3][1] = 0x45;
            g_cfg.playerKeys[4][1] = 0x3f;
            g_cfg.playerKeys[0][1] = 8;
            g_cfg.playerKeys[5][1] = 6;
            g_cfg.playerKeys[0][1] = 0x26;
            g_cfg.playerKeys[5][1] = 0x25;
            g_cfg.menuKeys[0][1] = 0x28;
            g_cfg.menuKeys[1][1] = 0x17;
            g_cfg.playerKeys[6][1] = 7;
            g_cfg.joy[0][1] = 1;
            g_cfg.joy[1][1] = 2;
            g_cfg.joy[2][1] = 4;
            g_cfg.menuKeys[2][1] = 8;
            g_cfg.menuKeys[3][1] = 0x10;
        }

        if (g_hiscore > g_cfg.best)
            g_cfg.best = g_hiscore;
        if (g_cfg.best < 0)
            g_cfg.best = 0;
        g_cfg.alienBuffer = savedC8;
    }
    WriteSettings();
    if (g_cfg.difficulty == DIFF_EASY && g_acc.easy == 0) {
        g_cfg.difficulty = DIFF_NORMAL;
        g_hofMode = HOF_NORMAL;
        g_cfg.fps = FPS_NORMAL;
    }

    if (g_cfg.difficulty != DIFF_EASY && g_acc.easy != 0) {
        g_cfg.difficulty = DIFF_EASY;
        g_hofMode = HOF_EASY;
        g_cfg.fps = FPS_EASY;
    }
    if (g_cfg.alienBuffer < 5)
        g_cfg.alienBuffer = 5;
    if (g_cfg.fps < 50 || g_cfg.fps > 90)
        g_cfg.fps = FPS_HARD;
    if (g_cfg.collisionDetail != COLLISION_SIMPLE && g_cfg.collisionDetail != COLLISION_NORMAL)
        g_cfg.collisionDetail = COLLISION_NORMAL;

    if (g_cfg.sparks < 10.0 || g_cfg.sparks > 150.0)
        g_cfg.sparks = 50.0f;
    g_maxSparks = ((int)g_cfg.sparks >> 1 < 5) ? 5 : (int)g_cfg.sparks >> 1;
    if (g_cfg.numStars < 50.0 || g_cfg.numStars > 3000.0)
        g_cfg.numStars = 500.0f;
    if (g_cfg.difficulty < DIFF_EASY || g_cfg.difficulty > DIFF_ACE)
        g_cfg.difficulty = DIFF_NORMAL;
    if (g_cfg.bgTint < BG_BRIGHTNESS_MIN || g_cfg.bgTint > BG_BRIGHTNESS_MAX)
        g_cfg.bgTint = BG_BRIGHTNESS_DEFAULT;
    if (g_cfg.bgEnabled != 0 && g_cfg.bgEnabled != 1)
        g_cfg.bgEnabled = 0;

    if (g_cfg.bgStars != 0)
        g_fnPtr = DrawStarsPlayer;
    else
        g_fnPtr = DrawStarsGlow;
    SetSfxVolume(g_cfg.sfxVol);
    SetMusicVolTable(g_cfg.musicVol);

    if (g_cfg.sfxOn != 0) {
        int track;
        int tries = 0;
        bool done;
        if (g_profileIndex != -1)
            track = GetProfileVoiceIndex(g_profileIndex);
        else
            track = g_cfg.voice;
        if (track < 1 || track > 99)
            track = 1;
        done = false;
        do {
            if (VoiceExists(track)) {

                if (g_profileIndex != -1)
                    SetProfileVoiceIndex(g_profileIndex, track);
                else
                    g_cfg.voice = track;
                LoadVoices();
                done = true;
            } else {
                track++;
                if (track > 99) {
                    track = 1;
                    tries++;

                    if (tries > 2)
                        done = true;
                }
            }
        } while (!done);
    }
    g_loginWinOpen = 0;
    SaveSetPro();
    LoadProfile();
    g_musicRestartTime = g_time + 1500;
}

// Opens the "DO YOU WANT TO CREATE A NEW PLAYER PROFILE?" confirmation window.
void OpenCreateProfileWin()
{
#ifndef __EMSCRIPTEN__
    g_curWin = WinOpen(POS_CENTERED, 225, 600, 150, WIN_MODE_SLIDING);
    WinAddText(POS_CENTERED, 20, g_curWin, "CREATE A NEW PROFILE", 4);
    WinAddText(POS_CENTERED, 35, g_curWin, "DO YOU WANT TO CREATE A NEW PLAYER PROFILE ?", 3);
    WinAddText(POS_CENTERED, 45, g_curWin, "WITH THE PROFILE SYSTEM YOU CAN RECORD YOUR PROGRESS", 3);
    WinAddText(POS_CENTERED, 55, g_curWin, "AND OPEN UP LOCKS AND NEW FEATURES OF THE GAME", 3);
    WinAddText(POS_CENTERED, 70, g_curWin,
               "NB!! REMEMBER TO OPEN AND USE THE PROFILE AFTER YOU HAVE CREATED IT !", 2);
    WinAddText(POS_CENTERED, 80, g_curWin,
               "     PLEASE READ THE MANUAL FOR MORE INFO ON THE PROFILE SYSTEM !    ", 2);
    WinAddMenuItem(30, 115, g_curWin, MENUID_DIALOG_NO, "  NO  ", 3);
    WinAddMenuItem(500, 115, g_curWin, MENUID_CREATE_PROFILE_YES, "  YES  ", 2);
    WinSetSelected(g_curWin, MENUID_CREATE_PROFILE_YES);
#endif
}

// Registers a top-level or sub-menu text label in g_menuEntries (a fixed MAX_MENU_ENTRIES-entry pool).
// `x == -1` centres it on screen; `parent == -1` marks it as a visible top-level entry and
// updates g_last*, otherwise it updates g_last2* for the next sibling in the same submenu.
// `page` is the menu page, `unused` is never read, `style` the text style.
void AddMenuText(int x, int y, const char *text, int page, int id, int parent, int unused, int style)
{
    int len;
    int i;
    int width;
    int j;

    len = 0;
    for (i = 0; i < 50; i++) {
        if (text[i] == 0) break;
        len++;
    }
    width = len * 8 + 16;

    // Find a free entry and fill it in.
    for (j = 0; j < MAX_MENU_ENTRIES; j++) {
        if (g_menuEntries[j].active == 0) {
            g_menuEntries[j].active = 1;
            g_menuEntries[j].hover = 0;
            if (x != -1) {
                g_menuEntries[j].x = x;
            } else {
                g_menuEntries[j].x = (g_screenW >> 1) - (width >> 1);
            }
            g_menuEntries[j].y = y;
            g_menuEntries[j].group = id;
            g_menuEntries[j].h = 15;
            g_menuEntries[j].w = width;
            g_menuEntries[j].segs = len;
            g_menuEntries[j].parent = parent;
            g_menuEntries[j].page = page;
            g_menuEntries[j].style = style;
            g_menuEntries[j].width = 0;
            if (parent == -1) {
                g_menuEntries[j].visible = 1;
            }

            // The original copied 50 bytes whatever the string's length (reading past the end
            // of short literals); the bytes after the terminator are now zeros.
            strncpy(g_menuEntries[j].text, text, 50);

            // Track the last placed entry's bounds for the next sibling to lay out against.
            if (parent == -1) {
                g_lastLeft = g_menuEntries[j].x;
                g_lastRight = g_menuEntries[j].x + g_menuEntries[j].w;
                g_lastTop = y;
                g_lastBottom = y + 15;
            } else {
                g_last2Left = g_menuEntries[j].x;
                g_last2Right = g_menuEntries[j].x + g_menuEntries[j].w;
                g_last2Top = y;
                g_last2Bottom = y + 15;
            }
            g_nextId = g_menuEntries[j].group + 1;
            break;
        }
    }
}

// Registers a clickable menu item (button/toggle) in g_menuEntries, same pool and
// parent/g_last* conventions as AddMenuText(), but with a fixed row height of 11 and an
// explicit `width`.
void AddMenuItem(int x, int y, const char *text, int page, int id, int parent, int unused, int style, int width)
{
    int len;
    int i;
    int j;

    len = 0;
    for (i = 0; i < 50; i++) {
        if (text[i] == 0) break;
        len++;
    }
    // Find a free entry and fill it in.
    for (j = 0; j < MAX_MENU_ENTRIES; j++) {
        if (g_menuEntries[j].active == 0) {
            g_menuEntries[j].active = 1;
            g_menuEntries[j].hover = 0;
            g_menuEntries[j].x = x;
            g_menuEntries[j].y = y;
            g_menuEntries[j].group = id;
            g_menuEntries[j].h = 11;
            g_menuEntries[j].w = width;
            g_menuEntries[j].segs = len;
            g_menuEntries[j].parent = parent;
            g_menuEntries[j].page = page;
            g_menuEntries[j].style = style;
            g_menuEntries[j].width = width;
            if (parent == -1) {
                g_menuEntries[j].visible = 1;
            }

            // The original copied 50 bytes whatever the string's length (reading past the end
            // of short literals); the bytes after the terminator are now zeros.
            strncpy(g_menuEntries[j].text, text, 50);

            // Track the last placed entry's bounds for the next sibling to lay out against.
            if (parent == -1) {
                g_lastLeft = g_menuEntries[j].x;
                g_lastRight = g_menuEntries[j].x + g_menuEntries[j].w;
                g_lastTop = y;
                g_lastBottom = y + g_menuEntries[j].h;
            } else {
                g_last2Left = g_menuEntries[j].x;
                g_last2Right = g_menuEntries[j].x + g_menuEntries[j].w;
                g_last2Top = y;
                g_last2Bottom = y + g_menuEntries[j].h;
            }
            g_nextId = g_menuEntries[j].group + 1;
            break;
        }
    }
}

// Clears and rebuilds the entire main-menu entry list (top-level texts and their
// submenus, plus the settings-page items) by calling AddMenuText()/AddMenuItem() for
// each label in layout order. Called once at startup.
void InitMenu()
{
    int i;
    int y;
    int x;

    for (i = 0; i < MAX_MENU_ENTRIES; i++) {
        g_menuEntries[i].active = 0;
    }

    // Top-level: START (+ its 1/2 player, duel, time trial and demo submenu).
    AddMenuText(0, g_screenH - 18, "START", 0, MENUID_START, -1, 0, 1);
    y = g_nextId - 1;
    AddMenuText(g_lastLeft, g_lastTop - 15, "START 1 PLAYER GAME", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "START 2 PLAYER GAME", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "START 2 PLAYER GAME DUEL", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "                        ", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "START TIME TRIAL", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "DEMO GAME", 0, g_nextId, y, 0, 1);

    // Top-level: ABOUT, STORY, SETTINGS (+ its INPUT submenu).
    AddMenuText(g_lastRight + 2, g_lastTop, "ABOUT", 0, MENUID_ABOUT, -1, 0, 1);
    AddMenuText(g_lastRight + 2, g_lastTop, "STORY", 0, MENUID_STORY, -1, 0, 1);
    AddMenuText(g_lastRight + 2, g_lastTop, "SETTINGS", 0, MENUID_SETTINGS, -1, 0, 1);
    y = g_nextId - 1;
    AddMenuText(g_lastLeft, g_lastTop - 15, "INPUT", 0, g_nextId, y, 0, 1);

    // Top-level: BONUSES, HISCORE (+ its difficulty submenu).
    AddMenuText(g_lastRight + 2, g_lastTop, "BONUSES", 0, MENUID_BONUSES, -1, 0, 1);
    AddMenuText(g_lastRight + 2, g_lastTop, "HISCORE", 0, MENUID_HISCORE, -1, 0, 1);
    y = g_nextId - 1;
    AddMenuText(g_lastLeft, g_lastTop - 15, "EASY       ", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "NORMAL     ", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "HARD       ", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "ACE        ", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "METEORSTORM", 0, g_nextId, y, 0, 1);
    AddMenuText(g_lastLeft, g_last2Top - 15, "TIME TRIAL ", 0, g_nextId, y, 0, 1);

    // Top-level: HELP, F.A.Q., USER PROFILES, USER MANUAL.
    AddMenuText(g_lastRight + 2, g_lastTop, "HELP", 0, MENUID_HELP, -1, 0, 1);
    AddMenuText(g_lastRight + 2, g_lastTop, "F.A.Q.", 0, MENUID_FAQ, -1, 0, 1);
    AddMenuText(g_lastRight + 2, g_lastTop,
#ifdef __EMSCRIPTEN__
                "MY PROFILE",
#else
                "USER PROFILES",
#endif
                0, MENUID_USER_PROFILES, -1, 0, 1);
    AddMenuText(g_lastRight + 2, g_lastTop, "USER MANUAL", 0, MENUID_USER_MANUAL, -1, 0, 1);

    // Top-level: QUIT (+ its YES confirm submenu). A browser tab is closed, not quit: the web
    // build leaves it out, numbering the items after it as if it were there.
#ifdef __EMSCRIPTEN__
    g_nextId = MENUID_QUIT_YES + 1;
#else
    AddMenuText(g_screenW - 50, g_screenH - 18, "QUIT", 0, g_nextId, -1, 0, 3);
    y = g_nextId - 1;
    AddMenuText(g_lastLeft, g_lastTop - 15, "YES ", 0, MENUID_QUIT_YES, y, 0, 3);
#endif

    // Settings-page items (arrows/toggles), laid out top to bottom at a fixed column.
    x = 0x26c;
    AddMenuItem(x, 127, "<", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom + 11, "<", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "O", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, "<", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, "M", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(x, g_lastBottom, "O", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, "<", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, "M", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(x, g_lastBottom, "O", 1, MENUID_MUSICVOL_MUTE, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, "<", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 40);
    AddMenuItem(g_lastRight, g_lastTop, "M", 1, g_nextId, -1, 0, 2, 40);

    AddMenuItem(x, g_lastBottom, "<", 1, MENUID_SFX_TOGGLE_PREV, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, MENUID_SFX_TOGGLE_NEXT, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "NEXT VOICE PACK", 1, MENUID_VOICE_PACK_NEXT, -1, 0, 2, 160);
    AddMenuItem(x, g_lastBottom, "<", 1, MENUID_BORDER_MODE_PREV, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, MENUID_BORDER_MODE_NEXT, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, MENUID_BG_ENABLED_PREV, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    g_nextId += 6;  // 121-124 were never used; 125-126 were the colour-depth arrows

    AddMenuItem(x, g_lastBottom, "<", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, MENUID_SPARKS_DOWN, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, MENUID_SPARKS_UP, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, MENUID_BULLET_INTENSITY_PREV, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, g_nextId, -1, 0, 2, 80);
    AddMenuItem(x, g_lastBottom, "<", 1, MENUID_ALIEN_BUFFER_DOWN, -1, 0, 2, 80);
    AddMenuItem(g_lastRight, g_lastTop, ">", 1, MENUID_ALIEN_BUFFER_UP, -1, 0, 2, 80);

    AddMenuItem(x, g_lastBottom, "JUKEBOX", 1, MENUID_JUKEBOX, -1, 0, 2, 160);
    AddMenuItem(x, g_lastBottom + 11, "CONFIG", 1, MENUID_CONFIG, -1, 0, 2, 160);

    // Renderer (was PTK's "USE OPENGL" / "USE DIRECT X"), vsync, interpolation: the
    // settings-page lines beside them show the values.
    AddMenuItem(x, g_lastBottom, "CYCLE RENDERER", 1, MENUID_TOGGLE_RENDERER, -1, 0, 2, 160);
    AddMenuItem(x, g_lastBottom, "TOGGLE VSYNC", 1, MENUID_TOGGLE_VSYNC, -1, 0, 2, 160);
    AddMenuItem(x, g_lastBottom, "CYCLE INTERPOLATION", 1, MENUID_TOGGLE_INTERPOLATION, -1, 0, 2, 160);

    if (!g_cfg.shuffle) {
        AddMenuItem(x, g_lastBottom, "USE RANDOM MODE", 1, MENUID_TOGGLE_SHUFFLE, -1, 0, 2, 160);
    }
    else {
        AddMenuItem(x, g_lastBottom, "USE SEQ MODE", 1, MENUID_TOGGLE_SHUFFLE, -1, 0, 2, 160);
    }
    AddMenuItem(x, g_lastBottom,
#ifdef __EMSCRIPTEN__
                "MY PROFILE",
#else
                "USER PROFILES",
#endif
                1, MENUID_TOGGLE_PROFILE_LIST, -1, 0, 2, 160);
    AddMenuItem(x, g_lastBottom + 11, "TOGGLE INPUT", 1, MENUID_TOGGLE_INPUT_SWAP, -1, 0, 2, 160);
    AddMenuItem(x, g_lastBottom, "TOGGLE SCREENMODE", 1, MENUID_TOGGLE_WINDOWED, -1, 0, 2, 160);
}

// Hides every menu entry belonging to page owner.
void HidePageButtons(int owner)
{
    int i;
    for (i = 0; i < MAX_MENU_ENTRIES; i++) {
        if (g_menuEntries[i].page == owner)
            g_menuEntries[i].visible = 0;
    }
}

// Draws all active menu-entry buttons on page: makes top-level entries visible, tracks
// mouse hover (revealing a hovered group's children), and renders each visible entry's
// background segments and label. Hides child entries again if nothing on the page is
// hovered.
void DrawButtons(int page)
{
    int unused = 0;
    int t;
    int k;
    int v;
    int tf;
    int h;
    int yo;
    int ty;
    int hit;
    int i;
    int j;
    int m;
    int a;
    int b;

    if (!g_buttonsOn)
        return;

    hit = 0;
    for (i = 0; i < MAX_MENU_ENTRIES; i++) {
        if (g_menuEntries[i].page == page && g_menuEntries[i].parent == -1)
            g_menuEntries[i].visible = 1;
    }

    for (j = 0; j < MAX_MENU_ENTRIES; j++) {
        if (g_menuEntries[j].active && g_menuEntries[j].page == page) {
            if (g_mouseX >= g_menuEntries[j].x &&
                g_mouseX < g_menuEntries[j].x + g_menuEntries[j].w &&
                g_mouseY >= g_menuEntries[j].y &&
                g_mouseY < g_menuEntries[j].y + g_menuEntries[j].h &&
                g_menuEntries[j].visible && !AnyWindowActive()) {
                hit = 1;
                g_menuEntries[j].hover = 1;
                if (g_menuEntries[j].parent == -1) {
                    for (k = 0; k < MAX_MENU_ENTRIES; k++) {
                        if (g_menuEntries[k].parent != -1) {
                            if (g_menuEntries[k].parent == g_menuEntries[j].group && g_menuEntries[k].active)
                                g_menuEntries[k].visible = 1;
                            else
                                g_menuEntries[k].visible = 0;
                        }
                    }
                }
            } else {
                g_menuEntries[j].hover = 0;
            }

            if (g_menuEntries[j].style == 1) {
                v = 0;
                tf = 4;
                if (g_menuEntries[j].hover)
                    t = 0x10d;
                else
                    t = 0xfe;
            }
            if (g_menuEntries[j].style == 2) {
                v = 0x18;
                tf = 3;
                if (g_menuEntries[j].hover)
                    t = 0x10d;
                else
                    t = 0xfe;
            }
            if (g_menuEntries[j].style == 3) {
                v = 0x30;
                tf = 2;
                if (g_menuEntries[j].hover)
                    t = 0x10d;
                else
                    t = 0xfe;
            }

            yo = 0;
            h = 0xf;
            ty = 4;
            if (g_menuEntries[j].h < 0xf) {
                yo = 2;
                h = 0xb;
                ty = 2;
            }

            if (g_menuEntries[j].visible) {
                if (g_menuEntries[j].width) {
                    Blit2(g_menuEntries[j].x, g_menuEntries[j].y, 0, g_gfxLogos,
                          v, t + yo, 8, h);
                    QueueQuad(0, g_menuEntries[j].x + 8.0, (float)g_menuEntries[j].y,
                                     g_menuEntries[j].width - 16.0, (float)h, g_gfxLogos,
                                     v + 8.0, (float)t + yo, 8.0f, (float)h);
                    Blit2(g_menuEntries[j].x + g_menuEntries[j].width - 8, g_menuEntries[j].y,
                          0, g_gfxLogos, v + 16, t + yo, 8, h);
                    DrawTinyText2(g_menuEntries[j].text,
                                    g_menuEntries[j].x + g_menuEntries[j].width / 2
                                        - g_menuEntries[j].segs * 8 / 2,
                                    g_menuEntries[j].y + ty, tf);

                } else {
                    Blit2(g_menuEntries[j].x, g_menuEntries[j].y, 0, g_gfxLogos,
                          v, t + yo, 8, h);
                    for (m = 0; m < g_menuEntries[j].segs; m++)
                        Blit2(g_menuEntries[j].x + m * 8 + 8, g_menuEntries[j].y,
                              0, g_gfxLogos, v + 8, t + yo, 8, h);
                    Blit2(g_menuEntries[j].x + 8 + g_menuEntries[j].segs * 8, g_menuEntries[j].y,
                          0, g_gfxLogos, v + 16, t + yo, 8, h);
                    DrawTinyText2(g_menuEntries[j].text, g_menuEntries[j].x + 8,
                                  g_menuEntries[j].y + ty, tf);
                }
            }
        }
    }

    if (!hit) {
        for (a = 0; a < MAX_MENU_ENTRIES; a++) {
            for (b = 0; b < MAX_MENU_ENTRIES; b++) {
                if (g_menuEntries[b].parent == g_menuEntries[a].group && g_menuEntries[a].page == page)
                    g_menuEntries[b].visible = 0;
            }
        }
    }
}

// Opens a one-button "OK" confirmation popup with the given message (the profile reset
// confirmations below all use exactly this shape).
#define OPEN_OK_POPUP(msg)                                                          \
    g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, OK_POPUP_W, OK_POPUP_H, WIN_MODE_SLIDING);      \
    WinAddText(POS_CENTERED, 20, g_curWin, msg, 7);                                        \
    WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_CANCEL_ALL, " OK ", 5);              \
    WinSetSelected(g_curWin, MENUID_CANCEL_ALL)

// Per-frame update of the popup window system (g_windows[]): hit-tests the mouse against
// each window's fields/buttons/gadgets/links, tracks drag/press/click state, then runs the
// dispatcher that reacts to whichever button id was clicked (profile management, options
// presets, hiscore submission, quit dialogs, ...) and handles keyboard input for any window
// that has a focused edit box. `moved` is true when the mouse moved since the last call.
void MenuUpdate(bool moved)
{
#ifdef __EMSCRIPTEN__
    WebProcessAccountQueue();
#endif
    int i;
    int j;
    bool overItem = false;

    // ---- hover: hit-test the mouse against fields/text-links/gadgets/buttons of the topmost window ----
    if (g_buttonsOn != 0) {
        for (i = 0; i < MAX_WINDOWS; i++) {
            for (j = 0; j < g_windows[i].nF + 1; j++) {
                g_windows[i].menuItems[j].state = 0;
            }
        }
        for (i = MAX_WINDOWS - 1; i != -1; i--) {
            if (g_windows[i].active != 0
                && g_mouseX > g_windows[i].x
                && g_mouseX < g_windows[i].x + g_windows[i].w
                && g_mouseY > g_windows[i].y
                && g_mouseY < g_windows[i].y + g_windows[i].h) {

                for (j = 0; j < g_windows[i].nF + 1; j++) {
                    if (g_mouseX >= g_windows[i].x + g_windows[i].menuItems[j].x
                        && g_mouseX < g_windows[i].x + g_windows[i].menuItems[j].x
                            + g_windows[i].menuItems[j].w
                        && g_mouseY >= g_windows[i].y + g_windows[i].menuItems[j].y
                        && g_mouseY < g_windows[i].y + g_windows[i].menuItems[j].y
                            + g_windows[i].menuItems[j].h) {
                        g_windows[i].menuItems[j].state = 1;
                        overItem = true;
                        break;
                    }
                }
                for (j = 0; j < g_windows[i].nD + 1; j++) {
                    g_windows[i].links[j].hilite = 0;
                }

                for (j = 0; j < g_windows[i].nD + 1; j++) {
                    g_windows[i].links[j].hilite = 0;
                    if (g_mouseX >= g_windows[i].x + g_windows[i].links[j].x
                        && g_mouseX < g_windows[i].x + g_windows[i].links[j].x
                                        + StrLenPlat(g_windows[i].links[j].text1) * 8
                        && g_mouseY >= g_windows[i].y + g_windows[i].links[j].y
                        && g_mouseY < g_windows[i].y + g_windows[i].links[j].y + 8) {
                        g_windows[i].links[j].hilite = 1;
                        g_mouseOverTextItem = 1;
                        break;
                    }
                }
                if (g_gadgetHitLatch == 0) {

                    for (j = 0; j < g_windows[i].nG + 1; j++) {
                        g_windows[i].toggles[j].state = 0;
                    }
                    for (j = 0; j < g_windows[i].nG + 1; j++) {
                        g_windows[i].toggles[j].state = 0;
                        if (g_mouseX >= g_windows[i].x + g_windows[i].toggles[j].x
                            && g_mouseX < g_windows[i].x + g_windows[i].toggles[j].x
                                + g_windows[i].toggles[j].w
                            && g_mouseY >= g_windows[i].y + g_windows[i].toggles[j].y
                            && g_mouseY < g_windows[i].y + g_windows[i].toggles[j].y
                                + g_windows[i].toggles[j].h
                            && g_mouseDown != 0) {

                            if (g_windows[i].toggles[j].text[0] != 0) {
                                g_windows[i].toggles[j].state = 1;
                            }
                            g_gadgetHitLatch = 1;
                            break;
                        }
                    }
                }
                if (g_buttonHitLatch == 0) {
                    for (j = 0; j < g_windows[i].nB + 1; j++) {
                        g_windows[i].buttons[j].hover = 0;
                    }
                    for (j = 0; j < g_windows[i].nB + 1; j++) {
                        g_windows[i].buttons[j].hover = 0;

                        if (g_mouseX >= g_windows[i].x + g_windows[i].buttons[j].x
                            && g_mouseX < g_windows[i].x + g_windows[i].buttons[j].x
                                + g_windows[i].buttons[j].w
                            && g_mouseY >= g_windows[i].y + g_windows[i].buttons[j].y
                            && g_mouseY < g_windows[i].y + g_windows[i].buttons[j].y
                                + g_windows[i].buttons[j].h
                            && g_mouseDown != 0) {
                            if (g_windows[i].buttons[j].text[0] != 0) {
                                g_windows[i].buttons[j].hover = 1;
                            }
                            g_buttonHitLatch = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    // ---- mouse press: start a window drag, or arm a field/link for a click ----
    g_winDragActive = 0;
    g_clicked = -1;
    if ((g_mouseClickHandled == 0) & g_mouseDown) {
        g_menuIdleTimeout = g_time + MENU_IDLE_MS;
        g_buttonsOn = 1;
        g_idleFrames = 50;
        g_pressWin = -1;
        g_pressItem = -1;
        for (i = MAX_WINDOWS - 1; i != -1; i--) {

            if (g_windows[i].active != 0
                && g_mouseX > g_windows[i].x
                && g_mouseX < g_windows[i].x + g_windows[i].w
                && g_mouseY > g_windows[i].y
                && g_mouseY < g_windows[i].y + g_windows[i].h
                && g_gadgetHitLatch == 0
                && g_buttonHitLatch == 0) {
                g_winDragActive = 1;
                if (g_dragWin == -1) {
                    g_dragWin = i;
                    g_dragDX = g_mouseX - g_windows[i].x;
                    g_dragDY = g_mouseY - g_windows[i].y;
                    for (j = 0; j < g_windows[i].nF + 1; j++) {

                        if (g_mouseX >= g_windows[i].x + g_windows[i].menuItems[j].x
                            && g_mouseX < g_windows[i].x + g_windows[i].menuItems[j].x
                                + g_windows[i].menuItems[j].w
                            && g_mouseY >= g_windows[i].y + g_windows[i].menuItems[j].y
                            && g_mouseY < g_windows[i].y + g_windows[i].menuItems[j].y
                                + g_windows[i].menuItems[j].h) {
                            g_dragWin = -1;
                            g_pressWin = g_windows[i].index;
                            g_pressItem = g_windows[i].menuItems[j].id;
                            g_mouseClickHandled = 1;
                            g_mouseDown = 0;
                        }
                    }
                    for (j = 0; j < g_windows[i].nD + 1; j++) {
                        g_windows[i].links[j].hilite = 0;

                        if (g_mouseX >= g_windows[i].x + g_windows[i].links[j].x
                            && g_mouseX < g_windows[i].x + g_windows[i].links[j].x
                                            + StrLenPlat(g_windows[i].links[j].text1) * 8
                            && g_mouseY >= g_windows[i].y + g_windows[i].links[j].y
                            && g_mouseY < g_windows[i].y + g_windows[i].links[j].y + 8) {
                            g_windows[i].links[j].hilite = 1;
                            g_dragWin = -1;
                            g_pressWin = g_windows[i].index;
                            g_pressLink = g_windows[i].links[j].id;
                            g_mouseClickHandled = 1;
                            g_mouseDown = 0;
                        }
                    }
                }
            }
        }
    } else {
        g_dragWin = -1;
    }

    // ---- mouse release: commit the click if it landed on the field/link that was armed ----
    if (g_mouseClick != 0) {
        for (i = MAX_WINDOWS - 1; i != -1; i--) {
            if (g_dragWin == -1
                && g_windows[i].active != 0
                && g_windows[i].visible != 0
                && g_pressWin != -1
                && g_mouseX > g_windows[i].x
                && g_mouseX < g_windows[i].x + g_windows[i].w
                && g_mouseY > g_windows[i].y
                && g_mouseY < g_windows[i].y + g_windows[i].h) {
                for (j = 0; j < g_windows[i].nF + 1; j++) {

                    if (g_mouseX >= g_windows[i].x + g_windows[i].menuItems[j].x
                        && g_mouseX < g_windows[i].x + g_windows[i].menuItems[j].x
                            + g_windows[i].menuItems[j].w
                        && g_mouseY >= g_windows[i].y + g_windows[i].menuItems[j].y
                        && g_mouseY < g_windows[i].y + g_windows[i].menuItems[j].y
                            + g_windows[i].menuItems[j].h) {
                        if (g_pressWin == g_windows[i].index
                            && g_pressItem == g_windows[i].menuItems[j].id) {
                            g_clickWin = g_windows[i].index;
                            g_clickItem = g_windows[i].menuItems[j].id;
                            SoundPlay(g_sfxButtonClick, -1, 150, 0.0f, 127, g_sndFlags);
                        } else {
                            g_clickWin = -1;
                            g_clickItem = -1;
                        }
                    }
                }

                for (j = 0; j < g_windows[i].nD + 1; j++) {
                    g_windows[i].links[j].hilite = 0;
                    if (g_mouseX >= g_windows[i].x + g_windows[i].links[j].x
                        && g_mouseX < g_windows[i].x + g_windows[i].links[j].x
                                        + StrLenPlat(g_windows[i].links[j].text1) * 8
                        && g_mouseY >= g_windows[i].y + g_windows[i].links[j].y
                        && g_mouseY < g_windows[i].y + g_windows[i].links[j].y + 8) {
                        g_windows[i].links[j].hilite = 1;
                        if (g_pressWin == g_windows[i].index
                            && g_pressLink == g_windows[i].links[j].id) {
                            g_clickWin = g_windows[i].index;
                            g_clickLink = g_windows[i].links[j].id;
                        } else {
                            g_clickWin = -1;
                            g_clickLink = -1;
                        }
                    }
                }
            }
        }
        g_pressWin = -1;
        g_pressItem = -1;
        g_pressLink = -1;
    }

#ifdef __EMSCRIPTEN__
    if (g_clickItem == 9003) {
        if (WebCanPlay() && !WebIsGuest() && g_state == STATE_TITLE &&
            !g_profileReadOnly && ProfileValid(g_profileIndex)) {
            g_gameMode = MODE_SINGLE;
            g_hofMode = g_cfg.difficulty;
            g_newGameOnClose = 1;
            g_clickItem = MENUID_CANCEL_ALL;
        } else {
            g_clickItem = -1;
            g_clickWin = -1;
        }
    }
    if (g_clickItem == 9000 && g_clickWin >= 0) {
        WebQueueCredentials(g_windows[g_clickWin].edits[0].buf,
                            g_windows[g_clickWin].edits[1].buf, WebLoginMode());
        g_clickWin = g_clickItem = -1;
    } else if (g_clickItem == 9002) {
        WebPlayGuest();
        g_clickWin = g_clickItem = -1;
    } else if (g_clickItem == 9001) {
        WebOpenLogin(!WebLoginMode());
        g_clickWin = g_clickItem = -1;
    }
#endif
    // ---------------- button dispatch ----------------
    // ---- generic window close (CLOSE / LOGIN_CANCEL / CANCEL_ALL) ----
    if (g_clickItem == MENUID_CLOSE) {
        PlaySample();
        WinClose(g_clickWin);
        g_clickWin = -1;
        g_clickItem = -1;
    }
    if (g_clickItem == MENUID_LOGIN_CANCEL) {
        PlaySample();
        WinClose(g_clickWin);
        g_clickWin = -1;
        g_clickItem = -1;
        g_loginWinOpen = 0;
    }

    if (g_clickItem == MENUID_CANCEL_ALL) {
        PlaySample();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;

        if (g_newGameOnClose != 0) {
            g_freshStart = 0;
            NewGame(false);
            g_newGameOnClose = 0;
            g_save.players[g_curPlayer].started = 1;
            g_save.players[g_curPlayer].levelWarpPending = 0;
            StartLevel();
            g_timerA = 0;
            g_introDone = 0;
            g_getReadyFlagA = 0;
            TimerStart1();
            g_drawHudFn = DrawHud1P;
            g_state = STATE_PLAYING;
            LoadSuspended(g_profileIndex);
        } else {
            g_freshStart = 1;
            ResetProfileLives(g_profileIndex);
        }
    }

    // ---- profile menu: logout, profile select ----
    if (g_clickItem == MENUID_LOGOUT) {
#ifdef __EMSCRIPTEN__
        if (WebIsGuest()) WebOpenLogin(0);
        else WebQueueSignOut();
#else
        Logout();
#endif
    }
#ifndef __EMSCRIPTEN__
    if (g_clickItem == MENUID_TOGGLE_PROFILE_SEL) {
        g_clickWin = -1;
        g_clickItem = -1;
        if (g_cfg.profileSel == g_profileIndex) {
            g_cfg.profileSel = -1;
            UnpackAccount(g_profileIndex);
            g_hiscore = g_acc.settings.best;
            g_acc.settings = g_cfg;

            if (g_hiscore > g_acc.settings.best)
                g_acc.settings.best = g_hiscore;
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
        } else {
            g_cfg.profileSel = g_profileIndex;
            UnpackAccount(g_profileIndex);
            g_hiscore = g_acc.settings.best;
            g_acc.settings = g_cfg;
            if (g_hiscore > g_acc.settings.best)
                g_acc.settings.best = g_hiscore;
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
        }
        PlaySample();
        WinCloseAll();
    }

#endif
    // ---- profile: reset confirmation / reset-category dialogs ----
    if (g_clickItem == MENUID_OPEN_RESET_CATEGORY) {
        int h = 225;
        int w = OK_POPUP_W;
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 20, g_curWin, "RESET PROFILE", 6);
        WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_RESET_TIMETRIAL, "RESET TIMETRIAL SCORES AND LOCKS", 5);
        WinAddMenuItem(POS_CENTERED, 60, g_curWin, MENUID_RESET_SECRETS, "         RESET  SECRETS         ", 5);
        WinAddMenuItem(POS_CENTERED, 80, g_curWin, MENUID_RESET_PERCENTAGES, "       RESET  PERCENTAGES       ", 5);
        WinAddMenuItem(POS_CENTERED, 100, g_curWin, MENUID_RESET_BONUS_ROUNDS, "       RESET BONUS ROUNDS       ", 5);
        WinAddMenuItem(POS_CENTERED, 120, g_curWin, MENUID_RESET_TIMES, "          RESET  TIMES          ", 5);
        WinAddMenuItem(POS_CENTERED, 140, g_curWin, MENUID_RESET_ALL_SCORES, "        RESET ALL SCORES        ", 5);
        WinAddMenuItem(POS_CENTERED, 160, g_curWin, MENUID_RESET_ALL, "           RESET  ALL           ", 5);
        WinAddMenuItem(POS_CENTERED, 195, g_curWin, MENUID_CANCEL_ALL, "             CANCEL             ", 5);
        WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
    }

    if (g_clickItem == MENUID_RESET_TIMETRIAL) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        if (g_profileIndex != -1) {
            UnpackAccount(g_profileIndex);
            g_acc.timeTrialHighScore = 0;
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            ClearMedalsMask(g_profileIndex, MEDAL_OVERALL);
            OPEN_OK_POPUP("TIMETRIAL SCORES AND LOCKS RESET");
        }
    }

    if (g_clickItem == MENUID_RESET_SECRETS) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        if (g_profileIndex != -1) {
            UnpackAccount(g_profileIndex);
            g_acc.secretsInOneGame = 0;

            for (int k = 0; k < NUM_LEVEL_SECRETS; k++) {
                g_acc.levelDone[k] = 0;
            }
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            ClearMedalsMask(g_profileIndex, MEDAL_ALL_LEVELS);
            ClearMedalsMask(g_profileIndex, MEDAL_OVERALL);
            OPEN_OK_POPUP("SECRETS RESET");
        }
    }

    if (g_clickItem == MENUID_RESET_PERCENTAGES) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        if (g_profileIndex != -1) {
            UnpackAccount(g_profileIndex);
            g_acc.shotsFired = 0;
            g_acc.shotsHit = 0;
            g_acc.bestHitPctAbove25 = 0;
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            ClearMedalsMask(g_profileIndex, MEDAL_OVERALL);
            OPEN_OK_POPUP("PERCENTAGE RESET");
        }
    }

    if (g_clickItem == MENUID_RESET_BONUS_ROUNDS) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        if (g_profileIndex != -1) {
            UnpackAccount(g_profileIndex);
            g_acc.perfectBonusLevels = 0;
            g_acc.bonusLevelsPlayed = 0;
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            ClearMedalsMask(g_profileIndex, MEDAL_BONUS_RATIO);
            ClearMedalsMask(g_profileIndex, MEDAL_OVERALL);
            OPEN_OK_POPUP("BONUS ROUNDS RESET");
        }
    }

    if (g_clickItem == MENUID_RESET_TIMES) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        if (g_profileIndex != -1) {
            UnpackAccount(g_profileIndex);
            g_acc.bestLevelTime = NO_TIME_RECORDED;
            g_acc.bestMeteorstormTime = NO_TIME_RECORDED;
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            ClearMedalsMask(g_profileIndex, MEDAL_OVERALL);
            OPEN_OK_POPUP("FASTEST TIMES RESET");
        }
    }

    if (g_clickItem == MENUID_RESET_ALL_SCORES) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        if (g_profileIndex != -1) {
            UnpackAccount(g_profileIndex);
            g_acc.level100HighScore = 0;
            g_acc.highScore = 0;
            g_acc.meteorstormHighScore = 0;
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            ClearMedalsMask(g_profileIndex, MEDAL_OVERALL);
            OPEN_OK_POPUP("ALL SCORES IS RESET");
        }
    }

    if (g_clickItem == MENUID_RESET_ALL) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;

        if (g_profileIndex != -1) {
            UnpackAccount(g_profileIndex);
            g_acc.timeTrialHighScore = 0;
            g_acc.highestRank = 0;
            g_acc.unusedF4 = 0;
            g_acc.unlockedRank = 20;
            g_acc.highestLevel = 0;
            g_acc.totalLevelsPlayed = 0;
            g_acc.unusedC4 = 30;
            g_acc.shotsFired = 0;
            g_acc.shotsHit = 0;
            g_acc.secretsInOneGame = 0;
            g_acc.bonusLevelsPlayed = 0;

            g_acc.perfectBonusLevels = 0;
            g_acc.medals = 0;
            g_acc.bestHitPctAbove25 = 0;
            g_acc.level100HighScore = 0;
            g_acc.highScore = 0;
            g_acc.meteorstormHighScore = 0;
            g_acc.highestMoney = 0;
            g_acc.completionRank = 0;
            g_acc.medalStep = -1;
            g_acc.medalOrder[0] = 0;
            g_acc.medalOrder[1] = 0;
            g_acc.medalOrder[2] = 0;
            g_acc.medalOrder[3] = 0;
            g_acc.medalOrder[4] = 0;
            g_acc.medalOrder[5] = 0;
            g_acc.medalOrder[6] = 0;
            g_acc.gameCompleted = 0;
            g_acc.cheatDetected = 0;
            g_acc.bestLevelTime = NO_TIME_RECORDED;
            g_acc.bestMeteorstormTime = NO_TIME_RECORDED;

            for (int k = 0; k < NUM_LEVEL_SECRETS; k++) {
                g_acc.levelDone[k] = 0;
            }
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            OPEN_OK_POPUP("PROFILE IS RESET");
        }
    }

    // ---- profile: backup / restore ----
    if (g_clickItem == MENUID_OPEN_BACKUP_PROFILE) {
        int h = OK_POPUP_H;
        int w = OK_POPUP_W;
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        MakeProfilesDir();
        bool ok = true;
        char path[512];
        sprintf(path, "%s\\warblade\\backup", SysUserFolder());
        bool exists = SysFileExists(path);

        if (!exists) {
            char path[512];
            sprintf(path, "%s\\warblade\\backup", SysUserFolder());
            if (!SysMakeDir(path))
                ok = false;
        }
        if (ok) {
            char fn[260];
            char fnto[260];
            sprintf(fn, "%s\\warblade\\profiles\\profile%03d.acc",
                    SysUserFolder(), g_profileIndex);
            sprintf(fnto, "%s\\warblade\\backup\\profile%03d.acc",
                    SysUserFolder(), g_profileIndex);
            if (!CopyProfileFile(fn, fnto))
                ok = false;
        }

        if (ok) {
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 20, g_curWin, "BACKUP OF PROFILE IS COMPLETE", 7);
            WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_CANCEL_ALL, " OK ", 5);
            WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
        } else {
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 20, g_curWin, "BACKUP ERROR!", 7);
            WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_CANCEL_ALL, " OK ", 5);
            WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
        }
    }

    if (g_clickItem == MENUID_OPEN_RESTORE_WARNING) {
        int h = 110;
        int w = 500;
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 20, g_curWin, "W A R N I N G", 5);
        WinAddText(POS_CENTERED, 40, g_curWin, "DO YOU REALY WANT TO RESTORE THE PROFILE?", 7);
        WinAddText(POS_CENTERED, 50, g_curWin, "RECORDED INFO AND LOCKS MAY BE LOST!", 7);
        WinAddMenuItem(30, h - 30, g_curWin, MENUID_RESTORE_PROFILE_YES, " OK ", 5);
        WinAddMenuItem(w - 111, h - 30, g_curWin, MENUID_CANCEL_ALL, " CANCEL ", 5);
        WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
    }

    if (g_clickItem == MENUID_RESTORE_PROFILE_YES) {
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        int h = OK_POPUP_H;
        int w = OK_POPUP_W;
        int err = 0;
        int len;
        char path[512];
        sprintf(path, "%s\\warblade\\backup", SysUserFolder());
        bool exists = SysFileExists(path);
        if (exists) {

            if (GetAccountTime(g_profileIndex) == LoadBackupAccount(g_profileIndex)) {
                char fn[260];
                char fnto[260];
                sprintf(fnto, "%s\\warblade\\profiles\\profile%03d.acc",
                        SysUserFolder(), g_profileIndex);
                sprintf(fn, "%s\\warblade\\backup\\profile%03d.acc",
                        SysUserFolder(), g_profileIndex);
                if (!CopyProfileFile(fn, fnto))
                    err = 3;
                len = LoadAccount(g_profileIndex);
                if (!DecodeAccount(&g_accBuf[g_profileIndex], g_profileIndex, len))
                    ResetAccount();
#ifdef __EMSCRIPTEN__
                WebNormalizeAccount();
#endif
                PackAccount(g_profileIndex);
                SaveAccount(g_profileIndex);
            } else {
                err = 2;
            }
        } else {
            err = 1;
        }

        if (err == 0) {
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 20, g_curWin, "RESTORE OF PROFILE IS COMPLETE", 7);
            WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_CANCEL_ALL, " OK ", 5);
            WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
        }
        if (err == 1) {
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 20, g_curWin, "NO BACKUP FOUND!", 7);
            WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_CANCEL_ALL, " OK ", 5);
            WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
        }

        if (err == 3) {
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 20, g_curWin, "RESTORE COPY ERROR!", 7);
            WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_CANCEL_ALL, " OK ", 5);
            WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
        }
        if (err == 2) {
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 20, g_curWin, "RESTORE PROFILE MATCH ERROR!", 7);
            WinAddMenuItem(POS_CENTERED, 40, g_curWin, MENUID_CANCEL_ALL, " OK ", 5);
            WinSetSelected(g_curWin, MENUID_CANCEL_ALL);
        }
    }

    // ---- profile: change username / change password ----
#ifndef __EMSCRIPTEN__
    if (g_clickItem == MENUID_OPEN_CHANGE_USERNAME) {
        int h = 115;
        int w = 500;
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 20, g_curWin, "CHANGE USER NAME", 7);
        WinAddText(24, 40, g_curWin, "OLD USER NAME :", 7);
        GetProfileName(g_profileIndex);
        WinAddText(144, 40, g_curWin, g_logBuf, 7);
        WinAddText(24, 55, g_curWin, "NEW USER NAME :", 7);
        WinAddEdit(152, 55, g_curWin, 29, 0, 7, 1);
        WinAddMenuItem(24, h - 35, g_curWin, MENUID_CHANGE_USERNAME_OK, "OK", 5);
        WinSetSelected(g_curWin, MENUID_CHANGE_USERNAME_OK);
        WinAddMenuItem(w - 90, h - 35, g_curWin, MENUID_CANCEL_ALL, "CANCEL", 5);
    }

    if (g_clickItem == MENUID_CHANGE_USERNAME_OK) {
        if (StrLenPlat(g_windows[g_clickWin].edits[0].buf) >= 2) {
            UnpackAccount(g_profileIndex);
            for (int k = 0; k < NAME_LEN; k++) {
                g_acc.name[k] = g_windows[g_clickWin].edits[0].buf[k];
            }
            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            DecompressHiscores();
            __int64 t = GetAccountTime(g_profileIndex);
            for (int k = 0; k < MAX_HISCORES; k++) {
                if (g_hiscoreMagic.table[0][k].ownerStamp == t)
                    strcpy(g_hiscoreMagic.table[0][k].name, g_windows[g_clickWin].edits[0].buf);

                if (g_hiscoreMagic.table[1][k].ownerStamp == t)
                    strcpy(g_hiscoreMagic.table[1][k].name, g_windows[g_clickWin].edits[0].buf);
                if (g_hiscoreMagic.table[2][k].ownerStamp == t)
                    strcpy(g_hiscoreMagic.table[2][k].name, g_windows[g_clickWin].edits[0].buf);
                if (g_hiscoreMagic.table[3][k].ownerStamp == t)
                    strcpy(g_hiscoreMagic.table[3][k].name, g_windows[g_clickWin].edits[0].buf);
                if (g_hiscoreMagic.table[4][k].ownerStamp == t)
                    strcpy(g_hiscoreMagic.table[4][k].name, g_windows[g_clickWin].edits[0].buf);

                if (g_hiscoreMagic.table5[k].ownerStamp == t)
                    strcpy(g_hiscoreMagic.table5[k].name, g_windows[g_clickWin].edits[0].buf);
            }
            CompressHiscores();
            WriteHiscoreFile();
            WinCloseAll();
            g_clickWin = -1;
            g_clickItem = -1;
            g_profileWinOpen = 0;
            OPEN_OK_POPUP("USER NAME CHANGED");
        } else {
            g_clickWin = -1;
            g_clickItem = -1;
        }
    }

    if (g_clickItem == MENUID_OPEN_CHANGE_PASSWORD) {
        int h = 115;
        int w = 500;
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_profileWinOpen = 0;
        g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 20, g_curWin, "CHANGE PASSWORD", 7);
        WinAddText(24, 40, g_curWin, "ENTER OLD PASSWORD :", 7);
        WinAddEdit(192, 40, g_curWin, PASSWORD_LEN, 1, 7, 1);
        WinAddText(24, 55, g_curWin, "NEW PASSWORD :", 7);
        WinAddEdit(144, 55, g_curWin, PASSWORD_LEN, 1, 7, 0);
        WinAddMenuItem(24, h - 35, g_curWin, MENUID_CHANGE_PASSWORD_OK, "OK", 5);
        WinSetSelected(g_curWin, MENUID_CHANGE_PASSWORD_OK);
        WinAddMenuItem(w - 90, h - 35, g_curWin, MENUID_CANCEL_ALL, "CANCEL", 5);
    }

    if (g_clickItem == MENUID_CHANGE_PASSWORD_OK) {
        UnpackAccount(g_profileIndex);
        bool ok = true;
        if (StrLenPlat(g_windows[g_clickWin].edits[0].buf) != StrLenPlat(g_acc.password)) {
            ok = false;
        } else {
            for (int k = 0; k < StrLenPlat(g_windows[g_clickWin].edits[0].buf); k++) {
                if (g_acc.password[k] != g_windows[g_clickWin].edits[0].buf[k])
                    ok = false;
            }
        }
        if (ok && StrLenPlat(g_windows[g_clickWin].edits[1].buf) >= 1) {

            for (int k = 0; k < PASSWORD_LEN; k++) {
                g_acc.password[k] = g_windows[g_clickWin].edits[1].buf[k];
            }

            PackAccount(g_profileIndex);
            SaveAccount(g_profileIndex);
            WinCloseAll();
            g_clickWin = -1;
            g_clickItem = -1;
            g_profileWinOpen = 0;
            OPEN_OK_POPUP("PASSWORD IS NOW CHANGED");
        } else if (!ok) {
            WinCloseAll();
            g_clickWin = -1;
            g_clickItem = -1;
            g_profileWinOpen = 0;
            OPEN_OK_POPUP("INCORRECT PASSWORD !");
        } else {
            g_clickWin = -1;
            g_clickItem = -1;
        }
    }

#endif
    // ---- jukebox launch / quit-game / quit-to-desktop confirmations ----
    if (g_clickItem == MENUID_JUKEBOX_LAUNCH_CONFIRM) {
        SoundPause();
        g_clickWin = -1;
        g_clickItem = -1;
        SaveSetPro();
        g_lastActivityTime = g_time;
        g_attractScreen = ATTRACT_HELP_CONTROLS;
        g_idleTimeoutMs = 15000;
        WriteHiscoreFile();
        ClearHiscores();
        g_mouseDown = 0;
        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
        g_transitionLock = 1;
        BeforeOpenLink();

        SysOpenJukebox();
        SysMinimize();
        return;
    }
    if (g_clickItem == MENUID_QUIT_GAME_CONFIRM) {
        ResumeGame();
        PlaySample();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_quitGameWinOpen = 0;
        ResetToTitle();
        g_inputCooldown = 100;
        g_autoplay = 0;
    }

    if (g_clickItem == MENUID_RETIRE_CONFIRM) {
        ResumeGame();
        PlaySample();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_quitGameWinOpen = 0;
        g_frameFunc();
        FlushBlit(0);
        FlushQuads(0);
        FlushStretchF();
        FlushStretchRot();
        FlushStretchI();

        FlushStretchRot2();
        FlushBlit2(0);
        FlipBuffer(0);
        g_frameFunc();
        FlushBlit(0);
        FlushQuads(0);
        FlushStretchF();
        FlushStretchRot();
        FlushStretchI();
        FlushStretchRot2();
        FlushBlit2(0);
        FlipBuffer(0);
        g_frameFunc();
        FlushBlit(0);
        FlushQuads(0);
        FlushStretchF();
        FlushStretchRot();
        FlushStretchI();
        FlushStretchRot2();
        FlushBlit2(0);
        FlipBuffer(0);
        ShowHiscoreTable();
    }

    if (g_clickItem == MENUID_QUIT_GAME_TO_WINDOWS) {
        DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1);
        if (g_sfxGoodbye != 0) {
            AudioStop();
            AudioStart();
            g_exitSoundStartTime = g_time;
            SoundPlay2(g_sfxGoodbye, -1, 255, 0.0f, 255, g_sndFlags);
            unsigned int until = g_time + GOODBYE_WAIT_MS;
            do {
                g_time = SysMillis();
                if (g_time == 0)
                    g_time = SysMillis();

                if (g_soundEnabled != 0)
                    AudioUpdate();
            } while (until > g_time);
        }
        if (g_state != STATE_TITLE)
            ResumeGame();
        PlaySample();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_quitGameWinOpen = 0;
        MergeSettings(g_profileIndex);
        WriteSettings();
        WriteHiscoreFile();
        CLEAR_DRAW_COUNTERS();
        DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1);

        SysTerminate();
    }
    if (g_clickItem == MENUID_QUIT_GAME_CONTINUE) {
        if (g_state != STATE_TITLE)
            ResumeGame();
        PlaySample();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_quitGameWinOpen = 0;
    }

    if (g_clickItem == MENUID_QUIT_TO_WINDOWS_YES) {
        ApplyMusicVolume();
        SetSfxVolume(g_cfg.sfxVol);
        DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1);
        if (g_sfxGoodbye != 0) {
            AudioStop();
            AudioStart();
            g_exitSoundStartTime = g_time;
            SoundPlay2(g_sfxGoodbye, -1, 255, 0.0f, 255, g_sndFlags);
            unsigned int until = g_time + GOODBYE_WAIT_MS;
            do {
                g_time = SysMillis();
                if (g_time == 0)
                    g_time = SysMillis();

                if (g_soundEnabled != 0)
                    AudioUpdate();
            } while (until > g_time);
        }
        if (g_state != STATE_TITLE)
            ResumeGame();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_quitToWindowsWinOpen = 0;
        MergeSettings(g_profileIndex);
        WriteSettings();
        WriteHiscoreFile();
        CLEAR_DRAW_COUNTERS();
        DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1);

        SysTerminate();
    }
    if (g_clickItem == MENUID_QUIT_TO_WINDOWS_NO) {
        if (g_state != STATE_TITLE)
            ResumeGame();
        PlaySample();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        g_quitToWindowsWinOpen = 0;
    }

    if (g_clickItem == MENUID_DIALOG_DISMISS_A) {
        g_clickWin = -1;
        g_clickItem = -1;
        PlaySample();
        WinCloseAll();
    }

    // ---- initial quality presets offered on first run (4000 low .. 4003 ultra) ----
    if (g_clickItem == MENUID_QUALITY_PRESET_LOW) {
        g_cfg.borderMode = BORDER_OFF;
        g_cfg.sfxVol = 204;
        g_cfg.musicVolume = 179;
        g_cfg.musicVol = 255;
        g_cfg.unused030 = 0;
        g_cfg.musicFormat = MUSIC_FMT_MOD;
        g_cfg.numStars = 100.0f;
        g_cfg.sfxOn = 0;
        g_cfg.unused050 = 1;
        g_cfg.unused05c = 0;

        g_cfg.collisionDetail = COLLISION_SIMPLE;
        g_cfg.bgStars = 1;
        g_cfg.unused068 = 0;
        g_cfg.bulletIntensity = BULLETS_NORMAL;
        g_cfg.particlesOn = 0;
        g_cfg.bgEnabled = 0;
        g_cfg.bgTint = BG_BRIGHTNESS_PRESET;
        g_cfg.sparks = 15.0f;
        g_cfg.alienBuffer = 5;
        g_maxSparks = ((int)g_cfg.sparks >> 1 < 5) ? 5 : (int)g_cfg.sparks >> 1;
        g_restartNeeded = 1;
        g_clickWin = -1;
        g_clickItem = -1;
        WinCloseAll();

        if (g_cfg.bgStars != 0)
            g_fnPtr = DrawStarsPlayer;
        else
            g_fnPtr = DrawStarsGlow;
        OpenCreateProfileWin();
    }

    if (g_clickItem == MENUID_QUALITY_PRESET_MEDIUM) {
        g_cfg.borderMode = BORDER_OFF;
        g_cfg.sfxVol = 204;
        g_cfg.musicVolume = 179;
        g_cfg.musicVol = 255;
        g_cfg.unused030 = 0;
        g_cfg.musicFormat = MUSIC_FMT_MOD;
        g_cfg.numStars = 300.0f;
        g_cfg.sfxOn = 0;
        g_cfg.unused050 = 1;
        g_cfg.unused05c = 1;
        g_cfg.collisionDetail = COLLISION_NORMAL;

        g_cfg.bgStars = 1;
        g_cfg.unused068 = 0;
        g_cfg.bulletIntensity = BULLETS_NORMAL;
        g_cfg.particlesOn = 0;
        g_cfg.bgEnabled = 0;
        g_cfg.bgTint = BG_BRIGHTNESS_PRESET;
        g_cfg.sparks = 30.0f;
        g_cfg.alienBuffer = 5;
        g_maxSparks = ((int)g_cfg.sparks >> 1 < 5) ? 5 : (int)g_cfg.sparks >> 1;
        g_restartNeeded = 1;
        g_clickWin = -1;
        g_clickItem = -1;
        WinCloseAll();

        if (g_cfg.bgStars != 0)
            g_fnPtr = DrawStarsPlayer;
        else
            g_fnPtr = DrawStarsGlow;
        OpenCreateProfileWin();
    }

    if (g_clickItem == MENUID_QUALITY_PRESET_HIGH) {
        g_cfg.borderMode = BORDER_ON;
        g_cfg.sfxVol = 204;
        g_cfg.musicVolume = 179;
        g_cfg.musicVol = 255;
        g_cfg.unused030 = 0;
        g_cfg.musicFormat = MUSIC_FMT_MOD;
        g_cfg.numStars = 750.0f;
        g_cfg.sfxOn = 1;
        g_cfg.unused050 = 1;
        g_cfg.unused05c = 0;
        g_cfg.collisionDetail = COLLISION_NORMAL;

        g_cfg.bgStars = 0;
        g_cfg.unused068 = 1;
        g_cfg.bulletIntensity = BULLETS_NORMAL;
        g_cfg.particlesOn = 1;
        g_cfg.bgEnabled = 0;
        g_cfg.bgTint = BG_BRIGHTNESS_PRESET;
        g_cfg.sparks = 100.0f;
        g_cfg.alienBuffer = 5;
        g_maxSparks = ((int)g_cfg.sparks >> 1 < 5) ? 5 : (int)g_cfg.sparks >> 1;
        g_restartNeeded = 1;
        g_clickWin = -1;
        g_clickItem = -1;
        WinCloseAll();

        if (g_cfg.bgStars != 0)
            g_fnPtr = DrawStarsPlayer;
        else
            g_fnPtr = DrawStarsGlow;
        OpenCreateProfileWin();
    }

    if (g_clickItem == MENUID_QUALITY_PRESET_ULTRA) {
        g_cfg.borderMode = BORDER_ON;
        g_cfg.sfxVol = 204;
        g_cfg.musicVolume = 179;
        g_cfg.musicVol = 255;
        g_cfg.unused030 = 0;
        g_cfg.musicFormat = MUSIC_FMT_MOD;
        g_cfg.numStars = 1000.0f;
        g_cfg.sfxOn = 1;
        g_cfg.unused050 = 1;
        g_cfg.unused05c = 0;

        g_cfg.collisionDetail = COLLISION_NORMAL;
        g_cfg.bgStars = 0;
        g_cfg.unused068 = 1;
        g_cfg.bulletIntensity = BULLETS_FLARE_FX;
        g_cfg.particlesOn = 1;
        g_cfg.bgEnabled = 1;
        g_cfg.bgTint = BG_BRIGHTNESS_PRESET;
        g_cfg.sparks = 150.0f;
        g_cfg.alienBuffer = 5;
        g_maxSparks = ((int)g_cfg.sparks >> 1 < 5) ? 5 : (int)g_cfg.sparks >> 1;
        g_restartNeeded = 1;
        g_clickWin = -1;
        g_clickItem = -1;
        WinCloseAll();

        if (g_cfg.bgStars != 0)
            g_fnPtr = DrawStarsPlayer;
        else
            g_fnPtr = DrawStarsGlow;
        OpenCreateProfileWin();
    }
    if (g_clickItem == MENUID_DIALOG_DISMISS_B) {
        g_clickWin = -1;
        g_clickItem = -1;
        PlaySample();
        WinCloseAll();
    }

    // ---- "NO" on a confirmation window (OpenCreateProfileWin) ----
    if (g_clickItem == MENUID_DIALOG_NO) {
        g_clickWin = -1;
        g_clickItem = -1;
        PlaySample();
        WinCloseAll();
    }

    // ---- new-profile window, quick starts, profile-slot click ranges ----
#ifndef __EMSCRIPTEN__
    if (g_clickItem == MENUID_PROFILE_NEW || g_clickItem == MENUID_CREATE_PROFILE_YES) {
        WinCloseAll();
        WinHideAll();
        g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, LOGIN_WIN_W, LOGIN_WIN_H, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 20, g_curWin, "NEW PROFILE", 8);
        WinAddText(24, 40, g_curWin, "USER NAME :", 8);
        WinAddEdit(115, 40, g_curWin, 29, 0, 7, 1);
        WinAddText(24, 50, g_curWin, "PASSWORD  :", 8);
        WinAddEdit(115, 50, g_curWin, PASSWORD_LEN, 1, 7, 0);
        WinAddMenuItem(24, 75, g_curWin, MENUID_NEW_PROFILE_OK, "NORMAL PROFILE", 5);
        WinAddMenuItem(214, 75, g_curWin, MENUID_NEW_PROFILE_EASY, "EASY PROFILE", 5);
        WinSetSelected(g_curWin, MENUID_NEW_PROFILE_OK);
        WinAddMenuItem(360, 75, g_curWin, MENUID_CLOSE, "CANCEL", 5);
        g_clickWin = -1;
        g_clickItem = -1;
    }

#endif
    if (g_clickItem == MENUID_QUICK_START_TIME_TRIAL) {
        WinCloseAll();
        WinHideAll();
        g_clickWin = -1;
        g_clickItem = -1;
        if (CheckTimeTrialAvailable()) {
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            g_gameMode = MODE_TIME_TRIAL;
            g_timeTrialDeadline = g_time + TIME_TRIAL_DURATION_MS;
            g_hofMode = HOF_TIME_TRIAL;
            g_cfg.gamesPlayed = 0;
            NewGame(true);
            return;
        }
    }

    if (g_clickItem == MENUID_QUICK_START_1P) {
        WinCloseAll();
        WinHideAll();
        g_clickWin = -1;
        g_clickItem = -1;
        NewGame(true);
        return;
    }
#ifndef __EMSCRIPTEN__
    if (g_clickItem >= PROFILE_USE_ID_BASE && g_clickItem <= PROFILE_USE_ID_BASE + 10 && !AnyWindowHasEdit()) {

        if (g_profileIndex == g_clickItem - PROFILE_USE_ID_BASE) {
            WinCloseAll();
            g_profileReadOnly = 0;
            ProfileWindow(false);
        } else {
            WinCloseAll();
            g_selProfile = g_clickItem - PROFILE_USE_ID_BASE;
            WinHideAll();
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, LOGIN_WIN_W, LOGIN_WIN_H, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 20, g_curWin, "USE PROFILE", 8);
            WinAddText(24, 40, g_curWin, "PASSWORD :", 8);
            WinAddEdit(115, 40, g_curWin, PASSWORD_LEN, 1, 7, 1);
            WinAddMenuItem(24, 75, g_curWin, MENUID_LOGIN_OK, "OK", 5);
            WinSetSelected(g_curWin, MENUID_LOGIN_OK);
            WinAddMenuItem(360, 75, g_curWin, MENUID_CLOSE, "CANCEL", 5);
        }
        g_clickWin = -1;
        g_clickItem = -1;
    }

    if (g_clickItem >= PROFILE_VIEW_ID_BASE && g_clickItem <= PROFILE_VIEW_ID_BASE + 10 && !AnyWindowHasEdit()) {
        if (g_profileIndex == g_clickItem - PROFILE_VIEW_ID_BASE) {
            WinCloseAll();
            g_profileReadOnly = 0;
            ProfileWindow(false);
        } else {
            WinCloseAll();
            g_profileReadOnly = 1;
            int saved = g_profileIndex;
            g_profileIndex = g_clickItem - PROFILE_VIEW_ID_BASE;
            ProfileWindow(false);
            g_profileIndex = saved;
        }
        g_clickWin = -1;
        g_clickItem = -1;
    }

    // ---- login submit: password check, then config sanity clamps ----
    if (g_clickItem == MENUID_LOGIN_OK && StrLenPlat(g_windows[g_clickWin].edits[0].buf) > 0) {
        UnpackAccount(g_selProfile);
        bool ok = true;
        if (StrLenPlat(g_windows[g_clickWin].edits[0].buf) != StrLenPlat(g_acc.password)) {
            ok = false;
        } else {
            for (int k = 0; k < StrLenPlat(g_windows[g_clickWin].edits[0].buf); k++) {
                if (g_acc.password[k] != g_windows[g_clickWin].edits[0].buf[k])
                    ok = false;
            }
        }
        if (ok) {
            ActivateProfile(g_selProfile);
        }
        ClearAccount();
        g_newGameOnClose = 0;

        if (ok) {
            if (ProfileValid(g_profileIndex))
                g_newGameOnClose = 1;
            WinCloseAll();
            g_profileReadOnly = 0;
            ProfileWindow(false);
            g_clickWin = -1;
            g_clickItem = -1;
        } else {
            for (int k = 0; k < g_windows[g_clickWin].edits[0].len; k++) {
                g_windows[g_clickWin].edits[0].buf[k] = 0;
            }
            g_windows[g_clickWin].edits[0].cursor = 0;
            g_clickWin = -1;
            g_clickItem = -1;
        }
    }

    // ---- new-profile submit, easy-profile warning and submit ----
    if (g_clickItem == MENUID_NEW_PROFILE_OK && StrLenPlat(g_windows[g_clickWin].edits[0].buf) > 0
        && StrLenPlat(g_windows[g_clickWin].edits[1].buf) > 0) {
        int slot = NextAccountReset() - 1;
        if (slot != -1) {
            ResetAccount();
            for (int k = 0; k < NAME_LEN; k++) {
                g_acc.name[k] = g_windows[g_clickWin].edits[0].buf[k];
            }

            for (int k = 0; k < PASSWORD_LEN; k++) {
                g_acc.password[k] = g_windows[g_clickWin].edits[1].buf[k];
            }
            PackAccount(slot);
            SaveAccount(slot);
        }
        ClearAccount();
        WinCloseAll();
        g_clickWin = -1;
        g_clickItem = -1;
        int h = PROFILE_LIST_WIN_H;
        int w = PROFILE_LIST_WIN_W;
        WinHideAll();
        g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
        g_profileWin = g_curWin;
        WinAddText(POS_CENTERED, 20, g_curWin, "USER PROFILES", 8);

        for (int p = 0; p < g_profileCount; p++) {
            GetProfileName(p);
            WinAddText(30, p * 17 + 40, g_curWin, g_logBuf, 8);
            if (p == g_cfg.profileSel)
                WinAddText(20, p * 17 + 40, g_curWin, "*", 6);
            if (g_profileIndex == p)
                WinAddMenuItem(270, p * 17 + 36, g_curWin, p + PROFILE_USE_ID_BASE, "  IN  USE  ", 5);
            else
                WinAddMenuItem(270, p * 17 + 36, g_curWin, p + PROFILE_USE_ID_BASE, "USE PROFILE", 5);
            WinAddMenuItem(374, p * 17 + 36, g_curWin, p + PROFILE_VIEW_ID_BASE, "VIEW", 5);
        }

        if (g_profileCount != 10)
            WinAddMenuItem(27, h - 40, g_curWin, MENUID_PROFILE_NEW, "NEW", 5);
        WinAddMenuItem(w - 87, h - 40, g_curWin, MENUID_CLOSE, "CLOSE", 5);
        WinSetSelected(g_curWin, MENUID_CLOSE);
    }
    if (g_clickItem == MENUID_NEW_PROFILE_EASY) {
        for (int k = 0; k < 35; k++) {
            g_newName[k] = g_windows[g_clickWin].edits[0].buf[k];
        }

        for (int k = 0; k < 35; k++) {
            g_newPass[k] = g_windows[g_clickWin].edits[1].buf[k];
        }
        WinCloseAll();
        g_curWin = WinOpen(POS_CENTERED, 220, 550, 160, WIN_MODE_SLIDING);
        WinAddText(POS_CENTERED, 20, g_curWin, "W A R N I N G", 2);
        WinAddText(POS_CENTERED, 50, g_curWin, "USING AN EASY PROFILE WILL MAKE SOME", 2);
        WinAddText(POS_CENTERED, 65, g_curWin, "HIGH RANKS AND SOME OTHER FEATURES UNAVAILABLE!", 2);
        WinAddText(POS_CENTERED, 100, g_curWin, "DO YOU WANT TO CREATE THIS EASY PROFILE?", 2);
        WinAddMenuItem(20, 125, g_curWin, MENUID_EASY_PROFILE_YES, " YES! ", 2);
        WinAddMenuItem(470, 125, g_curWin, MENUID_CLOSE, " NO! ", 3);
        WinSetSelected(g_curWin, MENUID_CLOSE);
        g_clickWin = -1;
        g_clickItem = -1;
    }

    if (g_clickItem == MENUID_EASY_PROFILE_YES && StrLenPlat(g_newName) > 0 && StrLenPlat(g_newPass) > 0) {
        int slot = NextAccountReset() - 1;
        if (slot != -1) {
            ResetAccount();
            for (int k = 0; k < NAME_LEN; k++) {
                g_acc.name[k] = g_newName[k];
            }

            for (int k = 0; k < PASSWORD_LEN; k++) {
                g_acc.password[k] = g_newPass[k];
            }
            g_acc.easy = 1;
            PackAccount(slot);
            SaveAccount(slot);
        }
        ClearAccount();
        g_clickWin = -1;
        g_clickItem = -1;
        WinCloseAll();
        int h = PROFILE_LIST_WIN_H;
        int w = PROFILE_LIST_WIN_W;
        WinHideAll();
        g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, w, h, WIN_MODE_SLIDING);
        g_profileWin = g_curWin;
        WinAddText(POS_CENTERED, 20, g_curWin, "USER PROFILES", 8);

        for (int p = 0; p < g_profileCount; p++) {
            GetProfileName(p);
            WinAddText(30, p * 17 + 40, g_curWin, g_logBuf, 8);
            if (p == g_cfg.profileSel)
                WinAddText(20, p * 17 + 40, g_curWin, "*", 6);
            if (g_profileIndex == p)
                WinAddMenuItem(270, p * 17 + 36, g_curWin, p + PROFILE_USE_ID_BASE, "  IN  USE  ", 5);
            else
                WinAddMenuItem(270, p * 17 + 36, g_curWin, p + PROFILE_USE_ID_BASE, "USE PROFILE", 5);
            WinAddMenuItem(374, p * 17 + 36, g_curWin, p + PROFILE_VIEW_ID_BASE, "VIEW", 5);
        }

        if (g_profileCount != 10)
            WinAddMenuItem(27, h - 40, g_curWin, MENUID_PROFILE_NEW, "NEW", 5);
        WinAddMenuItem(w - 87, h - 40, g_curWin, MENUID_CLOSE, "CLOSE", 5);
        WinSetSelected(g_curWin, MENUID_CLOSE);
    }
#endif
    // ---- text-link click: open its URL ----
    if (g_clickLink != -1 && g_clickWin != -1) {
        SoundPause();
        BeforeOpenLink();
        OpenUrl(g_windows[g_clickWin].links[g_clickLink - 1].text2);
        SysMinimize();
        g_clickWin = -1;
        g_clickLink = -1;
        return;
    }

    // ---- keyboard input for the focused edit box: backspace, tab, enter, typed characters ----
    if (AnyWindowHasEdit()) {
        bool handled = false;
        int ew = -1;
        for (i = MAX_WINDOWS - 1; i != -1; i--) {
            if (g_windows[i].active && g_windows[i].visible && g_windows[i].nH >= 0
                && g_windows[i].firstH >= 0 && g_windows[i].firstH <= g_windows[i].nH)
                ew = i;
        }
        if (ew != -1) {
            if (KeyDown(K_VK_BACK)) {
                if (g_keyLatch[K_VK_BACK] != 0) {
                    g_pressWin = -1;
                    g_pressItem = -1;
                    g_pressLink = -1;

                    if (g_windows[ew].edits[g_windows[ew].firstH].cursor > 0) {
                        g_windows[ew].edits[g_windows[ew].firstH].cursor--;
                        g_windows[ew].edits[g_windows[ew].firstH]
                            .buf[g_windows[ew].edits[g_windows[ew].firstH].cursor] = 0;
                        PlayClick();
                        handled = true;
                    }
                    g_keyLatch[K_VK_BACK] = 0;
                }
            } else {
                g_keyLatch[K_VK_BACK] = 1;
            }
            if (KeyDown(K_VK_TAB) && !KeyDown(K_VK_MENU)) {

                if (g_keyLatch[K_VK_TAB] != 0) {
                    g_pressWin = -1;
                    g_pressItem = -1;
                    g_pressLink = -1;
                    PlayClick();
                    WinFocusNextEdit(ew);
                    g_keyLatch[K_VK_TAB] = 0;
                    handled = true;
                }
            } else {
                g_keyLatch[K_VK_TAB] = 1;
            }
            if (KeyDown(K_VK_RETURN)) {

                if (g_keyLatch[K_VK_RETURN] != 0) {
                    g_pressWin = -1;
                    g_pressItem = -1;
                    g_pressLink = -1;
                    PlayClick();
#ifdef __EMSCRIPTEN__
                    if (g_loginWinOpen && g_windows[ew].selF == 9000 && g_windows[ew].firstH == 1) {
                        g_clickWin = ew;
                        g_clickItem = 9000;
                    } else
#endif
                    if (g_windows[ew].nH + 1 == 1) {
                        g_clickWin = ew;
                        g_clickItem = g_windows[ew].selF;
                    } else {
                        WinFocusNextEdit(ew);
                    }
                    g_keyLatch[K_VK_RETURN] = 0;
                    handled = true;
                }
            } else {
                g_keyLatch[K_VK_RETURN] = 1;
            }
            int ch = 0;

            if (!handled
#ifdef __EMSCRIPTEN__
                && !g_loginWinOpen
#endif
            ) {
                GetPressedKeyName();
                if (g_pressedKey != -1) {
                    g_pressWin = -1;
                    g_pressItem = -1;
                    g_pressLink = -1;
                    if (g_pressedKey == 4 && g_windows[ew].edits[g_windows[ew].firstH].cursor == 0) {
                        g_keyLatch[g_pressedKey] = 0;
                        PlayClick();
                    } else if (g_keyLatch[g_pressedKey] != 0) {
                        ch = KeyToChar(g_pressedKey);
                        g_keyLatch[g_pressedKey] = 0;
                        PlayClick();
                    }
                } else {
                    ResetFlags();
                }
            }

            if (ch != 0
                && g_windows[ew].edits[g_windows[ew].firstH].cursor < g_windows[ew].edits[g_windows[ew].firstH].len) {
                g_windows[ew].edits[g_windows[ew].firstH]
                    .buf[g_windows[ew].edits[g_windows[ew].firstH].cursor] = (char)ch;
                g_windows[ew].edits[g_windows[ew].firstH].cursor++;
            }
        }
    }
    // ---- keyboard navigation of the topmost window's menu items (enter/up/down), no edit box focused ----
    if (!AnyWindowHasEdit()) {
        int top = -1;
        int count = 0;
        for (int w = 0; w < MAX_WINDOWS - 1; w++) {

            if (g_windows[w].active != 0) {
                top = w;
                count++;
            }
        }
        if (top != -1) {
            if (KeyDown(K_VK_RETURN)) {
                if (g_keyLatch[K_VK_RETURN] != 0) {
                    PlayClick();
                    int sel = -1;
                    for (int k = 0; k < g_windows[top].nF + 1; k++) {
                        if (g_windows[top].menuItems[k].checked != 0)
                            sel = k;
                    }

                    if (sel != -1) {
                        g_clickWin = top;
                        g_clickItem = g_windows[top].menuItems[sel].id;
                    } else {
                        g_clickWin = top;
                        g_clickItem = g_windows[top].selF;
                    }
                    g_keyLatch[K_VK_RETURN] = 0;
                }
            } else {
                g_keyLatch[K_VK_RETURN] = 1;
            }
            if (KeyDown(K_VK_UP)) {

                if (g_keyLatch[K_VK_UP] != 0) {
                    PlayClick();
                    if (g_windows[top].nF > 0) {
                        int sel = -1;
                        for (int k = 0; k < g_windows[top].nF + 1; k++) {
                            if (g_windows[top].menuItems[k].checked != 0)
                                sel = k;
                        }
                        if (sel == -1)
                            sel = 0;
                        else if (--sel < 0)
                            sel = g_windows[top].nF;

                        for (int k = 0; k < g_windows[top].nF + 1; k++) {
                            g_windows[top].menuItems[k].checked = 0;
                        }
                        g_windows[top].menuItems[sel].checked = 1;
                    }
                    g_keyLatch[K_VK_UP] = 0;
                }
            } else {
                g_keyLatch[K_VK_UP] = 1;
            }
            if (KeyDown(K_VK_DOWN)) {
                if (g_keyLatch[K_VK_DOWN] != 0) {
                    PlayClick();

                    if (g_windows[top].nF > 0) {
                        int sel = -1;
                        for (int k = 0; k < g_windows[top].nF + 1; k++) {
                            if (g_windows[top].menuItems[k].checked != 0)
                                sel = k;
                        }
                        if (sel == -1) {
                            sel = 0;
                        } else {
                            sel++;
                            if (sel > g_windows[top].nF)
                                sel = 0;
                        }

                        for (int k = 0; k < g_windows[top].nF + 1; k++) {
                            g_windows[top].menuItems[k].checked = 0;
                        }
                        g_windows[top].menuItems[sel].checked = 1;
                    }
                    g_keyLatch[K_VK_DOWN] = 0;
                }
            } else {
                g_keyLatch[K_VK_DOWN] = 1;
            }
        }
    }
}
#undef OPEN_OK_POPUP

// Per-frame update while in a menu/attract screen: runs MenuUpdate for the popup window
// system, hit-tests the main menu entries (g_menuEntries[]) for mouse clicks, then handles
// every menu button id and keyboard shortcut (difficulty, volumes, video/quality options,
// profile shortcuts, hiscore submission, jukebox/help/quit links, ...), and finally advances
// the attract-mode screen sequence (intro/about/mission/help/hiscores) on idle timeout.
void MenuHandler()
{
    int i;
    // NOTE: never read after being cleared; dead in this build (see `moved` below).
    char unused = 0;
    int moved;

    SoundStopAll();
    g_save.players[0].starVelX = g_sinDeg[(int)g_angX] * g_starSpeedX;
    g_save.players[0].starSpeed = g_sinDeg[(int)g_angY] * g_starSpeedY;
    g_save.players[0].starVelZ = g_sinDeg[(int)g_angZ] * g_starSpeedZ;
    // NOTE: always 0 here, so the mouse-moved branch below and the `moved` argument to
    // MenuUpdate are both permanently dead in this build.
    moved = 0;
    if (moved != 0) {
        g_lastActivityTime = g_time;
        g_buttonsOn = 1;
        g_idleFrames = 0;
        g_menuIdleTimeout = g_time + MENU_IDLE_MS;
    } else {
        g_idleFrames++;
        if (g_idleFrames > 300) g_buttonsOn = 0;
    }
    MenuUpdate(moved != 0);

    if (g_mouseDown != 0 && g_mouseClickHandled == 0 && !g_winDragActive && !AnyWindowActive()) {
        g_pressed = -1;
        for (i = 0; i < MAX_MENU_ENTRIES; i++) {
            if (g_mouseX >= g_menuEntries[i].x &&
                g_mouseX < g_menuEntries[i].x + g_menuEntries[i].w &&
                g_mouseY >= g_menuEntries[i].y &&
                g_mouseY < g_menuEntries[i].y + g_menuEntries[i].h &&
                g_menuEntries[i].visible != 0) {
                g_pressed = g_menuEntries[i].group;
                g_mouseClick = 1;
                break;
            }
        }
    }

    if (g_mouseClick != 0 && g_pressed != -1) {
        for (i = 0; i < MAX_MENU_ENTRIES; i++) {
            if (g_mouseX >= g_menuEntries[i].x &&
                g_mouseX < g_menuEntries[i].x + g_menuEntries[i].w &&
                g_mouseY >= g_menuEntries[i].y &&
                g_mouseY < g_menuEntries[i].y + g_menuEntries[i].h &&
                g_menuEntries[i].visible != 0 &&
                g_pressed == g_menuEntries[i].group) {
                g_pressed = -1;
                g_clicked = g_menuEntries[i].group;
                g_mouseClick = 0;
                break;
            }
        }
    }

    // ---- main menu button dispatch (mouse clicks on g_clicked ids) ----
    if (g_transitionLock == 0 && !AnyWindowHasEdit()) {
        // -- game-mode launch buttons: 1P, 2P, meteorstorm, time trial --
        if ((g_clicked == MENUID_START || g_clicked == MENUID_START_1P) && !KeyDown(K_VK_ESCAPE)) {
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            g_gameMode = MODE_SINGLE;
            NewGame(1);
            return;
        }

        if (g_clicked == MENUID_START_2P && !KeyDown(K_VK_ESCAPE)) {
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            Logout();
            g_gameMode = MODE_TWO_PLAYER;
            NewGame(1);
            return;
        }
        if (g_clicked == MENUID_START_2P_DUEL && !KeyDown(K_VK_ESCAPE)) {
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            Logout();
            g_gameMode = MODE_DUAL;
            NewGame(1);
            return;
        }

        if (g_clicked == MENUID_START_TIME_TRIAL && !KeyDown(K_VK_ESCAPE) && CheckTimeTrialAvailable()) {
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            g_gameMode = MODE_TIME_TRIAL;
            g_timeTrialDeadline = g_time + TIME_TRIAL_DURATION_MS;
            g_hofMode = HOF_TIME_TRIAL;
            NewGame(1);
            return;
        }
        // -- attract-screen shortcut buttons (about/mission/help/hiscores) --
        if (g_clicked == MENUID_ABOUT) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_ABOUT;
            g_idleTimeoutMs = 60000;
        }

        if (g_clicked == MENUID_STORY) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_MISSION;
            g_idleTimeoutMs = 60000;
        }
        if (g_clicked == MENUID_SETTINGS) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
        }
        if (g_clicked == MENUID_INPUT) {
            g_cursor = 0;
            g_state = STATE_INPUT_CONFIG;
        }

        if (g_clicked == MENUID_BONUSES) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_BONUSES;
            g_idleTimeoutMs = 60000;
        }
        if (g_clicked == MENUID_HISCORE) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 60000;
            g_hofMode = g_cfg.difficulty;
        }

        if (g_clicked == MENUID_HOF_EASY) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 60000;
            g_hofMode = HOF_EASY;
        }
        if (g_clicked == MENUID_HOF_NORMAL) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 60000;
            g_hofMode = HOF_NORMAL;
        }

        if (g_clicked == MENUID_HOF_HARD) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 60000;
            g_hofMode = HOF_HARD;
        }
        if (g_clicked == MENUID_HOF_ACE) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 60000;
            g_hofMode = HOF_ACE;
        }

        if (g_clicked == MENUID_HOF_METEORSTORM) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 60000;
            g_hofMode = HOF_METEORSTORM;
        }
        if (g_clicked == MENUID_HOF_TIME_TRIAL) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 60000;
            g_hofMode = HOF_TIME_TRIAL;
        }

        // -- help/FAQ/manual web links --
        if (g_clicked == MENUID_HELP) {
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            WriteHiscoreFile();
            ClearHiscores();
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            BeforeOpenLink();
            OpenUrl("http://www.warblade.as/help.asp");
            SysMinimize();
            return;
        }

        if (g_clicked == MENUID_FAQ) {
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            WriteHiscoreFile();
            ClearHiscores();
            BeforeOpenLink();
            OpenUrl("http://www.warblade.as/faq.asp");
            SysMinimize();
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            return;
        }

        if (g_clicked == MENUID_USER_MANUAL) {
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            WriteHiscoreFile();
            ClearHiscores();
            BeforeOpenLink();
            SysMinimize();
            OpenUrl("http://www.warblade.as/manual.txt");
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            return;
        }

        // -- "A" key / button 174: toggle the profile list window --
        if (g_clicked == MENUID_TOGGLE_PROFILE_LIST || KeyDown(K_VK_A)) {
            if ((&g_keyLatch[K_VK_A])[0] != 0) {
                if (AnyWindowActive()) {
                    WinCloseAll();
                    PlayClick();
                } else {
                    WinCloseAll();
                    PlayClick();
#ifdef __EMSCRIPTEN__
                    if (!WebCanPlay())
                        WebOpenLogin(0);
                    else {
                        g_profileReadOnly = 0;
                        ProfileWindow(false);
                    }
#else
                    int winH = PROFILE_LIST_WIN_H;
                    int winW = PROFILE_LIST_WIN_W;
                    WinHideAll();
                    g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, winW, winH, WIN_MODE_SLIDING);
                    g_profileWin = g_curWin;
                    WinAddText(POS_CENTERED, 20, g_curWin, "USER PROFILES", 8);

                    for (int j = 0; j < g_profileCount; j++) {
                        GetProfileName(j);
                        WinAddText(30, j * 17 + 40, g_curWin, g_logBuf, 8);
                        if (j == g_cfg.profileSel)
                            WinAddText(20, j * 17 + 40, g_curWin, "*", 6);
                        if (g_profileIndex == j)
                            WinAddMenuItem(270, j * 17 + 36, g_curWin, j + PROFILE_USE_ID_BASE, "  IN  USE  ", 5);
                        else
                            WinAddMenuItem(270, j * 17 + 36, g_curWin, j + PROFILE_USE_ID_BASE, "USE PROFILE", 5);
                        WinAddMenuItem(374, j * 17 + 36, g_curWin, j + PROFILE_VIEW_ID_BASE, "VIEW", 5);
                    }

                    if (g_profileCount != 10)
                        WinAddMenuItem(27, winH - 40, g_curWin, MENUID_PROFILE_NEW, "NEW", 5);
                    WinAddMenuItem(winW - 87, winH - 40, g_curWin, MENUID_CLOSE, "CLOSE", 5);
                    WinSetSelected(g_curWin, MENUID_CLOSE);
#endif
                }
                (&g_keyLatch[K_VK_A])[0] = 0;
            }
        } else {
            (&g_keyLatch[K_VK_A])[0] = 1;
        }
        // -- keep the profile window's popup system updated, and let TAB / the profile-open
        //    input close it (or open it, in the else branch below) --
        if (g_profileWinOpen) {

            if (moved != 0) WinClearMenuChecks();
            MenuUpdate(moved != 0);
            if (KeyDown(K_VK_TAB) && !KeyDown(K_VK_MENU)
                && g_profileWinOpen && g_time > g_statSubmitCooldown) {
                if (g_keyLatch[K_VK_TAB] != 0) {
                    PlaySample();
                    WinCloseAll();
                    g_profileWinOpen = false;
                    g_buttonsOn = 0;
                    HidePointer();
                    g_keyLatch[K_VK_TAB] = 0;
                    g_statSubmitCooldown = g_time + 750;
                }
            } else {
                g_keyLatch[K_VK_TAB] = 1;
            }

            if (InputProfile(g_curPlayer) && g_profileWinOpen && g_time > g_statSubmitCooldown) {
                if (g_save.players[g_curPlayer].keyLatchProfile != 0) {
                    PlaySample();
                    WinCloseAll();
                    g_profileWinOpen = false;
                    g_buttonsOn = 0;
                    HidePointer();
                    g_save.players[g_curPlayer].keyLatchProfile = 0;
                    g_statSubmitCooldown = g_time + 750;
                }
            } else {
                g_save.players[g_curPlayer].keyLatchProfile = 1;
            }
        } else {

            if (KeyDown(K_VK_TAB) && !KeyDown(K_VK_MENU)
                && !g_profileWinOpen && g_time > g_statSubmitCooldown) {
                if (g_keyLatch[K_VK_TAB] != 0) {
                    if (g_profileIndex != -1) {
                        g_profileWinOpen = true;
                        g_profileReadOnly = 0;
                        ProfileWindow(1);
                        g_statSubmitCooldown = g_time + 750;
                    }
                    g_keyLatch[K_VK_TAB] = 0;
                }
            } else {
                g_keyLatch[K_VK_TAB] = 1;
            }

            if (InputProfile(g_curPlayer) && !g_profileWinOpen && g_time > g_statSubmitCooldown) {
                if (g_save.players[g_curPlayer].keyLatchProfile != 0) {
                    if (g_profileIndex != -1) {
                        g_profileWinOpen = true;
                        g_profileReadOnly = 0;
                        ProfileWindow(1);
                        g_statSubmitCooldown = g_time + 750;
                    }
                    g_save.players[g_curPlayer].keyLatchProfile = 0;
                }
            } else {
                g_save.players[g_curPlayer].keyLatchProfile = 1;
            }
        }

        // -- button 90: open the profile list window directly --
        if (g_clicked == MENUID_USER_PROFILES) {
#ifdef __EMSCRIPTEN__
            WinCloseAll();
            if (!WebCanPlay())
                WebOpenLogin(0);
            else {
                g_profileReadOnly = 0;
                ProfileWindow(false);
            }
#else
            int winH = PROFILE_LIST_WIN_H;
            int winW = PROFILE_LIST_WIN_W;
            WinHideAll();
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, winW, winH, WIN_MODE_SLIDING);
            g_profileWin = g_curWin;
            WinAddText(POS_CENTERED, 20, g_curWin, "USER PROFILES", 8);
            for (int j = 0; j < g_profileCount; j++) {
                GetProfileName(j);
                WinAddText(30, j * 17 + 40, g_curWin, g_logBuf, 8);
                if (j == g_cfg.profileSel)
                    WinAddText(20, j * 17 + 40, g_curWin, "*", 6);

                if (g_profileIndex == j)
                    WinAddMenuItem(270, j * 17 + 36, g_curWin, j + PROFILE_USE_ID_BASE, "  IN  USE  ", 5);
                else
                    WinAddMenuItem(270, j * 17 + 36, g_curWin, j + PROFILE_USE_ID_BASE, "USE PROFILE", 5);
                WinAddMenuItem(374, j * 17 + 36, g_curWin, j + PROFILE_VIEW_ID_BASE, "VIEW", 5);
            }
            if (g_profileCount != 10)
                WinAddMenuItem(27, winH - 40, g_curWin, MENUID_PROFILE_NEW, "NEW", 5);
            WinAddMenuItem(winW - 87, winH - 40, g_curWin, MENUID_CLOSE, "CLOSE", 5);
            WinSetSelected(g_curWin, MENUID_CLOSE);
#endif
        }

        // -- button 100: quit to Windows (from the options screen) --
        if (g_clicked == MENUID_QUIT_YES) {
            MergeSettings(g_profileIndex);
            WriteSettings();
            WriteHiscoreFile();
            DrawRect(0.0f, 0.0f, (float)g_screenW, (float)g_screenH, 0.0f, 0.0f, 0.0f, 1.0f);
            if (g_sfxGoodbye != 0) {
                AudioStop();
                AudioStart();
                g_exitSoundStartTime = g_time;
                SoundPlay2(g_sfxGoodbye, -1, 255, 0.0f, 255, g_sndFlags);
                unsigned int until = g_time + GOODBYE_WAIT_MS;
                do {
                    g_time = SysMillis();

                    if (g_time == 0) g_time = SysMillis();
                    if (g_soundEnabled != 0) AudioUpdate();
                } while (until > g_time);
            }
            SysTerminate();
            return;
        }

        // -- options screen: difficulty, music/sfx volume, border, background, collision
        //    detail, particles, bullet intensity, alien buffer, background stars --
        if (g_clicked == MENUID_DIFFICULTY_PREV) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            if (g_cfg.difficulty > DIFF_EASY) {
                PlayClick();
                g_cfg.difficulty--;
            }
            g_hofMode = g_cfg.difficulty;

            if (g_profileIndex != -1 && g_cfg.difficulty == DIFF_EASY) {
                g_cfg.difficulty = DIFF_NORMAL;
                g_hofMode = HOF_NORMAL;
                g_cfg.fps = FPS_NORMAL;
                DoNothing();
            }
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_DIFFICULTY_NEXT) {
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;

            if (g_cfg.difficulty < DIFF_ACE) {
                PlayClick();
                g_cfg.difficulty++;
            }
            g_hofMode = g_cfg.difficulty;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_MUSIC_FORMAT_PREV || g_clicked == MENUID_MUSIC_FORMAT_NEXT) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            CycleMusicFormat();
            PlayClick();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        if (g_clicked == MENUID_MUSICVOLUME_MUTE) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.musicVolume = 0;
            ApplyMusicVolume();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_MUSICVOLUME_DOWN) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;

            if ((g_cfg.musicVolume -= 4) < 0) g_cfg.musicVolume = 0;
            ApplyMusicVolume();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }
        if (g_clicked == MENUID_MUSICVOLUME_UP) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.musicVolume += 4;

            if (g_cfg.musicVolume > 255) g_cfg.musicVolume = 255;
            ApplyMusicVolume();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }
        if (g_clicked == MENUID_MUSICVOLUME_MAX) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.musicVolume = 255;
            ApplyMusicVolume();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        if (g_clicked == MENUID_SFXVOL_MUTE) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.sfxVol = 0;
            SetSfxVolume(g_cfg.sfxVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_SFXVOL_DOWN) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;

            if ((g_cfg.sfxVol -= 4) < 0) g_cfg.sfxVol = 0;
            SetSfxVolume(g_cfg.sfxVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }
        if (g_clicked == MENUID_SFXVOL_UP) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.sfxVol += 4;

            if (g_cfg.sfxVol > 255) g_cfg.sfxVol = 255;
            SetSfxVolume(g_cfg.sfxVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }
        if (g_clicked == MENUID_SFXVOL_MAX) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.sfxVol = 255;
            SetSfxVolume(g_cfg.sfxVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            g_volumeSnapClick = 1;
        }

        if (g_clicked == MENUID_MUSICVOL_MUTE) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.musicVol = 0;
            SetMusicVolTable(g_cfg.musicVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            g_volumeSnapClick = 1;
        }
        if (g_clicked == MENUID_MUSICVOL_DOWN) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;

            if ((g_cfg.musicVol -= 4) < 0) g_cfg.musicVol = 0;
            SetMusicVolTable(g_cfg.musicVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }
        if (g_clicked == MENUID_MUSICVOL_UP) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.musicVol += 4;

            if (g_cfg.musicVol > 255) g_cfg.musicVol = 255;
            SetMusicVolTable(g_cfg.musicVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }
        if (g_clicked == MENUID_MUSICVOL_MAX) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.musicVol = 255;
            SetMusicVolTable(g_cfg.musicVol);
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            g_volumeSnapClick = 1;
        }

        if (g_clicked == MENUID_SFX_TOGGLE_PREV || g_clicked == MENUID_SFX_TOGGLE_NEXT) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.sfxOn = !g_cfg.sfxOn;
            PlayClick();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_BORDER_MODE_PREV || g_clicked == MENUID_BORDER_MODE_NEXT) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;

            if (g_cfg.borderMode == BORDER_BLACK) {
                g_cfg.borderMode = BORDER_ON;
                ImgSetBlitColor(g_gfxBorderEasy, 1, 1, 1, 1);
                ImgSetBlitColor(g_gfxBorderNormal, 1, 1, 1, 1);
                ImgSetBlitColor(g_gfxBorderHard, 1, 1, 1, 1);
                ImgSetBlitColor(g_gfxBorderAce, 1, 1, 1, 1);
            } else if (g_cfg.borderMode == BORDER_ON) {
                g_cfg.borderMode = BORDER_OFF;
                ImgSetBlitColor(g_gfxBorderEasy, 1, 1, 1, 1);
                ImgSetBlitColor(g_gfxBorderNormal, 1, 1, 1, 1);
                ImgSetBlitColor(g_gfxBorderHard, 1, 1, 1, 1);
                ImgSetBlitColor(g_gfxBorderAce, 1, 1, 1, 1);
            } else {
                g_cfg.borderMode = BORDER_BLACK;
                ImgSetBlitColor(g_gfxBorderEasy, 0, 0, 0, 1);
                ImgSetBlitColor(g_gfxBorderNormal, 0, 0, 0, 1);
                ImgSetBlitColor(g_gfxBorderHard, 0, 0, 0, 1);
                ImgSetBlitColor(g_gfxBorderAce, 0, 0, 0, 1);
            }
            PlayClick();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        if (g_clicked == MENUID_BG_ENABLED_PREV || g_clicked == MENUID_BG_ENABLED_NEXT) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            if (g_cfg.bgEnabled == 0) g_cfg.bgEnabled = 1;
            else g_cfg.bgEnabled = 0;
            PlayClick();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_BG_BRIGHTNESS_PREV) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;

            switch (g_cfg.bgTint) {
            case BG_BRIGHTNESS_MIN: g_cfg.bgTint = BG_BRIGHTNESS_MAX; break;
            case BG_BRIGHTNESS_DEFAULT: g_cfg.bgTint = BG_BRIGHTNESS_MIN; break;
            case BG_BRIGHTNESS_PRESET: g_cfg.bgTint = BG_BRIGHTNESS_DEFAULT; break;
            case 85: g_cfg.bgTint = BG_BRIGHTNESS_PRESET; break;
            case 100: g_cfg.bgTint = 85; break;
            case BG_BRIGHTNESS_MAX: g_cfg.bgTint = 100; break;
            default: g_cfg.bgTint = BG_BRIGHTNESS_MIN;
            }
            PlayClick();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        if (g_clicked == MENUID_BG_BRIGHTNESS_NEXT) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            switch (g_cfg.bgTint) {
            case BG_BRIGHTNESS_MIN: g_cfg.bgTint = BG_BRIGHTNESS_DEFAULT; break;
            case BG_BRIGHTNESS_DEFAULT: g_cfg.bgTint = BG_BRIGHTNESS_PRESET; break;
            case BG_BRIGHTNESS_PRESET: g_cfg.bgTint = 85; break;
            case 85: g_cfg.bgTint = 100; break;
            case 100: g_cfg.bgTint = BG_BRIGHTNESS_MAX; break;
            case BG_BRIGHTNESS_MAX: g_cfg.bgTint = BG_BRIGHTNESS_MIN; break;
            default: g_cfg.bgTint = BG_BRIGHTNESS_MIN;
            }
            PlayClick();
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        if (g_clicked == MENUID_NUM_STARS_DOWN) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.numStars -= 25.0;

            if (g_cfg.numStars < 50.0) g_cfg.numStars = 50.0f;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }
        if (g_clicked == MENUID_NUM_STARS_UP) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.numStars += 25.0;
            if (g_cfg.numStars > 3000.0) g_cfg.numStars = 3000.0f;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_FAST_MS;
            g_transitionLock = 1;
            g_mouseClick = 1;
        }

        if (g_clicked == MENUID_COLLISION_DETAIL_PREV) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            if (g_cfg.collisionDetail == COLLISION_SIMPLE) g_cfg.collisionDetail = COLLISION_NORMAL;
            else g_cfg.collisionDetail = COLLISION_SIMPLE;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_COLLISION_DETAIL_NEXT) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;

            if (g_cfg.collisionDetail == COLLISION_SIMPLE) g_cfg.collisionDetail = COLLISION_NORMAL;
            else g_cfg.collisionDetail = COLLISION_SIMPLE;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (KeyDown(K_VK_C)) {
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            if (g_cfg.collisionDetail == COLLISION_SIMPLE) g_cfg.collisionDetail = COLLISION_NORMAL;
            else g_cfg.collisionDetail = COLLISION_SIMPLE;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        if (g_clicked == MENUID_PARTICLES_PREV || g_clicked == MENUID_PARTICLES_NEXT ||
            (KeyDown(K_VK_F) && !KeyDown(K_VK_MENU) &&
             !KeyDown(K_VK_L_SHIFT) && !KeyDown(K_VK_R_SHIFT))) {
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.particlesOn = !g_cfg.particlesOn;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        if (g_clicked == MENUID_BULLET_INTENSITY_PREV || g_clicked == MENUID_BULLET_INTENSITY_NEXT || KeyDown(K_VK_I)) {
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.bulletIntensity++;
            if (g_cfg.bulletIntensity > BULLETS_FLARE_FX) g_cfg.bulletIntensity = BULLETS_NORMAL;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
        if (g_clicked == MENUID_ALIEN_BUFFER_DOWN) {
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.alienBuffer--;

            if (g_cfg.alienBuffer < 5) g_cfg.alienBuffer = 5;
            g_transitionLockUntil = g_time + 62;
            g_transitionLock = 1;
            g_restartNeeded = 1;
            g_memUntil = g_time + 10000;
        }
        if (g_clicked == MENUID_ALIEN_BUFFER_UP) {
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 60000;
            g_cfg.alienBuffer++;

            if (g_cfg.alienBuffer > 100) g_cfg.alienBuffer = 100;
            g_transitionLockUntil = g_time + 62;
            g_transitionLock = 1;
            g_restartNeeded = 1;
            g_memUntil = g_time + 10000;
        }
        if (g_clicked == MENUID_BG_STARS_PREV || g_clicked == MENUID_BG_STARS_NEXT || KeyDown(K_VK_Z)) {
            PlayClick();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.bgStars = !g_cfg.bgStars;

            if (g_cfg.bgStars != 0) g_fnPtr = DrawStarsPlayer;
            else g_fnPtr = DrawStarsGlow;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }
    }
    // ---- jukebox launch (button 137) ----
    if (g_clicked == MENUID_JUKEBOX && !AnyWindowActive()) {
        SoundPause();
        g_lastActivityTime = g_time;
        g_attractScreen = ATTRACT_HELP_CONTROLS;
        g_idleTimeoutMs = 15000;
        WriteHiscoreFile();
        ClearHiscores();
        g_mouseDown = 0;
        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
        g_transitionLock = 1;
        BeforeOpenLink();

        SysOpenJukebox();
        SysMinimize();
        return;
    }
    // ---- spark count +/- hotkeys ("E" / shift+"E") ----
    if ((g_clicked == MENUID_SPARKS_UP ||
         (KeyDown(K_VK_E) && !KeyDown(K_VK_L_SHIFT)
          && !KeyDown(K_VK_R_SHIFT))) &&
        !AnyWindowActive()) {
        g_lastActivityTime = g_time;
        g_attractScreen = ATTRACT_HELP_CONTROLS;
        g_idleTimeoutMs = 15000;
        g_cfg.sparks += 2.0;

        if (g_cfg.sparks > 150.0) g_cfg.sparks = 150.0f;
        g_maxSparks = ((int)g_cfg.sparks >> 1) < 5 ? 5 : (int)g_cfg.sparks >> 1;
    }
    if ((g_clicked == MENUID_SPARKS_DOWN ||
         (KeyDown(K_VK_E)
          && (KeyDown(K_VK_L_SHIFT) || KeyDown(K_VK_R_SHIFT)))) &&
        !AnyWindowActive()) {
        g_lastActivityTime = g_time;
        g_attractScreen = ATTRACT_HELP_CONTROLS;
        g_idleTimeoutMs = 15000;
        g_cfg.sparks -= 2.0;
        if (g_cfg.sparks < 10.0) g_cfg.sparks = 10.0f;
        g_maxSparks = ((int)g_cfg.sparks >> 1) < 5 ? 5 : (int)g_cfg.sparks >> 1;
    }

    // ---- escape key: open the quit-to-Windows dialog, or close any open window ----
    if (KeyDown(K_VK_ESCAPE)) {
        if (g_keyLatch[K_VK_ESCAPE] != 0) {
            g_keyLatch[K_VK_ESCAPE] = 0;
            if (!AnyWindowHasEdit() && !AnyWindowActive()) {
#ifndef __EMSCRIPTEN__      // the browser build has no quitting
                g_quitToWindowsWinOpen = true;
                g_buttonsOn = 1;
                QuitToWindowsDialog();
#endif
            } else {
                WinCloseAll();
                g_profileWinOpen = false;
                g_buttonsOn = 0;

                if (g_newGameOnClose) {
                    NewGame(0);
                    g_newGameOnClose = false;
                    g_save.players[g_curPlayer].started = 1;
                    g_save.players[g_curPlayer].levelWarpPending = 0;
                    StartLevel();
                    g_timerA = 0;
                    g_introDone = 0;
                    g_getReadyFlagA = 0;
                    TimerStart1();
                    g_state = STATE_PLAYING;
                    g_drawHudFn = DrawHud1P;
                    LoadSuspended(g_profileIndex);
                }
            }
        }
    } else {
        g_keyLatch[K_VK_ESCAPE] = 1;
    }

    // ---- keyboard shortcuts active while no window/edit box has focus: difficulty select,
    //      hall-of-fame browsing, volumes, quick-start keys, and misc toggles ----
    if (!AnyWindowHasEdit() && !AnyWindowActive()) {
        if (KeyDown(K_VK_1)) {
            if (g_keyLatch[K_VK_1] != 0) {
                if (g_profileIndex == -1 || GetProfileEasyFlag(g_profileIndex)) {
                    PlayClick();
                    g_lastActivityTime = g_time;
                    g_attractScreen = ATTRACT_HELP_CONTROLS;
                    g_idleTimeoutMs = 15000;
                    g_cfg.difficulty = DIFF_EASY;
                    g_hofMode = HOF_EASY;
                    g_keyLatch[K_VK_1] = 0;
                    g_cfg.fps = FPS_EASY;
                }
            }
        } else {
            g_keyLatch[K_VK_1] = 1;
        }

        if (KeyDown(K_VK_2)) {
            if (g_keyLatch[K_VK_2] != 0) {
                if (g_profileIndex == -1 || !GetProfileEasyFlag(g_profileIndex)) {
                    PlayClick();
                    g_lastActivityTime = g_time;
                    g_attractScreen = ATTRACT_HELP_CONTROLS;
                    g_idleTimeoutMs = 15000;
                    g_cfg.difficulty = DIFF_NORMAL;
                    g_hofMode = HOF_NORMAL;
                    g_keyLatch[K_VK_2] = 0;
                    g_cfg.fps = FPS_NORMAL;
                }
            }
        } else {
            g_keyLatch[K_VK_2] = 1;
        }

        if (KeyDown(K_VK_3)) {
            if (g_keyLatch[K_VK_3] != 0) {
                if (g_profileIndex == -1 || !GetProfileEasyFlag(g_profileIndex)) {
                    PlayClick();
                    g_lastActivityTime = g_time;
                    g_attractScreen = ATTRACT_HELP_CONTROLS;
                    g_idleTimeoutMs = 15000;
                    g_cfg.difficulty = DIFF_HARD;
                    g_hofMode = HOF_HARD;
                    g_keyLatch[K_VK_3] = 0;
                    g_cfg.fps = FPS_HARD;
                }
            }
        } else {
            g_keyLatch[K_VK_3] = 1;
        }

        if (KeyDown(K_VK_4)) {
            if (g_keyLatch[K_VK_4] != 0) {
                if (g_profileIndex == -1 || !GetProfileEasyFlag(g_profileIndex)) {
                    PlayClick();
                    g_lastActivityTime = g_time;
                    g_attractScreen = ATTRACT_HELP_CONTROLS;
                    g_idleTimeoutMs = 15000;
                    g_cfg.difficulty = DIFF_ACE;
                    g_hofMode = HOF_ACE;
                    g_keyLatch[K_VK_4] = 0;
                    g_cfg.fps = FPS_ACE_MENU;
                }
            }
        } else {
            g_keyLatch[K_VK_4] = 1;
        }

        if (KeyDown(K_VK_H)) {
            if (g_keyLatch[K_VK_H] != 0) {
                PlayClick();
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HALL_OF_FAME;
                g_idleTimeoutMs = 60000;
                g_hofMode = g_cfg.difficulty;
                g_keyLatch[K_VK_H] = 0;
            }
        } else {
            g_keyLatch[K_VK_H] = 1;
        }
        if (KeyDown(K_VK_UP)) {

            if (g_keyLatch[K_VK_UP] != 0) {
                PlayClick();
                if (g_hofMode < HOF_TIME_TRIAL) g_hofMode++;
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HALL_OF_FAME;
                g_idleTimeoutMs = 60000;
                g_keyLatch[K_VK_UP] = 0;
            }
        } else {
            g_keyLatch[K_VK_UP] = 1;
        }
        if (KeyDown(K_VK_DOWN)) {
            if (g_keyLatch[K_VK_DOWN] != 0) {
                PlayClick();

                if (g_hofMode > HOF_EASY) g_hofMode--;
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HALL_OF_FAME;
                g_idleTimeoutMs = 60000;
                g_keyLatch[K_VK_DOWN] = 0;
            }
        } else {
            g_keyLatch[K_VK_DOWN] = 1;
        }
        if (KeyDown(K_VK_PAGEDOWN)) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;

            if (--g_cfg.sfxVol < 0) g_cfg.sfxVol = 0;
            SetSfxVolume(g_cfg.sfxVol);
        }
        if (KeyDown(K_VK_PAGEUP)) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.sfxVol++;
            if (g_cfg.sfxVol > 255) g_cfg.sfxVol = 255;
            SetSfxVolume(g_cfg.sfxVol);
        }
        if (KeyDown(K_VK_SUBTRACT)) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;

            if (--g_cfg.musicVolume < 0) g_cfg.musicVolume = 0;
            ApplyMusicVolume();
        }
        if (KeyDown(K_VK_ADD)) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.musicVolume++;
            if (g_cfg.musicVolume > 255) g_cfg.musicVolume = 255;
            ApplyMusicVolume();
        }
        if (KeyDown(K_VK_MULTIPLY) && IsKeyFree(0x47)) StartMusic();

        if (KeyDown(K_VK_END)) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            if (--g_cfg.musicVol < 0) g_cfg.musicVol = 0;
            SetMusicVolTable(g_cfg.musicVol);
        }
        if (KeyDown(K_VK_HOME)) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.musicVol++;

            if (g_cfg.musicVol > 255) g_cfg.musicVol = 255;
            SetMusicVolTable(g_cfg.musicVol);
        }
        if ((KeyDown(K_VK_SPACE) || InputMenuFire(0))
            && !KeyDown(K_VK_ESCAPE) && g_transitionLock == 0) {
            if (g_autoplay) {

                if (CheckTimeTrialAvailable() && RandRange(0, 100) < 50) {
                    g_gameMode = MODE_TIME_TRIAL;
                    g_timeTrialDeadline = g_time + TIME_TRIAL_DURATION_MS;
                    g_hofMode = HOF_TIME_TRIAL;
                    Logout();
                    NewGame(1);
                    return;
                } else {
                    g_menuIdleTimeout = g_time + MENU_IDLE_MS;
                    g_gameMode = MODE_SINGLE;
                    g_hofMode = g_cfg.difficulty;
                    Logout();
                    NewGame(1);
                    return;
                }
            }
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            g_timeTrialDeadline = g_time + TIME_TRIAL_DURATION_MS;
            NewGame(1);
            return;
        }

        if (KeyDown(K_VK_F1) && !KeyDown(K_VK_ESCAPE)
            && g_transitionLock == 0) {
#ifndef __EMSCRIPTEN__
            if (KeyDown(K_VK_MENU)
                && (KeyDown(K_VK_L_SHIFT) || KeyDown(K_VK_R_SHIFT))) {
                g_autoplay = true;

                if (CheckTimeTrialAvailable() && RandRange(0, 100) < 10) {
                    PlayClick();
                    g_gameMode = MODE_TIME_TRIAL;
                    g_timeTrialDeadline = g_time + TIME_TRIAL_DURATION_MS;
                    g_hofMode = HOF_TIME_TRIAL;
                    Logout();
                    NewGame(1);
                    return;
                } else {
                    PlayClick();
                    g_menuIdleTimeout = g_time + MENU_IDLE_MS;
                    g_gameMode = MODE_SINGLE;
                    g_hofMode = g_cfg.difficulty;
                    Logout();
                    NewGame(1);
                    return;
                }
            }
#endif
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            g_gameMode = MODE_SINGLE;
            g_hofMode = g_cfg.difficulty;
            NewGame(1);
            return;
        }

        if (KeyDown(K_VK_F2) && !KeyDown(K_VK_ESCAPE)
            && g_transitionLock == 0) {
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            g_gameMode = MODE_TWO_PLAYER;
            Logout();
            g_hofMode = g_cfg.difficulty;
            NewGame(1);
            return;
        }

        if (KeyDown(K_VK_F3) && !KeyDown(K_VK_ESCAPE)
            && g_transitionLock == 0) {
            PlayClick();
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            g_gameMode = MODE_DUAL;
            Logout();
            g_hofMode = g_cfg.difficulty;
            NewGame(1);
            return;
        }
        if (KeyDown(K_VK_F5) && !KeyDown(K_VK_ESCAPE)
            && g_transitionLock == 0) {
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;

            if (CheckTimeTrialAvailable()) {
                PlayClick();
                g_gameMode = MODE_TIME_TRIAL;
                g_timeTrialDeadline = g_time + TIME_TRIAL_DURATION_MS;
                g_hofMode = HOF_TIME_TRIAL;
                NewGame(1);
                return;
            }
        }
        if (KeyDown(K_VK_N) && !KeyDown(K_VK_L_SHIFT)
            && !KeyDown(K_VK_R_SHIFT)) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.numStars += 10.0;

            if (g_cfg.numStars > 3000.0) g_cfg.numStars = 3000.0f;
        }
        if (KeyDown(K_VK_N)
            && (KeyDown(K_VK_L_SHIFT) || KeyDown(K_VK_R_SHIFT))) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.numStars -= 10.0;
            if (g_cfg.numStars < 50.0) g_cfg.numStars = 50.0f;
        }
        if (KeyDown(K_VK_B)) {
            if (g_keyLatch[K_VK_B] != 0) {
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HELP_CONTROLS;
                g_idleTimeoutMs = 15000;

                if (!KeyDown(K_VK_L_SHIFT) && !KeyDown(K_VK_R_SHIFT)
                    && !KeyDown(K_VK_MENU)) {
                    if (g_cfg.borderMode == BORDER_BLACK) {
                        g_cfg.borderMode = BORDER_ON;
                        ImgSetBlitColor(g_gfxBorderEasy, 1, 1, 1, 1);
                        ImgSetBlitColor(g_gfxBorderNormal, 1, 1, 1, 1);
                        ImgSetBlitColor(g_gfxBorderHard, 1, 1, 1, 1);
                        ImgSetBlitColor(g_gfxBorderAce, 1, 1, 1, 1);
                    } else if (g_cfg.borderMode == BORDER_ON) {
                        g_cfg.borderMode = BORDER_OFF;
                        ImgSetBlitColor(g_gfxBorderEasy, 1, 1, 1, 1);
                        ImgSetBlitColor(g_gfxBorderNormal, 1, 1, 1, 1);
                        ImgSetBlitColor(g_gfxBorderHard, 1, 1, 1, 1);
                        ImgSetBlitColor(g_gfxBorderAce, 1, 1, 1, 1);
                    } else {
                        g_cfg.borderMode = BORDER_BLACK;
                        ImgSetBlitColor(g_gfxBorderEasy, 0, 0, 0, 1);
                        ImgSetBlitColor(g_gfxBorderNormal, 0, 0, 0, 1);
                        ImgSetBlitColor(g_gfxBorderHard, 0, 0, 0, 1);
                        ImgSetBlitColor(g_gfxBorderAce, 0, 0, 0, 1);
                    }
                    g_keyLatch[K_VK_B] = 0;
                    PlayClick();
                }

                if (KeyDown(K_VK_L_SHIFT) || KeyDown(K_VK_R_SHIFT)) {
                    PlayClick();
                    switch (g_cfg.bgTint) {
                    case BG_BRIGHTNESS_MIN: g_cfg.bgTint = BG_BRIGHTNESS_DEFAULT; break;
                    case BG_BRIGHTNESS_DEFAULT: g_cfg.bgTint = BG_BRIGHTNESS_PRESET; break;
                    case BG_BRIGHTNESS_PRESET: g_cfg.bgTint = 85; break;
                    case 85: g_cfg.bgTint = 100; break;
                    case 100: g_cfg.bgTint = BG_BRIGHTNESS_MAX; break;
                    case BG_BRIGHTNESS_MAX: g_cfg.bgTint = BG_BRIGHTNESS_MIN; break;
                    default: g_cfg.bgTint = BG_BRIGHTNESS_MIN;
                    }
                    g_keyLatch[K_VK_B] = 0;
                }

                if (KeyDown(K_VK_MENU)) {
                    PlayClick();
                    if (g_cfg.bgEnabled == 0) g_cfg.bgEnabled = 1;
                    else g_cfg.bgEnabled = 0;
                    g_keyLatch[K_VK_B] = 0;
                }
            }
        } else {
            g_keyLatch[K_VK_B] = 1;
        }
        if (KeyDown(K_VK_F) && KeyDown(K_VK_MENU) &&
            (KeyDown(K_VK_L_SHIFT) || KeyDown(K_VK_R_SHIFT))
            && !AnyWindowActive())
            g_showFps = !g_showFps;

        if (!KeyDown(K_VK_MENU)) {
            if (KeyDown(K_VK_M)) {
                if (g_keyLatch[K_VK_M] != 0) {
                    g_lastActivityTime = g_time;
                    g_attractScreen = ATTRACT_HELP_CONTROLS;
                    g_idleTimeoutMs = 15000;
                    CycleMusicFormat();
                    g_keyLatch[K_VK_M] = 0;
                    PlayClick();
                }
            } else {
                g_keyLatch[K_VK_M] = 1;
            }
        } else {

            if (KeyDown(K_VK_M)) {
                if (g_keyLatch[K_VK_M] != 0) {
                    g_lastActivityTime = g_time;
                    g_attractScreen = ATTRACT_HELP_CONTROLS;
                    g_idleTimeoutMs = 15000;
                    g_cfg.shuffle = !g_cfg.shuffle;
                    if (g_profileIndex != -1 && (g_gameMode == 0 || g_gameMode == 6) &&
                        g_playerUpdateFn != StateDemo) {
                        UnpackAccount(g_profileIndex);
                        g_hiscore = g_acc.settings.best;
                        g_acc.settings = g_cfg;

                        if (g_hiscore > g_acc.settings.best) g_acc.settings.best = g_hiscore;
                        PackAccount(g_profileIndex);
                        SaveAccount(g_profileIndex);
                    }
                    g_keyLatch[K_VK_M] = 0;
                    PlayClick();
                }
            } else {
                g_keyLatch[K_VK_M] = 1;
            }
        }
        if (KeyDown(K_VK_U)) {

            if (g_keyLatch[K_VK_U] != 0) {
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HELP_CONTROLS;
                g_idleTimeoutMs = 15000;
                g_cfg.sfxOn = !g_cfg.sfxOn;
                g_keyLatch[K_VK_U] = 0;
                PlayClick();
            }
        } else {
            g_keyLatch[K_VK_U] = 1;
        }

        if (KeyDown(K_VK_W)) {
            if (g_keyLatch[K_VK_W] != 0) {
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HELP_CONTROLS;
                g_idleTimeoutMs = 15000;
                g_cfg.windowed = !g_cfg.windowed;

                SysSetFullscreen(!g_cfg.windowed);

                g_keyLatch[K_VK_W] = 0;
                MergeSettings(g_profileIndex);
                WriteSettings();
                PlayClick();
            }
        } else {
            g_keyLatch[K_VK_W] = 1;
        }

        // -- "S" key: cycle interpolation (before the SDL port it toggled BASS's
        // hardware/software sample mixing) --
        if (KeyDown(K_VK_S)) {
            if (g_keyLatch[K_VK_S] != 0) {
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HELP_CONTROLS;
                g_idleTimeoutMs = 15000;
                CycleInterpolation();
                g_keyLatch[K_VK_S] = 0;
                PlayClick();
            }
        } else {
            g_keyLatch[K_VK_S] = 1;
        }

        // -- ALT + V: toggle vsync (plain V below skips Alt, and resets the shared latch) --
        if (KeyDown(K_VK_MENU) && KeyDown(K_VK_V) && g_keyLatch[K_VK_V] != 0) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            ToggleVSync();
            g_keyLatch[K_VK_V] = 0;
            PlayClick();
        }

        // -- "V" key / buttons 150-151: cycle to the next available announcer voice --
        if (g_clicked == MENUID_VOICE_PACK_NEXT || g_clicked == 151 || KeyDown(K_VK_V)) {
            if (!KeyDown(K_VK_MENU) && g_keyLatch[K_VK_V] != 0) {
                g_keyLatch[K_VK_V] = 0;
                g_lastActivityTime = g_time;
                g_attractScreen = ATTRACT_HELP_CONTROLS;
                g_idleTimeoutMs = 15000;
                PlayClick();
                g_cfg.sfxOn = 1;
                if (g_cfg.sfxOn != 0) {
                    SoundResetQueue();
                    int voiceNo;

                    if (g_profileIndex != -1 && (g_gameMode == 0 || g_gameMode == 6) &&
                        g_playerUpdateFn != StateDemo)
                        voiceNo = GetProfileVoiceIndex(g_profileIndex) + 1;
                    else
                        voiceNo = g_cfg.voice + 1;
                    bool done = false;
                    do {
                        if (VoiceExists(voiceNo)) {

                            if (g_profileIndex != -1 && (g_gameMode == 0 || g_gameMode == 6) &&
                                g_playerUpdateFn != StateDemo)
                                SetProfileVoiceIndex(g_profileIndex, voiceNo);
                            else
                                g_cfg.voice = voiceNo;
                            sprintf(g_alertMsg, "*  L O A D I N G   V O I C E  *  :%d", voiceNo);
                            g_msgColor = 2;
                            g_msgTimer = g_time + 1000;
                            g_frameFunc();
                            FlushBlit(0);
                            FlushStretchF();
                            FlushStretchRot();
                            FlushStretchI();
                            FlushStretchRot2();
                            FlushBlit2(0);
                            FlipBuffer(0);
                            LoadVoices();
                            DoNothing();
                            SoundPlayVoice(g_sfxGetReady, -1, 255, 0.0f, 255, g_sndFlags);
                            done = true;
                            g_voiceSearchTries = 0;
                        } else {
                            voiceNo++;

                            if (voiceNo > 99) {
                                voiceNo = 1;
                                g_voiceSearchTries++;
                                if (g_voiceSearchTries > 3) done = true;
                            }
                        }
                    } while (!done);
                }
            }
        } else {
            g_keyLatch[K_VK_V] = 1;
        }
        if (!KeyDown(K_VK_MENU)) {

            // -- F6: launch the jukebox --
            if (KeyDown(K_VK_F6) && !AnyWindowActive()) {
                if (g_jukeboxKeyLatch != 0) {
                    g_lastActivityTime = g_time;
                    g_attractScreen = ATTRACT_HELP_CONTROLS;
                    g_idleTimeoutMs = 15000;
                    SoundPause();
                    WriteHiscoreFile();
                    ClearHiscores();
                    g_mouseDown = 0;
                    g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                    g_transitionLock = 1;
                    BeforeOpenLink();

                    SysOpenJukebox();
                    g_jukeboxKeyLatch = 0;
                    PlayClick();
                    SysMinimize();
                    return;
                }
            } else {
                g_jukeboxKeyLatch = 1;
            }
        }

        // -- F7: screenshot; F9/F10/F11 and buttons 139/170/171-177: attract screen, version
        //    check, news feed, DirectX/windowed/shuffle toggles, key rebinding --
        if (KeyDown(K_VK_F7)) {
            if (g_screenshotKeyEdge != 0) {
                TakeScreenshot();
                g_screenshotKeyEdge = 0;
                PlayClick();
            }
        } else {
            g_screenshotKeyEdge = 1;
        }
        if (g_clicked == MENUID_CONFIG || KeyDown(K_VK_F9)) {

            if (g_f9KeyLatch != 0) {
                g_cursor = 0;
                g_state = STATE_INPUT_CONFIG;
                g_f9KeyLatch = 0;
            }
        } else {
            g_f9KeyLatch = 1;
        }
        // Cycles auto, DirectX 9, 11, 12, OpenGL, Vulkan; takes effect after a restart.
        if (g_clicked == MENUID_TOGGLE_RENDERER) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.renderer = (unsigned char)(RendererChoiceOf(g_cfg.renderer) + 1);
            if (g_cfg.renderer >= RENDERER_COUNT)
                g_cfg.renderer = RENDERER_AUTO;
            MergeSettings(g_profileIndex);
            WriteSettings();
            PlayClick();
            g_mouseClick = 0;
            g_mouseClickHandled = 1;
            g_mouseDown = 0;
        }

        // Vsync on/off and interpolation auto/on/off take effect at once.
        if (g_clicked == MENUID_TOGGLE_VSYNC || g_clicked == MENUID_TOGGLE_INTERPOLATION) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            if (g_clicked == MENUID_TOGGLE_VSYNC)
                ToggleVSync();
            else
                CycleInterpolation();
            PlayClick();
            g_mouseClick = 0;
            g_mouseClickHandled = 1;
            g_mouseDown = 0;
        }

        if (g_clicked == MENUID_TOGGLE_SHUFFLE) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.shuffle = !g_cfg.shuffle;
            MergeSettings(g_profileIndex);
            WriteSettings();
            PlayClick();
            InitMenu();
            g_mouseClick = 0;
            g_mouseClickHandled = 1;
            g_mouseDown = 0;
        }

        if (g_clicked == MENUID_TOGGLE_INPUT_SWAP) {
            SwapKeyBindings();
            g_mouseClick = 0;
            g_mouseClickHandled = 1;
            g_mouseDown = 0;
        }
        if (g_clicked == MENUID_TOGGLE_WINDOWED) {
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HELP_CONTROLS;
            g_idleTimeoutMs = 15000;
            g_cfg.windowed = !g_cfg.windowed;

            SysSetFullscreen(!g_cfg.windowed);

            MergeSettings(g_profileIndex);
            WriteSettings();
            PlayClick();
            g_mouseClick = 0;
            g_mouseClickHandled = 1;
            g_mouseDown = 0;
        }

        // -- button 555: the online hall of fame (an archived copy of the page) --
        if (g_clicked == MENUID_ONLINE_HALL_OF_FAME) {
            SoundPause();
            PlaySample();
            WinCloseAll();
            BeforeOpenLink();
            OpenUrl("http://www.warblade.as/halloffame.asp?mm=2&D=0&m=0");
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_HALL_OF_FAME;
            g_idleTimeoutMs = 50000;
            g_mouseClick = 0;
            g_mouseClickHandled = 1;
            g_mouseDown = 0;
        }
        // T swaps the input bindings. (Alt+T chose DirectX or OpenGL before the SDL port; it
        // does nothing now.)
        if (KeyDown(K_VK_T)) {
            if (g_keyLatch[K_VK_T] != 0) {
                if (!KeyDown(K_VK_MENU)) {
                    SwapKeyBindings();
                    PlayClick();
                }
                g_keyLatch[K_VK_T] = 0;
            }
        } else {
            g_keyLatch[K_VK_T] = 1;
        }
        // -- right arrow: request an attract-screen advance on key-up --
        if (KeyDown(K_VK_RIGHT)) {
            if (g_save.players[0].keyLatchFire != 0) {

                if (g_resetFlag != 0) {
                    EnsureTitleMusic();
                    g_resetFlag = 0;
                }
                g_titleResetPending = 1;
                g_save.players[0].keyLatchFire = 0;
                PlayClick();
            }
        } else {
            g_save.players[0].keyLatchFire = 1;
        }

        if ((g_clicked == MENUID_DEMO_GAME || g_time > g_menuIdleTimeout) && !KeyDown(K_VK_ESCAPE)) {
            g_pendingGameMode = g_gameMode;
            g_gameMode = MODE_SINGLE;
            g_playerUpdateFn = StateDemo;
            PlayClick();
            g_timeTrialDeadline = g_time + TIME_TRIAL_DURATION_MS;
            NewGame(1);
            return;
        }
    }
    // ---- attract-mode: advance to the next screen once triggered (idle timeout, F9, or
    //      button 16), cycling intro -> about -> mission -> help -> bonuses -> hiscores ----
    if (g_titleResetPending != 0) {
        g_titleResetPending = 0;
        g_lastActivityTime = g_time;
        g_attractScreen++;

        if (g_attractScreen > ATTRACT_HALL_OF_FAME) {
            g_attractScreen = ATTRACT_INTRO;
            g_streakActive = 1;
            g_streakX = 0;
            g_streakAlpha = 0;
            g_streakAlphaStep = 6.375f;

            do {
                g_streakColorR = RandRange(0, 255);
                g_streakColorG = RandRange(0, 255);
                g_streakColorB = RandRange(0, 255);
            } while (g_streakColorR + g_streakColorG + g_streakColorB < 50);
            g_streakSpinSpeed0 = RandFloat(-10.0f, 10.0f);
            g_streakSpinSpeed1 = RandFloat(-10.0f, 10.0f);
            g_streakSpinSpeed2 = RandFloat(-10.0f, 10.0f);
        }
        ClearHiscores();
        g_hiscoreTransitionFlag = 0;
        CLEAR_DRAW_COUNTERS();

        switch (g_attractScreen) {
        case ATTRACT_INTRO:
            g_idleTimeoutMs = 60000;
            g_introInit = 1;
            DoNothing();
            g_frameFunc = IntroFrame;
            ClipCursorOn();
            return;
        case ATTRACT_ABOUT:
            g_idleTimeoutMs = 30000;
            g_frameFunc = AboutScreen;
            return;
        case ATTRACT_MISSION:
            g_idleTimeoutMs = 30000;

            g_frameFunc = MissionScreen;
            return;
        case ATTRACT_HELP_CONTROLS:
            g_idleTimeoutMs = 15000;
            g_frameFunc = HelpControls;
            return;
        case ATTRACT_HELP_BONUSES:
            g_idleTimeoutMs = 15000;
            g_frameFunc = HelpBonuses;
            return;
        case ATTRACT_HALL_OF_FAME:
            g_idleTimeoutMs = 15000;
            g_frameFunc = HallOfFame;
            return;
        }
    }

    // ---- flag a reset once idle too long, then (re)select the current attract screen's frame function ----
    if (g_time - g_lastActivityTime > g_idleTimeoutMs) g_titleResetPending = 1;
    CLEAR_DRAW_COUNTERS();

    switch (g_attractScreen) {
    case ATTRACT_INTRO:
        g_frameFunc = IntroFrame;
        ClipCursorOn();
        break;
    case ATTRACT_ABOUT:
        g_frameFunc = AboutScreen;
        break;
    case ATTRACT_MISSION:
        g_frameFunc = MissionScreen;
        break;
    case ATTRACT_HELP_CONTROLS:
        g_frameFunc = HelpControls;
        break;
    case ATTRACT_HELP_BONUSES:
        g_frameFunc = HelpBonuses;
        break;
    case ATTRACT_HALL_OF_FAME:
        g_frameFunc = HallOfFame;
        break;
    }
}
