// Tests for src/profile/savefile.c (the suspended-game file format) and src/profile/savegame.c
// (SaveProfile, LoadSuspended, ProfileValid, DeleteProfile).
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <zlib.h>
#include "support.h"

enum {
    SAVE_SIZE = 0xa9a18,
    OFS_PLAYERS = 0x10, PLAYER_SIZE = 0x4d8,
    OFS_ENEMIES = 0x1370, ENEMY_SIZE = 0x3a8,
    OFS_LEVELRECS = 0x8a530,
    OFS_CURLEVEL = 0xa9930, OFS_WARP_R = 0xa993c, OFS_WARP_L = 0xa9940,
    OFS_SAVEVERSION = 0xa997c, OFS_RESUME = 0xa99e8, OFS_KILLCOUNT = 0xa9a14,
    MAGIC = 12345
};

static void SetUpDirs(void)
{
    MakeGameDir();
    MakeProfilesDir();
    SeedRand(4321);
    g_saveMagic = MAGIC;
}

static const char *SavePath(void) { return FakeUserPath("warblade\\test.svg"); }
static const char *SvgPath(int slot)
{
    char rel[64];
    snprintf(rel, sizeof rel, "warblade\\profiles\\profile%03d.svg", slot);
    return FakeUserPath(rel);
}

static long HostFileSize(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 ? (long)st.st_size : -1;
}

static void WriteHostFile(const char *path, const void *data, size_t n)
{
    FILE *f = fopen(path, "wb");
    CHECK_MSG(f != NULL, "can't create %s", path);
    fwrite(data, 1, n, f);
    fclose(f);
}

static unsigned char *ReadHostFile(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    unsigned char *buf = malloc(SAVE_SIZE * 2);
    *n = fread(buf, 1, SAVE_SIZE * 2, f);
    fclose(f);
    return buf;
}

// The uncompressed image of a save file (free() it).
static unsigned char *SaveImage(const char *path)
{
    size_t n = 0;
    unsigned char *packed = ReadHostFile(path, &n);
    CHECK_MSG(packed != NULL, "no file %s", path);
    unsigned char *img = calloc(1, SAVE_SIZE + 16);
    uLongf size = SAVE_SIZE + 16;
    CHECK_EQ_INT(uncompress(img, &size, packed, n), Z_OK);
    CHECK_EQ_INT(size, SAVE_SIZE);
    free(packed);
    return img;
}

// Writes `img` (`size` bytes), compressed, as the file at `path`.
static void WriteImage(const char *path, const unsigned char *img, size_t size)
{
    uLongf packed = compressBound(size);
    unsigned char *buf = malloc(packed);
    CHECK_EQ_INT(compress(buf, &packed, img, size), Z_OK);
    WriteHostFile(path, buf, packed);
    free(buf);
}

static int32_t I32(const unsigned char *img, size_t ofs)
{
    int32_t v;
    memcpy(&v, img + ofs, 4);
    return v;
}
static int64_t I64(const unsigned char *img, size_t ofs)
{
    int64_t v;
    memcpy(&v, img + ofs, 8);
    return v;
}

static Image *s_fakeGfx = (Image *)(uintptr_t)0x1234;

