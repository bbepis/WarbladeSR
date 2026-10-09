// Tests for src/core/platform.c (game folder, debug log, screenshots, clock helpers, cursor
// clipping), src/core/weblinks.c (archived web links) and the parts of src/core/init.c that
// run on their own (window creation, frame settings, ship tables).
#include <stdlib.h>
#include "support.h"

// The contents of a file under the user folder ("warblade\\x"), or "" if it can't be read.
static const char *ReadUserFile(const char *rel)
{
    static char buf[16384];
    buf[0] = 0;
    FILE *f = fopen(FakeUserPath(rel), "rb");
    if (!f)
        return buf;
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = 0;
    fclose(f);
    return buf;
}

static void WriteUserFile(const char *rel, const char *text)
{
    FILE *f = fopen(FakeUserPath(rel), "wb");
    CHECK(f != NULL);
    fputs(text, f);
    fclose(f);
}

#define LOG_HEADER "************ Warblade Debug information ************\r\n"

// ---------------------------------------------------------------- MakeGameDir

TEST(core_MakeGameDir_creates_the_warblade_folder)
{
    CHECK(!FakeFileExists(FakeUserPath("warblade")));
    MakeGameDir();
    CHECK(FakeFileExists(FakeUserPath("warblade")));
    // again: it's already there, nothing changes
    WriteUserFile("warblade\\keep.txt", "x");
    MakeGameDir();
    CHECK(FakeFileExists(FakeUserPath("warblade\\keep.txt")));
}

// ---------------------------------------------------------------- the debug log

TEST(core_LogInit_writes_the_header)
{
    MakeGameDir();
    LogInit();
    CHECK_STR(ReadUserFile("warblade\\warblade.dbg"), LOG_HEADER);
}

TEST(core_LogInit_truncates_an_old_log)
{
    MakeGameDir();
    WriteUserFile("warblade\\warblade.dbg",
                  "an old log from a previous run, longer than the header line is....... "
                  "and then some more text\r\n");
    LogInit();
    CHECK_STR(ReadUserFile("warblade\\warblade.dbg"), LOG_HEADER);
}

TEST(core_LogPrint_appends_to_the_log)
{
    MakeGameDir();
    LogInit();
    LogPrint("one\r\n");
    LogPrint("two");
    LogPrint("");
    LogPrint(" three\r\n");
    CHECK_STR(ReadUserFile("warblade\\warblade.dbg"), LOG_HEADER "one\r\ntwo three\r\n");
    CHECK_STR(g_fake.lastMessageBox, "");
}

TEST(core_LogPrint_without_a_log_creates_nothing)
{
    MakeGameDir();
    LogPrint("lost\r\n");
    CHECK(!FakeFileExists(FakeUserPath("warblade\\warblade.dbg")));
}

TEST(core_LogCrashReport_writes_state_and_players)
{
    MakeGameDir();
    LogInit();
    g_state = STATE_SHOP;
    g_savedState = STATE_GET_READY;
    g_gameMode = MODE_TWO_PLAYER;
    g_windowed = 1;
    g_save.players[0].level = 7;
    g_save.players[0].score = 123456;
    g_save.players[0].money = 900;
    g_save.players[1].level = 3;
    g_save.players[1].weapon = 2;
    g_profileIndex = 2;
    LogCrashReport("boom");
    const char *log = ReadUserFile("warblade\\warblade.dbg");
    char want[64];
    snprintf(want, sizeof want, "Program State: %d\r\n", STATE_SHOP);
    const char *start = LOG_HEADER "\r\n\r\n\r\n## CRASH ## v2.0\r\nVersion : FULL VERSION\r\n";
    CHECK(strncmp(log, start, strlen(start)) == 0);
    CHECK(strstr(log, want) != NULL);
    CHECK(strstr(log, "Game mode : TWO PLAYER GAME\r\n") != NULL);
    CHECK(strstr(log, "Game screen mode : WINDOWED\r\n") != NULL);
    CHECK(strstr(log, "**** PLAYER ONE ****\r\n On level: 7\r\n    Score: 123456\r\n   Weapon: 0\r\n     Cash: 900\r\n") != NULL);
    CHECK(strstr(log, "**** PLAYER TWO ****\r\n On level: 3\r\n    Score: 0\r\n   Weapon: 2\r\n") != NULL);
    CHECK(strstr(log, "Profile is in use...\r\n") != NULL);
    CHECK(strstr(log, "Error: boom\r\nError occurred at ") != NULL);
}

