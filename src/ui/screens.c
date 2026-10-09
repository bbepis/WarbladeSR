// screens.c: Information screens: about, mission, help, hall of fame, tally/game-over/versus
// backgrounds, the bonus tally screen.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// AboutScreen credit-link ids used with WinAddTextPair.
enum { LINK_EMAIL = 1, LINK_WEB = 2 };

// HelpBonuses/HallOfFame: g_gfxBonus/g_gfxCoins icon cell size, and g_gfxRanks rank-badge
// sprite size.
enum { BONUS_ICON_CELL = 0x14, RANK_ICON_W = 0x40, RANK_ICON_H = 0xd };


// Draws the scrolling credits, play-time summary and clickable name/URL links; a click on a
// credited name opens an info popup (WinOpen), a click on a URL link opens it in the browser.
void AboutScreen()
{
    SysDate sy;
    int months;
    int days;
    int hours;
    int minutes;
    int seconds;
    int r;
    int g;
    int sel;
    bool hover;
    int i;

    // ---- credits text ----
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();

    DrawMenuText("@@ABOUT THE GAME", POS_CENTERED, 0x16, 2);
    Blit((g_screenW >> 1) - 0x90, 4, 0, g_gfxLogos, 0, 0x11e, 0x30, 0x2c);
    Blit((g_screenW >> 1) + 0x62, 4, 0, g_gfxLogos, 0x30, 0x11e, 0x30, 0x2c);

    g_textCursorY = g_textCursorY + 0x22;
    DrawMenuText("WARBLADE SR VERSION 2.0", POS_CENTERED, g_textAutoY, 0);
    g_textCursorY = g_textCursorY + 8;
    DrawTinyText("  CODING, GFX, SFX AND GAME DESIGN  ", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 2;
    DrawTinyText("          @S001EDGAR M. VIGDAL@E           ", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 0xc;

    DrawTinyText("EXTRA GAME DESIGN IDEAS BY", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 2;
    DrawTinyText("@S004SIMON QUINCEY@E, @S003JOSHUA WAYNE@E", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 6;
    DrawTinyText("                                    ", POS_CENTERED, g_textAutoY, 3);

    DrawTinyText("VOCALS BY", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 2;
    DrawTinyText("@S004SIMON QUINCEY@E AND VANESSA QUINCEY", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 3;
    DrawTinyText("@S005CAMREN HARM@E AND ANGELA STRAUSS", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 3;
    DrawTinyText("@S006VIOLETTE BROWN@E AND @S006YANNIS BROWN@E", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 0xe;

    DrawTinyText("MP3 AND MOD MUSIC CREDITS", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 4;
    DrawTinyText("@S006YANNIS BROWN@E", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 9;

    DrawTinyText("OTHER MOD MUSIC CREDITS", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 5;
    DrawTinyText("  MEMORYSTATION MUSIC : DRAX         ", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 1;
    DrawTinyText("  PROMOTED MUSIC      : MANIAC       ", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 1;
    DrawTinyText("  BOSS MUSIC          : XTD - UNION  ", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 1;
    DrawTinyText("  TIMETRIAL MUSIC     : XTD - UNION  ", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 1;
    DrawTinyText("  END MUSIC           : PETER HAJBA  ", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 10;

    DrawTinyText("BETA TESTER", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 4;
    DrawTinyText("@S004SIMON QUINCEY@E, @S017SCOTT BELL@E, @S011ODD RUNE OLSEN@E, ALAN DELAUGHTER",
                 POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 2;
    DrawTinyText("JOHN DRAKE, LISA KIRKHAM, @S016SHARLENE WADE@E, @S003JOSHUA WAYNE@E",
                 POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 2;
    DrawTinyText("ROLAND MEINHARD", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 2;
    g_textCursorY = g_textCursorY + 10;

    DrawTinyText("THANKS TO", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 4;
    DrawTinyText("@S014SHANE R MONROE@E", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 0xd;
    DrawTinyText("AND A BIG THANK YOU TO ALL THAT SUPPORTED ME AND BELIEVED IN ME!", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 10;

    DrawTinyText("DECOMPILER", POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + 4;
    DrawTinyText("@S026BEPIS@E", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 0x10;

    if (g_cfg.playTime < 0)
        g_cfg.playTime = 0;
    SysFileTimeToDate(g_cfg.playTime, &sy);
    months = (sy.month - 1) * 30;
    days = sy.day + months - 1;
    hours = sy.hour;
    minutes = sy.minute;
    seconds = sy.second;

    sprintf(g_logBuf, "TOTAL GAMES PLAYED: %-4d", g_cfg.gamesPlayed);
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6);
    g_textCursorY = g_textCursorY + 3;
    if (minutes == 0 && seconds == 0)
        sprintf(g_logBuf, "NO GAME TIME RECORDED");
    if (minutes == 0 && seconds > 0)
        sprintf(g_logBuf, "TOTAL GAME TIME : %02d SECONDS", seconds);
    if (minutes > 0)
        sprintf(g_logBuf, "TOTAL GAME TIME : %02d MINUTES. %02d SECONDS", minutes, seconds);
    if (hours > 0)
        sprintf(g_logBuf, "TOTAL GAME TIME : %2d HOURS. %02d MINUTES. %02d SECONDS", hours, minutes, seconds);
    if (days > 0)
        sprintf(g_logBuf, "TOTAL GAME TIME : %1d DAYS. %2d HOURS. %02d MINUTES. %02d SECONDS",
                days, hours, minutes, seconds);
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6);
    g_textCursorY = g_textCursorY + 0x14;
    DrawTinyText("@S025THIS VERSION USES THE SDL LIBRARY. CLICK FOR INFO@E", POS_CENTERED, g_textAutoY, 4);
    g_textCursorY = g_textCursorY + 0x1e;

    // ---- link geometry: hover test and underline for each credit-link entry ----
    r = 0;
    g = 200;
    sel = -1;
    hover = false;
    for (i = 0; i < g_linkCount; i++) {
        r = 0;
        g = 200;
        if (g_mouseX > g_links[i].x1 && g_mouseX < g_links[i].x2 &&
            g_mouseY > g_links[i].y1 && g_mouseY < g_links[i].y2 &&
            !AnyWindowActive()) {
            r = 200;
            g = 255;
            hover = true;
            sel = g_links[i].id;
        }

        if (g_mouseDown == 0) {
            FlushBlit(0);
            FlushQuads(0);
            FlushStretchF();
            FlushStretchRot();
            FlushStretchI();
            FlushStretchRot2();
            FlushBlit2(0);
            DrawLine((float)g_links[i].x1, (float)(g_links[i].y2 + 1.0),
                            (float)g_links[i].x2, (float)(g_links[i].y2 + 1.0),
                            0, (float)(r / 255.0), (float)(g / 255.0), 1.0f);
        }
    }
    if (!hover)
        g_linkHover = false;
    if (hover && g_mouseDown != 0 && !g_linkHover)
        g_linkHover = true;

    if (g_linkHover && g_mouseClick != 0) {
        // ---- link click ----
        // Consume the press: after a link minimizes the window, the mouse position is still over
        // the link on restore, and a live latch would follow the link again.
        g_linkHover = false;
        switch (sel) { // sel is the id of the clicked credit-link entry (@Snnn marker in the credits text above)
        case 1: // Edgar Vigdal
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 300, 300, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 0x14, g_curWin, "EDGAR VIGDAL", 4);
            if (g_creditPic != 0) {
                ImgFree(g_creditPic);
                g_creditPic = 0;
            }
            g_creditPic = LoadGraphic("edgar.jpg", true, true);
            WinAddRect(POS_CENTERED, 0x23, 0xbe, 200, g_curWin, g_creditPic);
            WinAddTextPair(0x37, 0xf6, g_curWin, LINK_EMAIL, "EMAIL", "mailto:edgar@warblade.as", 4, 3);
            WinAddTextPair(0xb4, 0xf6, g_curWin, LINK_WEB, "WEB PAGE", "http://www.warblade.as", 4, 3);
            WinAddMenuItem(POS_CENTERED, 0x109, g_curWin, 0xff, "   CLOSE   ", 1);
            WinSetSelected(g_curWin, 0xff);
            break;

        case 15: // no matching @S015 credit link in this screen's text; quits without opening a URL
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_ABOUT;
            g_idleTimeoutMs = 15000;
            WriteHiscoreFile();
            ClearHiscores();
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            BeforeOpenLink();
            SysMinimize();
            break;

        case 18: // no matching @S018 credit link in this screen's text; minimizes only
            SysMinimize();
            break;

        case 8: // no matching @S008 credit link in this screen's text; quits without opening a URL
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_ABOUT;
            g_idleTimeoutMs = 15000;
            WriteHiscoreFile();
            ClearHiscores();
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            BeforeOpenLink();
            SysMinimize();
            break;
            break;

        case 3: // Joshua Wayne
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 300, 300, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 0x14, g_curWin, "JOSHUA WAYNE", 4);
            if (g_creditPic != 0) {
                ImgFree(g_creditPic);
                g_creditPic = 0;
            }
            g_creditPic = LoadGraphic("wayne.jpg", true, true);
            WinAddRect(POS_CENTERED, 0x23, 0xbe, 200, g_curWin, g_creditPic);
            WinAddTextPair(0x37, 0xf6, g_curWin, LINK_EMAIL, "EMAIL", "mailto:firstdraconan@msn.com", 4, 3);
            WinAddTextPair(0xb4, 0xf6, g_curWin, LINK_WEB, "WEB PAGE", "http://karthesios.tripod.com", 4, 3);
            WinAddMenuItem(POS_CENTERED, 0x109, g_curWin, 0xff, "   CLOSE   ", 1);
            WinSetSelected(g_curWin, 0xff);
            break;

        case 4: // Simon Quincey
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 300, 300, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 0x14, g_curWin, "SIMON QUINCEY", 4);
            if (g_creditPic != 0) {
                ImgFree(g_creditPic);
                g_creditPic = 0;
            }
            g_creditPic = LoadGraphic("hitman.jpg", true, true);
            WinAddRect(POS_CENTERED, 0x23, 0xbe, 200, g_curWin, g_creditPic);
            WinAddTextPair(0x37, 0xf6, g_curWin, LINK_EMAIL, "EMAIL", "mailto:hitt.man@ntlworld.com", 4, 3);
            // NOTE: target has a second, unpooled copy of this literal; the explicit NUL escape
            // keeps it a distinct symbol.
            WinAddTextPair(0xb4, 0xf6, g_curWin, LINK_WEB, "WEB PAGE", "http://www.warblade.as\0", 4, 3);
            WinAddMenuItem(POS_CENTERED, 0x109, g_curWin, 0xff, "   CLOSE   ", 1);
            WinSetSelected(g_curWin, 0xff);
            break;

        case 6: // Violette/Yannis Brown (GroovyAudio)
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_ABOUT;
            g_idleTimeoutMs = 15000;
            WriteHiscoreFile();
            ClearHiscores();
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            BeforeOpenLink();
            OpenUrl("http://www.GroovyAudio.com");
            SysMinimize();
            break;

        case 11: // Odd Rune Olsen
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 300, 300, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 0x14, g_curWin, "ODD RUNE OLSEN", 4);
            if (g_creditPic != 0) {
                ImgFree(g_creditPic);
                g_creditPic = 0;
            }
            g_creditPic = LoadGraphic("olsen.jpg", true, true);
            WinAddRect(POS_CENTERED, 0x23, 0xbe, 200, g_curWin, g_creditPic);
            WinAddTextPair(0x37, 0xf6, g_curWin, LINK_EMAIL, "EMAIL", "mailto:olsodd@hotmail.com", 4, 3);
            WinAddMenuItem(POS_CENTERED, 0x109, g_curWin, 0xff, "   CLOSE   ", 1);
            WinSetSelected(g_curWin, 0xff);
            break;

        case 16: // Sharlene Wade
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 300, 300, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 0x14, g_curWin, "SHARLENE A WADE", 4);
            if (g_creditPic != 0) {
                ImgFree(g_creditPic);
                g_creditPic = 0;
            }
            g_creditPic = LoadGraphic("wade.jpg", true, true);
            WinAddRect(POS_CENTERED, 0x23, 0xbe, 200, g_curWin, g_creditPic);
            WinAddTextPair(0x37, 0xf6, g_curWin, LINK_EMAIL, "EMAIL", "mailto:birds110@msn.com", 4, 3);
            WinAddMenuItem(POS_CENTERED, 0x109, g_curWin, 0xff, "   CLOSE   ", 1);
            WinSetSelected(g_curWin, 0xff);
            break;

        case 17: // Scott Bell
            g_curWin = WinOpen(POS_CENTERED, POS_CENTERED, 300, 300, WIN_MODE_SLIDING);
            WinAddText(POS_CENTERED, 0x14, g_curWin, "SCOTT BELL", 4);
            if (g_creditPic != 0) {
                ImgFree(g_creditPic);
                g_creditPic = 0;
            }
            g_creditPic = LoadGraphic("bell.jpg", true, true);
            WinAddRect(POS_CENTERED, 0x23, 0xbe, 200, g_curWin, g_creditPic);
            WinAddTextPair(0x37, 0xf6, g_curWin, LINK_EMAIL, "EMAIL", "mailto:scott@sbelectronics.com.au", 4, 3);
            WinAddTextPair(0xb4, 0xf6, g_curWin, LINK_WEB, "WEB PAGE", "http://www.sbelectronics.com.au", 4, 3);
            WinAddMenuItem(POS_CENTERED, 0x109, g_curWin, 0xff, "   CLOSE   ", 1);
            WinSetSelected(g_curWin, 0xff);
            break;

        case 25: // engine credit (the PTK credit before the SDL port)
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_ABOUT;
            g_idleTimeoutMs = 15000;
            WriteHiscoreFile();
            ClearHiscores();
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            BeforeOpenLink();
            OpenUrl("https://www.libsdl.org/");
            SysMinimize();
            break;

        case 26: // decompiler credit
            SoundPause();
            g_lastActivityTime = g_time;
            g_attractScreen = ATTRACT_ABOUT;
            g_idleTimeoutMs = 15000;
            WriteHiscoreFile();
            ClearHiscores();
            g_mouseDown = 0;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            BeforeOpenLink();
            OpenUrl("https://github.com/bbepis/WarbladeSR");
            SysMinimize();
            break;
        }
    }

    DrawMenuPrompt();
    HidePageButtons(1);
    DrawButtons(0);
}

// Draws the static "THE MISSION" instructions page.
void MissionScreen()
{
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    if (g_resetFlag != 0) {
        EnsureTitleMusic();
        g_resetFlag = 0;
    }
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawMenuText("THE MISSION", POS_CENTERED, 0x16, 2);
    g_textCursorY = g_textCursorY + 0x28;
    Blit((g_screenW >> 1) - 0x7b, 4, 0, g_gfxLogos, 0, 0x11e, 0x30, 0x2c);
    Blit((g_screenW >> 1) + 0x4d, 4, 0, g_gfxLogos, 0x30, 0x11e, 0x30, 0x2c);

    DrawMenuText("GUIDE YOUR SHIP THROUGH WAVES OF   ", POS_CENTERED, g_textAutoY, 1);
    DrawMenuText("ALIEN ATTACKERS. SHOOT AND DESTROY ", POS_CENTERED, g_textAutoY, 1);
    DrawMenuText("ALL ENEMIES, LOOK OUT FOR KAMIKAZE ", POS_CENTERED, g_textAutoY, 1);
    DrawMenuText("ATTACKERS AND METEOR STORMS.       ", POS_CENTERED, g_textAutoY, 1);
    g_textCursorY = g_textCursorY + 10;

    DrawMenuText("GET EXTRA WEAPONS FOR A BETTER SHIP", POS_CENTERED, g_textAutoY, 1);
    DrawMenuText("AND A HIGHER SCORE.                ", POS_CENTERED, g_textAutoY, 1);
    g_textCursorY = g_textCursorY + 10;

    DrawMenuText("LOOK OUT FOR ENEMY SECRETS AND TRY ", POS_CENTERED, g_textAutoY, 1);
    DrawMenuText("AND FIND ALL THE HIDDEN BONUSES.   ", POS_CENTERED, g_textAutoY, 1);
    g_textCursorY = g_textCursorY + 10;

    DrawMenuText("HAVE FUN.....                      ", POS_CENTERED, g_textAutoY, 1);
    DrawMenuPrompt();
    if (g_debug != 0)
        DrawTextBox(100, 100, 500, 500, " ");
    HidePageButtons(1);
    DrawButtons(0);
}

// Draws the controls/options help page: key bindings and the current value of every
// user-configurable option, plus (when active) the news ticker and estimated alien-graphics
// memory usage.
void HelpControls()
{
    int unused1 = 0x5a;
    int unused2 = 300;
    int lineStep = 3;
    int voice;
    int bytesPerPixel = 4;  // textures are 32-bit
    int minMem;
    int maxMem;

    if (g_resetFlag != 0) {
        EnsureTitleMusic();
        g_resetFlag = 0;
    }
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawMenuText("GAME CONTROLS", POS_CENTERED, 0x16, 2);
    Blit((g_screenW >> 1) - 0x89, 4, 0, g_gfxLogos, 0, 0x11e, 0x30, 0x2c);
    Blit((g_screenW >> 1) + 0x5b, 4, 0, g_gfxLogos, 0x30, 0x11e, 0x30, 0x2c);

    g_textCursorY = g_textCursorY + 0x19;
    // ---- F1/F2/F3/F5: start game modes ----
    DrawTinyText("F1           START 1 PLAYER GAME           ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("  ...........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;
    DrawTinyText("F2           START 2 PLAYERS GAME          ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("  ...........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;
    DrawTinyText("F3           START 2 PLAYERS DUEL GAME     ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("  ...........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;
    g_textCursorY = g_textCursorY + lineStep + 10;
    DrawTinyText("F5           START TIME TRIAL GAME         ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("  ...........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + 0x10;

    // ---- 1-4: difficulty ----
    if (g_cfg.difficulty == DIFF_EASY)
        DrawTinyText("1 2 3 4      SELECT DIFFICULTY      :EASY  ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.difficulty == DIFF_NORMAL)
        DrawTinyText("1 2 3 4      SELECT DIFFICULTY      :NORMAL", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.difficulty == DIFF_HARD)
        DrawTinyText("1 2 3 4      SELECT DIFFICULTY      :HARD  ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.difficulty == DIFF_ACE)
        DrawTinyText("1 2 3 4      SELECT DIFFICULTY      :ACE   ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" . . . ......                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- P: pause ----
    DrawTinyText("P            PAUSE GAME                    ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- M: music mode ----
    if (g_cfg.musicFormat == MUSIC_FMT_MOD)
        DrawTinyText("  M            MUSIC MODE             :MOD     ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.musicFormat == MUSIC_FMT_MP3)
        DrawTinyText("  M            MUSIC MODE             :MP3     ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.musicFormat == MUSIC_FMT_PLAYLIST)
        DrawTinyText("  M            MUSIC MODE             :PLAYLIST", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- +/-, PGUP/PGDN, HOME/END: volumes ----
    sprintf(g_logBuf, "+ -          MUSIC VOLUME           :%-3d   ", (int)(g_cfg.musicVolume / 2.55));
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" . ..........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    sprintf(g_logBuf, "PGUP PGDN    EFFECT VOLUME          :%-3d   ", (int)(g_cfg.sfxVol / 2.55));
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("    .    ....                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    sprintf(g_logBuf, "HOME END     VOICE VOLUME           :%-3d   ", (int)(g_cfg.musicVol / 2.55));
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("    .   .....                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- U/V: voice messages ----
    if (g_cfg.sfxOn != 0)
        DrawTinyText("U            USE VOICE MESSAGES     :YES   ", POS_CENTERED, g_textAutoY, 3);
    else
        DrawTinyText("U            USE VOICE MESSAGES     :NO    ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    voice = g_cfg.voice;
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo)
        voice = GetProfileVoiceIndex(g_profileIndex);
    sprintf(g_logBuf, "V            SELECT NEXT VOICE      :%-2d    ", voice);
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- B: scrolling border ----
    if (g_cfg.borderMode == BORDER_ON) {
        DrawTinyText("B            SCROLLING BORDER       :ON    ", POS_CENTERED, g_textAutoY, 3);
        ImgSetBlitColor(g_gfxBorderEasy, 1.0f, 1.0f, 1.0f, 1.0f);
        ImgSetBlitColor(g_gfxBorderNormal, 1.0f, 1.0f, 1.0f, 1.0f);
        ImgSetBlitColor(g_gfxBorderHard, 1.0f, 1.0f, 1.0f, 1.0f);
        ImgSetBlitColor(g_gfxBorderAce, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (g_cfg.borderMode == BORDER_OFF) {
        DrawTinyText("B            SCROLLING BORDER       :OFF   ", POS_CENTERED, g_textAutoY, 3);
        ImgSetBlitColor(g_gfxBorderEasy, 1.0f, 1.0f, 1.0f, 1.0f);
        ImgSetBlitColor(g_gfxBorderNormal, 1.0f, 1.0f, 1.0f, 1.0f);
        ImgSetBlitColor(g_gfxBorderHard, 1.0f, 1.0f, 1.0f, 1.0f);
        ImgSetBlitColor(g_gfxBorderAce, 1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (g_cfg.borderMode == BORDER_BLACK) {
        DrawTinyText("B            SCROLLING BORDER       :BLACK ", POS_CENTERED, g_textAutoY, 3);
        ImgSetBlitColor(g_gfxBorderEasy, 0, 0, 0, 1.0f);
        ImgSetBlitColor(g_gfxBorderNormal, 0, 0, 0, 1.0f);
        ImgSetBlitColor(g_gfxBorderHard, 0, 0, 0, 1.0f);
        ImgSetBlitColor(g_gfxBorderAce, 0, 0, 0, 1.0f);
    }
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- ALT+B / SHIFT+B: background on/off and brightness ----
    if (g_cfg.bgEnabled == 0)
        DrawTinyText("ALT + B      BACKGROUND             :OFF   ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bgEnabled == 1)
        DrawTinyText("ALT + B      BACKGROUND             :ON    ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("   . . ......                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // g_cfg.bgTint's 6-step brightness cycle (BG_BRIGHTNESS_MIN..MAX by BG_BRIGHTNESS_STEP);
    // 0x55/0x64 are the two middle steps and have no shared name.
    if (g_cfg.bgTint == BG_BRIGHTNESS_MIN)
        DrawTinyText("SHIFT + B    BACKGROUND BRIGHTNESS  :1     ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bgTint == BG_BRIGHTNESS_DEFAULT)
        DrawTinyText("SHIFT + B    BACKGROUND BRIGHTNESS  :2     ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bgTint == BG_BRIGHTNESS_PRESET)
        DrawTinyText("SHIFT + B    BACKGROUND BRIGHTNESS  :3     ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bgTint == 0x55)
        DrawTinyText("SHIFT + B    BACKGROUND BRIGHTNESS  :4     ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bgTint == 0x64)
        DrawTinyText("SHIFT + B    BACKGROUND BRIGHTNESS  :5     ", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bgTint == BG_BRIGHTNESS_MAX)
        DrawTinyText("SHIFT + B    BACKGROUND BRIGHTNESS  :6     ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("     . . ....                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- N: star count ----
    sprintf(g_logBuf, "N            NUMBER OF STARS        :%-4d  ", (int)g_cfg.numStars);
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- C: collision detection ----
    if (g_cfg.collisionDetail == COLLISION_SIMPLE)
        DrawTinyText("C            COLLISION DETECTION    :SIMPLE", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.collisionDetail == COLLISION_NORMAL)
        DrawTinyText("C            COLLISION DETECTION    :NORMAL", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- F: flare effects ----
    if (g_cfg.particlesOn != 0)
        DrawTinyText("F            FLARE EFFECTS          :ON    ", POS_CENTERED, g_textAutoY, 3);
    else
        DrawTinyText("F            FLARE EFFECTS          :OFF   ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- E: explosion sparks ----
    sprintf(g_logBuf, "E            EXPLOSION SPARKS       :%-4d  ", (int)g_cfg.sparks);
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- I: bullet intensity ----
    if (g_cfg.bulletIntensity == BULLETS_NORMAL)
        DrawTinyText("I            BULLET INTENSITY       :NORMAL", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bulletIntensity == BULLETS_BRIGHT)
        DrawTinyText("I            BULLET INTENSITY       :BRIGHT", POS_CENTERED, g_textAutoY, 3);
    if (g_cfg.bulletIntensity == BULLETS_FLARE_FX)
        DrawTinyText("  I            BULLET INTENSITY       :FLARE FX", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("   ............                                ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- Z: background stars style ----
    if (g_cfg.bgStars != 0)
        DrawTinyText("Z            BACKGROUND STARS       :POINTS", POS_CENTERED, g_textAutoY, 3);
    else
        DrawTinyText("Z            BACKGROUND STARS       :FLARES", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- alien graphics buffer size ----
    sprintf(g_logBuf, "ALIEN GRAPHICS TO BUFFER IN MEMORY  :%-4d  ", g_cfg.alienBuffer);
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- F6/F7/F9: jukebox, screenshot, config ----
    DrawTinyText("F6           START JUKEBOX - NEXT SONG     ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("  ...........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;
    DrawTinyText("F7           SAVE SCREENSHOT               ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("  ...........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;
    DrawTinyText("F9           CONFIG INPUT DEVICES          ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("  ...........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- renderer (the original's ALT+T DirectX/OpenGL line; set with the renderer button beside it) ----
    {
        static const char *const values[RENDERER_COUNT] = {
            "AUTO    ", "AUTO    ", "DX9     ", "DX11    ", "DX12    ", "OPENGL  ", "VULKAN  "
        };
        sprintf(g_logBuf, "               RENDERER               :%s", values[RendererChoiceOf(g_cfg.renderer)]);
        DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
        g_textCursorY = g_textCursorY + lineStep;
    }

    // ---- vsync and interpolation (SDL port; set with the buttons beside them, or ALT + V
    // and S) ----
    sprintf(g_logBuf, "  ALT + V      VSYNC                  :%s", g_cfg.vsyncOff == 1 ? "OFF     " : "ON      ");
    DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("     . . ......                                ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;
    {
        const char *mode = "AUTO";
        if (g_cfg.interpolation == SYS_INTERP_ON)
            mode = "ON";
        else if (g_cfg.interpolation == SYS_INTERP_OFF)
            mode = "OFF";
        // Auto also shows what it chose for this display.
        sprintf(g_logBuf, "  S            INTERPOLATION          :%-4s%-4s", mode,
                g_cfg.interpolation == SYS_INTERP_ON || g_cfg.interpolation == SYS_INTERP_OFF ? ""
                    : SysInterpolating() ? " ON" : " OFF");
        DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 3);
        DrawTinyText("   ............                                ", POS_CENTERED, g_curY, 2);
        g_textCursorY = g_textCursorY + lineStep;
    }

    // ---- ALT+M: song play mode ----
    if (g_cfg.shuffle)
        DrawTinyText("  ALT + M      SONG PLAY MODE         :RANDOM  ", POS_CENTERED, g_textAutoY, 3);
    else
        DrawTinyText("  ALT + M      SONG PLAY MODE         :SEQUENCE", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("     . . ......                                ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- A/ESC: profiles, quit ----
    DrawTinyText("A            SHOW USER PROFILES            ", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;
    DrawTinyText("ESC          QUIT CURRENT GAME OR QUIT GAME", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText("   ..........                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- T: input device ----
    if (g_cfg.device0 == DEVICE_KEYBOARD)
        DrawTinyText("  T            TOGGLE INPUT DEVICE    :KEYBOARD", POS_CENTERED, g_textAutoY, 3);
    else
        DrawTinyText("  T            TOGGLE INPUT DEVICE    :JOYSTICK", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    g_textCursorY = g_textCursorY + lineStep;

    // ---- W: screen mode ----
    if (g_cfg.windowed)
        DrawTinyText("  W            TOGGLE SCREEN MODE     :WINDOW  ", POS_CENTERED, g_textAutoY, 3);
    else
        DrawTinyText("    W            TOGGLE SCREEN MODE     :FULLSCREEN", POS_CENTERED, g_textAutoY, 3);
    DrawTinyText(" ............                              ", POS_CENTERED, g_curY, 2);
    // (The last line was "S  TOGGLE SOUNDMIXER : HARDWARE/SOFTWARE", BASS's sample mixing,
    // before the SDL port.)

    // ---- current control bindings banner (after a swap) ----
    if (g_time < g_bindingsBannerUntil) {
        g_textCursorY = g_textCursorY + 8;
        ControlsText(0);
        DrawNewsText(g_logBuf, POS_CENTERED, g_textAutoY, 11);
        g_textCursorY = g_textCursorY + 1;
        ControlsText(1);
        DrawNewsText(g_logBuf, POS_CENTERED, g_textAutoY, 7);
    }

    // ---- estimated alien-graphics memory usage ----
    if (g_time < g_memUntil) {
        g_textCursorY = g_textCursorY + 10;
        // rough min/max estimate: alienBuffer frames at a small (576x96) vs. full (1024x1024)
        // sprite sheet size
        minMem = g_cfg.alienBuffer * 576 * 96 * bytesPerPixel * 2 * 6 + g_cfg.alienBuffer * 576 * 96 * 6;
        maxMem = g_cfg.alienBuffer * 1024 * 1024 * bytesPerPixel * 2 + g_cfg.alienBuffer * 576 * 96;
        Int64ToStrGrouped(minMem, g_logBuf);
        Int64ToStrGrouped(maxMem, g_buf);
        sprintf(g_songPath, "APPROXIMATELY MEMORY NEEDED   MIN:%s BYTES   MAX:%s BYTES", g_logBuf, g_buf);
        DrawNewsText(g_songPath, POS_CENTERED, g_textAutoY, 11);
    }

    DrawMenuPrompt();
    DrawButtons(1);
    DrawButtons(0);
}

// Draws the bonus/weapon/coin/rank legend, animating each icon through g_gfxBonus's frames.
void HelpBonuses()
{
    int x = 0xd2;
    int textX = 0x186;
    int unused = 5;
    int lineStep = 8;

    if (g_resetFlag != 0) {
        EnsureTitleMusic();
        g_resetFlag = 0;
    }
    if (g_time - g_lastTick > g_animInterval) {
        g_lastTick = g_time;
        g_animFrame++;
        if (g_animFrame > 9)
            g_animFrame = 0;
    }
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();

    DrawMenuText("BONUSES AND WEAPONS", POS_CENTERED, 0x16, 2);
    Blit((g_screenW >> 1) - 0xb3, 4, 0, g_gfxLogos, 0, 0x11e, 0x30, 0x2c);
    Blit((g_screenW >> 1) + 0x85, 4, 0, g_gfxLogos, 0x30, 0x11e, 0x30, 0x2c);

    g_textCursorY = g_textCursorY + 0x1e;
    DrawMenuText("EXTRA LIFE", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x3c, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x16, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x50, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x2c, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x64, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x42, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x78, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x58, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x8c, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("EXTRA BULLET", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x118, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("EXTRA SPEED", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x12c, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("SHIELD", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x28, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("WEAPONS", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x154, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x16, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x140, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x2c, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x168, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x42, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x190, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("ALIEN SCOOP", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x17c, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("SMART BOMB", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x1a4, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x16, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x1b8, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("MULTIPLY SCORE", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0xa0, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x16, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0xb4, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("METEOR STORM", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("RANDOM BONUS", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x14, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("DECREASE STRENGTH", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x208, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x16, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x21c, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x2c, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x230, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("EXTRA BONUS TIME", textX, g_textAutoY, 1);
    Blit(x, g_curY - 3, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x1e0, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + lineStep;
    DrawMenuText("NEW RANK", textX, g_textAutoY, 1);
    Blit(x, g_curY - 4, 0, g_gfxCoins, g_animFrame * BONUS_ICON_CELL, 0, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x18, g_curY - 4, 0, g_gfxCoins, g_animFrame * BONUS_ICON_CELL, 0x14, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x30, g_curY - 4, 0, g_gfxCoins, g_animFrame * BONUS_ICON_CELL, 0x28, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x48, g_curY - 4, 0, g_gfxCoins, g_animFrame * BONUS_ICON_CELL, 0x3c, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x60, g_curY - 4, 0, g_gfxCoins, g_animFrame * BONUS_ICON_CELL, 0x50, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit(x + 0x78, g_curY - 4, 0, g_gfxCoins, g_animFrame * BONUS_ICON_CELL, 0x64, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + 0x28;
    DrawMenuText("THE COINS HAVE THESE VALUES", POS_CENTERED, g_textAutoY, 1);
    g_textCursorY = g_textCursorY + 0x23;
    DrawMenuText("$10", (g_screenW >> 1) - 0xaa, g_textAutoY, 1);
    DrawMenuText("$50", (g_screenW >> 1) - 0x46, g_curY, 1);
    DrawMenuText("$100", (g_screenW >> 1) + 0x1e, g_curY, 1);
    DrawMenuText("$200", (g_screenW >> 1) + 0x82, g_curY, 1);
    Blit((g_screenW >> 1) - 0xa2, g_curY - 0x19, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x258, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit((g_screenW >> 1) - 0x3e, g_curY - 0x19, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x244, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit((g_screenW >> 1) + 0x2b, g_curY - 0x19, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x26c, BONUS_ICON_CELL, BONUS_ICON_CELL);
    Blit((g_screenW >> 1) + 0x8f, g_curY - 0x19, 0, g_gfxBonus, g_animFrame * BONUS_ICON_CELL, 0x280, BONUS_ICON_CELL, BONUS_ICON_CELL);

    g_textCursorY = g_textCursorY + 0x28;
    DrawMenuText("THE RANK MARKERS", POS_CENTERED, g_textAutoY, 1);
    g_textCursorY = g_textCursorY + 0x1e;
    DrawMenuText("ENSIGN", (g_screenW >> 1) - 0x127, g_textAutoY, 1);
    DrawMenuText("LIEUTENANT", (g_screenW >> 1) - 0xc1, g_curY, 1);
    DrawMenuText("COMMANDER", (g_screenW >> 1) - 0x32, g_curY, 1);
    DrawMenuText("CAPTAIN", (g_screenW >> 1) + 0x67, g_curY, 1);
    DrawMenuText("ADMIRAL", (g_screenW >> 1) + 0xe6, g_curY, 1);
    Blit((g_screenW >> 1) - 0x124, g_curY - 0x14, 0, g_gfxRanks, 0, 0, RANK_ICON_W, RANK_ICON_H);
    Blit((g_screenW >> 1) - 0xa7, g_curY - 0x14, 0, g_gfxRanks, 0, 0xd, RANK_ICON_W, RANK_ICON_H);
    Blit((g_screenW >> 1) - 0x20, g_curY - 0x14, 0, g_gfxRanks, 0, 0x1a, RANK_ICON_W, RANK_ICON_H);
    Blit((g_screenW >> 1) + 0x6f, g_curY - 0x14, 0, g_gfxRanks, 0, 0x27, RANK_ICON_W, RANK_ICON_H);
    Blit((g_screenW >> 1) + 0xec, g_curY - 0x14, 0, g_gfxRanks, 0, 0x34, RANK_ICON_W, RANK_ICON_H);

    g_textCursorY = g_textCursorY + 10;
    DrawTinyText("THERE MAY BE MORE BONUSES AND RANKS OUT THERE....", POS_CENTERED, g_textAutoY, 3);
    DrawMenuPrompt();
    if (g_debug != 0)
        DrawTextBox(100, 100, 500, 500, " ");
    HidePageButtons(1);
    DrawButtons(0);
}

// Draws the top-20 hiscore table for the selected mode (g_hofMode: 0-3 difficulty, 4
// meteorstorm, 5 time trial), each row's rank icon and (for ranks with a row of pips) extra
// pip graphics, then, if the mouse is held over a row, that entry's detail line (date,
// shots/hits/hit rate, duration, rank name).
// One row of a HOF per-difficulty table: score list line, rank icon, and (for ranks 5-13
// and 18-20) a small pip row via DrawRow.
#define DRAW_HOF_TABLE_ROW(tbl) \
    Int64ToStrGrouped(g_hiscoreMagic.table[tbl][i].score, g_scoreBuf);\
    sprintf(g_logBuf, "%-30s  %13s  %4d        ",\
            g_hiscoreMagic.table[tbl][i].name, g_scoreBuf, g_hiscoreMagic.table[tbl][i].level);\
    if (g_hiscoreMagic.table[tbl][i].highlight != 0) {\
        DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 2);\
        g_textCursorY = g_textCursorY + 5;\
    } else {\
        DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 1);\
        g_textCursorY = g_textCursorY + 5;\
    }\
    Blit(x, g_curY - 1, 0, g_gfxRanks,\
         0, g_rankSprY[g_hiscoreMagic.table[tbl][i].rank], RANK_ICON_W, RANK_ICON_H);\
    int rank = g_hiscoreMagic.table[tbl][i].rank;\
    if (rank == RANK_ADMIRAL_1_1)\
        DrawRow(x + 0x43, g_curY - 2, 0, 1);\
    if (rank == RANK_ADMIRAL_1_2)\
        DrawRow(x + 0x43, g_curY - 2, 0, 2);\
    if (rank == RANK_ADMIRAL_1_3)\
        DrawRow(x + 0x43, g_curY - 2, 0, 3);\
    if (rank == RANK_ADMIRAL_2_1)\
        DrawRow(x + 0x43, g_curY - 2, 1, 1);\
    if (rank == RANK_ADMIRAL_2_2)\
        DrawRow(x + 0x43, g_curY - 2, 1, 2);\
    if (rank == RANK_ADMIRAL_2_3)\
        DrawRow(x + 0x43, g_curY - 2, 1, 3);\
    if (rank == RANK_ADMIRAL_3_1)\
        DrawRow(x + 0x43, g_curY - 2, 2, 1);\
    if (rank == RANK_ADMIRAL_3_2)\
        DrawRow(x + 0x43, g_curY - 2, 2, 2);\
    if (rank == RANK_ADMIRAL_3_3)\
        DrawRow(x + 0x43, g_curY - 2, 2, 3);\
    if (rank == RANK_GRANDMASTER_1)\
        DrawRow(x + 0x43, g_curY - 2, 2, 1);\
    if (rank == RANK_GRANDMASTER_2)\
        DrawRow(x + 0x43, g_curY - 2, 2, 2);\
    if (rank == RANK_GRANDMASTER_3)\
        DrawRow(x + 0x43, g_curY - 2, 2, 3);

void HallOfFame()
{
    int x = 0x29e;
    SysDate sy;
    int hours;
    int minutes;
    int seconds;

    if (g_pendingGameMode != -1) {
        g_gameMode = g_pendingGameMode;
        g_pendingGameMode = -1;
    }
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();

    DrawMenuText("HALL OF FAME", POS_CENTERED, 0x16, 2);
    Blit((g_screenW >> 1) - 0x82, 4, 0, g_gfxLogos, 0, 0x11e, 0x30, 0x2c);
    Blit((g_screenW >> 1) + 0x54, 4, 0, g_gfxLogos, 0x30, 0x11e, 0x30, 0x2c);
    g_textCursorY = g_textCursorY + 0x1e;

    // ---- mode label ----
    if (g_hofMode == HOF_EASY)
        DrawMenuText("EASY GAME MODE", POS_CENTERED, g_textAutoY, 5);
    if (g_hofMode == HOF_NORMAL)
        DrawMenuText("NORMAL GAME MODE", POS_CENTERED, g_textAutoY, 5);
    if (g_hofMode == HOF_HARD)
        DrawMenuText("HARD GAME MODE", POS_CENTERED, g_textAutoY, 5);
    if (g_hofMode == HOF_ACE)
        DrawMenuText("ACE GAME MODE", POS_CENTERED, g_textAutoY, 5);
    if (g_hofMode == HOF_METEORSTORM)
        DrawMenuText("METEORSTORM", POS_CENTERED, g_textAutoY, 5);
    if (g_hofMode == HOF_TIME_TRIAL)
        DrawMenuText("TIME TRIAL", POS_CENTERED, g_textAutoY, 5);
    g_textCursorY = g_textCursorY + 0xf;

    // ---- per-difficulty tables ----
    DecompressHiscores();
    if (g_hofMode == HOF_EASY) {
        for (int i = 0; i < MAX_HISCORES; i++) {
            DRAW_HOF_TABLE_ROW(0)
        }
    }

    if (g_hofMode == HOF_NORMAL) {
        for (int i = 0; i < MAX_HISCORES; i++) {
            DRAW_HOF_TABLE_ROW(1)
        }
    }

    if (g_hofMode == HOF_HARD) {
        for (int i = 0; i < MAX_HISCORES; i++) {
            DRAW_HOF_TABLE_ROW(2)
        }
    }

    if (g_hofMode == HOF_ACE) {
        for (int i = 0; i < MAX_HISCORES; i++) {
            DRAW_HOF_TABLE_ROW(3)
        }
    }

    if (g_hofMode == HOF_METEORSTORM) {
        for (int i = 0; i < MAX_HISCORES; i++) {
            DRAW_HOF_TABLE_ROW(4)
        }
    }

    if (g_hofMode == HOF_TIME_TRIAL) {
        for (int i = 0; i < MAX_HISCORES; i++) {
            Int64ToStrGrouped(g_hiscoreMagic.table5[i].score, g_scoreBuf);
            sprintf(g_logBuf, "%-30s  %13s", g_hiscoreMagic.table5[i].name, g_scoreBuf);
            if (g_hiscoreMagic.table5[i].highlight != 0) {
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 2);
                g_textCursorY = g_textCursorY + 5;
            } else {
                DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 1);
                g_textCursorY = g_textCursorY + 5;
            }
        }
    }

    ClearHiscores();
    g_textCursorY = g_textCursorY + 0x14;

    // ---- row hit test ----
    int sel = -1;
    // 0x11 px row height; hovering the score list picks the entry under the mouse
    if (g_mouseDown != 0 && !AnyWindowActive() && g_mouseY > 0x57 && g_mouseY < 0x1ab)
        sel = (g_mouseY - 0x57) / 0x11;
    // Detail line for the row under the mouse: date, shots/hits/hit rate, duration,
    // rank name.
#define DRAW_HOF_ROW_DETAIL(mode, tbl) \
    if (g_hofMode == mode && g_hiscoreMagic.table[tbl][sel].date.year != 0) { \
        sprintf(g_logBuf, "DATE OF RECORDING: %0d %0d %d", \
                g_hiscoreMagic.table[tbl][sel].date.day, g_hiscoreMagic.table[tbl][sel].date.month, \
                g_hiscoreMagic.table[tbl][sel].date.year); \
        DrawTinyText(g_logBuf, POS_CENTERED, 0x1b8, 1); \
        g_textCursorY = g_textCursorY + 2; \
        float shots = (float)g_hiscoreMagic.table[tbl][sel].shots; \
        if (shots == 0.0) \
            shots = 1; \
        int rate = (int)(g_hiscoreMagic.table[tbl][sel].hits / shots * 100.0) > 100 \
                       ? 100 \
                       : (int)(g_hiscoreMagic.table[tbl][sel].hits / shots * 100.0); \
        sprintf(g_logBuf, "NUMBER OF BULLETS FIRED: %d    NUMBER OF HITS: %d   HIT RATE: %d%%", \
                g_hiscoreMagic.table[tbl][sel].shots, g_hiscoreMagic.table[tbl][sel].hits, rate); \
        DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 1); \
        g_textCursorY = g_textCursorY + 2; \
        SysFileTimeToDate(g_hiscoreMagic.table[tbl][sel].duration, &sy); \
        hours = sy.hour; \
        minutes = sy.minute; \
        seconds = sy.second; \
        sprintf(g_logBuf, "GAME LASTED: %d HOURS, %d MINUTES, %d SECONDS", hours, minutes, seconds); \
        DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 1); \
        sprintf(g_logBuf, "RANK : %s", g_rankNames[g_hiscoreMagic.table[tbl][sel].rank]); \
        DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 1); \
    }

    if (sel != -1) {
        DecompressHiscores();
        DRAW_HOF_ROW_DETAIL(HOF_EASY, 0)

        DRAW_HOF_ROW_DETAIL(HOF_NORMAL, 1)

        DRAW_HOF_ROW_DETAIL(HOF_HARD, 2)

        DRAW_HOF_ROW_DETAIL(HOF_ACE, 3)

        if (g_hofMode == HOF_TIME_TRIAL && g_hiscoreMagic.table5[sel].date.year != 0) {
            sprintf(g_logBuf, "DATE OF RECORDING: %0d %0d %d",
                    g_hiscoreMagic.table5[sel].date.day, g_hiscoreMagic.table5[sel].date.month,
                    g_hiscoreMagic.table5[sel].date.year);
            DrawTinyText(g_logBuf, POS_CENTERED, 0x1b8, 1);
            g_textCursorY = g_textCursorY + 2;
            float shots = (float)g_hiscoreMagic.table5[sel].shots;
            if (shots == 0.0)
                shots = 1;
            int rate = (int)(g_hiscoreMagic.table5[sel].hits / shots * 100.0) > 100
                           ? 100
                           : (int)(g_hiscoreMagic.table5[sel].hits / shots * 100.0);
            sprintf(g_logBuf, "NUMBER OF BULLETS FIRED: %d    NUMBER OF HITS: %d   HIT RATE: %d%%",
                    g_hiscoreMagic.table5[sel].shots, g_hiscoreMagic.table5[sel].hits, rate);
            DrawTinyText(g_logBuf, POS_CENTERED, g_textAutoY, 1);
        }
        ClearHiscores();
    }

#undef DRAW_HOF_ROW_DETAIL

    // ---- footer ----
    DrawTinyText("PRESS UP OR DOWN TO CHANGE HALL OF FAME", POS_CENTERED, 0x1e8, 3);
    DrawTinyText("HOLD MOUSEPOINTER OVER SCORE AND PRESS LEFT MOUSEBUTTON FOR MORE INFO",
                 POS_CENTERED, g_textAutoY, 3);
    AddMenuText(-1, 0x204, "VIEW THE ONLINE HALL OF FAME", 3, MENUID_ONLINE_HALL_OF_FAME, -1, 0, 2);
    DrawButtons(3);
    DrawMenuPrompt();
    if (g_debug != 0)
        DrawTextBox(100, 100, 500, 500, " ");
    HidePageButtons(1);
    DrawButtons(0);
}

// Draws the shared background/spark layer behind the bonus-tally overlay (the tally itself
// is drawn elsewhere via g_fnPtr's state callback).
#undef DRAW_HOF_TABLE_ROW

void TallyScreen()
{
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
}

// Draws the background/spark layer and warp-flash effect behind the "GAME OVER" message.
void GameOverScreen()
{
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawWarpFlash();
    g_textCursorY += 20;
}

// Draws the versus-mode result screen: occasional firework/explosion flourish, then the
// winner (or draw) message based on each player's score plus their bonus tally.
void VersusResult()
{
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (RandRange(0, 100) < 3 && g_state != STATE_PAUSED) {
        SpawnFirework();
        SoundPlay(g_sfxExplo3, RandRange(10000, 22000), RandRange(30, 100),
                          g_panTable[ClampX(400)], 127, g_sndFlags);
    }
    if (g_flag)
        UpdateSparks();
    if (g_save.players[0].score + g_bonusTally[0].total > g_save.players[1].score + g_p2Bonus) {
        DrawMenuText("P L A Y E R   O N E   I S   T H E   W I N N E R", POS_CENTERED, 300, 1);
        g_textCursorY += 20;
    } else if (g_save.players[1].score + g_p2Bonus > g_save.players[0].score + g_bonusTally[0].total) {
        DrawMenuText("P L A Y E R   T W O   I S   T H E   W I N N E R", POS_CENTERED, 300, 4);
        g_textCursorY += 20;
    } else {
        DrawMenuText("I T   W A S   A   D R A W ,   S O   Y O U   B O T H   W I N", POS_CENTERED, 300, 2);
        g_textCursorY += 20;
    }
    DrawMenuPrompt();
}

// No-op stub for a screen's optional draw callback.
void EmptyScreenStub()
{
}

// The rank line: 33 ranks, the admiral / grandmaster grades also get insignia.
#define RANK_CASES \
    case RANK_ENSIGN: \
        sprintf(g_logBuf, "                              ENSIGN = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_LIEUTENANT: \
        sprintf(g_logBuf, "                          LIEUTENANT = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_COMMANDER: \
        sprintf(g_logBuf, "                           COMMANDER = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_CAPTAIN: \
        sprintf(g_logBuf, "                             CAPTAIN = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_ADMIRAL: \
        sprintf(g_logBuf, "                             ADMIRAL = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_ADMIRAL_1_1: \
        sprintf(g_logBuf, "                           ADMIRAL   = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX, g_curY - 4, 0, 1); \
        break; \
    case RANK_ADMIRAL_1_2: \
        sprintf(g_logBuf, "                         ADMIRAL     = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 19, g_curY - 4, 0, 2); \
        break; \
    case RANK_ADMIRAL_1_3: \
        sprintf(g_logBuf, "                       ADMIRAL       = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 38, g_curY - 4, 0, 3); \
        break; \
    case RANK_ADMIRAL_2_1: \
        sprintf(g_logBuf, "                           ADMIRAL   = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX, g_curY - 4, 1, 1); \
        break; \
    case RANK_ADMIRAL_2_2: \
        sprintf(g_logBuf, "                         ADMIRAL     = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 19, g_curY - 4, 1, 2); \
        break; \
    case RANK_ADMIRAL_2_3: \
        sprintf(g_logBuf, "                       ADMIRAL       = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 38, g_curY - 4, 1, 3); \
        break; \
    case RANK_ADMIRAL_3_1: \
        sprintf(g_logBuf, "                           ADMIRAL   = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX, g_curY - 4, 2, 1); \
        break; \
    case RANK_ADMIRAL_3_2: \
        sprintf(g_logBuf, "                         ADMIRAL     = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 19, g_curY - 4, 2, 2); \
        break; \
    case RANK_ADMIRAL_3_3: \
        sprintf(g_logBuf, "                       ADMIRAL       = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 38, g_curY - 4, 2, 3); \
        break; \
    case RANK_KNIGHT: \
        sprintf(g_logBuf, "                     WARBLADE KNIGHT = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_LORD: \
        sprintf(g_logBuf, "                       WARBLADE LORD = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_OVERLORD: \
        sprintf(g_logBuf, "                   WARBLADE OVERLORD = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GRANDMASTER: \
        sprintf(g_logBuf, "                WARBLADE GRANDMASTER = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GRANDMASTER_1: \
        sprintf(g_logBuf, "              WARBLADE GRANDMASTER   = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX, g_curY - 4, 2, 1); \
        break; \
    case RANK_GRANDMASTER_2: \
        sprintf(g_logBuf, "            WARBLADE GRANDMASTER     = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 19, g_curY - 4, 2, 2); \
        break; \
    case RANK_GRANDMASTER_3: \
        sprintf(g_logBuf, "           WARBLADE GRANDMASTER      = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        DrawRow(rankX - 38, g_curY - 4, 2, 3); \
        break; \
    case RANK_CHAMPION: \
        sprintf(g_logBuf, "                   WARBLADE CHAMPION = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD: \
        sprintf(g_logBuf, "                        WARBLADE GOD = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_PLUTO: \
        sprintf(g_logBuf, "                 WARBLADE GOD  PLUTO = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_NEPTUNE: \
        sprintf(g_logBuf, "               WARBLADE GOD  NEPTUNE = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_URANUS: \
        sprintf(g_logBuf, "                WARBLADE GOD  URANUS = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_SATURN: \
        sprintf(g_logBuf, "                WARBLADE GOD  SATURN = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_JUPITER: \
        sprintf(g_logBuf, "               WARBLADE GOD  JUPITER = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_MARS: \
        sprintf(g_logBuf, "                  WARBLADE GOD  MARS = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_TELLUS: \
        sprintf(g_logBuf, "                WARBLADE GOD  TELLUS = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_VENUS: \
        sprintf(g_logBuf, "                 WARBLADE GOD  VENUS = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_MERCURY: \
        sprintf(g_logBuf, "               WARBLADE GOD  MERCURY = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    case RANK_GOD_SOL: \
        sprintf(g_logBuf, "                   WARBLADE GOD  SOL = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \
    default: \
        sprintf(g_logBuf, "                              ENSIGN = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        break; \


// (the single-player and player 1 tables pass a stray extra argument to the cash-left sprintf)
#define SCOREBUF_ARG , g_scoreBuf

// One player's bonus table (identical for single player, player 1 and player 2).
#define BONUS_BLOCK(p, score, cashExtra) \
    sprintf(g_logBuf, "-------------------              -------------------"); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 1); \
    g_textCursorY += 10; \
    sprintf(g_logBuf, "CASH LEFT                                           "); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6); \
    len = Int64ToStrGrouped(p.cash, g_scoreBuf); \
    sprintf(g_logBuf, "                         %5lld X 100 =              ", p.cash / 100 cashExtra); \
    CopyStrAt(g_logBuf, g_scoreBuf, 13 - len + 39, len); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
    g_textCursorY += 10; \
    if (p.maxCashFlag != -1) { \
        sprintf(g_logBuf, "MAX CASH BONUS                                      "); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6); \
        Int64ToStrGrouped(p.maxCashBonus, g_scoreBuf); \
        sprintf(g_logBuf, "                                     = %13s", g_scoreBuf); \
        DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
        g_textCursorY += 10; \
    } \
    sprintf(g_logBuf, "RANK BONUS                                          "); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6); \
    Int64ToStrGrouped(p.rankBonus, g_scoreBuf); \
    switch (p.rank) { \
    RANK_CASES \
    } \
    g_textCursorY += 10; \
    sprintf(g_logBuf, "PERFECTS                                            "); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6); \
    len = Int64ToStrGrouped(p.perfectsBonus, g_scoreBuf); \
    sprintf(g_logBuf, "                       %3lld X 100.000 =              ", p.perfects); \
    CopyStrAt(g_logBuf, g_scoreBuf, 13 - len + 39, len); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
    g_textCursorY += 10; \
    sprintf(g_logBuf, "HIT PERCENTAGE                                      "); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6); \
    Int64ToStrGrouped(p.hitBonus, g_scoreBuf); \
    sprintf(g_logBuf, "                          %3d X 1000 = %13s", p.hitPercent, g_scoreBuf); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
    g_textCursorY += 10; \
    sprintf(g_logBuf, "----------------------------------------------------"); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 1); \
    g_textCursorY += 10; \
    sprintf(g_logBuf, "SUM BONUS POINTS                                    "); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6); \
    Int64ToStrGrouped(p.total, g_scoreBuf); \
    sprintf(g_logBuf, "                                     = %13s", g_scoreBuf); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6); \
    g_textCursorY += 10; \
    sprintf(g_logBuf, "TOTAL SCORE                                         "); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6); \
    Int64ToStrGrouped(p.total + score, g_scoreBuf); \
    sprintf(g_logBuf, "                                     = %13s", g_scoreBuf); \
    DrawMenuText(g_logBuf, POS_CENTERED, g_curY, 6);

#define TIP(n, a, b) \
    if (g_tipIndex == n) { \
        DrawMenuText(a, POS_CENTERED, 0x226, 5); \
        DrawMenuText(b, POS_CENTERED, 0x235, 5); \
    }

// Draws the end-of-game bonus tally: one BONUS_BLOCK per player (game mode 0/6: single
// player; 1-3: two players), advances the tally count-up via TallyStep on its timer, and
// shows a rotating gameplay tip while the tally isn't yet fully counted up.
void BonusScreen()
{
    int len;
    int rankX;

    g_tallyState = 0;
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawBackground();
    g_fnPtr();
    if (g_flag)
        UpdateSparks();
    DrawMenuText("GAME BONUSES", POS_CENTERED, 10, 2);
    g_textCursorY += 20;
    rankX = 0x200;

    // ---- per-mode bonus tally block ----
    switch (g_gameMode) {
    case MODE_SINGLE:
    case MODE_TIME_TRIAL:
        sprintf(g_logBuf, "BONUS POINTS");
        DrawMenuText(g_logBuf, POS_CENTERED, 0x96, 6);
        BONUS_BLOCK(g_bonusTally[0], g_save.players[0].score, SCOREBUF_ARG)
        break;

    case MODE_TWO_PLAYER:
    case MODE_DUAL:
        sprintf(g_logBuf, "PLAYER 1");
        DrawMenuText(g_logBuf, POS_CENTERED, 0x32, 0);
        g_textCursorY += 5;
        sprintf(g_logBuf, "BONUS POINTS");
        DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6);
        BONUS_BLOCK(g_bonusTally[0], g_save.players[0].score, SCOREBUF_ARG)
        g_textCursorY += 35;
        sprintf(g_logBuf, "PLAYER 2");
        DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 0);
        g_textCursorY += 5;
        sprintf(g_logBuf, "BONUS POINTS");
        DrawMenuText(g_logBuf, POS_CENTERED, g_textAutoY, 6);
        BONUS_BLOCK(g_bonusTally[1], g_save.players[1].score, )
        break;
    }

    // ---- tally count-up advance ----
    if (g_time - g_tallyTime > g_tallyDelay) {
        switch (g_gameMode) {
        case MODE_SINGLE:
            g_tallyTime = g_time;
            g_tallyState = TallyStep(1);
            break;
        case MODE_TWO_PLAYER:
            g_tallyTime = g_time;
            g_tallyState = TallyStep(2);
            break;
        case MODE_DUAL:
            g_tallyTime = g_time;
            g_tallyState = TallyStep(2);
            break;
        case MODE_TEAM:
            g_tallyTime = g_time;
            g_tallyState = TallyStep(2);
            break;
            break; // stray duplicate; kept because removing it changes the compiled bytes
        case MODE_TIME_TRIAL:
            g_tallyTime = g_time;
            g_tallyState = TallyStep(1);
            break;
        }
    }

    // ---- rotating gameplay tip ----
    if (g_tipIndex > 0 && g_tipIndex < 19) {
        DrawMenuText("A TIP FROM OUTER SPACE", POS_CENTERED, 0x212, 0);
        TIP(1, "CREATE A PROFILE TO RECORD YOUR PROGRESS", "AND UNLOCK NEW FEATURES IN THE GAME!")
        TIP(2, "USE THE GRAPHICS BUFFER SYSTEM TO LOAD LEVEL", "DATA TO MEMORY AND GET A SMOOTHER GAME!")
        TIP(3, "REMEMBER TO RESTART THE GAME AFTER CHANGING",
            "MAJOR SETTINGS LIKE LEVEL BUFFERING!")
        if (g_tipIndex == 4) {
            DrawMenuText("YOU CAN SAVE THE GAME AT THE SHOP", POS_CENTERED, 0x226, 5);
            DrawMenuText("", g_textStartX, 0x235, 5);
        }
        TIP(5, "PLACE A WINAMP PLAYLIST IN THE GAME FOLDER TO USE",
            "YOUR OWN FAVORITE MUSIC, REMEMBER TO PRESS M!")
        TIP(6, "PRESS M KEY TO TOGGLE BETWEEN MOD MUSIC, MP3 MUSIC",
            "AND ANY AVAILABLE WINAMP PLAYLIST IN GAME FOLDER")
        TIP(7, "REMEMBER TO PLAY THE TIME TRIAL GAME TOO! ", "IT WILL HELP YOU UNLOCK STUFF IN YOUR PROFILE!")
        TIP(8, "YOU CAN CONFIGURE THE KEYS AND INPUT DEVICE TO",
            "USE ON THE INPUT CONFIG SCREEN, PRESS F9 ON MENUS")
        TIP(9, "LEARNING GAME SECRETS WILL HELP", "YOU GET MUCH FURTHER IN THE GAME")

        TIP(10, "MORE BULLETS...   MORE BULLETS...   MORE BULLETS!!", "")
        TIP(11, "PRESS F7 TO SAVE A SCREENSHOT OF THE GAME", "")
        TIP(12, "USING OPENGL MAY MAKE THE GAME GRAPHICS MOVE", "MUCH SMOOTHER ON SOME GRAPHICS CARDS!!")
        TIP(13, "IF THE GAME IS SLOW, TRY TURNING OFF GRAPHICS EFFECTS",
            "NUMBER OF STARS, BACKGROUNDS AND CHANGE SCREEN MODE")
        TIP(14, "SPEED...   MORE SPEED...   AND EVEN MORE SPEED!!", "")
        TIP(15, "FLARE STARS ARE FASTER THAN POINT STARS IF LOTS USED!!", "")
        TIP(16, "OPENGL CAN BE FASTER AND SMOOTHER THAN DIRECTX ON MANY SYSTEMS", "")
        TIP(17, "TURNING BACKGROUNDS OFF AND OPTIONS DOWN IS A TACTIC THE PROS USE...", "")
        TIP(18, "USE TAB KEY TO SHOW AND HIDE PROFILE WINDOW WHEN PLAYING", "")
    }

    if (g_tallyState != 0)
        g_screenTimerStart = g_time;
    if (g_time - g_uiBlinkTime > g_blinkRate) {
        g_uiBlinkTime = g_time;
        g_uiBlink = g_uiBlink == 0;
    }
}

#undef RANK_CASES
#undef SCOREBUF_ARG
#undef BONUS_BLOCK
#undef TIP
