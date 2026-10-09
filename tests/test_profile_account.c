// Tests for src/profile/account.c: profile accounts (.acc files), their upgrade chain,
// scanning/renumbering, per-profile getters/setters, setpro.dat, logout.
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <zlib.h>
#include "support.h"

enum { ACC_SIZE = 0x40d0, ACC_V2 = 0x3c00, ACC_V2_NEWS = 0x3c08 };

static void SetUpDirs(void)
{
    MakeGameDir();
    MakeProfilesDir();
    SeedRand(1234);
}

static const char *SlotPath(const char *dir, int slot, const char *ext)
{
    char rel[128];
    snprintf(rel, sizeof rel, "warblade\\%s\\profile%03d.%s", dir, slot, ext);
    return FakeUserPath(rel);
}
static const char *AccPath(int slot) { return SlotPath("profiles", slot, "acc"); }

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

static size_t ReadHostFile(const char *path, void *buf, size_t max)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    size_t n = fread(buf, 1, max, f);
    fclose(f);
    return n;
}

// Writes `size` bytes of `acc`, zlib-compressed, to `path`, padded with zeros to `fileSize`
// bytes (the original's account files are that long whatever the packed size).
static void WriteAccountFile(const char *path, const void *acc, size_t size, size_t fileSize)
{
    static unsigned char buf[ACC_SIZE * 2];
    uLongf packed = sizeof buf;
    memset(buf, 0, sizeof buf);
    CHECK_EQ_INT(compress(buf, &packed, acc, size), Z_OK);
    CHECK(packed <= fileSize);
    WriteHostFile(path, buf, fileSize);
}

// Unpacks the account file at `path` into *out.
static bool ReadAccountFile(const char *path, Account *out)
{
    static unsigned char buf[ACC_SIZE];
    size_t n = ReadHostFile(path, buf, sizeof buf);
    uLongf size = sizeof *out;
    memset(out, 0, sizeof *out);
    return n > 0 && uncompress((unsigned char *)out, &size, buf, n) == Z_OK && size == ACC_SIZE;
}

// A fresh account named `name` in slot `slot`, packed and saved.
static void MakeProfile(int slot, const char *name)
{
    ResetAccount();
    snprintf(g_acc.name, sizeof g_acc.name, "%s", name);
    PackAccount(slot);
    SaveAccount(slot);
}

// Forgets slot's packed account and reads it back from its file, as ScanProfiles does.
static void ReloadSlot(int slot)
{
    memset(&g_accBuf[slot], 0, ACC_SIZE);
    CHECK(ReadHostFile(AccPath(slot), &g_accBuf[slot], ACC_SIZE) > 0);
    g_accSizes[slot] = ACC_SIZE;
}

// Decodes `acc` (compressed first) as if read from a file of `len` bytes.
static bool Decode(const Account *acc, int len)
{
    static unsigned char buf[ACC_SIZE * 2];
    uLongf packed = sizeof buf;
    memset(buf, 0, sizeof buf);
    CHECK_EQ_INT(compress(buf, &packed, (const unsigned char *)acc, ACC_SIZE), Z_OK);
    g_accSizes[0] = ACC_SIZE;
    return DecodeAccount(buf, 0, len);
}

// Compresses the v0 score-layout prefix inside a known account-file container size.
static bool DecodeV0Account(const void *account)
{
    static unsigned char buf[ACC_V2_NEWS];
    uLongf packed = sizeof buf;
    memset(buf, 0, sizeof buf);
    CHECK_EQ_INT(compress(buf, &packed, account, sizeof(AccountV0)), Z_OK);
    // Historical files are read using their padded container length; remaining bytes are 0.
    g_accSizes[0] = ACC_V2_NEWS;
    return DecodeAccount(buf, 0, ACC_V2_NEWS);
}

// An account of version `version` with recognizable fields.
static Account OldAccount(int version)
{
    Account a;
    memset(&a, 0, sizeof a);
    a.version = version;
    snprintf(a.name, sizeof a.name, "OLDTIMER");
    a.highScore = 4242;
    a.ownerStamp = 555;
    a.unlockedRank = 3;
    return a;
}

// ---------------------------------------------------------------- Reset / pack / save

TEST(Profile_ResetAccount_sets_new_profile_values)
{
    SetUpDirs();
    memset(&g_acc, 0x55, sizeof g_acc);
    DefaultSettings();
    g_cfg.sfxVol = 0x12;
    g_cfg.best = 777;
    ResetAccount();
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_EQ_INT(g_acc.unlockedRank, RANK_GRANDMASTER_3);
    CHECK_EQ_INT(g_acc.unusedC4, 30);
    CHECK_EQ_INT(g_acc.unusedDc, 1);
    CHECK_EQ_INT(g_acc.medalStep, -1);
    CHECK_EQ_INT(g_acc.bestLevelTime, 999999999);
    CHECK_EQ_INT(g_acc.bestMeteorstormTime, 999999999);
    CHECK_EQ_INT(g_acc.highScore, 0);
    CHECK_EQ_INT(g_acc.lives, 0);
    CHECK_EQ_INT(g_acc.lastSaveId, 0);
    CHECK_EQ_INT(g_acc.saveIdHistory[0], 0);
    CHECK_EQ_INT(g_acc.saveIdHistory[49], 0);
    CHECK_EQ_INT(g_acc.levelDoneBytes[49], 0);
    CHECK_EQ_INT(g_acc.name[0], 0);
    CHECK(g_acc.ownerStamp > 0 && g_acc.ownerStamp <= 9999999999999LL);
    CHECK_EQ_INT(g_acc.created.year, 2009);
    CHECK_EQ_INT(g_acc.created.month, 6);
    CHECK_EQ_INT(g_acc.created.day, 15);
    CHECK_EQ_INT(g_acc.settings.sfxVol, 0x12);
    CHECK_EQ_INT(g_acc.settings.best, 777);
}

