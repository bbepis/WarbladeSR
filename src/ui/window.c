// window.c: GUI windows: the window table, widgets (text, rects, menu items, toggles, edit
// boxes), focus, frames and text boxes, drawing.
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "game.h"


// Closes every active GUI window: sliding (mode 2) windows are set sliding off-screen
// and stay active, others are deactivated and hidden immediately.
void WinCloseAll()
{
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].active != 0) {
            if (g_windows[i].mode == WIN_MODE_SLIDING) {
                g_windows[i].visible = 0;
                g_windows[i].nD = -1;
                g_windows[i].nF = -1;
                g_windows[i].nG = -1;
                g_windows[i].nH = -1;
                g_windows[i].slideX = -5.0f;
                g_windows[i].slideY = 1.2f;

            } else {
                g_windows[i].visible = 0;
                g_windows[i].active = 0;
                g_windows[i].nE = -1;
                g_windows[i].nD = -1;
                g_windows[i].nF = -1;
                g_windows[i].nG = -1;
                g_windows[i].nH = -1;
                g_windows[i].nC = -1;
                g_windows[i].nB = -1;
                g_windows[i].nA = -1;
            }
        }
    }
}

// Resets the whole GUI window table to its inactive default state.
void WinInit()
{
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        g_windows[i].active = 0;
        g_windows[i].index = i;
        g_windows[i].nD = -1;
        g_windows[i].nE = -1;
        g_windows[i].nF = -1;
        g_windows[i].nG = -1;
        g_windows[i].nH = -1;
        g_windows[i].nC = -1;
        g_windows[i].nB = -1;
        g_windows[i].nA = -1;
        g_windows[i].blinkTimer = 25.0f;
        g_windows[i].blinkOn = 0;
        g_windows[i].firstH = -1;
    }
    WinCloseAll();
}

// Returns the index of the first inactive window slot, or -1 if all 10 are in use.
int WinFindFree()
{
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].active == 0)
            return i;
    }
    return -1;
}

// Allocates and opens a GUI window at (x,y) sized w x h in the given mode, clearing
// its control/field lists. Mode 2 windows start off-screen and slide in. Plays the
// window-open sound. Returns the window index, or -1 if none is free.
int WinOpen(int x, int y, int w, int h, int mode)
{
    int win;
    int i;
    int j;

    win = WinFindFree();
    if (win != -1) {
        SoundPlay(g_sfxWindow, 30000, 200, 0.0f, 191, g_sndFlags);
        g_windows[win].visible = 1;
        g_windows[win].active = 1;
        g_windows[win].x = x;
        g_windows[win].y = y;
        g_windows[win].nF = -1;
        g_windows[win].selF = -1;
        g_windows[win].nE = -1;
        g_windows[win].nD = -1;
        g_windows[win].nG = -1;
        g_windows[win].nH = -1;
        g_windows[win].nC = -1;
        g_windows[win].nB = -1;
        g_windows[win].nA = -1;
        g_windows[win].blinkTimer = 25.0f;
        g_windows[win].blinkOn = 0;
        g_windows[win].firstH = -1;
        g_windows[win].h = h;
        g_windows[win].w = w;
        g_windows[win].mode = mode;
        g_windows[win].slideX = 0.0f;
        g_windows[win].slideY = 0.0f;

        if (mode == WIN_MODE_SLIDING) {
            g_windows[win].slideX = -700.0f;
            g_windows[win].slideY = 0.85f;
        }

        for (i = 0; i < MAX_WINDOW_IMAGE_RECTS; i++)
            g_windows[win].imageRects[i].graphic = 0;
        for (j = 0; j < MAX_WINDOW_MENU_ITEMS; j++)
            g_windows[win].menuItems[j].checked = 0;
    }
    return win;
}

// Hides every window without deactivating it.
void WinHideAll()
{
    int i;
    for (i = 0; i < MAX_WINDOWS; i++)
        g_windows[i].visible = 0;
}

// Closes window win: a sliding (mode 2) window is set sliding off-screen and stays
// active, otherwise it is deactivated and hidden immediately.
// NOTE: the outer loop counts down an unused index and has no effect other than
// repeating the close 10 times; kept as in the original.
void WinClose(int win)
{
    int i;
    for (i = 9; i != -1; i--) {
        if (g_windows[win].active != 0) {
            if (g_windows[win].mode == WIN_MODE_SLIDING) {
                g_windows[win].slideX = -5.0f;
                g_windows[win].slideY = 1.2f;
            } else {
                g_windows[win].visible = 0;
                g_windows[win].active = 0;
                g_windows[win].nE = -1;
                g_windows[win].nD = -1;
                g_windows[win].nF = -1;
                g_windows[win].nG = -1;
                g_windows[win].nH = -1;
                g_windows[win].nC = -1;
                g_windows[win].nB = -1;
                g_windows[win].nA = -1;
            }
        }
    }
}

// Appends a static text line (list E, up to 50 per window) to window win at (x,y) in
// the given color.
void WinAddText(int x, int y, int win, char *text, int color)
{
    char *p;
    int i = 0;
    if (g_windows[win].active != 0 && g_windows[win].nE < 50) {
        g_windows[win].nE++;
        g_windows[win].texts[g_windows[win].nE].x = x;
        g_windows[win].texts[g_windows[win].nE].y = y;
        g_windows[win].texts[g_windows[win].nE].color = color;
        p = text;
        for (i = 0; *p != 0 || i >= 255; p++, i++) {
            g_windows[win].texts[g_windows[win].nE].text[i] = *p;
        }
        if (i < 255) {
            g_windows[win].texts[g_windows[win].nE].text[i] = 0;
        }
    }
}

