// text.c: Bitmap fonts and text drawing, digit strips, blinking prompts.
#include "globals.h"
#include "game.h"

// g_smallFont glyph cell width (DrawMenuText) and g_tinyFont glyph cell width
// (DrawTinyText/DrawTinyText2/DrawTinyTextAlt/DrawNumberRow): also the fixed per-glyph
// advance, so a space just advances by the same amount without drawing.
// Uppercase-only bitmap rows display lowercase with the matching capital glyph.
// Map at draw time so case-sensitive credentials and names remain unchanged.
enum { MENU_GLYPH_W = 12, TINY_GLYPH_W = 8 };


// Draws a blinking "PLEASE WAIT..." message.
void DrawPleaseWait()
{
    if (g_time - g_uiBlinkTime > g_blinkRate) {
        g_uiBlinkTime = g_time;
        g_uiBlink = g_uiBlink == 0;
    }
    if (g_uiBlink != 0)
        DrawMenuText("PLEASE WAIT...", POS_CENTERED, g_screenH - 0x37, 5);
}

// Draws a 12x9 bitmap-font string from g_smallFont (menu text). `x == POS_CENTERED` centers on
// screen; `y == g_textAutoY` continues below the last drawn text. `row` selects which
// font row (color/style) to use. `@@` resets the hyperlink list, `@Snnn` opens a link
// with 3-digit id `nnn` at the current position, `@E` closes it (see g_links); these
// escapes are consumed without being drawn. '\n' starts a new line.
void DrawMenuText(const char *text, int x, int y, int row)
{
    const char *p;
    Rect16 src;
    int glyph;
    int width;
    int advance;
    int dx;
    int dy;
    int w;
    int h;
    float fx;
    float fy;

    fx = (float)x;
    fy = (float)y;
    if (y == g_textAutoY)
        fy = (float)g_textCursorY;

    // Centering: measure the string width (accounting for @-escapes), then start there.
    width = 0;
    if (x == POS_CENTERED) {
        for (p = text; *p != 0; p++) {
            advance = 0;

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
                advance = MENU_GLYPH_W;
                if (*p == '.')
                    advance = MENU_GLYPH_W;
                if (*p == ',')
                    advance = MENU_GLYPH_W;
            }
            width += advance;
        }

        fx = (float)((g_screenW >> 1) - width / 2);
    }
    g_textStartX = (int)fx;

    // Draw pass: same @-escape handling, plus recording each link's on-screen box.
    for (p = text; *p != 0; p++) {
        if (*p == '@') {
            switch (p[1]) {
            case '@':
                g_linkCount = 0;
                p++;
                break;
            case 'S':
                g_links[g_linkCount].id = (p[2] - '0') * 100 + (p[3] - '0') * 10 + p[4] - '0';
                g_links[g_linkCount].x1 = (int)fx;
                g_links[g_linkCount].y1 = (int)fy;
                p += 4;
                break;
            case 'E':
                g_links[g_linkCount].x2 = (int)fx;
                g_links[g_linkCount].y2 = (int)fy + 12;
                g_linkCount++;
                p++;
                break;
            }

        } else {
            while (1) {
                glyph = 0;
                advance = MENU_GLYPH_W;
                if (*p >= '0' && *p <= '9')
                    glyph = *p - '0';
                if (*p >= 'A' && *p <= 'Z')
                    glyph = *p - '7';
                if (*p >= 'a' && *p <= 'z')
                    glyph = *p - 'a' + 10;
                if (*p == '.') {
                    glyph = 36;
                    advance = MENU_GLYPH_W;
                }
                if (*p == ',') {
                    glyph = 37;
                    advance = MENU_GLYPH_W;
                }

                if (*p == ':')
                    glyph = 41;
                if (*p == '?')
                    glyph = 39;
                if (*p == '!')
                    glyph = 42;
                if (*p == '*')
                    glyph = 43;
                if (*p == '=')
                    glyph = 40;
                if (*p == '$')
                    glyph = 38;

                if (*p == '+')
                    glyph = 44;
                if (*p == '-')
                    glyph = 39;
                if (*p == '/')
                    glyph = 45;
                if (*p == '#')
                    glyph = 46;
                if (*p == '_')
                    glyph = 47;
                if (*p == '~')
                    glyph = 48;

                if (*p == 0xC2A7)
                    glyph = 49;
                if (*p == '<')
                    glyph = 50;
                if (*p == '>')
                    glyph = 51;

                if (*p == '\n') {
                    fx = (float)x;
                    fy = fy + 13.0;
                    fx = fx - advance;
                    break;
                } else if (*p != ' ') {
                    src.x1 = glyph * MENU_GLYPH_W;
                    src.y1 = row * 9;
                    src.x2 = src.x1 + MENU_GLYPH_W;
                    src.y2 = src.y1 + 9;
                    dx = (int)fx;
                    dy = (int)fy;
                    w = MENU_GLYPH_W;
                    h = 9;

                    if (!(dx < g_clipRight && dy < g_clipBottom &&
                          dx + w > g_clipLeft && dy + h > g_clipTop))
                        break;

                    if (dx < g_clipLeft) {
                        src.x1 = g_clipLeft - dx + src.x1;
                        w = w - (g_clipLeft - dx);
                        dx = g_clipLeft;
                    } else if (dx + w >= g_clipRight) {
                        w = g_clipRight - dx;
                    }

                    if (dy < g_clipTop) {
                        src.y1 = g_clipTop - dy + src.y1;
                        h = h - (g_clipTop - dy);
                        dy = g_clipTop;
                    } else if (dy + h >= g_clipBottom) {
                        h = g_clipBottom - dy;
                    }
                    src.x2 = src.x1 + w;
                    src.y2 = src.y1 + h;
                    QueueBlit((float)dx, (float)dy, g_smallFont, &src);
                    break;
                } else {
                    break;
                }
            }
            fx += advance;
            g_cursorX = (int)fx;
        }
    }
    g_curY = (int)fy;
    g_textCursorY = (int)fy + MENU_GLYPH_W;
}

