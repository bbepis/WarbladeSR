// level.c: Level data: .lvd loading, the packed-level and alien-graphics cache, the loading
// screen, attack patterns, ship frame rects.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "game.h"
#include <zlib.h>


// Local to this file: the unpacked/decompressed size of a packed level image (g_levelRaw).
enum { LEVEL_RAW_SIZE = 0x1cb98 };
// Local to this file: which .lvd family a level comes from (LoadLevelData/PackLevelData `mode`).
enum {
    LEVEL_MODE_CLASSIC     = 0,
    LEVEL_MODE_TIME_TRIAL  = 1,
    LEVEL_MODE_MALFUNCTION = 2,
};
// Local to this file: g_curLevelData array bounds, matching the struct's fixed-size members.
enum {
    LEVEL_MAX_GROUPS  = 25,   // g_curLevelData.grp[]
    MAX_SUBWEAPONS    = 50,   // Group::sub[]
    MAX_PATH_POINTS   = 150,  // g_curLevelData.obj[][]
    MAX_AUX           = 50,   // g_curLevelData.aux[]
    MAX_HEADERS       = 5,    // g_curLevelData.hdr[]
};
// Local to this file: cooldown before StealOldestAlienGfx() can steal another slot.
enum { ALIEN_GFX_STEAL_COOLDOWN_MS = 0x28a };


// Scan columns from the left for the first one whose average is non-zero.
int ScanLeft(unsigned char* data, int left, int top, int width, int height, int pitch, int unusedImgH)
{
    int x;
    int y;
    int sum;
    float avg;
    for (x = 0; x < width; x++) {
        sum = 0;
        for (y = 0; y < height; y++)
            sum = sum + data[top * pitch + y * pitch + left + x];
        if (height == 0)
            height = 1;
        avg = (float)sum / height;
        if (avg > 0.0)
            return x;
    }
    return 0;
}

// Scan rows from the top.
int ScanTop(unsigned char* data, int left, int top, int width, int height, int pitch, int unusedImgH)
{
    int x;
    int y;
    int sum;
    float avg;
    for (y = 0; y < height; y++) {
        sum = 0;
        for (x = 0; x < width; x++)
            sum = sum + data[top * pitch + y * pitch + left + x];
        if (width == 0)
            width = 1;
        avg = (float)sum / width;
        if (avg > 0.0)
            return y;
    }
    return 0;
}

// Scan columns from the right.
int ScanRight(unsigned char* data, int left, int top, int width, int height, int pitch, int unusedImgH)
{
    int x;
    int y;
    int sum;
    float avg;
    for (x = width; x > 0; x--) {
        sum = 0;
        for (y = 0; y < height; y++)
            sum = sum + data[top * pitch + y * pitch + left + x];
        if (height == 0)
            height = 1;
        avg = (float)sum / height;
        if (avg > 0.0)
            return x;
    }
    return width;
}

// Scan rows from the bottom.
int ScanBottom(unsigned char* data, int left, int top, int width, int height, int pitch, int unusedImgH)
{
    int x;
    int y;
    int sum;
    float avg;
    for (y = height; y > 0; y--) {
        sum = 0;
        for (x = 0; x < width; x++)
            sum = sum + data[top * pitch + y * pitch + left + x];
        if (width == 0)
            width = 1;
        avg = (float)sum / width;
        if (avg > 0.0)
            return y;
    }
    return height;
}

// Scans two fixed 31x31 (0x1f) regions of `img` (the ship's "frame A" and "frame B" corner sprites, at
// x=0x1e0 and x=0x1c0) for their tight bounding boxes, storing the results in g_frameA*/g_frameB*.
void ScanFrameRects(void* img, int w, int h)
{
    g_frameAX1 = ScanLeft((unsigned char *)img, 0x1e0, 0, 0x1f, 0x1f, w, h);
    g_frameAY1 = ScanTop((unsigned char *)img, 0x1e0, 0, 0x1f, 0x1f, w, h);
    g_frameAX2 = ScanRight((unsigned char *)img, 0x1e0, 0, 0x1f, 0x1f, w, h);
    g_frameAY2 = ScanBottom((unsigned char *)img, 0x1e0, 0, 0x1f, 0x1f, w, h);
    g_frameBX1 = ScanLeft((unsigned char *)img, 0x1c0, 0, 0x1f, 0x1f, w, h);
    g_frameBY1 = ScanTop((unsigned char *)img, 0x1c0, 0, 0x1f, 0x1f, w, h);
    g_frameBX2 = ScanRight((unsigned char *)img, 0x1c0, 0, 0x1e, 0x1e, w, h);
    g_frameBY2 = ScanBottom((unsigned char *)img, 0x1c0, 0, 0x1e, 0x1e, w, h);
}

// Re-scans ship graphic slot i's frame rects (if its "gfx1" is loaded) and copies the results into
// g_frames[i], used to draw the ship-select highlight frame around the right-sized portrait.
void CacheFrameRect(int i)
{
    if (i == 0 && g_alienGfxCache[0].gfx1)
        ScanFrameRects(g_alienGfxMem[0], (int)ImgWidth(g_alienGfxCache[0].gfx1),
                             (int)ImgHeight(g_alienGfxCache[0].gfx1));
    if (i == 1 && g_alienGfxCache[1].gfx1)
        ScanFrameRects(g_alienGfxMem[1], (int)ImgWidth(g_alienGfxCache[1].gfx1),
                             (int)ImgHeight(g_alienGfxCache[1].gfx1));
    if (i == 2 && g_alienGfxCache[2].gfx1)
        ScanFrameRects(g_alienGfxMem[2], (int)ImgWidth(g_alienGfxCache[2].gfx1),
                             (int)ImgHeight(g_alienGfxCache[2].gfx1));
    if (i == 3 && g_alienGfxCache[3].gfx1)
        ScanFrameRects(g_alienGfxMem[3], (int)ImgWidth(g_alienGfxCache[3].gfx1),
                             (int)ImgHeight(g_alienGfxCache[3].gfx1));
    if (i == 4 && g_alienGfxCache[4].gfx1)
        ScanFrameRects(g_alienGfxMem[4], (int)ImgWidth(g_alienGfxCache[4].gfx1),
                             (int)ImgHeight(g_alienGfxCache[4].gfx1));
    if (i == 5 && g_alienGfxCache[5].gfx1)
        ScanFrameRects(g_alienGfxMem[5], (int)ImgWidth(g_alienGfxCache[5].gfx1),
                             (int)ImgHeight(g_alienGfxCache[5].gfx1));

    g_frames[i].raw[4] = g_frameAX1;
    g_frames[i].raw[5] = g_frameAY1;
    g_frames[i].raw[6] = g_frameAX2;
    g_frames[i].raw[7] = g_frameAY2;
    g_frames[i].raw[0] = g_frameBX1;
    g_frames[i].raw[1] = g_frameBY1;
    g_frames[i].raw[2] = g_frameBX2;
    g_frames[i].raw[3] = g_frameBY2;
}

// Resets frame-rect slot i to its default 32x32 (A) and 64x64 (B) boxes, before ScanFrameRects overwrites
// them with the actual scanned bounds.
void InitFrameRectDefaults(int i)
{
    g_rectsA[i].x1 = 0;
    g_rectsA[i].y1 = 0;
    g_rectsA[i].x2 = 0x20;
    g_rectsA[i].y2 = 0x20;
    g_rectsB[i].x1 = 0;
    g_rectsB[i].y1 = 0;
    g_rectsB[i].x2 = 0x40;
    g_rectsB[i].y2 = 0x40;
}

