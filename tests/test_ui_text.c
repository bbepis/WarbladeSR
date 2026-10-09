// Tests for src/gfx/text.c: the bitmap fonts' glyph mapping, layout (advance, centering, line
// breaks, link escapes) and clipping, observed through the queued blits (g_blit[]).
#include "support.h"

static void Screen(void)
{
    g_screenW = 800;
    g_screenH = 600;
    ResetClip();
    g_blitCount = 0;
    g_blit2Count = 0;
}

// ---- DrawMenuText: 12x9 cells from g_smallFont ----

TEST(ui_DrawMenuText_maps_letters_and_digits_to_cells)
{
    Screen();
    DrawMenuText("A3Z", 100, 50, 2);
    CHECK_EQ_INT(g_blitCount, 3);
    CHECK_EQ_INT(g_blit[0].src.x1, 10 * 12);   // 'A' is cell 10
    CHECK_EQ_INT(g_blit[0].src.y1, 2 * 9);
    CHECK_EQ_INT(g_blit[0].src.x2, 10 * 12 + 12);
    CHECK_EQ_INT(g_blit[0].src.y2, 2 * 9 + 9);
    CHECK_EQ_INT(g_blit[1].src.x1, 3 * 12);
    CHECK_EQ_INT(g_blit[2].src.x1, 35 * 12);
    CHECK_NEAR(g_blit[0].destX, 100, 0);
    CHECK_NEAR(g_blit[1].destX, 112, 0);
    CHECK_NEAR(g_blit[2].destX, 124, 0);
    CHECK_NEAR(g_blit[2].destY, 50, 0);
    CHECK(g_blit[0].graphic == g_smallFont);
}

TEST(ui_DrawMenuText_punctuation_cells)
{
    Screen();
    DrawMenuText(".,:?!*=$+/#_~<>-", 0, 0, 0);
    int cells[] = { 36, 37, 41, 39, 42, 43, 40, 38, 44, 45, 46, 47, 48, 50, 51, 39 };
    CHECK_EQ_INT(g_blitCount, 16);
    for (int i = 0; i < 16; i++)
        CHECK_EQ_INT(g_blit[i].src.x1, cells[i] * 12);
}

TEST(ui_DrawMenuText_spaces_advance_without_drawing)
{
    Screen();
    DrawMenuText("A  B", 10, 20, 0);
    CHECK_EQ_INT(g_blitCount, 2);
    CHECK_NEAR(g_blit[1].destX, 10 + 3 * 12, 0);
    CHECK_EQ_INT(g_cursorX, 10 + 4 * 12);
    CHECK_EQ_INT(g_textStartX, 10);
    CHECK_EQ_INT(g_curY, 20);
    CHECK_EQ_INT(g_textCursorY, 32);
}

TEST(ui_DrawMenuText_centers_on_the_screen)
{
    Screen();
    DrawMenuText("ABCD", POS_CENTERED, 30, 0);
    CHECK_EQ_INT(g_textStartX, 400 - 24);
    CHECK_NEAR(g_blit[0].destX, 376, 0);
}

TEST(ui_DrawMenuText_centering_ignores_link_escapes)
{
    Screen();
    DrawMenuText("@@@S123AB@E", POS_CENTERED, 30, 0);
    CHECK_EQ_INT(g_textStartX, 400 - 12);
    CHECK_EQ_INT(g_blitCount, 2);
}

TEST(ui_DrawMenuText_records_links)
{
    Screen();
    g_linkCount = 5;
    DrawMenuText("@@X@S042AB@E@S007C@E", 100, 40, 0);
    CHECK_EQ_INT(g_linkCount, 2);
    CHECK_EQ_INT(g_links[0].id, 42);
    CHECK_EQ_INT(g_links[0].x1, 112);
    CHECK_EQ_INT(g_links[0].y1, 40);
    CHECK_EQ_INT(g_links[0].x2, 136);
    CHECK_EQ_INT(g_links[0].y2, 52);
    CHECK_EQ_INT(g_links[1].id, 7);
    CHECK_EQ_INT(g_links[1].x1, 136);
    CHECK_EQ_INT(g_links[1].x2, 148);
    CHECK_EQ_INT(g_blitCount, 4);
}

