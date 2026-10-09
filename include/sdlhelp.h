#pragma once
// sdlhelp.h: the engine interface the game uses instead of PTK, BASS and Win32.
//
// A plain C API: free functions, opaque handles, no default arguments, no overloads. It is
// shaped like the PTK, BASS and Win32 calls the game made (same arguments, same meanings), so
// the game code converted mechanically (SDL_PLAN.md Step 2). It is implemented on SDL3,
// SDL3_image and SDL3_mixer in src/core/sdl.c and src/core/sdl_audio.c (Step 4), the only
// files that include SDL's headers.
//
#include <stdbool.h>
#include <stddef.h>

// MSVC's 64-bit integer keyword (MinGW defines it too).
#if !defined(_MSC_VER) && !defined(__int64)
#define __int64 long long
#endif

// Compile-time check (the Win32 SDK's definition, so the two never clash).
#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e) ? 1 : -1]
#endif

// ---------------------------------------------------------------------------------------------
// Window, frame and events (PTK KWindow)
// ---------------------------------------------------------------------------------------------

void SysInit(void);                 // first call of all (KMiscTools::initMiscTools)

// Creates the game window and its w x h back buffer, which the game draws into and which keeps
// its contents between frames. Fullscreen shows it scaled to the screen, letterboxed; a
// window can be resized. `renderDriver` is an SDL_HINT_RENDER_DRIVER value ("direct3d11", "opengl", ...) or NULL to let SDL choose;
// only that driver is tried. Returns false if the window or its renderer couldn't be created.
bool SysCreateWindow(int w, int h, bool windowed, const char *title, const char *renderDriver);
const char *SysRendererName(void);  // the renderer in use ("direct3d11", ...), "" if none
void SysDestroyWindow(void);        // safe when there is no window
bool SysHasWindow(void);
void SysTerminate(void);            // ends the game loop (SysQuitRequested())
bool SysQuitRequested(void);        // quit asked for: SysTerminate, Alt-F4, the close box
void SysProcessEvents(void);        // handles pending window events
// Shows the back buffer, processes events, then waits so that frames are at least 1000/fps
// whole milliseconds apart (PTK's cap: 16 ms, 62.5 fps, for 60). While the window is in the
// background it waits for it to come back, as PTK's flipBackBuffer(true) did.
// With interpolation on, it shows frames at the display's rate until the next frame is due,
// each between the last two frames' pictures; the game's frame rate stays the same.
void SysFlip(void);
void SysSetMaxFps(int fps);         // 0 or less: no cap
void SysSetVSync(bool on);          // presents wait for the vertical blank (default on)
// Interpolation modes. Auto interpolates unless vsync is on and the display's refresh rate is
// the game's frame rate (a 60 Hz display at 60 fps), where it gains nothing.
enum { SYS_INTERP_AUTO = 0, SYS_INTERP_ON = 1, SYS_INTERP_OFF = 2, SYS_INTERP_COUNT };
void SysSetInterpolation(int mode); // SYS_INTERP_*; anything else is auto
bool SysInterpolating(void);        // whether frames are being interpolated now
void SysSetClearColor(float r, float g, float b, float a);
// PTK's world transform; only the identity is supported (the game sets nothing else).
// `clear` clears the back buffer to the clear colour.
void SysSetWorldView(float x, float y, float rotation, float zoom, bool clear);
void SysMinimize(void);
bool SysHasFocus(void);
void SysSetIcon(const char *icoFile);   // window icon from an .ico file
void SysDisableScreenSaver(void);
void SysSetFullscreen(const bool fullscreen);
// Called with true when the window gains focus, false when it loses it. Replaces the
// WM_ACTIVATE half of the game's window-message hook (SDL itself keeps the screensaver and
// the Alt system menu away).
void SysSetFocusCallback(void (*fn)(bool focused));
// Saves the back buffer as a JPEG, resized to w x h (KWindow::saveBackBuffer(file, 1, w, h)).
bool SysScreenshot(const char *file, int w, int h);
int  SysDesktopWidth(void);         // usable desktop size of the primary display
int  SysDesktopHeight(void);
void SysMessageBox(const char *title, const char *text);

// ---------------------------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------------------------

unsigned SysMillis(void);           // ms clock (was timeGetTime); never near 0
long long SysPerfCounter(void);     // high-resolution counter (was QueryPerformanceCounter)
long long SysPerfFreq(void);        // its ticks per second

