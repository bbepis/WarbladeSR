// Tests for src/profile/hiscore.c: qualifying (CheckHiscore), name-entry insertion, the packed
// tables (Compress/Decompress), warblade_132.his, reset/clear, best score.
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <zlib.h>
#include "support.h"

enum { HIS_SIZE = 0xa1558, HIS_OLD_SIZE = 0xed6d8, REPLAY_OFS = 0x5158 };

static const char *HisPath(void) { return FakeUserPath("warblade\\warblade_132.his"); }

static void WriteHostFile(const char *path, const void *data, size_t n)
{
    FILE *f = fopen(path, "wb");
    CHECK_MSG(f != NULL, "can't create %s", path);
    fwrite(data, 1, n, f);
    fclose(f);
}

// Compresses `size` bytes of `img` into `out` (at most `max` bytes); returns the packed size.
static size_t Pack(void *out, size_t max, const void *img, size_t size)
{
    uLongf packed = max;
    CHECK_EQ_INT(compress(out, &packed, img, size), Z_OK);
    return packed;
}

// Writes a hiscore file whose image is `img` (`size` bytes).
static void WriteHisImage(const void *img, size_t size)
{
    size_t max = compressBound(size);
    void *buf = malloc(max);
    size_t n = Pack(buf, max, img, size);
    WriteHostFile(HisPath(), buf, n);
    free(buf);
}

// Known tables: table[t][i].score = base[t] - step[t] * i, names "T<t>R<i>".
static const long long kBase[6] = {20000, 2000, 3000, 4000, 500, 900};
static const long long kStep[6] = {1000, 100, 100, 100, 20, 40};

static HiscoreEntry *Entry(int t, int i)
{
    return t < 5 ? &g_hiscoreMagic.table[t][i] : &g_hiscoreMagic.table5[i];
}

static void FillTables(void)
{
    for (int t = 0; t < 6; t++)
        for (int i = 0; i < MAX_HISCORES; i++) {
            HiscoreEntry *e = Entry(t, i);
            e->score = kBase[t] - kStep[t] * i;
            snprintf(e->name, sizeof e->name, "T%dR%d", t, i);
        }
}

static void SetUpTables(void)
{
    MakeGameDir();
    SeedRand(5);
    ResetHiscores();
    DecompressHiscores();
    FillTables();
    CompressHiscores();
    for (int p = 0; p < 4; p++) {
        g_hsTable[p] = 100;
        g_hsRank[p] = g_hsRank2[p] = g_hsPlayer[p] = g_hsPlayer2[p] = -1;
    }
    g_gameMode = MODE_SINGLE;
    g_cfg.difficulty = DIFF_NORMAL;
    memset(g_save.players, 0, sizeof g_save.players);
}

// ---------------------------------------------------------------- CheckHiscore

TEST(Profile_CheckHiscore_finds_the_slot_in_the_difficulty_table)
{
    SetUpTables();
    g_nameLen = 7;
    g_save.players[0].score = 1850;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 2);
    CHECK_EQ_INT(g_hsTable[0], HOF_NORMAL);
    CHECK_EQ_INT(g_hsPlayer[0], 0);
    CHECK_EQ_INT(g_nameLen, 0);
    CHECK_EQ_INT(g_hsRank2[0], -1);
}

TEST(Profile_CheckHiscore_equal_score_takes_the_slot)
{
    SetUpTables();
    g_save.players[0].score = 1800;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 2);
    g_save.players[0].score = 100;   // ties the last entry
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 19);
}

TEST(Profile_CheckHiscore_too_low_does_not_qualify)
{
    SetUpTables();
    g_save.players[0].score = 99;
    CHECK_EQ_INT(CheckHiscore(0), 0);
    CHECK_EQ_INT(g_hsRank[0], -1);
    CHECK_EQ_INT(g_hsTable[0], 100);
}

TEST(Profile_CheckHiscore_zero_score_never_qualifies)
{
    MakeGameDir();
    ResetHiscores();   // all scores 0
    g_hsRank[0] = -1;
    g_gameMode = MODE_SINGLE;
    g_cfg.difficulty = DIFF_NORMAL;
    g_save.players[0].score = 0;
    CHECK_EQ_INT(CheckHiscore(0), 0);
    CHECK_EQ_INT(g_hsRank[0], -1);
    g_save.players[0].score = 1;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 0);
}

