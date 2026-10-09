// Tests for src/ui/window.c (the popup window table, its widgets and drawing) and the window
// half of src/ui/menu.c's MenuUpdate (keyboard/mouse navigation of popup windows).
#include "support.h"

// A screen and clip rect for the unit tests that don't boot the game.
static void Screen(void)
{
    g_screenW = 800;
    g_screenH = 600;
    ResetClip();
}

// ---------------------------------------------------------------------------------------
// The window table
// ---------------------------------------------------------------------------------------

TEST(ui_WinInit_deactivates_and_resets_every_window)
{
    for (int i = 0; i < MAX_WINDOWS; i++) {
        g_windows[i].active = 1;
        g_windows[i].index = 99;
        g_windows[i].nF = 3;
        g_windows[i].nE = 3;
        g_windows[i].firstH = 2;
        g_windows[i].blinkTimer = 3;
        g_windows[i].blinkOn = 1;
    }
    WinInit();
    for (int i = 0; i < MAX_WINDOWS; i++) {
        CHECK_EQ_INT(g_windows[i].active, 0);
        CHECK_EQ_INT(g_windows[i].index, i);
        CHECK_EQ_INT(g_windows[i].nF, -1);
        CHECK_EQ_INT(g_windows[i].nE, -1);
        CHECK_EQ_INT(g_windows[i].firstH, -1);
        CHECK_NEAR(g_windows[i].blinkTimer, 25.0, 0);
        CHECK_EQ_INT(g_windows[i].blinkOn, 0);
    }
    CHECK(!AnyWindowActive());
}

TEST(ui_WinOpen_takes_the_first_free_slot)
{
    WinInit();
    CHECK_EQ_INT(WinOpen(10, 20, 100, 50, WIN_MODE_TILED), 0);
    CHECK_EQ_INT(WinOpen(10, 20, 100, 50, WIN_MODE_TILED), 1);
    CHECK_EQ_INT(WinOpen(10, 20, 100, 50, WIN_MODE_TILED), 2);
    WinClose(1);
    CHECK_EQ_INT(g_windows[1].active, 0);
    CHECK_EQ_INT(WinFindFree(), 1);
    CHECK_EQ_INT(WinOpen(10, 20, 100, 50, WIN_MODE_TILED), 1);
    CHECK_EQ_INT(WinFindFree(), 3);
}

TEST(ui_WinOpen_returns_minus_one_when_all_ten_are_open)
{
    WinInit();
    for (int i = 0; i < MAX_WINDOWS; i++)
        CHECK_EQ_INT(WinOpen(0, 0, 64, 64, WIN_MODE_TILED), i);
    CHECK_EQ_INT(WinFindFree(), -1);
    CHECK_EQ_INT(WinOpen(0, 0, 64, 64, WIN_MODE_TILED), -1);
}

TEST(ui_WinOpen_sets_geometry_and_clears_the_lists)
{
    WinInit();
    g_windows[0].menuItems[3].checked = 1;
    g_windows[0].imageRects[2].graphic = (Image *)&g_windows[0];
    g_windows[0].nE = 7;
    int w = WinOpen(11, 22, 333, 144, WIN_MODE_TILED);
    CHECK_EQ_INT(w, 0);
    CHECK_EQ_INT(g_windows[w].active, 1);
    CHECK_EQ_INT(g_windows[w].visible, 1);
    CHECK_EQ_INT(g_windows[w].x, 11);
    CHECK_EQ_INT(g_windows[w].y, 22);
    CHECK_EQ_INT(g_windows[w].w, 333);
    CHECK_EQ_INT(g_windows[w].h, 144);
    CHECK_EQ_INT(g_windows[w].mode, WIN_MODE_TILED);
    CHECK_EQ_INT(g_windows[w].selF, -1);
    CHECK_EQ_INT(g_windows[w].nE, -1);
    CHECK_EQ_INT(g_windows[w].nF, -1);
    CHECK_EQ_INT(g_windows[w].nH, -1);
    CHECK_EQ_INT(g_windows[w].firstH, -1);
    CHECK_EQ_INT(g_windows[w].menuItems[3].checked, 0);
    CHECK(g_windows[w].imageRects[2].graphic == NULL);
    CHECK_NEAR(g_windows[w].slideX, 0.0, 0);
    CHECK_NEAR(g_windows[w].slideY, 0.0, 0);
}

TEST(ui_WinOpen_sliding_window_starts_off_screen)
{
    WinInit();
    int w = WinOpen(0, 0, 100, 100, WIN_MODE_SLIDING);
    CHECK_NEAR(g_windows[w].slideX, -700.0, 0);
    CHECK_NEAR(g_windows[w].slideY, 0.85, 1e-6);
}