TEST(Profile_PackAccount_SaveAccount_writes_compressed_account)
{
    SetUpDirs();
    ResetAccount();
    snprintf(g_acc.name, sizeof g_acc.name, "MAVERICK");
    g_acc.highScore = 123456789012LL;
    g_acc.saveIdHistory[49] = 99;
    Account want = g_acc;
    PackAccount(2);
    CHECK_EQ_INT(g_acc.version, 0);   // cleared after packing
    CHECK_EQ_INT(g_acc.highScore, 0);
    CHECK(g_accSizes[2] > 0 && g_accSizes[2] < ACC_SIZE);
    CHECK(!FakeFileExists(AccPath(2)));
    SaveAccount(2);
    // the whole buffer is written, whatever the packed size
    CHECK_EQ_INT(HostFileSize(AccPath(2)), ACC_SIZE);
    Account got;
    CHECK(ReadAccountFile(AccPath(2), &got));
    CHECK_MEM(&got, &want, ACC_SIZE);
}

TEST(Profile_SaveAccount_negative_slot_writes_nothing)
{
    SetUpDirs();
    SaveAccount(-1);
    CHECK(!FakeFileExists(FakeUserPath("warblade\\profiles\\profile-01.acc")));
    CHECK(!FakeFileExists(AccPath(0)));
}

TEST(Profile_UnpackAccount_restores_packed_account)
{
    SetUpDirs();
    ResetAccount();
    snprintf(g_acc.name, sizeof g_acc.name, "GOOSE");
    g_acc.voiceIndex = 3;
    Account want = g_acc;
    PackAccount(4);
    UnpackAccount(4);
    CHECK_MEM(&g_acc, &want, ACC_SIZE);
}

TEST(Profile_UnpackAccount_corrupt_buffer_resets_and_saves_profile)
{
    SetUpDirs();
    memset(&g_accBuf[1], 0x5a, ACC_SIZE);
    UnpackAccount(1);
    // the fresh account was packed (which clears g_acc) and saved
    CHECK_EQ_INT(g_acc.version, 0);
    CHECK(FakeFileExists(AccPath(1)));
    Account got;
    CHECK(ReadAccountFile(AccPath(1), &got));
    CHECK_EQ_INT(got.version, 7);
    CHECK_EQ_INT(got.medalStep, -1);
    UnpackAccount(1);
    CHECK_EQ_INT(g_acc.version, 7);
}

TEST(Profile_NextAccountReset_allocates_up_to_ten)
{
    g_profileCount = 8;
    g_acc.version = 7;
    CHECK_EQ_INT(NextAccountReset(), 9);
    CHECK_EQ_INT(g_acc.version, 0);
    CHECK_EQ_INT(NextAccountReset(), 10);
    CHECK_EQ_INT(g_profileCount, 10);
    CHECK_EQ_INT(NextAccountReset(), -1);
    CHECK_EQ_INT(g_profileCount, 10);
}

TEST(Profile_GetAccountTime_returns_owner_stamp)
{
    SetUpDirs();
    ResetAccount();
    g_acc.ownerStamp = 31337;
    PackAccount(0);
    CHECK_EQ_INT(GetAccountTime(0), 31337);
    CHECK_EQ_INT(g_acc.ownerStamp, 0);   // cleared again
    CHECK_EQ_INT(GetAccountTime(-1), 0);
}

// ---------------------------------------------------------------- backups

TEST(Profile_LoadAccount_reads_backup_file_packed)
{
    SetUpDirs();
    SysMakeDir(FakeUserPath("warblade\\backup"));
    Account a = OldAccount(7);
    WriteAccountFile(SlotPath("backup", 3, "acc"), &a, ACC_SIZE, ACC_SIZE);
    static unsigned char file[ACC_SIZE];
    ReadHostFile(SlotPath("backup", 3, "acc"), file, ACC_SIZE);

    CHECK_EQ_INT(LoadAccount(3), ACC_SIZE);
    CHECK_EQ_INT(g_accSizes[3], ACC_SIZE);
    CHECK_MEM(&g_accBuf[3], file, ACC_SIZE);
    CHECK_EQ_INT(LoadAccount(4), 0);   // no backup
}

TEST(Profile_LoadBackupAccount_decodes_and_returns_owner_stamp)
{
    SetUpDirs();
    SysMakeDir(FakeUserPath("warblade\\backup"));
    CHECK_EQ_INT(LoadBackupAccount(2), 0);   // no backup yet

    Account a = OldAccount(7);
    a.ownerStamp = 8888;
    WriteAccountFile(SlotPath("backup", 2, "acc"), &a, ACC_SIZE, ACC_SIZE);
    g_accSizes[2] = ACC_SIZE;   // as GetAccountTime leaves it in the restore flow
    CHECK_EQ_INT(LoadBackupAccount(2), 8888);
    CHECK_STR(g_acc.name, "OLDTIMER");
    CHECK_EQ_INT(g_acc.highScore, 4242);
}