// Draws a string of digits (and ':') from the 32x24 g_digitFont, used for the score and
// clock. `x == POS_CENTERED` centers on screen; `y == g_textAutoY` continues below the last
// drawn text. `a` and `scale` are accepted but unused. Spaces are skipped (no blit).
void DrawScoreDigits(const char* text, int x, int y, int a, float scale)
{
    const char *p;
    int digit;
    int width;
    int charW;
    int charH;
    int cx;
    int cy;

    cx = x;
    cy = y;
    charH = 24;
    charW = 32;
    if (y == g_textAutoY)
        y = g_textCursorY;

    // Centering: measure the string width first, then start drawing from its left edge.
    width = 0;
    if (x == POS_CENTERED) {
        for (p = text; *p != 0; p++)
            width += charW;
        cx = (g_screenW >> 1) - (width >> 1);
    }
    g_textStartX = cx;

    for (p = text; *p != 0; p++) {
        digit = 0;
        if (*p >= '0' && *p <= '9')
            digit = *p - '0';
        if (*p == ':')
            digit = 10;
        if (*p != ' ')
            Blit(cx, cy, 0, g_digitFont, digit << 5, 0, 32, 24);
        cx += 32;
        g_cursorX = cx;
    }
    g_curY = cy;
    g_textCursorY = cy + 24;
}

// Draws an 8x8 bitmap-font string from g_tinyFont. `x == POS_CENTERED` centers on screen;
// `y == g_textAutoY` continues below the last drawn text. `row` (1-4) selects which font
// row to use (src.y1 = 13/21/29/37).
// NOTE: `src` is a local Rect16 that is never fully initialized; for `row` outside 1-4,
// src.y1 is left as whatever garbage was on the stack. Kept as-is for the byte match.
// Supports the same `@@`/`@Snnn`/`@E` link escapes as DrawMenuText(); '\n' starts a new line.
void DrawTinyText(const char *text, int x, int y, int row)
{
    const char *p;
    Rect16 src;
    int glyph;
    int width;
    int dx;
    int dy;
    int w;
    int h;
    int advance;
    float fx;
    float fy;

    fx = (float)x;
    fy = (float)y;
    if (y == g_textAutoY)
        fy = (float)g_textCursorY;

    // Centering: measure the string width (accounting for @-escapes), then start there.
    width = 0;
    if (x == POS_CENTERED) {
        for (p = text; *p != 0; p++) {
            advance = 0;

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
                advance = TINY_GLYPH_W;
                if (*p == '.')
                    advance = TINY_GLYPH_W;
                if (*p == ',')
                    advance = TINY_GLYPH_W;
            }
            width += advance;
        }

        fx = (float)((g_screenW >> 1) - width / 2);
    }
    g_textStartX = (int)fx;

    // Draw pass: same @-escape handling, plus recording each link's on-screen box.
    for (p = text; *p != 0; p++) {
        if (*p == '@') {
            switch (p[1]) {
            case '@':
                g_linkCount = 0;
                p++;
                break;
            case 'S':
                g_links[g_linkCount].id = (p[2] - '0') * 100 + (p[3] - '0') * 10 + p[4] - '0';
                g_links[g_linkCount].x1 = (int)fx;
                g_links[g_linkCount].y1 = (int)fy;
                p += 4;
                break;
            case 'E':
                g_links[g_linkCount].x2 = (int)fx;
                g_links[g_linkCount].y2 = (int)fy + 8;
                g_linkCount++;
                p++;
                break;
            }

        } else {
            while (1) {
                glyph = 0;
                if (*p >= '0' && *p <= '9')
                    glyph = *p - 22;
                if (*p >= 'A' && *p <= 'Z')
                    glyph = *p - 'A';
                if (*p >= 'a' && *p <= 'z')
                    glyph = *p - 'a';
                advance = TINY_GLYPH_W;
                if (*p == '.') {
                    glyph = 36;
                    advance = TINY_GLYPH_W;
                }
                if (*p == ',') {
                    glyph = 37;
                    advance = TINY_GLYPH_W;
                }

                if (*p == ':')
                    glyph = 38;
                if (*p == '?')
                    glyph = 39;
                if (*p == '!')
                    glyph = 40;
                if (*p == '*')
                    glyph = 41;
                if (*p == '=')
                    glyph = 42;
                if (*p == '$')
                    glyph = 43;

                if (*p == 0xC2A3)
                    glyph = 44;
                if (*p == '-')
                    glyph = 45;
                if (*p == '+')
                    glyph = 47;
                if (*p == '<')
                    glyph = 48;
                if (*p == '>')
                    glyph = 49;
                if (*p == '_')
                    glyph = 50;
                if (*p == '#')
                    glyph = 51;
                if (*p == '%')
                    glyph = 52;

                if (*p == '\n') {
                    fx = (float)x;
                    fy = fy + 11.0;
                    fx = fx - advance;
                    break;
                } else if (*p != ' ') {
                    if (row == 1)
                        src.y1 = 13;
                    if (row == 2)
                        src.y1 = 21;
                    if (row == 3)
                        src.y1 = 29;
                    if (row == 4)
                        src.y1 = 37;
                    src.x1 = glyph << 3;
                    src.x2 = src.x1 + 8;
                    src.y2 = src.y1 + 8;
                    dx = (int)fx;
                    dy = (int)fy;
                    w = advance;
                    h = 8;

                    if (!(dx < g_clipRight && dy < g_clipBottom &&
                          dx + w > g_clipLeft && dy + h > g_clipTop))
                        break;

                    if (dx < g_clipLeft) {
                        src.x1 = g_clipLeft - dx + src.x1;
                        w = w - (g_clipLeft - dx);
                        dx = g_clipLeft;
                    } else if (dx + w >= g_clipRight) {
                        w = g_clipRight - dx;
                    }

                    if (dy < g_clipTop) {
                        src.y1 = g_clipTop - dy + src.y1;
                        h = h - (g_clipTop - dy);
                        dy = g_clipTop;
                    } else if (dy + h >= g_clipBottom) {
                        h = g_clipBottom - dy;
                    }
                    src.x2 = src.x1 + w;
                    src.y2 = src.y1 + h;
                    QueueBlit((float)dx, (float)dy, g_tinyFont, &src);
                    break;
                } else {
                    break;
                }
            }
            fx += advance;
            g_cursorX = (int)fx;
        }
    }
    g_curY = (int)fy;
    g_textCursorY = (int)fy + 8;
}

