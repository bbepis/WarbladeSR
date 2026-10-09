// fake_engine.c: See fake_engine.h.
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "fake_engine.h"

FakeEngine g_fake;

struct Image {
    float w, h;
    bool freed;
    short alphaMode;
    float tint[4];
    char name[256];
};

enum { MAX_PAC = 256, MAX_HANDLES = 4096 };

typedef struct PacFile {
    char name[256];
    unsigned char *data;
    unsigned size;
} PacFile;

static PacFile s_pac[MAX_PAC];
static int s_pacCount;

// Handle n (1-based) is s_handles[n - 1]; a voice records its sample's name.
typedef struct FakeHandle {
    char name[256];
    int plays;
} FakeHandle;
static FakeHandle s_handles[MAX_HANDLES];
static int s_handleCount;

static void (*s_focusFn)(bool);

void FakeReset(void)
{
    char folder[512];
    memcpy(folder, g_fake.userFolder, sizeof folder);
    memset(&g_fake, 0, sizeof g_fake);
    memcpy(g_fake.userFolder, folder, sizeof folder);
    g_fake.millis = 100000;
    g_fake.flipAdvanceMs = 16;
    g_fake.imageW = g_fake.imageH = 64;
    g_fake.joyX[0] = g_fake.joyX[1] = 0x7fff;
    g_fake.joyY[0] = g_fake.joyY[1] = 0x7fff;
    g_fake.localDate = (SysDate){2009, 6, 1, 15, 12, 30, 45, 500};
    g_fake.fileTime = 128895606455000000LL;    // 2009-06-15 12:30:45.5 UTC
    g_fake.vsync = true;
    g_fake.maxFps = 60;
    if (!g_fake.userFolder[0]) {
        char tmpl[] = "/tmp/wbtest-XXXXXX";
        const char *dir = mkdtemp(tmpl);
        snprintf(g_fake.userFolder, sizeof g_fake.userFolder, "%s", dir ? dir : "/tmp");
    }
    s_handleCount = 0;
    memset(s_handles, 0, sizeof s_handles);
}

void FakePressKey(enum EKeyboardLayout key)   { g_fake.keys[key] = true; }
void FakeReleaseKey(enum EKeyboardLayout key) { g_fake.keys[key] = false; }
void FakeReleaseAllKeys(void)                 { memset(g_fake.keys, 0, sizeof g_fake.keys); }

static void NormalizeName(const char *in, char *out, size_t size)
{
    size_t i = 0;
    for (; in[i] && i + 1 < size; i++)
        out[i] = in[i] == '\\' ? '/' : in[i];
    out[i] = 0;
}

void FakePacAdd(const char *name, const void *data, unsigned size)
{
    if (s_pacCount == MAX_PAC)
        abort();
    PacFile *f = &s_pac[s_pacCount++];
    NormalizeName(name, f->name, sizeof f->name);
    f->data = malloc(size ? size : 1);
    memcpy(f->data, data, size);
    f->size = size;
}

void FakePacClear(void)
{
    for (int i = 0; i < s_pacCount; i++)
        free(s_pac[i].data);
    s_pacCount = 0;
}

static PacFile *PacFind(const char *name)
{
    char n[256];
    NormalizeName(name, n, sizeof n);
    for (int i = s_pacCount - 1; i >= 0; i--) {
        if (!strcasecmp(s_pac[i].name, n))
            return &s_pac[i];
        // the game asks for "data\\x" or just "x" depending on the caller
        size_t a = strlen(s_pac[i].name), b = strlen(n);
        if (a > b && s_pac[i].name[a - b - 1] == '/' && !strcasecmp(s_pac[i].name + a - b, n))
            return &s_pac[i];
        if (b > a && n[b - a - 1] == '/' && !strcasecmp(n + b - a, s_pac[i].name))
            return &s_pac[i];
    }
    return NULL;
}

static AudioHandle NewHandle(const char *name)
{
    if (s_handleCount == MAX_HANDLES)
        return 0;
    snprintf(s_handles[s_handleCount].name, sizeof s_handles[0].name, "%s", name ? name : "");
    return (AudioHandle)++s_handleCount;
}

const char *FakeSampleName(AudioHandle h)
{
    return h >= 1 && h <= (AudioHandle)s_handleCount ? s_handles[h - 1].name : "";
}

int FakePlayCount(const char *fragment)
{
    int n = 0;
    for (int i = 0; i < s_handleCount; i++)
        if (strstr(s_handles[i].name, fragment))
            n += s_handles[i].plays;
    return n;
}