TEST(Profile_LoadBackupAccount_corrupt_backup_gives_fresh_account)
{
    SetUpDirs();
    SysMakeDir(FakeUserPath("warblade\\backup"));
    char junk[ACC_SIZE];
    memset(junk, 0x33, sizeof junk);
    WriteHostFile(SlotPath("backup", 1, "acc"), junk, sizeof junk);
    g_accSizes[1] = ACC_SIZE;
    __int64 t = LoadBackupAccount(1);
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK(t > 0);
    CHECK_EQ_INT(t, g_acc.ownerStamp);
}

// ---------------------------------------------------------------- DecodeAccount

TEST(Profile_DecodeAccount_current_version_is_kept)
{
    SetUpDirs();
    ResetAccount();
    snprintf(g_acc.name, sizeof g_acc.name, "ICEMAN");
    g_acc.highScore = 1000000;
    g_acc.saveIdHistory[10] = 1234;
    g_acc.lastSaveId = 5;
    Account a = g_acc;
    memset(&g_acc, 0, sizeof g_acc);
    CHECK(Decode(&a, ACC_SIZE));
    CHECK_MEM(&g_acc, &a, ACC_SIZE);
}

TEST(Profile_DecodeAccount_garbage_returns_false)
{
    static char junk[ACC_SIZE];
    memset(junk, 0x77, sizeof junk);
    g_accSizes[0] = ACC_SIZE;
    CHECK(!DecodeAccount(junk, 0, ACC_SIZE));
}

TEST(Profile_DecodeAccount_v6_gains_empty_save_history)
{
    SetUpDirs();
    Account a = OldAccount(6);
    for (int i = 0; i < 50; i++)
        a.saveIdHistory[i] = 100 + i;
    a.lastSaveId = 77;
    CHECK(Decode(&a, ACC_SIZE));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_EQ_INT(g_acc.saveIdHistory[0], 0);
    CHECK_EQ_INT(g_acc.saveIdHistory[49], 0);
    CHECK_EQ_INT(g_acc.lastSaveId, 0);
    CHECK_STR(g_acc.name, "OLDTIMER");
    CHECK_EQ_INT(g_acc.highScore, 4242);
}

TEST(Profile_DecodeAccount_unknown_version_resets)
{
    SetUpDirs();
    Account a = OldAccount(8);
    CHECK(Decode(&a, ACC_SIZE));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_EQ_INT(g_acc.name[0], 0);
    CHECK_EQ_INT(g_acc.highScore, 0);
}

TEST(Profile_DecodeAccount_v5_of_current_size_resets)
{
    // a v5 account must come from a 0x3c00- or 0x3c08-byte file to be upgraded
    SetUpDirs();
    Account a = OldAccount(5);
    CHECK(Decode(&a, ACC_SIZE));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_EQ_INT(g_acc.name[0], 0);
}

TEST(Profile_DecodeAccount_v5_news_layout_upgrade)
{
    SetUpDirs();
    g_numLevels = 30;
    Account a = OldAccount(5);
    a.settings.checkVersion = 1;
    for (int i = 0; i < 4; i++) {
        a.settings.newsIds[i] = 9;
        a.settings.newsCounts[i] = 9;
    }
    a.medalOrder[3] = 4;
    a.medals = 0xff;
    a.medalStep = 3;
    a.cheatDetected = 1;
    a.gameCompletedByte = 1;
    a.highestMoney = 5000;
    a.secretsInOneGame = 3;
    memset(a.levelDoneBytes, 1, sizeof a.levelDoneBytes);
    a.bonusLevelsPlayed = 900;
    a.perfectBonusLevels = 100;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_STR(g_acc.name, "OLDTIMER");
    CHECK_EQ_INT(g_acc.highScore, 4242);
    CHECK_EQ_INT(g_acc.settings.checkVersion, 0);
    for (int i = 0; i < 4; i++) {
        CHECK_EQ_INT(g_acc.settings.newsIds[i], -1);
        CHECK_EQ_INT(g_acc.settings.newsCounts[i], 0);
    }
    CHECK_EQ_INT(g_acc.medalOrder[3], 0);
    CHECK_EQ_INT(g_acc.medals, 0);
    CHECK_EQ_INT(g_acc.medalStep, -1);
    CHECK_EQ_INT(g_acc.cheatDetected, 0);
    CHECK_EQ_INT(g_acc.gameCompletedByte, 0);
    CHECK_NEAR(g_acc.highestMoney, 0, 0);
    CHECK_EQ_INT(g_acc.secretsInOneGame, 0);
    CHECK(g_acc.ownerStamp != 555 && g_acc.ownerStamp > 0);
    CHECK_EQ_INT(g_acc.levelDoneBytes[0], 0);
    CHECK_EQ_INT(g_acc.levelDoneBytes[24], 0);
    CHECK_EQ_INT(g_acc.levelDoneBytes[25], 1);   // the bonus level counts as done
    CHECK_EQ_INT(g_acc.levelDoneBytes[29], 0);
    CHECK_EQ_INT(g_acc.levelDoneBytes[30], 1);   // past g_numLevels: untouched
    // bonus stats rescaled to at most 450
    CHECK_EQ_INT(g_acc.bonusLevelsPlayed, 450);
    CHECK_EQ_INT(g_acc.perfectBonusLevels, 50);
}

TEST(Profile_DecodeAccount_v5_news_layout_keeps_small_bonus_stats)
{
    SetUpDirs();
    Account a = OldAccount(5);
    a.bonusLevelsPlayed = 450;
    a.perfectBonusLevels = -3;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK_EQ_INT(g_acc.bonusLevelsPlayed, 450);
    CHECK_EQ_INT(g_acc.perfectBonusLevels, 0);

    a.perfectBonusLevels = 40;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK_EQ_INT(g_acc.perfectBonusLevels, 40);
}