// Counts how many "timetrial_NN.lvd" level files (01-50) exist on disk.
int CountTimeTrialLevels()
{
    char levelname[512];
    int count;
    int i;

    count = 0;
    for (i = 0; i < MAX_TIME_TRIAL_LEVELS; i++) {
        sprintf(levelname, "timetrial_%02d.lvd", i + 1);
        if (PacExists(levelname))
            count++;
    }
    return count;
}

// Refreshes the time-trial level count; if none are installed, locks the mode for 10s and returns false.
int CheckTimeTrialAvailable()
{
    int n = CountTimeTrialLevels();
    g_numLevels2 = n;
    if (n != 0) {
        g_timeTrialLocked = 0;
        return 1;
    }
    g_timeTrialDeadline = g_time + 10000;
    g_timeTrialLocked = 1;
    return 0;
}

// Counts how many "classic_level_NNN.lvd" level files (001-500) exist on disk.
int CountClassicLevels()
{
    char levelname[512];
    int count;
    int i;

    count = 0;
    for (i = 0; i < MAX_CLASSIC_LEVELS; i++) {
        sprintf(levelname, "classic_level_%03d.lvd", i + 1);
        if (PacExists(levelname))
            count++;
    }
    return count;
}

// Frees the two level-packing scratch buffers, if allocated.
void FreeBuffers()
{
    if (g_levelBufA) {
        free(g_levelBufA);
        g_levelBufA = 0;
    }
    if (g_levelBufB) {
        free(g_levelBufB);
        g_levelBufB = 0;
    }
}

// Frees one alien-graphics cache slot's two pictures and its HMA hit-mask buffer, if set.
#define RELEASE_CACHE(n) \
    if (g_alienGfxCache[n].gfx1) { \
        ImgFreePicture(g_alienGfxCache[n].gfx1); \
        ImgFree(g_alienGfxCache[n].gfx1); \
        g_alienGfxCache[n].gfx1 = 0; \
        g_alienGfxFreedA++; \
    } \
    if (g_alienGfxCache[n].gfx2) { \
        ImgFreePicture(g_alienGfxCache[n].gfx2); \
        ImgFree(g_alienGfxCache[n].gfx2); \
        g_alienGfxCache[n].gfx2 = 0; \
        g_alienGfxFreedB++; \
    } \
    if (g_alienGfxMem[n]) { \
        free(g_alienGfxMem[n]); \
        g_alienGfxMem[n] = 0; \
        g_alienGfxFreedM++; \
    }

// Frees all 6 alien-graphics cache slots and resets the per-frame draw-call counters.
void ReleaseAlienGfxCache()
{
    RELEASE_CACHE(0)
    RELEASE_CACHE(1)
    RELEASE_CACHE(2)
    RELEASE_CACHE(3)
    RELEASE_CACHE(4)
    RELEASE_CACHE(5)
    g_blitCount = 0;
    g_blit2Count = 0;
    g_quadCount = 0;
    g_stretchFCount = 0;
    g_stretchRotCount = 0;
    g_stretchRot2Count = 0;
}

#undef RELEASE_CACHE

// Looks for another loaded sound-slot channel sharing graphics `key` (so its already-loaded
// gfx/hma can be shared instead of reloading). Skips (slot, chan) itself. Sets g_foundSlot/
// g_foundChan and returns true on a hit.
bool FindOtherSlotWithKey(int slot, int chan, int key)
{
    int i;
    int j;

    for (i = 0; i < MAX_ALIEN_GFX_SLOTS; i++) {
        for (j = 0; j < NUM_HAZARD_GFX; j++) {
            if (g_alienGfxSlots[i].loaded[j] && g_alienGfxSlots[i].key[j] == key
                && (i != slot || j != chan)) {
                g_foundSlot = i;
                g_foundChan = j;
                return true;
            }
        }
    }
    return false;
}

// Finds the slot/channel using graphics `snd` and boosts its age count so it survives
// eviction by StealOldestAlienGfx() a while longer.
void BumpAlienGfxAge(Image *snd)
{
    int i;
    int j;

    for (i = 0; i < MAX_ALIEN_GFX_SLOTS; i++) {
        for (j = 0; j < NUM_HAZARD_GFX; j++) {
            if (g_alienGfxSlots[i].loaded[j] && g_alienGfxSlots[i].gfx[j] == snd) {
                g_alienGfxSlots[i].count[j] += 200;
                return;
            }
        }
    }
}

// Finds the slot/channel using graphics `snd` and lowers its age count, making it more
// likely to be reused/evicted next; clamps to a small positive floor instead of going negative.
void DropAlienGfxAge(Image *snd)
{
    int i;
    int j;

    for (i = 0; i < MAX_ALIEN_GFX_SLOTS; i++) {
        for (j = 0; j < NUM_HAZARD_GFX; j++) {
            if (g_alienGfxSlots[i].gfx[j] == snd) {
                g_alienGfxSlots[i].count[j] -= 0x62;
                if (g_alienGfxSlots[i].count[j] < 0)
                    g_alienGfxSlots[i].count[j] = 5;
                return;
            }
        }
    }
}

