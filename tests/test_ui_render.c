// Tests for src/gfx/render.c: the blit/stretch/quad queues, their flushes (draws counted by the
// fake engine), clipped blits, and the screen clears.
#include "support.h"

static void Screen(void)
{
    g_screenW = 800;
    g_screenH = 600;
    ResetClip();
    CLEAR_DRAW_COUNTERS();
    g_stretchICount = 0;
}

static Rect16 R(int x1, int y1, int x2, int y2)
{
    Rect16 r;
    r.x1 = x1;
    r.y1 = y1;
    r.x2 = x2;
    r.y2 = y2;
    return r;
}

// ---- QueueBlit / FlushBlit ----

TEST(ui_QueueBlit_copies_the_rect_and_position)
{
    Screen();
    Image *img = ImgLoad("a", false, true);
    Rect16 r = R(3, 4, 13, 24);
    QueueBlit(5.5f, 6.5f, img, &r);
    CHECK_EQ_INT(g_blitCount, 1);
    CHECK_EQ_INT(g_blit[0].src.x1, 3);
    CHECK_EQ_INT(g_blit[0].src.y1, 4);
    CHECK_EQ_INT(g_blit[0].src.x2, 13);
    CHECK_EQ_INT(g_blit[0].src.y2, 24);
    CHECK_NEAR(g_blit[0].destX, 5.5, 0);
    CHECK_NEAR(g_blit[0].destY, 6.5, 0);
    CHECK(g_blit[0].graphic == img);
}

TEST(ui_QueueBlit_rejects_bad_source_rects)
{
    Screen();
    g_blitErrors = 0;
    Rect16 empty = R(5, 0, 5, 10), neg = R(-1, 0, 5, 10), wide = R(0, 0, 2001, 10);
    Rect16 edge = R(0, 0, 2000, 10);
    QueueBlit(0, 0, NULL, &empty);
    QueueBlit(0, 0, NULL, &neg);
    QueueBlit(0, 0, NULL, &wide);
    CHECK_EQ_INT(g_blitCount, 0);
    CHECK_EQ_INT(g_blitErrors, 3);
    QueueBlit(0, 0, NULL, &edge);
    CHECK_EQ_INT(g_blitCount, 1);
    CHECK_EQ_INT(g_blitErrors, 3);
    QueueBlit2(0, 0, NULL, &neg);
    QueueBlit2(0, 0, NULL, &empty);
    QueueBlit2(0, 0, NULL, &wide);
    CHECK_EQ_INT(g_blit2Count, 0);
    CHECK_EQ_INT(g_blitErrors, 6);
}

TEST(ui_QueueBlit_holds_1999_entries)
{
    Screen();
    Rect16 r = R(0, 0, 8, 8);
    for (int i = 0; i < 2100; i++) {
        QueueBlit(i, 0, NULL, &r);
        QueueBlit2(i, 0, NULL, &r);
    }
    CHECK_EQ_INT(g_blitCount, MAX_BLITS - 1);
    CHECK_EQ_INT(g_blit2Count, MAX_BLITS - 1);
    CHECK_NEAR(g_blit[1998].destX, 1998, 0);
}

TEST(ui_FlushBlit_draws_live_images_and_empties_the_queue)
{
    Screen();
    Image *a = ImgLoad("a", false, true), *b = ImgLoad("b", false, true);
    Rect16 r = R(0, 0, 8, 8), flat = R(0, 5, 8, 5);
    QueueBlit(0, 0, a, &r);
    QueueBlit(0, 0, b, &r);
    QueueBlit(0, 0, NULL, &r);
    QueueBlit(0, 0, a, &flat);   // zero height: skipped at flush
    QueueBlit(0, 0, a, &r);
    ImgFree(b);
    FlushBlit(0);
    CHECK_EQ_INT(g_fake.blits, 2);
    CHECK_EQ_INT(g_blitCount, 0);
    FlushBlit(0);
    CHECK_EQ_INT(g_fake.blits, 2);
}

TEST(ui_FlushBlit2_draws_the_second_queue)
{
    Screen();
    Image *a = ImgLoad("a", false, true);
    Rect16 r = R(0, 0, 8, 8);
    QueueBlit2(0, 0, a, &r);
    QueueBlit2(0, 0, a, &r);
    QueueBlit(0, 0, a, &r);
    FlushBlit2(0);
    CHECK_EQ_INT(g_fake.blits, 2);
    CHECK_EQ_INT(g_blit2Count, 0);
    CHECK_EQ_INT(g_blitCount, 1);
}

// ---- the stretch queues ----

TEST(ui_QueueStretchI_stores_an_integer_rect)
{
    Screen();
    Image *a = ImgLoad("a", false, true);
    QueueStretchI(a, 1.7f, 2.2f, 30.9f, 40.1f, 10, 20, 30, 40);
    CHECK_EQ_INT(g_stretchICount, 1);
    CHECK_EQ_INT(g_stretchI[0].x1, 1);
    CHECK_EQ_INT(g_stretchI[0].x2, 30);
    CHECK_EQ_INT(g_stretchI[0].y2, 40);
    CHECK_EQ_INT(g_stretchI[0].g, 20);
    CHECK_EQ_INT(g_stretchI[0].a, 40);
}

