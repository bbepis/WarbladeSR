#pragma once
// engine_util.h: Helpers shared by the engine tests (engine_*.c, the wbengine executable):
// temporary folders, files written byte by byte (TGA, WAV, tar), the headless SDL drivers and
// reading back the pixels the real engine (src/core/sdl.c) drew.
//
// The SDL libraries the tests link are static, so a test may also reach SDL's internal event
// functions (SDL_SendKeyboardKey, ...) to fake input; they are declared here.
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <ftw.h>
#include <sys/stat.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "test.h"
#include "sdlhelp.h"

// SDL internals (src/events/SDL_keyboard_c.h, SDL_mouse_c.h, SDL_keymap_c.h).
bool SDL_SendKeyboardKey(Uint64 timestamp, SDL_KeyboardID keyboardID, int rawcode,
                         SDL_Scancode scancode, bool down);
void SDL_SendMouseMotion(Uint64 timestamp, SDL_Window *window, SDL_MouseID mouseID,
                         bool relative, float x, float y);
void SDL_SendMouseButton(Uint64 timestamp, SDL_Window *window, SDL_MouseID mouseID,
                         Uint8 button, bool down);
typedef struct SDL_Keymap SDL_Keymap;
SDL_Keymap *SDL_CreateKeymap(bool auto_release);
void SDL_SetKeymapEntry(SDL_Keymap *keymap, SDL_Scancode scancode, SDL_Keymod modstate,
                        SDL_Keycode keycode);
void SDL_SetKeymap(SDL_Keymap *keymap, bool send_event);

// A fresh folder under $TMPDIR (or /tmp) into `out`, without a trailing slash.
static void MakeTempDir(char *out, size_t size)
{
    const char *base = getenv("TMPDIR");
    snprintf(out, size, "%s/wbengine-XXXXXX", base && base[0] ? base : "/tmp");
    CHECK_MSG(mkdtemp(out) != NULL, "mkdtemp failed");
}

static int RmEntry(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
    (void)st; (void)flag; (void)ftw;
    return remove(path);
}

// Removes a folder made by MakeTempDir, with what is in it.
static void RmTree(const char *path)
{
    nftw(path, RmEntry, 16, FTW_DEPTH | FTW_PHYS);
}

static void WriteBytes(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    CHECK_MSG(f != NULL, "can't write %s", path);
    CHECK(fwrite(data, 1, size, f) == size);
    fclose(f);
}