TEST(ui_WinClose_tiled_window_closes_at_once)
{
    WinInit();
    int w = WinOpen(0, 0, 100, 100, WIN_MODE_TILED);
    WinAddText(1, 1, w, "HI", 1);
    WinAddMenuItem(1, 1, w, 5, "OK", 1);
    WinClose(w);
    CHECK_EQ_INT(g_windows[w].active, 0);
    CHECK_EQ_INT(g_windows[w].visible, 0);
    CHECK_EQ_INT(g_windows[w].nE, -1);
    CHECK_EQ_INT(g_windows[w].nF, -1);
    CHECK(!AnyWindowActive());
}

TEST(ui_WinClose_sliding_window_slides_out_and_stays_active)
{
    WinInit();
    int w = WinOpen(0, 0, 100, 100, WIN_MODE_SLIDING);
    WinAddMenuItem(1, 1, w, 5, "OK", 1);
    WinClose(w);
    CHECK_EQ_INT(g_windows[w].active, 1);
    CHECK_EQ_INT(g_windows[w].nF, 0);
    CHECK_NEAR(g_windows[w].slideX, -5.0, 0);
    CHECK_NEAR(g_windows[w].slideY, 1.2, 1e-6);
}

TEST(ui_WinCloseAll_hides_sliding_windows_and_closes_the_rest)
{
    WinInit();
    int a = WinOpen(0, 0, 100, 100, WIN_MODE_TILED);
    int b = WinOpen(0, 0, 100, 100, WIN_MODE_SLIDING);
    WinAddText(1, 1, b, "TEXT", 1);
    WinAddMenuItem(1, 1, b, 5, "OK", 1);
    WinAddEdit(1, 1, b, 5, 0, 1, 1);
    WinCloseAll();
    CHECK_EQ_INT(g_windows[a].active, 0);
    CHECK_EQ_INT(g_windows[b].active, 1);
    CHECK_EQ_INT(g_windows[b].visible, 0);
    CHECK_EQ_INT(g_windows[b].nF, -1);
    CHECK_EQ_INT(g_windows[b].nH, -1);
    CHECK_EQ_INT(g_windows[b].nE, 0);   // the texts stay while it slides out
    CHECK_NEAR(g_windows[b].slideX, -5.0, 0);
    CHECK_NEAR(g_windows[b].slideY, 1.2, 1e-6);
}

TEST(ui_WinHideAll_keeps_windows_active)
{
    WinInit();
    int a = WinOpen(0, 0, 100, 100, WIN_MODE_TILED);
    int b = WinOpen(0, 0, 100, 100, WIN_MODE_SLIDING);
    WinHideAll();
    CHECK_EQ_INT(g_windows[a].visible, 0);
    CHECK_EQ_INT(g_windows[b].visible, 0);
    CHECK_EQ_INT(g_windows[a].active, 1);
    CHECK_EQ_INT(g_windows[b].active, 1);
}

// ---------------------------------------------------------------------------------------
// Widgets
// ---------------------------------------------------------------------------------------

TEST(ui_WinAddText_appends_lines)
{
    WinInit();
    int w = WinOpen(0, 0, 100, 100, WIN_MODE_TILED);
    WinAddText(3, 4, w, "FIRST", 2);
    WinAddText(5, 6, w, "SECOND LINE", 7);
    CHECK_EQ_INT(g_windows[w].nE, 1);
    CHECK_STR(g_windows[w].texts[0].text, "FIRST");
    CHECK_EQ_INT(g_windows[w].texts[0].x, 3);
    CHECK_EQ_INT(g_windows[w].texts[0].y, 4);
    CHECK_EQ_INT(g_windows[w].texts[0].color, 2);
    CHECK_STR(g_windows[w].texts[1].text, "SECOND LINE");
    CHECK_EQ_INT(g_windows[w].texts[1].x, 5);
    CHECK_EQ_INT(g_windows[w].texts[1].y, 6);
    CHECK_EQ_INT(g_windows[w].texts[1].color, 7);
}

TEST(ui_WinAddText_ignores_inactive_windows)
{
    WinInit();
    WinAddText(3, 4, 2, "NOPE", 2);
    CHECK_EQ_INT(g_windows[2].nE, -1);
}

TEST(ui_WinAddText_holds_fifty_lines)
{
    WinInit();
    int w = WinOpen(0, 0, 100, 100, WIN_MODE_TILED);
    char buf[8];
    for (int i = 0; i < 50; i++) {
        sprintf(buf, "L%d", i);
        WinAddText(0, i, w, buf, 1);
    }
    CHECK_EQ_INT(g_windows[w].nE, 49);
    CHECK_STR(g_windows[w].texts[49].text, "L49");
    CHECK_EQ_INT(g_windows[w].texts[49].y, 49);
}