// Calendar date and time; the same layout as Win32's SYSTEMTIME (it is stored raw in the
// account, settings and hiscore files).
typedef struct SysDate {
    unsigned short year;
    unsigned short month;           // 1-12
    unsigned short dayOfWeek;       // 0 = Sunday
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short milliseconds;
} SysDate;
C_ASSERT(sizeof(SysDate) == 16);

// Wall-clock time as a Win32 FILETIME value: 100 ns units since 1601-01-01 UTC. The game
// stores these in its files and subtracts them for durations.
long long SysFileTimeNow(void);
void SysFileTimeToDate(long long ft, SysDate *out);    // UTC
void SysUtcDate(SysDate *out);
void SysLocalDate(SysDate *out);

// ---------------------------------------------------------------------------------------------
// Files, folders, programs
// ---------------------------------------------------------------------------------------------

// The user's documents folder (KMiscTools::getUserFolder), without a trailing separator; the
// game keeps its files in "<folder>\warblade". Static buffer.
const char *SysUserFolder(void);
// `rel` resolved against the program's folder (KMiscTools::makeFilePath). Static buffer,
// overwritten by the next call.
const char *SysAppPath(const char *rel);
// The game's Windows-style path ("data\\samples\\x.mp3") as the host OS wants it. On Windows
// that is `path` itself. Elsewhere backslashes become slashes and each component is matched
// case-insensitively against what's on disk, as Windows' file system would (a component that
// doesn't exist yet is kept as written). Returns one of 4 rotating static buffers.
const char *SysPath(const char *path);
bool SysFileExists(const char *path);   // a file or folder
bool SysMakeDir(const char *path);      // true if created
// Opens `url` in the default browser.
void SysOpenUrl(const char *url);
// Starts Jukebox.exe from the program's folder (the original brought a running one to the
// front instead).
void SysOpenJukebox(void);

// The data archive (data\warblade.pac, a tar). Names inside it are looked up
// case-insensitively, as KResource did; a name not in the archive is read as a loose file.
void PacAddArchive(const char *path);                      // KResourceArchive::addArchive
bool PacExists(const char *name);
// Reads the first `size` bytes of `name` into `buf` (a shorter file fills only its length).
// False if it can't be opened.
bool PacRead(const char *name, void *buf, unsigned size);

// ---------------------------------------------------------------------------------------------
// Images (PTK KGraphic). g->method(args) became ImgMethod(g, args), same arguments.
// ---------------------------------------------------------------------------------------------

typedef struct Image Image;         // opaque

// Loads a picture (TGA/JPG/PNG, from the archive or the disk). NULL on failure.
// Replaces createKGraphic + loadPicture; `hiDef` and `hasAlpha` no longer matter.
// A new image has alpha mode 1, no tint and texture quality off.
Image *ImgLoad(const char *file, bool hiDef, bool hasAlpha);
void  ImgFree(Image *img);          // delete; NULL is fine
void  ImgFreePicture(Image *img);   // frees the picture but keeps the object (freePicture)
// True once the image has been ImgFree'd. The render queues use it to skip an image freed
// since it was queued; the original read the freed KGraphic for this (a use-after-free),
// here a freed image's memory is kept for a few frames so the check is safe.
bool  ImgIsFreed(const Image *img);
float ImgWidth(Image *img);
float ImgHeight(Image *img);
void  ImgSetTextureQuality(Image *img, bool quality);    // true = linear filtering
// Blend factors (src, dst) per mode, as KGraphicGL::setAlphaMode set them: 0 SRC_ALPHA/
// DST_ALPHA (additive, the back buffer's alpha being 1), 1 SRC_ALPHA/ONE_MINUS_SRC_ALPHA
// (normal), 2 ONE/ONE_MINUS_SRC_ALPHA, 3 ZERO/ONE_MINUS_SRC_ALPHA, 4 SRC_ALPHA/ONE (additive),
// 5 ONE/ZERO, 6 ONE_MINUS_SRC_ALPHA/ONE; anything else: no change.
void  ImgSetAlphaMode(Image *img, short mode);
// Tint, 0-1 (above 1 is clamped): multiplies the colour, and `a` the alpha, of every blit.
void  ImgSetBlitColor(Image *img, float r, float g, float b, float a);
// The blits draw the source rect (x1,y1)-(x2,y2) of the picture; given backwards (x2 < x1)
// it is mirrored. Angles are degrees, counter-clockwise, about the centre of the destination
// rect + (centerX, centerY); the alpha is the tint's times `blend`.
void  ImgBlitAlphaRect(Image *img, float x1, float y1, float x2, float y2,
                       short destX, short destY, bool flipX, bool flipY);