// A recognizable game state: every byte of g_save, g_enemies and g_levelRecs patterned, the
// listed globals set to distinct values.
static void FillGameState(void)
{
    unsigned char *p = (unsigned char *)&g_save;
    for (size_t i = 0; i < sizeof g_save; i++)
        p[i] = (unsigned char)(i * 7 + 1);
    p = (unsigned char *)g_enemies;
    for (size_t i = 0; i < sizeof g_enemies; i++)
        p[i] = (unsigned char)(i * 13 + 5);
    p = (unsigned char *)g_levelRecs;
    for (size_t i = 0; i < sizeof g_levelRecs; i++)
        p[i] = (unsigned char)(i * 3 + 2);
    memcpy(g_save.sig, "SDY", 4);
    g_save.saveId = 1234567890123LL;
    g_save.players[0].score = 987654321098LL;
    g_save.players[0].level = 42;
    g_save.players[0].money = 31337;
    g_save.players[0].lives = 3;
    g_save.players[0].x = 123.5f;
    g_save.players[0].gfx = s_fakeGfx;
    g_save.players[3].level = 77;
    g_save.players[3].score = 55;
    g_enemies[0][0].x = 10.25f;
    g_enemies[1][149].x = 99.5f;
    g_enemies[1][149].hp = 4321;
    g_enemies[1][149].active = 1;
    g_levelRecs[0].score = 111;
    g_levelRecs[3999].score = 3999;
    g_curLevelNum = 17;
    g_levelDataLoaded = 1;
    g_loadedLevel = 16;
    g_warpLevelR = 21;
    g_warpLevelL = 22;
    g_marksBonusGiven = 1;
    g_enemyAimAtPlayer = 2;
    g_fastEnemyBullets = 3;
    g_diffEnemyFireChance = 10;
    g_diffShotSpeedMax = 4.3f;
    g_curPlayer = 0;
    g_shopCurPlayer = 1;
    g_saveVersion = MAGIC;
    g_diffScoreBonus = 3;
    g_enemyBulletSpeed = 5.5f;
    g_speedBase = 3.5f;
    g_bonusDuration = 30000;
    g_fireDelayBiasA = -50;
    g_fireDelayBiasB = -60;
    g_resumeTimeOffset = 7777777777LL;
    g_comboLevel = 4;
    g_sessionScore = 4444;
    g_hits = 55;
    g_gameMode = MODE_TIME_TRIAL;
    g_hofMode = 2;
    g_savedDifficultySave = DIFF_HARD;
    g_perfectCount = 6;
    g_killCount = 1001;
}

static void ClearGameState(void)
{
    memset(&g_save, 0, sizeof g_save);
    memset(g_enemies, 0, sizeof g_enemies);
    memset(g_levelRecs, 0, sizeof g_levelRecs);
    g_curLevelNum = g_levelDataLoaded = g_loadedLevel = g_warpLevelR = g_warpLevelL = 0;
    g_marksBonusGiven = g_enemyAimAtPlayer = g_fastEnemyBullets = 0;
    g_diffEnemyFireChance = 0;
    g_diffShotSpeedMax = 0;
    g_shopCurPlayer = 0;
    g_saveVersion = 0;
    g_diffScoreBonus = 0;
    g_enemyBulletSpeed = 0;
    g_speedBase = 0;
    g_bonusDuration = 0;
    g_fireDelayBiasA = g_fireDelayBiasB = 0;
    g_resumeTimeOffset = 0;
    g_comboLevel = g_sessionScore = g_hits = 0;
    g_gameMode = 3;
    g_hofMode = 0;
    g_savedDifficultySave = 0;
    g_perfectCount = g_killCount = 0;
}

// ---------------------------------------------------------------- SaveGameToFile / Load