// Evicts the least-recently-used alien graphics/hma from the sound-slot cache to make room
// for loading level (owner, id). Skips the slot belonging to (owner, id) itself, and skips
// any slot currently in use as a live player shield graphic. Called before loading a level's
// gfx so g_gfxLoaded stays within its buffer.
void StealOldestAlienGfx(int owner, short id)
{
    int best = 10000000;
    int bestJ = 0;
    int bestI = 0;
    int k;
    int i;
    int j;
    bool ok;
    int i2;
    int j2;

    for (i = 0; i < MAX_ALIEN_GFX_SLOTS; i++) {
        for (j = 0; j < NUM_HAZARD_GFX; j++) {
            if (g_alienGfxSlots[i].loaded[j]
                && (g_alienGfxSlots[i].owner != owner || g_alienGfxSlots[i].id != id)
                && g_alienGfxSlots[i].count[j] <= best) {
                ok = true;
                // Don't steal graphics currently shown as a player's active shield.
                if (g_save.players[0].shieldL) {
                    k = g_save.players[0].shieldLIdx;
                    if (g_enemies[0][k].gfxA && g_enemies[0][k].gfxA == g_alienGfxSlots[i].gfx[j])
                        ok = false;
                }
                if (g_save.players[0].shieldR) {
                    k = g_save.players[0].shieldRIdx;
                    if (g_enemies[0][k].gfxA && g_enemies[0][k].gfxA == g_alienGfxSlots[i].gfx[j])
                        ok = false;
                }

                if (g_gameMode == MODE_TWO_PLAYER || g_gameMode == MODE_DUAL || g_gameMode == MODE_TEAM) {
                    // Two-player modes: also protect player 2's shields.
                    if (g_save.players[1].shieldL) {
                        k = g_save.players[1].shieldLIdx;
                        if (g_enemies[1][k].gfxA && g_enemies[1][k].gfxA == g_alienGfxSlots[i].gfx[j])
                            ok = false;
                    }
                    if (g_save.players[1].shieldR) {
                        k = g_save.players[1].shieldRIdx;
                        if (g_enemies[1][k].gfxA && g_enemies[1][k].gfxA == g_alienGfxSlots[i].gfx[j])
                            ok = false;
                    }
                }

                if (ok) {
                    best = g_alienGfxSlots[i].count[j];
                    bestJ = j;
                    bestI = i;
                }
            }
        }
    }

    if (best < 10000000) {
        // Clear stale (unloaded) slot entries that were only sharing this same gfx pointer,
        // so nothing is left pointing at memory we're about to free below.
        if (g_alienGfxSlots[bestI].gfx[bestJ]) {
            for (i2 = 0; i2 < MAX_ALIEN_GFX_SLOTS; i2++) {
                for (j2 = 0; j2 < NUM_HAZARD_GFX; j2++) {
                    if (!g_alienGfxSlots[i2].loaded[j2]
                        && g_alienGfxSlots[i2].gfx[j2] == g_alienGfxSlots[bestI].gfx[bestJ]) {
                        g_alienGfxSlots[i2].gfx[j2] = 0;
                        g_alienGfxSlots[i2].gfx2[j2] = 0;
                        g_alienGfxSlots[i2].hma[j2] = 0;
                        g_alienGfxSlots[i2].count[j2] = 0;
                    }
                }
            }
        }

        // Unlink from the fixed-slot alien graphics cache (g_alienGfxCache[0..5]) before freeing,
        // then release the picture and count it toward the frame's freed-graphics stats.
        if (g_alienGfxSlots[bestI].gfx[bestJ]) {
            if (g_alienGfxCache[0].gfx1 == g_alienGfxSlots[bestI].gfx[bestJ]) g_alienGfxCache[0].gfx1 = 0;
            if (g_alienGfxCache[1].gfx1 == g_alienGfxSlots[bestI].gfx[bestJ]) g_alienGfxCache[1].gfx1 = 0;
            if (g_alienGfxCache[2].gfx1 == g_alienGfxSlots[bestI].gfx[bestJ]) g_alienGfxCache[2].gfx1 = 0;
            if (g_alienGfxCache[3].gfx1 == g_alienGfxSlots[bestI].gfx[bestJ]) g_alienGfxCache[3].gfx1 = 0;
            if (g_alienGfxCache[4].gfx1 == g_alienGfxSlots[bestI].gfx[bestJ]) g_alienGfxCache[4].gfx1 = 0;
            if (g_alienGfxCache[5].gfx1 == g_alienGfxSlots[bestI].gfx[bestJ]) g_alienGfxCache[5].gfx1 = 0;
            ImgFreePicture(g_alienGfxSlots[bestI].gfx[bestJ]);
            ImgFree(g_alienGfxSlots[bestI].gfx[bestJ]);
            g_alienGfxSlots[bestI].gfx[bestJ] = 0;
            g_alienGfxFreedA++;
        }

        if (g_alienGfxSlots[bestI].gfx2[bestJ]) {
            if (g_alienGfxCache[0].gfx2 == g_alienGfxSlots[bestI].gfx2[bestJ]) g_alienGfxCache[0].gfx2 = 0;
            if (g_alienGfxCache[1].gfx2 == g_alienGfxSlots[bestI].gfx2[bestJ]) g_alienGfxCache[1].gfx2 = 0;
            if (g_alienGfxCache[2].gfx2 == g_alienGfxSlots[bestI].gfx2[bestJ]) g_alienGfxCache[2].gfx2 = 0;
            if (g_alienGfxCache[3].gfx2 == g_alienGfxSlots[bestI].gfx2[bestJ]) g_alienGfxCache[3].gfx2 = 0;
            if (g_alienGfxCache[4].gfx2 == g_alienGfxSlots[bestI].gfx2[bestJ]) g_alienGfxCache[4].gfx2 = 0;
            if (g_alienGfxCache[5].gfx2 == g_alienGfxSlots[bestI].gfx2[bestJ]) g_alienGfxCache[5].gfx2 = 0;
            ImgFreePicture(g_alienGfxSlots[bestI].gfx2[bestJ]);
            ImgFree(g_alienGfxSlots[bestI].gfx2[bestJ]);
            g_alienGfxSlots[bestI].gfx2[bestJ] = 0;
            g_alienGfxFreedB++;
        }

        if (g_alienGfxSlots[bestI].hma[bestJ]) {
            if (g_alienGfxMem[0] == g_alienGfxSlots[bestI].hma[bestJ]) g_alienGfxMem[0] = 0;
            if (g_alienGfxMem[1] == g_alienGfxSlots[bestI].hma[bestJ]) g_alienGfxMem[1] = 0;
            if (g_alienGfxMem[2] == g_alienGfxSlots[bestI].hma[bestJ]) g_alienGfxMem[2] = 0;
            if (g_alienGfxMem[3] == g_alienGfxSlots[bestI].hma[bestJ]) g_alienGfxMem[3] = 0;
            if (g_alienGfxMem[4] == g_alienGfxSlots[bestI].hma[bestJ]) g_alienGfxMem[4] = 0;
            if (g_alienGfxMem[5] == g_alienGfxSlots[bestI].hma[bestJ]) g_alienGfxMem[5] = 0;
            free(g_alienGfxSlots[bestI].hma[bestJ]);
            g_alienGfxSlots[bestI].hma[bestJ] = 0;
            g_alienGfxFreedM++;
        }
        g_alienGfxSlots[bestI].loaded[bestJ] = 0;
        g_alienGfxSlots[bestI].count[bestJ] = 0;
    }
}

// The on-disk/packed level image (g_levelRaw) is little-endian, matching the live level
// struct field-for-field, so unpacking is a plain copy into g_curLevelData.

// Shared unpack helpers used by both LoadLevelData() and PackLevelData(): the two functions
// unpack the same g_levelRaw fields into g_curLevelData, but PackLevelData interleaves extra
// has1..has6 hazard-presence bookkeeping into the header/sub-weapon loops, so only the pieces
// that are byte-for-byte identical in both places are factored out here.

// Clears every group/sub-weapon/object-path slot before unpacking (see the comment at its use).
#define CLEAR_LEVEL_SLOTS()                                             \
    for (grpClrIdx = 0; grpClrIdx < LEVEL_MAX_GROUPS; grpClrIdx++) {    \
        for (subClrIdx = 0; subClrIdx < MAX_SUBWEAPONS; subClrIdx++) { \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].xOffset = 0;  \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].yOffset = 0;  \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].fireDelay = 0; \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].pathId = 0;   \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].type = 0;     \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].hp = 0;       \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].fireRateMin = 0; \
            g_curLevelData.grp[grpClrIdx].sub[subClrIdx].fireRateMax = 0; \
        }                                                               \
        for (objClrIdx = 0; objClrIdx < MAX_PATH_POINTS; objClrIdx++) { \
            g_curLevelData.obj[grpClrIdx][objClrIdx].pathX = 0;        \
            g_curLevelData.obj[grpClrIdx][objClrIdx].pathY = 0;        \
            g_curLevelData.obj[grpClrIdx][objClrIdx].cmd = 0;          \
            g_curLevelData.obj[grpClrIdx][objClrIdx].unusedD = 0;      \
            g_curLevelData.obj[grpClrIdx][objClrIdx].holdTime = 0;     \
        }                                                               \
    }

// Unpacks header slot `hdrIdx`'s fields.
#define UNPACK_HDR_FIELDS(hdrIdx)                                                    \
    g_curLevelData.hdr[hdrIdx].type = g_levelRaw.hdr[hdrIdx].type; \
    g_curLevelData.hdr[hdrIdx].count = g_levelRaw.hdr[hdrIdx].count; \
    g_curLevelData.hdr[hdrIdx].hp = g_levelRaw.hdr[hdrIdx].hp;  \
    g_curLevelData.hdr[hdrIdx].fireRateMin =                                         \
        g_levelRaw.hdr[hdrIdx].fireRateMin;                    \
    g_curLevelData.hdr[hdrIdx].fireRateMax =                                         \
        g_levelRaw.hdr[hdrIdx].fireRateMax;