// Appends a two-line hilite-able text entry (list D, up to 10 per window) to window
// win at (x,y), with normal color a and hover color b. id selects the entry's id, or
// its list index if -1.
void WinAddTextPair(int x, int y, int win, int id, char *text1, char *text2, int a, int b)
{
    char *p;
    int i = 0;
    if (g_windows[win].active != 0 && g_windows[win].nD < 10) {
        g_windows[win].nD++;
        g_windows[win].links[g_windows[win].nD].x = x;
        g_windows[win].links[g_windows[win].nD].y = y;
        g_windows[win].links[g_windows[win].nD].color = a;
        g_windows[win].links[g_windows[win].nD].hoverColor = b;

        for (i = 0; i < 255; i++) {
            g_windows[win].links[g_windows[win].nD].text1[i] = 0;
        }
        for (i = 0; i < 255; i++) {
            g_windows[win].links[g_windows[win].nD].text2[i] = 0;
        }

        p = text1;
        for (i = 0; *p != 0 || i >= 255; p++, i++) {
            g_windows[win].links[g_windows[win].nD].text1[i] = *p;
        }
        p = text2;
        for (i = 0; *p != 0 || i >= 255; p++, i++) {
            g_windows[win].links[g_windows[win].nD].text2[i] = *p;
        }
        if (id == -1) {
            g_windows[win].links[g_windows[win].nD].id = g_windows[win].nD;
        } else {
            g_windows[win].links[g_windows[win].nD].id = id;
        }
    }
}

// Appends an image rect (list C, up to 5 per window) to window win: destination
// (a,b) sized c x d, drawing graphic e.
void WinAddRect(int a, int b, int c, int d, int win, Image* e)
{
    int i = 0;
    if (g_windows[win].active != 0 && g_windows[win].nC < MAX_WINDOW_IMAGE_RECTS) {
        g_windows[win].nC++;
        g_windows[win].imageRects[g_windows[win].nC].x = a;
        g_windows[win].imageRects[g_windows[win].nC].y = b;
        g_windows[win].imageRects[g_windows[win].nC].w = c;
        g_windows[win].imageRects[g_windows[win].nC].h = d;
        g_windows[win].imageRects[g_windows[win].nC].graphic = e;
    }
}

// Appends a source/dest blit-quad item (list A, up to 150 per window) to window win:
// destination rect (a,b,c,d), source rect (e,f,g,h) in graphic j.
void WinAddItemA(int a, int b, int c, int d, int e, int f, int g, int h, int win, void *j)
{
    int i = 0;
    if (g_windows[win].active != 0 && g_windows[win].nA < 150) {
        g_windows[win].nA++;
        g_windows[win].quadImages[g_windows[win].nA].destX = a;
        g_windows[win].quadImages[g_windows[win].nA].destY = b;
        g_windows[win].quadImages[g_windows[win].nA].destW = c;
        g_windows[win].quadImages[g_windows[win].nA].destH = d;
        g_windows[win].quadImages[g_windows[win].nA].srcX = e;
        g_windows[win].quadImages[g_windows[win].nA].srcY = f;
        g_windows[win].quadImages[g_windows[win].nA].srcW = g;
        g_windows[win].quadImages[g_windows[win].nA].srcH = h;
        g_windows[win].quadImages[g_windows[win].nA].graphic = j;
    }
}

// Appends a medal/icon item with a hover tooltip (list B, up to 10 per window) to
// window win: destination (a,b), source (c,d) sized e x f, tooltip text.
void WinAddItemB(int a, int b, int c, int d, int e, int f, char *text, int win)
{
    char *p;
    int i = 0;
    if (g_windows[win].active != 0 && g_windows[win].nB < 10) {
        g_windows[win].nB++;
        g_windows[win].buttons[g_windows[win].nB].x = a;
        g_windows[win].buttons[g_windows[win].nB].y = b;
        g_windows[win].buttons[g_windows[win].nB].srcX = c;
        g_windows[win].buttons[g_windows[win].nB].srcY = d;
        g_windows[win].buttons[g_windows[win].nB].w = e;
        g_windows[win].buttons[g_windows[win].nB].h = f;
        g_windows[win].buttons[g_windows[win].nB].hover = 0;
        p = text;
        for (i = 0; *p != 0 || i >= 255; p++, i++) {
            g_windows[win].buttons[g_windows[win].nB].text[i] = *p;
        }
        if (i < 255) {
            g_windows[win].buttons[g_windows[win].nB].text[i] = 0;
        }
    }
}

// Does nothing; used as a placeholder callback.
void EmptyWindowStub()
{
}

// Appends a clickable menu item/button (list F, up to 40 per window) to window win at
// (x,y): text sets its width (8px/char + 16), param selects its visual style. id
// selects the entry's id, or its list index if -1.
void WinAddMenuItem(int x, int y, int win, int id, char *text, int param)
{
    char *p;
    int i = 0;
    if (g_windows[win].active != 0 && g_windows[win].nF < MAX_WINDOW_MENU_ITEMS) {
        g_windows[win].nF++;
        g_windows[win].menuItems[g_windows[win].nF].hilite = 0;
        g_windows[win].menuItems[g_windows[win].nF].x = x;
        g_windows[win].menuItems[g_windows[win].nF].y = y;
        g_windows[win].menuItems[g_windows[win].nF].param = param;

        p = text;
        for (i = 0; *p != 0 || i >= 255; p++, i++) {
            g_windows[win].menuItems[g_windows[win].nF].text[i] = *p;
        }
        if (i < 255) {
            g_windows[win].menuItems[g_windows[win].nF].text[i] = 0;
        }

        g_windows[win].menuItems[g_windows[win].nF].len = i;
        g_windows[win].menuItems[g_windows[win].nF].h = 15;
        g_windows[win].menuItems[g_windows[win].nF].w = i * 8 + 16;
        g_windows[win].menuItems[g_windows[win].nF].state = 0;
        if (id == -1) {
            g_windows[win].menuItems[g_windows[win].nF].id = g_windows[win].nF;
        } else {
            g_windows[win].menuItems[g_windows[win].nF].id = id;
        }
    }
}