TEST(Profile_DecodeAccount_v5_old_layout_copies_fields)
{
    SetUpDirs();
    static AccountV2 v;
    memset(&v, 0, sizeof v);
    v.version = 5;
    snprintf(v.id, sizeof v.id, "WARBP");
    snprintf(v.name, sizeof v.name, "VETERAN");
    snprintf(v.password, sizeof v.password, "pw");
    v.created.year = 2003;
    v.bestLevelTime = 4000;
    v.highScore = 99999;
    v.meteorstormHighScore = 2222;
    v.highestRank = 7;
    v.highestLevel = 88;
    v.totalLevelsPlayed = 1500;
    v.gamesPlayed = 40;
    v.playTimeRaw = 360000;
    v.levelDoneBytes[3] = 1;
    v.shotsFired = 1000;
    v.shotsHit = 600;
    v.voiceIndex = 2;
    v.lives = 4;
    v.easyByte = 1;
    v.medalOrder[2] = 5;
    v.ownerStamp = 4321;
    v.settings.sfxVol = 0x21;
    v.settings.playTime = 5555;
    v.settings.difficulty = DIFF_HARD;
    v.settings.joyPauseAlt1 = 6;
    v.settings.version = 110;
    static Account image;   // the file's bytes past the old layout are zero
    memset(&image, 0, sizeof image);
    memcpy(&image, &v, sizeof v);
    CHECK(Decode(&image, ACC_V2));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_STR(g_acc.id, "WARBP");
    CHECK_STR(g_acc.name, "VETERAN");
    CHECK_STR(g_acc.password, "pw");
    CHECK_EQ_INT(g_acc.created.year, 2003);
    CHECK_EQ_INT(g_acc.bestLevelTime, 4000);
    CHECK_EQ_INT(g_acc.highScore, 99999);
    CHECK_EQ_INT(g_acc.meteorstormHighScore, 2222);
    CHECK_EQ_INT(g_acc.highestRank, 7);
    CHECK_EQ_INT(g_acc.highestLevel, 88);
    CHECK_EQ_INT(g_acc.totalLevelsPlayed, 1500);
    CHECK_EQ_INT(g_acc.gamesPlayed, 40);
    CHECK_EQ_INT(g_acc.playTimeRaw, 360000);
    CHECK_EQ_INT(g_acc.levelDoneBytes[3], 1);
    CHECK_EQ_INT(g_acc.shotsFired, 1000);
    CHECK_EQ_INT(g_acc.shotsHit, 600);
    CHECK_EQ_INT(g_acc.voiceIndex, 2);
    CHECK_EQ_INT(g_acc.lives, 0);   // not carried over
    CHECK_EQ_INT(g_acc.easyByte, 1);
    CHECK_EQ_INT(g_acc.medalOrder[2], 5);
    CHECK_EQ_INT(g_acc.ownerStamp, 4321);
    CHECK_EQ_INT(g_acc.settings.sfxVol, 0x21);
    CHECK_EQ_INT(g_acc.settings.best, 5555);   // the old play time slot
    CHECK_EQ_INT(g_acc.settings.difficulty, DIFF_HARD);
    CHECK_EQ_INT(g_acc.settings.joyPauseAlt1, 6);
    CHECK_EQ_INT(g_acc.settings.version, 110);
    CHECK_EQ_INT(g_acc.saveIdHistory[0], 0);
}

TEST(Profile_DecodeAccount_v3_clamps_ranks_and_levels)
{
    SetUpDirs();
    Account a = OldAccount(3);
    a.highestRank = 30;
    a.unlockedRank = 25;
    a.totalLevelsPlayed = 100000;
    a.completionRank = 5;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_EQ_INT(g_acc.highestRank, RANK_GOD);
    CHECK_EQ_INT(g_acc.unlockedRank, RANK_GOD);
    CHECK_EQ_INT(g_acc.totalLevelsPlayed, 95000);
    CHECK_EQ_INT(g_acc.completionRank, 1);
    CHECK_EQ_INT(g_acc.highScore, 4242);

    a.highestRank = 21;
    a.unlockedRank = 22;
    a.totalLevelsPlayed = 95000;
    a.completionRank = 1;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK_EQ_INT(g_acc.highestRank, 21);
    CHECK_EQ_INT(g_acc.unlockedRank, 22);
    CHECK_EQ_INT(g_acc.totalLevelsPlayed, 95000);
    CHECK_EQ_INT(g_acc.completionRank, 1);
}

TEST(Profile_DecodeAccount_v2_resets_stats_and_caps_levels_played)
{
    SetUpDirs();
    Account a = OldAccount(2);
    a.totalLevelsPlayed = 30000;
    a.shotsFired = 50;
    a.highestLevel = 9;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_STR(g_acc.name, "OLDTIMER");
    CHECK_EQ_INT(g_acc.totalLevelsPlayed, 20000);
    CHECK_EQ_INT(g_acc.highScore, 0);
    CHECK_EQ_INT(g_acc.shotsFired, 0);
    CHECK_EQ_INT(g_acc.highestLevel, 0);
    CHECK_EQ_INT(g_acc.unlockedRank, RANK_GRANDMASTER_3);
    CHECK_EQ_INT(g_acc.unusedC4, 30);
    CHECK_EQ_INT(g_acc.bestLevelTime, 999999999);
}