TEST(core_LogCrashReport_fullscreen_single_no_profile)
{
    MakeGameDir();
    LogInit();
    g_gameMode = MODE_SINGLE;
    g_windowed = 0;
    LogCrashReport("x");
    const char *log = ReadUserFile("warblade\\warblade.dbg");
    CHECK(strstr(log, "Game mode : SINGLE PLAYER GAME\r\n") != NULL);
    CHECK(strstr(log, "Game screen mode : FULLSCREEN\r\n") != NULL);
    CHECK(strstr(log, "Profile is in use") == NULL);
}

// ---------------------------------------------------------------- cursor clipping

TEST(core_ClipCursor_only_in_fullscreen)
{
    g_windowed = 0;
    ClipCursorOn();
    CHECK(g_fake.pointerClipped);
    CHECK_EQ_INT(g_fake.clipPointerCalls, 1);
    ClipCursorOff();
    CHECK(!g_fake.pointerClipped);
    CHECK_EQ_INT(g_fake.clipPointerCalls, 2);

    g_windowed = 1;
    ClipCursorOn();
    ClipCursorOff();
    CHECK_EQ_INT(g_fake.clipPointerCalls, 2);
}

// ---------------------------------------------------------------- clock helpers

TEST(core_CurMonth_CurYear_from_the_date)
{
    CHECK_EQ_INT(CurMonth(), 6);
    CHECK_EQ_INT(CurYear(), 2009);
    g_fake.localDate.month = 12;
    g_fake.localDate.year = 2031;
    CHECK_EQ_INT(CurMonth(), 12);
    CHECK_EQ_INT(CurYear(), 2031);
}

TEST(core_GetDaySeconds_combines_day_and_time)
{
    // 2009-06-15 12:30:45
    CHECK_EQ_INT(GetDaySeconds(), 15 * 86400 + 12 * 3600 + 30 * 60 + 45);
    CHECK_EQ_INT(g_sysTime.day, 15);
    g_fake.localDate.day = 1;
    g_fake.localDate.hour = 0;
    g_fake.localDate.minute = 1;
    g_fake.localDate.second = 2;
    CHECK_EQ_INT(GetDaySeconds(), 86400 + 62);
}

TEST(core_StampTime_records_the_file_time)
{
    g_fake.fileTime = 1000;
    StampTimeA();
    g_fake.fileTime = 2000;
    StampTimeB();
    g_fake.fileTime = 3000;
    StampTimeC();
    CHECK_EQ_INT(g_timeA, 1000);
    CHECK_EQ_INT(g_timeMarkB, 2000);
    CHECK_EQ_INT(g_timeMarkC, 3000);
}

TEST(core_StampTimeE_waits_for_StampTimeD)
{
    g_fake.fileTime = 5000;
    StampTimeE();
    CHECK_EQ_INT(g_timeE, 0);
    StampTimeD();
    CHECK_EQ_INT(g_timeD, 5000);
    CHECK_EQ_INT(g_timeE, 0);
    g_fake.fileTime = 7000;
    StampTimeE();
    CHECK_EQ_INT(g_timeE, 7000);
    CHECK_EQ_INT(g_timeD, 5000);
}