// Sets the highlighted menu item in window win, if it has any (list F non-empty).
void WinSetSelected(int win, int sel)
{
    if (g_windows[win].nF == -1) {
        return;
    }
    g_windows[win].selF = sel;
}

// Marks the menu item with the given id "checked" in window win.
void WinCheckMenuItem(int win, int id)
{
    int i;
    if (g_windows[win].nF == -1) {
        return;
    }
    for (i = 0; i < g_windows[win].nF + 1; i++) {
        if (g_windows[win].menuItems[i].id == id) {
            g_windows[win].menuItems[i].checked = 1;
        }
    }
}

// Clears the "checked" flag on every menu item in every window.
void WinClearMenuChecks()
{
    int w;
    int i;
    for (w = 0; w < MAX_WINDOWS; w++) {
        for (i = 0; i < MAX_WINDOW_MENU_ITEMS; i++) {
            g_windows[w].menuItems[i].checked = 0;
        }
    }
}

// Appends an on/off toggle (list G, up to 50 per window) to window win at (x,y) with
// its initial value and label text; flag2 enables the hover tooltip. Tracks the
// running on/off toggle counts and advances g_nextX past the toggle box.
void WinAddToggle(int x, int y, int win, bool value, char *text, bool flag2)
{
    char *p;
    int i = 0;
    if (g_windows[win].active != 0 && g_windows[win].nG < 50) {
        g_windows[win].nG++;
        g_windows[win].toggles[g_windows[win].nG].x = x;
        g_windows[win].toggles[g_windows[win].nG].y = y;
        g_windows[win].toggles[g_windows[win].nG].w = 8;
        g_windows[win].toggles[g_windows[win].nG].h = 11;
        g_windows[win].toggles[g_windows[win].nG].value = value;
        if (value) {
            g_toggleOnCount++;
        } else {
            g_toggleOffCount++;
        }

        g_windows[win].toggles[g_windows[win].nG].state = 0;
        g_windows[win].toggles[g_windows[win].nG].index = g_windows[win].nG;
        g_windows[win].toggles[g_windows[win].nG].flag2 = flag2;
        p = text;
        for (i = 0; *p != 0 || i >= 255; p++, i++) {
            g_windows[win].toggles[g_windows[win].nG].text[i] = *p;
        }
        if (i < 255) {
            g_windows[win].toggles[g_windows[win].nG].text[i] = 0;
        }
        g_nextX = x + 8;
    }
}

// Appends a text edit field (list H, up to 30 per window) to window win at (x,y): len
// characters, masked (password-style) if c is set, color/style param, initially
// focused if d is set. The first edit field added becomes the window's tab-focus start.
// Advances g_nextX past the field.
void WinAddEdit(int x, int y, int win, int len, unsigned char c, int param, unsigned char d)
{
    int i;
    if (g_windows[win].active != 0 && g_windows[win].nH < 30) {
        g_windows[win].nH++;
        g_windows[win].edits[g_windows[win].nH].x = x;
        g_windows[win].edits[g_windows[win].nH].y = y;
        g_windows[win].edits[g_windows[win].nH].len = len;
        g_windows[win].edits[g_windows[win].nH].masked = c;
        g_windows[win].edits[g_windows[win].nH].cursor = 0;
        g_windows[win].edits[g_windows[win].nH].index = g_windows[win].nH;
        g_windows[win].edits[g_windows[win].nH].param = param;
#ifdef __EMSCRIPTEN__
        memset(g_windows[win].edits[g_windows[win].nH].buf, 0,
               sizeof(g_windows[win].edits[g_windows[win].nH].buf));
#endif
        for (i = 0; i < len; i++) {
            g_windows[win].edits[g_windows[win].nH].buf[i] = 0;
        }
        g_windows[win].edits[g_windows[win].nH].focused = d;
        if (g_windows[win].firstH == -1) {
            g_windows[win].firstH = g_windows[win].nH;
        }
        g_nextX = x + len * 8;
    }
}

// Returns true if any GUI window is currently active.
bool AnyWindowActive()
{
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].active != 0) {
            return true;
        }
    }
    return false;
}

// Returns true if any active GUI window has an edit field.
bool AnyWindowHasEdit()
{
    int i;
    for (i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].active && g_windows[i].visible && g_windows[i].nH >= 0
            && g_windows[i].firstH >= 0 && g_windows[i].firstH <= g_windows[i].nH) {
            return true;
        }
    }
    return false;
}

// Moves keyboard focus to the next edit field below the currently focused one in
// window win (by y position), wrapping to the topmost field if none is lower. Does
// nothing if the window has 0 or 1 edit fields.
void WinFocusNextEdit(int win)
{
    int curY;
    int i;
    int j;
    int minY;
    int k;
    if (g_windows[win].nH + 1 == 1) {
        return;
    }
    curY = g_windows[win].edits[g_windows[win].firstH].y;
    for (i = 0; i < g_windows[win].nH + 1; i++) {
        g_windows[win].edits[i].focused = 0;
    }

    // ---- find the first field below the current one ----
    for (j = 0; j < g_windows[win].nH + 1; j++) {
        if (g_windows[win].edits[j].y > curY) {
            g_windows[win].firstH = j;
            g_windows[win].edits[j].focused = 1;
            return;
        }
    }

    // ---- none lower: wrap to the topmost field ----
    minY = 10000;
    for (k = 0; k < g_windows[win].nH + 1; k++) {
        if (g_windows[win].edits[k].y < minY) {
            minY = g_windows[win].edits[k].y;
            g_windows[win].firstH = k;
            g_windows[win].edits[k].focused = 1;
        }
    }
}