// Identical to DrawTinyText(), but queues the glyph blit with QueueBlit2() instead of
// QueueBlit() (a separate draw list/layer).
void DrawTinyText2(const char *text, int x, int y, int row)
{
    const char *p;
    Rect16 src;
    int glyph;
    int width;
    int dx;
    int dy;
    int w;
    int h;
    int advance;
    float fx;
    float fy;

    fx = (float)x;
    fy = (float)y;
    if (y == g_textAutoY)
        fy = (float)g_textCursorY;

    // Centering: measure the string width (accounting for @-escapes), then start there.
    width = 0;
    if (x == POS_CENTERED) {
        for (p = text; *p != 0; p++) {
            advance = 0;

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
                advance = TINY_GLYPH_W;
                if (*p == '.')
                    advance = TINY_GLYPH_W;
                if (*p == ',')
                    advance = TINY_GLYPH_W;
            }
            width += advance;
        }

        fx = (float)((g_screenW >> 1) - width / 2);
    }
    g_textStartX = (int)fx;

    // Draw pass: same @-escape handling, plus recording each link's on-screen box.
    for (p = text; *p != 0; p++) {
        if (*p == '@') {
            switch (p[1]) {
            case '@':
                g_linkCount = 0;
                p++;
                break;
            case 'S':
                g_links[g_linkCount].id = (p[2] - '0') * 100 + (p[3] - '0') * 10 + p[4] - '0';
                g_links[g_linkCount].x1 = (int)fx;
                g_links[g_linkCount].y1 = (int)fy;
                p += 4;
                break;
            case 'E':
                g_links[g_linkCount].x2 = (int)fx;
                g_links[g_linkCount].y2 = (int)fy + 8;
                g_linkCount++;
                p++;
                break;
            }

        } else {
            while (1) {
                glyph = 0;
                if (*p >= '0' && *p <= '9')
                    glyph = *p - 22;
                if (*p >= 'A' && *p <= 'Z')
                    glyph = *p - 'A';
                if (*p >= 'a' && *p <= 'z')
                    glyph = *p - 'a';
                advance = TINY_GLYPH_W;
                if (*p == '.') {
                    glyph = 36;
                    advance = TINY_GLYPH_W;
                }
                if (*p == ',') {
                    glyph = 37;
                    advance = TINY_GLYPH_W;
                }

                if (*p == ':')
                    glyph = 38;
                if (*p == '?')
                    glyph = 39;
                if (*p == '!')
                    glyph = 40;
                if (*p == '*')
                    glyph = 41;
                if (*p == '=')
                    glyph = 42;
                if (*p == '$')
                    glyph = 43;

                if (*p == 0xC2A3)
                    glyph = 44;
                if (*p == '-')
                    glyph = 45;
                if (*p == '+')
                    glyph = 47;
                if (*p == '<')
                    glyph = 48;
                if (*p == '>')
                    glyph = 49;
                if (*p == '_')
                    glyph = 50;
                if (*p == '#')
                    glyph = 51;
                if (*p == '%')
                    glyph = 52;

                if (*p == '\n') {
                    fx = (float)x;
                    fy = fy + 11.0;
                    fx = fx - advance;
                    break;
                } else if (*p != ' ') {
                    if (row == 1)
                        src.y1 = 13;
                    if (row == 2)
                        src.y1 = 21;
                    if (row == 3)
                        src.y1 = 29;
                    if (row == 4)
                        src.y1 = 37;
                    src.x1 = glyph << 3;
                    src.x2 = src.x1 + 8;
                    src.y2 = src.y1 + 8;
                    dx = (int)fx;
                    dy = (int)fy;
                    w = advance;
                    h = 8;

                    if (!(dx < g_clipRight && dy < g_clipBottom &&
                          dx + w > g_clipLeft && dy + h > g_clipTop))
                        break;

                    if (dx < g_clipLeft) {
                        src.x1 = g_clipLeft - dx + src.x1;
                        w = w - (g_clipLeft - dx);
                        dx = g_clipLeft;
                    } else if (dx + w >= g_clipRight) {
                        w = g_clipRight - dx;
                    }

                    if (dy < g_clipTop) {
                        src.y1 = g_clipTop - dy + src.y1;
                        h = h - (g_clipTop - dy);
                        dy = g_clipTop;
                    } else if (dy + h >= g_clipBottom) {
                        h = g_clipBottom - dy;
                    }
                    src.x2 = src.x1 + w;
                    src.y2 = src.y1 + h;
                    QueueBlit2((float)dx, (float)dy, g_tinyFont, &src);
                    break;
                } else {
                    break;
                }
            }
            fx += advance;
            g_cursorX = (int)fx;
        }
    }
    g_curY = (int)fy;
    g_textCursorY = (int)fy + 8;
}