TEST(core_Timer1_keeps_the_shortest_interval)
{
    TimerReset1();
    CHECK_EQ_INT(g_timerMin1, 999999999);
    g_fake.fileTime = 10000;
    TimerStart1();
    g_fake.fileTime = 10500;
    TimerStop1();
    CHECK_EQ_INT(g_timerEnd1, 10500);
    CHECK_EQ_INT(g_timerMin1, 500);
    TimerStart1();
    g_fake.fileTime = 11300;
    TimerStop1();
    CHECK_EQ_INT(g_timerMin1, 500);
    TimerStart1();
    g_fake.fileTime = 11600;
    TimerStop1();
    CHECK_EQ_INT(g_timerMin1, 300);
    TimerReset1();
    CHECK_EQ_INT(g_timerMin1, 999999999);
}

TEST(core_Timer2_keeps_the_shortest_interval)
{
    TimerReset2();
    CHECK_EQ_INT(g_timerMin2, 999999999);
    g_fake.fileTime = 20000;
    TimerStart2();
    g_fake.fileTime = 20800;
    TimerStop2();
    CHECK_EQ_INT(g_timerEnd2, 20800);
    CHECK_EQ_INT(g_timerMin2, 800);
    TimerStart2();
    g_fake.fileTime = 21800;
    TimerStop2();
    CHECK_EQ_INT(g_timerMin2, 800);
    TimerStart2();
    g_fake.fileTime = 21900;
    TimerStop2();
    CHECK_EQ_INT(g_timerMin2, 100);
    CHECK_EQ_INT(g_timerMin1, 0);
}

// ---------------------------------------------------------------- screenshots

TEST(core_TakeScreenshot_numbers_files_from_001)
{
    MakeGameDir();
    TakeScreenshot();
    CHECK(FakeFileExists(FakeUserPath("warblade\\screenshots")));
    CHECK(FakeFileExists(FakeUserPath("warblade\\screenshots\\ScreenShot001.jpg")));
    TakeScreenshot();
    CHECK(FakeFileExists(FakeUserPath("warblade\\screenshots\\ScreenShot002.jpg")));
    CHECK(!FakeFileExists(FakeUserPath("warblade\\screenshots\\ScreenShot003.jpg")));
}

TEST(core_TakeScreenshot_fills_the_first_gap)
{
    MakeGameDir();
    TakeScreenshot();
    TakeScreenshot();
    TakeScreenshot();
    remove(FakeUserPath("warblade\\screenshots\\ScreenShot002.jpg"));
    TakeScreenshot();
    CHECK(FakeFileExists(FakeUserPath("warblade\\screenshots\\ScreenShot002.jpg")));
    CHECK(!FakeFileExists(FakeUserPath("warblade\\screenshots\\ScreenShot004.jpg")));
}

TEST(core_TakeScreenshot_without_game_folder_does_nothing)
{
    // the screenshots folder can't be made without warblade\ (no recursive mkdir)
    TakeScreenshot();
    CHECK(!FakeFileExists(FakeUserPath("warblade\\screenshots")));
    CHECK(!FakeFileExists(FakeUserPath("warblade\\screenshots\\ScreenShot001.jpg")));
}

// ---------------------------------------------------------------- weblinks.c

TEST(core_ArchiveUrl_known_pages)
{
    CHECK_STR(ArchiveUrl("http://www.warblade.as/faq.asp"),
              "https://web.archive.org/web/20061231212123/http://www.warblade.as:80/faq.asp");
    CHECK_STR(ArchiveUrl("http://www.warblade.as/help.asp"),
              "https://web.archive.org/web/20061230151009/http://www.warblade.as:80/help.asp");
    CHECK_STR(ArchiveUrl("http://www.warblade.as/manual.txt"),
              "https://web.archive.org/web/20080402231704/http://www.warblade.as:80/manual.txt");
    CHECK_STR(ArchiveUrl("warblade.as/gamenews.asp"),
              "https://web.archive.org/web/20051228161911/http://www.warblade.as:80/gamenews.asp");
    CHECK_STR(ArchiveUrl("http://karthesios.tripod.com"),
              "https://web.archive.org/web/20030227224434/http://karthesios.tripod.com:80/");
    CHECK_STR(ArchiveUrl("http://sbelectronics.com.au/"),
              "https://web.archive.org/web/20080829054615/http://www.sbelectronics.com.au/");
}