#define W g_windows[win]

// Draws window win: the sliding-in/out frame (a tiled dialog frame for mode 1, or a
// skinned/scanline frame for mode 2, sliding by W.slideX and closing the window once
// it has slid fully off-screen), then each of its widget lists in turn (text E, text
// pairs D, menu items F, toggles G, edit fields H, image rects C, blit items A, icon
// items B), and finally flushes the queued draw batches.
void WinDraw(int win)
{
    int x1;
    int y1;
    int x2;
    int y2;
    int i;
    int j;
    int t;
    int v;
    int tf;
    float slide;
    int k;
    int m;
    int k2;
    int m2;
    int e;
    int d;

    bool anyChecked;
    int f;
    int n;
    int q;
    int g;
    int g2;
    int off;
    int h;
    int c;
    int z;
    int ci;
    int a;
    int b;

    x1 = W.x;
    y1 = W.y;
    x2 = x1 + W.w;
    y2 = y1 + W.h;
    W.slideX = W.slideX * W.slideY;
    if (W.slideX < -750.0) {
        W.active = 0;
        return;
    }
    if (FabsWindow(W.slideX) < 0.1f)
        W.slideX = 0;
    slide = W.slideX;

    // ---- resolve a centered position (POS_CENTERED sentinel) ----
    if (x1 == POS_CENTERED) {
        x1 = (g_screenW >> 1) - (W.w >> 1);
        W.x = x1;
        x2 = x1 + W.w;
    }
    if (y1 == POS_CENTERED) {
        y1 = (g_screenH >> 1) - (W.h >> 1);
        W.y = y1;
        y2 = y1 + W.h;
    }
    // ---- mode 1: tiled dialog frame ----
    if (W.mode == WIN_MODE_TILED) {
        for (i = 0; i < (x2 - (x1 + 32)) / 32; i++) {
            for (j = 0; j < (y2 - (y1 + 32)) / 32; j++)
                Blit(x1 + i * 32 + 32, y1 + j * 32 + 32, 0, g_gfxLogos, 0x70, 0xbe, 0x20, 0x20);
        }
        for (k = 0; k < (x2 - 16 - (x1 + 16)) / 16; k++) {
            Blit(x1 + k * 16 + 16, y1, 0, g_gfxLogos, 0x20, 0xbe, 0x10, 0x20);
            Blit(x1 + k * 16 + 16, y2 - 32, 0, g_gfxLogos, 0x20, 0xde, 0x10, 0x20);
        }
        for (m = 0; m < (y2 - 16 - (y1 + 16)) / 16; m++) {
            Blit(x1, y1 + m * 16 + 16, 0, g_gfxLogos, 0x50, 0xce, 0x20, 0x10);
            Blit(x2 - 32, y1 + m * 16 + 16, 0, g_gfxLogos, 0x50, 0xbe, 0x20, 0x10);
        }

        Blit(x1, y1, 0, g_gfxLogos, 0, 0xbe, 0x20, 0x20);
        Blit(x2 - 32, y1, 0, g_gfxLogos, 0x30, 0xbe, 0x20, 0x20);
        Blit(x1, y2 - 32, 0, g_gfxLogos, 0, 0xde, 0x20, 0x20);
        Blit(x2 - 32, y2 - 32, 0, g_gfxLogos, 0x30, 0xde, 0x20, 0x20);
    }

    // ---- mode 2: skinned/scanline frame ----
    if (W.mode == WIN_MODE_SLIDING) {
        if (g_winGfx != 0) {
            Blit(x1 - 90 + (int)slide, y1 - 81, 0, g_winGfx, 0x53, 0, 0xa5, 0x51);
            Blit(x1 - 43 + (int)slide, y1, 0, g_winGfx, 0x82, 0x52, 0x2b, 0x47);
            QueueQuad(0, x1 - 320.0 + slide, y1 - 73.0, 230, 31, g_winGfx, 0, 8, 83, 31);
            QueueQuad(0, x1 + slide, (float)y1, (float)W.w, (float)W.h, g_winGfx, 173, 82, 504, 498);
            QueueQuad(0, x1 + 75.0 + slide, y1 - 13.0, W.w - 72.0, 13, g_winGfx, 249, 68, 424, 13);
            QueueQuad(0, x1 + 75.0 + slide, y1 - 36.0, W.w - 100.0, 21, g_winGfx, 249, 45, 417, 21);
            QueueQuad(0, x1 - 15.0 + slide, y1 + 71.0, 15, W.h - 72.0, g_winGfx, 158, 153, 15, 424);
            QueueQuad(0, x1 - 35.0 + slide, y1 + 71.0, 18, W.h - 100.0, g_winGfx, 138, 153, 18, 371);
            QueueQuad(0, x1 - 35.0 + slide, y1 + 71.0 + W.h - 100.0, 18, 35,
                             g_winGfx, 138, 525, 18, 50);
            QueueQuad(0, x1 - 19.0 + slide, (float)y1 + W.h - 4.0, 20, 22,
                             g_winGfx, 154, 576, 20, 22);
            QueueQuad(0, x1 + 1.0 + slide, (float)y1 + W.h, W.w - 3.0, 14,
                             g_winGfx, 174, 580, 500, 14);
            QueueQuad(0, x1 - 3.0 + W.w + slide, (float)y1 + W.h - 3.0, 21, 21,
                             g_winGfx, 675, 577, 21, 21);
            QueueQuad(0, x1 - 3.0 + W.w + slide, y1 - 17.0, 19, 19, g_winGfx, 675, 64, 19, 19);
            QueueQuad(0, x1 - 1.0 + W.w + slide, y1 + 2.0, 15, W.h - 3.0, g_winGfx, 677, 82, 15, 495);

            FlushQuads(0);
            FlushBlit(0);
            FlushStretchF();
            FlushStretchRot();
            FlushStretchI();
            FlushStretchRot2();
            FlushBlit2(0);
        } else {
            for (i = 0; i < (x2 - (x1 + 32)) / 32; i++) {
                for (j = 0; j < (y2 - (y1 + 32)) / 32; j++)
                    Blit(x1 + 32 + i * 32 + (int)slide, y1 + j * 32 + 32, 0, g_gfxLogos,
                              0x80, 0x9e, 0x20, 0x20);
            }
            for (k2 = 0; k2 < (x2 - 16 - (x1 + 16)) / 16; k2++) {
                Blit(x1 + 16 + k2 * 16 + (int)slide, y1, 0, g_gfxLogos, 0x20, 0x60, 0x10, 0x20);
                Blit(x1 + 16 + k2 * 16 + (int)slide, y2 - 32, 0, g_gfxLogos, 0x20, 0x80, 0x10, 0x20);
            }
            for (m2 = 0; m2 < (y2 - 16 - (y1 + 16)) / 16; m2++) {
                Blit(x1 + (int)slide, y1 + m2 * 16 + 16, 0, g_gfxLogos, 0x60, 0x58, 0x20, 0x10);
                Blit(x2 - 32 + (int)slide, y1 + m2 * 16 + 16, 0, g_gfxLogos, 0x60, 0x48, 0x20, 0x10);
            }
            Blit(x1 + (int)slide, y1, 0, g_gfxLogos, 0, 0x60, 0x20, 0x20);
            Blit(x2 - 32 + (int)slide, y1, 0, g_gfxLogos, 0x30, 0x60, 0x20, 0x20);
            Blit(x1 + (int)slide, y2 - 32, 0, g_gfxLogos, 0, 0x80, 0x20, 0x20);
            Blit(x2 - 32 + (int)slide, y2 - 32, 0, g_gfxLogos, 0x30, 0x80, 0x20, 0x20);

            // ---- scanline sweep highlight ----
            g_scanY += g_frameDt * 4.0;
            if (g_scanY > g_screenH)
                g_scanY = 0;
            if (g_scanY >= y1 + 14 && g_scanY <= y2 - 14) {
                for (i = 0; i < (x2 - (x1 + 32)) / 32; i++)
                    Blit(x1 + 32 + i * 32 + (int)slide, (int)g_scanY, 0, g_gfxLogos,
                              0x70, 0xea, 0x20, 6);
                Blit(x1 + (int)slide, (int)g_scanY, 0, g_gfxLogos, 0x70, 0xe4, 0x20, 6);
                Blit(x2 - 32 + (int)slide, (int)g_scanY, 0, g_gfxLogos, 0x70, 0xde, 0x20, 6);
            }
        }
    }

    // ---- text list E ----
    if (W.nE > -1) {
        for (e = 0; e < W.nE + 1; e++) {
            i = W.texts[e].x;
            if (i == POS_CENTERED)
                i = (W.w >> 1) - (StrLenPlat(W.texts[e].text) * 8 >> 1);
            i = i + x1;
            if (W.texts[e].color <= 4)
                DrawTinyText(W.texts[e].text, i + (int)slide, y1 + W.texts[e].y, W.texts[e].color);
            else
                DrawNewsText(W.texts[e].text, i + (int)slide, y1 + W.texts[e].y, W.texts[e].color);
        }
    }

    // ---- hilite-able text pairs, list D ----
    if (W.nD > -1) {
        for (d = 0; d < W.nD + 1; d++) {
            i = W.links[d].x;
            if (i == POS_CENTERED)
                i = (W.w >> 1) - (StrLenPlat(W.links[d].text1) * 8 >> 1);
            i = i + x1;
            if (W.links[d].color <= 4) {
                if (W.links[d].hilite)
                    DrawTinyText(W.links[d].text1, i + (int)slide, y1 + W.links[d].y, W.links[d].hoverColor);
                else
                    DrawTinyText(W.links[d].text1, i + (int)slide, y1 + W.links[d].y, W.links[d].color);
            } else {
                if (W.links[d].hilite)
                    DrawNewsText(W.links[d].text1, i + (int)slide, y1 + W.links[d].y, W.links[d].color + 4);
                else
                    DrawNewsText(W.links[d].text1, i + (int)slide, y1 + W.links[d].y, W.links[d].color);
            }
        }
    }

    // ---- menu items, list F ----
    if (W.nF > -1) {
        anyChecked = false;
        for (f = 0; f < W.nF + 1; f++) {
            if (W.menuItems[f].checked)
                anyChecked = true;
        }
        for (n = 0; n < W.nF + 1; n++) {
            if (W.menuItems[n].param == 1) {
                v = 0;
                tf = 4;
                if ((W.menuItems[n].state != 0 && !anyChecked) || W.menuItems[n].checked)
                    t = 0x10d;
                else
                    t = 0xfe;
            }

            if (W.menuItems[n].param == 2) {
                v = 0x18;
                tf = 3;
                if ((W.menuItems[n].state != 0 && !anyChecked) || W.menuItems[n].checked)
                    t = 0x10d;
                else
                    t = 0xfe;
            }

            if (W.menuItems[n].param == 3) {
                v = 0x30;
                tf = 2;
                if ((W.menuItems[n].state != 0 && !anyChecked) || W.menuItems[n].checked)
                    t = 0x10d;
                else
                    t = 0xfe;
            }

            if (W.menuItems[n].param == 5) {
                v = 0x60;
                tf = 8;
                if ((W.menuItems[n].state != 0 && !anyChecked) || W.menuItems[n].checked) {
                    t = 0x87;
                    tf = 0xc;
                } else
                    t = 0x78;
            }

            if (W.menuItems[n].param == 6) {
                v = 0x40;
                tf = 7;
                if ((W.menuItems[n].state != 0 && !anyChecked) || W.menuItems[n].checked) {
                    t = 0x48;
                    tf = 0xb;
                } else
                    t = 0x39;
            }

            if (W.menuItems[n].param == 7) {
                v = 0x60;
                tf = 6;
                if ((W.menuItems[n].state != 0 && !anyChecked) || W.menuItems[n].checked) {
                    t = 0xa5;
                    tf = 0xa;
                } else
                    t = 0x96;
            }

            // ---- draw the item box, its 8px-tile label background, and its text ----
            i = W.menuItems[n].x;
            if (i == POS_CENTERED) {
                i = (W.w >> 1) - ((StrLenPlat(W.menuItems[n].text) * 8 + 16) >> 1);
                W.menuItems[n].hilite = 1;
                W.menuItems[n].x = i;
            }
            Blit(x1 + i + (int)slide, y1 + W.menuItems[n].y, 0, g_gfxLogos, v, t, 8, 15);
            for (q = 0; q < W.menuItems[n].len; q++)
                Blit(x1 + i + 8 + q * 8 + (int)slide, y1 + W.menuItems[n].y, 0, g_gfxLogos,
                          v + 8, t, 8, 15);
            Blit(x1 + i + 8 + W.menuItems[n].len * 8 + (int)slide, y1 + W.menuItems[n].y,
                      0, g_gfxLogos, v + 16, t, 8, 15);
            if (tf <= 4) {
                DrawTinyText(W.menuItems[n].text, x1 + i + (int)slide + 8, y1 + W.menuItems[n].y + 4, tf);
            } else {
                tf = 9;
                DrawNewsText(W.menuItems[n].text, x1 + i + (int)slide + 8, y1 + W.menuItems[n].y + 4, tf);
            }
        }
    }

    // ---- toggles, list G ----
    if (W.nG > -1) {
        for (g = 0; g < W.nG + 1; g++) {
            if (W.toggles[g].value == 0)
                Blit(x1 + W.toggles[g].x + (int)slide, y1 + W.toggles[g].y, 0, g_gfxLogos,
                          2, 0x4e, 7, 0xb);
            else
                Blit(x1 + W.toggles[g].x + (int)slide, y1 + W.toggles[g].y, 0, g_gfxLogos,
                          10, 0x4e, 8, 0xb);
        }

        // ---- hover tooltip for a toggle whose label doesn't fit on-screen ----
        for (g2 = 0; g2 < W.nG + 1; g2++) {
            if (W.toggles[g2].state != 0 && W.toggles[g2].flag2 != 0) {
                off = 0;
                if (x1 + W.toggles[g2].x + (W.toggles[g2].w >> 1) + StrLenPlat(W.toggles[g2].text) * 4
                        > g_clipRight - 15)
                    off = x1 + W.toggles[g2].x + (W.toggles[g2].w >> 1)
                              + StrLenPlat(W.toggles[g2].text) * 4 - (g_clipRight - 15);
                if (x1 + W.toggles[g2].x + (W.toggles[g2].w >> 1) - StrLenPlat(W.toggles[g2].text) * 4
                        < g_clipLeft + 15)
                    off = x1 + W.toggles[g2].x + (W.toggles[g2].w >> 1)
                              - StrLenPlat(W.toggles[g2].text) * 4 - (g_clipLeft + 15);
                DrawTextFrame(W.toggles[g2].text,
                              x1 + W.toggles[g2].x + (W.toggles[g2].w >> 1)
                                  - StrLenPlat(W.toggles[g2].text) * 4 - off + (int)slide,
                              y1 + W.toggles[g2].y - 12, 1);
                DrawNewsText(W.toggles[g2].text,
                              x1 + W.toggles[g2].x + (W.toggles[g2].w >> 1)
                                  - StrLenPlat(W.toggles[g2].text) * 4 - off + (int)slide,
                              y1 + W.toggles[g2].y - 12, 5);
            }
        }
    }

    // ---- edit fields, list H ----
    if (W.nH > -1) {
        for (h = 0; h < W.nH + 1; h++) {
            int visibleLen = W.edits[h].len;
            int displayCursor = W.edits[h].cursor;
            char *displayText = W.edits[h].buf;
#ifdef __EMSCRIPTEN__
            // Credential limits exceed their visual width. Keep the insertion
            // point visible while retaining the complete text in the edit buffer.
            int capacity = (W.w - W.edits[h].x - 24) / 8;
            if (capacity < 1) capacity = 1;
            if (visibleLen > capacity) visibleLen = capacity;
            int offset = displayCursor >= visibleLen ? displayCursor - visibleLen + 1 : 0;
            displayCursor -= offset;
            char visibleText[259];
            snprintf(visibleText, sizeof(visibleText), "%.*s", visibleLen, W.edits[h].buf + offset);
            displayText = visibleText;
#endif
            for (c = displayCursor; c < visibleLen; c++) {
                if (W.edits[h].param <= 4)
                    DrawTinyText("_", x1 + W.edits[h].x + c * 8 + (int)slide, y1 + W.edits[h].y,
                                    W.edits[h].param);
                else
                    DrawNewsText("_", x1 + W.edits[h].x + c * 8 + (int)slide, y1 + W.edits[h].y,
                                    W.edits[h].param);
            }

            if (W.edits[h].masked) {
                for (z = 0; z < StrLenPlat(displayText); z++) {
                    if (W.edits[h].param <= 4)
                        DrawTinyText("*", x1 + W.edits[h].x + z * 8 + (int)slide, y1 + W.edits[h].y,
                                        W.edits[h].param);
                    else
                        DrawNewsText("*", x1 + W.edits[h].x + z * 8 + (int)slide, y1 + W.edits[h].y,
                                        W.edits[h].param);
                }
            } else {
                if (W.edits[h].param <= 4)
                    DrawTinyText(displayText, x1 + W.edits[h].x + (int)slide, y1 + W.edits[h].y,
                                    W.edits[h].param);
                else
                    DrawNewsText(displayText, x1 + W.edits[h].x + (int)slide, y1 + W.edits[h].y,
                                    W.edits[h].param);
            }

            if (W.blinkOn && W.firstH != -1 && W.edits[h].focused) {
                if (W.edits[h].param <= 4)
                    DrawTinyText("#", x1 + W.edits[h].x + displayCursor * 8 + (int)slide,
                                    y1 + W.edits[h].y, W.edits[h].param);
                else
                    DrawNewsText("#", x1 + W.edits[h].x + displayCursor * 8 + (int)slide,
                                    y1 + W.edits[h].y, W.edits[h].param);
            }
        }
    }
    W.blinkTimer -= 1.0;
    if (W.blinkTimer < 0.0) {
        W.blinkTimer = 25.0f;
        W.blinkOn = !W.blinkOn;
    }

    // ---- image rects, list C ----
    if (W.nC > -1) {
        for (ci = 0; ci < W.nC + 1; ci++) {
            i = W.imageRects[ci].x;
            if (i == POS_CENTERED)
                i = (W.w >> 1) - (W.imageRects[ci].w >> 1);
            i = i + x1;
            BlitLocal(i + (int)slide, y1 + W.imageRects[ci].y, 0, (Image *)W.imageRects[ci].graphic,
                            0, 0, W.imageRects[ci].w, W.imageRects[ci].h);
        }
    }

    // ---- blit-quad items, list A ----
    if (W.nA > -1) {
        for (a = 0; a < W.nA + 1; a++)
            QueueQuad(0,
                             (float)(W.quadImages[a].destX + x1 + (int)slide),
                             (float)(W.quadImages[a].destY + y1),
                             (float)W.quadImages[a].destW, (float)W.quadImages[a].destH,
                             (Image *)W.quadImages[a].graphic,
                             (float)W.quadImages[a].srcX, (float)W.quadImages[a].srcY,
                             (float)W.quadImages[a].srcW, (float)W.quadImages[a].srcH);
    }

    // ---- icon items with hover tooltip, list B ----
    if (W.nB > -1) {
        for (b = 0; b < W.nB + 1; b++) {
            if (g_gfxMedals != 0)
                Blit(x1 + W.buttons[b].x + (int)slide, y1 + W.buttons[b].y, 0, g_gfxMedals,
                          W.buttons[b].srcX, W.buttons[b].srcY, W.buttons[b].w, W.buttons[b].h);
            if (W.buttons[b].hover) {
                DrawTextFrame(W.buttons[b].text,
                              x1 + W.buttons[b].x + (W.buttons[b].w >> 1) - StrLenPlat(W.buttons[b].text) * 4
                                  + (int)slide,
                              y1 + W.buttons[b].y - 12, 1);
                DrawNewsText(W.buttons[b].text,
                             x1 + W.buttons[b].x + (W.buttons[b].w >> 1) - StrLenPlat(W.buttons[b].text) * 4
                                 + (int)slide,
                             y1 + W.buttons[b].y - 12, 5);
            }
        }
    }

    // ---- flush all queued draw batches ----
    FlushQuads(0);
    FlushBlit(0);
    FlushStretchF();
    FlushStretchRot();
    FlushStretchI();
    FlushStretchRot2();
    FlushBlit2(0);
}

