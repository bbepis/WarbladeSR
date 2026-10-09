// account.c: Profiles and accounts: setpro.dat, .wpl/.acc files, pack/unpack/decode, scanning,
// per-profile accessors, logout.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"
#include "game.h"
#include <zlib.h>

// Sizes of the packed/unpacked Account struct at its various on-disk versions.
enum {
    SIZEOF_ACCOUNT       = 0x40d0,  // sizeof(Account), current version
    ACCOUNT_SIZE_V2      = 0x3c00,  // pre-news-tracking AccountV2 layout
    ACCOUNT_SIZE_V2_NEWS = 0x3c08,  // AccountV2 layout, already news-tracking sized
    OWNER_STAMP_MAX      = 999999999  // valid-range ceiling for Account::ownerStamp (coincidentally
                                       // the same value as NO_TIME_RECORDED, but unrelated)
};



// Writes the path of the currently active profile's .wpl file to setpro.dat, so the game
// can reopen the same profile next launch. Skipped in demo mode or while no profile is set.
void SaveSetPro()
{
    int fd;
    char c[512];
    char p[512];
    int n;

    fd = 0;
    n = 0;
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo)
    {
        sprintf(c, "%s\\warblade\\profiles\\profile%03d.wpl", SysUserFolder(), g_profileIndex);
        _set_fmode(_O_BINARY);
        sprintf(p, "%s\\warblade\\setpro.dat", SysUserFolder());
        fd = _open(p, _O_CREAT | _O_TRUNC | _O_RDWR, _S_IREAD | _S_IWRITE);
        if (fd != -1)
        {
            _write(fd, c, StrLen(c));
            _close(fd);
        }
    }
}

// Deletes setpro.dat, so the next launch falls back to the default profile.
void DeleteSetPro()
{
    char p[512];

    sprintf(p, "%s\\warblade\\setpro.dat", SysUserFolder());
    remove(p);
}

// Each line starts with a one-letter tag (T/G/M/N/B/P/H/S/D/I/E) followed by ':' and a
// value, terminated by '\r'. 'G' lines are repeated (song list); the rest are single fields.

// Reads one '\r'-terminated line from fh into c[pos..], stopping after the '\r' (which is
// replaced with a NUL). Sets len to the number of bytes read, including the terminator.
#define READ_LINE()                                         \
    len = 0;                                                  \
    done = false;                                             \
    do {                                                      \
        r = _read(fh, &c[pos], 1);                            \
        pos++;                                                \
        len++;                                                \
        if (c[pos - 1] == '\r') {                             \
            c[pos - 1] = 0;                                   \
            done = true;                                      \
        }                                                     \
    } while (r > 0 && !done);

// Skips the line's trailing '\n' and reads the first byte (the tag letter) of the next line.
#define NEXT_LINE()                                           \
    r = _read(fh, &c[pos], 1);                                \
    pos = 0;                                                  \
    r = _read(fh, &c[pos], 1);                                \
    pos++;

// If the current line's tag matches ch, reads the rest of the line into dst and advances to
// the next line.
#define SIMPLE_FIELD(ch, dst)                                 \
    if (c[0] == ch) {                                         \
        READ_LINE()                                         \
        CopyStrAt(dst, &c[1], 0, len - 1);             \
        NEXT_LINE()                                           \
    }

// Loads the active profile's .wpl file (or the shared warblade.wpl when no profile is
// selected) into the g_str* globals and g_songNames[]. Called on startup / profile switch.
void LoadProfile()
{
    int fh = 0;
    int pos;
    int len;
    unsigned int r;
    char c[260];
    bool done;
    int gcount = 0;
    char fn[260];
    int flen;
    int i;

    MakeProfilesDir();
    if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) && g_playerUpdateFn != StateDemo)
        sprintf(fn, "%s\\warblade\\profiles\\profile%03d.wpl", SysUserFolder(), g_profileIndex);
    else
        sprintf(fn, "%s\\warblade\\warblade.wpl", SysUserFolder());
    g_customSongs = 0;
    pos = 0;
    _set_fmode(_O_BINARY);
    fh = _open(fn, 0, 0);

    if (fh != -1) {
        flen = _filelength(fh);
        if (flen > 0) {
            // Clear all fields to "empty" (a leading '\n') before parsing, so a missing tag
            // in the file leaves its field blank rather than stale.
            g_strT[0] = '\n';
            for (i = 0; i < 256; i++)
                g_songNames[i][0] = '\n';
            g_strB[0] = '\n';
            g_strM[0] = '\n';
            g_strN[0] = '\n';
            g_strH[0] = '\n';
            g_strS[0] = '\n';
            g_strP[0] = '\n';
            g_strD[0] = '\n';
            g_strI[0] = '\n';
            g_strE[0] = '\n';
            g_customSongs = 1;

            // Parse the file one tagged line at a time until EOF.
            r = _read(fh, &c[pos], 1);
            pos++;
            while (r > 0) {
                SIMPLE_FIELD('T', g_strT)
                if (c[0] == 'G') {
                    READ_LINE()
                    if (gcount < 256) {
                        CopyStrAt(g_songNames[gcount], &c[1], 0, len - 1);
                        gcount++;
                    }
                    NEXT_LINE()
                    g_songCount = gcount;
                }

                SIMPLE_FIELD('M', g_strM)
                SIMPLE_FIELD('N', g_strN)
                SIMPLE_FIELD('B', g_strB)
                SIMPLE_FIELD('P', g_strP)
                SIMPLE_FIELD('H', g_strH)
                SIMPLE_FIELD('S', g_strS)
                SIMPLE_FIELD('D', g_strD)
                SIMPLE_FIELD('I', g_strI)
                SIMPLE_FIELD('E', g_strE)
            }
            _close(fh);
        }
    }
}

