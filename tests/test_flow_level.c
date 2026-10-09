// test_flow_level.c: Level data (src/game/level.c): counting the .lvd files, attack patterns,
// packing levels into the alien-graphics slots, unpacking them on level start, and the
// alien-graphics cache bookkeeping. Levels and patterns are synthesized from the structs in
// include/types.h and added to the fake archive.
#include <stdlib.h>
#include <zlib.h>
#include "support.h"

// ---- helpers ----

static LevelRaw *NewLevel(int type, const char *name)
{
    LevelRaw *L = calloc(1, sizeof *L);
    L->type = type;
    L->name1[0] = (char)strlen(name);
    memcpy(L->name1 + 1, name, strlen(name));
    return L;
}

static void PutFile(const char *name, const void *data, unsigned size)
{
    FakePacAdd(name, data, size);
}

static void PutLevel(const char *fmt, int n, LevelRaw *L)
{
    char name[64];
    snprintf(name, sizeof name, fmt, n);
    PutFile(name, L, sizeof *L);
    free(L);
}

static void PutEmpty(const char *fmt, int n)
{
    PutLevel(fmt, n, NewLevel(LEVEL_WAVE, "X"));
}

// A small but complete level: one group of two subs, a two-step path, one header entry,
// an aux rect and one alien graphic.
static LevelRaw *RichLevel(void)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE_AIMED, "RICH");
    L->count = 2;
    L->hdr[0] = (LvRawHdr){.count = 3, .type = 2, .hp = 40, .fireRateMin = 600, .fireRateMax = 900};
    L->grp[0].spawnX = -120;
    L->grp[0].spawnY = 30;
    L->grp[0].spawnDelay = 11;
    L->grp[0].spawnStep = 4;
    L->grp[0].count = 2;
    L->grp[0].velX = 512;     // 2.0
    L->grp[0].velY = -64;     // -0.25
    L->grp[0].groupId = 5;
    L->grp[0].kind = 7;
    L->grp[0].sub[0] = (LvRawSub){.xOffset = -40, .yOffset = 90, .type = 1, .hp = 12,
                                  .fireRateMin = 300, .fireRateMax = 400, .fireDelay = 77, .pathId = 3};
    L->grp[0].sub[1] = (LvRawSub){.xOffset = 40, .yOffset = 95, .type = 1, .hp = 13};
    L->grp[1].count = 1;
    L->grp[1].sub[0].type = 1;
    L->objCount[0] = 2;
    L->objCount[1] = 1;
    L->obj[0][0] = (LvRawObj){.pathX = -128, .pathY = 384, .cmd = 2, .unusedD = 9, .holdTime = 25};
    L->obj[0][1] = (LvRawObj){.pathX = 0, .pathY = 0, .cmd = 1, .holdTime = 100};
    L->aux[0] = (LvRawQ){.x1 = 6, .y1 = 1, .x2 = 3, .y2 = 4};
    L->w[0] = 150;
    L->h[0] = 250;
    strcpy(L->gfx[0], "gfx\\aliens\\Alien1.bmp");
    return L;
}

static Image *FakeImage(const char *name)
{
    return ImgLoad(name, false, false);
}

// ---- counting the level files ----

TEST(Flow_CountClassicLevels_counts_files_001_to_500)
{
    PutEmpty("classic_level_%03d.lvd", 1);
    PutEmpty("classic_level_%03d.lvd", 7);
    PutEmpty("classic_level_%03d.lvd", 500);
    PutEmpty("classic_level_%03d.lvd", 501);
    CHECK_EQ_INT(CountClassicLevels(), 3);
}

TEST(Flow_CountTimeTrialLevels_counts_files_01_to_50)
{
    PutEmpty("timetrial_%02d.lvd", 0);
    PutEmpty("timetrial_%02d.lvd", 1);
    PutEmpty("timetrial_%02d.lvd", 50);
    PutEmpty("timetrial_%02d.lvd", 51);
    CHECK_EQ_INT(CountTimeTrialLevels(), 2);
}

TEST(Flow_CountMalfunctionLevels_counts_files_00_to_49)
{
    g_numMalfunction = 99;
    PutEmpty("malfunction_%02d.lvd", 0);
    PutEmpty("malfunction_%02d.lvd", 49);
    PutEmpty("malfunction_%02d.lvd", 50);
    CountMalfunctionLevels();
    CHECK_EQ_INT(g_numMalfunction, 2);
}

TEST(Flow_CheckTimeTrialAvailable_locks_the_mode_for_10s_without_levels)
{
    g_time = 5000;
    g_numLevels2 = 7;
    CHECK_EQ_INT(CheckTimeTrialAvailable(), 0);
    CHECK_EQ_INT(g_numLevels2, 0);
    CHECK_EQ_INT(g_timeTrialLocked, 1);
    CHECK_EQ_INT(g_timeTrialDeadline, 15000);
}

TEST(Flow_CheckTimeTrialAvailable_unlocks_with_levels)
{
    PutEmpty("timetrial_%02d.lvd", 1);
    PutEmpty("timetrial_%02d.lvd", 2);
    g_timeTrialLocked = 1;
    g_timeTrialDeadline = 1234;
    CHECK_EQ_INT(CheckTimeTrialAvailable(), 1);
    CHECK_EQ_INT(g_numLevels2, 2);
    CHECK_EQ_INT(g_timeTrialLocked, 0);
    CHECK_EQ_INT(g_timeTrialDeadline, 1234);
}

// ---- attack patterns ----

static void PutPattern(int n, int count, int tag)
{
    Pattern p;
    memset(&p, 0, sizeof p);
    p.count = count;
    p.unused5 = tag;
    for (int i = 0; i < MAX_PATTERN_ENTRIES; i++)
        p.entries[i] = (PatternPt){.x = 100 + i, .y = 200 + i, .type = 1, .uParam = 3, .tParam = 4};
    char name[32];
    snprintf(name, sizeof name, "att%03d.swd", n);
    PutFile(name, &p, sizeof p);
}