#undef W

// Redraws every active window (g_windows[0..9]).
void WinUpdateAll()
{
    int i;
    for (i = 0; i < 10; i++) {
        if (g_windows[i].active != 0) {
            WinDraw(i);
        }
    }
}

// Draws a tiled window frame with corner/edge pieces from g_gfxLogos across
// (x1,y1)-(x2,y2), then centers title in the top border.
void DrawFrame(int x1, int y1, int x2, int y2, char *title)
{
    char *p;
    int i;
    int j;
    int k;
    int m;
    int len;
    for (i = 0; i < (x2 - 16 - (x1 + 16)) / 16; i++) {
        for (j = 0; j < (y2 - 16 - (y1 + 16)) / 16; j++)
            Blit(x1 + i * 16 + 16, y1 + j * 16 + 16, 0, g_gfxLogos, 0x60, 0x68, 0x10, 0x10);
    }
    for (k = 0; k < (x2 - 16 - (x1 + 16)) / 16; k++) {
        Blit(x1 + k * 16 + 16, y1, 0, g_gfxLogos, 0x20, 0x60, 0x10, 0x20);
        Blit(x1 + k * 16 + 16, y2 - 32, 0, g_gfxLogos, 0x20, 0x80, 0x10, 0x20);
    }
    for (m = 0; m < (y2 - 16 - (y1 + 16)) / 16; m++) {
        Blit(x1, y1 + m * 16 + 16, 0, g_gfxLogos, 0x60, 0x58, 0x20, 0x10);
        Blit(x2 - 32, y1 + m * 16 + 16, 0, g_gfxLogos, 0x60, 0x48, 0x20, 0x10);
    }
    Blit(x1, y1, 0, g_gfxLogos, 0, 0x60, 0x20, 0x20);
    Blit(x2 - 32, y1, 0, g_gfxLogos, 0x30, 0x60, 0x20, 0x20);
    Blit(x1, y2 - 32, 0, g_gfxLogos, 0, 0x80, 0x20, 0x20);
    Blit(x2 - 32, y2 - 32, 0, g_gfxLogos, 0x30, 0x80, 0x20, 0x20);

    len = 0;
    for (p = title; *p; p++)
        len++;
    DrawTinyText(title, (x2 - x1) / 2 + x1 - len * 8 / 2, y1 + 20, 1);
}