TEST(Profile_CheckHiscore_uses_each_difficultys_table)
{
    static const struct { int diff; long long score; int rank; } c[] = {
        {DIFF_EASY, 18500, 2}, {DIFF_NORMAL, 1650, 4}, {DIFF_HARD, 2950, 1}, {DIFF_ACE, 3250, 8},
    };
    for (int k = 0; k < 4; k++) {
        SetUpTables();
        g_cfg.difficulty = c[k].diff;
        g_save.players[1].score = c[k].score;
        CHECK_EQ_INT(CheckHiscore(1), 1);
        CHECK_MSG(g_hsRank[1] == c[k].rank, "difficulty %d: rank %d, expected %d", c[k].diff,
                  g_hsRank[1], c[k].rank);
        CHECK_EQ_INT(g_hsTable[1], c[k].diff);
        CHECK_EQ_INT(g_hsPlayer[1], 1);
        CHECK_EQ_INT(g_hsRank[0], -1);
    }
}

TEST(Profile_CheckHiscore_new_first_place_keeps_the_runs_level_records)
{
    SetUpTables();
    g_levelRecs[7].score = 777;
    g_levelRecs[3999].money = 42;
    g_save.players[0].score = 5000;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 0);
    DecompressHiscores();
    CHECK_EQ_INT(g_replayRecs[1][7].score, 777);
    CHECK_EQ_INT(g_replayRecs[1][3999].money, 42);
    CHECK_EQ_INT(g_replayRecs[0][7].score, 0);
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 2000);   // the table itself isn't changed
}

TEST(Profile_CheckHiscore_second_place_keeps_no_records)
{
    SetUpTables();
    g_levelRecs[7].score = 777;
    g_save.players[0].score = 1950;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 1);
    DecompressHiscores();
    CHECK_EQ_INT(g_replayRecs[1][7].score, 0);
}

TEST(Profile_CheckHiscore_time_trial_table)
{
    SetUpTables();
    g_gameMode = MODE_TIME_TRIAL;
    g_save.players[0].score = 860;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 1);
    CHECK_EQ_INT(g_hsPlayer[0], 0);
    CHECK_EQ_INT(g_hsTable[0], 100);   // not a difficulty table

    g_hsRank[0] = -1;
    g_save.players[0].score = 139;
    CHECK_EQ_INT(CheckHiscore(0), 0);
    CHECK_EQ_INT(g_hsRank[0], -1);
}

TEST(Profile_CheckHiscore_time_trial_first_place_keeps_records)
{
    SetUpTables();
    g_gameMode = MODE_TIME_TRIAL;
    g_levelRecs[3].score = 333;
    g_save.players[0].score = 901;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 0);
    DecompressHiscores();
    CHECK_EQ_INT(g_replayRecs[4][3].score, 333);
}

TEST(Profile_CheckHiscore_meteorstorm_table_qualifies_on_its_own)
{
    SetUpTables();
    g_save.players[2].bonusHighScore = 470;
    g_save.players[2].score = 0;
    CHECK_EQ_INT(CheckHiscore(2), 1);
    CHECK_EQ_INT(g_hsRank2[2], 2);
    CHECK_EQ_INT(g_hsPlayer2[2], 2);
    CHECK_EQ_INT(g_hsRank[2], -1);

    g_hsRank2[2] = -1;
    g_save.players[2].bonusHighScore = 100;   // below the last (120)
    CHECK_EQ_INT(CheckHiscore(2), 0);
    CHECK_EQ_INT(g_hsRank2[2], -1);
}

// ---------------------------------------------------------------- name entry (insertion)

static void BootForEntry(int mode)
{
    BootGame();
    SetUpTables();
    g_gameMode = mode;
    g_curPlayer = 0;
    g_profileIndex = -1;
    g_save.players[0].score = 1850;
    g_save.players[0].displayLevel = 9;
    g_save.players[0].rank = 4;
    g_save.players[0].buffDuration = 6;
    g_save.players[0].hits = 30;
    g_save.players[0].shots = 60;
    g_save.players[0].sessionPlayTime = 1234;
    g_timeStampA = 1000;
    g_timeStampB = 6000;
    g_pausedDuration = 1000;
    for (int i = 0; i < NAME_LEN; i++)
        g_name[i] = '_';
    memcpy(g_name, "BOB", 3);
    g_name[NAME_LEN] = 0;
    g_hsEntryIndex = -1;
    g_hiscoreInsertGate = 0;
    g_inputCooldown = 0;
}