TEST(Flow_LoadPatterns_loads_att001_to_att050_that_exist)
{
    PutPattern(1, 5, 7);
    PutPattern(4, 5, 9);
    PutPattern(50, 5, 11);
    PutPattern(51, 5, 13);
    LoadPatterns();
    CHECK_EQ_INT(g_patternCount, 3);
    CHECK_EQ_INT(g_patterns[0].unused5, 7);
    CHECK_EQ_INT(g_patterns[1].unused5, 9);
    CHECK_EQ_INT(g_patterns[2].unused5, 11);
}

TEST(Flow_LoadPatterns_zero_fills_entries_past_count)
{
    PutPattern(1, 2, 0);
    LoadPatterns();
    CHECK_EQ_INT(g_patternCount, 1);
    CHECK_EQ_INT(g_patterns[0].count, 2);
    CHECK_EQ_INT(g_patterns[0].entries[0].x, 100);
    CHECK_EQ_INT(g_patterns[0].entries[1].y, 201);
    CHECK_EQ_INT(g_patterns[0].entries[1].type, 1);
    CHECK_EQ_INT(g_patterns[0].entries[1].uParam, 3);
    CHECK_EQ_INT(g_patterns[0].entries[1].tParam, 4);
    for (int i = 2; i < MAX_PATTERN_ENTRIES; i++) {
        PatternPt *e = &g_patterns[0].entries[i];
        CHECK_MSG(e->x == 0 && e->y == 0 && e->type == 0 && e->uParam == 0 && e->tParam == 0,
                  "entry %d not cleared", i);
    }
}

// ---- reading one level file ----

TEST(Flow_LoadClassicLevel_reads_classic_level_NNN)
{
    PutLevel("classic_level_%03d.lvd", 12, NewLevel(LEVEL_BOSS, "B"));
    CHECK_EQ_INT(LoadClassicLevel(12), 1);
    CHECK_EQ_INT(g_levelRaw.type, LEVEL_BOSS);
    CHECK_EQ_INT(LoadClassicLevel(13), 0);
}

TEST(Flow_LoadTimeTrialLevel_reads_timetrial_NN)
{
    PutLevel("timetrial_%02d.lvd", 7, NewLevel(LEVEL_RACE, "T"));
    CHECK_EQ_INT(LoadTimeTrialLevel(7), 1);
    CHECK_EQ_INT(g_levelRaw.type, LEVEL_RACE);
    CHECK_EQ_INT(LoadTimeTrialLevel(8), 0);
}

TEST(Flow_LoadMalfunctionLevel_reads_malfunction_NN)
{
    PutLevel("malfunction_%02d.lvd", 3, NewLevel(LEVEL_BONUS_WAVE, "M"));
    CHECK_EQ_INT(LoadMalfunctionLevel(3), 1);
    CHECK_EQ_INT(g_levelRaw.type, LEVEL_BONUS_WAVE);
    CHECK_EQ_INT(LoadMalfunctionLevel(4), 0);
}

// ---- packing ----

TEST(Flow_PackLevelData_unpacks_the_level_into_g_curLevelData)
{
    PutLevel("classic_level_%03d.lvd", 4, RichLevel());
    PackLevelData(0, 4, 0);
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_WAVE_AIMED);
    CHECK_EQ_INT(g_curLevelData.count, 2);
    CHECK_EQ_INT(g_curLevelData.hdr[0].count, 3);
    CHECK_EQ_INT(g_curLevelData.hdr[0].type, 2);
    CHECK_EQ_INT(g_curLevelData.hdr[0].hp, 40);
    CHECK_EQ_INT(g_curLevelData.hdr[0].fireRateMin, 600);
    CHECK_EQ_INT(g_curLevelData.hdr[0].fireRateMax, 900);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnX, -120);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnY, 30);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnDelay, 11);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnStep, 4);
    CHECK_EQ_INT(g_curLevelData.grp[0].count, 2);
    CHECK_NEAR(g_curLevelData.grp[0].velX, 2.0, 1e-6);
    CHECK_NEAR(g_curLevelData.grp[0].velY, -0.25, 1e-6);
    CHECK_EQ_INT(g_curLevelData.grp[0].groupId, 5);
    CHECK_EQ_INT(g_curLevelData.grp[0].kind, 7);
    LvSub *s = &g_curLevelData.grp[0].sub[0];
    CHECK_EQ_INT(s->xOffset, -40);
    CHECK_EQ_INT(s->yOffset, 90);
    CHECK_EQ_INT(s->type, 1);
    CHECK_EQ_INT(s->hp, 12);
    CHECK_EQ_INT(s->fireRateMin, 300);
    CHECK_EQ_INT(s->fireRateMax, 400);
    CHECK_EQ_INT(s->fireDelay, 77);
    CHECK_EQ_INT(s->pathId, 3);
    CHECK_EQ_INT(g_curLevelData.grp[0].sub[1].hp, 13);
    CHECK_EQ_INT(g_curLevelData.objCount[0], 2);
    CHECK_EQ_INT(g_curLevelData.objCount[1], 1);
    CHECK_NEAR(g_curLevelData.obj[0][0].pathX, -0.5, 1e-6);
    CHECK_NEAR(g_curLevelData.obj[0][0].pathY, 1.5, 1e-6);
    CHECK_EQ_INT(g_curLevelData.obj[0][0].cmd, 2);
    CHECK_EQ_INT(g_curLevelData.obj[0][0].unusedD, 9);
    CHECK_EQ_INT(g_curLevelData.obj[0][0].holdTime, 25);
    CHECK_EQ_INT(g_curLevelData.obj[0][1].cmd, 1);
    CHECK_EQ_INT(g_curLevelData.obj[0][1].holdTime, 100);
    CHECK_EQ_INT(g_curLevelData.name1[0], 4);
    CHECK_MEM(g_curLevelData.name1 + 1, "RICH", 4);
    CHECK_EQ_INT(g_curLevelData.aux[0].x1, 6);
    CHECK_EQ_INT(g_curLevelData.aux[0].y1, 1);
    CHECK_EQ_INT(g_curLevelData.aux[0].x2, 3);
    CHECK_EQ_INT(g_curLevelData.aux[0].y2, 4);
    CHECK_EQ_INT(g_curLevelData.w[0], 150);
    CHECK_EQ_INT(g_curLevelData.h[0], 250);
}