TEST(ui_DrawMenuText_newline_returns_to_x)
{
    Screen();
    DrawMenuText("AB\nC", 100, 40, 0);
    CHECK_EQ_INT(g_blitCount, 3);
    CHECK_NEAR(g_blit[2].destX, 100, 0);
    CHECK_NEAR(g_blit[2].destY, 53, 0);
    CHECK_EQ_INT(g_curY, 53);
    CHECK_EQ_INT(g_textCursorY, 65);
}

TEST(ui_DrawMenuText_auto_y_continues_below)
{
    Screen();
    DrawMenuText("A", 10, 100, 0);
    DrawMenuText("B", 10, g_textAutoY, 0);
    CHECK_NEAR(g_blit[1].destY, 112, 0);
    DrawMenuText("C", 10, g_textAutoY, 0);
    CHECK_NEAR(g_blit[2].destY, 124, 0);
}

TEST(ui_DrawMenuText_clips_at_the_edges)
{
    Screen();
    DrawMenuText("AB", 794, 10, 0);       // A: 6 px visible, B: off screen
    CHECK_EQ_INT(g_blitCount, 1);
    CHECK_EQ_INT(g_blit[0].src.x2 - g_blit[0].src.x1, 6);
    DrawMenuText("A", -5, 10, 0);          // 7 px visible, source moved right by 5
    CHECK_EQ_INT(g_blitCount, 2);
    CHECK_NEAR(g_blit[1].destX, 0, 0);
    CHECK_EQ_INT(g_blit[1].src.x1, 120 + 5);
    CHECK_EQ_INT(g_blit[1].src.x2, 120 + 12);
    DrawMenuText("A", 10, 596, 1);         // 4 rows visible
    CHECK_EQ_INT(g_blit[2].src.y2 - g_blit[2].src.y1, 4);
    DrawMenuText("A", 10, -3, 1);
    CHECK_NEAR(g_blit[3].destY, 0, 0);
    CHECK_EQ_INT(g_blit[3].src.y1, 9 + 3);
}

// ---- DrawTinyText / DrawTinyText2: 8x8 cells from g_tinyFont ----

TEST(ui_DrawTinyText_rows_and_cells)
{
    Screen();
    DrawTinyText("A0", 10, 10, 1);
    DrawTinyText("Z", 10, 10, 2);
    DrawTinyText("9", 10, 10, 3);
    DrawTinyText("B", 10, 10, 4);
    CHECK_EQ_INT(g_blitCount, 5);
    CHECK_EQ_INT(g_blit[0].src.x1, 0);
    CHECK_EQ_INT(g_blit[0].src.y1, 13);
    CHECK_EQ_INT(g_blit[1].src.x1, 26 * 8);
    CHECK_EQ_INT(g_blit[2].src.x1, 25 * 8);
    CHECK_EQ_INT(g_blit[2].src.y1, 21);
    CHECK_EQ_INT(g_blit[3].src.x1, 35 * 8);
    CHECK_EQ_INT(g_blit[3].src.y1, 29);
    CHECK_EQ_INT(g_blit[4].src.y1, 37);
    CHECK_EQ_INT(g_blit[4].src.y2, 45);
    CHECK(g_blit[0].graphic == g_tinyFont);
}

TEST(ui_DrawTinyText_punctuation_cells)
{
    Screen();
    DrawTinyText(".,:?!*=$-+<>_#%", 0, 0, 1);
    int cells[] = { 36, 37, 38, 39, 40, 41, 42, 43, 45, 47, 48, 49, 50, 51, 52 };
    CHECK_EQ_INT(g_blitCount, 15);
    for (int i = 0; i < 15; i++)
        CHECK_EQ_INT(g_blit[i].src.x1, cells[i] * 8);
}