// Unpacks group slot `grpIdx`'s own fields (not its sub-weapons).
#define UNPACK_GRP_FIELDS(grpIdx)                                                              \
    g_curLevelData.grp[grpIdx].spawnX = g_levelRaw.grp[grpIdx].spawnX;   \
    g_curLevelData.grp[grpIdx].spawnY = g_levelRaw.grp[grpIdx].spawnY;   \
    g_curLevelData.grp[grpIdx].spawnDelay = g_levelRaw.grp[grpIdx].spawnDelay; \
    g_curLevelData.grp[grpIdx].spawnStep = g_levelRaw.grp[grpIdx].spawnStep; \
    g_curLevelData.grp[grpIdx].count = g_levelRaw.grp[grpIdx].count;     \
    g_curLevelData.grp[grpIdx].velX = g_levelRaw.grp[grpIdx].velX / 256.0; \
    g_curLevelData.grp[grpIdx].velY = g_levelRaw.grp[grpIdx].velY / 256.0; \
    g_curLevelData.grp[grpIdx].groupId = g_levelRaw.grp[grpIdx].groupId; \
    g_curLevelData.grp[grpIdx].kind = g_levelRaw.grp[grpIdx].kind;

// Unpacks group `grpIdx`'s sub-weapon slot `subIdx`'s fields.
#define UNPACK_SUB_FIELDS(grpIdx, subIdx)                                                          \
    g_curLevelData.grp[grpIdx].sub[subIdx].xOffset =                                               \
        g_levelRaw.grp[grpIdx].sub[subIdx].xOffset;                          \
    g_curLevelData.grp[grpIdx].sub[subIdx].yOffset =                                               \
        g_levelRaw.grp[grpIdx].sub[subIdx].yOffset;                          \
    g_curLevelData.grp[grpIdx].sub[subIdx].fireDelay =                                             \
        g_levelRaw.grp[grpIdx].sub[subIdx].fireDelay;                        \
    g_curLevelData.grp[grpIdx].sub[subIdx].pathId =                                                \
        g_levelRaw.grp[grpIdx].sub[subIdx].pathId;                           \
    g_curLevelData.grp[grpIdx].sub[subIdx].type =                                                  \
        g_levelRaw.grp[grpIdx].sub[subIdx].type;                             \
    g_curLevelData.grp[grpIdx].sub[subIdx].hp =                                                    \
        g_levelRaw.grp[grpIdx].sub[subIdx].hp;                               \
    g_curLevelData.grp[grpIdx].sub[subIdx].fireRateMin =                                           \
        g_levelRaw.grp[grpIdx].sub[subIdx].fireRateMin;                      \
    g_curLevelData.grp[grpIdx].sub[subIdx].fireRateMax =                                           \
        g_levelRaw.grp[grpIdx].sub[subIdx].fireRateMax;

// Unpacks group `grpIdx`'s objCount entry.
#define UNPACK_OBJCOUNT(cntIdx) \
    g_curLevelData.objCount[cntIdx] = g_levelRaw.objCount[cntIdx];

// Copies the Pascal-style (length-prefixed) level name and subtitle out of g_levelRaw.
#define COPY_LEVEL_NAMES()                                              \
    len = g_levelRaw.name1[0];                                          \
    for (k = 0; k < len + 1; k++)                                       \
        g_curLevelData.name1[k] = g_levelRaw.name1[k];                  \
    g_curLevelData.name2[len] = 0;                                      \
    len = g_levelRaw.name2[0];                                          \
    for (name2Idx = 1; name2Idx < len - 1; name2Idx++)                  \
        g_curLevelData.name2[name2Idx] = g_levelRaw.name2[name2Idx];    \
    g_curLevelData.name2[len] = 0;

// Unpacks group `objGrpIdx`'s path-point `objIdx`.
#define UNPACK_OBJ_PATH(objGrpIdx, objIdx)                                                   \
    g_curLevelData.obj[objGrpIdx][objIdx].pathX =                                            \
        g_levelRaw.obj[objGrpIdx][objIdx].pathX / 256.0;               \
    g_curLevelData.obj[objGrpIdx][objIdx].pathY =                                            \
        g_levelRaw.obj[objGrpIdx][objIdx].pathY / 256.0;               \
    g_curLevelData.obj[objGrpIdx][objIdx].cmd =                                              \
        g_levelRaw.obj[objGrpIdx][objIdx].cmd;                         \
    g_curLevelData.obj[objGrpIdx][objIdx].unusedD =                                          \
        g_levelRaw.obj[objGrpIdx][objIdx].unusedD;                     \
    g_curLevelData.obj[objGrpIdx][objIdx].holdTime =                                         \
        g_levelRaw.obj[objGrpIdx][objIdx].holdTime;

// Unpacks aux slot `auxIdx` (raw fields are named a/c, not x1/x2, but map straight across).
#define UNPACK_AUX_FIELDS(auxIdx)                                                    \
    g_curLevelData.aux[auxIdx].y2 = g_levelRaw.aux[auxIdx].y2; \
    g_curLevelData.aux[auxIdx].y1 = g_levelRaw.aux[auxIdx].y1; \
    g_curLevelData.aux[auxIdx].x2 = g_levelRaw.aux[auxIdx].x2; \
    g_curLevelData.aux[auxIdx].x1 = g_levelRaw.aux[auxIdx].x1;