// Draws mixed-case text from a separate glyph strip (row fixed at y=78, or 86 when
// `flags == 1`); '\n' or '|' starts a new line. Advance widths come from
// g_smallFontWidths[idx], looked up per glyph (spaces use a fixed 3px advance).
void DrawMixedCaseText(const char *text, int x, int y, int flags)
{
    const char *p;
    Rect16 src;
    int idx;
    int total;
    int dx;
    int dy;
    int w;
    int h;
    int width;
    float fx;
    float fy;

    fx = (float)x;
    fy = (float)y;
    if (y == g_textAutoY) {
        fy = (float)g_textCursorY;
    }

    total = 0;
    g_textStartX = (int)fx;
    for (p = text; *p != 0; p++) {
        while (1) {
            idx = 0;

            if (*p >= '0' && *p <= '9') {
                idx = *p + 4;
            }
            if (*p >= 'A' && *p <= 'Z') {
                idx = *p - 65;
            }
            if (*p >= 'a' && *p <= 'z') {
                idx = *p - 71;
            }

            if (*p == '.') {
                idx = 62;
            }
            if (*p == ',') {
                idx = 63;
            }
            if (*p == '!') {
                idx = 64;
            }
            if (*p == '?') {
                idx = 65;
            }
            if (*p == ':') {
                idx = 66;
            }
            if (*p == ';') {
                idx = 67;
            }

            if (*p == '*') {
                idx = 68;
            }
            if (*p == '-') {
                idx = 69;
            }
            if (*p == '\'') {
                idx = 70;
            }
            if (*p == '(') {
                idx = 71;
            }
            if (*p == ')') {
                idx = 72;
            }
            if (*p == '%') {
                idx = 73;
            }

            width = g_smallFontWidths[idx];
            if (*p == '\n' || *p == '|') {
                fx = (float)x;
                fy += 8.0;
                fx -= width;
                break;
            } else if (*p != ' ') {
                src.y1 = 78;
                if (flags == 1) {
                    src.y1 = 86;
                }
                src.x1 = idx * 8;
                src.x2 = src.x1 + 8;
                src.y2 = src.y1 + 8;
                dx = (int)fx;
                dy = (int)fy;
                w = width;
                h = 8;

                if (!(dx < g_clipRight && dy < g_clipBottom &&
                      dx + w > g_clipLeft && dy + h > g_clipTop)) {
                    break;
                }

                if (dx < g_clipLeft) {
                    src.x1 = g_clipLeft - dx + src.x1;
                    w = w - (g_clipLeft - dx);
                    dx = g_clipLeft;
                } else if (dx + w >= g_clipRight) {
                    w = g_clipRight - dx;
                }

                if (dy < g_clipTop) {
                    src.y1 = g_clipTop - dy + src.y1;
                    h = h - (g_clipTop - dy);
                    dy = g_clipTop;
                } else if (dy + h >= g_clipBottom) {
                    h = g_clipBottom - dy;
                }
                src.x2 = src.x1 + w;
                src.y2 = src.y1 + h;
                QueueBlit((float)dx, (float)dy, g_tinyFont, &src);
                break;
            } else {
                width = 3;
                break;
            }
        }
        fx += width + 1;
        g_cursorX = (int)fx;
    }
    g_curY = (int)fy;
    g_textCursorY = (int)fy + 8;
}