TEST(Profile_SaveGameToFile_round_trips_the_game_state)
{
    SetUpDirs();
    FillGameState();
    LevelRec *recs = malloc(sizeof g_levelRecs);
    memcpy(recs, g_levelRecs, sizeof g_levelRecs);
    CHECK(SaveGameToFile(SavePath()));
    ClearGameState();
    CHECK(LoadGameFromFile(SavePath()));

    CHECK_STR(g_save.sig, "SDY");
    CHECK_EQ_INT(g_save.saveId, 1234567890123LL);
    CHECK_EQ_INT(g_save.players[0].score, 987654321098LL);
    CHECK_EQ_INT(g_save.players[0].level, 42);
    CHECK_EQ_INT(g_save.players[0].money, 31337);
    CHECK_EQ_INT(g_save.players[0].lives, 3);
    CHECK_NEAR(g_save.players[0].x, 123.5, 0);
    CHECK(g_save.players[0].gfx == NULL);   // pointers aren't saved
    CHECK_EQ_INT(g_save.players[3].level, 77);
    CHECK_EQ_INT(g_save.players[3].score, 55);
    CHECK_NEAR(g_enemies[0][0].x, 10.25, 0);
    CHECK_NEAR(g_enemies[1][149].x, 99.5, 0);
    CHECK_EQ_INT(g_enemies[1][149].hp, 4321);
    CHECK_EQ_INT(g_enemies[1][149].active, 1);
    CHECK_MEM(g_levelRecs, recs, sizeof g_levelRecs);
    CHECK_EQ_INT(g_curLevelNum, 17);
    CHECK_EQ_INT(g_levelDataLoaded, 1);
    CHECK_EQ_INT(g_loadedLevel, 16);
    CHECK_EQ_INT(g_warpLevelR, 21);
    CHECK_EQ_INT(g_warpLevelL, 22);
    CHECK_EQ_INT(g_marksBonusGiven, 1);
    CHECK_EQ_INT(g_enemyAimAtPlayer, 2);
    CHECK_EQ_INT(g_fastEnemyBullets, 3);
    CHECK_EQ_INT(g_diffEnemyFireChance, 10);
    CHECK_NEAR(g_diffShotSpeedMax, 4.3, 1e-6);
    CHECK_EQ_INT(g_shopCurPlayer, 1);
    CHECK_EQ_INT(g_saveVersion, MAGIC);
    CHECK_EQ_INT(g_diffScoreBonus, 3);
    CHECK_NEAR(g_enemyBulletSpeed, 5.5, 0);
    CHECK_NEAR(g_speedBase, 3.5, 0);
    CHECK_EQ_INT(g_bonusDuration, 30000);
    CHECK_EQ_INT(g_fireDelayBiasA, -50);
    CHECK_EQ_INT(g_fireDelayBiasB, -60);
    CHECK_EQ_INT(g_resumeTimeOffset, 7777777777LL);
    CHECK_EQ_INT(g_comboLevel, 4);
    CHECK_EQ_INT(g_sessionScore, 4444);
    CHECK_EQ_INT(g_hits, 55);
    CHECK_EQ_INT(g_gameMode, MODE_TIME_TRIAL);
    CHECK_EQ_INT(g_hofMode, 2);
    CHECK_EQ_INT(g_savedDifficultySave, DIFF_HARD);
    CHECK_EQ_INT(g_perfectCount, 6);
    CHECK_EQ_INT(g_killCount, 1001);
    free(recs);
}

TEST(Profile_SaveGameToFile_load_then_save_gives_the_same_file)
{
    SetUpDirs();
    FillGameState();
    CHECK(SaveGameToFile(SavePath()));
    unsigned char *a = SaveImage(SavePath());
    ClearGameState();
    CHECK(LoadGameFromFile(SavePath()));
    CHECK(SaveGameToFile(SavePath()));
    unsigned char *b = SaveImage(SavePath());
    for (size_t i = 0; i < SAVE_SIZE; i++)
        CHECK_MSG(a[i] == b[i], "byte 0x%zx: 0x%02x then 0x%02x", i, a[i], b[i]);
    free(a);
    free(b);
}

TEST(Profile_SaveGameToFile_uses_the_original_layout)
{
    SetUpDirs();
    FillGameState();
    CHECK(SaveGameToFile(SavePath()));
    unsigned char *img = SaveImage(SavePath());
    CHECK_MEM(img, "SDY", 4);
    CHECK_EQ_INT(I64(img, 8), 1234567890123LL);
    CHECK_EQ_INT(I32(img, OFS_PLAYERS + 0x68), 3);                        // lives
    CHECK_EQ_INT(I64(img, OFS_PLAYERS + 0x78), 987654321098LL);           // score
    CHECK_EQ_INT(I32(img, OFS_PLAYERS + 0xac), 31337);                    // money
    CHECK_EQ_INT(I32(img, OFS_PLAYERS + 0xd4), 42);                       // level
    CHECK_EQ_INT(I32(img, OFS_PLAYERS + 3 * PLAYER_SIZE + 0xd4), 77);     // players[3].level
    CHECK_EQ_INT(I32(img, OFS_PLAYERS + 0x40), 0);                        // dead gfx pointer
    CHECK_EQ_INT(I32(img, OFS_ENEMIES + 299 * ENEMY_SIZE + 0x4), 1);      // [1][149].active
    float hp;
    memcpy(&hp, img + OFS_ENEMIES + 299 * ENEMY_SIZE + 0x164, 4);         // [1][149].hp
    CHECK_NEAR(hp, 4321.0, 0);
    CHECK_EQ_INT(I64(img, OFS_LEVELRECS), 111);
    CHECK_EQ_INT(I64(img, OFS_LEVELRECS + 3999 * 0x20), 3999);
    CHECK_EQ_INT(I32(img, OFS_CURLEVEL), 17);
    CHECK_EQ_INT(I32(img, OFS_WARP_R), 21);
    CHECK_EQ_INT(I32(img, OFS_WARP_L), 22);
    CHECK_EQ_INT(I32(img, OFS_SAVEVERSION), MAGIC);
    CHECK_EQ_INT(I64(img, OFS_RESUME), 7777777777LL);
    CHECK_EQ_INT(I32(img, OFS_KILLCOUNT), 1001);
    // the old dead zone between the enemies and the level records is zero
    for (size_t i = 0x45c50; i < OFS_LEVELRECS; i += 997)
        CHECK_EQ_INT(img[i], 0);
    free(img);
}