// Finds the sound-slot cache entry already holding the current level (owner = level number,
// id = mode: LEVEL_MODE_CLASSIC/TIME_TRIAL/MALFUNCTION) and unpacks it into g_curLevelData:
// decompresses g_levelRaw, copies every field into g_curLevelData, and hooks up the level's alien graphics
// (loading them now if this is the first time the level is played). Called on level start.
void LoadLevelData()
{
    short mode = 0;
    int level = 0;
    unsigned long usize;
    unsigned long srcLen;
    int i;
    int res;
    int grpClrIdx;
    int subClrIdx;
    int objClrIdx;
    int hdrIdx;
    int grpIdx;
    int subIdx;
    int cntIdx;
    int len;
    int k;
    int name2Idx;
    int objGrpIdx;
    int objIdx;
    int auxIdx;
    int j;

    // ---- mode ----
    // mode: LEVEL_MODE_CLASSIC / LEVEL_MODE_TIME_TRIAL / LEVEL_MODE_MALFUNCTION.
    if (g_state == STATE_MALFUNCTION) {
        mode = LEVEL_MODE_MALFUNCTION;
        level = g_curLevelNum;
    } else if (g_gameMode == MODE_TIME_TRIAL) {
        mode = LEVEL_MODE_TIME_TRIAL;
        level = g_curLevelNum - 1 % g_numLevels2 + 1;
    } else {
        mode = LEVEL_MODE_CLASSIC;
        level = g_curLevelNum - 1 % 100 + 1;
    }

    for (i = 0; i < MAX_ALIEN_GFX_SLOTS; i++) {
        if (g_alienGfxSlots[i].owner == level && g_alienGfxSlots[i].id == mode) {
            // ---- decompress and clear ----
            usize = LEVEL_RAW_SIZE;  // decompressed size of a packed level image
            srcLen = g_alienGfxSlots[i].srcLen;
            memset(&g_levelRaw, 0, LEVEL_RAW_SIZE);
            res = uncompress((unsigned char *)&g_levelRaw, (unsigned long *)&usize,
                             (const unsigned char *)g_alienGfxSlots[i].src, srcLen);

            // Clear all group/sub-weapon and object-path slots before unpacking; the packed
            // image only carries entries up to g_curLevelData.count/objCount, leaving the rest
            // as leftover data from the previous level otherwise.
            CLEAR_LEVEL_SLOTS()

            // ---- unpack header / groups / objects / aux ----
            g_curLevelData.type = g_levelRaw.type;
            g_curLevelData.count = g_levelRaw.count;
            for (hdrIdx = 0; hdrIdx < MAX_HEADERS; hdrIdx++) {
                UNPACK_HDR_FIELDS(hdrIdx)
            }

            for (grpIdx = 0; grpIdx < g_curLevelData.count; grpIdx++) {
                UNPACK_GRP_FIELDS(grpIdx)

                for (subIdx = 0; subIdx < g_curLevelData.grp[grpIdx].count; subIdx++) {
                    UNPACK_SUB_FIELDS(grpIdx, subIdx)
                }
            }

            for (cntIdx = 0; cntIdx < g_curLevelData.count; cntIdx++)
                UNPACK_OBJCOUNT(cntIdx)

            // Level/subtitle names are Pascal-style: byte [0] is the length, so this copies
            // the length byte plus the string itself.
            COPY_LEVEL_NAMES()

            for (objGrpIdx = 0; objGrpIdx < g_curLevelData.count; objGrpIdx++) {
                for (objIdx = 0; objIdx < g_curLevelData.objCount[objGrpIdx]; objIdx++) {
                    UNPACK_OBJ_PATH(objGrpIdx, objIdx)
                }
            }

            // NOTE: raw fields are named a/c (not x1/x2) but map straight across.
            for (auxIdx = 0; auxIdx < MAX_AUX; auxIdx++) {
                UNPACK_AUX_FIELDS(auxIdx)
            }

            // ---- hazard graphics hookup ----
            g_animFrameCount = g_alienGfxSlots[i].frameCount;
            // Hook up each of the level's 6 alien-graphics channels: if the slot already has
            // graphics loaded (from a previous play of this level), point the fixed hazard
            // gfx/hma cache slots (0-5) at them and bump their reference/age count; otherwise
            // (below) load them fresh.
// Points fixed hazard-graphics cache slot `n`'s gfx1/gfx2/hma at channel j's, and sets its
// frame size (HAZARD_GFX_W x HAZARD_GFX_H, the fixed size for hazard sprites).
#define CACHE_HAZARD_GFX(n)                                     \
    g_alienGfxCache[n].gfx1 = g_alienGfxSlots[i].gfx[j];         \
    g_alienGfxCache[n].gfx2 = g_alienGfxSlots[i].gfx2[j];        \
    g_alienGfxMem[n] = g_alienGfxSlots[i].hma[j];                \
    g_hazard##n##GfxW = HAZARD_GFX_W;                            \
    g_hazard##n##GfxH = HAZARD_GFX_H;
            for (j = 0; j < NUM_HAZARD_GFX; j++) {
                if (g_alienGfxSlots[i].gfx[j] != 0) {
                    if (j == 0) {
                        CACHE_HAZARD_GFX(0)
                    }

                    if (j == 1) {
                        CACHE_HAZARD_GFX(1)
                    }
                    if (j == 2) {
                        CACHE_HAZARD_GFX(2)
                    }

                    if (j == 3) {
                        CACHE_HAZARD_GFX(3)
                    }
                    if (j == 4) {
                        CACHE_HAZARD_GFX(4)
                    }
                    if (j == 5) {
                        CACHE_HAZARD_GFX(5)
                    }

                    g_frames[j] = g_alienGfxSlots[i].blk[j];
                    g_curLevelData.w[j] = g_alienGfxSlots[i].frameW[j];
                    g_curLevelData.h[j] = g_alienGfxSlots[i].frameH[j];
                    if (g_alienGfxSlots[i].loaded[j]) {
                        g_alienGfxSlots[i].count[j]++;
                    } else if (FindOtherSlotWithKey(i, j, g_alienGfxSlots[i].key[j])) {
                        g_alienGfxSlots[g_foundSlot].count[g_foundChan]++;
                    }
                } else {
                    // Not loaded yet: still expose whatever block/size data the slot has, then
                    // load the graphics fresh if the level actually has a channel here.
                    g_frames[j] = g_alienGfxSlots[i].blk[j];
                    g_curLevelData.w[j] = g_alienGfxSlots[i].frameW[j];
                    g_curLevelData.h[j] = g_alienGfxSlots[i].frameH[j];

                    if (g_alienGfxSlots[i].name1[j][0] != 0) {
                        StealOldestAlienGfx(level, mode);
                        g_soundStealCooldown = g_time + ALIEN_GFX_STEAL_COOLDOWN_MS;  // 650 ms before another steal
                        g_alienGfxSlots[i].gfx[j] = LoadGraphic2(g_alienGfxSlots[i].name1[j], 1, 1);
                        if (g_alienGfxSlots[i].gfx[j] != 0) {
                            g_gfxLoaded++;
                            g_alienGfxSlots[i].gfx2[j] = LoadGraphic2(g_alienGfxSlots[i].name2[j], 1, 1);
                            if (g_alienGfxSlots[i].gfx2[j] != 0) {
                                g_gfx2Loaded++;
                                g_alienGfxSlots[i].hma[j] = LoadHma(g_alienGfxSlots[i].name3[j],
                                    (int)ImgWidth(g_alienGfxSlots[i].gfx[j]),
                                    (int)ImgHeight(g_alienGfxSlots[i].gfx[j]));
                                if (g_alienGfxSlots[i].hma[j] != 0) {
                                    g_hmaLoaded++;
                                    ScanFrameRects(g_alienGfxSlots[i].hma[j],
                                        (int)ImgWidth(g_alienGfxSlots[i].gfx[j]),
                                        (int)ImgHeight(g_alienGfxSlots[i].gfx[j]));

                                    g_alienGfxSlots[i].blk[j].aLeft = g_frameAX1;
                                    g_alienGfxSlots[i].blk[j].aTop = g_frameAY1;
                                    g_alienGfxSlots[i].blk[j].aWidth = g_frameAX2;
                                    g_alienGfxSlots[i].blk[j].aHeight = g_frameAY2;
                                    g_alienGfxSlots[i].blk[j].bLeft = g_frameBX1;
                                    g_alienGfxSlots[i].blk[j].bTop = g_frameBY1;
                                    g_alienGfxSlots[i].blk[j].bWidth = g_frameBX2;
                                    g_alienGfxSlots[i].blk[j].bHeight = g_frameBY2;
                                    g_frames[j] = g_alienGfxSlots[i].blk[j];
                                    g_alienGfxSlots[i].loaded[j] = 1;
                                    g_alienGfxSlots[i].count[j] = 1;

                                    if (j == 0) {
                                        CACHE_HAZARD_GFX(0)
                                    }
                                    if (j == 1) {
                                        CACHE_HAZARD_GFX(1)
                                    }
                                    if (j == 2) {
                                        CACHE_HAZARD_GFX(2)
                                    }

                                    if (j == 3) {
                                        CACHE_HAZARD_GFX(3)
                                    }

                                    if (j == 4) {
                                        CACHE_HAZARD_GFX(4)
                                    }
                                    if (j == 5) {
                                        CACHE_HAZARD_GFX(5)
                                    }
                                }
                            }
                        }
                    }
                }
            }
#undef CACHE_HAZARD_GFX

            break;
        }
    }
}