// Draws an 8x8 bitmap-font string from g_tinyFont using one of 8 glyph rows selected by
// `font` (5-12, src.y1 = 94..150 in steps of 8). `x == POS_CENTERED` centers on screen;
// `y == g_textAutoY` continues below the last drawn text. Supports the same
// `@@`/`@Snnn`/`@E` link escapes as DrawMenuText(); '\n' starts a new line.
// NOTE: `src` is a local Rect16 that is never fully initialized; for `font` outside
// 5-12, src.y1 is left as whatever garbage was on the stack.
void DrawNewsText(char *text, int x, int y, int font)
{
    char *p;
    Rect16 src;
    int idx;
    int total;
    int dx;
    int dy;
    int w;
    int h;
    int width;
    float fx;
    float fy;

    fx = (float)x;
    fy = (float)y;
    if (y == g_textAutoY) {
        fy = (float)g_textCursorY;
    }

    // Centering: measure the string width (accounting for @-escapes), then start there.
    total = 0;
    if (x == POS_CENTERED) {
        for (p = text; *p != 0; p++) {
            width = 0;

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
                width = 8;
                if (*p == '.') {
                    width = 8;
                }
                if (*p == ',') {
                    width = 8;
                }
            }
            total = total + width;
        }

        fx = (float)((g_screenW >> 1) - total / 2);
    }
    g_textStartX = (int)fx;

    // Draw pass: same @-escape handling, plus recording each link's on-screen box.
    for (p = text; *p != 0; p++) {
        if (*p == '@') {
            switch (p[1]) {
            case '@':
                g_linkCount = 0;
                p++;
                break;
            case 'S':
                g_links[g_linkCount].id = (p[2] - '0') * 100 + (p[3] - '0') * 10 + p[4] - '0';
                g_links[g_linkCount].x1 = (int)fx;
                g_links[g_linkCount].y1 = (int)fy;
                p += 4;
                break;
            case 'E':
                g_links[g_linkCount].x2 = (int)fx;
                g_links[g_linkCount].y2 = (int)fy + 8;
                g_linkCount++;
                p++;
                break;
            }

        } else {
            while (1) {
                idx = 0;
                if (*p >= '0' && *p <= '9') {
                    idx = *p - 22;
                }
                if (*p >= 'A' && *p <= 'Z') {
                    idx = *p - 65;
                }
                if (*p >= 'a' && *p <= 'z') {
                    idx = *p - 'a';
                }
                width = 8;
                if (*p == '.') {
                    idx = 36;
                    width = 8;
                }
                if (*p == ',') {
                    idx = 37;
                    width = 8;
                }

                if (*p == ':') {
                    idx = 38;
                }
                if (*p == '?') {
                    idx = 39;
                }
                if (*p == '!') {
                    idx = 40;
                }
                if (*p == '*') {
                    idx = 41;
                }
                if (*p == '=') {
                    idx = 42;
                }
                if (*p == '$') {
                    idx = 43;
                }

                if (*p == 0xC2A3) {
                    idx = 44;
                }
                if (*p == '-') {
                    idx = 45;
                }
                if (*p == '+') {
                    idx = 47;
                }
                if (*p == '<') {
                    idx = 48;
                }
                if (*p == '>') {
                    idx = 49;
                }
                if (*p == '_') {
                    idx = 50;
                }
                if (*p == '#') {
                    idx = 51;
                }
                if (*p == '%') {
                    idx = 52;
                }

                if (*p == '\n') {
                    fx = (float)x;
                    fy += 11.0;
                    fx -= width;
                    break;
                } else if (*p != ' ') {
                    if (font == 5) {
                        src.y1 = 94;
                    }
                    if (font == 6) {
                        src.y1 = 102;
                    }
                    if (font == 7) {
                        src.y1 = 110;
                    }
                    if (font == 8) {
                        src.y1 = 118;
                    }

                    if (font == 9) {
                        src.y1 = 126;
                    }
                    if (font == 10) {
                        src.y1 = 134;
                    }
                    if (font == 11) {
                        src.y1 = 142;
                    }
                    if (font == 12) {
                        src.y1 = 150;
                    }

                    // NOTE: both branches are identical (dead distinction); kept as-is for the byte match.
                    if (font <= 8) {
                        src.x1 = idx * 8;
                    } else {
                        src.x1 = idx * 8;
                    }
                    src.x2 = src.x1 + 8;
                    src.y2 = src.y1 + 8;
                    dx = (int)fx;
                    dy = (int)fy;
                    w = width;
                    h = 8;

                    if (!(dx < g_clipRight && dy < g_clipBottom &&
                          dx + w > g_clipLeft && dy + h > g_clipTop)) {
                        break;
                    }

                    if (dx < g_clipLeft) {
                        src.x1 = g_clipLeft - dx + src.x1;
                        w = w - (g_clipLeft - dx);
                        dx = g_clipLeft;
                    } else if (dx + w >= g_clipRight) {
                        w = g_clipRight - dx;
                    }

                    if (dy < g_clipTop) {
                        src.y1 = g_clipTop - dy + src.y1;
                        h = h - (g_clipTop - dy);
                        dy = g_clipTop;
                    } else if (dy + h >= g_clipBottom) {
                        h = g_clipBottom - dy;
                    }
                    src.x2 = src.x1 + w;
                    src.y2 = src.y1 + h;
                    QueueBlit((float)dx, (float)dy, g_tinyFont, &src);
                    break;
                } else {
                    break;
                }
            }
            fx += width;
            g_cursorX = (int)fx;
        }
    }
    g_curY = (int)fy;
    g_textCursorY = (int)fy + 8;
}