TEST(Flow_PackLevelData_clears_subs_and_paths_left_from_the_previous_level)
{
    LevelRaw *big = NewLevel(LEVEL_WAVE, "BIG");
    big->count = 1;
    big->grp[0].count = 3;
    big->grp[0].sub[2].hp = 99;
    big->objCount[0] = 3;
    big->obj[0][2].holdTime = 55;
    PutLevel("classic_level_%03d.lvd", 1, big);
    LevelRaw *small = NewLevel(LEVEL_WAVE, "SMALL");
    small->count = 1;
    small->grp[0].count = 1;
    small->objCount[0] = 1;
    PutLevel("classic_level_%03d.lvd", 2, small);
    PackLevelData(0, 1, 0);
    CHECK_EQ_INT(g_curLevelData.grp[0].sub[2].hp, 99);
    CHECK_EQ_INT(g_curLevelData.obj[0][2].holdTime, 55);
    PackLevelData(1, 2, 0);
    CHECK_EQ_INT(g_curLevelData.grp[0].sub[2].hp, 0);
    CHECK_EQ_INT(g_curLevelData.obj[0][2].holdTime, 0);
}

TEST(Flow_PackLevelData_stores_the_compressed_level_in_the_slot)
{
    PutLevel("classic_level_%03d.lvd", 4, RichLevel());
    int size = PackLevelData(9, 4, 0);
    AlienGfxSlot *s = &g_alienGfxSlots[9];
    CHECK_EQ_INT(s->owner, 4);
    CHECK_EQ_INT(s->id, 0);
    CHECK_EQ_INT(s->srcLen, size);
    CHECK(size > 0 && size < (int)sizeof(LevelRaw));
    LevelRaw *out = malloc(sizeof *out);
    uLongf len = sizeof *out;
    CHECK_EQ_INT(uncompress((Bytef *)out, &len, s->src, (uLong)s->srcLen), Z_OK);
    CHECK_EQ_INT(len, sizeof(LevelRaw));
    LevelRaw *want = RichLevel();
    CHECK_MEM(out, want, sizeof *want);
    free(want);
    free(out);
}

TEST(Flow_PackLevelData_records_the_mode_of_time_trial_and_malfunction_levels)
{
    PutLevel("timetrial_%02d.lvd", 3, NewLevel(LEVEL_RACE, "T"));
    PutLevel("malfunction_%02d.lvd", 3, NewLevel(LEVEL_BONUS_WAVE, "M"));
    PackLevelData(0, 3, 1);
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_RACE);
    CHECK_EQ_INT(g_alienGfxSlots[0].id, 1);
    CHECK_EQ_INT(g_alienGfxSlots[0].owner, 3);
    PackLevelData(1, 3, 2);
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_BONUS_WAVE);
    CHECK_EQ_INT(g_alienGfxSlots[1].id, 2);
}

TEST(Flow_PackLevelData_derives_the_tga_mask_and_hma_names)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "N");
    strcpy(L->gfx[0], "gfx\\aliens\\Alien1.bmp");
    strcpy(L->gfx[2], "d:\\x\\Boss.png");
    PutLevel("classic_level_%03d.lvd", 1, L);
    PackLevelData(0, 1, 0);
    AlienGfxSlot *s = &g_alienGfxSlots[0];
    CHECK_STR(g_curLevelData.gfx[0], "Alien1.tga");
    CHECK_STR(g_curLevelData.mask[0], "Alien1_mask.tga");
    CHECK_STR(s->name1[0], "Alien1.tga");
    CHECK_STR(s->name2[0], "Alien1_mask.tga");
    CHECK_STR(s->name3[0], "Alien1");
    CHECK_EQ_INT(s->key[0], StrHash("Alien1.tga"));
    CHECK_STR(s->name1[2], "Boss.tga");
    CHECK_STR(s->name3[2], "Boss");
    CHECK_STR(s->name1[1], "");
    CHECK_STR(g_curLevelData.gfx[1], "");
    CHECK_EQ_INT(s->frameCount, 2);
}

// The path scan starts at index 1, so a bare file name loses its first character.
TEST(Flow_PackLevelData_drops_the_first_character_of_a_path_without_folder)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "N");
    strcpy(L->gfx[0], "Alien.bmp");
    PutLevel("classic_level_%03d.lvd", 1, L);
    PackLevelData(0, 1, 0);
    CHECK_STR(g_alienGfxSlots[0].name1[0], "lien.tga");
}

TEST(Flow_PackLevelData_copies_the_frame_sizes_into_the_slot)
{
    LevelRaw *L = NewLevel(LEVEL_WAVE, "N");
    L->w[3] = 31;
    L->h[3] = 47;
    L->w[5] = 7;
    PutLevel("classic_level_%03d.lvd", 1, L);
    PackLevelData(2, 1, 0);
    CHECK_EQ_INT(g_alienGfxSlots[2].frameW[3], 31);
    CHECK_EQ_INT(g_alienGfxSlots[2].frameH[3], 47);
    CHECK_EQ_INT(g_alienGfxSlots[2].frameW[5], 7);
    CHECK_EQ_INT(g_curLevelData.h[3], 47);
}

TEST(Flow_PackLevelData_preloads_the_graphics)
{
    PutLevel("classic_level_%03d.lvd", 1, RichLevel());
    g_maxBuffered = 10;
    PackLevelData(0, 1, 0);
    AlienGfxSlot *s = &g_alienGfxSlots[0];
    CHECK(s->gfx[0] != NULL);
    CHECK_STR(FakeImageName(s->gfx[0]), "alien1.tga");
    CHECK(s->gfx2[0] != NULL);
    CHECK_STR(FakeImageName(s->gfx2[0]), "alien1_mask.tga");
    CHECK_EQ_INT(s->loaded[0], 1);
    CHECK_EQ_INT(s->count[0], 1);
    CHECK_EQ_INT(g_alienGfxBufferedCount, 1);
    CHECK_EQ_INT(g_lastLevelBuffered, 1);
    CHECK(s->gfx[1] == NULL);
}