TEST(Profile_DecodeAccount_v0_migrates_compressed_legacy_account_without_neighbor_overwrite)
{
    SetUpDirs();
    static Account image;
    memset(&image, 0, sizeof image);
    snprintf(image.id, sizeof image.id, "OLDV0");
    snprintf(image.name, sizeof image.name, "LEGACYV0");
    snprintf(image.password, sizeof image.password, "v0-pass");
    image.created.year = 2001;
    image.version = 0;
    // The v0 high-score fields were doubles at these same offsets.
    double oldHighScore = 12345.0;
    double oldMeteorstormScore = 23456.0;
    double oldTimeTrialScore = 34567.0;
    double oldLevel100Score = 45678.0;
    memcpy(&image.highScore, &oldHighScore, sizeof oldHighScore);
    memcpy(&image.meteorstormHighScore, &oldMeteorstormScore, sizeof oldMeteorstormScore);
    memcpy(&image.timeTrialHighScore, &oldTimeTrialScore, sizeof oldTimeTrialScore);
    memcpy(&image.level100HighScore, &oldLevel100Score, sizeof oldLevel100Score);

    // g_menuEntries follows g_accV0 in the test binary's globals; a sentinel here detects
    // copying the full current Account over the smaller legacy object even without fortify.
    memset(g_menuEntries, 0x5a, sizeof g_menuEntries);
    CHECK(DecodeV0Account(&image));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_STR(g_acc.id, "OLDV0");
    CHECK_STR(g_acc.name, "LEGACYV0");
    CHECK_STR(g_acc.password, "v0-pass");
    CHECK_EQ_INT(g_acc.created.year, 2001);
    CHECK_NEAR(g_accV0.highScore, oldHighScore, 0.0);
    CHECK_NEAR(g_accV0.meteorstormHighScore, oldMeteorstormScore, 0.0);
    CHECK_NEAR(g_accV0.timeTrialHighScore, oldTimeTrialScore, 0.0);
    CHECK_NEAR(g_accV0.level100HighScore, oldLevel100Score, 0.0);
    CHECK_EQ_INT(g_menuEntries[0].active, 0x5a5a5a5a);
    CHECK_EQ_INT(g_menuEntries[84].width, 0x5a5a5a5a);
}

TEST(Profile_DecodeAccount_v1_walks_the_whole_chain)
{
    SetUpDirs();
    Account a = OldAccount(1);
    a.totalLevelsPlayed = 25000;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK_EQ_INT(g_acc.version, 7);
    CHECK_STR(g_acc.name, "OLDTIMER");
    CHECK_EQ_INT(g_acc.totalLevelsPlayed, 20000);
}

// A hiscore file whose some entries belong to `stamp`.
static void WriteHiscoresOwnedBy(__int64 stamp)
{
    ResetHiscores();
    DecompressHiscores();
    g_hiscoreMagic.table[0][5].ownerStamp = stamp;
    g_hiscoreMagic.table1[3].ownerStamp = stamp;
    g_hiscoreMagic.table2[19].ownerStamp = stamp;
    g_hiscoreMagic.table3[0].ownerStamp = stamp;
    g_hiscoreMagic.table4[19].ownerStamp = stamp;
    g_hiscoreMagic.table5[1].ownerStamp = stamp;
    g_hiscoreMagic.table2[0].ownerStamp = 42;
    CompressHiscores();
    WriteHiscoreFile();
}

TEST(Profile_DecodeAccount_v4_rerolls_stamp_and_repoints_hiscores)
{
    SetUpDirs();
    WriteHiscoresOwnedBy(777);
    Account a = OldAccount(4);
    a.ownerStamp = 777;
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK(g_acc.ownerStamp != 777 && g_acc.ownerStamp > 0);
    memset(g_hiscoreBuf, 0, 0x1000);
    LoadHiscores();
    DecompressHiscores();
    // Entries follow the final stamp after both migration steps.
    __int64 stamp = g_hiscoreMagic.table1[3].ownerStamp;
    CHECK(stamp != 777 && stamp > 0);
    CHECK_EQ_INT(g_hiscoreMagic.table[0][5].ownerStamp, stamp);
    CHECK_EQ_INT(g_hiscoreMagic.table4[19].ownerStamp, stamp);
    CHECK_EQ_INT(g_hiscoreMagic.table5[1].ownerStamp, stamp);
    CHECK_EQ_INT(g_hiscoreMagic.table2[0].ownerStamp, 42);
    // The v5 -> v6 news-layout step rolls the stamp again and follows through in hiscores.
    CHECK_EQ_INT(g_acc.ownerStamp, stamp);
}

TEST(Profile_DecodeAccount_v5_news_layout_repoints_hiscores)
{
    SetUpDirs();
    WriteHiscoresOwnedBy(555);
    Account a = OldAccount(5);
    CHECK(Decode(&a, ACC_V2_NEWS));
    CHECK(g_acc.ownerStamp != 555 && g_acc.ownerStamp > 0);
    memset(g_hiscoreBuf, 0, 0x1000);
    LoadHiscores();
    DecompressHiscores();
    CHECK_EQ_INT(g_hiscoreMagic.table[0][5].ownerStamp, g_acc.ownerStamp);
    CHECK_EQ_INT(g_hiscoreMagic.table1[3].ownerStamp, g_acc.ownerStamp);
    CHECK_EQ_INT(g_hiscoreMagic.table2[19].ownerStamp, g_acc.ownerStamp);
    CHECK_EQ_INT(g_hiscoreMagic.table3[0].ownerStamp, g_acc.ownerStamp);
    CHECK_EQ_INT(g_hiscoreMagic.table4[19].ownerStamp, g_acc.ownerStamp);
    CHECK_EQ_INT(g_hiscoreMagic.table5[1].ownerStamp, g_acc.ownerStamp);
    CHECK_EQ_INT(g_hiscoreMagic.table2[0].ownerStamp, 42);
}