TEST(ui_DrawTinyText_layout)
{
    Screen();
    DrawTinyText("AB C\nD", 100, 40, 1);
    CHECK_EQ_INT(g_blitCount, 4);
    CHECK_NEAR(g_blit[1].destX, 108, 0);
    CHECK_NEAR(g_blit[2].destX, 124, 0);
    CHECK_NEAR(g_blit[3].destX, 100, 0);
    CHECK_NEAR(g_blit[3].destY, 51, 0);
    CHECK_EQ_INT(g_textCursorY, 59);
    DrawTinyText("ABCDEF", POS_CENTERED, 0, 1);
    CHECK_EQ_INT(g_textStartX, 400 - 24);
}

TEST(ui_DrawTinyText_records_links)
{
    Screen();
    DrawTinyText("@@@S100AB@E", 50, 60, 1);
    CHECK_EQ_INT(g_linkCount, 1);
    CHECK_EQ_INT(g_links[0].id, 100);
    CHECK_EQ_INT(g_links[0].x1, 50);
    CHECK_EQ_INT(g_links[0].x2, 66);
    CHECK_EQ_INT(g_links[0].y2, 68);
}

TEST(ui_DrawTinyText2_uses_the_second_queue)
{
    Screen();
    DrawTinyText2("ABC", POS_CENTERED, 10, 2);
    CHECK_EQ_INT(g_blitCount, 0);
    CHECK_EQ_INT(g_blit2Count, 3);
    CHECK_EQ_INT(g_textStartX, 400 - 12);
    CHECK_EQ_INT(g_blit2[2].src.x1, 2 * 8);
    CHECK_EQ_INT(g_blit2[2].src.y1, 21);
}

TEST(ui_DrawTinyTextAlt_draws_like_DrawTinyText)
{
    Screen();
    DrawTinyTextAlt("AB", 20, 30, 3);
    CHECK_EQ_INT(g_blitCount, 2);
    CHECK_EQ_INT(g_blit[1].src.x1, 8);
    CHECK_EQ_INT(g_blit[1].src.y1, 29);
    CHECK_NEAR(g_blit[1].destX, 28, 0);
}

// ---- DrawNewsText: 8x8 cells, rows 94..150 ----

TEST(ui_DrawNewsText_font_rows)
{
    Screen();
    int rows[] = { 94, 102, 110, 118, 126, 134, 142, 150 };
    for (int f = 5; f <= 12; f++)
        DrawNewsText("C", 0, 0, f);
    CHECK_EQ_INT(g_blitCount, 8);
    for (int i = 0; i < 8; i++) {
        CHECK_EQ_INT(g_blit[i].src.y1, rows[i]);
        CHECK_EQ_INT(g_blit[i].src.x1, 16);
    }
}

TEST(ui_DrawNewsText_layout_and_centering)
{
    Screen();
    DrawNewsText("1 2\n3", 300, 100, 5);
    CHECK_EQ_INT(g_textStartX, 300);
    CHECK_EQ_INT(g_blitCount, 3);
    CHECK_EQ_INT(g_blit[0].src.x1, 27 * 8);
    CHECK_NEAR(g_blit[1].destX, 300 + 16, 0);
    CHECK_NEAR(g_blit[2].destX, 300, 0);
    CHECK_NEAR(g_blit[2].destY, 111, 0);
    CHECK_EQ_INT(g_textCursorY, 119);
    DrawNewsText("12@S001X@E", POS_CENTERED, 0, 5);   // 3 glyphs
    CHECK_EQ_INT(g_textStartX, 400 - 12);
}

TEST(ui_DrawNewsText_centered_text_loses_its_second_line)
{
    // NOTE: a line break goes back to x, which is still the POS_CENTERED sentinel: the
    // following lines of centered text land far off screen and are clipped away.
    Screen();
    DrawNewsText("12\n3", POS_CENTERED, 100, 5);
    CHECK_EQ_INT(g_blitCount, 2);
}