TEST(Flow_PackLevelData_shares_graphics_already_loaded_by_another_slot)
{
    PutLevel("classic_level_%03d.lvd", 1, RichLevel());
    PutLevel("classic_level_%03d.lvd", 2, RichLevel());
    g_maxBuffered = 10;
    PackLevelData(0, 1, 0);
    int images = g_fake.imagesLoaded;
    PackLevelData(1, 2, 0);
    CHECK_EQ_INT(g_fake.imagesLoaded, images);
    CHECK(g_alienGfxSlots[1].gfx[0] == g_alienGfxSlots[0].gfx[0]);
    CHECK(g_alienGfxSlots[1].gfx2[0] == g_alienGfxSlots[0].gfx2[0]);
    CHECK_EQ_INT(g_alienGfxSlots[1].loaded[0], 0);
    CHECK_EQ_INT(g_alienGfxSlots[0].count[0], 2);
    CHECK_EQ_INT(g_alienGfxBufferedCount, 1);
}

TEST(Flow_PackLevelData_stops_preloading_at_the_buffer_cap)
{
    LevelRaw *L = RichLevel();
    strcpy(L->gfx[1], "gfx\\aliens\\Alien2.bmp");
    PutLevel("classic_level_%03d.lvd", 1, L);
    g_maxBuffered = 1;
    PackLevelData(0, 1, 0);
    CHECK(g_alienGfxSlots[0].gfx[0] != NULL);
    CHECK(g_alienGfxSlots[0].gfx[1] == NULL);
    CHECK_EQ_INT(g_alienGfxSlots[0].loaded[1], 0);
    CHECK_STR(g_alienGfxSlots[0].name1[1], "Alien2.tga");
    CHECK_EQ_INT(g_alienGfxBufferedCount, 1);
}

TEST(Flow_PackLevelData_finds_a_hit_mask_under_its_real_name)
{
    g_fake.imageW = 512;
    g_fake.imageH = 32;
    unsigned char *m = calloc(512 * 32, 1);
    PutFile("alien1.hma", m, 512 * 32);
    free(m);
    PutLevel("classic_level_%03d.lvd", 1, RichLevel());
    g_maxBuffered = 10;
    PackLevelData(0, 1, 0);
    CHECK(g_alienGfxSlots[0].gfx[0] != NULL);
    CHECK(g_alienGfxSlots[0].hma[0] != NULL);
}

// A 512 x 32 hit mask with a solid box in frame A (x 0x1e0+3..10, y 2..20) and frame B
// (x 0x1c0+5..7, y 4..6).
static unsigned char *MakeHma(void)
{
    unsigned char *m = calloc(512 * 32, 1);
    for (int y = 2; y <= 20; y++)
        for (int x = 3; x <= 10; x++)
            m[y * 512 + 0x1e0 + x] = 255;
    for (int y = 4; y <= 6; y++)
        for (int x = 5; x <= 7; x++)
            m[y * 512 + 0x1c0 + x] = 9;
    return m;
}

TEST(Flow_PackLevelData_scans_the_hit_mask_for_frame_rects)
{
    g_fake.imageW = 512;
    g_fake.imageH = 32;
    unsigned char *m = MakeHma();
    PutFile("alien1.hma", m, 512 * 32);
    free(m);
    PutLevel("classic_level_%03d.lvd", 1, RichLevel());
    g_maxBuffered = 10;
    PackLevelData(0, 1, 0);
    FrameSet *b = &g_alienGfxSlots[0].blk[0];
    CHECK(g_alienGfxSlots[0].hma[0] != NULL);
    CHECK_EQ_INT(b->aLeft, 3);
    CHECK_EQ_INT(b->aTop, 2);
    CHECK_EQ_INT(b->aWidth, 10);
    CHECK_EQ_INT(b->aHeight, 20);
    CHECK_EQ_INT(b->bLeft, 5);
    CHECK_EQ_INT(b->bTop, 4);
    CHECK_EQ_INT(b->bWidth, 7);
    CHECK_EQ_INT(b->bHeight, 6);
}

// ---- frame rects ----

TEST(Flow_ScanFrameRects_finds_the_bounding_boxes)
{
    unsigned char *m = MakeHma();
    ScanFrameRects(m, 512, 32);
    CHECK_EQ_INT(g_frameAX1, 3);
    CHECK_EQ_INT(g_frameAY1, 2);
    CHECK_EQ_INT(g_frameAX2, 10);
    CHECK_EQ_INT(g_frameAY2, 20);
    CHECK_EQ_INT(g_frameBX1, 5);
    CHECK_EQ_INT(g_frameBY1, 4);
    CHECK_EQ_INT(g_frameBX2, 7);
    CHECK_EQ_INT(g_frameBY2, 6);
    free(m);
}

TEST(Flow_ScanFrameRects_of_an_empty_mask_is_the_whole_frame)
{
    unsigned char *m = calloc(512 * 32, 1);
    ScanFrameRects(m, 512, 32);
    CHECK_EQ_INT(g_frameAX1, 0);
    CHECK_EQ_INT(g_frameAY1, 0);
    CHECK_EQ_INT(g_frameAX2, 0x1f);
    CHECK_EQ_INT(g_frameAY2, 0x1f);
    CHECK_EQ_INT(g_frameBX2, 0x1e);
    CHECK_EQ_INT(g_frameBY2, 0x1e);
    free(m);
}

// The right and bottom scans start at x = width and y = height: one column and one row
// past the 31 x 31 frame.
TEST(Flow_ScanFrameRects_reads_one_column_and_row_past_the_frame)
{
    unsigned char *m = calloc(512 * 32, 1);
    m[2 * 512 + 0x1e0 + 2] = 1;
    m[5 * 512 + 0x1e0 + 31] = 1;     // column 31: seen by the right scan only
    m[31 * 512 + 0x1e0 + 5] = 1;     // row 31: seen by the bottom scan only
    ScanFrameRects(m, 512, 32);
    CHECK_EQ_INT(g_frameAX1, 2);
    CHECK_EQ_INT(g_frameAY1, 2);
    CHECK_EQ_INT(g_frameAX2, 31);
    CHECK_EQ_INT(g_frameAY2, 31);
    free(m);
}

