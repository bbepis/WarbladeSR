// engine_img.c: Pictures and drawing in the real engine (src/core/sdl.c): ImgLoad, the blits,
// tints, the primitives and the back buffer, read back from the render target.
#include "engine_util.h"

static char s_dir[512];

// The image's fields (struct Image in sdl.c), to check what the software renderer can't show.
typedef struct ImageFields {
    SDL_Texture *tex;
    float w, h;
    bool freed;
    unsigned freedFrame;
    Image *nextFreed;
    bool tinted;
    float r, g, b, a;
    short mode;
    bool linear;
} ImageFields;

static const ImageFields *Fields(Image *img)
{
    const ImageFields *f = (const ImageFields *)img;
    CHECK_MSG(f->w == ImgWidth(img) && f->h == ImgHeight(img), "struct Image changed");
    return f;
}

enum { RED_ = 0xffff0000, GREEN_ = 0xff00ff00, BLUE_ = 0xff0000ff, WHITE_ = 0xffffffff };

// An 8x8 picture in four 4x4 quarters: red, green / blue, white.
static Image *LoadQuads(void)
{
    Uint32 px[64];
    char p[1024];
    Image *img;
    int x, y;

    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++)
            px[y * 8 + x] = y < 4 ? (x < 4 ? RED_ : GREEN_) : (x < 4 ? BLUE_ : WHITE_);
    if (!s_dir[0])
        MakeTempDir(s_dir, sizeof s_dir);
    snprintf(p, sizeof p, "%s/Quads.tga", s_dir);
    WriteTga(p, 8, 8, px);
    snprintf(p, sizeof p, "%s\\quads.TGA", s_dir);
    img = ImgLoad(p, false, true);
    CHECK(img != NULL);
    return img;
}

// The four quarters of an 8-pixel-square draw at (x, y), 2 pixels in from each corner.
static void CheckQuads(int x, int y, Uint32 tl, Uint32 tr, Uint32 bl, Uint32 br)
{
    CHECK_RGB(Rgb(x + 1, y + 1), tl, 2);
    CHECK_RGB(Rgb(x + 6, y + 1), tr, 2);
    CHECK_RGB(Rgb(x + 1, y + 6), bl, 2);
    CHECK_RGB(Rgb(x + 6, y + 6), br, 2);
}

static void Done(void)
{
    if (s_dir[0])
        RmTree(s_dir);
}

// ---------------------------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------------------------

TEST(engine_ImgLoad_reads_a_tga_and_its_size)
{
    Uint32 px[15];
    char p[1024];
    Image *img;
    int i;

    OpenWindow(64, 48);
    for (i = 0; i < 15; i++)
        px[i] = 0xff000000 | (Uint32)i * 0x101010;
    MakeTempDir(s_dir, sizeof s_dir);
    snprintf(p, sizeof p, "%s/wide.tga", s_dir);
    WriteTga(p, 5, 3, px);
    img = ImgLoad(p, true, false);
    CHECK(img != NULL);
    CHECK_EQ_INT((int)ImgWidth(img), 5);
    CHECK_EQ_INT((int)ImgHeight(img), 3);
    CHECK(!ImgIsFreed(img));
    ImgFree(img);
    Done();
}

TEST(engine_ImgLoad_reads_a_png)
{
    SDL_Surface *s;
    char p[1024];
    Image *img;

    OpenWindow(64, 48);
    MakeTempDir(s_dir, sizeof s_dir);
    s = SDL_CreateSurface(6, 2, SDL_PIXELFORMAT_ARGB8888);
    SDL_FillSurfaceRect(s, NULL, 0xff00ff00);
    SDL_Rect left = { 0, 0, 3, 2 };
    SDL_FillSurfaceRect(s, &left, 0xffff0000);
    snprintf(p, sizeof p, "%s/Pic.png", s_dir);
    CHECK(IMG_SavePNG(s, p));
    SDL_DestroySurface(s);
    snprintf(p, sizeof p, "%s/pic.PNG", s_dir);
    img = ImgLoad(p, false, true);
    CHECK(img != NULL);
    CHECK_EQ_INT((int)ImgWidth(img), 6);
    CHECK_EQ_INT((int)ImgHeight(img), 2);
    ImgBlitAlphaRect(img, 0, 0, 6, 2, 10, 10, false, false);
    CHECK_RGB(Rgb(10, 10), 0xff0000, 0);
    CHECK_RGB(Rgb(15, 11), 0x00ff00, 0);
    Done();
}

