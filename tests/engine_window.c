// engine_window.c: The real engine's (src/core/sdl.c) window and frame loop: SysCreateWindow,
// events, focus, SysFlip's frame cap and interpolation, read back from the offscreen driver.
#include "engine_util.h"
#include <dirent.h>

TEST(engine_SysCreateWindow_makes_a_window_with_the_renderer_asked)
{
    InitVideo();
    CHECK(!SysHasWindow());
    CHECK_STR(SysRendererName(), "");
    CHECK(SysCreateWindow(64, 48, true, "wbengine", "software"));
    CHECK(SysHasWindow());
    CHECK_STR(SysRendererName(), "software");
    {
        int w = 0, h = 0;
        SDL_GetWindowSize(TheWindow(), &w, &h);
        CHECK_EQ_INT(w, 64);
        CHECK_EQ_INT(h, 48);
        CHECK_STR(SDL_GetWindowTitle(TheWindow()), "wbengine");
        CHECK((SDL_GetWindowFlags(TheWindow()) & SDL_WINDOW_FULLSCREEN) == 0);
        CHECK((SDL_GetWindowFlags(TheWindow()) & SDL_WINDOW_RESIZABLE) != 0);
    }
    SysDestroyWindow();
    CHECK(!SysHasWindow());
    CHECK_STR(SysRendererName(), "");
    SysDestroyWindow();                     // twice: nothing happens
    {
        int n = -1;
        SDL_free(SDL_GetWindows(&n));
        CHECK_EQ_INT(n, 0);
    }
}

TEST(engine_SysCreateWindow_fails_with_an_unknown_renderer)
{
    InitVideo();
    CHECK(!SysCreateWindow(64, 48, true, "wbengine", "no-such-renderer"));
    CHECK(!SysHasWindow());
    CHECK_STR(SysRendererName(), "");
    // The hint doesn't stay: the next window can use any renderer.
    CHECK(SysCreateWindow(64, 48, true, "wbengine", NULL));
    CHECK(SysHasWindow());
}

TEST(engine_SysCreateWindow_again_replaces_the_window)
{
    int n = 0, w = 0, h = 0;
    OpenWindow(64, 48);
    CHECK(SysCreateWindow(32, 16, true, "second", "software"));
    SDL_free(SDL_GetWindows(&n));
    CHECK_EQ_INT(n, 1);
    SDL_GetWindowSize(TheWindow(), &w, &h);
    CHECK_EQ_INT(w, 32);
    CHECK_EQ_INT(h, 16);
    CHECK_STR(SDL_GetWindowTitle(TheWindow()), "second");
}

TEST(engine_SysCreateWindow_fullscreen_and_SysSetFullscreen)
{
    InitVideo();
    CHECK(SysCreateWindow(64, 48, false, "wbengine", "software"));
    CHECK((SDL_GetWindowFlags(TheWindow()) & SDL_WINDOW_FULLSCREEN) != 0);
    SysSetFullscreen(false);
    CHECK((SDL_GetWindowFlags(TheWindow()) & SDL_WINDOW_FULLSCREEN) == 0);
    SysSetFullscreen(true);
    CHECK((SDL_GetWindowFlags(TheWindow()) & SDL_WINDOW_FULLSCREEN) != 0);
}

TEST(engine_SysTerminate_requests_quit)
{
    OpenWindow(64, 48);
    CHECK(!SysQuitRequested());
    SysTerminate();
    CHECK(SysQuitRequested());
    // A new window starts over.
    CHECK(SysCreateWindow(64, 48, true, "wbengine", "software"));
    CHECK(!SysQuitRequested());
}

static void PushWindowEvent(Uint32 type)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = type;
    e.window.windowID = SDL_GetWindowID(TheWindow());
    CHECK(SDL_PushEvent(&e));
}