TEST(ui_WinAddTextPair_uses_the_index_as_default_id)
{
    WinInit();
    int w = WinOpen(0, 0, 100, 100, WIN_MODE_TILED);
    WinAddTextPair(1, 2, w, -1, "LINK", "http://a", 3, 4);
    WinAddTextPair(5, 6, w, 77, "OTHER", "http://b", 5, 6);
    CHECK_EQ_INT(g_windows[w].nD, 1);
    CHECK_EQ_INT(g_windows[w].links[0].id, 0);
    CHECK_EQ_INT(g_windows[w].links[1].id, 77);
    CHECK_STR(g_windows[w].links[0].text1, "LINK");
    CHECK_STR(g_windows[w].links[0].text2, "http://a");
    CHECK_STR(g_windows[w].links[1].text2, "http://b");
    CHECK_EQ_INT(g_windows[w].links[0].color, 3);
    CHECK_EQ_INT(g_windows[w].links[0].hoverColor, 4);
    CHECK_EQ_INT(g_windows[w].links[1].x, 5);
    CHECK_EQ_INT(g_windows[w].links[1].y, 6);
}

TEST(ui_WinAddMenuItem_sizes_the_item_from_its_text)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    WinAddMenuItem(10, 20, w, -1, "OK", 5);
    WinAddMenuItem(30, 40, w, 1234, "CANCEL", 3);
    CHECK_EQ_INT(g_windows[w].nF, 1);
    CHECK_EQ_INT(g_windows[w].menuItems[0].id, 0);
    CHECK_EQ_INT(g_windows[w].menuItems[0].len, 2);
    CHECK_EQ_INT(g_windows[w].menuItems[0].w, 2 * 8 + 16);
    CHECK_EQ_INT(g_windows[w].menuItems[0].h, 15);
    CHECK_EQ_INT(g_windows[w].menuItems[0].param, 5);
    CHECK_EQ_INT(g_windows[w].menuItems[1].id, 1234);
    CHECK_EQ_INT(g_windows[w].menuItems[1].len, 6);
    CHECK_EQ_INT(g_windows[w].menuItems[1].w, 6 * 8 + 16);
    CHECK_EQ_INT(g_windows[w].menuItems[1].x, 30);
    CHECK_EQ_INT(g_windows[w].menuItems[1].y, 40);
    CHECK_STR(g_windows[w].menuItems[1].text, "CANCEL");
}

TEST(ui_WinAddMenuItem_holds_forty_items)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    for (int i = 0; i < 40; i++)
        WinAddMenuItem(0, i, w, 100 + i, "X", 1);
    CHECK_EQ_INT(g_windows[w].nF, 39);
    CHECK_EQ_INT(g_windows[w].menuItems[39].id, 139);
}

TEST(ui_WinSetSelected_needs_menu_items)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    WinSetSelected(w, 42);
    CHECK_EQ_INT(g_windows[w].selF, -1);
    WinAddMenuItem(0, 0, w, 42, "A", 1);
    WinSetSelected(w, 42);
    CHECK_EQ_INT(g_windows[w].selF, 42);
}

TEST(ui_WinCheckMenuItem_checks_by_id_and_WinClearMenuChecks_clears)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    WinAddMenuItem(0, 0, w, 7, "A", 1);
    WinAddMenuItem(0, 20, w, 8, "B", 1);
    WinAddMenuItem(0, 40, w, 9, "C", 1);
    WinCheckMenuItem(w, 9);
    CHECK_EQ_INT(g_windows[w].menuItems[0].checked, 0);
    CHECK_EQ_INT(g_windows[w].menuItems[1].checked, 0);
    CHECK_EQ_INT(g_windows[w].menuItems[2].checked, 1);
    int v = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    WinAddMenuItem(0, 0, v, 1, "Z", 1);
    WinCheckMenuItem(v, 1);
    WinClearMenuChecks();
    CHECK_EQ_INT(g_windows[w].menuItems[2].checked, 0);
    CHECK_EQ_INT(g_windows[v].menuItems[0].checked, 0);
}