// ---- other fonts ----

TEST(ui_DrawScoreDigits_blits_32px_digits)
{
    Screen();
    DrawScoreDigits("12:5 0", 100, 20, 0, 1.0f);
    CHECK_EQ_INT(g_blitCount, 5);
    CHECK_EQ_INT(g_blit[0].src.x1, 32);
    CHECK_EQ_INT(g_blit[1].src.x1, 64);
    CHECK_EQ_INT(g_blit[2].src.x1, 320);
    CHECK_EQ_INT(g_blit[4].src.x1, 0);
    CHECK_NEAR(g_blit[4].destX, 100 + 5 * 32, 0);
    CHECK_EQ_INT(g_cursorX, 100 + 6 * 32);
    CHECK_EQ_INT(g_textCursorY, 44);
    DrawScoreDigits("99", POS_CENTERED, 20, 0, 1.0f);
    CHECK_EQ_INT(g_textStartX, 400 - 32);
}

TEST(ui_DrawMixedCaseText_proportional_advance)
{
    Screen();
    // 'A' 4 wide, 'i' 2 wide (cell 34), space 3: each glyph advances width + 1
    DrawMixedCaseText("Ai A", 10, 10, 0);
    CHECK_EQ_INT(g_blitCount, 3);
    CHECK_NEAR(g_blit[0].destX, 10, 0);
    CHECK_NEAR(g_blit[1].destX, 15, 0);
    CHECK_EQ_INT(g_blit[1].src.x1, 34 * 8);
    CHECK_EQ_INT(g_blit[1].src.x2, 34 * 8 + 2);
    CHECK_NEAR(g_blit[2].destX, 15 + 3 + 4, 0);
    CHECK_EQ_INT(g_blit[0].src.y1, 78);
    DrawMixedCaseText("a", 10, 10, 1);
    CHECK_EQ_INT(g_blit[3].src.y1, 86);
    CHECK_EQ_INT(g_blit[3].src.x1, 26 * 8);
}

TEST(ui_DrawMixedCaseText_bar_breaks_lines)
{
    Screen();
    DrawMixedCaseText("A|B", 10, 10, 0);
    // NOTE: the break takes the glyph width off but the loop then adds width + 1, so the
    // next line starts one pixel to the right of x.
    CHECK_NEAR(g_blit[1].destX, 11, 0);
    CHECK_NEAR(g_blit[1].destY, 18, 0);
}

TEST(ui_DrawNumberRow_steps_seven_pixels)
{
    Screen();
    DrawNumberRow("4 2", 100, 10, 2);
    CHECK_EQ_INT(g_blitCount, 2);
    CHECK_EQ_INT(g_blit[0].src.x1, 2 * 160 + 4 * 8);
    CHECK_NEAR(g_blit[1].destX, 114, 0);
    CHECK_EQ_INT(g_blit[1].src.x1, 2 * 160 + 2 * 8);
    CHECK_EQ_INT(g_blit[1].src.y2, 9);
}

TEST(ui_DrawPleaseWait_blinks)
{
    Screen();
    g_blinkRate = 300;
    g_time = 10000;
    g_uiBlinkTime = 10000;
    g_uiBlink = 0;
    DrawPleaseWait();
    CHECK_EQ_INT(g_blitCount, 0);
    g_time = 10301;
    DrawPleaseWait();
    CHECK_EQ_INT(g_uiBlink, 1);
    CHECK_EQ_INT(g_blitCount, 13);   // "PLEASE WAIT..." without the space
    CHECK_EQ_INT(g_textStartX, 400 - 7 * 12);
}