// Draws proportional big-font text from g_fontGfx using the Txstart/Tystart/Twidth/
// Theight/Tkern/Ttop glyph tables below (indexed 0-70: digits, then uppercase, then
// lowercase-height punctuation). Alignment is per run of text up to the next '\n': '|'
// centers it, '<' left-aligns it (x = 0), '>' right-aligns it against g_screenW; passing
// x == -1 starts centered. `y == g_textAutoY` continues below the last drawn text; each
// '\n' advances y by 25 and resets alignment to the value at line start (`g_textStartX`).
void DrawBigText(int x, int y, const char *text)
{
    const char *p;
    const char *q;
    Rect16 src;
    int idx;
    int unused;
    int dx;
    int dy;
    int w;
    int h;
    int adv;
    int total;
    // Glyph metrics for indices 0-70 (0-9 digits, 10-35 'A'-'Z', 36 '!', 37 '?', 38 '.', 39 ',',
    // 40 '-', ..., 67 ';', 68 ':', 69 '(', 70 ')'): source x/y in g_fontGfx, glyph width/height,
    // extra kerning added to the advance, and the y offset from the baseline to the glyph's top.
    int Txstart[71] = {
        0, 16, 32, 48, 64, 80, 96, 128, 144, 160, 176, 192, 208, 224, 240,
        272, 288, 320, 336, 352, 368, 384, 400, 432, 448, 464, 480, 496, 512, 528,
        544, 560, 576, 592, 608, 624, 640, 656, 672, 688, 704, 0, 16, 32, 48,
        64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 256, 272, 288,
        304, 320, 336, 352, 368, 384, 400, 720, 736, 416, 432
    };

    int Tystart[71] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 22, 22, 22,
        22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
        22, 22, 22, 22, 22, 22, 22, 0, 0, 22, 22
    };

    int Twidth[71] = {
        12, 13, 16, 16, 9, 9, 18, 12, 5, 11, 12, 8, 14, 12, 19,
        15, 32, 16, 13, 11, 11, 13, 20, 15, 13, 9, 15, 7, 9, 11,
        11, 10, 11, 10, 11, 11, 2, 9, 3, 2, 7, 10, 12, 11, 13,
        12, 8, 11, 10, 4, 7, 10, 5, 13, 10, 12, 12, 13, 8, 10,
        8, 11, 10, 14, 13, 12, 9, 5, 5, 10, 10
    };

    int Theight[71] = {
        17, 17, 18, 18, 18, 17, 18, 17, 17, 18, 17, 18, 17, 18, 18,
        17, 22, 18, 18, 17, 18, 18, 18, 17, 17, 18, 18, 17, 17, 18,
        17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 11, 19, 19, 19, 19,
        19, 19, 26, 19, 19, 26, 19, 19, 19, 19, 19, 26, 26, 19, 19,
        19, 19, 19, 19, 19, 26, 20, 20, 19, 22, 22
    };

    int Tkern[71] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, -13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    int Ttop[71] = {
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2
    };

    bool center;
    bool left;
    bool right;
    bool centerDone;
    bool leftDone;
    bool rightDone;

    // Maps character `ch` to its glyph index into Txstart/Tystart/Twidth/Theight/Tkern/Ttop
    // above (0 for space, which DrawBigText's callers treat as a fixed 8px advance instead).
    #define BIGTEXT_GLYPH_IDX(ch, idx)      \
        idx = 0;                             \
        if ((ch) >= '0' && (ch) <= '9') {   \
            idx = (ch) - 22;                 \
        }                                     \
        if ((ch) >= 'A' && (ch) <= 'Z') {   \
            idx = (ch) - 65;                 \
        }                                     \
        if ((ch) >= 'a' && (ch) <= 'z') {   \
            idx = (ch) - 56;                 \
        }                                     \
                                              \
        if ((ch) == '!') {                  \
            idx = 36;                        \
        }                                     \
        if ((ch) == '?') {                  \
            idx = 37;                        \
        }                                     \
        if ((ch) == '.') {                  \
            idx = 38;                        \
        }                                     \
        if ((ch) == ',') {                  \
            idx = 39;                        \
        }                                     \
                                              \
        if ((ch) == '-') {                  \
            idx = 40;                        \
        }                                     \
        if ((ch) == ';') {                  \
            idx = 67;                        \
        }                                     \
        if ((ch) == ':') {                  \
            idx = 68;                        \
        }                                     \
        if ((ch) == '(') {                  \
            idx = 69;                        \
        }                                     \
        if ((ch) == ')') {                  \
            idx = 70;                        \
        }

    if (y == g_textAutoY) {
        y = g_textCursorY;
    }

    unused = 0;
    g_textStartX = x;
    center = false;
    left = false;
    right = false;
    centerDone = false;
    leftDone = false;
    rightDone = false;
    if (x == -1) {
        center = true;
    }

    for (p = text; *p != 0; p++) {
        // '|'/'>'/'<' switch the current line's alignment for the rest of the string.
        if (*p == '|') {
            center = true;
            left = false;
            right = false;
            centerDone = false;
            continue;
        }
        if (*p == '>') {
            right = true;
            left = false;
            center = false;
            rightDone = false;
            continue;
        }
        if (*p == '<') {
            left = true;
            center = false;
            right = false;
            leftDone = false;
            continue;
        }

        {
            // Alignment is only computed once per line (guarded by *Done); measures ahead
            // to the next '\n' using the same glyph-width table as the draw pass below.
            if (center && !centerDone) {
                q = p;
                total = 0;
                while (*q != 0 && *q != '\n') {
                    BIGTEXT_GLYPH_IDX(*q, idx);

                    if (*q != ' ') {
                        adv = Twidth[idx] + Tkern[idx];
                    } else {
                        adv = 8;
                    }
                    total = total + adv;
                    q++;
                }
                centerDone = true;
                x = (g_screenW - total) >> 1;
            }
            if (left && !leftDone) {
                leftDone = true;
                x = 0;
            }

            if (right && !rightDone) {
                q = p;
                total = 0;
                while (*q != 0 && *q != '\n') {
                    BIGTEXT_GLYPH_IDX(*q, idx);

                    if (*q != ' ') {
                        adv = Twidth[idx] + Tkern[idx];
                    } else {
                        adv = 8;
                    }
                    total = total + adv;
                    q++;
                }
                rightDone = true;
                x = g_screenW - total;
            }

            if (*p == '\n') {
                g_curY = y;
                g_textCursorY = y + 25;
                y = g_textCursorY;
                x = g_textStartX;
                centerDone = false;
                leftDone = false;
                rightDone = false;
                continue;
            }

            // ---- draw the glyph, clipped to the current clip rect ----
            {
                BIGTEXT_GLYPH_IDX(*p, idx);

                if (*p != ' ') {
                    src.x1 = Txstart[idx];
                    src.y1 = Tystart[idx];
                    src.y2 = src.y1 + Theight[idx];
                    src.x2 = src.x1 + Twidth[idx];
                    adv = Twidth[idx] + Tkern[idx];
                    dx = x;
                    dy = y + Ttop[idx];
                    w = Twidth[idx];
                    h = Theight[idx];
                    if (!(dx < g_clipRight && dy < g_clipBottom &&
                          dx + w > g_clipLeft && dy + h > g_clipTop)) {
                    } else {
                        if (dx < g_clipLeft) {
                            src.x1 = g_clipLeft - dx + src.x1;
                            w = w - (g_clipLeft - dx);
                            dx = g_clipLeft;
                        } else if (dx + w >= g_clipRight) {
                            w = g_clipRight - dx;
                        }

                        if (dy < g_clipTop) {
                            src.y1 = g_clipTop - dy + src.y1;
                            h = h - (g_clipTop - dy);
                            dy = g_clipTop;
                        } else if (dy + h >= g_clipBottom) {
                            h = g_clipBottom - dy;
                        }
                        src.x2 = src.x1 + w;
                        src.y2 = src.y1 + h;
                        QueueBlit((float)dx, (float)dy, g_fontGfx, &src);
                    }
                } else {
                    adv = 8;
                }
                x = x + adv;
                g_cursorX = x;
            }
        }
    }
    g_curY = y;
    g_textCursorY = y + 25;
}