TEST(ui_WinAddToggle_counts_values_and_advances_nextX)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    g_toggleOnCount = g_toggleOffCount = 0;
    WinAddToggle(40, 5, w, true, "ON", false);
    WinAddToggle(60, 5, w, false, "OFF", true);
    WinAddToggle(80, 5, w, false, "OFF2", false);
    CHECK_EQ_INT(g_windows[w].nG, 2);
    CHECK_EQ_INT(g_toggleOnCount, 1);
    CHECK_EQ_INT(g_toggleOffCount, 2);
    CHECK_EQ_INT(g_nextX, 88);
    CHECK_EQ_INT(g_windows[w].toggles[1].w, 8);
    CHECK_EQ_INT(g_windows[w].toggles[1].h, 11);
    CHECK_EQ_INT(g_windows[w].toggles[1].index, 1);
    CHECK_EQ_INT(g_windows[w].toggles[1].flag2, 1);
    CHECK_EQ_INT(g_windows[w].toggles[0].value, 1);
    CHECK_STR(g_windows[w].toggles[2].text, "OFF2");
}

TEST(ui_WinAddEdit_first_field_takes_the_focus_start)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    CHECK(!AnyWindowHasEdit());
    memset(g_windows[w].edits[0].buf, 'x', sizeof g_windows[w].edits[0].buf);
    WinAddEdit(100, 40, w, 12, 1, 7, 1);
    WinAddEdit(100, 60, w, 5, 0, 3, 0);
    CHECK(AnyWindowHasEdit());
    CHECK_EQ_INT(g_windows[w].nH, 1);
    CHECK_EQ_INT(g_windows[w].firstH, 0);
    CHECK_EQ_INT(g_nextX, 100 + 5 * 8);
    CHECK_EQ_INT(g_windows[w].edits[0].len, 12);
    CHECK_EQ_INT(g_windows[w].edits[0].masked, 1);
    CHECK_EQ_INT(g_windows[w].edits[0].param, 7);
    CHECK_EQ_INT(g_windows[w].edits[0].focused, 1);
    CHECK_EQ_INT(g_windows[w].edits[1].focused, 0);
    CHECK_EQ_INT(g_windows[w].edits[1].index, 1);
    CHECK_EQ_INT(g_windows[w].edits[0].cursor, 0);
    CHECK_EQ_INT(g_windows[w].edits[0].buf[0], 0);
    CHECK_EQ_INT(g_windows[w].edits[0].buf[11], 0);
}

TEST(ui_AnyWindowActive_follows_the_table)
{
    WinInit();
    CHECK(!AnyWindowActive());
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    CHECK(AnyWindowActive());
    WinClose(w);
    CHECK(!AnyWindowActive());
    w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    WinClose(0);
    CHECK(AnyWindowActive());
}

TEST(ui_AnyWindowHasEdit_ignores_closed_windows)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 100, WIN_MODE_TILED);
    WinAddEdit(10, 10, w, 5, 0, 1, 1);
    CHECK(AnyWindowHasEdit());
    WinClose(w);
    CHECK_EQ_INT(g_windows[w].firstH, 0);   // WinClose leaves it
    CHECK(!AnyWindowHasEdit());
}

TEST(ui_WinFocusNextEdit_moves_down_then_wraps_to_the_top)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 200, WIN_MODE_TILED);
    WinAddEdit(10, 50, w, 5, 0, 1, 1);
    WinAddEdit(10, 70, w, 5, 0, 1, 0);
    WinAddEdit(10, 30, w, 5, 0, 1, 0);
    CHECK_EQ_INT(g_windows[w].firstH, 0);
    WinFocusNextEdit(w);
    CHECK_EQ_INT(g_windows[w].firstH, 1);
    CHECK_EQ_INT(g_windows[w].edits[0].focused, 0);
    CHECK_EQ_INT(g_windows[w].edits[1].focused, 1);
    WinFocusNextEdit(w);   // nothing below y=70: the topmost (y=30)
    CHECK_EQ_INT(g_windows[w].firstH, 2);
    CHECK_EQ_INT(g_windows[w].edits[1].focused, 0);
    CHECK_EQ_INT(g_windows[w].edits[2].focused, 1);
    WinFocusNextEdit(w);   // the first one below y=30 in list order: y=50
    CHECK_EQ_INT(g_windows[w].firstH, 0);
    CHECK_EQ_INT(g_windows[w].edits[0].focused, 1);
    CHECK_EQ_INT(g_windows[w].edits[2].focused, 0);
}

TEST(ui_WinFocusNextEdit_does_nothing_for_a_single_field)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 200, WIN_MODE_TILED);
    WinAddEdit(10, 50, w, 5, 0, 1, 0);
    WinFocusNextEdit(w);
    CHECK_EQ_INT(g_windows[w].firstH, 0);
    CHECK_EQ_INT(g_windows[w].edits[0].focused, 0);
}

