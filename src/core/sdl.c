// sdl.c: The engine interface (include/sdlhelp.h) on SDL3 and SDL3_image: window, frame
// loop, time, files, the data archive, images and input. sdl_audio.c has the audio half.
//
// PTK's behaviour is kept where the game can see it (SDL_PLAN.md Step 4):
// - The game draws into an 800x600 back buffer (a render-target texture), which persists
//   between frames; SysFlip scales it into the window, letterboxed.
// - SysFlip caps the frame rate the way PTK did: at least 1000/fps whole milliseconds per
//   frame (16 ms, i.e. 62.5 fps, for 60), and it waits while the window is in the background.
//   With vsync on (the default) a 60 Hz display sets the pace instead.
// - On other displays SysFlip can interpolate: each tick's draws are recorded, matched to the
//   previous tick's and shown blended at the display's rate, while the game still ticks at
//   the rate above (see "Interpolation" below).
// - Blits, stretches and primitives follow KGraphicGL: blend factors per alpha mode, tint times
//   blend for the alpha, rotation (counter-clockwise, in degrees) about the destination centre
//   plus (centerX, centerY).
// - Key codes stay PTK's (they are stored in WarBlade.inf); letters, digits and punctuation
//   follow the keyboard layout, as Windows virtual keys did.
#include <string.h>
#ifndef _WIN32
#include <dirent.h>
#include <unistd.h>
#endif
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "sdlhelp.h"

// Audio slides that need the game thread (sdl_audio.c).
void AudioTick(void);

static SDL_Window   *s_window;
static SDL_Renderer *s_renderer;
static SDL_Texture  *s_canvas;       // the back buffer; always the render target between flips
static SDL_ScaleMode s_canvasScaleMode = SDL_SCALEMODE_PIXELART;
static int           s_width, s_height;
static bool          s_fullscreen;
static bool          s_quit;
static bool          s_focused = true;
static void        (*s_focusFn)(bool focused);
static int           s_fps = 60;         // SysSetMaxFps
static int           s_frameMs = 16;     // PTK: 1000 / fps, whole milliseconds; 0 = no cap
static Uint64        s_lastFlipNs;
static unsigned      s_frame;            // flips so far
static float         s_clearR, s_clearG, s_clearB, s_clearA = 1.0f;
static bool          s_vsync = true;
static int           s_interpMode = SYS_INTERP_AUTO;

// Interpolation state (see "Interpolation").
static SDL_Texture  *s_base;             // the back buffer as it was before this tick's draws
static SDL_Texture  *s_view;             // the picture at window resolution (RenderInterpolated)
static bool          s_recording;        // this tick's draws are being recorded
static Uint64        s_tickDueNs;        // when the tick being shown was due to start
static Uint64        s_nextFrameNs;      // without vsync: when the next interpolated frame is due

static void RecordFill(SDL_FRect rect, SDL_BlendMode blend, float r, float g, float b, float a);

// ---------------------------------------------------------------------------------------------
// Window, frame and events
// ---------------------------------------------------------------------------------------------

void SysInit(void)
{
    SDL_SetAppMetadata("Warblade", "2.0", "as.warblade.warblade");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD))
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
}

// The rectangle (window pixels) the back buffer is shown in: as large as fits, centred.
// SDL_GetRenderOutputSize, not SDL_GetCurrentRenderOutputSize: between flips the render target
// is the back buffer, whose size the latter would return.
static SDL_FRect OutputRect(void)
{
    SDL_FRect r;
    int pw = s_width, ph = s_height;
    float scale;

    SDL_GetRenderOutputSize(s_renderer, &pw, &ph);
    scale = SDL_min((float)pw / s_width, (float)ph / s_height);
    r.w = s_width * scale;
    r.h = s_height * scale;
    r.x = (pw - r.w) / 2;
    r.y = (ph - r.h) / 2;
    return r;
}

// Window (mouse) coordinates to back-buffer coordinates and back.
static void WindowToCanvas(float wx, float wy, float *cx, float *cy)
{
    int ww = 1, wh = 1, pw = 1, ph = 1;
    SDL_FRect r;

    SDL_GetWindowSize(s_window, &ww, &wh);
    SDL_GetWindowSizeInPixels(s_window, &pw, &ph);
    r = OutputRect();
    *cx = (wx * pw / ww - r.x) * s_width / r.w;
    *cy = (wy * ph / wh - r.y) * s_height / r.h;
}

static void CanvasToWindow(float cx, float cy, float *wx, float *wy)
{
    int ww = 1, wh = 1, pw = 1, ph = 1;
    SDL_FRect r;

    SDL_GetWindowSize(s_window, &ww, &wh);
    SDL_GetWindowSizeInPixels(s_window, &pw, &ph);
    r = OutputRect();
    *wx = (cx * r.w / s_width + r.x) * ww / pw;
    *wy = (cy * r.h / s_height + r.y) * wh / ph;
}

bool SysCreateWindow(int w, int h, bool windowed, const char *title, const char *renderDriver)
{
    SDL_WindowFlags flags;
    bool ok;

#ifdef __EMSCRIPTEN__
    // A page can only go fullscreen from a click or key press: start in the page (W and the
    // settings page still switch).
    windowed = true;
#endif
    flags = windowed ? SDL_WINDOW_RESIZABLE : SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN;
#ifdef __EMSCRIPTEN__
    // Keep CSS/input coordinates in logical pixels while presenting at the
    // display density. Otherwise thin bitmap strokes are lost in a low-resolution
    // canvas before a phone's browser enlarges it again.
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif

    SysDestroyWindow();
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderDriver);     // NULL clears it
    ok = SDL_CreateWindowAndRenderer(title, w, h, flags, &s_window, &s_renderer);
    SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
    if (!ok) {
        SDL_Log("SDL_CreateWindowAndRenderer (%s) failed: %s", renderDriver ? renderDriver : "auto",
                SDL_GetError());
        s_window = NULL;
        s_renderer = NULL;
        return false;
    }
    // Vsync presents on the vertical blank (no tearing); SysFlip's frame cap still sets the pace.
    SysSetVSync(s_vsync);
    s_canvas = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    s_base = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (s_canvas == NULL || s_base == NULL) {
        SDL_Log("SDL_CreateTexture (back buffer) failed: %s", SDL_GetError());
        SysDestroyWindow();
        return false;
    }
    SDL_SetTextureBlendMode(s_canvas, SDL_BLENDMODE_NONE);
    SDL_SetTextureBlendMode(s_base, SDL_BLENDMODE_NONE);
    // Pixel-art scaling keeps the 800x600 picture (and its bitmap fonts) sharp in large windows.
    s_canvasScaleMode = SDL_SCALEMODE_PIXELART;
    SDL_SetTextureScaleMode(s_canvas, s_canvasScaleMode);
    SDL_SetTextureScaleMode(s_base, SDL_SCALEMODE_PIXELART);
    s_recording = false;
    s_width = w;
    s_height = h;
    s_fullscreen = !windowed;
    s_quit = false;
    s_focused = true;
    SDL_SetRenderTarget(s_renderer, s_canvas);
    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    s_lastFlipNs = SDL_GetTicksNS();
    return true;
}

void SysDestroyWindow(void)
{
    s_recording = false;
    if (s_canvas) {
        SDL_DestroyTexture(s_canvas);
        s_canvas = NULL;
    }
    if (s_base) {
        SDL_DestroyTexture(s_base);
        s_base = NULL;
    }
    if (s_view) {
        SDL_DestroyTexture(s_view);
        s_view = NULL;
    }
    if (s_renderer) {
        SDL_DestroyRenderer(s_renderer);
        s_renderer = NULL;
    }
    if (s_window) {
        SDL_DestroyWindow(s_window);
        s_window = NULL;
    }
}

bool SysHasWindow(void)          { return s_window != NULL; }

const char *SysRendererName(void)
{
    const char *name = s_renderer ? SDL_GetRendererName(s_renderer) : NULL;
    return name ? name : "";
}
void SysTerminate(void)          { s_quit = true; }
bool SysQuitRequested(void)      { return s_quit; }
void SysMinimize(void)           { SDL_MinimizeWindow(s_window); }
bool SysHasFocus(void)           { return s_focused; }
void SysDisableScreenSaver(void) { SDL_DisableScreenSaver(); }