#undef READ_LINE
#undef NEXT_LINE
#undef SIMPLE_FIELD

// Zeroes the in-memory account struct (used to drop the unpacked g_acc after reading a
// single field out of it).
void ClearAccount()
{
    memset(&g_acc, 0, SIZEOF_ACCOUNT);
}

// Resets g_acc to default/new-profile values (fresh stats, unlocked rank 20, current
// timestamp) and merges in the current global settings, keeping the higher of the two
// best-score values.
void ResetAccount()
{
    int i;
    int j;

    memset(&g_acc, 0, SIZEOF_ACCOUNT);
    SysUtcDate(&g_acc.created);

    // Rank/level progress and play-time stats.
    g_acc.timeTrialHighScoreLo = 0;
    g_acc.timeTrialHighScoreHi = 0;
    g_acc.highestRank = 0;
    g_acc.unusedF4 = 0;
    g_acc.unlockedRank = RANK_GRANDMASTER_3;
    g_acc.highestLevel = 0;
    g_acc.totalLevelsPlayed = 0;
    g_acc.gamesPlayed = 0;
    g_acc.playTimeLo = 0;
    g_acc.playTimeHi = 0;
    g_acc.unusedC4 = 30;

    // Combat/game-session stats and the per-mode high scores.
    g_acc.shotsFired = 0;
    g_acc.shotsHit = 0;
    g_acc.secretsInOneGame = 0;
    g_acc.bonusLevelsPlayed = 0;
    g_acc.perfectBonusLevels = 0;
    g_acc.unusedDc = 1;
    g_acc.easyByte = 0;
    g_acc.medals = 0;
    g_acc.bestHitPctAbove25 = 0;
    g_acc.level100HighScoreLo = 0;
    g_acc.level100HighScoreHi = 0;
    g_acc.highScoreLo = 0;
    g_acc.highScoreHi = 0;
    g_acc.meteorstormHighScoreLo = 0;
    g_acc.meteorstormHighScoreHi = 0;
    g_acc.highestMoney = 0;
    g_acc.completionRank = 0;

    // Medal progress and the owner/anti-cheat markers.
    g_acc.medalStep = -1;
    CLEAR_MEDAL_ORDER(g_acc)
    g_acc.gameCompletedByte = 0;
    g_acc.cheatDetected = 0;
    g_acc.ownerStamp = MakeRandomId();

    // Best-time sentinels and per-level completion flags.
    g_acc.bestLevelTime = NO_TIME_RECORDED;        // sentinel: "no time recorded yet"
    g_acc.bestMeteorstormTime = NO_TIME_RECORDED;  // sentinel: "no time recorded yet"
    for (i = 0; i < NUM_LEVEL_SECRETS; i++)
        g_acc.levelDoneBytes[i] = 0;
    g_acc.version = 7;  // current account struct version

    // Bring in the current global settings, keeping whichever best score is higher.
    g_hiscore = g_acc.settings.best;
    g_acc.settings = g_cfg;
    if (g_hiscore > g_acc.settings.best)
        g_acc.settings.best = g_hiscore;
    for (j = 0; j < 50; j++)
        g_acc.saveIdHistory[j] = 0;
    g_acc.lastSaveId = 0;
}

// Allocates the next free profile slot (up to 10), clearing g_acc for it. Returns the new
// profile count, or -1 if all 10 slots are in use.
int NextAccountReset()
{
    if (g_profileCount < 10) {
        ClearAccount();
        g_profileCount++;
        return g_profileCount;
    }
    return -1;
}

// Writes the already-packed (compressed) g_accBuf[profile] out to profileNNN.acc. Pops a
// message box and sets g_fileWriteErrorFlag on write failure.
// NOTE: `nnn` is unused here (kept for the stack layout to match the original).
void SaveAccount(int profile)
{
    if (profile > -1) {
        int fh = 0;
        int r;
        char nnn[7] = "WARBP1";
        char levelname[512];

        MakeProfilesDir();
        sprintf(levelname, "%s\\warblade\\profiles\\profile%03d.acc", SysUserFolder(), profile);
        _set_fmode(_O_BINARY);
        fh = _open(levelname, _O_CREAT | _O_TRUNC | _O_RDWR, _S_IREAD | _S_IWRITE);
        if (fh != -1) {
            r = _write(fh, &g_accBuf[profile], SIZEOF_ACCOUNT);
            if (r == -1) {
                g_fileWriteErrorFlag = 1;
                SysMessageBox("Warblade SR 2.0, Copyright 1999-2009 Edgar M Vigdal",
                              "Could not open/create Profile file");
                g_fileWriteErrorFlag = 0;
            }
            _close(fh);
        }
    }
}