TEST(ui_WinAddRect_and_items_A_B_append)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 200, WIN_MODE_TILED);
    Image *img = (Image *)&g_windows[9];
    WinAddRect(1, 2, 3, 4, w, img);
    WinAddItemA(1, 2, 3, 4, 5, 6, 7, 8, w, img);
    WinAddItemB(9, 8, 7, 6, 5, 4, "TIP", w);
    CHECK_EQ_INT(g_windows[w].nC, 0);
    CHECK(g_windows[w].imageRects[0].graphic == img);
    CHECK_EQ_INT(g_windows[w].imageRects[0].w, 3);
    CHECK_EQ_INT(g_windows[w].imageRects[0].h, 4);
    CHECK_EQ_INT(g_windows[w].nA, 0);
    CHECK_EQ_INT(g_windows[w].quadImages[0].destW, 3);
    CHECK_EQ_INT(g_windows[w].quadImages[0].srcX, 5);
    CHECK_EQ_INT(g_windows[w].quadImages[0].srcH, 8);
    CHECK_EQ_INT(g_windows[w].nB, 0);
    CHECK_EQ_INT(g_windows[w].buttons[0].x, 9);
    CHECK_EQ_INT(g_windows[w].buttons[0].srcX, 7);
    CHECK_EQ_INT(g_windows[w].buttons[0].w, 5);
    CHECK_EQ_INT(g_windows[w].buttons[0].h, 4);
    CHECK_STR(g_windows[w].buttons[0].text, "TIP");
}

TEST(ui_WinAddRect_holds_five_rects)
{
    WinInit();
    int w = WinOpen(0, 0, 300, 200, WIN_MODE_TILED);
    for (int i = 0; i < 5; i++)
        WinAddRect(i, 0, 1, 1, w, NULL);
    CHECK_EQ_INT(g_windows[w].nC, 4);
    CHECK_EQ_INT(g_windows[w].imageRects[4].x, 4);
}

// ---------------------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------------------

TEST(ui_WinDraw_resolves_a_centered_position)
{
    Screen();
    WinInit();
    int w = WinOpen(POS_CENTERED, POS_CENTERED, 300, 100, WIN_MODE_TILED);
    WinDraw(w);
    CHECK_EQ_INT(g_windows[w].x, 400 - 150);
    CHECK_EQ_INT(g_windows[w].y, 300 - 50);
}

TEST(ui_WinDraw_centers_menu_items_once)
{
    Screen();
    WinInit();
    int w = WinOpen(100, 100, 300, 100, WIN_MODE_TILED);
    WinAddMenuItem(POS_CENTERED, 10, w, 3, "ABCD", 1);
    WinDraw(w);
    CHECK_EQ_INT(g_windows[w].menuItems[0].x, 150 - (4 * 8 + 16) / 2);
    CHECK_EQ_INT(g_windows[w].menuItems[0].hilite, 1);
}

TEST(ui_WinDraw_tiled_frame_blits)
{
    Screen();
    WinInit();
    g_gfxLogos = ImgLoad("logos", false, true);
    int w = WinOpen(100, 100, 128, 96, WIN_MODE_TILED);
    WinDraw(w);
    // 3x2 inner tiles, 2x6 top/bottom edges, 2x4 side edges, 4 corners
    CHECK_EQ_INT(g_fake.blits, 6 + 12 + 8 + 4);
}

TEST(ui_WinDraw_menu_item_blits_one_tile_per_letter_and_two_caps)
{
    Screen();
    WinInit();
    g_gfxLogos = ImgLoad("logos", false, true);
    g_tinyFont = NULL;
    int w = WinOpen(100, 100, 300, 100, 0);
    WinAddMenuItem(10, 10, w, 3, "HELLO", 1);
    WinAddMenuItem(10, 40, w, 4, "AB", 2);
    WinDraw(w);
    CHECK_EQ_INT(g_fake.blits, (5 + 2) + (2 + 2));
}

TEST(ui_WinDraw_toggles_blit_one_box_each)
{
    Screen();
    WinInit();
    g_gfxLogos = ImgLoad("logos", false, true);
    int w = WinOpen(100, 100, 300, 100, 0);
    WinAddToggle(10, 10, w, true, "A", false);
    WinAddToggle(30, 10, w, false, "B", false);
    WinAddToggle(50, 10, w, false, "C", false);
    WinDraw(w);
    CHECK_EQ_INT(g_fake.blits, 3);
}