TEST(Profile_DecodeAccount_zero_stamp_keeps_unowned_hiscores_in_all_migrations)
{
    for (int version = 4; version <= 5; version++) {
        SetUpDirs();
        WriteHiscoresOwnedBy(0);
        Account a = OldAccount(version);
        a.ownerStamp = 0;
        CHECK(Decode(&a, ACC_V2_NEWS));
        CHECK(g_acc.ownerStamp > 0);
        memset(g_hiscoreBuf, 0, 0x1000);
        LoadHiscores();
        DecompressHiscores();
        CHECK_EQ_INT(g_hiscoreMagic.table[0][5].ownerStamp, 0);
        CHECK_EQ_INT(g_hiscoreMagic.table1[3].ownerStamp, 0);
        CHECK_EQ_INT(g_hiscoreMagic.table2[19].ownerStamp, 0);
        CHECK_EQ_INT(g_hiscoreMagic.table3[0].ownerStamp, 0);
        CHECK_EQ_INT(g_hiscoreMagic.table4[19].ownerStamp, 0);
        CHECK_EQ_INT(g_hiscoreMagic.table5[1].ownerStamp, 0);
        CHECK_EQ_INT(g_hiscoreMagic.table2[0].ownerStamp, 42);
    }
}

TEST(Profile_DecodeAccount_v4_out_of_range_stamp_repoints_hiscores)
{
    SetUpDirs();
    WriteHiscoresOwnedBy(-5);
    Account a = OldAccount(4);
    a.ownerStamp = -5;
    CHECK(Decode(&a, ACC_V2_NEWS));
    LoadHiscores();
    DecompressHiscores();
    // repointed by the range fixup, then again by the v4 -> v5 step
    __int64 stamp = g_hiscoreMagic.table1[3].ownerStamp;
    CHECK(stamp > 0);
    CHECK_EQ_INT(g_hiscoreMagic.table5[1].ownerStamp, stamp);
    CHECK_EQ_INT(g_hiscoreMagic.table[0][5].ownerStamp, stamp);
    CHECK_EQ_INT(g_acc.ownerStamp, stamp);
}

// ---------------------------------------------------------------- ScanProfiles

TEST(Profile_ScanProfiles_counts_contiguous_profiles)
{
    SetUpDirs();
    MakeProfile(0, "ZERO");
    MakeProfile(1, "ONE");
    g_profileCount = 0;
    g_cfg.profileSel = 1;
    memset(g_accBuf, 0, sizeof(Account) * 2);
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 2);
    CHECK_EQ_INT(g_cfg.profileSel, 1);
    GetProfileName(1);
    CHECK_STR(g_logBuf, "ONE");
}

TEST(Profile_ScanProfiles_renumbers_over_gaps)
{
    SetUpDirs();
    MakeProfile(0, "ZERO");
    MakeProfile(2, "TWO");
    MakeProfile(3, "THREE");
    WriteHostFile(SlotPath("profiles", 2, "wpl"), "T:two\r\n", 7);
    WriteHostFile(SlotPath("profiles", 3, "wpl"), "T:three\r\n", 9);
    g_profileCount = 0;
    g_cfg.profileSel = 3;
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 3);
    CHECK_EQ_INT(g_cfg.profileSel, 2);   // follows its profile down
    CHECK(FakeFileExists(AccPath(0)));
    CHECK(FakeFileExists(AccPath(1)));
    CHECK(FakeFileExists(AccPath(2)));
    CHECK(!FakeFileExists(AccPath(3)));
    char buf[16] = {0};
    ReadHostFile(SlotPath("profiles", 1, "wpl"), buf, sizeof buf - 1);
    CHECK_STR(buf, "T:two\r\n");
    memset(buf, 0, sizeof buf);
    ReadHostFile(SlotPath("profiles", 2, "wpl"), buf, sizeof buf - 1);
    CHECK_STR(buf, "T:three\r\n");
    CHECK(!FakeFileExists(SlotPath("profiles", 3, "wpl")));
    GetProfileName(0);
    CHECK_STR(g_logBuf, "ZERO");
    GetProfileName(1);
    CHECK_STR(g_logBuf, "TWO");
    GetProfileName(2);
    CHECK_STR(g_logBuf, "THREE");
}

TEST(Profile_ScanProfiles_selection_before_gap_stays)
{
    SetUpDirs();
    MakeProfile(0, "ZERO");
    MakeProfile(1, "ONE");
    MakeProfile(3, "THREE");
    g_profileCount = 0;
    g_cfg.profileSel = 1;
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 3);
    CHECK_EQ_INT(g_cfg.profileSel, 1);
}

TEST(Profile_ScanProfiles_selection_of_a_missing_slot_keeps_its_number)
{
    SetUpDirs();
    MakeProfile(0, "ZERO");
    MakeProfile(2, "TWO");
    g_profileCount = 0;
    g_cfg.profileSel = 1;   // its file is gone: the selection moves to the next one
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 2);
    CHECK_EQ_INT(g_cfg.profileSel, 1);
    GetProfileName(1);
    CHECK_STR(g_logBuf, "TWO");
}

TEST(Profile_ScanProfiles_closes_a_wide_gap)
{
    SetUpDirs();
    MakeProfile(0, "ZERO");
    MakeProfile(5, "FIVE");
    g_profileCount = 0;
    g_cfg.profileSel = 5;
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 2);
    CHECK_EQ_INT(g_cfg.profileSel, 1);
    CHECK(FakeFileExists(AccPath(1)));
    CHECK(!FakeFileExists(AccPath(5)));
    GetProfileName(1);
    CHECK_STR(g_logBuf, "FIVE");
}