// Compresses g_acc into g_accBuf[profile] (zlib) and clears g_acc afterwards.
// NOTE: the destination buffer is first filled with random bytes before compress() writes
// into it; harmless (compress() overwrites exactly g_accSizes[profile] bytes) but kept as-is.
void PackAccount(int profile)
{
    if (profile > -1) {
        int res;
        char *bytePtr;
        unsigned int i;

        g_accSize = SIZEOF_ACCOUNT;
        g_accSizes[profile] = SIZEOF_ACCOUNT;
        bytePtr = (char *)&g_accBuf[profile];
        for (i = 0; i < SIZEOF_ACCOUNT; i++, bytePtr++)
            *bytePtr = RandRange(0, 0xff);
        {
            uLongf packed = (uLongf)g_accSizes[profile];   // zlib's sizes are `unsigned long`
            res = compress((unsigned char *)&g_accBuf[profile], &packed,
                           (const unsigned char *)&g_acc, g_accSize);
            g_accSizes[profile] = (int)packed;
        }
        memset(&g_acc, 0, SIZEOF_ACCOUNT);
    }
}

// Decompresses g_accBuf[profile] into g_acc. On corrupt data, resets the profile to
// defaults and re-saves it so subsequent reads succeed.
void UnpackAccount(int profile)
{
    if (profile > -1) {
        int res;

        g_accSize = SIZEOF_ACCOUNT;
        g_accSizes[profile] = SIZEOF_ACCOUNT;
        {
            uLongf unpacked = (uLongf)g_accSize;   // zlib's sizes are `unsigned long`
            res = uncompress((unsigned char *)&g_acc, &unpacked,
                             (const unsigned char *)&g_accBuf[profile], g_accSizes[profile]);
            g_accSize = (int)unpacked;
        }
        if (res != 0) {
            ResetAccount();
            PackAccount(profile);
            SaveAccount(profile);
        }
    }
}

// Reads a profile's backup .acc file (backup\profileNNN.acc) and decodes it into g_acc,
// upgrading it if it's an older version. Returns the resulting ownerStamp (0 if no backup
// file exists).
__int64 LoadBackupAccount(int profile)
{
    int fh = 0;
    __int64 t = 0;
    char levelname[255];
    char b[SIZEOF_ACCOUNT];
    int r;

    sprintf(levelname, "%s\\warblade\\backup\\profile%03d.acc", SysUserFolder(), profile);
    _set_fmode(_O_BINARY);
    fh = _open(levelname, 0, 0);
    if (fh != -1) {
        r = _read(fh, b, SIZEOF_ACCOUNT);
        _close(fh);
        if (!DecodeAccount(b, profile, r))
            ResetAccount();
        t = g_acc.ownerStamp;
    }
    return t;
}

// Returns the ownerStamp of a saved profile (0 if none/-1), unpacking and clearing g_acc.
__int64 GetAccountTime(int profile)
{
    __int64 t = 0;
    if (profile != -1) {
        UnpackAccount(profile);
        t = g_acc.ownerStamp;
        ClearAccount();
    }
    return t;
}

// Reads a profile's backup .acc file straight into g_accBuf[profile] (still packed) without
// decoding it. Returns the number of bytes read (0 on failure).
int LoadAccount(int profile)
{
    int fh = 0;
    char levelname[255];
    int r = 0;

    MakeProfilesDir();
    sprintf(levelname, "%s\\warblade\\backup\\profile%03d.acc", SysUserFolder(), profile);
    _set_fmode(_O_BINARY);
    fh = _open(levelname, 0, 0);
    if (fh != -1) {
        r = _read(fh, &g_accBuf[profile], SIZEOF_ACCOUNT);
        g_accSizes[profile] = SIZEOF_ACCOUNT;
        _close(fh);
    }
    return r;
}

// Repoints matching entries across all 6 hiscore tables. Stamp zero means unowned and must
// stay zero; non-zero stamps, including invalid negative legacy stamps, still follow migration.
#define REPOINT_OWNER_STAMP(idx, oldStamp)                                \
    if ((oldStamp) != 0) {                                                \
        if (g_hiscoreMagic.table[0][idx].ownerStamp == (oldStamp))        \
            g_hiscoreMagic.table[0][idx].ownerStamp = g_acc.ownerStamp;   \
        if (g_hiscoreMagic.table1[idx].ownerStamp == (oldStamp))          \
            g_hiscoreMagic.table1[idx].ownerStamp = g_acc.ownerStamp;     \
        if (g_hiscoreMagic.table2[idx].ownerStamp == (oldStamp))          \
            g_hiscoreMagic.table2[idx].ownerStamp = g_acc.ownerStamp;     \
        if (g_hiscoreMagic.table3[idx].ownerStamp == (oldStamp))          \
            g_hiscoreMagic.table3[idx].ownerStamp = g_acc.ownerStamp;     \
        if (g_hiscoreMagic.table4[idx].ownerStamp == (oldStamp))          \
            g_hiscoreMagic.table4[idx].ownerStamp = g_acc.ownerStamp;     \
        if (g_hiscoreMagic.table5[idx].ownerStamp == (oldStamp))          \
            g_hiscoreMagic.table5[idx].ownerStamp = g_acc.ownerStamp;     \
    }