TEST(Flow_CacheFrameRect_copies_the_scanned_rects_of_a_loaded_slot)
{
    unsigned char *m = MakeHma();
    g_fake.imageW = 512;
    g_fake.imageH = 32;
    g_alienGfxCache[2].gfx1 = FakeImage("a.tga");
    g_alienGfxMem[2] = m;
    CacheFrameRect(2);
    CHECK_EQ_INT(g_frames[2].aLeft, 3);
    CHECK_EQ_INT(g_frames[2].aTop, 2);
    CHECK_EQ_INT(g_frames[2].aWidth, 10);
    CHECK_EQ_INT(g_frames[2].aHeight, 20);
    CHECK_EQ_INT(g_frames[2].bLeft, 5);
    CHECK_EQ_INT(g_frames[2].bTop, 4);
    CHECK_EQ_INT(g_frames[2].bWidth, 7);
    CHECK_EQ_INT(g_frames[2].bHeight, 6);
    free(m);
}

TEST(Flow_InitFrameRectDefaults_sets_32_and_64_pixel_boxes)
{
    g_rectsA[4] = (Rect16){.x1 = 9, .y1 = 9, .x2 = 9, .y2 = 9};
    g_rectsB[4] = (Rect16){.x1 = 9, .y1 = 9, .x2 = 9, .y2 = 9};
    InitFrameRectDefaults(4);
    CHECK_EQ_INT(g_rectsA[4].x1, 0);
    CHECK_EQ_INT(g_rectsA[4].y1, 0);
    CHECK_EQ_INT(g_rectsA[4].x2, 32);
    CHECK_EQ_INT(g_rectsA[4].y2, 32);
    CHECK_EQ_INT(g_rectsB[4].x1, 0);
    CHECK_EQ_INT(g_rectsB[4].y1, 0);
    CHECK_EQ_INT(g_rectsB[4].x2, 64);
    CHECK_EQ_INT(g_rectsB[4].y2, 64);
}

// ---- unpacking on level start ----

static void PackRich(int slot, int level)
{
    PutLevel("classic_level_%03d.lvd", level, RichLevel());
    g_maxBuffered = 10;
    PackLevelData(slot, level, 0);
}

TEST(Flow_LoadLevelData_unpacks_the_slot_of_the_current_level)
{
    PutLevel("classic_level_%03d.lvd", 1, NewLevel(LEVEL_BOSS, "OTHER"));
    PackLevelData(0, 1, 0);
    PackRich(5, 3);
    memset(&g_curLevelData, 0, sizeof g_curLevelData);
    g_state = STATE_PLAYING;
    g_gameMode = MODE_SINGLE;
    g_curLevelNum = 3;
    LoadLevelData();
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_WAVE_AIMED);
    CHECK_EQ_INT(g_curLevelData.count, 2);
    CHECK_EQ_INT(g_curLevelData.hdr[0].count, 3);
    CHECK_EQ_INT(g_curLevelData.hdr[0].type, 2);
    CHECK_EQ_INT(g_curLevelData.hdr[0].hp, 40);
    CHECK_EQ_INT(g_curLevelData.hdr[0].fireRateMin, 600);
    CHECK_EQ_INT(g_curLevelData.hdr[0].fireRateMax, 900);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnX, -120);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnY, 30);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnDelay, 11);
    CHECK_EQ_INT(g_curLevelData.grp[0].spawnStep, 4);
    CHECK_EQ_INT(g_curLevelData.grp[0].count, 2);
    CHECK_NEAR(g_curLevelData.grp[0].velX, 2.0, 1e-6);
    CHECK_NEAR(g_curLevelData.grp[0].velY, -0.25, 1e-6);
    CHECK_EQ_INT(g_curLevelData.grp[0].groupId, 5);
    CHECK_EQ_INT(g_curLevelData.grp[0].kind, 7);
    LvSub *s = &g_curLevelData.grp[0].sub[0];
    CHECK_EQ_INT(s->xOffset, -40);
    CHECK_EQ_INT(s->yOffset, 90);
    CHECK_EQ_INT(s->type, 1);
    CHECK_EQ_INT(s->hp, 12);
    CHECK_EQ_INT(s->fireRateMin, 300);
    CHECK_EQ_INT(s->fireRateMax, 400);
    CHECK_EQ_INT(s->fireDelay, 77);
    CHECK_EQ_INT(s->pathId, 3);
    CHECK_EQ_INT(g_curLevelData.grp[0].sub[1].xOffset, 40);
    CHECK_EQ_INT(g_curLevelData.objCount[0], 2);
    CHECK_EQ_INT(g_curLevelData.objCount[1], 1);
    CHECK_NEAR(g_curLevelData.obj[0][0].pathX, -0.5, 1e-6);
    CHECK_NEAR(g_curLevelData.obj[0][0].pathY, 1.5, 1e-6);
    CHECK_EQ_INT(g_curLevelData.obj[0][0].cmd, 2);
    CHECK_EQ_INT(g_curLevelData.obj[0][0].unusedD, 9);
    CHECK_EQ_INT(g_curLevelData.obj[0][0].holdTime, 25);
    CHECK_EQ_INT(g_curLevelData.obj[0][1].holdTime, 100);
    CHECK_EQ_INT(g_curLevelData.name1[0], 4);
    CHECK_MEM(g_curLevelData.name1 + 1, "RICH", 4);
    CHECK_EQ_INT(g_curLevelData.aux[0].x1, 6);
    CHECK_EQ_INT(g_curLevelData.aux[0].y1, 1);
    CHECK_EQ_INT(g_curLevelData.aux[0].x2, 3);
    CHECK_EQ_INT(g_curLevelData.aux[0].y2, 4);
    CHECK_EQ_INT(g_curLevelData.w[0], 150);
    CHECK_EQ_INT(g_curLevelData.h[0], 250);
}

TEST(Flow_LoadLevelData_clears_subs_left_from_the_previous_level)
{
    PackRich(0, 1);
    g_curLevelData.grp[0].sub[7].hp = 66;
    g_curLevelData.obj[1][9].holdTime = 66;
    g_state = STATE_PLAYING;
    g_curLevelNum = 1;
    LoadLevelData();
    CHECK_EQ_INT(g_curLevelData.grp[0].sub[7].hp, 0);
    CHECK_EQ_INT(g_curLevelData.obj[1][9].holdTime, 0);
}