static void CheckBob(const HiscoreEntry *e, long long score)
{
    char want[NAME_LEN];
    memset(want, ' ', sizeof want);
    memcpy(want, "BOB", 3);
    CHECK_MEM(e->name, want, NAME_LEN);
    CHECK_EQ_INT(e->score, score);
    CHECK_EQ_INT(e->level, 9);
    CHECK_EQ_INT(e->rank, 4);
    CHECK_EQ_INT(e->power, 6);
    CHECK_EQ_INT(e->hits, 30);
    CHECK_EQ_INT(e->shots, 60);
    CHECK_EQ_INT(e->highlight, 1);
    CHECK_EQ_INT(e->duration, 4000);
    CHECK_EQ_INT(e->ownerStamp, 1234);
    CHECK_EQ_INT(e->year, 2009);
}

TEST(Profile_EnterHiscore_inserts_and_shifts_the_table)
{
    BootForEntry(MODE_SINGLE);
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 2);
    FakePressKey(K_VK_RETURN);
    EnterHiscore();
    CHECK_EQ_INT(g_hsEntryIndex, 0);
    CHECK_EQ_INT(g_hsTable[0], 100);
    CHECK_EQ_INT(g_state, STATE_TITLE);
    CHECK_EQ_INT(g_curPlayer, -1);

    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 2000);
    CHECK_EQ_INT(g_hiscoreMagic.table1[1].score, 1900);
    CheckBob(&g_hiscoreMagic.table1[2], 1850);
    CHECK_EQ_INT(g_hiscoreMagic.table1[3].score, 1800);
    CHECK_STR(g_hiscoreMagic.table1[3].name, "T1R2");
    CHECK_EQ_INT(g_hiscoreMagic.table1[19].score, 200);
    CHECK_STR(g_hiscoreMagic.table1[19].name, "T1R18");
    CHECK_EQ_INT(g_hiscoreMagic.table2[2].score, 2800);   // other tables untouched
}

TEST(Profile_EnterHiscore_writes_the_hiscore_file)
{
    BootForEntry(MODE_SINGLE);
    CheckHiscore(0);
    FakePressKey(K_VK_RETURN);
    EnterHiscore();
    CHECK(FakeFileExists(HisPath()));
    memset(g_hiscoreBuf, 0, 0x1000);
    LoadHiscores();
    DecompressHiscores();
    CheckBob(&g_hiscoreMagic.table1[2], 1850);
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 2000);
}

TEST(Profile_EnterHiscore_inserts_meteorstorm_score)
{
    BootForEntry(MODE_SINGLE);
    g_save.players[0].score = 0;
    g_save.players[0].bonusHighScore = 470;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    FakePressKey(K_VK_RETURN);
    EnterHiscore();
    CHECK_EQ_INT(g_hsRank2[0], -1);
    DecompressHiscores();
    CheckBob(&g_hiscoreMagic.table4[2], 470);
    CHECK_EQ_INT(g_hiscoreMagic.table4[3].score, 460);
    CHECK_EQ_INT(g_hiscoreMagic.table1[2].score, 1800);
}

TEST(Profile_EnterHiscore_time_trial_inserts_into_its_table)
{
    BootForEntry(MODE_TIME_TRIAL);
    g_save.players[0].score = 860;
    CHECK_EQ_INT(CheckHiscore(0), 1);
    FakePressKey(K_VK_RETURN);
    EnterHiscore();
    CHECK_EQ_INT(g_hsRank[0], -1);
    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table5[0].score, 900);
    CheckBob(&g_hiscoreMagic.table5[1], 860);
    CHECK_EQ_INT(g_hiscoreMagic.table5[2].score, 860);
    CHECK_STR(g_hiscoreMagic.table5[2].name, "T5R1");
    CHECK_EQ_INT(g_hiscoreMagic.table1[1].score, 1900);
}