void SysSetFocusCallback(void (*fn)(bool focused))
{
    s_focusFn = fn;
}

void SysSetFullscreen(const bool fullscreen)
{
    if (s_fullscreen == fullscreen)
        return;

    SDL_SetWindowFullscreen(s_window, fullscreen);
    s_fullscreen = fullscreen;
}

static void PadAdded(SDL_JoystickID id);
static void PadRemoved(SDL_JoystickID id);

static void HandleEvent(const SDL_Event *e)
{
    switch (e->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
        PadAdded(e->gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        PadRemoved(e->gdevice.which);
        break;
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        s_quit = true;
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        s_focused = true;
        if (s_focusFn)
            s_focusFn(true);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        s_focused = false;
        if (s_focusFn)
            s_focusFn(false);
        break;
    }
}

void SysProcessEvents(void)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
        HandleEvent(&e);
    AudioTick();
}

// Like PTK, the game stands still while its window is in the background.
static void WaitForFocus(void)
{
    while (!s_focused && !s_quit) {
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, 100))
            HandleEvent(&e);
        AudioTick();
    }
}

// The refresh rate of the window's display in Hz, 0 if unknown.
static float RefreshRate(void)
{
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(s_window));
    return mode ? mode->refresh_rate : 0.0f;
}

// Whether frames are interpolated. Auto turns it off where it gains nothing: vsynced on a
// display whose refresh rate is the game's frame rate, where each tick gets one refresh.
static bool WantInterpolation(void)
{
    float hz;

#ifdef __EMSCRIPTEN__
    return false;   // the browser paces the page; frames are shown as they are made
#endif

    if (s_frameMs == 0 || s_base == NULL || s_interpMode == SYS_INTERP_OFF)
        return false;
    if (s_interpMode == SYS_INTERP_ON)
        return true;
    hz = RefreshRate();
    return !(s_vsync && hz > 0 && SDL_fabsf(hz - s_fps) < 0.5f);
}

static void FlipInterpolated(void);
static void StartRecording(bool fresh);

// Shows the back buffer as it is, then waits for the frame cap.
static void FlipPlain(void)
{
    SDL_FRect dst;
    Uint64 frameNs;
    Uint64 now;

    // Show the back buffer, letterboxed, and go back to drawing into it.
    SDL_SetRenderTarget(s_renderer, NULL);
    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    dst = OutputRect();
    // Average neighbouring pixels when shrinking, preserving coverage of thin
    // text strokes. Pixel-art magnification still keeps the original look.
    SDL_ScaleMode scaleMode = SDL_SCALEMODE_PIXELART;
#ifdef __EMSCRIPTEN__
    // WebGL supports filtered render-target minification. Keep the standalone
    // software renderer's presentation path unchanged; its linear target copy
    // can produce a black surface with the pinned SDL version.
    if (dst.w < s_width || dst.h < s_height)
        scaleMode = SDL_SCALEMODE_LINEAR;
#endif
    if (scaleMode != s_canvasScaleMode) {
        SDL_SetTextureScaleMode(s_canvas, scaleMode);
        s_canvasScaleMode = scaleMode;
    }
    SDL_RenderTexture(s_renderer, s_canvas, NULL, &dst);
    SDL_RenderPresent(s_renderer);
    SDL_SetRenderTarget(s_renderer, s_canvas);

    SysProcessEvents();

    // Frame cap: at least s_frameMs since the end of the previous flip.
    frameNs = (Uint64)s_frameMs * SDL_NS_PER_MS;
    now = SDL_GetTicksNS();
    if (frameNs && now - s_lastFlipNs < frameNs)
        SDL_DelayPrecise(frameNs - (now - s_lastFlipNs));
#ifdef __EMSCRIPTEN__
    else
        emscripten_sleep(0);   // the browser shows the frame (and runs events) only once we yield
#endif

    WaitForFocus();
    s_lastFlipNs = SDL_GetTicksNS();
    s_tickDueNs = s_lastFlipNs;         // the next tick starts now
}

void SysFlip(void)
{
    bool wasRecording = s_recording;

    s_frame++;
    // Interpolation needs this tick's draws, so it starts (or resumes) on the next tick.
    if (wasRecording && WantInterpolation())
        FlipInterpolated();
    else
        FlipPlain();

    s_recording = WantInterpolation();
    if (s_recording)
        StartRecording(!wasRecording);
}

void SysSetMaxFps(int fps)
{
    s_fps = fps;
    s_frameMs = fps > 0 ? 1000 / fps : 0;
}

void SysSetVSync(bool on)
{
    s_vsync = on;
    if (s_renderer && !SDL_SetRenderVSync(s_renderer, on ? 1 : SDL_RENDERER_VSYNC_DISABLED))
        SDL_Log("SDL_SetRenderVSync failed: %s", SDL_GetError());
}

void SysSetInterpolation(int mode)
{
    s_interpMode = mode >= 0 && mode < SYS_INTERP_COUNT ? mode : SYS_INTERP_AUTO;
}

bool SysInterpolating(void)
{
    return WantInterpolation();
}

void SysSetClearColor(float r, float g, float b, float a)
{
    s_clearR = r;
    s_clearG = g;
    s_clearB = b;
    s_clearA = a;
}

// The game only ever sets the identity view; `clear` clears the back buffer.
void SysSetWorldView(float x, float y, float rotation, float zoom, bool clear)
{
    if (x != 0 || y != 0 || rotation != 0 || zoom != 1.0f)
        SDL_Log("SysSetWorldView: only the identity view is supported (%g %g %g %g)", x, y, rotation, zoom);
    if (clear) {
        SDL_FRect all = { 0, 0, (float)s_width, (float)s_height };
        SDL_SetRenderDrawBlendMode(s_renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColorFloat(s_renderer, s_clearR, s_clearG, s_clearB, 1.0f);
        SDL_RenderClear(s_renderer);
        RecordFill(all, SDL_BLENDMODE_NONE, s_clearR, s_clearG, s_clearB, 1.0f);
    }
}

void SysSetIcon(const char *icoFile)
{
    SDL_Surface *icon = IMG_Load(SysPath(icoFile));
    if (icon) {
        SDL_SetWindowIcon(s_window, icon);
        SDL_DestroySurface(icon);
    }
}

bool SysScreenshot(const char *file, int w, int h)
{
    SDL_Surface *shot = SDL_RenderReadPixels(s_renderer, NULL);
    bool ok;

    if (shot == NULL)
        return false;
    if (shot->w != w || shot->h != h) {
        SDL_Surface *scaled = SDL_ScaleSurface(shot, w, h, SDL_SCALEMODE_LINEAR);
        SDL_DestroySurface(shot);
        shot = scaled;
        if (shot == NULL)
            return false;
    }
    ok = IMG_SaveJPG(shot, SysPath(file), 90);
    SDL_DestroySurface(shot);
    return ok;
}

static SDL_Rect UsableDesktop(void)
{
    SDL_Rect r = { 0, 0, 0, 0 };
    SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &r);
    return r;
}

int SysDesktopWidth(void)  { return UsableDesktop().w; }
int SysDesktopHeight(void) { return UsableDesktop().h; }

void SysMessageBox(const char *title, const char *text)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, text, s_window);
}

// ---------------------------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------------------------

// The game subtracts up to a few minutes from this clock, so it must not start near 0
// (timeGetTime counted from boot).
#define MILLIS_OFFSET 86400000u

unsigned SysMillis(void)
{
    return (unsigned)SDL_GetTicks() + MILLIS_OFFSET;
}

long long SysPerfCounter(void) { return (long long)SDL_GetPerformanceCounter(); }
long long SysPerfFreq(void)    { return (long long)SDL_GetPerformanceFrequency(); }

// FILETIME counts 100 ns units from 1601-01-01; SDL_Time counts ns from 1970-01-01.
#define FILETIME_1970 116444736000000000LL

long long SysFileTimeNow(void)
{
    SDL_Time t = 0;
    SDL_GetCurrentTime(&t);
    return t / 100 + FILETIME_1970;
}

static void ToSysDate(SDL_Time t, bool local, SysDate *out)
{
    SDL_DateTime dt;

    memset(out, 0, sizeof(*out));
    if (!SDL_TimeToDateTime(t, &dt, local))
        return;
    out->year = (unsigned short)dt.year;
    out->month = (unsigned short)dt.month;
    out->dayOfWeek = (unsigned short)dt.day_of_week;
    out->day = (unsigned short)dt.day;
    out->hour = (unsigned short)dt.hour;
    out->minute = (unsigned short)dt.minute;
    out->second = (unsigned short)dt.second;
    out->milliseconds = (unsigned short)(dt.nanosecond / 1000000);
}