TEST(engine_SysProcessEvents_quits_on_quit_events)
{
    SDL_Event e;
    OpenWindow(64, 48);
    SDL_zero(e);
    e.type = SDL_EVENT_QUIT;
    CHECK(SDL_PushEvent(&e));
    SysProcessEvents();
    CHECK(SysQuitRequested());

    CHECK(SysCreateWindow(64, 48, true, "wbengine", "software"));
    SysProcessEvents();
    CHECK(!SysQuitRequested());
    PushWindowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED);
    SysProcessEvents();
    CHECK(SysQuitRequested());
}

static int s_focusCalls;
static bool s_focusLast;

static void OnFocus(bool focused)
{
    s_focusCalls++;
    s_focusLast = focused;
}

TEST(engine_SysProcessEvents_tracks_focus)
{
    OpenWindow(64, 48);
    SysProcessEvents();
    SysSetFocusCallback(OnFocus);
    CHECK(SysHasFocus());
    PushWindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST);
    SysProcessEvents();
    CHECK(!SysHasFocus());
    CHECK_EQ_INT(s_focusCalls, 1);
    CHECK(!s_focusLast);
    PushWindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED);
    SysProcessEvents();
    CHECK(SysHasFocus());
    CHECK_EQ_INT(s_focusCalls, 2);
    CHECK(s_focusLast);
    // Without a callback, still tracked.
    SysSetFocusCallback(NULL);
    PushWindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST);
    SysProcessEvents();
    CHECK(!SysHasFocus());
    CHECK_EQ_INT(s_focusCalls, 2);
}

static int SDLCALL RefocusLater(void *data)
{
    SDL_Event e;
    SDL_Delay(150);
    SDL_zero(e);
    e.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    e.window.windowID = (SDL_WindowID)(uintptr_t)data;
    SDL_PushEvent(&e);
    return 0;
}

TEST(engine_SysFlip_waits_while_in_the_background)
{
    double t;
    SDL_Thread *th;
    OpenWindow(64, 48);
    SysSetVSync(false);
    SysSetInterpolation(SYS_INTERP_OFF);
    SysSetMaxFps(0);
    PushWindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST);
    SysProcessEvents();
    CHECK(!SysHasFocus());
    th = SDL_CreateThread(RefocusLater, "refocus",
                          (void *)(uintptr_t)SDL_GetWindowID(TheWindow()));
    t = NowSeconds();
    SysFlip();
    t = NowSeconds() - t;
    SDL_WaitThread(th, NULL);
    CHECK_MSG(t >= 0.14, "SysFlip returned after %g s", t);
    CHECK(SysHasFocus());
}

TEST(engine_SysFlip_doesnt_wait_in_the_background_after_quit)
{
    double t;
    OpenWindow(64, 48);
    SysSetVSync(false);
    SysSetInterpolation(SYS_INTERP_OFF);
    SysSetMaxFps(0);
    PushWindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST);
    SysProcessEvents();
    SysTerminate();
    t = NowSeconds();
    SysFlip();
    t = NowSeconds() - t;
    CHECK_MSG(t < 0.08, "SysFlip took %g s", t);
}

// ---------------------------------------------------------------------------------------------
// Frame cap
// ---------------------------------------------------------------------------------------------

// The shortest and longest time between `n` flips' returns, in ms, with `workMs` spent between
// them.
static void FlipTimesWorking(int n, int workMs, double *minMs, double *maxMs)
{
    double last, now;
    int i;
    SysFlip();
    last = NowSeconds();
    *minMs = 1e9;
    *maxMs = 0;
    for (i = 0; i < n; i++) {
        if (workMs)
            SDL_Delay(workMs);
        SysFlip();
        now = NowSeconds();
        *minMs = SDL_min(*minMs, (now - last) * 1000);
        *maxMs = SDL_max(*maxMs, (now - last) * 1000);
        last = now;
    }
}

static void FlipTimes(int n, double *minMs, double *maxMs)
{
    FlipTimesWorking(n, 0, minMs, maxMs);
}

static void PlainFlips(void)
{
    OpenWindow(64, 48);
    SysSetVSync(false);
    SysSetInterpolation(SYS_INTERP_OFF);
}