// Copies the 4 "AltN" alternate-binding fields (N = 0..3) for `prefix` from g_accV2 into
// g_acc (used by the v5->v6 AccountV2 field-by-field copy below).
#define COPY_ALT4(prefix)                                       \
    g_acc.settings.prefix##0 = g_accV2.settings.prefix##0;      \
    g_acc.settings.prefix##1 = g_accV2.settings.prefix##1;      \
    g_acc.settings.prefix##2 = g_accV2.settings.prefix##2;      \
    g_acc.settings.prefix##3 = g_accV2.settings.prefix##3;

// Decompresses `buf` (a profile's packed .acc data, `len` bytes as read from disk) into
// g_acc, then walks the version-upgrade chain below to bring it up to the current struct
// version (v0 -> v7), migrating/clearing fields as each version's layout changed. Returns
// false if the buffer failed to decompress (corrupt/foreign data).
bool DecodeAccount(void *buf, int profile, int len)
{
    int res;
    bool ok;
    int i;
    int i2;
    __int64 oldTime;
    int j;
    __int64 oldTime2;
    int j2;
    int k1;
    int k2;
    int k3;
    int k4;
    int k5;
    int k6;
    int k7;
    float scale;
    int k8;

    ok = true;
    g_accSize = SIZEOF_ACCOUNT;
    {
        uLongf unpacked = (uLongf)g_accSize;   // zlib's sizes are `unsigned long`
        res = uncompress((unsigned char *)&g_acc, &unpacked,
                         (const unsigned char *)buf, g_accSizes[profile]);
        g_accSize = (int)unpacked;
    }
    if (res == 0) {

        // v0 -> v1: high scores were doubles; convert them to the current integer fields.
        if (g_acc.version == 0) {
            memcpy(&g_accV0, &g_acc, sizeof g_accV0);
            g_acc.highScore = (__int64)g_accV0.highScore;
            g_acc.meteorstormHighScore = (__int64)g_accV0.meteorstormHighScore;
            g_acc.timeTrialHighScore = (__int64)g_accV0.timeTrialHighScore;
            g_acc.level100HighScore = (__int64)g_accV0.level100HighScore;
            g_acc.version = 1;
        }

        // v1 -> v2: reset play stats (fields added/reordered in this version).
        if (g_acc.version == 1) {
            g_acc.timeTrialHighScore = 0;
            g_acc.highestRank = 0;
            g_acc.unusedF4 = 0;
            g_acc.unlockedRank = RANK_GRANDMASTER_3;
            g_acc.highestLevel = 0;
            g_acc.gamesPlayed = 0;
            g_acc.unusedC4 = 30;
            g_acc.shotsFired = 0;
            g_acc.shotsHit = 0;
            g_acc.secretsInOneGame = 0;
            g_acc.bonusLevelsPlayed = 0;
            g_acc.perfectBonusLevels = 0;
            g_acc.unusedDc = 1;
            g_acc.medals = 0;
            g_acc.bestHitPctAbove25 = 0;
            g_acc.level100HighScore = 0;
            g_acc.highScore = 0;
            g_acc.meteorstormHighScore = 0;
            g_acc.highestMoney = 0;

            g_acc.bestLevelTime = NO_TIME_RECORDED;
            g_acc.bestMeteorstormTime = NO_TIME_RECORDED;
            for (i = 0; i < NUM_LEVEL_SECRETS; i++)
                g_acc.levelDoneBytes[i] = 0;
            g_acc.version = 2;
        }

        // v2 -> v3: same again, plus clamp totalLevelsPlayed and add medals/completion fields.
        if (g_acc.version == 2) {
            g_acc.timeTrialHighScore = 0;
            g_acc.highestRank = 0;
            g_acc.unusedF4 = 0;
            g_acc.unlockedRank = RANK_GRANDMASTER_3;
            g_acc.highestLevel = 0;
            g_acc.totalLevelsPlayed = g_acc.totalLevelsPlayed > 20000 ? 20000 : g_acc.totalLevelsPlayed;
            g_acc.gamesPlayed = 0;
            g_acc.unusedC4 = 30;
            g_acc.shotsFired = 0;
            g_acc.shotsHit = 0;
            g_acc.secretsInOneGame = 0;
            g_acc.bonusLevelsPlayed = 0;
            g_acc.perfectBonusLevels = 0;
            g_acc.unusedDc = 1;
            g_acc.medals = 0;
            g_acc.bestHitPctAbove25 = 0;
            g_acc.level100HighScore = 0;
            g_acc.highScore = 0;
            g_acc.meteorstormHighScore = 0;
            g_acc.highestMoney = 0;

            g_acc.completionRank = 0;

            g_acc.medalStep = -1;
            CLEAR_MEDAL_ORDER(g_acc)
            g_acc.gameCompletedByte = 0;
            g_acc.cheatDetected = 0;
            g_acc.ownerStamp = MakeRandomId();
            g_acc.bestLevelTime = NO_TIME_RECORDED;
            g_acc.bestMeteorstormTime = NO_TIME_RECORDED;
            for (i2 = 0; i2 < NUM_LEVEL_SECRETS; i2++)
                g_acc.levelDoneBytes[i2] = 0;
            g_acc.version = 3;
        }

        // v3 -> v4: clamp rank/level counters to new (higher) ranges and re-roll ownerStamp.
        if (g_acc.version == 3) {
            g_acc.unusedF4 = 0;
            if (g_acc.highestRank > RANK_GOD)
                g_acc.highestRank = RANK_GOD;
            if (g_acc.unlockedRank > RANK_GOD)
                g_acc.unlockedRank = RANK_GOD;
            if (g_acc.totalLevelsPlayed > 95000)
                g_acc.totalLevelsPlayed = 95000;
            g_acc.medals = 0;
            if (g_acc.completionRank > 1)
                g_acc.completionRank = 1;
            g_acc.bestHitPctAbove25 = 0;

            g_acc.medalStep = -1;
            CLEAR_MEDAL_ORDER(g_acc)
            g_acc.gameCompletedByte = 0;
            g_acc.cheatDetected = 0;
            g_acc.ownerStamp = MakeRandomId();
            g_acc.level100HighScore = 0;
            g_acc.version = 4;
        }

        // v4 fixup: if ownerStamp is out of the valid range, re-roll it and repoint any
        // hiscore-table entries that referenced the old stamp.
        if (g_acc.version == 4) {
            oldTime = g_acc.ownerStamp;
            if (g_acc.ownerStamp < 0 || g_acc.ownerStamp > OWNER_STAMP_MAX) {
                g_acc.ownerStamp = MakeRandomId();
                LoadHiscores();
                DecompressHiscores();

                for (j = 0; j < 20; j++) {
                    REPOINT_OWNER_STAMP(j, oldTime)
                }
                CompressHiscores();
                WriteHiscoreFile();
            }
        }

        // v4 -> v5: unconditionally re-roll ownerStamp again and repoint hiscore entries.
        // NOTE: runs right after the fixup above, so a v4 account gets ownerStamp rerolled
        // twice on this pass; kept as-is (matches the original).
        if (g_acc.version == 4) {
            oldTime2 = g_acc.ownerStamp;
            g_acc.ownerStamp = MakeRandomId();
            LoadHiscores();
            DecompressHiscores();

            for (j2 = 0; j2 < 20; j2++) {
                REPOINT_OWNER_STAMP(j2, oldTime2)
            }
            CompressHiscores();
            WriteHiscoreFile();
            g_acc.version = 5;
        }

        // v5 -> v6, from the smaller pre-news-tracking layout (AccountV2, no saveIdHistory):
        // field-by-field copy from the old struct shape into the current one.
        if (g_acc.version == 5 && len == ACCOUNT_SIZE_V2) {
            memcpy(&g_accV2, &g_acc, ACCOUNT_SIZE_V2);
            memset(&g_acc, 0, SIZEOF_ACCOUNT);
            for (k1 = 0; k1 < 7; k1++)
                g_acc.id[k1] = g_accV2.id[k1];
            for (k2 = 0; k2 < NAME_LEN; k2++)
                g_acc.name[k2] = g_accV2.name[k2];
            for (k3 = 0; k3 < 16; k3++)
                g_acc.password[k3] = g_accV2.password[k3];
            g_acc.created = g_accV2.created;
            g_acc.bestLevelTime = g_accV2.bestLevelTime;
            g_acc.bestMeteorstormTime = g_accV2.bestMeteorstormTime;
            g_acc.highScore = g_accV2.highScore;
            g_acc.meteorstormHighScore = g_accV2.meteorstormHighScore;
            g_acc.timeTrialHighScore = g_accV2.timeTrialHighScore;
            g_acc.highestMoney = g_accV2.highestMoney;
            g_acc.highestRank = g_accV2.highestRank;
            g_acc.highestLevel = g_accV2.highestLevel;
            g_acc.totalLevelsPlayed = g_accV2.totalLevelsPlayed;
            g_acc.gamesPlayed = g_accV2.gamesPlayed;
            g_acc.playTimeRaw = g_accV2.playTimeRaw;
            for (k4 = 0; k4 < NUM_LEVEL_SECRETS; k4++)
                g_acc.levelDoneBytes[k4] = g_accV2.levelDoneBytes[k4];

            g_acc.unusedC4 = g_accV2.unusedC4;
            g_acc.shotsFired = g_accV2.shotsFired;
            g_acc.shotsHit = g_accV2.shotsHit;
            g_acc.secretsInOneGame = g_accV2.secretsInOneGame;
            g_acc.bonusLevelsPlayed = g_accV2.bonusLevelsPlayed;
            g_acc.perfectBonusLevels = g_accV2.perfectBonusLevels;
            g_acc.unusedDc = g_accV2.unusedDc;
            g_acc.medals = g_accV2.medals;
            g_acc.bestHitPctAbove25 = g_accV2.bestHitPctAbove25;
            g_acc.voiceIndex = g_accV2.voiceIndex;
            g_acc.completionRank = g_accV2.completionRank;
            g_acc.unusedF4 = g_accV2.unusedF4;
            g_acc.unlockedRank = g_accV2.unlockedRank;
            g_acc.medalStep = g_accV2.medalStep;
            g_acc.lives = 0;
            g_acc.unused104 = g_accV2.unused104;
            g_acc.level100HighScore = g_accV2.level100HighScore;
            g_acc.unused110 = g_accV2.unused110;
            g_acc.unused114 = g_accV2.unused114;
            g_acc.unused118 = g_accV2.unused118;
            g_acc.easyByte = g_accV2.easyByte;
            g_acc.unused11d = g_accV2.unused11d;
            g_acc.unused11e = g_accV2.unused11e;
            g_acc.unused11f = g_accV2.unused11f;
            g_acc.unused120 = g_accV2.unused120;

            for (k5 = 0; k5 < 20; k5++)
                g_acc.names[k5] = g_accV2.names[k5];
            g_acc.medalOrder[0] = g_accV2.medalOrder[0];
            g_acc.medalOrder[1] = g_accV2.medalOrder[1];
            g_acc.medalOrder[2] = g_accV2.medalOrder[2];
            g_acc.medalOrder[3] = g_accV2.medalOrder[3];
            g_acc.medalOrder[4] = g_accV2.medalOrder[4];
            g_acc.medalOrder[5] = g_accV2.medalOrder[5];
            g_acc.medalOrder[6] = g_accV2.medalOrder[6];
            g_acc.gameCompletedByte = g_accV2.gameCompletedByte;
            g_acc.cheatDetected = g_accV2.cheatDetected;
            g_acc.ownerStamp = g_accV2.ownerStamp;
            for (k6 = 0; k6 < 30; k6++)
                g_acc.settings.title[k6] = g_accV2.settings.title[k6];

            g_acc.settings.unused020 = g_accV2.settings.unused020;
            g_acc.settings.gamesPlayed = g_accV2.settings.gamesPlayed;
            g_acc.settings.particlesOn = g_accV2.settings.particlesOn;
            g_acc.settings.borderMode = g_accV2.settings.borderMode;
            g_acc.settings.unused030 = g_accV2.settings.unused030;
            g_acc.settings.musicFormat = g_accV2.settings.musicFormat;
            g_acc.settings.unused038 = g_accV2.settings.unused038;
            g_acc.settings.unused03c = g_accV2.settings.unused03c;
            g_acc.settings.musicVolume = g_accV2.settings.musicVolume;
            g_acc.settings.sfxVol = g_accV2.settings.sfxVol;
            g_acc.settings.fps = g_accV2.settings.fps;
            g_acc.settings.numStars = g_accV2.settings.numStars;
            g_acc.settings.unused050 = g_accV2.settings.unused050;
            g_acc.settings.unused054 = g_accV2.settings.unused054;
            g_acc.settings.sfxOn = g_accV2.settings.sfxOn;
            g_acc.settings.unused05c = g_accV2.settings.unused05c;

            g_acc.settings.collisionDetail = g_accV2.settings.collisionDetail;
            g_acc.settings.bgStars = g_accV2.settings.bgStars;
            g_acc.settings.unused068 = g_accV2.settings.unused068;
            g_acc.settings.bulletIntensity = g_accV2.settings.bulletIntensity;
            g_acc.settings.best = g_accV2.settings.playTime;
            g_acc.settings.device0 = g_accV2.settings.device0;
            g_acc.settings.device1 = g_accV2.settings.device1;
            g_acc.settings.device2 = g_accV2.settings.device2;
            g_acc.settings.device3 = g_accV2.settings.device3;
            g_acc.settings.difficulty = g_accV2.settings.difficulty;
            g_acc.settings.bgEnabled = g_accV2.settings.bgEnabled;
            g_acc.settings.bgTint = g_accV2.settings.bgTint;
            g_acc.settings.unused104 = g_accV2.settings.unused104;
            g_acc.settings.sparks = g_accV2.settings.sparks;
            g_acc.settings.profileSel = g_accV2.settings.profileSel;

            COPY_ALT4(joyFireAlt)
            COPY_ALT4(joyRocketAlt)
            COPY_ALT4(joyBtn2Alt)

            g_acc.settings.voice = g_accV2.settings.voice;
            g_acc.settings.musicVol = g_accV2.settings.musicVol;
            g_acc.settings.version = g_accV2.settings.version;
            g_acc.settings.checkVersion = g_accV2.settings.checkVersion;
            g_acc.settings.renderer = g_accV2.settings.renderer;
            g_acc.settings.shuffleByte = g_accV2.settings.shuffleByte;
            g_acc.settings.windowedByte = g_accV2.settings.windowedByte;
            COPY_ALT4(joyPauseAlt)
            COPY_ALT4(joyProfileAlt)
            g_acc.version = 6;
        }

        // v5 -> v6, from the news-tracking layout (already saveIdHistory-sized): clear the
        // news history and re-derive bonus-level stats that used a different scale before.
        if (g_acc.version == 5 && len == ACCOUNT_SIZE_V2_NEWS) {
            g_acc.settings.checkVersion = 0;
            g_acc.settings.newsIds[0] = -1;
            g_acc.settings.newsCounts[0] = 0;
            g_acc.settings.newsIds[1] = -1;
            g_acc.settings.newsCounts[1] = 0;
            g_acc.settings.newsIds[2] = -1;
            g_acc.settings.newsCounts[2] = 0;
            g_acc.settings.newsIds[3] = -1;
            g_acc.settings.newsCounts[3] = 0;

            CLEAR_MEDAL_ORDER(g_acc)
            g_acc.cheatDetected = 0;
            g_acc.gameCompletedByte = 0;
            g_acc.highestMoney = 0;
            g_acc.secretsInOneGame = 0;
            g_acc.version = 6;
            oldTime = g_acc.ownerStamp;
            g_acc.ownerStamp = MakeRandomId();
            LoadHiscores();
            DecompressHiscores();

            for (j = 0; j < 20; j++) {
                REPOINT_OWNER_STAMP(j, oldTime)
            }
            CompressHiscores();
            WriteHiscoreFile();

            g_acc.medals &= 0;
            g_acc.medalStep = -1;

            for (k7 = 0; k7 < g_numLevels; k7++)
                g_acc.levelDoneBytes[k7] = 0;
            g_acc.levelDoneBytes[25] = 1;  // level 25 (the bonus level) counts as always done
            if (g_acc.bonusLevelsPlayed > 450.0) {
                // Old versions counted bonus levels on a finer scale; rescale down to the
                // new 0..450 range and shrink the perfect-bonus count by the same factor.
                scale = g_acc.bonusLevelsPlayed / 450.0;
                if (scale == 0.0)
                    scale = 1.0;
                g_acc.perfectBonusLevels = g_acc.perfectBonusLevels / scale;
                g_acc.bonusLevelsPlayed = 450;
            }
            if (g_acc.perfectBonusLevels < 0)
                g_acc.perfectBonusLevels = 0;
        }

        // v6 -> v7: add the save-id history (online score submission dedup).
        if (g_acc.version == 6) {
            for (k8 = 0; k8 < 50; k8++)
                g_acc.saveIdHistory[k8] = 0;
            g_acc.lastSaveId = 0;
            g_acc.version = 7;
        }
        if (g_acc.version != 7)
            ResetAccount();  // unknown/newer version, or the chain above didn't reach 7: start fresh
    } else {
        ok = false;
    }
    return ok;
}