// Loads level `level` (mode: LEVEL_MODE_CLASSIC/TIME_TRIAL/MALFUNCTION) from disk into
// g_levelRaw/g_curLevelData, derives the .tga/mask/hitmask filenames for each of its 6
// alien-graphics channels from the raw gfx path, recompresses the raw image into sound-slot
// `slot` for later reuse by LoadLevelData(), and preloads the graphics (sharing them with an
// existing slot that already has the same key when possible). Returns the compressed size.
// Called by BufferAllLevels() while filling the level buffer.
int PackLevelData(int slot, int level, short mode)
{
    char filename[255];
    int nGfx = 0;
    bool has1 = 0;
    bool has2 = 0;
    bool has3 = 0;
    bool has4 = 0;
    bool has5 = 0;
    bool has6 = 0;
    int grpClrIdx;
    int subClrIdx;
    int objClrIdx;
    int hdrIdx;
    int grpIdx;
    int subIdx;
    int cntIdx;
    int len;
    int k;
    int name2Idx;
    int objGrpIdx;
    int objIdx;
    int auxIdx;
    int j;
    int i;

    int start;
    int stop;
    int dot;
    bool found;
    bool end;
    int n1;
    int n2;
    int n3;
    void *buf;
    void *buf2;
    bool res;
    int usize;
    int csize;
    int s;

    if (mode == LEVEL_MODE_MALFUNCTION)
        LoadMalfunctionLevel(level);
    else if (mode == LEVEL_MODE_TIME_TRIAL)
        LoadTimeTrialLevel(level);
    else
        LoadClassicLevel(level);

    // ---- unpack ----
    CLEAR_LEVEL_SLOTS()
    has1 = 0;
    has2 = 0;
    has3 = 0;
    has4 = 0;
    has5 = 0;
    has6 = 0;

    g_curLevelData.type = g_levelRaw.type;
    g_curLevelData.count = g_levelRaw.count;
    // NOTE: any header entry at all marks every hazard type (1-6) as present, unconditionally;
    // the per-type has1..has6 flags only get selectively cleared by the sub-weapon scan below.
    for (hdrIdx = 0; hdrIdx < MAX_HEADERS; hdrIdx++) {
        UNPACK_HDR_FIELDS(hdrIdx)
        has1 = 1;
        has2 = 1;
        has3 = 1;
        has4 = 1;
        has5 = 1;
        has6 = 1;
    }

    for (grpIdx = 0; grpIdx < g_curLevelData.count; grpIdx++) {
        UNPACK_GRP_FIELDS(grpIdx)

        for (subIdx = 0; subIdx < g_curLevelData.grp[grpIdx].count; subIdx++) {
            UNPACK_SUB_FIELDS(grpIdx, subIdx)

            if (g_curLevelData.grp[grpIdx].sub[subIdx].type == 1)
                has1 = 1;
            if (g_curLevelData.grp[grpIdx].sub[subIdx].type == 2)
                has2 = 1;
            if (g_curLevelData.grp[grpIdx].sub[subIdx].type == 3)
                has3 = 1;
            if (g_curLevelData.grp[grpIdx].sub[subIdx].type == 4)
                has4 = 1;
            if (g_curLevelData.grp[grpIdx].sub[subIdx].type == 5)
                has5 = 1;
            if (g_curLevelData.grp[grpIdx].sub[subIdx].type == 6)
                has6 = 1;
        }
    }

    for (cntIdx = 0; cntIdx < g_curLevelData.count; cntIdx++)
        UNPACK_OBJCOUNT(cntIdx)

    COPY_LEVEL_NAMES()

    for (objGrpIdx = 0; objGrpIdx < g_curLevelData.count; objGrpIdx++) {
        for (objIdx = 0; objIdx < g_curLevelData.objCount[objGrpIdx]; objIdx++) {
            UNPACK_OBJ_PATH(objGrpIdx, objIdx)
        }
    }

    for (auxIdx = 0; auxIdx < MAX_AUX; auxIdx++) {
        UNPACK_AUX_FIELDS(auxIdx)
    }

    // ---- derive filenames ----
    nGfx = 0;
    for (j = 0; j < NUM_HAZARD_GFX; j++) {
        g_alienGfxSlots[slot].name1[j][0] = 0;
        g_alienGfxSlots[slot].name2[j][0] = 0;
        g_alienGfxSlots[slot].name3[j][0] = 0;

        // Drop the raw gfx path for hazard types the level doesn't actually use (see NOTE
        // above: with the header loop always setting all six flags, this never triggers).
        if (j == 0 && !has1)
            g_levelRaw.gfx[j][0] = 0;
        if (j == 1 && !has2)
            g_levelRaw.gfx[j][0] = 0;
        if (j == 2 && !has3)
            g_levelRaw.gfx[j][0] = 0;
        if (j == 3 && !has4)
            g_levelRaw.gfx[j][0] = 0;
        if (j == 4 && !has5)
            g_levelRaw.gfx[j][0] = 0;
        if (j == 5 && !has6)
            g_levelRaw.gfx[j][0] = 0;

        // Scan the raw path (e.g. "gfx\hazards\foo.tga") to find the start of the base
        // filename after the last backslash (`start`) and the position of the extension's
        // dot (`dot`); `end` means the path is empty.
        i = 1;
        start = 1;
        stop = 1;
        dot = 0;
        found = 0;
        end = 0;
        do {
            if (g_levelRaw.gfx[j][0] == 0)
                end = 1;
            if (g_levelRaw.gfx[j][i] == '\\')
                start = i + 1;
            if (g_levelRaw.gfx[j][i] == '.') {
                dot = i;
                stop = i + 4;
                found = 1;
            }
            i++;
        } while (!found && !end);

        n1 = 0;
        n2 = 0;
        n3 = 0;
        if (!end) {
            // Rebuild three names from the base filename (without its original extension):
            // gfx[j] (unchanged, extension replaced with .tga below), mask[j] (same name with
            // "_mask" inserted before .tga), and filename (base name with no extension at all,
            // used as the hitmask/.hma lookup key).

            for (i = start; i < stop - 3; i++) {
                if (i < dot) {
                    filename[n3] = g_levelRaw.gfx[j][i];
                    n3++;
                    filename[n3] = 0;
                }
                g_curLevelData.gfx[j][n1] = g_levelRaw.gfx[j][i];
                g_curLevelData.mask[j][n2] = g_levelRaw.gfx[j][i];
                if (i == dot) {
                    g_curLevelData.mask[j][n2] = '_';
                    n2++;
                    g_curLevelData.mask[j][n2] = 'm';
                    n2++;
                    g_curLevelData.mask[j][n2] = 'a';
                    n2++;
                    g_curLevelData.mask[j][n2] = 's';
                    n2++;
                    g_curLevelData.mask[j][n2] = 'k';
                    n2++;
                    g_curLevelData.mask[j][n2] = '.';
                }
                n1++;
                n2++;
            }

            g_curLevelData.gfx[j][n1] = 't';
            n1++;
            g_curLevelData.gfx[j][n1] = 'g';
            n1++;
            g_curLevelData.gfx[j][n1] = 'a';
            n1++;
            g_curLevelData.mask[j][n2] = 't';
            n2++;
            g_curLevelData.mask[j][n2] = 'g';
            n2++;
            g_curLevelData.mask[j][n2] = 'a';
            n2++;
        }

        g_curLevelData.gfx[j][n1] = 0;
        g_curLevelData.mask[j][n2] = 0;
        if (g_curLevelData.gfx[j][0] != 0) {
            nGfx++;
            CopyBytesAt(g_alienGfxSlots[slot].name1[j], g_curLevelData.gfx[j], 0,
                StrLen(g_curLevelData.gfx[j]));
            CopyBytesAt(g_alienGfxSlots[slot].name2[j], g_curLevelData.mask[j], 0,
                StrLen(g_curLevelData.mask[j]));
            CopyBytesAt(g_alienGfxSlots[slot].name3[j], filename, 0, StrLen(filename));
            g_alienGfxSlots[slot].loaded[j] = 0;
            g_alienGfxSlots[slot].key[j] = StrHash(g_curLevelData.gfx[j]);
        }

        g_alienGfxSlots[slot].unusedI51c[j] = -1;
        g_alienGfxSlots[slot].unusedI534[j] = -1;
        g_curLevelData.w[j] = g_levelRaw.w[j];
        g_curLevelData.h[j] = g_levelRaw.h[j];
        g_alienGfxSlots[slot].frameW[j] = g_curLevelData.w[j];
        g_alienGfxSlots[slot].frameH[j] = g_curLevelData.h[j];
    }

    // ---- preload channels ----
    // Compress the raw level image into a worst-case buffer, then shrink-copy it into a
    // tightly-sized buffer for the slot to hold onto (this is what LoadLevelData() later
    // decompresses back out).
    buf = calloc(LEVEL_RAW_SIZE, 1);
    usize = LEVEL_RAW_SIZE;
    csize = LEVEL_RAW_SIZE;
    {
        uLongf packed = (uLongf)csize;   // zlib's sizes are `unsigned long`
        res = compress((unsigned char *)buf, &packed, (const unsigned char *)&g_levelRaw, usize);
        csize = (int)packed;
    }
    buf2 = calloc(csize, 1);
    memcpy(buf2, buf, csize);
    g_alienGfxSlots[slot].src = buf2;
    g_alienGfxSlots[slot].srcLen = csize;
    free(buf);
    g_alienGfxSlots[slot].owner = level;
    g_alienGfxSlots[slot].id = mode;
    g_alienGfxSlots[slot].frameCount = nGfx;

    // Preload each channel's alien graphics up to the buffering cap, reusing another slot's
    // already-loaded gfx/hma when one shares the same graphics key instead of reloading them.
    for (s = 0; s < NUM_HAZARD_GFX; s++) {
        if (g_alienGfxBufferedCount < g_maxBuffered && g_alienGfxSlots[slot].name1[s][0] != 0) {
            if (FindOtherSlotWithKey(slot, s, g_alienGfxSlots[slot].key[s])) {
                g_alienGfxSlots[slot].gfx[s] = g_alienGfxSlots[g_foundSlot].gfx[g_foundChan];
                g_alienGfxSlots[slot].gfx2[s] = g_alienGfxSlots[g_foundSlot].gfx2[g_foundChan];
                g_alienGfxSlots[slot].hma[s] = g_alienGfxSlots[g_foundSlot].hma[g_foundChan];
                g_alienGfxSlots[slot].blk[s] = g_alienGfxSlots[g_foundSlot].blk[g_foundChan];
                g_alienGfxSlots[slot].loaded[s] = 0;
                g_alienGfxSlots[slot].count[s] = 0;
                g_alienGfxSlots[g_foundSlot].count[g_foundChan]++;

            } else {
                g_lastLevelBuffered = level;
                g_alienGfxSlots[slot].gfx[s] = LoadGraphic2(g_alienGfxSlots[slot].name1[s], 1, 1);
                g_gfxLoaded++;
                g_alienGfxSlots[slot].gfx2[s] = LoadGraphic2(g_alienGfxSlots[slot].name2[s], 1, 1);
                g_gfx2Loaded++;
                g_alienGfxSlots[slot].hma[s] = LoadHma(g_alienGfxSlots[slot].name3[s],
                    (int)ImgWidth(g_alienGfxSlots[slot].gfx[s]),
                        (int)ImgHeight(g_alienGfxSlots[slot].gfx[s]));
                g_hmaLoaded++;
                if (g_alienGfxSlots[slot].hma[s] != 0)
                    ScanFrameRects(g_alienGfxSlots[slot].hma[s],
                        (int)ImgWidth(g_alienGfxSlots[slot].gfx[s]),
                        (int)ImgHeight(g_alienGfxSlots[slot].gfx[s]));

                g_alienGfxSlots[slot].blk[s].aLeft = g_frameAX1;
                g_alienGfxSlots[slot].blk[s].aTop = g_frameAY1;
                g_alienGfxSlots[slot].blk[s].aWidth = g_frameAX2;
                g_alienGfxSlots[slot].blk[s].aHeight = g_frameAY2;
                g_alienGfxSlots[slot].blk[s].bLeft = g_frameBX1;
                g_alienGfxSlots[slot].blk[s].bTop = g_frameBY1;
                g_alienGfxSlots[slot].blk[s].bWidth = g_frameBX2;
                g_alienGfxSlots[slot].blk[s].bHeight = g_frameBY2;
                g_alienGfxSlots[slot].loaded[s] = 1;
                g_alienGfxSlots[slot].count[s] = 1;
                g_alienGfxBufferedCount++;
            }
        }
    }
    return csize;
}
#undef CLEAR_LEVEL_SLOTS
#undef UNPACK_HDR_FIELDS
#undef UNPACK_GRP_FIELDS
#undef UNPACK_SUB_FIELDS
#undef UNPACK_OBJCOUNT
#undef COPY_LEVEL_NAMES
#undef UNPACK_OBJ_PATH
#undef UNPACK_AUX_FIELDS