TEST(engine_ImgLoad_returns_NULL_for_missing_or_broken_files)
{
    char p[1024];
    OpenWindow(64, 48);
    MakeTempDir(s_dir, sizeof s_dir);
    snprintf(p, sizeof p, "%s/missing.tga", s_dir);
    CHECK(ImgLoad(p, false, true) == NULL);
    snprintf(p, sizeof p, "%s/broken.png", s_dir);
    WriteBytes(p, "not a picture", 13);
    CHECK(ImgLoad(p, false, true) == NULL);
    Done();
}

TEST(engine_ImgLoad_starts_with_alpha_mode_1_and_no_tint)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    CHECK_EQ_INT(Fields(img)->mode, 1);
    CHECK(!Fields(img)->tinted);
    CHECK(!Fields(img)->linear);
    CHECK(Fields(img)->r == 1.0f && Fields(img)->a == 1.0f);
    Done();
}

TEST(engine_ImgSetAlphaMode_takes_modes_0_to_6)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgSetAlphaMode(img, 0);
    CHECK_EQ_INT(Fields(img)->mode, 0);
    ImgSetAlphaMode(img, 6);
    CHECK_EQ_INT(Fields(img)->mode, 6);
    ImgSetAlphaMode(img, 7);
    CHECK_EQ_INT(Fields(img)->mode, 6);
    ImgSetAlphaMode(img, 4);
    ImgSetAlphaMode(img, -1);
    CHECK_EQ_INT(Fields(img)->mode, 4);
    Done();
}

TEST(engine_ImgFree_marks_the_image_freed)
{
    Image *a, *b;
    OpenWindow(64, 48);
    a = LoadQuads();
    b = LoadQuads();
    ImgFree(a);
    CHECK(ImgIsFreed(a));
    CHECK(!ImgIsFreed(b));
    ImgFree(a);                             // twice: nothing happens
    ImgFree(NULL);
    // Still readable while the render queues may hold it.
    SysFlip();
    CHECK(ImgIsFreed(a));
    CHECK(Fields(b)->tex != NULL);
    ImgFree(b);
    CHECK(ImgIsFreed(b));
    CHECK(Fields(b)->tex == NULL);
    Done();
}

TEST(engine_ImgFreePicture_keeps_the_object_but_draws_nothing)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgFreePicture(img);
    CHECK(!ImgIsFreed(img));
    CHECK_EQ_INT((int)ImgWidth(img), 8);
    ImgBlitAlphaRect(img, 0, 0, 8, 8, 10, 10, false, false);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 10, 10, 0, 1, 1, false, false, 0, 0);
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 10, 10, 18, 18, 1, 0, false, false, 0, 0);
    CHECK_RGB(Rgb(11, 11), 0, 0);
    ImgFreePicture(img);                    // twice
    ImgFree(img);
    Done();
}

// ---------------------------------------------------------------------------------------------
// Blits
// ---------------------------------------------------------------------------------------------

TEST(engine_ImgBlitAlphaRect_copies_the_picture)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgBlitAlphaRect(img, 0, 0, 8, 8, 10, 20, false, false);
    CheckQuads(10, 20, 0xff0000, 0x00ff00, 0x0000ff, 0xffffff);
    // Exactly 8x8.
    CHECK_RGB(Rgb(10, 20), 0xff0000, 0);
    CHECK_RGB(Rgb(17, 27), 0xffffff, 0);
    CHECK_RGB(Rgb(18, 27), 0, 0);
    CHECK_RGB(Rgb(17, 28), 0, 0);
    CHECK_RGB(Rgb(9, 20), 0, 0);
    CHECK_RGB(Rgb(10, 19), 0, 0);
    Done();
}

TEST(engine_ImgBlitAlphaRect_draws_a_source_rect)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    // The right half, 4x8, at (30, 5).
    ImgBlitAlphaRect(img, 4, 0, 8, 8, 30, 5, false, false);
    CHECK_RGB(Rgb(30, 5), 0x00ff00, 0);
    CHECK_RGB(Rgb(33, 12), 0xffffff, 0);
    CHECK_RGB(Rgb(34, 5), 0, 0);
    CHECK_RGB(Rgb(30, 13), 0, 0);
    Done();
}