TEST(Profile_EnterHiscore_inserts_at_the_last_slot)
{
    BootForEntry(MODE_SINGLE);
    g_cfg.difficulty = DIFF_ACE;
    g_save.players[0].score = 2150;   // ACE: 4000 ... 2100
    CHECK_EQ_INT(CheckHiscore(0), 1);
    CHECK_EQ_INT(g_hsRank[0], 19);
    FakePressKey(K_VK_RETURN);
    EnterHiscore();
    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table3[18].score, 2200);
    CheckBob(&g_hiscoreMagic.table3[19], 2150);
}

// ---------------------------------------------------------------- packing

TEST(Profile_CompressHiscores_round_trip)
{
    MakeGameDir();
    ResetHiscores();
    DecompressHiscores();
    FillTables();
    g_hiscoreMagic.count = 3;
    memcpy(g_hiscoreMagic.tag, "ABCD", 5);
    g_replayRecs[2][100].score = 4242;
    g_replayRecs[4][3999].shots = 17;
    static HiscoreData want;
    want = g_hiscoreMagic;
    CompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 0);   // cleared once packed
    CHECK_EQ_INT(g_replayRecs[2][100].score, 0);
    CHECK_EQ_INT(g_hiscoresCleared, 1);
    CHECK(g_hiscorePackedSize > 0 && g_hiscorePackedSize < HIS_SIZE);
    DecompressHiscores();
    CHECK_MEM(&g_hiscoreMagic, &want, sizeof want);
    CHECK_EQ_INT(g_replayRecs[2][100].score, 4242);
    CHECK_EQ_INT(g_replayRecs[4][3999].shots, 17);
    CHECK_EQ_INT(g_hiscoreSize, HIS_SIZE);
}

TEST(Profile_DecompressHiscores_garbage_clears_the_tables)
{
    FillTables();
    g_replayRecs[0][0].score = 5;
    g_hiscoresCleared = 0;
    memset(g_hiscoreBuf, 0x6b, 0x10000);
    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 0);
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].name[0], 0);
    CHECK_EQ_INT(g_replayRecs[0][0].score, 0);
    CHECK_EQ_INT(g_hiscoresCleared, 1);
}

TEST(Profile_DecompressHiscores_prefers_the_old_layout_buffer)
{
    static unsigned char img[HIS_OLD_SIZE];
    memset(img, 0, sizeof img);
    FillTables();
    memcpy(g_hiscoreMagic.tag, "OLDX", 5);
    g_hiscoreMagic.tag[4] = 'Q';
    g_hiscoreMagic.count = 9;
    memcpy(img, &g_hiscoreMagic, sizeof g_hiscoreMagic);
    memset(img + sizeof g_hiscoreMagic, 0x11, 1000);   // the dropped rest
    Pack(g_hiscorePackedOld, 972504, img, sizeof img);
    memset(&g_hiscoreMagic, 0, sizeof g_hiscoreMagic);
    g_replayRecs[1][1].score = 9;

    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreSize, HIS_OLD_SIZE);
    for (int t = 0; t < 6; t++) {
        CHECK_EQ_INT(Entry(t, 0)->score, kBase[t]);
        CHECK_EQ_INT(Entry(t, 19)->score, kBase[t] - kStep[t] * 19);
    }
    CHECK_STR(g_hiscoreMagic.table5[4].name, "T5R4");
    CHECK_MEM(g_hiscoreMagic.tag, "OLDX", 4);
    CHECK_EQ_INT(g_hiscoreMagic.tag[4], 0);
    CHECK_EQ_INT(g_hiscoreMagic.count, 9);
    CHECK_EQ_INT(g_replayRecs[1][1].score, 0);
}

// ---------------------------------------------------------------- reset / clear