TEST(Flow_LoadLevelData_hooks_up_the_preloaded_graphics)
{
    PackRich(0, 1);
    AlienGfxSlot *s = &g_alienGfxSlots[0];
    s->blk[0].aLeft = 21;
    g_state = STATE_PLAYING;
    g_curLevelNum = 1;
    LoadLevelData();
    CHECK(g_alienGfxCache[0].gfx1 == s->gfx[0]);
    CHECK(g_alienGfxCache[0].gfx2 == s->gfx2[0]);
    CHECK_EQ_INT(g_hazard0GfxW, HAZARD_GFX_W);
    CHECK_EQ_INT(g_frames[0].aLeft, 21);
    CHECK_EQ_INT(s->count[0], 2);
    CHECK_EQ_INT(g_animFrameCount, 1);
}

TEST(Flow_LoadLevelData_bumps_the_owner_of_shared_graphics)
{
    PutLevel("classic_level_%03d.lvd", 1, RichLevel());
    PutLevel("classic_level_%03d.lvd", 2, RichLevel());
    g_maxBuffered = 10;
    PackLevelData(0, 1, 0);
    PackLevelData(1, 2, 0);
    CHECK_EQ_INT(g_alienGfxSlots[0].count[0], 2);
    g_state = STATE_PLAYING;
    g_curLevelNum = 2;
    LoadLevelData();
    CHECK(g_alienGfxCache[0].gfx1 == g_alienGfxSlots[0].gfx[0]);
    CHECK_EQ_INT(g_alienGfxSlots[0].count[0], 3);
    CHECK_EQ_INT(g_alienGfxSlots[1].count[0], 0);
}

TEST(Flow_LoadLevelData_loads_graphics_that_were_not_preloaded)
{
    g_fake.imageW = 512;
    g_fake.imageH = 32;
    unsigned char *m = MakeHma();
    PutFile("alien1.hma", m, 512 * 32);
    free(m);
    PutLevel("classic_level_%03d.lvd", 1, RichLevel());
    g_maxBuffered = 0;
    PackLevelData(0, 1, 0);
    AlienGfxSlot *s = &g_alienGfxSlots[0];
    CHECK(s->gfx[0] == NULL);
    g_state = STATE_PLAYING;
    g_curLevelNum = 1;
    g_time = 7000;
    LoadLevelData();
    CHECK(s->gfx[0] != NULL);
    CHECK_STR(FakeImageName(s->gfx[0]), "alien1.tga");
    CHECK_STR(FakeImageName(s->gfx2[0]), "alien1_mask.tga");
    CHECK(s->hma[0] != NULL);
    CHECK_EQ_INT(s->loaded[0], 1);
    CHECK_EQ_INT(s->count[0], 1);
    CHECK_EQ_INT(s->blk[0].aWidth, 10);
    CHECK_EQ_INT(g_frames[0].aHeight, 20);
    CHECK(g_alienGfxCache[0].gfx1 == s->gfx[0]);
    CHECK(g_alienGfxMem[0] == s->hma[0]);
    CHECK_EQ_INT(g_soundStealCooldown, 7000 + 650);
}

TEST(Flow_LoadLevelData_without_a_hit_mask_leaves_the_graphics_unloaded)
{
    PutLevel("classic_level_%03d.lvd", 1, RichLevel());
    g_maxBuffered = 0;
    PackLevelData(0, 1, 0);
    g_state = STATE_PLAYING;
    g_curLevelNum = 1;
    LoadLevelData();
    CHECK(g_alienGfxSlots[0].gfx[0] != NULL);
    CHECK_EQ_INT(g_alienGfxSlots[0].loaded[0], 0);
    CHECK(g_alienGfxCache[0].gfx1 == NULL);
}

TEST(Flow_LoadLevelData_uses_the_time_trial_slot_in_time_trial_mode)
{
    PutLevel("classic_level_%03d.lvd", 2, NewLevel(LEVEL_WAVE, "C"));
    PutLevel("timetrial_%02d.lvd", 2, NewLevel(LEVEL_BOSS, "T"));
    PutLevel("malfunction_%02d.lvd", 2, NewLevel(LEVEL_RACE, "M"));
    PackLevelData(0, 2, 0);
    PackLevelData(1, 2, 1);
    PackLevelData(2, 2, 2);
    g_state = STATE_PLAYING;
    g_gameMode = MODE_TIME_TRIAL;
    g_numLevels2 = 5;
    g_curLevelNum = 2;
    LoadLevelData();
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_BOSS);
    g_gameMode = MODE_SINGLE;
    LoadLevelData();
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_WAVE);
}

TEST(Flow_LoadLevelData_uses_the_malfunction_slot_during_a_malfunction)
{
    PutLevel("classic_level_%03d.lvd", 2, NewLevel(LEVEL_WAVE, "C"));
    PutLevel("malfunction_%02d.lvd", 2, NewLevel(LEVEL_RACE, "M"));
    PackLevelData(0, 2, 0);
    PackLevelData(1, 2, 2);
    memset(&g_curLevelData, 0, sizeof g_curLevelData);
    g_state = STATE_MALFUNCTION;
    g_gameMode = MODE_SINGLE;
    g_curLevelNum = 2;
    LoadLevelData();
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_RACE);
}

// `g_curLevelNum - 1 % 100 + 1` is g_curLevelNum (% binds first): level 101 does not wrap to
// level 1's slot, so nothing is loaded and the previous level's data stays.
TEST(Flow_LoadLevelData_does_not_wrap_levels_past_100)
{
    PutLevel("classic_level_%03d.lvd", 1, NewLevel(LEVEL_BOSS, "ONE"));
    PackLevelData(0, 1, 0);
    g_curLevelData.type = LEVEL_RACE;
    g_state = STATE_PLAYING;
    g_gameMode = MODE_SINGLE;
    g_curLevelNum = 101;
    LoadLevelData();
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_RACE);
}

// ---- the whole level buffer, as packed at start-up ----