TEST(engine_ImgBlitAlphaRect_mirrors_a_backwards_rect)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgBlitAlphaRect(img, 8, 0, 0, 8, 0, 0, false, false);
    CheckQuads(0, 0, 0x00ff00, 0xff0000, 0xffffff, 0x0000ff);
    ImgBlitAlphaRect(img, 0, 8, 8, 0, 20, 0, false, false);
    CheckQuads(20, 0, 0x0000ff, 0xffffff, 0xff0000, 0x00ff00);
    Done();
}

TEST(engine_ImgBlitAlphaRect_flips)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgBlitAlphaRect(img, 0, 0, 8, 8, 0, 0, true, false);
    CheckQuads(0, 0, 0x00ff00, 0xff0000, 0xffffff, 0x0000ff);
    ImgBlitAlphaRect(img, 0, 0, 8, 8, 10, 0, false, true);
    CheckQuads(10, 0, 0x0000ff, 0xffffff, 0xff0000, 0x00ff00);
    // A flip of a backwards rect turns it back.
    ImgBlitAlphaRect(img, 8, 0, 0, 8, 20, 0, true, false);
    CheckQuads(20, 0, 0xff0000, 0x00ff00, 0x0000ff, 0xffffff);
    Done();
}

TEST(engine_ImgBlitRectF_copies_the_picture)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgBlitRectF(img, 0, 0, 8, 8, 40, 30, false, false);
    CheckQuads(40, 30, 0xff0000, 0x00ff00, 0x0000ff, 0xffffff);
    CHECK_RGB(Rgb(48, 30), 0, 0);
    ImgBlitRectF(img, 0, 0, 8, 8, 20, 30, true, true);
    CheckQuads(20, 30, 0xffffff, 0x0000ff, 0x00ff00, 0xff0000);
    Done();
}

TEST(engine_ImgSetBlitColor_tints_the_colour)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgSetBlitColor(img, 0.5f, 1, 0.25f, 1);
    ImgBlitAlphaRect(img, 0, 0, 8, 8, 0, 0, false, false);
    CheckQuads(0, 0, 0x800000, 0x00ff00, 0x000040, 0x80ff40);
    // Back to no tint.
    ImgSetBlitColor(img, 1, 1, 1, 1);
    CHECK(!Fields(img)->tinted);
    ImgBlitAlphaRect(img, 0, 0, 8, 8, 10, 0, false, false);
    CheckQuads(10, 0, 0xff0000, 0x00ff00, 0x0000ff, 0xffffff);
    Done();
}

TEST(engine_ImgSetBlitColor_alpha_blends)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    DrawRect(0, 0, 64, 48, 1, 1, 1, 1);
    ImgSetBlitColor(img, 1, 1, 1, 0.5f);
    ImgBlitAlphaRect(img, 0, 0, 8, 8, 0, 0, false, false);
    CheckQuads(0, 0, 0xff8080, 0x80ff80, 0x8080ff, 0xffffff);
    ImgBlitRectF(img, 0, 0, 8, 8, 10, 0, false, false);
    CheckQuads(10, 0, 0xff8080, 0x80ff80, 0x8080ff, 0xffffff);
    Done();
}

TEST(engine_ImgSetBlitColor_clamps_above_1)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    DrawRect(0, 0, 64, 48, 1, 1, 1, 1);
    // An alpha of 3 is 1: times the blend, half.
    ImgSetBlitColor(img, 2, 1, 1, 3);
    CHECK(Fields(img)->r == 1.0f && Fields(img)->a == 1.0f);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 0, 0, 0, 1, 0.5f, false, false, 0, 0);
    CheckQuads(0, 0, 0xff8080, 0x80ff80, 0x8080ff, 0xffffff);
    Done();
}