// Draws a framed box like DrawFrame, then word-wraps text inside it (breaking at the
// last space before the line would overflow, or at '\n'), drawing one tiny-text line
// per wrapped row.
void DrawTextBox(int x1, int y1, int x2, int y2, char *text)
{
    char *p = text;
    char tbuf[256];
    int i;
    int j;
    int k;
    int m;
    int maxch;
    int n;
    int yoff;
    int lastsp;
    int unused;

    for (i = 0; i < (x2 - 16 - (x1 + 16)) / 16; i++) {
        for (j = 0; j < (y2 - 16 - (y1 + 16)) / 16; j++)
            Blit(x1 + i * 16 + 16, y1 + j * 16 + 16, 0, g_gfxLogos, 0x60, 0x68, 0x10, 0x10);
    }
    for (k = 0; k < (x2 - 16 - (x1 + 16)) / 16; k++) {
        Blit(x1 + k * 16 + 16, y1, 0, g_gfxLogos, 0x20, 0x60, 0x10, 0x20);
        Blit(x1 + k * 16 + 16, y2 - 32, 0, g_gfxLogos, 0x20, 0x80, 0x10, 0x20);
    }
    for (m = 0; m < (y2 - 16 - (y1 + 16)) / 16; m++) {
        Blit(x1, y1 + m * 16 + 16, 0, g_gfxLogos, 0x60, 0x58, 0x20, 0x10);
        Blit(x2 - 32, y1 + m * 16 + 16, 0, g_gfxLogos, 0x60, 0x48, 0x20, 0x10);
    }
    Blit(x1, y1, 0, g_gfxLogos, 0, 0x60, 0x20, 0x20);
    Blit(x2 - 32, y1, 0, g_gfxLogos, 0x30, 0x60, 0x20, 0x20);
    Blit(x1, y2 - 32, 0, g_gfxLogos, 0, 0x80, 0x20, 0x20);
    Blit(x2 - 32, y2 - 32, 0, g_gfxLogos, 0x30, 0x80, 0x20, 0x20);

    maxch = (x2 - 20 - (x1 + 20)) / 8;
    n = 0;
    yoff = 0;
    lastsp = 0;
    unused = 0;

    for (p = text; *p; p++) {
        tbuf[n] = *p;
        if (tbuf[n] == ' ')
            lastsp = n;
        if (tbuf[n] == '\n') {
            tbuf[n] = 0;
            DrawTinyText(tbuf, x1 + 20, y1 + yoff + 20, 4);
            n = -1;
            yoff += 9;
        }
        n++;
        if (n == maxch) {
            tbuf[lastsp] = 0;
            DrawTinyText(tbuf, x1 + 20, y1 + yoff + 20, 4);
            p -= n - lastsp - 1;
            n = 0;
            yoff += 9;
        }
    }

    if (n > 0) {
        tbuf[n] = 0;
        DrawTinyText(tbuf, x1 + 20, y1 + yoff + 20, 4);
        n = 0;
        yoff += 9;
    }
}