const char *FakeImageName(const Image *img) { return img ? img->name : ""; }

const char *FakeUserPath(const char *rel)
{
    static char buf[1024];
    char n[512];
    NormalizeName(rel, n, sizeof n);
    snprintf(buf, sizeof buf, "%s/%s", g_fake.userFolder, n);
    return buf;
}

bool FakeFileExists(const char *hostPath)
{
    struct stat st;
    return stat(hostPath, &st) == 0;
}

// ---- window, frame, events ----
void SysInit(void) {}
bool SysCreateWindow(int w, int h, bool windowed, const char *title, const char *renderDriver)
{
    (void)w; (void)h; (void)title;
    snprintf(g_fake.lastRenderDriver, sizeof g_fake.lastRenderDriver, "%s",
             renderDriver ? renderDriver : "");
    if (g_fake.createWindowFails > 0) {
        g_fake.createWindowFails--;
        return false;
    }
    g_fake.windowsCreated++;
    g_fake.fullscreen = !windowed;
    return true;
}
const char *SysRendererName(void) { return "fake"; }
void SysDestroyWindow(void) {}
bool SysHasWindow(void) { return true; }
void SysTerminate(void) { g_fake.quit = true; }
bool SysQuitRequested(void) { return g_fake.quit; }
void SysProcessEvents(void) {}
void SysFlip(void)
{
    g_fake.flips++;
    g_fake.millis += g_fake.flipAdvanceMs;
    if (g_fake.quitAfterFlips > 0 && g_fake.flips >= g_fake.quitAfterFlips)
        g_fake.quit = true;
    if (g_fake.onFlip)
        g_fake.onFlip();
}
void SysSetMaxFps(int fps) { g_fake.maxFps = fps; }
void SysSetVSync(bool on) { g_fake.vsync = on; }
void SysSetInterpolation(int mode) { g_fake.interpolation = mode; }
bool SysInterpolating(void) { return false; }
void SysSetClearColor(float r, float g, float b, float a) { (void)r; (void)g; (void)b; (void)a; }
void SysSetWorldView(float x, float y, float rotation, float zoom, bool clear)
{
    (void)x; (void)y; (void)rotation; (void)zoom; (void)clear;
}
void SysMinimize(void) {}
bool SysHasFocus(void) { return true; }
void SysSetIcon(const char *icoFile) { (void)icoFile; }
void SysDisableScreenSaver(void) {}
void SysSetFullscreen(const bool fullscreen) { g_fake.fullscreen = fullscreen; }
void SysSetFocusCallback(void (*fn)(bool focused))
{
    s_focusFn = fn;
    g_fake.focusCallback = fn;
}
bool SysScreenshot(const char *file, int w, int h)
{
    (void)w; (void)h;
    FILE *f = fopen(SysPath(file), "wb");
    if (!f)
        return false;
    fputs("fake jpeg", f);
    fclose(f);
    return true;
}
int SysDesktopWidth(void) { return 1920; }
int SysDesktopHeight(void) { return 1080; }
void SysMessageBox(const char *title, const char *text)
{
    (void)title;
    snprintf(g_fake.lastMessageBox, sizeof g_fake.lastMessageBox, "%s", text);
}

// ---- time ----
unsigned SysMillis(void) { return g_fake.millis; }
long long SysPerfCounter(void) { return (long long)g_fake.millis * 1000; }
long long SysPerfFreq(void) { return 1000000; }
long long SysFileTimeNow(void) { return g_fake.fileTime; }
void SysFileTimeToDate(long long ft, SysDate *out)
{
    // days since 1601-01-01, then the civil date (Howard Hinnant's algorithm)
    long long ms = ft / 10000;
    long long days = ms / 86400000LL, rem = ms % 86400000LL;
    long long z = days - 134774 + 719468;   // 1601 -> 1970 -> 0000-03-01 epoch
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long y = (long long)yoe + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp = (5 * doy + 2) / 153;
    unsigned d = doy - (153 * mp + 2) / 5 + 1;
    unsigned m = mp < 10 ? mp + 3 : mp - 9;
    out->year = (unsigned short)(y + (m <= 2));
    out->month = (unsigned short)m;
    out->day = (unsigned short)d;
    out->dayOfWeek = (unsigned short)((days + 1) % 7);   // 1601-01-01 was a Monday
    out->hour = (unsigned short)(rem / 3600000);
    out->minute = (unsigned short)(rem / 60000 % 60);
    out->second = (unsigned short)(rem / 1000 % 60);
    out->milliseconds = (unsigned short)(rem % 1000);
}
void SysUtcDate(SysDate *out) { *out = g_fake.localDate; }
void SysLocalDate(SysDate *out) { *out = g_fake.localDate; }