#undef BIGTEXT_GLYPH_IDX

// Like DrawTinyText(), but keeps x/y as ints instead of floats (no fractional
// positioning) and sets src.x1/x2 inline with each row instead of once before the switch.
// Same `@@`/`@Snnn`/`@E` link escapes; unlike DrawTinyText() this one has no '\n' handling.
void DrawTinyTextAlt(const char *text, int x, int y, int row)
{
    const char *p;
    Rect16 src;
    int glyph;
    int width;
    int dx;
    int dy;
    int w;
    int h;
    int advance;

    if (y == g_textAutoY)
        y = g_textCursorY;

    // Centering: measure the string width (accounting for @-escapes), then start there.
    width = 0;
    if (x == POS_CENTERED) {
        for (p = text; *p != 0; p++) {
            advance = 0;

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
                advance = 8;
                if (*p == '.')
                    advance = 8;
                if (*p == ',')
                    advance = 8;
            }
            width += advance;
        }

        x = (g_screenW >> 1) - width / 2;
    }
    g_textStartX = x;

    // Draw pass: same @-escape handling, plus recording each link's on-screen box.
    for (p = text; *p != 0; p++) {
        if (*p == '@') {
            switch (p[1]) {
            case '@':
                g_linkCount = 0;
                p++;
                break;
            case 'S':
                g_links[g_linkCount].id = (p[2] - '0') * 100 + (p[3] - '0') * 10 + p[4] - '0';
                g_links[g_linkCount].x1 = x;
                g_links[g_linkCount].y1 = y;
                p += 4;
                break;
            case 'E':
                g_links[g_linkCount].x2 = x;
                g_links[g_linkCount].y2 = y + 8;
                g_linkCount++;
                p++;
                break;
            }

        } else {
            while (1) {
                glyph = 0;
                if (*p >= '0' && *p <= '9')
                    glyph = *p - 22;
                if (*p >= 'A' && *p <= 'Z')
                    glyph = *p - 'A';
                if (*p >= 'a' && *p <= 'z')
                    glyph = *p - 'a';
                advance = 8;
                if (*p == '.') {
                    glyph = 36;
                    advance = 8;
                }
                if (*p == ',') {
                    glyph = 37;
                    advance = 8;
                }

                if (*p == ':')
                    glyph = 38;
                if (*p == '?')
                    glyph = 39;
                if (*p == '!')
                    glyph = 40;
                if (*p == '*')
                    glyph = 41;
                if (*p == '=')
                    glyph = 42;
                if (*p == '$')
                    glyph = 43;

                if (*p == 0xC2A3)
                    glyph = 44;
                if (*p == '-')
                    glyph = 45;
                if (*p == '+')
                    glyph = 47;
                if (*p == '<')
                    glyph = 48;
                if (*p == '>')
                    glyph = 49;
                if (*p == '_')
                    glyph = 50;
                if (*p == '#')
                    glyph = 51;
                if (*p == '%')
                    glyph = 52;

                if (*p != ' ') {
                    if (row == 1) {
                        src.x1 = glyph * 8;
                        src.y1 = 13;
                        src.y2 = src.y1 + 8;
                    }
                    if (row == 2) {
                        src.x1 = glyph * 8;
                        src.y1 = 21;
                        src.y2 = src.y1 + 8;
                    }
                    if (row == 3) {
                        src.x1 = glyph * 8;
                        src.y1 = 29;
                        src.y2 = src.y1 + 8;
                    }
                    if (row == 4) {
                        src.x1 = glyph * 8;
                        src.y1 = 37;
                        src.y2 = src.y1 + 8;
                    }

                    src.x2 = src.x1 + 8;
                    dx = x;
                    dy = y;
                    w = advance;
                    h = 8;

                    if (!(dx < g_clipRight && dy < g_clipBottom &&
                          dx + w > g_clipLeft && dy + h > g_clipTop))
                        break;
                    if (dx < g_clipLeft) {
                        src.x1 = g_clipLeft - dx + src.x1;
                        w = w - (g_clipLeft - dx);
                        dx = g_clipLeft;
                    } else if (dx + w >= g_clipRight) {
                        w = g_clipRight - dx;
                    }

                    if (dy < g_clipTop) {
                        src.y1 = g_clipTop - dy + src.y1;
                        h = h - (g_clipTop - dy);
                        dy = g_clipTop;
                    } else if (dy + h >= g_clipBottom) {
                        h = g_clipBottom - dy;
                    }
                    src.x2 = src.x1 + w;
                    src.y2 = src.y1 + h;
                    QueueBlit((float)dx, (float)dy, g_tinyFont, &src);
                    break;
                } else {
                    break;
                }
            }
            x += advance;
            g_cursorX = x;
        }
    }
    g_curY = y;
    g_textCursorY = y + 8;
}

// Empty in the original binary (no-op stubs); kept for the byte match.
void EmptyTextStubA()
{
}

// Empty in the original binary (no-op stub); kept for the byte match.
void EmptyTextStubB()
{
}

// Draws a digit string `s` (spaces skipped) from sprite sheet row `row` starting at (x, y).
void DrawNumberRow(const char *s, int x, int y, int row)
{
    int pos;
    const char *p;
    float off;
    off = 0;
    for (p = s; *p; p++) {
        if (*p >= '0' && *p <= '9')
            pos = *p - '0';
        if (*p != ' ')
            Blit((int)(x + off), y, 0, g_numbersGfx, row * 160 + pos * 8, 0, 8, 9);
        off += 7.0;
    }
}