TEST(Profile_ScanProfiles_no_profiles_clears_selection)
{
    SetUpDirs();
    g_profileCount = 0;
    g_cfg.profileSel = 4;
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 0);
    CHECK_EQ_INT(g_cfg.profileSel, -1);
}

TEST(Profile_ScanProfiles_stops_at_ten)
{
    SetUpDirs();
    for (int i = 0; i < 10; i++)
        MakeProfile(i, "P");
    Account extra = OldAccount(7);
    WriteAccountFile(AccPath(10), &extra, ACC_SIZE, ACC_SIZE);
    g_profileCount = 0;
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 10);
    CHECK(FakeFileExists(AccPath(10)));   // never looked at
}

TEST(Profile_ScanProfiles_upgrades_and_resaves_old_accounts)
{
    SetUpDirs();
    Account a = OldAccount(6);
    a.saveIdHistory[7] = 1234;
    WriteAccountFile(AccPath(0), &a, ACC_SIZE, ACC_SIZE);
    g_profileCount = 0;
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 1);
    Account got;
    CHECK(ReadAccountFile(AccPath(0), &got));
    CHECK_EQ_INT(got.version, 7);
    CHECK_EQ_INT(got.saveIdHistory[7], 0);
    CHECK_STR(got.name, "OLDTIMER");
}

TEST(Profile_ScanProfiles_corrupt_file_becomes_fresh_profile)
{
    SetUpDirs();
    char junk[ACC_SIZE];
    memset(junk, 0x44, sizeof junk);
    WriteHostFile(AccPath(0), junk, sizeof junk);
    g_profileCount = 0;
    ScanProfiles();
    CHECK_EQ_INT(g_profileCount, 1);
    Account got;
    CHECK(ReadAccountFile(AccPath(0), &got));
    CHECK_EQ_INT(got.version, 7);
    CHECK_EQ_INT(got.unlockedRank, RANK_GRANDMASTER_3);
}

// ---------------------------------------------------------------- getters / setters

TEST(Profile_getters_read_the_packed_account)
{
    SetUpDirs();
    ResetAccount();
    snprintf(g_acc.name, sizeof g_acc.name, "VIPER");
    g_acc.easyByte = 1;
    g_acc.settings.windowedByte = 1;
    g_acc.voiceIndex = 4;
    g_acc.lastSaveId = 987654321012LL;
    g_acc.lives = 3;
    PackAccount(5);
    GetProfileName(5);
    CHECK_STR(g_logBuf, "VIPER");
    CHECK(GetProfileEasyFlag(5));
    CHECK(GetProfileCfgFlag(5));
    CHECK_EQ_INT(GetProfileVoiceIndex(5), 4);
    CHECK_EQ_INT(GetProfileLastSaveId(5), 987654321012LL);
    CHECK_EQ_INT(GetProfileLives(5), 3);
    CHECK_EQ_INT(g_acc.version, 0);   // each getter clears g_acc again

    ResetAccount();
    PackAccount(6);
    CHECK(!GetProfileEasyFlag(6));
    CHECK(!GetProfileCfgFlag(6));
}

TEST(Profile_getters_without_profile_return_defaults)
{
    snprintf(g_logBuf, 16, "stale");
    GetProfileName(-1);
    CHECK_STR(g_logBuf, "");
    CHECK(!GetProfileEasyFlag(-1));
    CHECK(!GetProfileCfgFlag(-1));
    CHECK_EQ_INT(GetProfileVoiceIndex(-1), 0);
    CHECK_EQ_INT(GetProfileLastSaveId(-1), 0);
    CHECK_EQ_INT(GetProfileLives(-1), 0);
    CHECK(!ProfileHistHas(-1, 0));
}

TEST(Profile_setters_save_to_disk)
{
    SetUpDirs();
    MakeProfile(1, "HOLLYWOOD");
    SetProfileLastSaveId(1, 4444444444LL);
    SetProfileVoiceIndex(1, 3);
    ReloadSlot(1);
    CHECK_EQ_INT(GetProfileVoiceIndex(1), 3);
    CHECK_EQ_INT(GetProfileLastSaveId(1), 4444444444LL);
    GetProfileName(1);
    CHECK_STR(g_logBuf, "HOLLYWOOD");
}

TEST(Profile_MergeSettings_copies_config_keeping_best_score)
{
    SetUpDirs();
    DefaultSettings();
    ResetAccount();
    g_acc.settings.best = 1000;
    PackAccount(0);
    g_cfg.sfxVol = 0x31;
    g_cfg.best = 500;
    MergeSettings(0);
    ReloadSlot(0);
    UnpackAccount(0);
    CHECK_EQ_INT(g_acc.settings.sfxVol, 0x31);
    CHECK_EQ_INT(g_acc.settings.best, 1000);

    PackAccount(0);
    g_cfg.best = 2000;
    MergeSettings(0);
    ReloadSlot(0);
    UnpackAccount(0);
    CHECK_EQ_INT(g_acc.settings.best, 2000);
}

// ---------------------------------------------------------------- lives

TEST(Profile_lives_reset_to_five_and_count_down)
{
    SetUpDirs();
    MakeProfile(2, "LIVES");
    CHECK_EQ_INT(GetProfileLives(2), 0);
    ResetProfileLives(2);
    CHECK_EQ_INT(GetProfileLives(2), 5);
    DecProfileLives(2);
    DecProfileLives(2);
    CHECK_EQ_INT(GetProfileLives(2), 3);
    ReloadSlot(2);   // saved each time
    CHECK_EQ_INT(GetProfileLives(2), 3);
}

