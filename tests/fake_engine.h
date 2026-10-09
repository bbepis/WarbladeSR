#pragma once
// fake_engine.h: The engine interface (include/sdlhelp.h) as a test double, for the game-logic
// tests: no window, no sound, no SDL. Tests steer it (keys, mouse, clock, files) and inspect
// what the game did with it (sounds played, images loaded, draws).
#include "sdlhelp.h"

typedef struct FakeEngine {
    // ---- driven by the test ----
    bool keys[K_VK_ERROR + 1];
    int mouseX, mouseY;
    bool mouseLeft, mouseRight;
    unsigned millis;            // SysMillis; SysFlip advances it by flipAdvanceMs
    unsigned flipAdvanceMs;     // default 16
    bool quit;                  // SysQuitRequested
    int quitAfterFlips;         // > 0: SysQuitRequested turns true after that many flips
    SysDate localDate;          // SysLocalDate/SysUtcDate
    long long fileTime;         // SysFileTimeNow
    bool joyPresent[2];
    unsigned padBits[2], virtualPad;
    long joyX[2], joyY[2];
    float imageW, imageH;       // the size of every image ImgLoad returns (default 64 x 64)
    bool imageLoadFails;        // ImgLoad returns NULL
    bool sampleLoadFails;       // SampleLoad returns 0
    void (*onFlip)(void);       // called at the end of every SysFlip
    int createWindowFails;      // > 0: that many SysCreateWindow calls fail (counting down)

    // ---- recorded ----
    int flips;
    int imagesLoaded, imagesFreed;
    int blits;                  // Img*Blit*/ImgStretch* calls
    int rects, lines, pixels;   // DrawRect/DrawLine/PlotPixel calls
    int samplesLoaded, samplesFreed;
    int voicesStarted;          // SampleGetVoice
    int chanPlays;              // ChanPlay
    AudioHandle lastPlayed;     // the last ChanPlay'd handle
    char lastUrl[512];          // SysOpenUrl
    char lastMessageBox[512];
    bool fullscreen;
    bool vsync;
    int maxFps;
    int interpolation;
    char userFolder[512];       // a fresh temporary folder per test process
    int windowsCreated;         // successful SysCreateWindow calls
    char lastRenderDriver[32];  // SysCreateWindow's renderDriver ("" for NULL), failed or not
    bool pointerClipped;        // ClipPointer's last value
    int clipPointerCalls;
    bool pointerHidden;         // HidePointer/ShowPointer
    void (*focusCallback)(bool focused);   // SysSetFocusCallback's
} FakeEngine;

extern FakeEngine g_fake;

// Resets everything above (called before each test, and safe to call again).
void FakeReset(void);
void FakePressKey(enum EKeyboardLayout key);
void FakeReleaseKey(enum EKeyboardLayout key);
void FakeReleaseAllKeys(void);

// The data archive: files added here are what PacExists/PacRead see (names are matched
// case-insensitively, backslashes and slashes alike). Copies `data`.
void FakePacAdd(const char *name, const void *data, unsigned size);
void FakePacClear(void);

// The sample file a sample/voice handle came from ("" if unknown); SampleLoad's file name.
const char *FakeSampleName(AudioHandle handle);
// How many times a voice of the sample loaded from a file whose name contains `fragment` was
// played (ChanPlay), since the last FakeReset.
int FakePlayCount(const char *fragment);
// The file name an image was loaded from.
const char *FakeImageName(const Image *img);
// The path of `rel` (Windows-style, "warblade\\x") under the user folder, as a host path.
const char *FakeUserPath(const char *rel);
bool FakeFileExists(const char *hostPath);