TEST(engine_ImgSetTextureQuality_filters_scaled_blits)
{
    Image *img;
    Uint32 c;
    OpenWindow(64, 48);
    img = LoadQuads();
    // Zoom 2: the red/green edge is at x = 8 of the 16 pixels.
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 4, 4, 0, 2, 1, false, false, 0, 0);
    CHECK_RGB(Rgb(7, 4), 0xff0000, 0);
    CHECK_RGB(Rgb(8, 4), 0x00ff00, 0);
    ImgSetTextureQuality(img, true);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 4, 24, 0, 2, 1, false, false, 0, 0);
    c = Rgb(7, 26);
    CHECK_MSG(RED(c) > 20 && RED(c) < 235 && GREEN(c) > 20 && GREEN(c) < 235,
              "linear edge pixel %06x", (unsigned)c);
    ImgSetTextureQuality(img, false);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 40, 24, 0, 2, 1, false, false, 0, 0);
    CHECK_RGB(Rgb(43, 26), 0xff0000, 0);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_zooms_about_the_centre)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    // Zoom 2 at (20, 20): 16x16 from (16, 16).
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 0, 2, 1, false, false, 0, 0);
    CHECK_RGB(Rgb(16, 16), 0xff0000, 0);
    CHECK_RGB(Rgb(23, 16), 0xff0000, 0);
    CHECK_RGB(Rgb(24, 16), 0x00ff00, 0);
    CHECK_RGB(Rgb(31, 31), 0xffffff, 0);
    CHECK_RGB(Rgb(15, 16), 0, 0);
    CHECK_RGB(Rgb(32, 31), 0, 0);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_zooms_about_an_offset_centre)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    // About the bottom-right corner (centre + (4, 4)): the 16x16 ends at (28, 28).
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 0, 2, 1, false, false, 4, 4);
    CHECK_RGB(Rgb(12, 12), 0xff0000, 0);
    CHECK_RGB(Rgb(27, 27), 0xffffff, 0);
    CHECK_RGB(Rgb(28, 27), 0, 0);
    CHECK_RGB(Rgb(11, 12), 0, 0);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_zooms_and_rotates_about_the_same_centre)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    // Zoom 2 about the right edge's middle: x 12-27, y 16-31; then a half turn about that
    // point (28, 24): x 28-43.
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 180, 2, 1, false, false, 4, 0);
    CHECK_RGB(Rgb(29, 17), 0xffffff, 0);
    CHECK_RGB(Rgb(42, 17), 0x0000ff, 0);
    CHECK_RGB(Rgb(29, 30), 0x00ff00, 0);
    CHECK_RGB(Rgb(42, 30), 0xff0000, 0);
    CHECK_RGB(Rgb(26, 24), 0, 0);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_rotates_counter_clockwise)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 90, 1, 1, false, false, 0, 0);
    CheckQuads(20, 20, 0x00ff00, 0xffffff, 0xff0000, 0x0000ff);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 40, 20, -90, 1, 1, false, false, 0, 0);
    CheckQuads(40, 20, 0x0000ff, 0xff0000, 0xffffff, 0x00ff00);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_rotates_about_an_offset_centre)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    // A half turn about the right edge's middle (centre + (4, 0)): x 20-27 goes to 28-35.
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 180, 1, 1, false, false, 4, 0);
    CheckQuads(28, 20, 0xffffff, 0x0000ff, 0x00ff00, 0xff0000);
    CHECK_RGB(Rgb(26, 24), 0, 0);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_negative_zoom_turns_half_way)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 0, -1, 1, false, false, 0, 0);
    CheckQuads(20, 20, 0xffffff, 0x0000ff, 0x00ff00, 0xff0000);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_blends_by_the_blend)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    DrawRect(0, 0, 64, 48, 1, 1, 1, 1);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 0, 1, 0.5f, false, false, 0, 0);
    CheckQuads(20, 20, 0xff8080, 0x80ff80, 0x8080ff, 0xffffff);
    // Times the tint's alpha: a quarter.
    ImgSetBlitColor(img, 1, 1, 1, 0.5f);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 40, 20, 0, 1, 0.5f, false, false, 0, 0);
    CheckQuads(40, 20, 0xffbfbf, 0xbfffbf, 0xbfbfff, 0xffffff);
    // Nothing at zoom 0 or a blend of 0 or less.
    ImgSetBlitColor(img, 1, 1, 1, 1);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 30, 0, 0, 1, false, false, 0, 0);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 30, 0, 1, 0, false, false, 0, 0);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 30, 0, 1, -1, false, false, 0, 0);
    CheckQuads(20, 30, 0xffffff, 0xffffff, 0xffffff, 0xffffff);
    Done();
}