TEST(core_ArchiveUrl_ignores_case_scheme_www_and_slashes)
{
    const char *groovy = "https://web.archive.org/web/20061230021822/http://www.groovyaudio.com:80/";
    CHECK_STR(ArchiveUrl("http://www.GroovyAudio.com"), groovy);
    CHECK_STR(ArchiveUrl("HTTPS://WWW.GROOVYAUDIO.COM//"), groovy);
    CHECK_STR(ArchiveUrl("https://groovyaudio.com/"), groovy);
    CHECK_STR(ArchiveUrl("groovyaudio.com"), groovy);
}

TEST(core_ArchiveUrl_drops_the_query)
{
    CHECK_STR(ArchiveUrl("http://www.warblade.as/halloffame.asp?mm=2&D=0&m=0"),
              "https://web.archive.org/web/20061230083019/http://www.warblade.as:80/halloffame.asp?");
}

TEST(core_ArchiveUrl_other_warblade_pages_go_to_the_front_page)
{
    const char *front = "https://web.archive.org/web/20061230082208/http://www.warblade.as:80/";
    CHECK_STR(ArchiveUrl("http://www.warblade.as/"), front);
    CHECK_STR(ArchiveUrl("http://www.warblade.as/download.asp?x=1"), front);
    CHECK_STR(ArchiveUrl("warblade.as/news/2008"), front);
}

TEST(core_ArchiveUrl_unknown_sites_unchanged)
{
    const char *url = "https://www.libsdl.org/";
    CHECK(ArchiveUrl(url) == url);
    const char *gh = "https://github.com/bbepis/WarbladeSR";
    CHECK(ArchiveUrl(gh) == gh);
}

TEST(core_OpenUrl_opens_the_archive_and_saves_hiscores)
{
    MakeGameDir();
    g_clickWin = 3;
    g_clickItem = 4;
    g_hiscoresCleared = 0;
    OpenUrl("http://www.warblade.as/help.asp");
    CHECK_STR(g_fake.lastUrl,
              "https://web.archive.org/web/20061230151009/http://www.warblade.as:80/help.asp");
    CHECK_EQ_INT(g_clickWin, -1);
    CHECK_EQ_INT(g_clickItem, -1);
    CHECK(FakeFileExists(FakeUserPath("warblade\\warblade_132.his")));
    CHECK_EQ_INT(g_hiscoresCleared, 1);

    OpenUrl("https://www.libsdl.org/");
    CHECK_STR(g_fake.lastUrl, "https://www.libsdl.org/");
}

// ---------------------------------------------------------------- init.c: tables, settings

TEST(core_InitTablePtrs_points_at_each_ship)
{
    InitTablePtrs();
    CHECK(g_shipDefs[0] == (ShipDef *)g_shipStats0);
    CHECK(g_shipDefs[1] == (ShipDef *)g_shipStats1);
    CHECK(g_shipDefs[2] == (ShipDef *)g_shipStats2);
    CHECK(g_shipDefs[3] == (ShipDef *)g_shipStats3);
    CHECK(g_shipDefs[4] == (ShipDef *)g_shipStats4);
    CHECK(g_shipDefs[5] == (ShipDef *)g_shipStats5);
    CHECK(g_shipDefs[6] == (ShipDef *)g_shipStats6);
    CHECK(g_shipDefs[7] == (ShipDef *)g_shipStats7);
    CHECK(g_shipDefs[8] == (ShipDef *)g_shipStats8);
    CHECK(g_shipDefs[9] == (ShipDef *)g_shipStats9);
    // the first stat of a ship, through its definition
    CHECK_EQ_INT(*(int *)g_shipDefs[3], 215);
}