static bool IsDir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool IsFile(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

// An uncompressed 32-bit TGA, top-left origin; `argb` is w*h pixels, rows top to bottom.
static void WriteTga(const char *path, int w, int h, const Uint32 *argb)
{
    size_t size = 18 + (size_t)w * h * 4;
    unsigned char *b = calloc(1, size);
    int i;

    b[2] = 2;                       // uncompressed true colour
    b[12] = w & 255; b[13] = w >> 8;
    b[14] = h & 255; b[15] = h >> 8;
    b[16] = 32;
    b[17] = 8 | 0x20;               // 8 alpha bits, top-left origin
    for (i = 0; i < w * h; i++) {
        b[18 + i * 4 + 0] = argb[i] & 255;          // B
        b[18 + i * 4 + 1] = (argb[i] >> 8) & 255;   // G
        b[18 + i * 4 + 2] = (argb[i] >> 16) & 255;  // R
        b[18 + i * 4 + 3] = argb[i] >> 24;          // A
    }
    WriteBytes(path, b, size);
    free(b);
}

static void Put16(unsigned char *p, unsigned v) { p[0] = v & 255; p[1] = (v >> 8) & 255; }
static void Put32(unsigned char *p, unsigned v) { Put16(p, v & 0xffff); Put16(p + 2, v >> 16); }

// A 16-bit mono PCM WAV of `frames` frames at `rate` Hz, every sample `value`.
static void WriteWav(const char *path, int rate, int frames, short value)
{
    size_t data = (size_t)frames * 2, size = 44 + data;
    unsigned char *b = calloc(1, size);
    int i;

    memcpy(b, "RIFF", 4);
    Put32(b + 4, (unsigned)(size - 8));
    memcpy(b + 8, "WAVEfmt ", 8);
    Put32(b + 16, 16);
    Put16(b + 20, 1);               // PCM
    Put16(b + 22, 1);               // mono
    Put32(b + 24, rate);
    Put32(b + 28, rate * 2);
    Put16(b + 32, 2);
    Put16(b + 34, 16);
    memcpy(b + 36, "data", 4);
    Put32(b + 40, (unsigned)data);
    for (i = 0; i < frames; i++)
        Put16(b + 44 + i * 2, (unsigned short)value);
    WriteBytes(path, b, size);
    free(b);
}

// The headless video driver, then SysInit.
static void InitVideo(void)
{
    setenv("SDL_VIDEO_DRIVER", "offscreen", 1);
    setenv("SDL_AUDIO_DRIVER", "dummy", 1);
    SysInit();
}

// SysInit and a w x h window with the software renderer.
static void OpenWindow(int w, int h)
{
    InitVideo();
    CHECK_MSG(SysCreateWindow(w, h, true, "wbengine", "software"), "SysCreateWindow: %s",
              SDL_GetError());
}

static SDL_Window *TheWindow(void)
{
    int n = 0;
    SDL_Window **all = SDL_GetWindows(&n);
    SDL_Window *w = n > 0 ? all[0] : NULL;
    SDL_free(all);
    CHECK_MSG(w != NULL, "no window");
    return w;
}

static SDL_Renderer *TheRenderer(void)
{
    SDL_Renderer *r = SDL_GetRenderer(TheWindow());
    CHECK_MSG(r != NULL, "no renderer");
    return r;
}

// The pixel (x, y) of the render target (between flips, the back buffer) as 0xAARRGGBB.
static Uint32 Pixel(int x, int y)
{
    SDL_Rect r = { x, y, 1, 1 };
    SDL_Surface *s = SDL_RenderReadPixels(TheRenderer(), &r);
    SDL_Surface *c;
    Uint32 v;

    CHECK_MSG(s != NULL, "SDL_RenderReadPixels: %s", SDL_GetError());
    c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(s);
    CHECK(c != NULL);
    v = *(Uint32 *)c->pixels;
    SDL_DestroySurface(c);
    return v;
}

// The RGB of a pixel of the back buffer, alpha ignored.
static Uint32 Rgb(int x, int y)
{
    return Pixel(x, y) & 0xffffff;
}

// A pixel of a surface as 0xRRGGBB.
static Uint32 SurfaceRgb(SDL_Surface *s, int x, int y)
{
    Uint8 r = 0, g = 0, b = 0, a = 0;
    SDL_ReadSurfacePixel(s, x, y, &r, &g, &b, &a);
    return (Uint32)r << 16 | (Uint32)g << 8 | b;
}

// Colour channels of 0xRRGGBB.
#define RED(c)   ((int)(((c) >> 16) & 255))
#define GREEN(c) ((int)(((c) >> 8) & 255))
#define BLUE(c)  ((int)((c) & 255))

// Checks a 0xRRGGBB colour, each channel within `tol`.
#define CHECK_RGB(actual, expected, tol)                                               \
    do {                                                                               \
        Uint32 ca_ = (actual), ce_ = (expected);                                       \
        if (abs(RED(ca_) - RED(ce_)) > (tol) || abs(GREEN(ca_) - GREEN(ce_)) > (tol) || \
            abs(BLUE(ca_) - BLUE(ce_)) > (tol))                                        \
            TestFail(__FILE__, __LINE__, "%s == %06x, expected %06x", #actual,        \
                     (unsigned)ca_, (unsigned)ce_);                                    \
    } while (0)

static double NowSeconds(void)
{
    return (double)SDL_GetTicksNS() / 1e9;
}