// Worked out here rather than through SDL_Time: the game also converts durations (FILETIMEs
// near 1601), which are outside SDL_Time's range of about 1678-2262.
void SysFileTimeToDate(long long ft, SysDate *out)
{
    long long secs, days, z, era, doe, yoe, doy, mp, y;
    long long rem;

    memset(out, 0, sizeof(*out));
    if (ft < 0)
        return;
    secs = ft / 10000000;
    days = secs / 86400;                    // since 1601-01-01, a Monday
    rem = secs % 86400;
    out->dayOfWeek = (unsigned short)((days + 1) % 7);
    out->hour = (unsigned short)(rem / 3600);
    out->minute = (unsigned short)(rem / 60 % 60);
    out->second = (unsigned short)(rem % 60);
    out->milliseconds = (unsigned short)(ft / 10000 % 1000);

    // Civil date from a day count (H. Hinnant's days_from_civil, inverted), from 0000-03-01.
    z = days + 584694;                      // 1601-01-01 is day 584694 after 0000-03-01
    era = z / 146097;
    doe = z - era * 146097;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    out->day = (unsigned short)(doy - (153 * mp + 2) / 5 + 1);
    out->month = (unsigned short)(mp < 10 ? mp + 3 : mp - 9);
    out->year = (unsigned short)(out->month <= 2 ? y + 1 : y);
}

void SysUtcDate(SysDate *out)
{
    SDL_Time t = 0;
    SDL_GetCurrentTime(&t);
    ToSysDate(t, false, out);
}

void SysLocalDate(SysDate *out)
{
    SDL_Time t = 0;
    SDL_GetCurrentTime(&t);
    ToSysDate(t, true, out);
}

// ---------------------------------------------------------------------------------------------
// Files, folders, programs
// ---------------------------------------------------------------------------------------------

static void CopyPath(char *dst, size_t size, const char *src)
{
    size_t n;
    SDL_strlcpy(dst, src ? src : "", size);
    n = strlen(dst);
    while (n > 0 && (dst[n - 1] == '\\' || dst[n - 1] == '/'))
        dst[--n] = 0;
}

const char *SysUserFolder(void)
{
    static char folder[1024];
    if (folder[0] == 0) {
        const char *docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
#ifdef __EMSCRIPTEN__
        // The page keeps this folder in memory; durable saves belong to the server account.
        docs = "/save";
#elif !defined(_WIN32)
        // Not every Linux home has a Documents folder.
        if (docs == NULL || !SDL_GetPathInfo(docs, NULL))
            docs = SDL_GetUserFolder(SDL_FOLDER_HOME);
#endif
        CopyPath(folder, sizeof(folder), docs ? docs : SDL_GetBasePath());
    }
    return folder;
}

const char *SysAppPath(const char *rel)
{
    static char path[1024];
    const char *base = SDL_GetBasePath();
    SDL_snprintf(path, sizeof(path), "%s%s", base ? base : "", rel);
    return path;
}