#undef REPOINT_OWNER_STAMP
#undef COPY_ALT4

// Scans profiles/profileNNN.acc for slot = 0, 1, 2, ... loading and re-saving (upgrading)
// each one found. On a missing slot, shifts every later slot's .acc/.wpl files down by one
// so the profile numbering stays contiguous, and adjusts g_cfg.profileSel to follow. Stops
// once 10 profiles are found or 10 slots have been checked. Called on startup.
// NOTE: `nnn` is unused here (kept for the stack layout to match the original).
void ScanProfiles()
{
    int fh = 0;
    char nnn[7] = "WARBP1";
    char levelname[1024];
    char oldname[1024];
    char newname[1024];
    int bytesread;
    int slot = 0;
    int count = 0;
    bool done;
    int j;

    MakeProfilesDir();
    done = false;

    do {
        count++;
        sprintf(levelname, "%s\\warblade\\profiles\\profile%03d.acc", SysUserFolder(), slot);
        _set_fmode(_O_BINARY);
        fh = _open(levelname, 0, 0);
        if (fh != -1) {
            bytesread = _read(fh, &g_accBuf[slot], SIZEOF_ACCOUNT);
            g_accSizes[slot] = SIZEOF_ACCOUNT;
            _close(fh);

            if (!DecodeAccount(&g_accBuf[slot], slot, bytesread))
                ResetAccount();
            PackAccount(slot);
            SaveAccount(slot);
            g_profileCount++;
            slot++;
            if (g_profileCount == 10)
                done = true;
        } else {
            if (g_cfg.profileSel > slot)
                g_cfg.profileSel--;
            if (g_cfg.profileSel < -1)
                g_cfg.profileSel = -1;

            for (j = slot + 1; j < 10; j++) {
                sprintf(oldname, "%s\\warblade\\profiles\\profile%03d.acc", SysUserFolder(), j);
                sprintf(newname, "%s\\warblade\\profiles\\profile%03d.acc",
                        SysUserFolder(), j - 1);
                rename(oldname, newname);
                sprintf(oldname, "%s\\warblade\\profiles\\profile%03d.wpl", SysUserFolder(), j);
                sprintf(newname, "%s\\warblade\\profiles\\profile%03d.wpl",
                        SysUserFolder(), j - 1);
                rename(oldname, newname);
            }
            if (count >= 10)
                done = true;
        }
    } while (!done);
    if (g_profileCount == 0)
        g_cfg.profileSel = -1;
}