static void BootWithLevels(void)
{
    LevelRaw *one = NewLevel(LEVEL_WAVE, "ONE");
    strcpy(one->gfx[0], "gfx\\a\\One.bmp");
    PutLevel("classic_level_%03d.lvd", 1, one);
    LevelRaw *two = NewLevel(LEVEL_BOSS, "TWO");
    strcpy(two->gfx[0], "gfx\\a\\Two.bmp");
    strcpy(two->gfx[1], "gfx\\a\\Three.bmp");
    PutLevel("classic_level_%03d.lvd", 2, two);
    PutLevel("timetrial_%02d.lvd", 1, NewLevel(LEVEL_RACE, "T1"));
    PutLevel("timetrial_%02d.lvd", 2, NewLevel(LEVEL_RACE, "T2"));
    PutLevel("malfunction_%02d.lvd", 1, NewLevel(LEVEL_BONUS_WAVE, "M1"));
    BootGame();
}

TEST(Flow_BufferAllLevels_packs_100_classic_then_time_trial_then_malfunction_levels)
{
    BootWithLevels();
    CHECK_EQ_INT(g_numLevels2, 2);
    CHECK_EQ_INT(g_numMalfunction, 1);
    for (int i = 0; i < 100; i++) {
        CHECK_EQ_INT(g_alienGfxSlots[i].owner, i + 1);
        CHECK_EQ_INT(g_alienGfxSlots[i].id, 0);
    }
    CHECK_EQ_INT(g_alienGfxSlots[100].owner, 1);
    CHECK_EQ_INT(g_alienGfxSlots[100].id, 1);
    CHECK_EQ_INT(g_alienGfxSlots[101].owner, 2);
    CHECK_EQ_INT(g_alienGfxSlots[101].id, 1);
    CHECK_EQ_INT(g_alienGfxSlots[102].owner, 1);
    CHECK_EQ_INT(g_alienGfxSlots[102].id, 2);
    CHECK(g_alienGfxSlots[102].src != NULL);
    CHECK(g_alienGfxSlots[103].src == NULL);
    CHECK_EQ_INT(g_alienGfxSlots[103].owner, 0);
}

TEST(Flow_BufferAllLevels_buffers_each_distinct_alien_graphic_once)
{
    BootWithLevels();
    CHECK_EQ_INT(g_alienGfxSlots[0].frameCount, 1);
    CHECK_EQ_INT(g_alienGfxSlots[1].frameCount, 2);
    CHECK_STR(FakeImageName(g_alienGfxSlots[1].gfx[1]), "three.tga");
    // One.tga, Two.tga, Three.tga; the missing levels 3-100 share level 2's (see below).
    CHECK_EQ_INT(g_alienGfxBufferedCount, 3);
    CHECK_EQ_INT(g_lastLevelBuffered, 2);
}

// A classic level without a file keeps g_levelRaw from the level packed before it.
TEST(Flow_BufferAllLevels_fills_missing_classic_levels_with_the_previous_one)
{
    BootWithLevels();
    g_state = STATE_PLAYING;
    g_gameMode = MODE_SINGLE;
    g_curLevelNum = 57;
    LoadLevelData();
    CHECK_EQ_INT(g_curLevelData.type, LEVEL_BOSS);
    CHECK_MEM(g_curLevelData.name1, "\x03TWO", 4);
    CHECK(g_alienGfxSlots[56].gfx[0] == g_alienGfxSlots[1].gfx[0]);
}

TEST(Flow_BufferAllLevels_stops_when_the_window_is_closed)
{
    BootWithLevels();
    FreeLevelBufs();
    for (int i = 0; i < MAX_ALIEN_GFX_SLOTS; i++)
        g_alienGfxSlots[i].owner = 0;
    g_fake.quit = true;
    BufferAllLevels();
    CHECK(g_alienGfxSlots[0].src == NULL);
    CHECK_EQ_INT(g_alienGfxSlots[0].owner, 0);
    CHECK_EQ_INT(g_alienGfxBufferedCount, 0);
    g_fake.quit = false;
    BufferAllLevels();
    CHECK_EQ_INT(g_alienGfxSlots[0].owner, 1);
    CHECK_EQ_INT(g_alienGfxSlots[102].id, 2);
}

TEST(Flow_BufferAllLevels_draws_the_loading_screen_once_at_the_end)
{
    BootWithLevels();
    int flips = g_fake.flips;
    // the fake clock stands still between flips: no 16 ms ever pass while packing
    g_fake.flipAdvanceMs = 0;
    BufferAllLevels();
    CHECK_EQ_INT(g_fake.flips, flips + 1);
}

// ---- the alien graphics cache ----

TEST(Flow_FreeLevelBufs_frees_packed_levels_and_loaded_graphics)
{
    BootWithLevels();
    Image *g = g_alienGfxSlots[0].gfx[0];
    g_alienGfxCache[0].gfx1 = g;
    int freed = g_fake.imagesFreed;
    FreeLevelBufs();
    CHECK(g_alienGfxSlots[0].src == NULL);
    CHECK(g_alienGfxSlots[102].src == NULL);
    CHECK(g_alienGfxSlots[0].gfx[0] == NULL);
    CHECK_EQ_INT(g_alienGfxSlots[0].loaded[0], 0);
    CHECK(ImgIsFreed(g));
    CHECK(g_alienGfxCache[0].gfx1 == NULL);
    // One/Two/Three: their picture and mask; the sharing slots don't free them again
    CHECK_EQ_INT(g_fake.imagesFreed, freed + 6);
    CHECK_EQ_INT(g_freedA, 3);
    CHECK_EQ_INT(g_freedB, 3);
}

static void LoadedSlot(int i, int owner, int count, Image *g)
{
    g_alienGfxSlots[i].owner = owner;
    g_alienGfxSlots[i].id = 0;
    g_alienGfxSlots[i].loaded[0] = 1;
    g_alienGfxSlots[i].count[0] = count;
    g_alienGfxSlots[i].gfx[0] = g;
    g_alienGfxSlots[i].hma[0] = malloc(4);
}