TEST(Profile_SaveGameToFile_fails_on_unwritable_path)
{
    SetUpDirs();
    FillGameState();
    CHECK(!SaveGameToFile(FakeUserPath("warblade\\no\\such\\dir\\x.svg")));
}

TEST(Profile_SaveFileValid_accepts_a_current_save)
{
    SetUpDirs();
    FillGameState();
    CHECK(SaveGameToFile(SavePath()));
    g_curLevelNum = 99;
    CHECK(SaveFileValid(SavePath()));
    CHECK_EQ_INT(g_curLevelNum, 99);   // the state is left alone
}

TEST(Profile_SaveFileValid_rejects_missing_or_garbage_files)
{
    SetUpDirs();
    CHECK(!SaveFileValid(SavePath()));
    char junk[5000];
    memset(junk, 0x3c, sizeof junk);
    WriteHostFile(SavePath(), junk, sizeof junk);
    CHECK(!SaveFileValid(SavePath()));
    WriteHostFile(SavePath(), junk, 0);
    CHECK(!SaveFileValid(SavePath()));
}

TEST(Profile_SaveFileValid_rejects_a_truncated_file)
{
    SetUpDirs();
    FillGameState();
    CHECK(SaveGameToFile(SavePath()));
    size_t n = 0;
    unsigned char *packed = ReadHostFile(SavePath(), &n);
    WriteHostFile(SavePath(), packed, n / 2);
    CHECK(!SaveFileValid(SavePath()));
    WriteHostFile(SavePath(), packed, n - 1);
    CHECK(!SaveFileValid(SavePath()));
    free(packed);
}

TEST(Profile_SaveFileValid_rejects_a_short_image)
{
    SetUpDirs();
    FillGameState();
    CHECK(SaveGameToFile(SavePath()));
    unsigned char *img = SaveImage(SavePath());
    WriteImage(SavePath(), img, SAVE_SIZE - 4);
    CHECK(!SaveFileValid(SavePath()));
    WriteImage(SavePath(), img, SAVE_SIZE);
    CHECK(SaveFileValid(SavePath()));
    free(img);
}

TEST(Profile_SaveFileValid_checks_the_SDY_signature)
{
    SetUpDirs();
    FillGameState();
    CHECK(SaveGameToFile(SavePath()));
    unsigned char *img = SaveImage(SavePath());
    for (int i = 0; i < 3; i++) {
        unsigned char keep = img[i];
        img[i] = 'X';
        WriteImage(SavePath(), img, SAVE_SIZE);
        CHECK_MSG(!SaveFileValid(SavePath()), "signature byte %d not checked", i);
        img[i] = keep;
    }
    free(img);
}

TEST(Profile_SaveFileValid_checks_the_save_version)
{
    SetUpDirs();
    FillGameState();
    g_saveVersion = MAGIC - 1;
    CHECK(SaveGameToFile(SavePath()));
    CHECK(!SaveFileValid(SavePath()));
}

TEST(Profile_LoadGameFromFile_invalid_file_changes_nothing)
{
    SetUpDirs();
    FillGameState();
    memcpy(g_save.sig, "SDZ", 4);
    CHECK(SaveGameToFile(SavePath()));
    ClearGameState();
    g_curLevelNum = 5;
    CHECK(!LoadGameFromFile(SavePath()));
    CHECK_EQ_INT(g_curLevelNum, 5);
    CHECK_EQ_INT(g_save.players[0].level, 0);
    CHECK(!LoadGameFromFile(FakeUserPath("warblade\\missing.svg")));
}