TEST(ui_DrawBigText_proportional_glyphs)
{
    Screen();
    g_fontGfx = NULL;
    // NOTE: the glyph table's comment says digits come first, but the code maps 'A'-'Z' to
    // glyphs 0-25 and the digits to 26-35.
    DrawBigText(100, 50, "AB Q");
    CHECK_EQ_INT(g_blitCount, 3);
    CHECK_EQ_INT(g_blit[0].src.x1, 0);     // 'A': glyph 0, 12 x 17, 2 below the top
    CHECK_EQ_INT(g_blit[0].src.x2, 12);
    CHECK_EQ_INT(g_blit[0].src.y2, 17);
    CHECK_NEAR(g_blit[0].destX, 100, 0);
    CHECK_NEAR(g_blit[0].destY, 52, 0);
    CHECK_EQ_INT(g_blit[1].src.x1, 16);    // 'B': 13 wide
    CHECK_NEAR(g_blit[1].destX, 112, 0);
    CHECK_NEAR(g_blit[2].destX, 125 + 8, 0);   // a space is 8
    CHECK_EQ_INT(g_blit[2].src.x1, 288);
    CHECK_EQ_INT(g_blit[2].src.y2, 22);
    CHECK_EQ_INT(g_cursorX, 133 + 32 - 13);    // 'Q' is kerned by -13
    CHECK_EQ_INT(g_textCursorY, 75);
}

TEST(ui_DrawBigText_digits_and_lowercase)
{
    Screen();
    DrawBigText(0, 0, "0a");
    CHECK_EQ_INT(g_blit[0].src.x1, 480);   // '0': glyph 26
    CHECK_EQ_INT(g_blit[1].src.x1, 0);     // 'a': glyph 41, second row
    CHECK_EQ_INT(g_blit[1].src.y1, 22);
    CHECK_NEAR(g_blit[1].destX, 15, 0);
}

TEST(ui_DrawBigText_alignment_marks)
{
    Screen();
    DrawBigText(-1, 50, "AB");   // centered: 25 px wide
    CHECK_NEAR(g_blit[0].destX, 387, 0);
    DrawBigText(100, 50, ">AB");
    CHECK_NEAR(g_blit[2].destX, 775, 0);
    DrawBigText(100, 50, "<AB");
    CHECK_NEAR(g_blit[4].destX, 0, 0);
    DrawBigText(100, 50, "|AB");
    CHECK_NEAR(g_blit[6].destX, 387, 0);
}

TEST(ui_DrawBigText_newline_restarts_the_line)
{
    Screen();
    DrawBigText(100, 50, "A\nB");
    CHECK_NEAR(g_blit[1].destX, 100, 0);
    CHECK_NEAR(g_blit[1].destY, 77, 0);
    CHECK_EQ_INT(g_curY, 75);
}

TEST(ui_bitmap_fonts_render_lowercase_without_changing_input)
{
    const char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char lower[] = "abcdefghijklmnopqrstuvwxyz";
    void (*draw[])(const char *, int, int, int) = {
        DrawMenuText, DrawTinyText, DrawTinyText2, DrawTinyTextAlt
    };
    Rect16 cells[26];
    for (int font = 0; font < 5; font++) {
        Screen();
        if (font < 4) draw[font](upper, 10, 20, 1);
        else DrawNewsText((char *)upper, 10, 20, 7);
        CHECK_EQ_INT(font == 2 ? g_blit2Count : g_blitCount, 26);
        for (int i = 0; i < 26; i++) cells[i] = font == 2 ? g_blit2[i].src : g_blit[i].src;
        Screen();
        if (font < 4) draw[font](lower, 10, 20, 1);
        else DrawNewsText(lower, 10, 20, 7);
        CHECK_EQ_INT(font == 2 ? g_blit2Count : g_blitCount, 26);
        for (int i = 0; i < 26; i++) {
            CHECK_EQ_INT((font == 2 ? g_blit2[i].src : g_blit[i].src).x1, cells[i].x1);
            CHECK_EQ_INT((font == 2 ? g_blit2[i].src : g_blit[i].src).y1, cells[i].y1);
        }
        CHECK(strcmp(lower, "abcdefghijklmnopqrstuvwxyz") == 0);
    }
}