TEST(engine_SysFlip_caps_the_frame_rate)
{
    double lo, hi;
    PlainFlips();
    SysSetMaxFps(20);
    FlipTimes(5, &lo, &hi);
    CHECK_MSG(lo >= 49.5 && lo < 75, "frames %g-%g ms apart at 20 fps", lo, hi);
}

TEST(engine_SysFlip_cap_counts_the_time_since_the_last_flip)
{
    double lo, hi;
    PlainFlips();
    SysSetMaxFps(20);
    // 20 ms of work a frame: the flip waits the other 30.
    FlipTimesWorking(5, 20, &lo, &hi);
    CHECK_MSG(lo >= 49.5 && lo < 62, "frames %g-%g ms apart at 20 fps", lo, hi);
}

// The time 10 flips take, in ms (SDL's simulated vsync catches up after a late frame, so the
// total is what it paces, not each frame).
static double TenFlipsMs(void)
{
    double t;
    int i;
    SysFlip();
    t = NowSeconds();
    for (i = 0; i < 10; i++)
        SysFlip();
    return (NowSeconds() - t) * 1000;
}

TEST(engine_SysSetVSync_paces_the_presents)
{
    double ms;
    OpenWindow(64, 48);                     // vsync on by default
    SysSetInterpolation(SYS_INTERP_OFF);
    SysSetMaxFps(0);
    ms = TenFlipsMs();
    CHECK_MSG(ms >= 140, "10 frames in %g ms with vsync", ms);
    SysSetVSync(false);
    ms = TenFlipsMs();
    CHECK_MSG(ms < 60, "10 frames in %g ms without vsync", ms);
    SysSetVSync(true);
    ms = TenFlipsMs();
    CHECK_MSG(ms >= 140, "10 frames in %g ms with vsync again", ms);
}

TEST(engine_SysSetVSync_before_the_window_applies_to_it)
{
    double ms;
    InitVideo();
    SysSetVSync(false);
    CHECK(SysCreateWindow(64, 48, true, "wbengine", "software"));
    SysSetInterpolation(SYS_INTERP_OFF);
    SysSetMaxFps(0);
    ms = TenFlipsMs();
    CHECK_MSG(ms < 60, "10 frames in %g ms without vsync", ms);
}

TEST(engine_SysFlip_caps_at_60_fps_by_default)
{
    double lo, hi;
    PlainFlips();
    FlipTimes(20, &lo, &hi);
    CHECK_MSG(lo >= 15.5 && lo < 16.9, "frames %g-%g ms apart at 60 fps", lo, hi);
}

TEST(engine_SysFlip_caps_in_whole_milliseconds)
{
    double lo, hi;
    PlainFlips();
    SysSetMaxFps(600);                      // 1000 / 600: 1 ms, not 1.67
    FlipTimes(40, &lo, &hi);
    CHECK_MSG(lo >= 0.95 && lo < 1.9, "frames %g-%g ms apart at 600 fps", lo, hi);
}

TEST(engine_SysFlip_without_a_cap)
{
    double lo, hi;
    PlainFlips();
    SysSetMaxFps(0);
    FlipTimes(20, &lo, &hi);
    CHECK_MSG(lo < 0.9, "frames %g-%g ms apart without a cap", lo, hi);
    SysSetMaxFps(-5);
    FlipTimes(20, &lo, &hi);
    CHECK_MSG(lo < 0.9, "frames %g-%g ms apart at -5 fps", lo, hi);
}