TEST(Profile_lives_increase_caps_at_five)
{
    SetUpDirs();
    MakeProfile(2, "LIVES");
    IncProfileLives(2);
    CHECK_EQ_INT(GetProfileLives(2), 1);
    for (int i = 0; i < 5; i++)
        IncProfileLives(2);
    CHECK_EQ_INT(GetProfileLives(2), 5);
    ReloadSlot(2);
    CHECK_EQ_INT(GetProfileLives(2), 5);
}

TEST(Profile_lives_decrease_has_no_floor)
{
    SetUpDirs();
    MakeProfile(2, "LIVES");
    DecProfileLives(2);
    CHECK_EQ_INT(GetProfileLives(2), -1);
}

// ---------------------------------------------------------------- save-id history

TEST(Profile_history_push_then_has)
{
    SetUpDirs();
    MakeProfile(3, "HIST");
    SetProfileLastSaveId(3, 42);
    CHECK(!ProfileHistHas(3, 42));
    ProfileHistPush(3, 42);
    CHECK(ProfileHistHas(3, 42));
    CHECK(!ProfileHistHas(3, 43));
    CHECK_EQ_INT(GetProfileLastSaveId(3), 0);   // pushing clears it
    ReloadSlot(3);
    CHECK(ProfileHistHas(3, 42));
}

TEST(Profile_history_keeps_last_fifty)
{
    SetUpDirs();
    MakeProfile(3, "HIST");
    for (int v = 1; v <= 51; v++)
        ProfileHistPush(3, 1000 + v);
    CHECK(!ProfileHistHas(3, 1001));   // the oldest dropped
    for (int v = 2; v <= 51; v++)
        CHECK_MSG(ProfileHistHas(3, 1000 + v), "lost %d", 1000 + v);
    UnpackAccount(3);
    CHECK_EQ_INT(g_acc.saveIdHistory[0], 1002);
    CHECK_EQ_INT(g_acc.saveIdHistory[48], 1050);
    CHECK_EQ_INT(g_acc.saveIdHistory[49], 1051);
}

// ---------------------------------------------------------------- setpro.dat

static const char *SetProPath(void) { return FakeUserPath("warblade\\setpro.dat"); }

TEST(Profile_SaveSetPro_writes_the_profile_path)
{
    SetUpDirs();
    char buf[1024] = {0};
    memset(buf, 'X', 1000);
    WriteHostFile(SetProPath(), buf, 1000);   // a longer old file
    memset(buf, 0, sizeof buf);
    g_profileIndex = 4;
    g_gameMode = MODE_SINGLE;
    SaveSetPro();
    ReadHostFile(SetProPath(), buf, sizeof buf - 1);
    char want[1024];
    snprintf(want, sizeof want, "%s\\warblade\\profiles\\profile004.wpl", g_fake.userFolder);
    CHECK_STR(buf, want);

    g_profileIndex = 1;
    g_gameMode = MODE_TIME_TRIAL;
    SaveSetPro();
    memset(buf, 0, sizeof buf);
    ReadHostFile(SetProPath(), buf, sizeof buf - 1);
    snprintf(want, sizeof want, "%s\\warblade\\profiles\\profile001.wpl", g_fake.userFolder);
    CHECK_STR(buf, want);
}

TEST(Profile_SaveSetPro_skips_without_profile_or_in_other_modes)
{
    SetUpDirs();
    g_profileIndex = -1;
    g_gameMode = MODE_SINGLE;
    SaveSetPro();
    CHECK(!FakeFileExists(SetProPath()));
    g_profileIndex = 2;
    g_gameMode = MODE_TWO_PLAYER;
    SaveSetPro();
    CHECK(!FakeFileExists(SetProPath()));
    g_gameMode = MODE_SINGLE;
    g_playerUpdateFn = StateDemo;
    SaveSetPro();
    CHECK(!FakeFileExists(SetProPath()));
}

TEST(Profile_DeleteSetPro_removes_the_file)
{
    SetUpDirs();
    g_profileIndex = 0;
    g_gameMode = MODE_SINGLE;
    SaveSetPro();
    CHECK(FakeFileExists(SetProPath()));
    DeleteSetPro();
    CHECK(!FakeFileExists(SetProPath()));
}

// ---------------------------------------------------------------- Logout

TEST(Profile_Logout_saves_settings_and_leaves_the_profile)
{
    BootGame();
    SeedRand(99);
    MakeProfile(0, "LOGGEDIN");
    g_profileCount = 1;
    g_profileIndex = 0;
    g_gameMode = MODE_SINGLE;
    SaveSetPro();
    CHECK(FakeFileExists(SetProPath()));
    g_profileWinOpen = 1;
    g_clickWin = 3;
    g_cfg.sfxVol = 0x13;
    Logout();
    CHECK_EQ_INT(g_profileIndex, -1);
    CHECK(!FakeFileExists(SetProPath()));
    CHECK_EQ_INT(g_profileWinOpen, 0);
    CHECK_EQ_INT(g_clickWin, -1);
    CHECK_EQ_INT(g_acc.version, 0);
    Account got;
    CHECK(ReadAccountFile(AccPath(0), &got));
    CHECK_EQ_INT(got.settings.sfxVol, 0x13);
    CHECK_STR(got.name, "LOGGEDIN");
}