// ---------------------------------------------------------------- SaveProfile & co.

// An account in `slot` with 5 lives.
static void MakeProfile(int slot)
{
    ResetAccount();
    snprintf(g_acc.name, sizeof g_acc.name, "PILOT");
    g_acc.lives = 5;
    PackAccount(slot);
    SaveAccount(slot);
}

static void SetUpSuspendable(void)
{
    g_curPlayer = 0;
    memcpy(g_save.sig, "XXX", 4);
    g_save.players[0].level = 12;
    g_save.players[0].score = 5000;
    g_time = 10000;
    g_save.players[0].scoreMult2Timer = 15000;
    g_save.players[0].scoreMult5Timer = 0;
    g_save.players[0].drunkModeTimer = 12000;
    g_save.players[0].mirrorTime = 10500;
    g_save.players[0].shieldTimer = 11000;
    g_timeA = g_fake.fileTime - 5000;
    g_pausedDuration = 1000;
    g_cfg.difficulty = DIFF_HARD;
    g_playerBroke = 0;
    g_freshStart = 0;
}

TEST(Profile_SaveProfile_writes_a_valid_save)
{
    SetUpDirs();
    MakeProfile(1);
    SetUpSuspendable();
    SaveProfile(1);
    CHECK(FakeFileExists(SvgPath(1)));
    CHECK(ProfileValid(1));
    CHECK(!ProfileValid(0));
    CHECK_STR(g_save.sig, "SDY");
    CHECK_EQ_INT(g_saveVersion, MAGIC);
    CHECK(g_save.saveId > 0);
    CHECK_EQ_INT(GetProfileLastSaveId(1), g_save.saveId);
    CHECK_EQ_INT(g_savedDifficultySave, DIFF_HARD);
    CHECK_EQ_INT(g_resumeTimeOffset, 4000);
    CHECK_EQ_INT(GetProfileLives(1), 5);

    unsigned char *img = SaveImage(SvgPath(1));
    CHECK_EQ_INT(I64(img, 8), g_save.saveId);
    CHECK_EQ_INT(I32(img, OFS_PLAYERS + 0xd4), 12);
    free(img);
}

TEST(Profile_SaveProfile_stores_running_timers_as_deltas)
{
    SetUpDirs();
    MakeProfile(1);
    SetUpSuspendable();
    SaveProfile(1);
    CHECK_EQ_INT(g_save.players[0].scoreMult2Timer, 5000);
    CHECK_EQ_INT(g_save.players[0].scoreMult5Timer, 0);
    CHECK_EQ_INT(g_save.players[0].drunkModeTimer, 2000);
    CHECK_EQ_INT(g_save.players[0].mirrorTime, 500);
    CHECK_EQ_INT(g_save.players[0].shieldTimer, 1000);
}

TEST(Profile_SaveProfile_costs_a_life_after_a_loss)
{
    SetUpDirs();
    MakeProfile(1);
    SetUpSuspendable();
    g_playerBroke = 1;
    SaveProfile(1);
    CHECK_EQ_INT(GetProfileLives(1), 4);
    g_playerBroke = 0;
    g_freshStart = 1;
    SaveProfile(1);
    CHECK_EQ_INT(GetProfileLives(1), 3);
}

TEST(Profile_SaveProfile_without_profile_writes_nothing)
{
    SetUpDirs();
    SetUpSuspendable();
    SaveProfile(-1);
    CHECK_STR(g_save.sig, "XXX");
    CHECK(!ProfileValid(-1));
}

TEST(Profile_DeleteProfile_removes_the_save)
{
    SetUpDirs();
    MakeProfile(2);
    SetUpSuspendable();
    SaveProfile(2);
    CHECK(ProfileValid(2));
    DeleteProfile(-1);
    CHECK(FakeFileExists(SvgPath(2)));
    DeleteProfile(2);
    CHECK(!FakeFileExists(SvgPath(2)));
    CHECK(!ProfileValid(2));
}