// ---- files ----
const char *SysUserFolder(void) { return g_fake.userFolder; }
const char *SysAppPath(const char *rel)
{
    static char buf[1024];
    snprintf(buf, sizeof buf, "%s", rel);
    return buf;
}
const char *SysPath(const char *path)
{
    static char bufs[4][1024];
    static int next;
    char *b = bufs[next++ & 3];
    NormalizeName(path, b, sizeof bufs[0]);
    return b;
}
bool SysFileExists(const char *path)
{
    struct stat st;
    return stat(SysPath(path), &st) == 0;
}
bool SysMakeDir(const char *path) { return mkdir(SysPath(path), 0755) == 0; }
void SysOpenUrl(const char *url) { snprintf(g_fake.lastUrl, sizeof g_fake.lastUrl, "%s", url); }
void SysOpenJukebox(void) {}

void PacAddArchive(const char *path) { (void)path; }
bool PacExists(const char *name) { return PacFind(name) != NULL; }
bool PacRead(const char *name, void *buf, unsigned size)
{
    PacFile *f = PacFind(name);
    if (!f)
        return false;
    memcpy(buf, f->data, size < f->size ? size : f->size);
    return true;
}

// ---- images ----
Image *ImgLoad(const char *file, bool hiDef, bool hasAlpha)
{
    (void)hiDef; (void)hasAlpha;
    if (g_fake.imageLoadFails)
        return NULL;
    Image *img = calloc(1, sizeof *img);
    img->w = g_fake.imageW;
    img->h = g_fake.imageH;
    img->alphaMode = 1;
    img->tint[0] = img->tint[1] = img->tint[2] = img->tint[3] = 1;
    snprintf(img->name, sizeof img->name, "%s", file ? file : "");
    g_fake.imagesLoaded++;
    return img;
}
// Freed images are kept (marked), as the real engine keeps them a few frames: the render
// queues may still check them.
void ImgFree(Image *img)
{
    if (img && !img->freed) {
        img->freed = true;
        g_fake.imagesFreed++;
    }
}
void ImgFreePicture(Image *img) { (void)img; }
bool ImgIsFreed(const Image *img) { return img->freed; }
float ImgWidth(Image *img) { return img ? img->w : 0; }
float ImgHeight(Image *img) { return img ? img->h : 0; }
void ImgSetTextureQuality(Image *img, bool quality) { (void)img; (void)quality; }
void ImgSetAlphaMode(Image *img, short mode) { if (img) img->alphaMode = mode; }
void ImgSetBlitColor(Image *img, float r, float g, float b, float a)
{
    if (img) {
        img->tint[0] = r; img->tint[1] = g; img->tint[2] = b; img->tint[3] = a;
    }
}
void ImgBlitAlphaRect(Image *img, float x1, float y1, float x2, float y2, short destX, short destY,
                      bool flipX, bool flipY)
{
    (void)img; (void)x1; (void)y1; (void)x2; (void)y2; (void)destX; (void)destY; (void)flipX; (void)flipY;
    g_fake.blits++;
}
void ImgBlitAlphaRectFx(Image *img, float x1, float y1, float x2, float y2, short destX, short destY,
                        float angle, float zoom, float blend, bool flipX, bool flipY, float centerX,
                        float centerY)
{
    (void)img; (void)x1; (void)y1; (void)x2; (void)y2; (void)destX; (void)destY; (void)angle;
    (void)zoom; (void)blend; (void)flipX; (void)flipY; (void)centerX; (void)centerY;
    g_fake.blits++;
}
void ImgBlitRectF(Image *img, float x1, float y1, float x2, float y2, float destX, float destY,
                  bool flipX, bool flipY)
{
    (void)img; (void)x1; (void)y1; (void)x2; (void)y2; (void)destX; (void)destY; (void)flipX; (void)flipY;
    g_fake.blits++;
}
void ImgStretchAlphaRect(Image *img, float sx1, float sy1, float sx2, float sy2, float dx1, float dy1,
                         float dx2, float dy2, float blend, float angle, bool flipX, bool flipY,
                         float centerX, float centerY)
{
    (void)img; (void)sx1; (void)sy1; (void)sx2; (void)sy2; (void)dx1; (void)dy1; (void)dx2;
    (void)dy2; (void)blend; (void)angle; (void)flipX; (void)flipY; (void)centerX; (void)centerY;
    g_fake.blits++;
}
void DrawRect(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
    (void)x1; (void)y1; (void)x2; (void)y2; (void)r; (void)g; (void)b; (void)a;
    g_fake.rects++;
}
void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
    (void)x1; (void)y1; (void)x2; (void)y2; (void)r; (void)g; (void)b; (void)a;
    g_fake.lines++;
}
void PlotPixel(float x, float y, float r, float g, float b, float a)
{
    (void)x; (void)y; (void)r; (void)g; (void)b; (void)a;
    g_fake.pixels++;
}