TEST(core_RendererChoiceOf_maps_unknown_to_auto)
{
    CHECK_EQ_INT(RendererChoiceOf(RENDERER_AUTO_OLD_OPENGL), RENDERER_AUTO);
    CHECK_EQ_INT(RendererChoiceOf(RENDERER_AUTO), RENDERER_AUTO);
    CHECK_EQ_INT(RendererChoiceOf(RENDERER_DIRECTX9), RENDERER_DIRECTX9);
    CHECK_EQ_INT(RendererChoiceOf(RENDERER_OPENGL), RENDERER_OPENGL);
    CHECK_EQ_INT(RendererChoiceOf(RENDERER_VULKAN), RENDERER_VULKAN);
    CHECK_EQ_INT(RendererChoiceOf(RENDERER_COUNT), RENDERER_AUTO);
    CHECK_EQ_INT(RendererChoiceOf(255), RENDERER_AUTO);
    CHECK_EQ_INT(RendererChoiceOf(-1), RENDERER_AUTO);
}

TEST(core_ApplyFrameSettings_passes_vsync_and_interpolation)
{
    g_cfg.vsyncOff = 1;
    g_cfg.interpolation = SYS_INTERP_OFF;
    ApplyFrameSettings();
    CHECK(!g_fake.vsync);
    CHECK_EQ_INT(g_fake.interpolation, SYS_INTERP_OFF);
    g_cfg.vsyncOff = 0;
    g_cfg.interpolation = SYS_INTERP_ON;
    ApplyFrameSettings();
    CHECK(g_fake.vsync);
    CHECK_EQ_INT(g_fake.interpolation, SYS_INTERP_ON);
    g_cfg.vsyncOff = 2;     // anything but 1 is on
    ApplyFrameSettings();
    CHECK(g_fake.vsync);
}

TEST(core_ToggleVSync_flips_applies_and_saves)
{
    MakeGameDir();
    g_cfg.vsyncOff = 0;
    CHECK_STR(ToggleVSync(), "VSYNC : OFF");
    CHECK_EQ_INT(g_cfg.vsyncOff, 1);
    CHECK(!g_fake.vsync);
    CHECK(FakeFileExists(FakeUserPath("warblade\\WarBlade.inf")));
    CHECK_STR(ToggleVSync(), "VSYNC : ON");
    CHECK_EQ_INT(g_cfg.vsyncOff, 0);
    CHECK(g_fake.vsync);
}

TEST(core_CycleInterpolation_auto_on_off_auto)
{
    MakeGameDir();
    g_cfg.interpolation = SYS_INTERP_AUTO;
    CHECK_STR(CycleInterpolation(), "INTERPOLATION : ON");
    CHECK_EQ_INT(g_fake.interpolation, SYS_INTERP_ON);
    CHECK(FakeFileExists(FakeUserPath("warblade\\WarBlade.inf")));
    CHECK_STR(CycleInterpolation(), "INTERPOLATION : OFF");
    CHECK_EQ_INT(g_fake.interpolation, SYS_INTERP_OFF);
    CHECK_STR(CycleInterpolation(), "INTERPOLATION : AUTO - OFF");   // the fake doesn't interpolate
    CHECK_EQ_INT(g_cfg.interpolation, SYS_INTERP_AUTO);
    CHECK_EQ_INT(g_fake.interpolation, SYS_INTERP_AUTO);
    g_cfg.interpolation = 9;    // out of range goes back to auto
    CycleInterpolation();
    CHECK_EQ_INT(g_cfg.interpolation, SYS_INTERP_AUTO);
}

// ---------------------------------------------------------------- init.c: the window