TEST(ui_stretch_queues_have_their_caps)
{
    Screen();
    for (int i = 0; i < 1600; i++) {
        QueueStretchI(NULL, 0, 0, 1, 1, 0, 0, 0, 0);
        QueueStretchF(NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0);
        QueueStretchRot(NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
        QueueStretchRot2(NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
    }
    // NOTE: QueueQuad accepts 149 entries but g_quad[] has 100 (as in the original), so
    // only fill what fits.
    for (int i = 0; i < 100; i++)
        QueueQuad(NULL, 0, 0, 1, 1, NULL, 0, 0, 1, 1);
    CHECK_EQ_INT(g_stretchICount, MAX_STRETCH_I - 1);
    CHECK_EQ_INT(g_stretchFCount, MAX_STRETCH_F - 1);
    CHECK_EQ_INT(g_stretchRotCount, MAX_STRETCH_ROT - 1);
    CHECK_EQ_INT(g_stretchRot2Count, MAX_STRETCH_ROT2 - 1);
    CHECK_EQ_INT(g_quadCount, 100);
    CHECK_EQ_INT(g_quadCountMax, 100);
}

TEST(ui_stretch_flushes_draw_live_images)
{
    Screen();
    Image *a = ImgLoad("a", false, true), *dead = ImgLoad("d", false, true);
    ImgFree(dead);
    QueueStretchI(a, 0, 0, 1, 1, 0, 0, 0, 0);
    QueueStretchI(dead, 0, 0, 1, 1, 0, 0, 0, 0);
    QueueStretchF(a, 0, 0, 1, 1, 0, 0, 0, 0, 0);
    QueueStretchF(a, 0, 0, 1, 1, 0, 0, 0, 0, 0);
    QueueStretchF(NULL, 0, 0, 1, 1, 0, 0, 0, 0, 0);
    QueueStretchRot(a, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
    QueueStretchRot(dead, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
    QueueStretchRot2(a, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
    QueueStretchRot2(a, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
    QueueStretchRot2(dead, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0);
    FlushStretchI();
    CHECK_EQ_INT(g_fake.blits, 1);
    CHECK_EQ_INT(g_stretchICount, 0);
    FlushStretchF();
    CHECK_EQ_INT(g_fake.blits, 3);
    CHECK_EQ_INT(g_stretchFCount, 0);
    FlushStretchRot();
    CHECK_EQ_INT(g_fake.blits, 4);
    CHECK_EQ_INT(g_stretchRotCount, 0);
    FlushStretchRot2();
    CHECK_EQ_INT(g_fake.blits, 6);
    CHECK_EQ_INT(g_stretchRot2Count, 0);
}

TEST(ui_QueueQuad_and_FlushQuads)
{
    Screen();
    Image *a = ImgLoad("a", false, true);
    g_quadCountMax = 0;
    QueueQuad(NULL, 1, 2, 3, 4, a, 5, 6, 7, 8);
    QueueQuad(NULL, 1, 2, 3, 4, NULL, 5, 6, 7, 8);
    QueueQuad(NULL, 1, 2, 3, 4, a, 5, 6, 7, 8);
    CHECK_EQ_INT(g_quadCount, 3);
    CHECK_EQ_INT(g_quadCountMax, 3);
    CHECK_NEAR(g_quad[0].dw, 3, 0);
    CHECK_NEAR(g_quad[0].sx, 5, 0);
    CHECK_NEAR(g_quad[0].sh, 8, 0);
    FlushQuads(0);
    CHECK_EQ_INT(g_fake.blits, 2);
    CHECK_EQ_INT(g_quadCount, 0);
    QueueQuad(NULL, 1, 2, 3, 4, a, 5, 6, 7, 8);
    CHECK_EQ_INT(g_quadCountMax, 3);
}

// ---- clipped blits ----

TEST(ui_Blit_clips_left_and_top)
{
    Screen();
    Blit(-10, -4, NULL, NULL, 100, 50, 32, 16);
    CHECK_EQ_INT(g_blitCount, 1);
    CHECK_NEAR(g_blit[0].destX, 0, 0);
    CHECK_NEAR(g_blit[0].destY, 0, 0);
    CHECK_EQ_INT(g_blit[0].src.x1, 110);
    CHECK_EQ_INT(g_blit[0].src.x2, 132);
    CHECK_EQ_INT(g_blit[0].src.y1, 54);
    CHECK_EQ_INT(g_blit[0].src.y2, 66);
}

TEST(ui_Blit_clips_right_and_bottom)
{
    Screen();
    Blit(790, 590, NULL, NULL, 100, 50, 32, 16);
    CHECK_EQ_INT(g_blitCount, 1);
    CHECK_EQ_INT(g_blit[0].src.x1, 100);
    CHECK_EQ_INT(g_blit[0].src.x2, 110);
    CHECK_EQ_INT(g_blit[0].src.y2, 60);
}

TEST(ui_Blit_skips_what_is_off_the_clip_rect)
{
    Screen();
    g_clipLeft = 100;
    g_clipRight = 200;
    g_clipTop = 100;
    g_clipBottom = 200;
    g_blitErrors = 0;
    Blit(68, 150, NULL, NULL, 0, 0, 32, 16);    // ends at the left edge
    Blit(200, 150, NULL, NULL, 0, 0, 32, 16);   // starts at the right edge
    Blit(150, 84, NULL, NULL, 0, 0, 32, 16);
    Blit(150, 200, NULL, NULL, 0, 0, 32, 16);
    CHECK_EQ_INT(g_blitCount, 0);
    CHECK_EQ_INT(g_blitErrors, 0);   // skipped, not clipped to an empty rect
    Blit(69, 150, NULL, NULL, 0, 0, 32, 16);
    CHECK_EQ_INT(g_blitCount, 1);
    CHECK_EQ_INT(g_blit[0].src.x1, 31);
}

TEST(ui_Blit2_BlitLocal_BlitLocal2_pick_their_queues)
{
    Screen();
    Blit2(-5, 10, NULL, NULL, 0, 0, 16, 16);
    BlitLocal(-5, 10, NULL, NULL, 0, 0, 16, 16);
    BlitLocal2(10, -6, NULL, NULL, 0, 0, 16, 16);
    BlitLocal(900, 10, NULL, NULL, 0, 0, 16, 16);
    BlitLocal2(10, 900, NULL, NULL, 0, 0, 16, 16);
    CHECK_EQ_INT(g_blitCount, 1);
    CHECK_EQ_INT(g_blit2Count, 2);
    CHECK_EQ_INT(g_blit2[0].src.x1, 5);
    CHECK_EQ_INT(g_blit[0].src.x1, 5);
    CHECK_EQ_INT(g_blit[0].src.x2, 16);
    CHECK_EQ_INT(g_blit2[1].src.y1, 6);
    CHECK_NEAR(g_blit2[1].destY, 0, 0);
}

// ---- immediate draws ----

TEST(ui_DrawImage_stretches_and_tints)
{
    Screen();
    DrawImage(NULL, 0, 0, 10, 10, 255, 255, 255, 255, 0, 0);
    CHECK_EQ_INT(g_fake.blits, 0);
    Image *a = ImgLoad("a", false, true);
    DrawImage(a, 0, 0, 10, 10, 255, 0, 51, 128, 0, 0);
    CHECK_EQ_INT(g_fake.blits, 1);
    DrawStretch(a, 0, 0, 5, 5, 0, 0, 5, 5);
    CHECK_EQ_INT(g_fake.blits, 2);
}

TEST(ui_SetView_fades_the_flash_colour_over_30_frames)
{
    Screen();
    g_flashOverlayActive = 1;
    g_fadeStep = 0;
    for (int i = 0; i < 29; i++)
        SetView();
    CHECK_EQ_INT(g_fadeStep, 29);
    CHECK_EQ_INT(g_flashOverlayActive, 1);
    SetView();
    CHECK_EQ_INT(g_flashOverlayActive, 0);
    CHECK_EQ_INT(g_fake.rects, 30);
    SetView();
    CHECK_EQ_INT(g_fadeStep, 30);
    CHECK_EQ_INT(g_fake.rects, 31);
}

TEST(ui_SetViewHud_skipped_with_the_background_on)
{
    Screen();
    g_flashOverlayActive = 0;
    g_cfg.bgEnabled = 1;
    SetViewHud();
    CHECK_EQ_INT(g_fake.rects, 0);
    g_cfg.bgEnabled = 0;
    SetViewHud();
    CHECK_EQ_INT(g_fake.rects, 1);
    g_flashOverlayActive = 1;
    g_fadeStep = 28;
    SetViewHud();
    SetViewHud();
    CHECK_EQ_INT(g_flashOverlayActive, 0);
    CHECK_EQ_INT(g_fake.rects, 3);
}

TEST(ui_ResetClip_and_counters)
{
    g_screenW = 640;
    g_screenH = 480;
    g_clipLeft = 5;
    g_clipTop = 6;
    CHECK(ResetClip());
    CHECK_EQ_INT(g_clipLeft, 0);
    CHECK_EQ_INT(g_clipTop, 0);
    CHECK_EQ_INT(g_clipRight, 640);
    CHECK_EQ_INT(g_clipBottom, 480);
    g_blitCount = g_blit2Count = g_quadCount = g_stretchFCount = g_stretchRotCount = 3;
    g_stretchRot2Count = 3;
    ResetF893();
    CHECK_EQ_INT(g_blitCount + g_blit2Count + g_quadCount + g_stretchFCount + g_stretchRotCount, 0);
    CHECK_EQ_INT(g_stretchRot2Count, 3);
    ResetBlitCounters();
    CHECK_EQ_INT(g_stretchRot2Count, 0);
    FlipBuffer(0);
    CHECK_EQ_INT(g_fake.flips, 1);
}