TEST(ui_WinDraw_texts_draw_glyphs)
{
    Screen();
    WinInit();
    g_tinyFont = ImgLoad("tiny", false, true);
    int w = WinOpen(100, 100, 300, 100, 0);
    WinAddText(10, 10, w, "AB C", 1);       // tiny font: 3 glyphs
    WinAddText(10, 30, w, "XYZW", 5);       // news font (same image): 4 glyphs
    WinDraw(w);
    CHECK_EQ_INT(g_fake.blits, 7);
}

TEST(ui_WinDraw_masked_edit_shows_stars)
{
    Screen();
    WinInit();
    g_tinyFont = ImgLoad("tiny", false, true);
    int w = WinOpen(100, 100, 300, 100, 0);
    WinAddEdit(10, 10, w, 6, 1, 1, 0);
    strcpy(g_windows[w].edits[0].buf, "PWD");
    g_windows[w].edits[0].cursor = 3;
    WinDraw(w);
    // 3 underscores after the cursor, 3 stars, no cursor mark (not blinking yet)
    CHECK_EQ_INT(g_fake.blits, 6);
}

TEST(ui_WinDraw_blink_toggles_every_26_draws)
{
    Screen();
    WinInit();
    int w = WinOpen(100, 100, 300, 100, 0);
    for (int i = 0; i < 25; i++)
        WinDraw(w);
    CHECK_EQ_INT(g_windows[w].blinkOn, 0);
    WinDraw(w);
    CHECK_EQ_INT(g_windows[w].blinkOn, 1);
    CHECK_NEAR(g_windows[w].blinkTimer, 25.0, 0);
    for (int i = 0; i < 26; i++)
        WinDraw(w);
    CHECK_EQ_INT(g_windows[w].blinkOn, 0);
}

TEST(ui_WinDraw_sliding_window_slides_in_to_zero)
{
    Screen();
    WinInit();
    int w = WinOpen(100, 100, 300, 100, WIN_MODE_SLIDING);
    WinDraw(w);
    CHECK_NEAR(g_windows[w].slideX, -700.0 * 0.85, 0.01);
    WinDraw(w);
    CHECK_NEAR(g_windows[w].slideX, -700.0 * 0.85 * 0.85, 0.01);
    for (int i = 0; i < 60; i++)
        WinDraw(w);
    CHECK_NEAR(g_windows[w].slideX, 0.0, 0);
    CHECK_EQ_INT(g_windows[w].active, 1);
}

TEST(ui_WinDraw_closed_sliding_window_goes_inactive_after_28_draws)
{
    Screen();
    WinInit();
    int w = WinOpen(100, 100, 300, 100, WIN_MODE_SLIDING);
    WinClose(w);
    for (int i = 0; i < 27; i++)
        WinDraw(w);
    CHECK_EQ_INT(g_windows[w].active, 1);
    WinDraw(w);   // -5 * 1.2^28 < -750
    CHECK_EQ_INT(g_windows[w].active, 0);
}

TEST(ui_WinUpdateAll_draws_only_active_windows)
{
    Screen();
    WinInit();
    int a = WinOpen(100, 100, 300, 100, 0);
    int b = WinOpen(100, 100, 300, 100, 0);
    int c = WinOpen(100, 100, 300, 100, 0);
    WinClose(b);
    WinUpdateAll();
    CHECK_NEAR(g_windows[a].blinkTimer, 24.0, 0);
    CHECK_NEAR(g_windows[b].blinkTimer, 25.0, 0);
    CHECK_NEAR(g_windows[c].blinkTimer, 24.0, 0);
}

TEST(ui_DrawTextFrame_blits_caps_around_the_glyphs)
{
    Screen();
    g_gfxLogos = ImgLoad("logos", false, true);
    g_blitCount = 0;
    DrawTextFrame("AB@S001C@E", 100, 50, 1);   // 3 glyphs, the link escapes take no tile
    CHECK_EQ_INT(g_blitCount, 5);
    CHECK_NEAR(g_blit[0].destX, 92, 0);
    CHECK_NEAR(g_blit[0].destY, 48, 0);
    CHECK_EQ_INT(g_blit[0].src.x1, 0);
    CHECK_EQ_INT(g_blit[1].src.x1, 8);
    CHECK_EQ_INT(g_blit[4].src.x1, 16);
    CHECK_NEAR(g_blit[4].destX, 92 + 4 * 8, 0);
}