// Draws one frame of the "loading data" progress screen shown while BufferAllLevels() packs
// every level; `n` is the number of levels packed so far.
void LoadingScreen(int n)
{
    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1.0f);
    DrawMenuText("WARBLADE SR VERSION 2.0", POS_CENTERED, 0x10e, 2);
    DrawMenuText("L O A D I N G   D A T A", POS_CENTERED, 0x136, 2);
    sprintf(g_logBuf, "FILLING LEVEL BUFFER : %d", n);
    DrawMenuText(g_logBuf, POS_CENTERED, 0x168, 2);
    sprintf(g_logBuf, "ALIEN GFX BUFFERED : %d", g_alienGfxBufferedCount);
    DrawMenuText(g_logBuf, POS_CENTERED, 0x17c, 2);
    sprintf(g_logBuf, "LEVELS COMPLETELY BUFFERED : %d", g_lastLevelBuffered);
    DrawMenuText(g_logBuf, POS_CENTERED, 400, 2);
    FlushBlit(0);
    FlushQuads(0);
    FlushStretchF();
    FlushStretchRot();
    FlushStretchI();
    FlushStretchRot2();
    FlushBlit2(0);
    FlipBuffer(0);
}

// Packs and preloads every classic, time-trial and malfunction level into the sound-slot
// buffer (up to 200 slots). The loading screen is redrawn at most every 16 ms (a frame at the
// 60 fps cap) and once at the end, so the frame cap doesn't hold up each level. Called once at
// startup. Stops early if the window is closed.
void BufferAllLevels()
{
    int slot = 0;
    int total = 0;
    unsigned lastDraw = SysMillis();
    int pass;
    int n;
    g_alienGfxBufferedCount = 0;
    g_lastLevelBuffered = 0;
    for (pass = 0; pass < 3; pass++) {
        int count = pass == 0 ? 100 : pass == 1 ? g_numLevels2 : g_numMalfunction;
        for (n = 0; n < count && slot < MAX_ALIEN_GFX_SLOTS && !SysQuitRequested(); n++) {
            total = PackLevelData(slot, n + 1, pass) + total;
            slot++;
            if (SysMillis() - lastDraw >= 16) {
                LoadingScreen(slot);
                lastDraw = SysMillis();
            }
        }
    }
    LoadingScreen(slot);
}