// ---------------------------------------------------------------- LoadSuspended

static __int64 SuspendAfterBoot(int difficulty)
{
    BootGame();
    SeedRand(77);
    MakeProfile(0);
    g_profileIndex = 0;
    g_gameMode = MODE_SINGLE;
    g_curPlayer = 0;
    g_save.players[0].level = 12;
    g_save.players[0].score = 5000;
    g_save.players[0].energy = 9;
    g_save.players[0].alienLock = 0;
    g_save.players[0].shieldL = 1;
    g_save.players[0].shieldR = 1;
    g_save.players[0].scoreMult2Timer = 0;
    g_save.players[0].scoreMult5Timer = 0;
    g_save.players[0].shieldTimer = g_time + 700;
    g_cfg.difficulty = difficulty;
    SaveProfile(0);
    __int64 id = g_save.saveId;
    // play on a bit, as if elsewhere
    g_save.players[0].level = 1;
    g_save.players[0].score = 0;
    g_cfg.difficulty = DIFF_EASY;
    g_time += 5000;
    return id;
}

TEST(Profile_LoadSuspended_resumes_in_the_shop)
{
    __int64 id = SuspendAfterBoot(DIFF_HARD);
    unsigned t = g_time;
    CHECK(ProfileValid(0));
    LoadSuspended(0);
    CHECK_EQ_INT(g_state, STATE_SHOP);
    CHECK_EQ_INT(g_save.players[0].level, 12);
    CHECK_EQ_INT(g_save.players[0].score, 5000);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_HARD);
    CHECK_EQ_INT(g_cfg.fps, 0x46);
    CHECK_EQ_INT(g_save.players[0].gameSpeedSetting, 0x46);
    CHECK_EQ_INT(g_save.players[0].shieldTimer, 700 + t);
    CHECK_EQ_INT(g_save.players[0].energy, 0);
    CHECK_EQ_INT(g_save.players[0].shieldL, 0);
    CHECK_EQ_INT(g_save.players[0].shieldR, 0);
    CHECK(g_save.players[0].gfx == g_gfxFighter1);
    CHECK(g_save.players[1].gfx == g_gfxFighter2);
    CHECK_NEAR(g_shopTransition, 500.0, 0);
    CHECK_EQ_INT(g_fireDelayMin, 190);   // SetupDifficulty for HARD
    // the save is used up and remembered
    CHECK(!FakeFileExists(SvgPath(0)));
    CHECK(ProfileHistHas(0, id));
}

TEST(Profile_LoadSuspended_restores_score_multipliers)
{
    SuspendAfterBoot(DIFF_EASY);
    // a save with the x2 multiplier running, made by hand
    g_save.players[0].scoreMult2Timer = g_time + 300;
    SaveProfile(0);
    g_scoreMul[0] = 1;
    unsigned t = g_time;
    LoadSuspended(0);
    CHECK_EQ_INT(g_scoreMul[0], 2);
    CHECK_EQ_INT(g_save.players[0].scoreMult2Timer, 300 + t);
    CHECK_EQ_INT(g_cfg.fps, 0x32);

    g_save.players[0].scoreMult2Timer = 0;
    g_save.players[0].scoreMult5Timer = g_time + 200;
    g_cfg.difficulty = DIFF_ACE;
    SaveProfile(0);
    LoadSuspended(0);
    CHECK_EQ_INT(g_scoreMul[0], 5);
    CHECK_EQ_INT(g_cfg.fps, 0x50);
}

TEST(Profile_LoadSuspended_normal_difficulty_runs_at_60)
{
    SuspendAfterBoot(DIFF_NORMAL);
    LoadSuspended(0);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_NORMAL);
    CHECK_EQ_INT(g_cfg.fps, 0x3c);
    CHECK_EQ_INT(g_save.players[0].gameSpeedSetting, 0x3c);
}

TEST(Profile_LoadSuspended_without_valid_save_does_nothing)
{
    SuspendAfterBoot(DIFF_HARD);
    DeleteProfile(0);
    int state = g_state;
    LoadSuspended(0);
    CHECK_EQ_INT(g_state, state);
    CHECK_EQ_INT(g_save.players[0].level, 1);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_EASY);
}