TEST(engine_ImgBlitAlphaRectFx_flips)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 20, 20, 0, 1, 1, true, false, 0, 0);
    CheckQuads(20, 20, 0x00ff00, 0xff0000, 0xffffff, 0x0000ff);
    ImgBlitAlphaRectFx(img, 0, 0, 8, 8, 40, 20, 0, 1, 1, false, true, 0, 0);
    CheckQuads(40, 20, 0x0000ff, 0xffffff, 0xff0000, 0x00ff00);
    Done();
}

TEST(engine_ImgStretchAlphaRect_stretches_into_the_rect)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 10, 12, 26, 20, 1, 0, false, false, 0, 0);
    CHECK_RGB(Rgb(10, 12), 0xff0000, 0);
    CHECK_RGB(Rgb(17, 15), 0xff0000, 0);
    CHECK_RGB(Rgb(18, 15), 0x00ff00, 0);
    CHECK_RGB(Rgb(10, 16), 0x0000ff, 0);
    CHECK_RGB(Rgb(25, 19), 0xffffff, 0);
    CHECK_RGB(Rgb(26, 19), 0, 0);
    CHECK_RGB(Rgb(25, 20), 0, 0);
    Done();
}

TEST(engine_ImgStretchAlphaRect_mirrors_a_backwards_destination)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 28, 10, 20, 18, 1, 0, false, false, 0, 0);
    CheckQuads(20, 10, 0x00ff00, 0xff0000, 0xffffff, 0x0000ff);
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 40, 18, 48, 10, 1, 0, false, false, 0, 0);
    CheckQuads(40, 10, 0x0000ff, 0xffffff, 0xff0000, 0x00ff00);
    Done();
}

TEST(engine_ImgStretchAlphaRect_rotates_about_the_centre)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 20, 20, 28, 28, 1, 90, false, false, 0, 0);
    CheckQuads(20, 20, 0x00ff00, 0xffffff, 0xff0000, 0x0000ff);
    // About the bottom edge's middle: a half turn puts it below.
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 40, 10, 48, 18, 1, 180, false, false, 0, 4);
    CheckQuads(40, 18, 0xffffff, 0x0000ff, 0x00ff00, 0xff0000);
    CHECK_RGB(Rgb(44, 14), 0, 0);
    Done();
}

TEST(engine_ImgStretchAlphaRect_blend)
{
    Image *img;
    OpenWindow(64, 48);
    img = LoadQuads();
    DrawRect(0, 0, 64, 48, 1, 1, 1, 1);
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 0, 0, 8, 8, 0.5f, 0, false, false, 0, 0);
    CheckQuads(0, 0, 0xff8080, 0x80ff80, 0x8080ff, 0xffffff);
    // Untinted, a negative blend is opaque.
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 10, 0, 18, 8, -1, 0, false, false, 0, 0);
    CheckQuads(10, 0, 0xff0000, 0x00ff00, 0x0000ff, 0xffffff);
    // Tinted, it is the tint's alpha times the blend: nothing.
    ImgSetBlitColor(img, 1, 1, 1, 0.5f);
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 20, 0, 28, 8, -1, 0, false, false, 0, 0);
    CheckQuads(20, 0, 0xffffff, 0xffffff, 0xffffff, 0xffffff);
    ImgStretchAlphaRect(img, 0, 0, 8, 8, 30, 0, 38, 8, 1, 0, false, false, 0, 0);
    CheckQuads(30, 0, 0xff8080, 0x80ff80, 0x8080ff, 0xffffff);
    Done();
}

// ---------------------------------------------------------------------------------------------
// Primitives and the back buffer
// ---------------------------------------------------------------------------------------------