// Draws a horizontal frame of 8x11 border tiles from g_gfxLogos wide enough to sit
// behind `text` (its length, minus link escapes, plus one tile of padding on each end).
// `style` is accepted but unused. `x`/`y` are the text's own top-left; the frame is
// offset up/left by (8, 2) to surround it.
void DrawTextFrame(const char* text, int x, int y, int style)
{
    const char *p;
    Rect16 src;
    int i;
    int unused;
    int count;
    int dx;
    int dy;
    int w;
    int h;

    if (y == g_textAutoY)
        y = g_textCursorY;

    // Count drawable glyphs (link escapes don't take a tile of frame).
    unused = 0;
    count = 0;
    for (p = text; *p != 0; p++) {
        if (*p == '@') {
            switch (p[1]) {
            case '@':
                p++;
                break;
            case 'S':
                p += 4;
                break;
            case 'E':
                p++;
                break;
            }
        } else {
            count++;
        }
    }
    x -= 8;
    y -= 2;

    for (i = 0; i < count + 2; i++) {
        // Middle tile at x=8 (row y=0xaf in g_gfxLogos); first tile (i==0) is the left cap
        // at x=0, last tile is the right cap at x=16.
        src.x1 = 8;
        src.y1 = 0xaf;
        src.y2 = src.y1 + 8;
        src.x2 = src.x1 + 8;
        if (i == 0) {
            src.x1 = 0;
            src.y1 = 0xaf;
            src.y2 = src.y1 + 8;
            src.x2 = src.x1 + 8;
        }
        if (i == count + 1) {
            src.x1 = 16;
            src.y1 = 0xaf;
            src.y2 = src.y1 + 8;
            src.x2 = src.x1 + 8;
        }
        dx = x;
        dy = y;
        w = 8;
        h = 11;

        if (CLIP_VISIBLE(dx, dy, w, h)) {
            CLIP_SRC_RECT(dx, dy, w, h, src)
            QueueBlit((float)dx, (float)dy, g_gfxLogos, &src);
        }
        x += 8;
    }
}