TEST(ui_DrawTextBox_wraps_at_the_last_space)
{
    Screen();
    g_tinyFont = ImgLoad("tiny", false, true);
    g_gfxLogos = NULL;
    // inner width (140 - 20 - 20) / 8 = 12 characters per line
    g_blitCount = 0;
    DrawTextBox(0, 0, 140, 200, "AAAA BBBB CCCC DD");
    int second = -1;
    for (int i = 0; i < g_blitCount; i++)
        if (g_blit[i].graphic == g_tinyFont && g_blit[i].destY == 29 && second < 0)
            second = i;
    CHECK(second >= 0);
    CHECK_NEAR(g_blit[second].destX, 20, 0);   // "CCCC DD" starts at the margin
    FlushBlit(0);
    // all 14 letters are drawn (the spaces aren't); line two starts at y = 20 + 9
    CHECK_EQ_INT(g_fake.blits, 14);
    CHECK_EQ_INT(g_curY, 29);
}

TEST(ui_DrawFrame_blits_tiles_and_centers_title)
{
    Screen();
    g_gfxLogos = ImgLoad("logos", false, true);
    g_tinyFont = NULL;
    g_blitCount = 0;
    DrawFrame(0, 0, 64, 64, "AB");
    // (64-32)/16 = 2: 2x2 fill, 2x2 top/bottom, 2x2 sides, 4 corners
    CHECK_EQ_INT(g_blitCount, 4 + 4 + 4 + 4 + 2);   // + the title's 2 glyphs
    CHECK_EQ_INT(g_textStartX, 32 - 8);
}

// ---------------------------------------------------------------------------------------
// MenuUpdate: popup windows with the keyboard and the mouse (booted game; the first-run
// "PRESETS" window is open: VERY OLD PC / OLD PC / NORMAL PC / VERY POWERFUL PC / CLOSE)
// ---------------------------------------------------------------------------------------

static int CheckedItem(int w)
{
    int sel = -1;
    for (int k = 0; k <= g_windows[w].nF; k++)
        if (g_windows[w].menuItems[k].checked)
            sel = k;
    return sel;
}

TEST(ui_Boot_opens_the_presets_window)
{
    BootGame();
    CHECK_EQ_INT(g_windows[0].active, 1);
    CHECK_EQ_INT(g_windows[0].nF, 4);
    CHECK_EQ_INT(g_windows[0].menuItems[0].id, MENUID_QUALITY_PRESET_LOW);
    CHECK_EQ_INT(g_windows[0].menuItems[4].id, MENUID_DIALOG_DISMISS_B);
    CHECK_EQ_INT(g_windows[0].selF, MENUID_DIALOG_DISMISS_B);
    CHECK_EQ_INT(FakePlayCount("maximize"), 1);
}

TEST(ui_MenuUpdate_down_key_walks_the_items_and_wraps)
{
    BootGame();
    CHECK_EQ_INT(CheckedItem(0), -1);
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(CheckedItem(0), 0);
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(CheckedItem(0), 1);
    CHECK_EQ_INT(g_windows[0].menuItems[0].checked, 0);
    TapKey(K_VK_DOWN, 1);
    TapKey(K_VK_DOWN, 1);
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(CheckedItem(0), 4);
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(CheckedItem(0), 0);
    CHECK_EQ_INT(FakePlayCount("tast"), 6);
}

TEST(ui_MenuUpdate_up_key_wraps_to_the_last_item)
{
    BootGame();
    TapKey(K_VK_UP, 1);
    CHECK_EQ_INT(CheckedItem(0), 0);
    TapKey(K_VK_UP, 1);
    CHECK_EQ_INT(CheckedItem(0), 4);
    TapKey(K_VK_UP, 1);
    CHECK_EQ_INT(CheckedItem(0), 3);
    CHECK_EQ_INT(g_windows[0].menuItems[4].checked, 0);
}

TEST(ui_MenuUpdate_holding_down_moves_once)
{
    BootGame();
    FakePressKey(K_VK_DOWN);
    RunFrames(10);
    CHECK_EQ_INT(CheckedItem(0), 0);
    FakeReleaseKey(K_VK_DOWN);
    RunFrames(1);
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(CheckedItem(0), 1);
}

TEST(ui_MenuUpdate_return_without_a_check_activates_the_selected_item)
{
    BootGame();
    int before = FakePlayCount("minimize");
    TapKey(K_VK_RETURN, 1);
    // CLOSE (4999): the sliding window is closed (it slides out)
    CHECK_EQ_INT(g_windows[0].visible, 0);
    CHECK_NEAR(g_windows[0].slideY, 1.2, 1e-6);
    CHECK_EQ_INT(g_windows[0].nF, -1);
    CHECK_EQ_INT(FakePlayCount("minimize"), before + 1);
    RunFrames(30);
    CHECK(!AnyWindowActive());
}

