// support.c: Shared set-up for the game-logic tests (support.h).
#include <setjmp.h>
#include "support.h"

// Before each test (runner.c): a fresh fake engine. The game's globals are already fresh, the
// test running in a new process.
void TestSetUp(void)
{
    FakeReset();
}

static jmp_buf s_bootDone;

// GameMain sets g_saveMagic just before its main loop; the loop's first flip leaves it.
static void LeaveAtMainLoop(void)
{
    if (g_saveMagic == 12345) {
        g_fake.onFlip = NULL;
        longjmp(s_bootDone, 1);
    }
}

void BootGame(void)
{
    g_fake.onFlip = LeaveAtMainLoop;
    if (setjmp(s_bootDone) == 0) {
        GameMain();
        TestFail(__FILE__, __LINE__, "GameMain returned before its main loop");
    }
}

void RunFrames(int n)
{
    for (int f = 0; f < n; f++) {
        g_frameDt = g_autoplay ? 3.0f : g_gameSpeedMul;
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
        if (g_skipRender == 0 && g_state != STATE_UNUSED_19) {
            g_frameFunc();
            FlushBlit(0);
            FlushQuads(0);
            FlushStretchF();
            FlushStretchRot();
            FlushStretchI();
            FlushStretchRot2();
            FlushBlit2(0);
            if (AnyWindowActive())
                WinUpdateAll();
            FlushBlit(0);
            FlushQuads(0);
            FlipBuffer(0);
        } else {
            g_fake.millis += g_fake.flipAdvanceMs;
        }
    }
}

bool RunFramesUntil(bool (*done)(void), int max)
{
    for (int f = 0; f < max; f++) {
        if (done())
            return true;
        RunFrames(1);
    }
    return done();
}

void TapKey(enum EKeyboardLayout key, int frames)
{
    FakePressKey(key);
    RunFrames(frames);
    FakeReleaseKey(key);
    RunFrames(frames);
}