TEST(engine_SysFlip_shows_the_back_buffer_letterboxed)
{
    SDL_Surface *s;
    OpenWindow(64, 48);
    SysSetVSync(false);
    SysSetInterpolation(SYS_INTERP_OFF);
    DrawRect(0, 0, 64, 48, 1, 0, 0, 1);
    DrawRect(0, 0, 1, 1, 0, 1, 0, 1);
    SysFlip();
    s = SDL_GetWindowSurface(TheWindow());
    CHECK(s != NULL);
    CHECK_EQ_INT(s->w, 64);
    CHECK_RGB(SurfaceRgb(s, 0, 0), 0x00ff00, 0);
    CHECK_RGB(SurfaceRgb(s, 63, 47), 0xff0000, 0);
    // Wider: bars left and right.
    SDL_SetWindowSize(TheWindow(), 128, 48);
    SysFlip();
    s = SDL_GetWindowSurface(TheWindow());
    CHECK(s != NULL);
    CHECK_EQ_INT(s->w, 128);
    CHECK_RGB(SurfaceRgb(s, 31, 24), 0, 0);
    CHECK_RGB(SurfaceRgb(s, 32, 0), 0x00ff00, 0);
    CHECK_RGB(SurfaceRgb(s, 33, 24), 0xff0000, 0);
    CHECK_RGB(SurfaceRgb(s, 95, 47), 0xff0000, 0);
    CHECK_RGB(SurfaceRgb(s, 96, 24), 0, 0);
    // Scaled up, twice the size, centred.
    SDL_SetWindowSize(TheWindow(), 128, 128);
    SysFlip();
    s = SDL_GetWindowSurface(TheWindow());
    CHECK_RGB(SurfaceRgb(s, 64, 15), 0, 0);
    CHECK_RGB(SurfaceRgb(s, 0, 16), 0x00ff00, 0);
    CHECK_RGB(SurfaceRgb(s, 1, 17), 0x00ff00, 0);
    CHECK_RGB(SurfaceRgb(s, 2, 16), 0xff0000, 0);
    CHECK_RGB(SurfaceRgb(s, 127, 111), 0xff0000, 0);
    CHECK_RGB(SurfaceRgb(s, 64, 112), 0, 0);
}

// ---------------------------------------------------------------------------------------------
// Interpolation
// ---------------------------------------------------------------------------------------------

// The display's refresh rate, as SDL reports it (the offscreen driver says 0).
static void SetDisplayHz(float hz)
{
    SDL_DisplayMode *m =
        (SDL_DisplayMode *)SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(TheWindow()));
    CHECK(m != NULL);
    m->refresh_rate = hz;
}

TEST(engine_SysInterpolating_needs_a_window)
{
    InitVideo();
    SysSetInterpolation(SYS_INTERP_ON);
    CHECK(!SysInterpolating());
    CHECK(SysCreateWindow(64, 48, true, "wbengine", "software"));
    CHECK(SysInterpolating());
    SysDestroyWindow();
    CHECK(!SysInterpolating());
}

TEST(engine_SysInterpolating_on_and_off)
{
    OpenWindow(64, 48);
    SetDisplayHz(60);
    SysSetInterpolation(SYS_INTERP_ON);
    CHECK(SysInterpolating());
    // At 30 fps, where auto would interpolate.
    SysSetMaxFps(30);
    SysSetInterpolation(SYS_INTERP_OFF);
    CHECK(!SysInterpolating());
    // Not without a frame cap: there's no tick to interpolate within.
    SysSetInterpolation(SYS_INTERP_ON);
    SysSetMaxFps(0);
    CHECK(!SysInterpolating());
}

TEST(engine_SysInterpolating_auto_skips_a_matching_vsynced_display)
{
    OpenWindow(64, 48);
    SetDisplayHz(60);
    // The defaults: auto, vsync, 60 fps on a 60 Hz display.
    CHECK(!SysInterpolating());
    SysSetMaxFps(30);
    CHECK(SysInterpolating());
    SysSetMaxFps(60);
    SetDisplayHz(60.3f);
    CHECK(!SysInterpolating());
    SetDisplayHz(144);
    CHECK(SysInterpolating());
    SetDisplayHz(60);
    SysSetVSync(false);
    CHECK(SysInterpolating());
    SysSetVSync(true);
    CHECK(!SysInterpolating());
    // An unknown rate: interpolate.
    SetDisplayHz(0);
    CHECK(SysInterpolating());
}