TEST(ui_MenuUpdate_return_activates_the_checked_item)
{
    BootGame();
    TapKey(K_VK_DOWN, 1);
    TapKey(K_VK_DOWN, 1);
    TapKey(K_VK_RETURN, 1);
    // OLD PC: the medium preset, then the "create a profile?" window
    CHECK_NEAR(g_cfg.numStars, 300.0, 0);
    CHECK_EQ_INT(g_windows[1].active, 1);
    CHECK_STR(g_windows[1].texts[0].text, "CREATE A NEW PROFILE");
    CHECK_EQ_INT(g_windows[1].selF, MENUID_CREATE_PROFILE_YES);
}

TEST(ui_MenuUpdate_keys_drive_the_topmost_window)
{
    BootGame();
    TapKey(K_VK_DOWN, 1);
    TapKey(K_VK_RETURN, 1);   // VERY OLD PC: window 1 asks to create a profile
    CHECK_EQ_INT(g_windows[1].active, 1);
    CHECK_EQ_INT(g_windows[0].active, 1);   // still sliding out
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(CheckedItem(1), 0);
    TapKey(K_VK_DOWN, 1);
    CHECK_EQ_INT(CheckedItem(1), 1);
    TapKey(K_VK_UP, 1);
    TapKey(K_VK_RETURN, 1);   // "NO": everything closes
    CHECK_EQ_INT(g_windows[1].visible, 0);
    RunFrames(30);
    CHECK(!AnyWindowActive());
}

static void WinItemCenter(int w, int item, int *x, int *y)
{
    *x = g_windows[w].x + g_windows[w].menuItems[item].x + g_windows[w].menuItems[item].w / 2;
    *y = g_windows[w].y + g_windows[w].menuItems[item].y + g_windows[w].menuItems[item].h / 2;
}

static void Click(int x, int y)
{
    g_fake.mouseX = x;
    g_fake.mouseY = y;
    RunFrames(1);
    g_fake.mouseLeft = true;
    RunFrames(1);
    g_fake.mouseLeft = false;
    RunFrames(1);
}

TEST(ui_MenuUpdate_mouse_click_on_an_item_activates_it)
{
    BootGame();
    RunFrames(40);   // slid in, items centered
    int x, y;
    WinItemCenter(0, 3, &x, &y);   // VERY POWERFUL PC
    Click(x, y);
    CHECK_EQ_INT(FakePlayCount("buttonclick"), 1);
    CHECK_NEAR(g_cfg.numStars, 1000.0, 0);
    CHECK_EQ_INT(g_cfg.bulletIntensity, BULLETS_FLARE_FX);
}

TEST(ui_MenuUpdate_hover_marks_the_item_under_the_mouse)
{
    BootGame();
    RunFrames(40);
    int x, y;
    WinItemCenter(0, 2, &x, &y);
    g_fake.mouseX = x;
    g_fake.mouseY = y;
    RunFrames(1);
    CHECK_EQ_INT(g_windows[0].menuItems[2].state, 1);
    CHECK_EQ_INT(g_windows[0].menuItems[1].state, 0);
    g_fake.mouseX = 5;
    g_fake.mouseY = 5;
    RunFrames(1);
    CHECK_EQ_INT(g_windows[0].menuItems[2].state, 0);
}

TEST(ui_MenuUpdate_release_off_the_pressed_item_does_nothing)
{
    BootGame();
    RunFrames(40);
    int x, y, x2, y2;
    WinItemCenter(0, 3, &x, &y);
    WinItemCenter(0, 0, &x2, &y2);
    float stars = g_cfg.numStars;
    g_fake.mouseX = x;
    g_fake.mouseY = y;
    g_fake.mouseLeft = true;
    RunFrames(1);
    g_fake.mouseX = x2;
    g_fake.mouseY = y2;
    g_fake.mouseLeft = false;
    RunFrames(2);
    CHECK_EQ_INT(FakePlayCount("buttonclick"), 0);
    CHECK_EQ_INT(g_windows[0].visible, 1);
    CHECK_NEAR(g_cfg.numStars, stars, 0);
}

TEST(ui_MenuUpdate_mouse_drag_moves_nothing_but_arms_drag)
{
    BootGame();
    RunFrames(40);
    // press on the window body, away from the items
    g_fake.mouseX = g_windows[0].x + 5;
    g_fake.mouseY = g_windows[0].y + 5;
    g_fake.mouseLeft = true;
    RunFrames(1);
    CHECK_EQ_INT(g_dragWin, 0);
    CHECK_EQ_INT(g_dragDX, 5);
    CHECK_EQ_INT(g_dragDY, 5);
    g_fake.mouseLeft = false;
    RunFrames(1);
    CHECK_EQ_INT(g_dragWin, -1);
}