// Also zoomed about that centre; nothing is drawn at zoom 0 or blend <= 0.
void  ImgBlitAlphaRectFx(Image *img, float x1, float y1, float x2, float y2,
                         short destX, short destY, float angle, float zoom, float blend,
                         bool flipX, bool flipY, float centerX, float centerY);
// PTK's "blit without alpha": it blends exactly like ImgBlitAlphaRect.
void  ImgBlitRectF(Image *img, float x1, float y1, float x2, float y2,
                   float destX, float destY, bool flipX, bool flipY);
// Without a tint, a negative blend draws opaque.
void  ImgStretchAlphaRect(Image *img, float sx1, float sy1, float sx2, float sy2,
                          float dx1, float dy1, float dx2, float dy2, float blend, float angle,
                          bool flipX, bool flipY, float centerX, float centerY);

// Primitives on the back buffer (KGraphic methods on the game's g_screen). Colours 0-1;
// whole-pixel coordinates; normal alpha blending when `a` is not 1, else opaque.
void DrawRect(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
void PlotPixel(float x, float y, float r, float g, float b, float a);

// ---------------------------------------------------------------------------------------------
// Input (PTK KInput)
// ---------------------------------------------------------------------------------------------

// Key codes: PTK's key-table order. The values are stored in the settings file
// (WarBlade.inf), so they never change. 0-92 are PTK's documented order; 93-99 exist only
// in the PTK build the game shipped with.
enum EKeyboardLayout {
    K_VK_LEFT = 0, K_VK_UP, K_VK_DOWN, K_VK_RIGHT, K_VK_SPACE,
    K_VK_L_SHIFT, K_VK_R_SHIFT, K_VK_RETURN, K_VK_R_CONTROL, K_VK_L_CONTROL,
    K_VK_F1, K_VK_F2, K_VK_F3, K_VK_F4, K_VK_F5, K_VK_F6,
    K_VK_F7, K_VK_F8, K_VK_F9, K_VK_F10, K_VK_F11, K_VK_F12,
    K_VK_BACK, K_VK_TAB, K_VK_ESCAPE,                                        // 22-24
    K_VK_A, K_VK_B, K_VK_C, K_VK_D, K_VK_E, K_VK_F, K_VK_G, K_VK_H, K_VK_I,  // 25-
    K_VK_J, K_VK_K, K_VK_L, K_VK_M, K_VK_N, K_VK_O, K_VK_P, K_VK_Q, K_VK_R,
    K_VK_S, K_VK_T, K_VK_U, K_VK_V, K_VK_W, K_VK_X, K_VK_Y, K_VK_Z,          // -50
    K_VK_0, K_VK_1, K_VK_2, K_VK_3, K_VK_4, K_VK_5, K_VK_6, K_VK_7, K_VK_8, K_VK_9,   // 51-60
    K_VK_NUM0, K_VK_NUM1, K_VK_NUM2, K_VK_NUM3, K_VK_NUM4,
    K_VK_NUM5, K_VK_NUM6, K_VK_NUM7, K_VK_NUM8, K_VK_NUM9,                   // 61-70
    K_VK_MULTIPLY, K_VK_ADD, K_VK_SUBTRACT, K_VK_DECIMAL, K_VK_DIVIDE,       // 71-75
    K_VK_CLEAR, K_VK_MENU, K_VK_LWIN, K_VK_RWIN, K_VK_NUMLOCK, K_VK_SCROLL,  // 76-81
    K_VK_OEM_1, K_VK_OEM_PLUS, K_VK_OEM_COMMA, K_VK_OEM_MINUS, K_VK_OEM_PERIOD,
    K_VK_OEM_2, K_VK_OEM_3, K_VK_OEM_4, K_VK_OEM_5, K_VK_OEM_6, K_VK_OEM_7,  // 82-92
    K_VK_END, K_VK_HOME, K_VK_DELETE, K_VK_INSERT,                           // 93-96
    K_VK_PRINTSCREEN, K_VK_PAGEUP, K_VK_PAGEDOWN,                            // 97-99
    K_VK_ERROR                                                               // 100: "no key"
};

// KInput::isPressed. Letters, digits and punctuation follow the keyboard layout, as the
// Windows virtual keys did; the other keys are by position.
typedef enum EKeyboardLayout EKeyboardLayout;

bool KeyDown(enum EKeyboardLayout key);
int  MouseX(void);                        // back-buffer coordinates (outside it: < 0 or >= w/h)
int  MouseY(void);
bool MouseLeft(void);
bool MouseRight(void);
void MouseWarp(int x, int y);             // moves the pointer, back-buffer coordinates
void ShowPointer(void);
void HidePointer(void);
void ClipPointer(bool on);                // fullscreen: confine the pointer to the picture
// Joysticks 0 and 1, reported like winmm's joyGetPosEx: axes 0-65535, centre 32767.
bool JoyEnable(char joy);
long JoyX(char joy, char hat);            // hat 0: X axis, else Z; 0x7fff if not enabled
long JoyY(char joy, char hat);            // hat 0: Y axis, else R
bool JoyButton(char joy, long mask);      // `mask` is a button bit mask

// Gamepads through SDL's gamepad API (Xbox-style layout, whatever the brand), not part of the
// original: pad n is the n-th one connected. Each reports the game's standard controls; the left
// stick and the d-pad move. Pad 0 also includes the virtual pad (the browser page's touch
// controls, SysSetVirtualPad).
enum {
    PAD_LEFT = 1, PAD_RIGHT = 2, PAD_UP = 4, PAD_DOWN = 8,
    PAD_FIRE = 16,      // A (south), right trigger
    PAD_ROCKET = 32,    // B (east), X (west), left trigger
    PAD_PAUSE = 64,     // Start
    PAD_PROFILE = 128   // Back / Select
};
unsigned SysPadState(int pad);            // PAD_* bits held now; 0 if there's no such pad
void SysSetVirtualPad(unsigned bits);     // PAD_* bits held on the touch controls

// ---------------------------------------------------------------------------------------------
// Audio (src/core/sdl_audio.c, shaped like BASS). Handles are BASS-style: a sample, a voice
// playing it, a tracker module or a stream; 0 is "none". A handle that has gone stale (a freed
// sample, a voice taken over by another sound) is ignored.
// ---------------------------------------------------------------------------------------------

typedef unsigned int AudioHandle;   // 32 bits, as on Windows: it is stored in the game's structs

enum AudioAttrib {
    AUDIO_FREQ = 1,                 // playback rate in Hz (a sample's own rate = normal pitch)
    AUDIO_VOL  = 2,                 // 0-1
    AUDIO_PAN  = 3                  // -1 (left) to 1 (right); outside that ignored, as BASS did
};
typedef enum AudioAttrib AudioAttrib;

enum {
    AUDIO_SAMPLE_LOOP     = 1       // SampleLoad flags
};

bool AudioInit(void);               // the default device, 44100 Hz stereo
void AudioShutdown(void);
int  AudioError(void);              // the last error code, for logging (0: none)
void AudioUpdate(void);             // BASS_Update: nothing to do (the mixer runs on its own)
void AudioStart(void);              // BASS_Start: resume output
void AudioStop(void);               // BASS_Stop: stop output and every channel
void AudioPause(void);              // BASS_Pause: pause output

// A sample plays on up to `maxVoices` voices at once; when all are busy, SampleGetVoice takes
// over the quietest (BASS_SAMPLE_OVER_VOL).
AudioHandle SampleLoad(const char *file, int maxVoices, int flags);   // 0 on failure
void        SampleFree(AudioHandle sample);
AudioHandle SampleGetVoice(AudioHandle sample);   // a voice to play it on, at volume 1
AudioHandle MusicLoad(const char *file);          // tracker module (or any stream), looping
void        MusicFree(AudioHandle music);
AudioHandle StreamLoad(const char *file);         // MP3/OGG stream, looping
void        StreamFree(AudioHandle stream);

// For voices, modules and streams:
bool   ChanPlay(AudioHandle ch, bool restart);
void   ChanStop(AudioHandle ch);
void   ChanSet(AudioHandle ch, enum AudioAttrib attrib, float value);
void   ChanSlide(AudioHandle ch, enum AudioAttrib attrib, float value, int ms);
double ChanLength(AudioHandle ch);                // seconds; also takes a sample handle
unsigned long ChanGetPos(AudioHandle ch);         // an opaque position for ChanSetPos
void   ChanSetPos(AudioHandle ch, unsigned long pos);