TEST(engine_DrawRect_fills_whole_pixels)
{
    OpenWindow(64, 48);
    DrawRect(10, 20, 30, 25, 1, 0, 0, 1);
    CHECK_EQ_INT(Pixel(10, 20), 0xffff0000);
    CHECK_EQ_INT(Pixel(29, 24), 0xffff0000);
    CHECK_EQ_INT(Pixel(30, 24), 0xff000000);
    CHECK_EQ_INT(Pixel(29, 25), 0xff000000);
    CHECK_EQ_INT(Pixel(9, 20), 0xff000000);
    CHECK_EQ_INT(Pixel(10, 19), 0xff000000);
    // Corners in any order, fractions dropped.
    DrawRect(50.9f, 10.7f, 40.2f, 5.5f, 0, 0, 1, 1);
    CHECK_EQ_INT(Pixel(40, 5), 0xff0000ff);
    CHECK_EQ_INT(Pixel(49, 9), 0xff0000ff);
    CHECK_EQ_INT(Pixel(50, 9), 0xff000000);
    CHECK_EQ_INT(Pixel(49, 10), 0xff000000);
    CHECK_EQ_INT(Pixel(39, 5), 0xff000000);
}

TEST(engine_DrawRect_blends_when_not_opaque)
{
    OpenWindow(64, 48);
    DrawRect(0, 0, 64, 48, 1, 1, 1, 1);
    DrawRect(10, 10, 20, 20, 1, 0, 0, 0.5f);
    CHECK_RGB(Rgb(15, 15), 0xff8080, 2);
    // Opaque replaces.
    DrawRect(10, 10, 20, 20, 0, 1, 0, 1);
    CHECK_RGB(Rgb(15, 15), 0x00ff00, 0);
    // Colours outside 0-1 are clamped.
    DrawRect(30, 10, 40, 20, 2, -1, 0.5f, 3);
    CHECK_RGB(Rgb(35, 15), 0xff0080, 1);
}

TEST(engine_DrawLine_draws_between_whole_pixels)
{
    OpenWindow(64, 48);
    DrawLine(10.9f, 30.9f, 20.2f, 30.2f, 0, 1, 0, 1);
    CHECK_RGB(Rgb(10, 30), 0x00ff00, 0);
    CHECK_RGB(Rgb(15, 30), 0x00ff00, 0);
    CHECK_RGB(Rgb(20, 30), 0x00ff00, 0);
    CHECK_RGB(Rgb(15, 31), 0, 0);
    CHECK_RGB(Rgb(9, 30), 0, 0);
    DrawLine(5, 5, 5, 15, 1, 0, 0, 1);
    CHECK_RGB(Rgb(5, 10), 0xff0000, 0);
    CHECK_RGB(Rgb(6, 10), 0, 0);
    DrawRect(30, 0, 64, 20, 1, 1, 1, 1);
    DrawLine(30, 10, 60, 10, 0, 0, 1, 0.5f);
    CHECK_RGB(Rgb(40, 10), 0x8080ff, 2);
}

TEST(engine_PlotPixel_sets_one_pixel)
{
    OpenWindow(64, 48);
    PlotPixel(5.7f, 6.9f, 1, 1, 0, 1);
    CHECK_RGB(Rgb(5, 6), 0xffff00, 0);
    CHECK_RGB(Rgb(6, 6), 0, 0);
    CHECK_RGB(Rgb(5, 7), 0, 0);
    DrawRect(10, 10, 20, 20, 1, 1, 1, 1);
    PlotPixel(12, 12, 0, 0, 0, 0.5f);
    CHECK_RGB(Rgb(12, 12), 0x808080, 2);
}

TEST(engine_SysSetWorldView_clears_to_the_clear_colour)
{
    OpenWindow(64, 48);
    DrawRect(0, 0, 10, 10, 1, 0, 0, 1);
    SysSetClearColor(0, 0.5f, 1, 0.25f);
    SysSetWorldView(0, 0, 0, 1, false);
    CHECK_EQ_INT(Pixel(5, 5), 0xffff0000);
    SysSetWorldView(0, 0, 0, 1, true);
    // Opaque whatever the clear alpha.
    CHECK_EQ_INT(Pixel(5, 5), 0xff0080ff);
    CHECK_EQ_INT(Pixel(63, 47), 0xff0080ff);
}