TEST(core_InitWindow_creates_an_800x600_window)
{
    MakeGameDir();
    LogInit();
    g_screenW = 1024;
    g_screenH = 768;
    g_cfg.renderer = RENDERER_AUTO;
    g_cfg.vsyncOff = 1;
    CHECK(InitWindow(true));
    CHECK_EQ_INT(g_screenW, 800);
    CHECK_EQ_INT(g_screenH, 600);
    CHECK_EQ_INT(g_screenHInit, 600);
    CHECK_NEAR(g_worldZoom, 1.0, 0);
    CHECK_EQ_INT(g_cfg.windowed, 1);
    CHECK_EQ_INT(g_fake.windowsCreated, 1);
    CHECK(!g_fake.fullscreen);
    CHECK_STR(g_fake.lastRenderDriver, "");
    CHECK_EQ_INT(g_windowRenderer, RENDERER_AUTO);
    CHECK(!g_fake.vsync);
    CHECK_NEAR(g_offsetX, 0, 0);
    CHECK_NEAR(g_offsetY, 0, 0);
    CHECK(strstr(ReadUserFile("warblade\\warblade.dbg"), "Renderer : fake\n") != NULL);
}

TEST(core_InitWindow_fullscreen_with_a_chosen_renderer)
{
    g_cfg.renderer = RENDERER_OPENGL;
    CHECK(InitWindow(false));
    CHECK_EQ_INT(g_cfg.windowed, 0);
    CHECK(g_fake.fullscreen);
    CHECK_STR(g_fake.lastRenderDriver, "opengl");
    CHECK_EQ_INT(g_windowRenderer, RENDERER_OPENGL);
    CHECK_EQ_INT(g_fake.windowsCreated, 1);
    CHECK_EQ_INT(g_cfg.renderer, RENDERER_OPENGL);
}

TEST(core_InitWindow_falls_back_to_auto_when_the_renderer_fails)
{
    MakeGameDir();
    LogInit();
    g_cfg.renderer = RENDERER_VULKAN;
    g_fake.createWindowFails = 1;
    CHECK(InitWindow(true));
    CHECK_STR(g_fake.lastRenderDriver, "");
    CHECK_EQ_INT(g_fake.windowsCreated, 1);
    CHECK_EQ_INT(g_cfg.renderer, RENDERER_AUTO);
    CHECK_EQ_INT(g_windowRenderer, RENDERER_AUTO);
    CHECK(FakeFileExists(FakeUserPath("warblade\\WarBlade.inf")));
    CHECK(strstr(ReadUserFile("warblade\\warblade.dbg"),
                 "ERROR :  Could not create the vulkan renderer, using auto\n") != NULL);
}

TEST(core_InitWindow_fails_without_any_window)
{
    MakeGameDir();
    LogInit();
    g_cfg.renderer = RENDERER_AUTO;
    g_fake.createWindowFails = 1;
    CHECK(!InitWindow(true));
    CHECK_EQ_INT(g_fake.windowsCreated, 0);
    CHECK(strstr(ReadUserFile("warblade\\warblade.dbg"), "ERROR :  Could not open window\n") != NULL);
}

TEST(core_InitWindowCfg_resets_the_clip_rect)
{
    g_clipLeft = 5;
    g_clipTop = 6;
    g_screenW = 640;
    g_screenH = 480;
    g_cfg.windowed = 1;
    CHECK_EQ_INT(InitWindowCfg(), 1);
    CHECK_EQ_INT(g_clipLeft, 0);
    CHECK_EQ_INT(g_clipTop, 0);
    // from the screen size before the window set it
    CHECK_EQ_INT(g_clipRight, 640);
    CHECK_EQ_INT(g_clipBottom, 480);
    CHECK(!g_fake.fullscreen);
}

TEST(core_InitFail_logs_and_returns_0)
{
    MakeGameDir();
    LogInit();
    CHECK_EQ_INT(InitFail("Something broke\r\n"), 0);
    CHECK_STR(ReadUserFile("warblade\\warblade.dbg"), LOG_HEADER "Something broke\r\n");
}