#ifdef _WIN32
const char *SysPath(const char *path)
{
    return path;
}
#else
// Finds the entry of folder `dir` named `name` ignoring case, into `out`; false if none.
static bool FindNoCase(const char *dir, const char *name, char *out, size_t size)
{
    DIR *d = opendir(dir[0] ? dir : ".");
    struct dirent *e;
    bool found = false;

    if (d == NULL)
        return false;
    while ((e = readdir(d)) != NULL) {
        if (SDL_strcasecmp(e->d_name, name) == 0) {
            SDL_strlcpy(out, e->d_name, size);
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

const char *SysPath(const char *path)
{
    static char bufs[4][1024];
    static int next;
    char *out = bufs[next];
    const size_t size = sizeof(bufs[0]);
    char name[256];
    char real[256];
    size_t len;
    const char *p = path;

    next = (next + 1) % 4;
    out[0] = 0;
    if (*p == '/' || *p == '\\')
        SDL_strlcpy(out, "/", size);
    while (*p) {
        size_t n;
        while (*p == '/' || *p == '\\')
            p++;
        for (n = 0; p[n] && p[n] != '/' && p[n] != '\\'; n++)
            ;
        if (n == 0)
            break;
        SDL_strlcpy(name, p, n + 1 < sizeof(name) ? n + 1 : sizeof(name));
        p += n;

        // `out` is the folder so far ("" for the current one); add this component to it,
        // spelled as on disk if only a different case exists.
        len = strlen(out);
        if (len > 0 && out[len - 1] != '/' && len + 1 < size)
            out[len++] = '/';
        out[len] = 0;
        SDL_strlcat(out, name, size);
        if (access(out, F_OK) != 0) {
            out[len] = 0;
            if (FindNoCase(out, name, real, sizeof(real)))
                SDL_strlcat(out, real, size);
            else
                SDL_strlcat(out, name, size);
        }
    }
    return out;
}
#endif

bool SysFileExists(const char *path)
{
    return SDL_GetPathInfo(SysPath(path), NULL);
}

bool SysMakeDir(const char *path)
{
    return SDL_CreateDirectory(SysPath(path));
}

void SysOpenUrl(const char *url)
{
    SDL_OpenURL(url);
}

// The original brought an already running jukebox to the front instead; a second instance
// starts here.
void SysOpenJukebox(void)
{
    char path[1024];
    const char *args[2];
    SDL_Process *proc;

    SDL_strlcpy(path, SysAppPath("Jukebox.exe"), sizeof(path));
    args[0] = path;
    args[1] = NULL;
    proc = SDL_CreateProcess(args, false);
    if (proc)
        SDL_DestroyProcess(proc);   // only the handle: the jukebox keeps running
    else
        SDL_Log("Could not start %s: %s", path, SDL_GetError());
}

// The data archive: a ustar file, indexed once. Names are looked up case-insensitively, as
// PTK's KResource did ("alien_10.tga" is "Alien_10.tga" in the archive).
typedef struct PacEntry {
    char name[256];
    Sint64 offset;
    Sint64 size;
} PacEntry;

static SDL_IOStream *s_pac;
static PacEntry     *s_pacEntries;
static int           s_pacCount;

static Sint64 OctalField(const unsigned char *p, int n)
{
    Sint64 v = 0;
    int i;
    for (i = 0; i < n && p[i] >= '0' && p[i] <= '7'; i++)
        v = v * 8 + (p[i] - '0');
    return v;
}

void PacAddArchive(const char *path)
{
    unsigned char hdr[512];
    Sint64 pos = 0;
    int cap = 0;

    s_pac = SDL_IOFromFile(SysPath(path), "rb");
    if (s_pac == NULL) {
        SDL_Log("Could not open %s: %s", path, SDL_GetError());
        return;
    }
    while (SDL_ReadIO(s_pac, hdr, 512) == 512 && hdr[0] != 0) {
        Sint64 size = OctalField(hdr + 124, 12);
        char type = (char)hdr[156];
        if (type == '0' || type == 0) {
            PacEntry *e;
            if (s_pacCount == cap) {
                cap = cap ? cap * 2 : 1024;
                s_pacEntries = (PacEntry *)SDL_realloc(s_pacEntries, cap * sizeof(PacEntry));
            }
            e = &s_pacEntries[s_pacCount++];
            if (memcmp(hdr + 257, "ustar", 5) == 0 && hdr[345] != 0)
                SDL_snprintf(e->name, sizeof(e->name), "%.155s/%.100s", (const char *)hdr + 345, (const char *)hdr);
            else
                SDL_snprintf(e->name, sizeof(e->name), "%.100s", (const char *)hdr);
            e->offset = pos + 512;
            e->size = size;
        }
        pos += 512 + (size + 511) / 512 * 512;
        if (SDL_SeekIO(s_pac, pos, SDL_IO_SEEK_SET) < 0)
            break;
    }
}

static const PacEntry *PacFind(const char *name)
{
    int i;
    for (i = 0; i < s_pacCount; i++) {
        if (SDL_strcasecmp(s_pacEntries[i].name, name) == 0)
            return &s_pacEntries[i];
    }
    return NULL;
}

// Reads a whole file, from the archive or else from the disk. SDL_free the result.
static void *PacLoad(const char *name, size_t *size)
{
    const PacEntry *e = PacFind(name);
    void *data;

    if (e == NULL)
        return SDL_LoadFile(SysPath(name), size);
    data = SDL_malloc((size_t)e->size + 1);
    if (data == NULL)
        return NULL;
    if (SDL_SeekIO(s_pac, e->offset, SDL_IO_SEEK_SET) < 0 ||
        SDL_ReadIO(s_pac, data, (size_t)e->size) != (size_t)e->size) {
        SDL_free(data);
        return NULL;
    }
    ((char *)data)[e->size] = 0;
    *size = (size_t)e->size;
    return data;
}

bool PacExists(const char *name)
{
    return PacFind(name) != NULL || SDL_GetPathInfo(SysPath(name), NULL);
}

// A file shorter than `size` fills only its own length of `buf`.
bool PacRead(const char *name, void *buf, unsigned size)
{
    size_t n = 0;
    void *data = PacLoad(name, &n);
    if (data == NULL)
        return false;
    memcpy(buf, data, n < size ? n : size);
    SDL_free(data);
    return true;
}

// ---------------------------------------------------------------------------------------------
// Images
// ---------------------------------------------------------------------------------------------

struct Image {
    SDL_Texture *tex;           // NULL after ImgFreePicture
    float w, h;
    bool freed;                 // ImgFree'd; the struct itself goes a few frames later
    unsigned freedFrame;
    Image *nextFreed;
    bool tinted;                // setBlitColor with anything but (1, 1, 1, 1)
    float r, g, b, a;
    short mode;                 // PTK alpha mode
    bool linear;                // setTextureQuality
};

static void RecordBlit(Image *img, SDL_FRect src, SDL_FRect dst, float angle, SDL_FPoint pivot,
                       SDL_FlipMode flip, float alpha);

// Freed images wait here until no render queue can still hold them (see ImgIsFreed).
static Image *s_freedImages;

static void ReleaseFreedImages(bool all)
{
    Image **p = &s_freedImages;
    while (*p) {
        Image *img = *p;
        if (all || s_frame - img->freedFrame > 2) {
            *p = img->nextFreed;
            SDL_free(img);
        } else {
            p = &img->nextFreed;
        }
    }
}

// KGraphicGL::setAlphaMode's blend factors (src, dst). The back buffer's alpha is kept at 1,
// so mode 0's DST_ALPHA is 1.
static SDL_BlendMode BlendModeFor(short mode)
{
    SDL_BlendFactor src = SDL_BLENDFACTOR_SRC_ALPHA;
    SDL_BlendFactor dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

    switch (mode) {
    case 0: src = SDL_BLENDFACTOR_SRC_ALPHA;           dst = SDL_BLENDFACTOR_DST_ALPHA; break;
    case 1: src = SDL_BLENDFACTOR_SRC_ALPHA;           dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; break;
    case 2: src = SDL_BLENDFACTOR_ONE;                 dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; break;
    case 3: src = SDL_BLENDFACTOR_ZERO;                dst = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; break;
    case 4: src = SDL_BLENDFACTOR_SRC_ALPHA;           dst = SDL_BLENDFACTOR_ONE; break;
    case 5: src = SDL_BLENDFACTOR_ONE;                 dst = SDL_BLENDFACTOR_ZERO; break;
    case 6: src = SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; dst = SDL_BLENDFACTOR_ONE; break;
    }
    return SDL_ComposeCustomBlendMode(src, dst, SDL_BLENDOPERATION_ADD,
                                      SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
}

Image *ImgLoad(const char *file, bool hiDef, bool hasAlpha)
{
    size_t size = 0;
    void *data;
    const char *ext;
    SDL_Surface *surf;
    SDL_Texture *tex;
    Image *img;

    (void)hiDef;
    (void)hasAlpha;
    data = PacLoad(file, &size);
    if (data == NULL)
        return NULL;
    // TGA has no magic number, so the type comes from the extension.
    ext = strrchr(file, '.');
    surf = IMG_LoadTyped_IO(SDL_IOFromConstMem(data, size), true, ext ? ext + 1 : NULL);
    SDL_free(data);
    if (surf == NULL) {
        SDL_Log("Could not decode %s: %s", file, SDL_GetError());
        return NULL;
    }
    tex = SDL_CreateTextureFromSurface(s_renderer, surf);
    SDL_DestroySurface(surf);
    if (tex == NULL) {
        SDL_Log("Could not create a texture for %s: %s", file, SDL_GetError());
        return NULL;
    }
    img = (Image *)SDL_calloc(1, sizeof(Image));
    img->tex = tex;
    img->w = (float)tex->w;
    img->h = (float)tex->h;
    img->r = img->g = img->b = img->a = 1.0f;
    img->mode = 1;      // KGraphicGL's default
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    return img;
}

void ImgFreePicture(Image *img)
{
    if (img->tex) {
        SDL_DestroyTexture(img->tex);
        img->tex = NULL;
    }
}

void ImgFree(Image *img)
{
    if (img == NULL || img->freed)
        return;
    ImgFreePicture(img);
    img->freed = true;
    img->freedFrame = s_frame;
    img->nextFreed = s_freedImages;
    s_freedImages = img;
    ReleaseFreedImages(false);
}

bool  ImgIsFreed(const Image *img) { return img->freed; }
float ImgWidth(Image *img)         { return img->w; }
float ImgHeight(Image *img)        { return img->h; }

// Applied per blit (Draw), as the interpolated frames draw with their own scale modes.
void ImgSetTextureQuality(Image *img, bool quality)
{
    img->linear = quality;
}

void ImgSetAlphaMode(Image *img, short mode)
{
    if (mode >= 0 && mode <= 6)
        img->mode = mode;
}

// Components above 1 are clamped, as PTK did.
void ImgSetBlitColor(Image *img, float r, float g, float b, float a)
{
    img->r = SDL_min(r, 1.0f);
    img->g = SDL_min(g, 1.0f);
    img->b = SDL_min(b, 1.0f);
    img->a = SDL_min(a, 1.0f);
    img->tinted = !(r == 1.0f && g == 1.0f && b == 1.0f && a == 1.0f);
}

// Draws the source rect (sx1,sy1)-(sx2,sy2) into dst, rotated by `angle` degrees
// counter-clockwise about dst's corner + (px, py), with alpha `alpha`. A source rect given
// backwards (x2 < x1) is drawn mirrored, as in PTK.
static void Draw(Image *img, float sx1, float sy1, float sx2, float sy2, SDL_FRect dst,
                 float angle, float px, float py, bool flipX, bool flipY, float alpha)
{
    SDL_FRect src;
    SDL_FPoint pivot;
    int flip = SDL_FLIP_NONE;

    if (img == NULL || img->tex == NULL)
        return;
    if (sx2 < sx1) {
        float t = sx1; sx1 = sx2; sx2 = t;
        flipX = !flipX;
    }
    if (sy2 < sy1) {
        float t = sy1; sy1 = sy2; sy2 = t;
        flipY = !flipY;
    }
    if (dst.w < 0) {
        dst.x += dst.w;
        dst.w = -dst.w;
        px = dst.w - px;
        flipX = !flipX;
    }
    if (dst.h < 0) {
        dst.y += dst.h;
        dst.h = -dst.h;
        py = dst.h - py;
        flipY = !flipY;
    }
    src.x = sx1;
    src.y = sy1;
    src.w = sx2 - sx1;
    src.h = sy2 - sy1;
    pivot.x = px;
    pivot.y = py;
    if (flipX)
        flip |= SDL_FLIP_HORIZONTAL;
    if (flipY)
        flip |= SDL_FLIP_VERTICAL;

    alpha = SDL_clamp(alpha, 0.0f, 1.0f);
    if (img->tinted)
        SDL_SetTextureColorModFloat(img->tex, img->r, img->g, img->b);
    else
        SDL_SetTextureColorModFloat(img->tex, 1.0f, 1.0f, 1.0f);
    SDL_SetTextureAlphaModFloat(img->tex, alpha);
    SDL_SetTextureBlendMode(img->tex, BlendModeFor(img->mode));
    SDL_SetTextureScaleMode(img->tex, img->linear ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
    SDL_RenderTextureRotated(s_renderer, img->tex, &src, &dst, -angle, &pivot, (SDL_FlipMode)flip);
    RecordBlit(img, src, dst, -angle, pivot, (SDL_FlipMode)flip, alpha);
}

static SDL_FRect Rect(float x, float y, float w, float h)
{
    SDL_FRect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

void ImgBlitAlphaRect(Image *img, float x1, float y1, float x2, float y2,
                      short destX, short destY, bool flipX, bool flipY)
{
    float w = SDL_fabsf(x2 - x1), h = SDL_fabsf(y2 - y1);
    Draw(img, x1, y1, x2, y2, Rect(destX, destY, w, h), 0, 0, 0, flipX, flipY,
         img->tinted ? img->a : 1.0f);
}

// PTK's "non-alpha" blit blends exactly like blitAlphaRect.
void ImgBlitRectF(Image *img, float x1, float y1, float x2, float y2,
                  float destX, float destY, bool flipX, bool flipY)
{
    float w = SDL_fabsf(x2 - x1), h = SDL_fabsf(y2 - y1);
    Draw(img, x1, y1, x2, y2, Rect(destX, destY, w, h), 0, 0, 0, flipX, flipY,
         img->tinted ? img->a : 1.0f);
}

// Rotated and zoomed about the centre of the destination rect + (centerX, centerY). Nothing
// is drawn at zoom 0 or blend <= 0.
void ImgBlitAlphaRectFx(Image *img, float x1, float y1, float x2, float y2,
                        short destX, short destY, float angle, float zoom, float blend,
                        bool flipX, bool flipY, float centerX, float centerY)
{
    float w = SDL_fabsf(x2 - x1), h = SDL_fabsf(y2 - y1);
    float px = w / 2 + centerX, py = h / 2 + centerY;

    if (zoom == 0 || blend <= 0)
        return;
    if (zoom < 0) {                 // a negative scale turns the picture half a turn
        zoom = -zoom;
        angle += 180.0f;
    }
    Draw(img, x1, y1, x2, y2, Rect(destX + px - px * zoom, destY + py - py * zoom, w * zoom, h * zoom),
         angle, px * zoom, py * zoom, flipX, flipY, img->tinted ? img->a * blend : blend);
}

// Rotated about the centre of the destination rect + (centerX, centerY). Without a tint, a
// negative blend draws opaque.
void ImgStretchAlphaRect(Image *img, float sx1, float sy1, float sx2, float sy2,
                         float dx1, float dy1, float dx2, float dy2, float blend, float angle,
                         bool flipX, bool flipY, float centerX, float centerY)
{
    float alpha;

    if (img->tinted)
        alpha = img->a * blend;
    else
        alpha = blend >= 0 ? blend : 1.0f;
    Draw(img, sx1, sy1, sx2, sy2, Rect(dx1, dy1, dx2 - dx1, dy2 - dy1),
         angle, (dx2 - dx1) / 2 + centerX, (dy2 - dy1) / 2 + centerY, flipX, flipY, alpha);
}

// PTK's primitives use whole-pixel coordinates, and blend (normal alpha) only when `a` is not 1.
static SDL_BlendMode PrimitiveBlend(float a)
{
    return a == 1.0f ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND;
}

static void SetDrawColor(float r, float g, float b, float a)
{
    SDL_SetRenderDrawBlendMode(s_renderer, PrimitiveBlend(a));
    SDL_SetRenderDrawColorFloat(s_renderer, SDL_clamp(r, 0.0f, 1.0f), SDL_clamp(g, 0.0f, 1.0f),
                                SDL_clamp(b, 0.0f, 1.0f), SDL_clamp(a, 0.0f, 1.0f));
}

static void RecordLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
static void RecordPoint(float x, float y, float r, float g, float b, float a);

void DrawRect(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
    int ix1 = (int)x1, iy1 = (int)y1, ix2 = (int)x2, iy2 = (int)y2;
    SDL_FRect rect;

    rect.x = (float)SDL_min(ix1, ix2);
    rect.y = (float)SDL_min(iy1, iy2);
    rect.w = (float)SDL_abs(ix2 - ix1);
    rect.h = (float)SDL_abs(iy2 - iy1);
    SetDrawColor(r, g, b, a);
    SDL_RenderFillRect(s_renderer, &rect);
    RecordFill(rect, PrimitiveBlend(a), r, g, b, a);
}

void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
    SetDrawColor(r, g, b, a);
    SDL_RenderLine(s_renderer, (float)(int)x1, (float)(int)y1, (float)(int)x2, (float)(int)y2);
    RecordLine((float)(int)x1, (float)(int)y1, (float)(int)x2, (float)(int)y2, r, g, b, a);
}

void PlotPixel(float x, float y, float r, float g, float b, float a)
{
    SetDrawColor(r, g, b, a);
    SDL_RenderPoint(s_renderer, (float)(int)x, (float)(int)y);
    RecordPoint((float)(int)x, (float)(int)y, r, g, b, a);
}

// ---------------------------------------------------------------------------------------------
// Interpolation
//
// The game ticks at its own rate (SysSetMaxFps), and its logic and drawing are interleaved, so
// it can't draw a picture between two ticks. Instead, while interpolation is on, each tick's
// draws are recorded (after being drawn into the back buffer as usual), and SysFlip shows the
// tick by replaying them: each draw is matched to the draw of the previous tick it continues
// (same picture and blending, nearest position) and drawn between the two positions, by how far
// the clock is between the two ticks. The draws go straight into the window at its scale, so
// movement is smooth to the screen's pixels rather than the back buffer's. Under them goes a
// copy of the back buffer from before the tick (s_base), for screens that don't redraw
// everything each tick.
//
// The game sees no difference: the back buffer, the tick rate and the input are the same with
// or without it. The picture shown is up to one tick behind the newest tick.
// ---------------------------------------------------------------------------------------------

typedef enum CmdType { CMD_BLIT, CMD_FILL, CMD_LINE, CMD_POINT } CmdType;

typedef struct DrawCmd {
    CmdType type;
    Image *img;                     // CMD_BLIT
    SDL_BlendMode blend;
    SDL_FlipMode flip;
    bool linear;                    // the image's texture quality
    float r, g, b, a;               // tint and alpha, or the primitive's colour
    float x1, y1, x2, y2;           // destination rect edges; the line's ends; the point (x1, y1)
    float sx1, sy1, sx2, sy2;       // source rect edges
    float angle;                    // degrees clockwise (SDL's direction)
    float px, py;                   // rotation centre, from (x1, y1)
    int prev;                       // the draw it continues in the previous tick's list, or -1
    bool whole;                     // the source rect changed with the destination (clipping,
                                    // zoom): both are interpolated; else (a new animation
                                    // frame) the draw only moves
    int next;                       // the next draw in the same grid bucket (MatchDraws)
} DrawCmd;

typedef struct DrawList {
    DrawCmd *cmds;
    int count, cap;
} DrawList;

static DrawList s_lists[2];
static int      s_curList;          // s_lists[s_curList] is this tick's; the other the last tick's

enum { GRID_BUCKETS = 4096 };       // MatchDraws' spatial hash
#define GRID_CELL         32.0f     // its cell size, back-buffer pixels
#define MATCH_DIST        32.0f     // the most a draw's edges move in a tick and still match
#define NEW_FRAME_PENALTY 8.0f      // prefers the draw of the same animation frame
static int s_grid[GRID_BUCKETS];

static DrawCmd *NewCmd(CmdType type)
{
    DrawList *list = &s_lists[s_curList];
    DrawCmd *c;

    if (!s_recording)
        return NULL;
    if (list->count == list->cap) {
        int cap = list->cap ? list->cap * 2 : 1024;
        DrawCmd *cmds = (DrawCmd *)SDL_realloc(list->cmds, cap * sizeof(DrawCmd));
        if (cmds == NULL) {
            s_recording = false;    // this tick is shown without interpolation
            return NULL;
        }
        list->cmds = cmds;
        list->cap = cap;
    }
    c = &list->cmds[list->count++];
    SDL_zerop(c);
    c->type = type;
    c->prev = -1;
    return c;
}

static void SetCmdColor(DrawCmd *c, float r, float g, float b, float a)
{
    c->r = SDL_clamp(r, 0.0f, 1.0f);
    c->g = SDL_clamp(g, 0.0f, 1.0f);
    c->b = SDL_clamp(b, 0.0f, 1.0f);
    c->a = SDL_clamp(a, 0.0f, 1.0f);
}

static void RecordBlit(Image *img, SDL_FRect src, SDL_FRect dst, float angle, SDL_FPoint pivot,
                       SDL_FlipMode flip, float alpha)
{
    DrawCmd *c = NewCmd(CMD_BLIT);

    if (c == NULL)
        return;
    c->img = img;
    c->blend = BlendModeFor(img->mode);
    c->flip = flip;
    c->linear = img->linear;
    if (img->tinted)
        SetCmdColor(c, img->r, img->g, img->b, alpha);
    else
        SetCmdColor(c, 1.0f, 1.0f, 1.0f, alpha);
    c->x1 = dst.x;
    c->y1 = dst.y;
    c->x2 = dst.x + dst.w;
    c->y2 = dst.y + dst.h;
    c->sx1 = src.x;
    c->sy1 = src.y;
    c->sx2 = src.x + src.w;
    c->sy2 = src.y + src.h;
    c->angle = angle;
    c->px = pivot.x;
    c->py = pivot.y;
}

static void RecordFill(SDL_FRect rect, SDL_BlendMode blend, float r, float g, float b, float a)
{
    DrawCmd *c = NewCmd(CMD_FILL);

    if (c == NULL)
        return;
    c->blend = blend;
    SetCmdColor(c, r, g, b, a);
    c->x1 = rect.x;
    c->y1 = rect.y;
    c->x2 = rect.x + rect.w;
    c->y2 = rect.y + rect.h;
}

static void RecordLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
    DrawCmd *c = NewCmd(CMD_LINE);

    if (c == NULL)
        return;
    c->blend = PrimitiveBlend(a);
    SetCmdColor(c, r, g, b, a);
    c->x1 = x1;
    c->y1 = y1;
    c->x2 = x2;
    c->y2 = y2;
}

static void RecordPoint(float x, float y, float r, float g, float b, float a)
{
    DrawCmd *c = NewCmd(CMD_POINT);

    if (c == NULL)
        return;
    c->blend = PrimitiveBlend(a);
    SetCmdColor(c, r, g, b, a);
    c->x1 = x;
    c->y1 = y;
}

// Starts recording a tick. The back buffer as it is now goes under its draws. `fresh`: the last
// tick wasn't recorded, so there is nothing to continue.
static void StartRecording(bool fresh)
{
    SDL_SetRenderTarget(s_renderer, s_base);
    SDL_SetTextureScaleMode(s_canvas, SDL_SCALEMODE_NEAREST);
    SDL_RenderTexture(s_renderer, s_canvas, NULL, NULL);
    SDL_SetTextureScaleMode(s_canvas, s_canvasScaleMode);
    SDL_SetRenderTarget(s_renderer, s_canvas);
    if (fresh)
        s_lists[s_curList].count = 0;
    s_curList ^= 1;
    s_lists[s_curList].count = 0;
}

static void CmdCenter(const DrawCmd *c, float *x, float *y)
{
    if (c->type == CMD_POINT) {
        *x = c->x1;
        *y = c->y1;
    } else {
        *x = (c->x1 + c->x2) * 0.5f;
        *y = (c->y1 + c->y2) * 0.5f;
    }
}

static int GridCell(float v)
{
    return (int)SDL_floorf(SDL_clamp(v, -1.0e6f, 1.0e6f) / GRID_CELL);
}

static int GridBucket(int cx, int cy)
{
    return (int)(((unsigned)cx * 73856093u ^ (unsigned)cy * 19349663u) % GRID_BUCKETS);
}

static bool SameKind(const DrawCmd *a, const DrawCmd *b)
{
    if (a->type != b->type || a->blend != b->blend)
        return false;
    return a->type != CMD_BLIT || (a->img == b->img && a->flip == b->flip);
}

// How far the draw's edges (or the point) moved.
static float EdgeDist(const DrawCmd *a, const DrawCmd *b)
{
    float d = SDL_max(SDL_fabsf(a->x1 - b->x1), SDL_fabsf(a->y1 - b->y1));
    if (a->type != CMD_POINT)
        d = SDL_max(d, SDL_max(SDL_fabsf(a->x2 - b->x2), SDL_fabsf(a->y2 - b->y2)));
    return d;
}

static float SrcDist(const DrawCmd *a, const DrawCmd *b)
{
    return SDL_max(SDL_max(SDL_fabsf(a->sx1 - b->sx1), SDL_fabsf(a->sy1 - b->sy1)),
                   SDL_max(SDL_fabsf(a->sx2 - b->sx2), SDL_fabsf(a->sy2 - b->sy2)));
}

// Matches each draw of this tick to the draw of the previous tick it continues: the same kind,
// its edges moved least and at most MATCH_DIST. The previous draws are hashed by the grid cell
// of their centre; a match's centre is at most a cell away.
static void MatchDraws(void)
{
    DrawList *cur = &s_lists[s_curList];
    DrawList *prev = &s_lists[s_curList ^ 1];
    int i, j, gx, gy;

    SDL_memset(s_grid, 0xff, sizeof(s_grid));
    for (i = 0; i < prev->count; i++) {
        float x, y;
        int bucket;
        CmdCenter(&prev->cmds[i], &x, &y);
        bucket = GridBucket(GridCell(x), GridCell(y));
        prev->cmds[i].next = s_grid[bucket];
        s_grid[bucket] = i;
    }
    for (i = 0; i < cur->count; i++) {
        DrawCmd *c = &cur->cmds[i];
        float x, y, best = MATCH_DIST + 0.001f;
        int cx, cy;

        c->prev = -1;
        CmdCenter(c, &x, &y);
        cx = GridCell(x);
        cy = GridCell(y);
        for (gy = cy - 1; gy <= cy + 1; gy++) {
            for (gx = cx - 1; gx <= cx + 1; gx++) {
                for (j = s_grid[GridBucket(gx, gy)]; j >= 0; j = prev->cmds[j].next) {
                    const DrawCmd *p = &prev->cmds[j];
                    bool whole = true;
                    float d;

                    if (!SameKind(c, p))
                        continue;
                    d = EdgeDist(c, p);
                    if (c->type == CMD_BLIT && SrcDist(c, p) > d + 0.5f) {
                        whole = false;
                        d += NEW_FRAME_PENALTY;
                    }
                    if (d < best) {
                        best = d;
                        c->prev = j;
                        c->whole = whole;
                    }
                }
            }
        }
    }
}

static float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

// Moves `c` back towards `p`, its draw in the previous tick, to `t` of the way from `p` to `c`.
static void Interpolate(DrawCmd *c, const DrawCmd *p, float t)
{
    if (c->type != CMD_BLIT || c->whole) {
        c->x1 = Lerp(p->x1, c->x1, t);
        c->y1 = Lerp(p->y1, c->y1, t);
        c->x2 = Lerp(p->x2, c->x2, t);
        c->y2 = Lerp(p->y2, c->y2, t);
        if (c->type == CMD_BLIT) {
            c->sx1 = Lerp(p->sx1, c->sx1, t);
            c->sy1 = Lerp(p->sy1, c->sy1, t);
            c->sx2 = Lerp(p->sx2, c->sx2, t);
            c->sy2 = Lerp(p->sy2, c->sy2, t);
            c->px = Lerp(p->px, c->px, t);
            c->py = Lerp(p->py, c->py, t);
        }
    } else {
        float dx = (1.0f - t) * ((p->x1 + p->x2) - (c->x1 + c->x2)) * 0.5f;
        float dy = (1.0f - t) * ((p->y1 + p->y2) - (c->y1 + c->y2)) * 0.5f;
        c->x1 += dx;
        c->x2 += dx;
        c->y1 += dy;
        c->y2 += dy;
    }
    if (c->type == CMD_BLIT) {
        float turn = SDL_fmodf(c->angle - p->angle, 360.0f);
        if (turn > 180.0f)
            turn -= 360.0f;
        else if (turn < -180.0f)
            turn += 360.0f;
        c->angle -= (1.0f - t) * turn;
    }
}

static void ReplayDraw(const DrawCmd *c)
{
    SDL_FRect src, dst;
    SDL_FPoint pivot;
    bool scaled;

    if (c->type == CMD_BLIT) {
        SDL_Texture *tex = c->img->tex;
        if (tex == NULL)            // freed since
            return;
        src = Rect(c->sx1, c->sy1, c->sx2 - c->sx1, c->sy2 - c->sy1);
        dst = Rect(c->x1, c->y1, c->x2 - c->x1, c->y2 - c->y1);
        pivot.x = c->px;
        pivot.y = c->py;
        scaled = c->angle != 0 || SDL_fabsf(dst.w - src.w) > 0.01f || SDL_fabsf(dst.h - src.h) > 0.01f;
        SDL_SetTextureColorModFloat(tex, c->r, c->g, c->b);
        SDL_SetTextureAlphaModFloat(tex, c->a);
        SDL_SetTextureBlendMode(tex, c->blend);
        // Unscaled blits get the back buffer's pixel-art scaling, so they look the same.
        SDL_SetTextureScaleMode(tex, c->linear && scaled ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_PIXELART);
        SDL_RenderTextureRotated(s_renderer, tex, &src, &dst, c->angle, &pivot, c->flip);
        return;
    }
    SDL_SetRenderDrawBlendMode(s_renderer, c->blend);
    SDL_SetRenderDrawColorFloat(s_renderer, c->r, c->g, c->b, c->a);
    if (c->type == CMD_FILL) {
        dst = Rect(c->x1, c->y1, c->x2 - c->x1, c->y2 - c->y1);
        SDL_RenderFillRect(s_renderer, &dst);
    } else if (c->type == CMD_LINE) {
        SDL_RenderLine(s_renderer, c->x1, c->y1, c->x2, c->y2);
    } else {
        SDL_RenderPoint(s_renderer, c->x1, c->y1);
    }
}

// Draws the tick `t` of the way from the previous tick into the window, letterboxed.
static void RenderInterpolated(float t)
{
    const DrawList *cur = &s_lists[s_curList];
    const DrawList *prev = &s_lists[s_curList ^ 1];
    SDL_FRect out = OutputRect();
    SDL_FRect all = Rect(0, 0, (float)s_width, (float)s_height);
    SDL_FRect dst;
    int w = (int)SDL_roundf(out.w), h = (int)SDL_roundf(out.h);
    int i;

    SDL_SetRenderTarget(s_renderer, NULL);
    SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255);
    SDL_RenderClear(s_renderer);
    if (w <= 0 || h <= 0)
        return;
    // Drawn into a texture of the picture's size in window pixels, which clips to it. (The
    // renderer's viewport would not do: SDL multiplies its position by the render scale.)
    if (s_view == NULL || s_view->w != w || s_view->h != h) {
        SDL_DestroyTexture(s_view);
        s_view = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
        if (s_view == NULL)
            return;
        SDL_SetTextureBlendMode(s_view, SDL_BLENDMODE_NONE);
        SDL_SetTextureScaleMode(s_view, SDL_SCALEMODE_NEAREST);
    }
    SDL_SetRenderTarget(s_renderer, s_view);
    SDL_SetRenderScale(s_renderer, (float)w / s_width, (float)h / s_height);
    SDL_RenderTexture(s_renderer, s_base, NULL, &all);
    for (i = 0; i < cur->count; i++) {
        DrawCmd c = cur->cmds[i];
        if (c.prev >= 0)
            Interpolate(&c, &prev->cmds[c.prev], t);
        ReplayDraw(&c);
    }
    SDL_SetRenderScale(s_renderer, 1.0f, 1.0f);
    SDL_SetRenderTarget(s_renderer, NULL);
    dst = Rect(SDL_roundf(out.x), SDL_roundf(out.y), (float)w, (float)h);
    SDL_RenderTexture(s_renderer, s_view, NULL, &dst);
}

// Shows the recorded tick, interpolated, until the next tick is due. The ticks keep the frame
// cap's rate on the clock (s_tickDueNs); a present that runs late is made up on the next ticks.
static void FlipInterpolated(void)
{
    Uint64 tickNs = (Uint64)s_frameMs * SDL_NS_PER_MS;
    Uint64 due = s_tickDueNs + tickNs;          // when the next tick is due
    Uint64 frameNs = 0;                         // without vsync: the refresh period
    Uint64 now;
    bool shown = false;

    MatchDraws();
    if (!s_vsync) {
        float hz = RefreshRate();
        frameNs = (Uint64)(SDL_NS_PER_SECOND / (hz > 0 ? hz : 60.0f));
    }
    for (;;) {
        float t;

        SysProcessEvents();
        now = SDL_GetTicksNS();
        if (s_quit || !s_focused)
            break;
        if (frameNs) {
            // Without vsync, frames keep their own clock at the refresh rate, across ticks.
            if (now > s_nextFrameNs + frameNs || now + frameNs < s_nextFrameNs)
                s_nextFrameNs = now;
            if (s_nextFrameNs >= due)
                break;
            if (now < s_nextFrameNs) {
                SDL_DelayPrecise(s_nextFrameNs - now);
                now = SDL_GetTicksNS();
            }
            s_nextFrameNs += frameNs;
        } else if (now >= due && (shown || now >= due + tickNs)) {
            // With vsync: at least one frame per tick, unless more than a tick behind.
            break;
        }
        t = (float)(Sint64)(now - s_tickDueNs) / (float)tickNs;
        RenderInterpolated(SDL_clamp(t, 0.0f, 1.0f));
        SDL_RenderPresent(s_renderer);
        shown = true;
    }
    SDL_SetRenderTarget(s_renderer, s_canvas);

    if (!s_focused) {
        WaitForFocus();
        due = SDL_GetTicksNS();
    }
    now = SDL_GetTicksNS();
    // Far behind (a slow tick, loading): the lost time is dropped rather than caught up.
    if (now > due + 4 * tickNs)
        due = now;
    s_tickDueNs = due;
    s_lastFlipNs = now;
}

// ---------------------------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------------------------

// PTK key -> scancode. Letters, digits and punctuation were Windows virtual keys, which follow
// the keyboard layout, so their scancodes are looked up from the key they produce.
static SDL_Scancode s_keys[K_VK_ERROR];

static const SDL_Scancode s_fixedKeys[K_VK_ERROR] = {
    SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE,
    SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RETURN, SDL_SCANCODE_RCTRL, SDL_SCANCODE_LCTRL,
    SDL_SCANCODE_F1, SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, SDL_SCANCODE_F5, SDL_SCANCODE_F6,
    SDL_SCANCODE_F7, SDL_SCANCODE_F8, SDL_SCANCODE_F9, SDL_SCANCODE_F10, SDL_SCANCODE_F11, SDL_SCANCODE_F12,
    SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_TAB, SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_A, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_D, SDL_SCANCODE_E, SDL_SCANCODE_F,
    SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_I, SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L,
    SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O, SDL_SCANCODE_P, SDL_SCANCODE_Q, SDL_SCANCODE_R,
    SDL_SCANCODE_S, SDL_SCANCODE_T, SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X,
    SDL_SCANCODE_Y, SDL_SCANCODE_Z,
    SDL_SCANCODE_0, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9,
    SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_4,
    SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_7, SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_9,
    SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_PERIOD,
    SDL_SCANCODE_KP_DIVIDE,
    SDL_SCANCODE_CLEAR, SDL_SCANCODE_LALT, SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI,
    SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_SCROLLLOCK,
    SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_EQUALS, SDL_SCANCODE_COMMA, SDL_SCANCODE_MINUS,
    SDL_SCANCODE_PERIOD, SDL_SCANCODE_SLASH, SDL_SCANCODE_GRAVE, SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_BACKSLASH, SDL_SCANCODE_RIGHTBRACKET, SDL_SCANCODE_APOSTROPHE,
    SDL_SCANCODE_END, SDL_SCANCODE_HOME, SDL_SCANCODE_DELETE, SDL_SCANCODE_INSERT,
    SDL_SCANCODE_PRINTSCREEN, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_PAGEDOWN,
};

static const SDL_Keycode s_oemKeys[K_VK_OEM_7 - K_VK_OEM_1 + 1] = {
    SDLK_SEMICOLON, SDLK_EQUALS, SDLK_COMMA, SDLK_MINUS, SDLK_PERIOD, SDLK_SLASH, SDLK_GRAVE,
    SDLK_LEFTBRACKET, SDLK_BACKSLASH, SDLK_RIGHTBRACKET, SDLK_APOSTROPHE,
};

static void BuildKeyTable(void)
{
    int k;
    SDL_Keycode key;
    SDL_Scancode sc;

    for (k = 0; k < K_VK_ERROR; k++) {
        key = SDLK_UNKNOWN;
        if (k >= K_VK_A && k <= K_VK_Z)
            key = SDLK_A + (k - K_VK_A);
        else if (k >= K_VK_0 && k <= K_VK_9)
            key = SDLK_0 + (k - K_VK_0);
        else if (k >= K_VK_OEM_1 && k <= K_VK_OEM_7)
            key = s_oemKeys[k - K_VK_OEM_1];
        sc = key != SDLK_UNKNOWN ? SDL_GetScancodeFromKey(key, NULL) : SDL_SCANCODE_UNKNOWN;
        s_keys[k] = sc != SDL_SCANCODE_UNKNOWN ? sc : s_fixedKeys[k];
    }
}

// PTK read the live key and button state (GetAsyncKeyState), and the game relies on it: some
// loops wait for a key to be released without processing events. SDL's state only changes when
// events are pumped, so the input functions pump them when the state is over 1 ms old.
static void RefreshInput(void)
{
    static Uint64 last;
    Uint64 now = SDL_GetTicksNS();
    if (now - last >= SDL_NS_PER_MS) {
        SDL_PumpEvents();
        last = now;
    }
}

bool KeyDown(enum EKeyboardLayout key)
{
    const bool *state;

    RefreshInput();
    state = SDL_GetKeyboardState(NULL);

    if ((int)key < 0 || key >= K_VK_ERROR)
        return false;
    if (s_keys[0] == SDL_SCANCODE_UNKNOWN)
        BuildKeyTable();
    if (key == K_VK_MENU)           // VK_MENU is either Alt
        return state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT];
    if (key == K_VK_RETURN && state[SDL_SCANCODE_KP_ENTER])
        return true;
    return state[s_keys[key]];
}

static void MouseCanvas(float *x, float *y)
{
    float wx = 0, wy = 0;
    RefreshInput();
    SDL_GetMouseState(&wx, &wy);
    WindowToCanvas(wx, wy, x, y);
}

int MouseX(void)
{
    float x = 0, y = 0;
    MouseCanvas(&x, &y);
    return (int)SDL_floorf(x);
}

int MouseY(void)
{
    float x = 0, y = 0;
    MouseCanvas(&x, &y);
    return (int)SDL_floorf(y);
}

bool MouseLeft(void)
{
    RefreshInput();
    return (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK) != 0;
}

bool MouseRight(void)
{
    RefreshInput();
    return (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_RMASK) != 0;
}
void ShowPointer(void) { SDL_ShowCursor(); }
void HidePointer(void) { SDL_HideCursor(); }

// In back-buffer coordinates (the original ran fullscreen at 800x600, where they were the
// screen's).
void MouseWarp(int x, int y)
{
    float wx, wy;
    CanvasToWindow((float)x, (float)y, &wx, &wy);
    SDL_WarpMouseInWindow(s_window, wx, wy);
}

// Confines the pointer to the picture (not the letterbox bars) in fullscreen.
void ClipPointer(bool on)
{
    if (on && s_fullscreen) {
        float x1, y1, x2, y2;
        SDL_Rect r;
        CanvasToWindow(0, 0, &x1, &y1);
        CanvasToWindow((float)s_width, (float)s_height, &x2, &y2);
        r.x = (int)x1;
        r.y = (int)y1;
        r.w = (int)(x2 - x1);
        r.h = (int)(y2 - y1);
        SDL_SetWindowMouseRect(s_window, &r);
    } else {
        SDL_SetWindowMouseRect(s_window, NULL);
    }
}

// Joysticks, reported like winmm's joyGetPosEx: axes 0-65535 (centre 32767), buttons a bit mask.
#define MAX_JOYS 2
static SDL_Joystick *s_joys[MAX_JOYS];

bool JoyEnable(char joy)
{
    int count = 0;
    SDL_JoystickID *ids;

    if (joy < 0 || joy >= MAX_JOYS)
        return false;
    if (s_joys[(int)joy])
        return true;
    ids = SDL_GetJoysticks(&count);
    if (ids && joy < count)
        s_joys[(int)joy] = SDL_OpenJoystick(ids[(int)joy]);
    SDL_free(ids);
    return s_joys[(int)joy] != NULL;
}

static long JoyAxis(char joy, int axis)
{
    SDL_Joystick *j = joy >= 0 && joy < MAX_JOYS ? s_joys[(int)joy] : NULL;
    if (j == NULL)
        return 0x7fff;
    if (axis >= SDL_GetNumJoystickAxes(j))
        return 0x7fff;
    RefreshInput();
    return (long)SDL_GetJoystickAxis(j, axis) + 32768;
}

long JoyX(char joy, char hat) { return JoyAxis(joy, hat == 0 ? 0 : 2); }
long JoyY(char joy, char hat) { return JoyAxis(joy, hat == 0 ? 1 : 3); }

bool JoyButton(char joy, long mask)
{
    SDL_Joystick *j = joy >= 0 && joy < MAX_JOYS ? s_joys[(int)joy] : NULL;
    unsigned long buttons = 0;
    int i, n;

    if (j == NULL)
        return false;
    RefreshInput();
    n = SDL_min(SDL_GetNumJoystickButtons(j), 32);
    for (i = 0; i < n; i++) {
        if (SDL_GetJoystickButton(j, i))
            buttons |= 1ul << i;
    }
    return (buttons & (unsigned long)mask) != 0;
}

// ---------------------------------------------------------------------------------------------
// Gamepads (sdlhelp.h): opened as they connect (SDL reports the ones present at start too).
// ---------------------------------------------------------------------------------------------

#define MAX_PADS 4
#define PAD_STICK_DEAD 12000        // of 32767: how far the stick has to go to count as a direction
#define PAD_TRIGGER_DOWN 16000      // of 32767: how far a trigger has to go to count as pressed

static SDL_Gamepad *s_pads[MAX_PADS];
static unsigned s_virtualPad;

static void PadAdded(SDL_JoystickID id)
{
    int i;
    for (i = 0; i < MAX_PADS; i++) {
        if (s_pads[i] && SDL_GetGamepadID(s_pads[i]) == id)
            return;
    }
    for (i = 0; i < MAX_PADS; i++) {
        if (s_pads[i] == NULL) {
            s_pads[i] = SDL_OpenGamepad(id);
            if (s_pads[i] == NULL)
                SDL_Log("SDL_OpenGamepad failed: %s", SDL_GetError());
            else
                SDL_Log("Gamepad %d: %s", i, SDL_GetGamepadName(s_pads[i]));
            return;
        }
    }
}

static void PadRemoved(SDL_JoystickID id)
{
    int i;
    for (i = 0; i < MAX_PADS; i++) {
        if (s_pads[i] && SDL_GetGamepadID(s_pads[i]) == id) {
            SDL_CloseGamepad(s_pads[i]);
            s_pads[i] = NULL;
        }
    }
}

unsigned SysPadState(int pad)
{
    SDL_Gamepad *g;
    unsigned bits;
    int x, y;

    if (pad < 0 || pad >= MAX_PADS)
        return 0;
    RefreshInput();
    bits = pad == 0 ? s_virtualPad : 0;
    g = s_pads[pad];
    if (g == NULL)
        return bits;

    x = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTX);
    y = SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFTY);
    if (x < -PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
        bits |= PAD_LEFT;
    if (x > PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
        bits |= PAD_RIGHT;
    if (y < -PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_UP))
        bits |= PAD_UP;
    if (y > PAD_STICK_DEAD || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
        bits |= PAD_DOWN;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_SOUTH) ||
        SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > PAD_TRIGGER_DOWN)
        bits |= PAD_FIRE;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_EAST) || SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_WEST) ||
        SDL_GetGamepadAxis(g, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > PAD_TRIGGER_DOWN)
        bits |= PAD_ROCKET;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_START))
        bits |= PAD_PAUSE;
    if (SDL_GetGamepadButton(g, SDL_GAMEPAD_BUTTON_BACK))
        bits |= PAD_PROFILE;
    return bits;
}

// Called by the browser page's touch controls (web/index.html).
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
void SysSetVirtualPad(unsigned bits)
{
    s_virtualPad = bits;
}