TEST(Profile_ResetHiscores_blanks_every_table)
{
    MakeGameDir();
    for (int t = 0; t < 6; t++)
        for (int i = 0; i < MAX_HISCORES; i++) {
            HiscoreEntry *e = Entry(t, i);
            memset(e, 0x7f, sizeof *e);
            e->name[3] = 0;
        }
    g_hiscoreMagic.count = 4;
    ResetHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 0);   // packed and cleared
    DecompressHiscores();
    for (int t = 0; t < 6; t++)
        for (int i = 0; i < MAX_HISCORES; i++) {
            HiscoreEntry *e = Entry(t, i);
            CHECK_MSG(!strcmp(e->name, " "), "table %d #%d name \"%s\"", t, i, e->name);
            CHECK_MSG(e->score == 0 && e->hits == 0 && e->shots == 0 && e->highlight == 0 &&
                          e->duration == 0 && e->year == 0 && e->ownerStamp == 0,
                      "table %d #%d not blank", t, i);
            if (t < 5)
                CHECK_MSG(e->level == 0 && e->rank == 0 && e->power == 0,
                          "table %d #%d level/rank/power not blank", t, i);
        }
    CHECK_EQ_INT(g_hiscoreMagic.count, 0);
}

TEST(Profile_ClearHiscores_zeroes_tables_and_records)
{
    FillTables();
    g_replayRecs[3][3].score = 3;
    g_hiscoresCleared = 0;
    ClearHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table[0][0].score, 0);
    CHECK_EQ_INT(g_hiscoreMagic.table5[19].score, 0);
    CHECK_EQ_INT(g_hiscoreMagic.table3[5].name[0], 0);
    CHECK_EQ_INT(g_replayRecs[3][3].score, 0);
    CHECK_EQ_INT(g_hiscoresCleared, 1);
}

TEST(Profile_ClearHiscoreHighlight_clears_the_right_table)
{
    for (int t = 0; t < 6; t++)
        for (int i = 0; i < MAX_HISCORES; i++)
            Entry(t, i)->highlight = 1;
    g_hsTable[0] = HOF_HARD;
    ClearHiscoreHighlight(0);
    CHECK_EQ_INT(g_hiscoreMagic.table2[0].highlight, 0);
    CHECK_EQ_INT(g_hiscoreMagic.table2[19].highlight, 0);
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].highlight, 1);
    CHECK_EQ_INT(g_hiscoreMagic.table3[0].highlight, 1);

    g_hsRank2[1] = -1;
    ClearMoneyHiscoreHighlight(1);
    CHECK_EQ_INT(g_hiscoreMagic.table4[0].highlight, 1);
    g_hsRank2[1] = 4;
    ClearMoneyHiscoreHighlight(1);
    CHECK_EQ_INT(g_hiscoreMagic.table4[19].highlight, 0);

    g_hsRank[1] = 2;
    ClearHiscoreHighlightTT(1);
    CHECK_EQ_INT(g_hiscoreMagic.table5[0].highlight, 0);
    CHECK_EQ_INT(g_hiscoreMagic.table[0][0].highlight, 1);
}

// ---------------------------------------------------------------- the file

TEST(Profile_WriteHiscoreFile_LoadHiscores_round_trip)
{
    MakeGameDir();
    ResetHiscores();
    DecompressHiscores();
    FillTables();
    g_hiscoreMagic.count = 5;
    g_replayRecs[0][12].score = 1212;
    CompressHiscores();
    WriteHiscoreFile();
    CHECK(FakeFileExists(HisPath()));
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 0);   // left packed

    memset(g_hiscoreBuf, 0, 0x10000);
    LoadHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 0);   // left packed
    DecompressHiscores();
    CHECK_MEM(g_hiscoreMagic.tag, "WARX", 5);
    CHECK_EQ_INT(g_hiscoreMagic.count, 0);
    for (int t = 0; t < 6; t++)
        for (int i = 0; i < MAX_HISCORES; i++) {
            char want[16];
            snprintf(want, sizeof want, "T%dR%d", t, i);
            CHECK_STR(Entry(t, i)->name, want);
            CHECK_EQ_INT(Entry(t, i)->score, kBase[t] - kStep[t] * i);
        }
    CHECK_EQ_INT(g_replayRecs[0][12].score, 1212);
}

TEST(Profile_LoadHiscores_missing_file_gives_blank_tables)
{
    MakeGameDir();
    FillTables();
    CompressHiscores();
    LoadHiscores();
    DecompressHiscores();
    CHECK_STR(g_hiscoreMagic.table1[0].name, " ");
    CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 0);
    CHECK_STR(g_hiscoreMagic.table5[19].name, " ");
    CHECK(!FakeFileExists(HisPath()));
}