// Merge the global settings into a profile, keeping the higher best score.
void MergeSettings(int slot)
{
    if (slot != -1) {
        UnpackAccount(slot);
        g_hiscore = g_acc.settings.best;
        g_acc.settings = g_cfg;
        if (g_hiscore > g_acc.settings.best)
            g_acc.settings.best = g_hiscore;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Copy the profile name into the shared text buffer.
void GetProfileName(int slot)
{
    int i;
    g_logBuf[0] = 0;
    if (slot != -1) {
        UnpackAccount(slot);
        for (i = 0; i < NAME_LEN; i++)
            g_logBuf[i] = g_acc.name[i];
        ClearAccount();
    }
}

// Returns a profile's "easy mode" flag.
bool GetProfileEasyFlag(int slot)
{
    bool r = false;
    if (slot != -1) {
        UnpackAccount(slot);
        r = *(bool *)&g_acc.easyByte;
        ClearAccount();
    }
    return r;
}

// Returns a profile's windowed-mode setting.
bool GetProfileCfgFlag(int slot)
{
    bool r = false;
    if (slot != -1) {
        UnpackAccount(slot);
        r = *(bool *)&g_acc.settings.windowedByte;
        ClearAccount();
    }
    return r;
}

// Returns a profile's selected announcer voice index.
int GetProfileVoiceIndex(int slot)
{
    int r = 0;
    if (slot != -1) {
        UnpackAccount(slot);
        r = g_acc.voiceIndex;
        ClearAccount();
    }
    return r;
}

// Sets a profile's selected announcer voice index and saves the profile.
void SetProfileVoiceIndex(int slot, int v)
{
    if (slot != -1) {
        UnpackAccount(slot);
        g_acc.voiceIndex = v;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Sets a profile's last-submitted online-score save id and saves the profile.
void SetProfileLastSaveId(int slot, __int64 v)
{
    if (slot != -1) {
        UnpackAccount(slot);
        g_acc.lastSaveId = v;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Returns a profile's last-submitted online-score save id.
__int64 GetProfileLastSaveId(int slot)
{
    __int64 r = 0;
    if (slot != -1) {
        UnpackAccount(slot);
        r = g_acc.lastSaveId;
        ClearAccount();
    }
    return r;
}

// Decrements a profile's remaining-lives counter (extra-life powerup was lost/used) and
// saves the profile.
void DecProfileLives(int slot)
{
    if (slot != -1) {
        UnpackAccount(slot);
        g_acc.lives--;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Increments a profile's remaining-lives counter, capped at 5, and saves the profile.
void IncProfileLives(int slot)
{
    if (slot != -1) {
        UnpackAccount(slot);
        g_acc.lives++;
        if (g_acc.lives > 5)
            g_acc.lives = 5;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Returns a profile's remaining-lives counter.
int GetProfileLives(int slot)
{
    int r = 0;
    if (slot != -1) {
        UnpackAccount(slot);
        r = g_acc.lives;
        ClearAccount();
    }
    return r;
}

// Resets a profile's remaining-lives counter back to the starting value and saves the
// profile.
// NOTE: the starting value is computed as (2308 / 114) >> 2, i.e. (20 >> 2) = 5, rather
// than the literal 5; kept as-is (matches the original).
void ResetProfileLives(int slot)
{
    int divisor = 114;
    int dividend;
    int startingLives;
    if (slot != -1) {
        UnpackAccount(slot);
        dividend = 2308;
        startingLives = (dividend / divisor) >> 2;
        g_acc.lives = startingLives;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// True if online score id `v` is already present in a profile's save-id history (used to
// avoid resubmitting/reapplying the same score).
bool ProfileHistHas(int slot, __int64 v)
{
    bool r = false;
    int i;
    if (slot != -1) {
        UnpackAccount(slot);
        for (i = 0; i < 50; i++) {
            if (v == g_acc.saveIdHistory[i])
                r = true;
        }
        ClearAccount();
    }
    return r;
}

// Pushes online score id `v` onto a profile's save-id history ring (oldest entry dropped)
// and clears lastSaveId, then saves the profile.
void ProfileHistPush(int slot, __int64 v)
{
    int i;
    if (slot != -1) {
        UnpackAccount(slot);
        for (i = 0; i < 49; i++)
            g_acc.saveIdHistory[i] = g_acc.saveIdHistory[i + 1];
        g_acc.saveIdHistory[49] = v;
        g_acc.lastSaveId = 0;
        PackAccount(slot);
        SaveAccount(slot);
    }
}

// Saves the current profile, clears the active account, closes the profile UI and
// returns to the login/profile-select state, restarting music.
void Logout()
{
    MergeSettings(g_profileIndex);
#ifndef __EMSCRIPTEN__
    DeleteSetPro();
#endif
    ClearAccount();
#ifndef __EMSCRIPTEN__
    g_profileIndex = -1;
#endif
    PlaySample();
    WinCloseAll();
    g_clickWin = -1;
    g_clickItem = -1;
    g_profileWinOpen = 0;
    LoadProfile();
    StartMusic();
}