TEST(Flow_StealOldestAlienGfx_frees_the_least_used_other_slot)
{
    Image *a = FakeImage("a"), *b = FakeImage("b"), *c = FakeImage("c");
    LoadedSlot(0, 2, 3, b);
    LoadedSlot(1, 1, 5, a);
    LoadedSlot(2, 7, 1, c);             // the level being loaded: never stolen
    g_alienGfxSlots[3].gfx[0] = b;      // an unloaded slot sharing b
    g_alienGfxSlots[3].count[0] = 4;
    g_alienGfxCache[1].gfx1 = b;
    g_alienGfxMem[4] = g_alienGfxSlots[0].hma[0];
    StealOldestAlienGfx(7, 0);
    CHECK(g_alienGfxSlots[0].gfx[0] == NULL);
    CHECK(g_alienGfxSlots[0].hma[0] == NULL);
    CHECK_EQ_INT(g_alienGfxSlots[0].loaded[0], 0);
    CHECK_EQ_INT(g_alienGfxSlots[0].count[0], 0);
    CHECK(ImgIsFreed(b));
    CHECK(g_alienGfxCache[1].gfx1 == NULL);
    CHECK(g_alienGfxMem[4] == NULL);
    CHECK(g_alienGfxSlots[3].gfx[0] == NULL);
    CHECK_EQ_INT(g_alienGfxSlots[3].count[0], 0);
    CHECK_EQ_INT(g_alienGfxFreedA, 1);
    CHECK_EQ_INT(g_alienGfxFreedM, 1);
    CHECK(g_alienGfxSlots[1].gfx[0] == a);
    CHECK(g_alienGfxSlots[2].gfx[0] == c);
    CHECK(!ImgIsFreed(a) && !ImgIsFreed(c));
}

TEST(Flow_StealOldestAlienGfx_spares_a_shield_graphic)
{
    Image *a = FakeImage("a"), *b = FakeImage("b");
    LoadedSlot(0, 1, 5, a);
    LoadedSlot(1, 2, 3, b);
    g_save.players[0].shieldL = 1;
    g_save.players[0].shieldLIdx = 4;
    g_enemies[0][4].gfxA = b;
    StealOldestAlienGfx(7, 0);
    CHECK(g_alienGfxSlots[1].gfx[0] == b);
    CHECK(g_alienGfxSlots[0].gfx[0] == NULL);
    CHECK(ImgIsFreed(a));
}

TEST(Flow_StealOldestAlienGfx_spares_player_2_shields_only_in_two_player_modes)
{
    Image *a = FakeImage("a"), *b = FakeImage("b");
    LoadedSlot(0, 1, 5, a);
    LoadedSlot(1, 2, 3, b);
    g_save.players[1].shieldR = 1;
    g_save.players[1].shieldRIdx = 9;
    g_enemies[1][9].gfxA = b;
    g_gameMode = MODE_DUAL;
    StealOldestAlienGfx(7, 0);
    CHECK(g_alienGfxSlots[1].gfx[0] == b);
    CHECK(g_alienGfxSlots[0].gfx[0] == NULL);
}

TEST(Flow_FindOtherSlotWithKey_skips_the_asking_slot)
{
    g_alienGfxSlots[2].loaded[3] = 1;
    g_alienGfxSlots[2].key[3] = 42;
    CHECK(!FindOtherSlotWithKey(2, 3, 42));
    g_alienGfxSlots[6].loaded[1] = 1;
    g_alienGfxSlots[6].key[1] = 42;
    CHECK(FindOtherSlotWithKey(2, 3, 42));
    CHECK_EQ_INT(g_foundSlot, 6);
    CHECK_EQ_INT(g_foundChan, 1);
    CHECK(FindOtherSlotWithKey(6, 1, 42));
    CHECK_EQ_INT(g_foundSlot, 2);
    CHECK_EQ_INT(g_foundChan, 3);
    g_alienGfxSlots[2].loaded[3] = 0;
    CHECK(!FindOtherSlotWithKey(6, 1, 42));
}

TEST(Flow_BumpAlienGfxAge_adds_200_to_the_loaded_owner)
{
    Image *a = FakeImage("a");
    g_alienGfxSlots[1].gfx[2] = a;          // not loaded: skipped
    g_alienGfxSlots[1].count[2] = 1;
    g_alienGfxSlots[4].gfx[2] = a;
    g_alienGfxSlots[4].loaded[2] = 1;
    g_alienGfxSlots[4].count[2] = 7;
    BumpAlienGfxAge(a);
    CHECK_EQ_INT(g_alienGfxSlots[4].count[2], 207);
    CHECK_EQ_INT(g_alienGfxSlots[1].count[2], 1);
}

TEST(Flow_DropAlienGfxAge_subtracts_98_with_a_floor_of_5)
{
    Image *a = FakeImage("a");
    g_alienGfxSlots[3].gfx[0] = a;
    g_alienGfxSlots[3].count[0] = 300;
    DropAlienGfxAge(a);
    CHECK_EQ_INT(g_alienGfxSlots[3].count[0], 202);
    g_alienGfxSlots[3].count[0] = 98;
    DropAlienGfxAge(a);
    CHECK_EQ_INT(g_alienGfxSlots[3].count[0], 0);
    DropAlienGfxAge(a);
    CHECK_EQ_INT(g_alienGfxSlots[3].count[0], 5);
}

TEST(Flow_ReleaseAlienGfxCache_frees_the_six_cache_slots)
{
    for (int i = 0; i < 6; i++) {
        g_alienGfxCache[i].gfx1 = FakeImage("g1");
        g_alienGfxCache[i].gfx2 = FakeImage("g2");
        g_alienGfxMem[i] = malloc(8);
    }
    g_blitCount = 5;
    g_quadCount = 6;
    ReleaseAlienGfxCache();
    for (int i = 0; i < 6; i++) {
        CHECK(g_alienGfxCache[i].gfx1 == NULL);
        CHECK(g_alienGfxCache[i].gfx2 == NULL);
        CHECK(g_alienGfxMem[i] == NULL);
    }
    CHECK_EQ_INT(g_fake.imagesFreed, 12);
    CHECK_EQ_INT(g_alienGfxFreedA, 6);
    CHECK_EQ_INT(g_alienGfxFreedB, 6);
    CHECK_EQ_INT(g_alienGfxFreedM, 6);
    CHECK_EQ_INT(g_blitCount, 0);
    CHECK_EQ_INT(g_quadCount, 0);
}