// ---- input ----
bool KeyDown(enum EKeyboardLayout key) { return key >= 0 && key <= K_VK_ERROR && g_fake.keys[key]; }
int MouseX(void) { return g_fake.mouseX; }
int MouseY(void) { return g_fake.mouseY; }
bool MouseLeft(void) { return g_fake.mouseLeft; }
bool MouseRight(void) { return g_fake.mouseRight; }
void MouseWarp(int x, int y) { g_fake.mouseX = x; g_fake.mouseY = y; }
void ShowPointer(void) { g_fake.pointerHidden = false; }
void HidePointer(void) { g_fake.pointerHidden = true; }
void ClipPointer(bool on)
{
    g_fake.pointerClipped = on;
    g_fake.clipPointerCalls++;
}
bool JoyEnable(char joy) { return joy >= 0 && joy < 2 && g_fake.joyPresent[(int)joy]; }
long JoyX(char joy, char hat) { (void)hat; return joy >= 0 && joy < 2 ? g_fake.joyX[(int)joy] : 0x7fff; }
long JoyY(char joy, char hat) { (void)hat; return joy >= 0 && joy < 2 ? g_fake.joyY[(int)joy] : 0x7fff; }
bool JoyButton(char joy, long mask) { (void)joy; (void)mask; return false; }

// ---- audio ----
bool AudioInit(void) { return true; }
void AudioShutdown(void) {}
int AudioError(void) { return 0; }
void AudioUpdate(void) {}
void AudioStart(void) {}
void AudioStop(void) {}
void AudioPause(void) {}
AudioHandle SampleLoad(const char *file, int maxVoices, int flags)
{
    (void)maxVoices; (void)flags;
    if (g_fake.sampleLoadFails)
        return 0;
    g_fake.samplesLoaded++;
    return NewHandle(file);
}
void SampleFree(AudioHandle sample) { if (sample) g_fake.samplesFreed++; }
AudioHandle SampleGetVoice(AudioHandle sample)
{
    if (!sample)
        return 0;
    g_fake.voicesStarted++;
    return NewHandle(FakeSampleName(sample));
}
AudioHandle MusicLoad(const char *file) { return NewHandle(file); }
void MusicFree(AudioHandle music) { (void)music; }
AudioHandle StreamLoad(const char *file) { return NewHandle(file); }
void StreamFree(AudioHandle stream) { (void)stream; }
bool ChanPlay(AudioHandle ch, bool restart)
{
    (void)restart;
    if (!ch)
        return false;
    g_fake.chanPlays++;
    g_fake.lastPlayed = ch;
    if (ch <= (AudioHandle)s_handleCount)
        s_handles[ch - 1].plays++;
    return true;
}
void ChanStop(AudioHandle ch) { (void)ch; }
void ChanSet(AudioHandle ch, enum AudioAttrib attrib, float value) { (void)ch; (void)attrib; (void)value; }
void ChanSlide(AudioHandle ch, enum AudioAttrib attrib, float value, int ms)
{
    (void)ch; (void)attrib; (void)value; (void)ms;
}
double ChanLength(AudioHandle ch) { (void)ch; return 1.0; }
unsigned long ChanGetPos(AudioHandle ch) { (void)ch; return 0; }
void ChanSetPos(AudioHandle ch, unsigned long pos) { (void)ch; (void)pos; }

unsigned SysPadState(int pad)
{
    return pad >= 0 && pad < 2 ? g_fake.padBits[pad] | (pad == 0 ? g_fake.virtualPad : 0) : 0;
}

void SysSetVirtualPad(unsigned bits) { g_fake.virtualPad = bits; }