// Frees every packed level's compressed source buffer and every loaded channel's graphics/
// hma across all 200 sound slots. Called on shutdown/full level-buffer reset.
void FreeLevelBufs()
{
    for (int i = 0; i < MAX_ALIEN_GFX_SLOTS; i++) {
        if (g_alienGfxSlots[i].src != 0) {
            free(g_alienGfxSlots[i].src);
            g_alienGfxSlots[i].src = 0;
        }
        for (int j = 0; j < NUM_HAZARD_GFX; j++) {
            if (g_alienGfxSlots[i].loaded[j]) {
                if (g_alienGfxSlots[i].gfx[j] != 0) {
                    if (g_alienGfxCache[0].gfx1 == g_alienGfxSlots[i].gfx[j]) g_alienGfxCache[0].gfx1 = 0;
                    if (g_alienGfxCache[1].gfx1 == g_alienGfxSlots[i].gfx[j]) g_alienGfxCache[1].gfx1 = 0;
                    if (g_alienGfxCache[2].gfx1 == g_alienGfxSlots[i].gfx[j]) g_alienGfxCache[2].gfx1 = 0;
                    if (g_alienGfxCache[3].gfx1 == g_alienGfxSlots[i].gfx[j]) g_alienGfxCache[3].gfx1 = 0;
                    if (g_alienGfxCache[4].gfx1 == g_alienGfxSlots[i].gfx[j]) g_alienGfxCache[4].gfx1 = 0;
                    if (g_alienGfxCache[5].gfx1 == g_alienGfxSlots[i].gfx[j]) g_alienGfxCache[5].gfx1 = 0;
                    ImgFreePicture(g_alienGfxSlots[i].gfx[j]);
                    ImgFree(g_alienGfxSlots[i].gfx[j]);
                    g_alienGfxSlots[i].gfx[j] = 0;
                    g_freedA++;
                }

                if (g_alienGfxSlots[i].gfx2[j] != 0) {
                    if (g_alienGfxCache[0].gfx2 == g_alienGfxSlots[i].gfx2[j]) g_alienGfxCache[0].gfx2 = 0;
                    if (g_alienGfxCache[1].gfx2 == g_alienGfxSlots[i].gfx2[j]) g_alienGfxCache[1].gfx2 = 0;
                    if (g_alienGfxCache[2].gfx2 == g_alienGfxSlots[i].gfx2[j]) g_alienGfxCache[2].gfx2 = 0;
                    if (g_alienGfxCache[3].gfx2 == g_alienGfxSlots[i].gfx2[j]) g_alienGfxCache[3].gfx2 = 0;
                    if (g_alienGfxCache[4].gfx2 == g_alienGfxSlots[i].gfx2[j]) g_alienGfxCache[4].gfx2 = 0;
                    if (g_alienGfxCache[5].gfx2 == g_alienGfxSlots[i].gfx2[j]) g_alienGfxCache[5].gfx2 = 0;
                    ImgFreePicture(g_alienGfxSlots[i].gfx2[j]);
                    ImgFree(g_alienGfxSlots[i].gfx2[j]);
                    g_alienGfxSlots[i].gfx2[j] = 0;
                    g_freedB++;
                }

                if (g_alienGfxSlots[i].hma[j] != 0) {
                    if (g_alienGfxMem[0] == g_alienGfxSlots[i].hma[j]) g_alienGfxMem[0] = 0;
                    if (g_alienGfxMem[1] == g_alienGfxSlots[i].hma[j]) g_alienGfxMem[1] = 0;
                    if (g_alienGfxMem[2] == g_alienGfxSlots[i].hma[j]) g_alienGfxMem[2] = 0;
                    if (g_alienGfxMem[3] == g_alienGfxSlots[i].hma[j]) g_alienGfxMem[3] = 0;
                    if (g_alienGfxMem[4] == g_alienGfxSlots[i].hma[j]) g_alienGfxMem[4] = 0;
                    if (g_alienGfxMem[5] == g_alienGfxSlots[i].hma[j]) g_alienGfxMem[5] = 0;
                    free(g_alienGfxSlots[i].hma[j]);
                    g_alienGfxSlots[i].hma[j] = 0;
                    g_freedC++;
                }
                g_alienGfxSlots[i].loaded[j] = 0;
            }
        }
    }
}

// Counts how many "malfunction_NN.lvd" level files (00-49) exist on disk and stores the
// count in g_numMalfunction. Called once at startup before buffering levels.
void CountMalfunctionLevels()
{
    int unused = 0;  // NOTE: written but never read; kept from the original.
    char levelname[255];
    int count;
    g_numMalfunction = 0;
    count = 0;
    for (int i = 0; i < 50; i++) {
        sprintf(levelname, "malfunction_%02d.lvd", i);
        if (PacExists(levelname))
            count++;
    }
    g_numMalfunction = count;
}

// Loads the attack pattern files `att001.swd`..`att050.swd` that exist into g_patterns[],
// zero-filling the entries past each pattern's stored `count` (up to 150 per pattern).
void LoadPatterns()
{
    char patternname[1024];
    int n;
    int done;
    int count;
    int i;

    n = 1;
    done = 0;
    g_patternCount = 0;
    while (!done) {
        sprintf(patternname, "att%03d.swd", n);
        if (PacRead(patternname, &g_pattern, 0xbe0)) { // sizeof(g_pattern)
            count = g_pattern.count;

            // zero-fill the unused entries past count
            for (i = 0; i < MAX_PATTERN_ENTRIES; i++) {
                if (i >= count) {
                    g_pattern.entries[i].type = 0;
                    g_pattern.entries[i].uParam = 0;
                    g_pattern.entries[i].tParam = 0;
                    g_pattern.entries[i].x = 0;
                    g_pattern.entries[i].y = 0;
                }
            }

            memcpy(&g_patterns[g_patternCount], &g_pattern, 0xbe0);
            g_patternCount++;
        }
        n++;
        if (n > 50)
            done = 1;
    }
}

// Loads classic-mode level `n` from `classic_level_%03d.lvd` into g_levelRaw. Returns 1 on
// success, 0 if the file couldn't be opened.
int LoadClassicLevel(int n)
{
    char levelname[1024];

    sprintf(levelname, "classic_level_%03d.lvd", n);
    if (PacRead(levelname, &g_levelRaw, LEVEL_RAW_SIZE)) {
        return 1;
    } else {
        return 0;
    }
}

// Loads time-trial level `n` from `timetrial_%02d.lvd` into g_levelRaw. Returns 1 on
// success, 0 if the file couldn't be opened.
int LoadTimeTrialLevel(int n)
{
    char levelname[512];

    sprintf(levelname, "timetrial_%02d.lvd", n);
    if (PacRead(levelname, &g_levelRaw, LEVEL_RAW_SIZE)) {
        return 1;
    } else {
        return 0;
    }
}

// Loads malfunction-mode level `n` from `malfunction_%02d.lvd` into g_levelRaw. Returns 1 on
// success, 0 if the file couldn't be opened.
int LoadMalfunctionLevel(int n)
{
    char levelname[255];

    sprintf(levelname, "malfunction_%02d.lvd", n);
    if (PacRead(levelname, &g_levelRaw, LEVEL_RAW_SIZE)) {
        return 1;
    } else {
        return 0;
    }
}