// A full-size image with the known tables and `tag`.
static unsigned char *TaggedImage(const char *tag)
{
    unsigned char *img = calloc(1, HIS_SIZE);
    memset(&g_hiscoreMagic, 0, sizeof g_hiscoreMagic);
    FillTables();
    memcpy(g_hiscoreMagic.tag, tag, 4);
    memcpy(img, &g_hiscoreMagic, sizeof g_hiscoreMagic);
    LevelRec r;
    memset(&r, 0, sizeof r);
    r.score = 31;
    memcpy(img + REPLAY_OFS, &r, sizeof r);
    return img;
}

TEST(Profile_LoadHiscores_accepts_a_WARX_file)
{
    MakeGameDir();
    unsigned char *img = TaggedImage("WARX");
    WriteHisImage(img, HIS_SIZE);
    LoadHiscores();
    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table2[0].score, 3000);
    CHECK_EQ_INT(g_replayRecs[0][0].score, 31);
    free(img);
}

TEST(Profile_LoadHiscores_rejects_another_tag)
{
    MakeGameDir();
    const char *tags[] = {"WARY", "WAXX", "WRRX", "XARX"};
    for (int k = 0; k < 4; k++) {
        unsigned char *img = TaggedImage(tags[k]);
        WriteHisImage(img, HIS_SIZE);
        LoadHiscores();
        DecompressHiscores();
        CHECK_MSG(g_hiscoreMagic.table2[0].score == 0, "tag %s accepted", tags[k]);
        CHECK_STR(g_hiscoreMagic.table2[0].name, " ");
        free(img);
    }
}

TEST(Profile_LoadHiscores_rejects_another_size)
{
    MakeGameDir();
    unsigned char *img = TaggedImage("WARX");
    WriteHisImage(img, HIS_SIZE - 8);
    LoadHiscores();
    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table2[0].score, 0);
    CHECK_STR(g_hiscoreMagic.table2[0].name, " ");
    free(img);
}

TEST(Profile_LoadHiscores_accepts_the_short_0x30c8_format)
{
    MakeGameDir();
    unsigned char *img = TaggedImage("NONE");   // no tag check for this format
    WriteHisImage(img, 0x30c8);
    LoadHiscores();
    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreSize, 0x30c8);
    CHECK_EQ_INT(g_hiscoreMagic.table2[0].score, 3000);
    CHECK_EQ_INT(g_hiscoreMagic.table4[19].score, 120);
    CHECK_EQ_INT(g_replayRecs[0][0].score, 0);
    free(img);
}

TEST(Profile_LoadHiscores_garbage_file_gives_blank_tables)
{
    MakeGameDir();
    char junk[4096];
    memset(junk, 0x2e, sizeof junk);
    WriteHostFile(HisPath(), junk, sizeof junk);
    FillTables();
    CompressHiscores();
    LoadHiscores();
    DecompressHiscores();
    CHECK_STR(g_hiscoreMagic.table0[0].name, " ");
    CHECK_EQ_INT(g_hiscoreMagic.table0[0].score, 0);
}

// ---------------------------------------------------------------- best score

TEST(Profile_LoadBestScore_takes_the_first_place_of_the_difficulty)
{
    MakeGameDir();
    SetUpTables();
    long long want[4] = {20000, 2000, 3000, 4000};
    for (int d = 0; d < 4; d++) {
        g_bestScore = 0;
        g_cfg.difficulty = d;
        LoadBestScore();
        CHECK_MSG(g_bestScore == want[d], "difficulty %d: best %lld", d, (long long)g_bestScore);
        CHECK_EQ_INT(g_hiscoreMagic.table1[0].score, 0);   // cleared again
    }
    g_gameMode = MODE_TIME_TRIAL;
    g_cfg.difficulty = DIFF_HARD;
    LoadBestScore();
    CHECK_EQ_INT(g_bestScore, 900);
}

TEST(Profile_UpdateBestScore_follows_the_players)
{
    g_bestScore = 1000;
    g_save.players[0].score = 900;
    g_save.players[1].score = 950;
    UpdateBestScore();
    CHECK_EQ_INT(g_bestScore, 1000);
    g_save.players[0].score = 1200;
    UpdateBestScore();
    CHECK_EQ_INT(g_bestScore, 1200);
    g_save.players[1].score = 1300;
    UpdateBestScore();
    CHECK_EQ_INT(g_bestScore, 1300);
}