TEST(engine_SysSetInterpolation_takes_unknown_modes_as_auto)
{
    OpenWindow(64, 48);
    SetDisplayHz(60);
    SysSetInterpolation(SYS_INTERP_ON);
    SysSetInterpolation(3);
    CHECK(!SysInterpolating());
    SysSetInterpolation(SYS_INTERP_ON);
    SysSetInterpolation(-1);
    CHECK(!SysInterpolating());
    SysSetInterpolation(SYS_INTERP_OFF);
    SysSetMaxFps(30);
    CHECK(!SysInterpolating());
    SysSetInterpolation(SYS_INTERP_AUTO);
    CHECK(SysInterpolating());
}

static char s_dir[512];

// Starts saving every frame shown, as BMPs in the current folder (a temp one).
static void SaveFrames(bool on)
{
    if (on && !s_dir[0]) {
        MakeTempDir(s_dir, sizeof s_dir);
        CHECK(chdir(s_dir) == 0);
    }
    SDL_SetHint(SDL_HINT_VIDEO_OFFSCREEN_SAVE_FRAMES, on ? "1" : "0");
}

static int CompareNames(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

// The x of the leftmost pixel of colour `rgb` in row y of each saved frame, in order, into
// xs (-1: none); returns the number of frames.
static int FramePositions(int y, Uint32 rgb, int *xs, int max)
{
    char *names[256];
    int n = 0, i, x;
    DIR *d = opendir(".");
    struct dirent *e;
    CHECK(d != NULL);
    while ((e = readdir(d)) != NULL && n < 256)
        if (strncmp(e->d_name, "SDL_window", 10) == 0)
            names[n++] = strdup(e->d_name);
    closedir(d);
    qsort(names, n, sizeof *names, CompareNames);
    CHECK(n <= max);
    for (i = 0; i < n; i++) {
        SDL_Surface *s = SDL_LoadBMP(names[i]);
        CHECK(s != NULL);
        xs[i] = -1;
        for (x = 0; x < s->w; x++)
            if (SurfaceRgb(s, x, y) == rgb) {
                xs[i] = x;
                break;
            }
        SDL_DestroySurface(s);
        remove(names[i]);
        free(names[i]);
    }
    return n;
}

// A tick: the back buffer cleared, a white 10x10 square at x.
static void Tick(int x)
{
    SysSetClearColor(0, 0, 0, 1);
    SysSetWorldView(0, 0, 0, 1, true);
    DrawRect((float)x, 10, (float)x + 10, 20, 1, 1, 1, 1);
}

// Interpolation on at 10 fps, without vsync (frames at 60 Hz), a tick shown with the square at
// x = 20, then the frames of a tick with it at `x` saved; their square positions into xs.
static int InterpolatedFrames(int x, int *xs, int max)
{
    OpenWindow(96, 32);
    SysSetVSync(false);
    SysSetInterpolation(SYS_INTERP_ON);
    SysSetMaxFps(10);
    SysFlip();
    Tick(20);
    SysFlip();
    Tick(x);
    SaveFrames(true);
    SysFlip();
    SaveFrames(false);
    return FramePositions(15, 0xffffff, xs, max);
}

TEST(engine_SysFlip_interpolates_between_ticks)
{
    int xs[64], n, i;
    bool between = false;
    n = InterpolatedFrames(50, xs, 64);
    CHECK_MSG(n >= 3 && n <= 8, "%d frames shown in a 100 ms tick at 60 Hz", n);
    for (i = 0; i < n; i++) {
        CHECK_MSG(xs[i] >= 20 && xs[i] <= 50, "frame %d: square at %d", i, xs[i]);
        if (i > 0)
            CHECK_MSG(xs[i] >= xs[i - 1], "frame %d: square went back from %d to %d", i,
                      xs[i - 1], xs[i]);
        between |= xs[i] > 20 && xs[i] < 50;
    }
    CHECK_MSG(xs[0] < 35, "the first frame shows the square at %d", xs[0]);
    CHECK(between);
    RmTree(s_dir);
}

TEST(engine_SysFlip_doesnt_interpolate_a_far_jump)
{
    int xs[64], n, i;
    // 34 pixels: further than a draw moves in a tick.
    n = InterpolatedFrames(54, xs, 64);
    CHECK(n >= 3);
    for (i = 0; i < n; i++)
        CHECK_MSG(xs[i] == 54, "frame %d: square at %d", i, xs[i]);
    RmTree(s_dir);
}

// Interpolation on at 10 fps without vsync, and one tick shown.
static void StartInterpolating(void)
{
    OpenWindow(96, 32);
    SysSetVSync(false);
    SysSetInterpolation(SYS_INTERP_ON);
    SysSetMaxFps(10);
    SysFlip();
}

TEST(engine_SysFlip_interpolates_each_draw_from_its_nearest)
{
    int xs[64], n, i;
    // Squares at 26 and 50, then one at 30: it comes from 26, not 50.
    StartInterpolating();
    Tick(26);
    DrawRect(50, 10, 60, 20, 1, 1, 1, 1);
    SysFlip();
    Tick(30);
    SaveFrames(true);
    SysFlip();
    SaveFrames(false);
    n = FramePositions(15, 0xffffff, xs, 64);
    CHECK(n >= 3);
    CHECK_MSG(xs[0] < 30, "first frame: square at %d", xs[0]);
    for (i = 0; i < n; i++)
        CHECK_MSG(xs[i] >= 26 && xs[i] <= 30, "frame %d: square at %d", i, xs[i]);
    RmTree(s_dir);
}

TEST(engine_SysFlip_interpolation_keeps_what_isnt_redrawn)
{
    int xs[64], n, i;
    // The square drawn once stays in the back buffer, and in every frame shown.
    StartInterpolating();
    Tick(20);
    SysFlip();
    PlotPixel(90, 30, 1, 0, 0, 1);
    SaveFrames(true);
    SysFlip();
    SaveFrames(false);
    n = FramePositions(15, 0xffffff, xs, 64);
    CHECK(n >= 3);
    for (i = 0; i < n; i++)
        CHECK_MSG(xs[i] == 20, "frame %d: square at %d", i, xs[i]);
    RmTree(s_dir);
}

TEST(engine_SysFlip_doesnt_interpolate_different_draws)
{
    int xs[64], n, i;
    // A white square becomes a blended red one: a different draw, shown where it is.
    StartInterpolating();
    Tick(20);
    SysFlip();
    SysSetWorldView(0, 0, 0, 1, true);
    DrawRect(40, 10, 50, 20, 1, 0, 0, 0.99f);
    SaveFrames(true);
    SysFlip();
    SaveFrames(false);
    n = FramePositions(15, 0xfc0000, xs, 64);
    CHECK(n >= 3);
    for (i = 0; i < n; i++)
        CHECK_MSG(xs[i] == 40, "frame %d: square at %d", i, xs[i]);
    RmTree(s_dir);
}

TEST(engine_SysFlip_without_interpolation_shows_each_tick_once)
{
    int xs[8], n;
    OpenWindow(96, 32);
    SysSetVSync(false);
    SysSetInterpolation(SYS_INTERP_OFF);
    SysSetMaxFps(10);
    SysFlip();
    Tick(20);
    SysFlip();
    Tick(50);
    SaveFrames(true);
    SysFlip();
    SaveFrames(false);
    n = FramePositions(15, 0xffffff, xs, 8);
    CHECK_EQ_INT(n, 1);
    CHECK_EQ_INT(xs[0], 50);
    RmTree(s_dir);
}

TEST(engine_SysDesktopWidth_and_Height_are_the_usable_desktop)
{
    SDL_Rect r;
    InitVideo();
    CHECK(SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &r));
    CHECK_EQ_INT(SysDesktopWidth(), r.w);
    CHECK_EQ_INT(SysDesktopHeight(), r.h);
    CHECK(r.w != r.h);
}