TEST(engine_SysCreateWindow_starts_with_a_black_back_buffer)
{
    OpenWindow(64, 48);
    CHECK_EQ_INT(Pixel(0, 0), 0xff000000);
    CHECK_EQ_INT(Pixel(63, 47), 0xff000000);
    {
        SDL_Texture *t = SDL_GetRenderTarget(TheRenderer());
        CHECK(t != NULL);
        CHECK_EQ_INT(t->w, 64);
        CHECK_EQ_INT(t->h, 48);
    }
}

TEST(engine_SysFlip_keeps_the_back_buffer)
{
    OpenWindow(64, 48);
    SysSetInterpolation(SYS_INTERP_OFF);
    DrawRect(10, 10, 20, 20, 0, 0, 1, 1);
    SysFlip();
    CHECK_EQ_INT(Pixel(15, 15), 0xff0000ff);
    // Drawn after a flip, still there after the next.
    DrawRect(10, 30, 20, 40, 1, 0, 0, 1);
    SysFlip();
    CHECK_EQ_INT(Pixel(15, 15), 0xff0000ff);
    CHECK_EQ_INT(Pixel(15, 35), 0xffff0000);
    // Still the target after an interpolated flip.
    SysSetInterpolation(SYS_INTERP_ON);
    SysSetMaxFps(200);
    SysFlip();
    SysFlip();
    CHECK_EQ_INT(Pixel(15, 15), 0xff0000ff);
    DrawRect(30, 10, 40, 20, 0, 1, 0, 1);
    SysFlip();
    CHECK_EQ_INT(Pixel(35, 15), 0xff00ff00);
}

TEST(engine_SysScreenshot_saves_the_back_buffer_resized)
{
    char p[1024];
    SDL_Surface *s;

    OpenWindow(64, 48);
    MakeTempDir(s_dir, sizeof s_dir);
    snprintf(p, sizeof p, "%s/Shots", s_dir);
    CHECK(mkdir(p, 0755) == 0);
    DrawRect(0, 0, 32, 48, 1, 0, 0, 1);
    DrawRect(32, 0, 64, 48, 0, 0, 1, 1);
    snprintf(p, sizeof p, "%s\\shots\\Shot1.jpg", s_dir);
    CHECK(SysScreenshot(p, 32, 24));
    snprintf(p, sizeof p, "%s/Shots/Shot1.jpg", s_dir);
    s = IMG_Load(p);
    CHECK_MSG(s != NULL, "%s not saved", p);
    CHECK_EQ_INT(s->w, 32);
    CHECK_EQ_INT(s->h, 24);
    CHECK_RGB(SurfaceRgb(s, 4, 12), 0xff0000, 40);
    CHECK_RGB(SurfaceRgb(s, 28, 12), 0x0000ff, 40);
    SDL_DestroySurface(s);
    // Drawing goes on into the back buffer.
    DrawRect(0, 0, 4, 4, 0, 1, 0, 1);
    CHECK_RGB(Rgb(1, 1), 0x00ff00, 0);
    Done();
}

TEST(engine_SysScreenshot_at_the_back_buffer_size)
{
    char p[1024];
    SDL_Surface *s;

    OpenWindow(64, 48);
    MakeTempDir(s_dir, sizeof s_dir);
    DrawRect(0, 0, 64, 24, 0, 1, 0, 1);
    snprintf(p, sizeof p, "%s/full.jpg", s_dir);
    CHECK(SysScreenshot(p, 64, 48));
    s = IMG_Load(p);
    CHECK(s != NULL);
    CHECK_EQ_INT(s->w, 64);
    CHECK_EQ_INT(s->h, 48);
    CHECK_RGB(SurfaceRgb(s, 30, 5), 0x00ff00, 40);
    CHECK_RGB(SurfaceRgb(s, 30, 40), 0, 40);
    SDL_DestroySurface(s);
    // Only the height changes.
    snprintf(p, sizeof p, "%s/flat.jpg", s_dir);
    CHECK(SysScreenshot(p, 64, 24));
    s = IMG_Load(p);
    CHECK(s != NULL);
    CHECK_EQ_INT(s->w, 64);
    CHECK_EQ_INT(s->h, 24);
    CHECK_RGB(SurfaceRgb(s, 30, 3), 0x00ff00, 40);
    CHECK_RGB(SurfaceRgb(s, 30, 20), 0, 40);
    SDL_DestroySurface(s);
    Done();
}
