#pragma once
// Game struct types: one per object in memory. Offsets in comments are from the start
// of the struct.
//
// Padding: bytes the game never reads or writes by name are declared as
// `char pad_<off>[<size>]; // +0x<off>`, with <off> the hex offset. Some are alignment
// holes the compiler would add anyway; the rest are members the code never touches
// (or, in the save-file structs, reserved space). They keep offsets and file layouts fixed.
#include "sdlhelp.h"
#pragma warning(disable: 4201)   // anonymous structs in unions

// A check that holds only where pointers are 32-bit (the original's layout of a struct that
// holds a pointer).
#define C_ASSERT_32(e) C_ASSERT(sizeof(void *) != 4 || (e))

typedef void (*VoidFn)();

// C needs the struct tag; these let the game name the types without it.
typedef struct Name255 Name255;
typedef struct Settings Settings;
typedef struct Account Account;
typedef struct AccountV0 AccountV0;
typedef struct SettingsV2 SettingsV2;
typedef struct AccountV2 AccountV2;
typedef struct Beam Beam;
typedef struct Rect16 Rect16;
typedef struct BlitItem BlitItem;
typedef struct Bonus Bonus;
typedef struct BonusStats BonusStats;
typedef struct FallingHazard FallingHazard;
typedef struct Box Box;
typedef struct Card Card;
typedef struct FadeColors FadeColors;
typedef struct EndRect EndRect;
typedef struct FrameSet FrameSet;
typedef struct Enemy Enemy;
typedef struct Explosion Explosion;
typedef struct FPair FPair;
typedef struct Flash Flash;
typedef struct Flash34 Flash34;
typedef struct PatternPt PatternPt;
typedef struct Pattern Pattern;
typedef struct HiscoreEntry HiscoreEntry;
typedef struct HiscoreData HiscoreData;
typedef struct IntPair IntPair;
typedef struct LvSub LvSub;
typedef struct LvGrp LvGrp;
typedef struct LvHdr LvHdr;
typedef struct LvObj LvObj;
typedef struct Level Level;
typedef struct LevelObj LevelObj;
typedef struct LvRawSub LvRawSub;
typedef struct LvRawGrp LvRawGrp;
typedef struct LvRawHdr LvRawHdr;
typedef struct LvRawObj LvRawObj;
typedef struct LvRawQ LvRawQ;
typedef struct LevelRaw LevelRaw;
typedef struct LevelRec LevelRec;
typedef struct LinkArea LinkArea;
typedef struct MapObj MapObj;
typedef struct MenuEntry MenuEntry;
typedef struct ScorePopup ScorePopup;
typedef struct EnemySet EnemySet;
typedef struct Particle Particle;
typedef struct FreeParticle FreeParticle;
typedef struct Player Player;
typedef struct QuadItem QuadItem;
typedef struct Ring Ring;
typedef struct SaveData SaveData;
typedef struct ShipDef ShipDef;
typedef struct BurstSpark BurstSpark;
typedef struct PulseFx PulseFx;
typedef struct AlienGfxSlot AlienGfxSlot;
typedef struct SoundQueueEntry SoundQueueEntry;
typedef struct Spark Spark;
typedef struct FallingSprite FallingSprite;
typedef struct ScoopTrail ScoopTrail;
typedef struct HyperspaceStar HyperspaceStar;
typedef struct StretchItemF StretchItemF;
typedef struct StretchItemI StretchItemI;
typedef struct StretchItemRot StretchItemRot;
typedef struct QuadImageItem QuadImageItem;
typedef struct ButtonItem ButtonItem;
typedef struct ImageRectItem ImageRectItem;
typedef struct LinkItem LinkItem;
typedef struct TextItem TextItem;
typedef struct MenuItem MenuItem;
typedef struct ToggleItem ToggleItem;
typedef struct EditItem EditItem;
typedef struct Window Window;

struct Name255 {   // 0xff bytes; at 0xa914e1[20], 0xafc221[20]
    char s[255]; // +0x0
};

struct Settings {   // 0x638 bytes; at 0xa928d0[1], 0xd2eb30[1], 0xaf7848
    char title[30]; // +0x0
    char pad_1e[0x2]; // +0x1e
    int unused020; // +0x20
    int gamesPlayed; // +0x24
    int particlesOn; // +0x28
    unsigned char borderMode; // +0x2c
    char pad_2d[0x3]; // +0x2d
    int unused030; // +0x30
    unsigned char musicFormat; // +0x34
    char pad_35[0x3]; // +0x35
    int unused038; // +0x38: BASS hardware/software mixing (0x20/0x40), unused since SDL
    int unused03c; // +0x3c
    int musicVolume; // +0x40
    int sfxVol; // +0x44
    int fps; // +0x48
    float numStars; // +0x4c
    unsigned char unused050; // +0x50
    // SDL port additions in padding bytes, which original files leave 0 (0 = the default).
    unsigned char vsyncOff; // +0x51 1: vsync off; anything else: on
    unsigned char interpolation; // +0x52 SYS_INTERP_* (sdlhelp.h); out of range: auto
    char pad_53; // +0x53
    int unused054; // +0x54 (was the 16/32-bit colour depth)
    int sfxOn; // +0x58
    int unused05c; // +0x5c
    int collisionDetail; // +0x60
    int bgStars; // +0x64
    int unused068; // +0x68
    unsigned char bulletIntensity; // +0x6c
    char pad_6d[0x3]; // +0x6d
    union {
        __int64 best; // +0x70
        __int64 playTime; // +0x70
    };
    union {
        struct {
            int device0; // +0x78
            int device1; // +0x7c
            int device2; // +0x80
            int device3; // +0x84
        };
        int device[4]; // +0x78
    };
    union {
        struct {
            int fire[4]; // +0x88
            int left[4]; // +0x98
            int right[4]; // +0xa8
            int up[4]; // +0xb8
            int down[4]; // +0xc8
            int rocket[4]; // +0xd8
            int key6[4]; // +0xe8
        };
        int playerKeys[7][4]; // +0x88: rows fire ... key6
    };
    int difficulty; // +0xf8
    int bgEnabled; // +0xfc
    int bgTint; // +0x100
    int unused104; // +0x104: BASS output rate (22050/44100), unused since SDL
    float sparks; // +0x108
    int profileSel; // +0x10c
    union {
        struct {
            int joyFireAlt0; // +0x110
            int joyFireAlt1; // +0x114
            int joyFireAlt2; // +0x118
            int joyFireAlt3; // +0x11c
            int joyRocketAlt0; // +0x120
            int joyRocketAlt1; // +0x124
            int joyRocketAlt2; // +0x128
            int joyRocketAlt3; // +0x12c
            int joyBtn2Alt0; // +0x130
            int joyBtn2Alt1; // +0x134
            int joyBtn2Alt2; // +0x138
            int joyBtn2Alt3; // +0x13c
        };
        struct {
            int joyFire[4]; // +0x110
            int joyRocket[4]; // +0x120
            int joyBtn2[4]; // +0x130
        };
        int joy[3][4]; // +0x110: rows joyFire, joyRocket, joyBtn2
    };
    int voice; // +0x140
    int musicVol; // +0x144
    int version; // +0x148
    float unusedF14c[5]; // +0x14c
    int newsIds[4]; // +0x160 (unused: the online code is gone)
    int newsCounts[4]; // +0x170 (unused: the online code is gone)
    int alienBuffer; // +0x180
    char serialCode[0x32]; // +0x184 (unused: the online code is gone)
    char unusedStr1b6[63]; // +0x1b6
    char unusedStr1f5[4][255]; // +0x1f5
    unsigned char checkVersion; // +0x5f1 (unused: the online code is gone)
    unsigned char renderer; // +0x5f2 RendererChoice (PTK's DirectX/OpenGL byte: 1 DirectX, 0 OpenGL)
    union {
        unsigned char shuffleByte; // +0x5f3
        bool shuffle; // +0x5f3
    };
    union {
        unsigned char windowedByte; // +0x5f4
        bool windowed; // +0x5f4
    };
    char pad_5f5[0x1]; // +0x5f5
    short netMode; // +0x5f6 (unused: the online code is gone)
    union {
        struct {
            int pause[4]; // +0x5f8
            int profile[4]; // +0x608
            union {
                struct {
                    int joyPauseAlt0; // +0x618
                    int joyPauseAlt1; // +0x61c
                    int joyPauseAlt2; // +0x620
                    int joyPauseAlt3; // +0x624
                    int joyProfileAlt0; // +0x628
                    int joyProfileAlt1; // +0x62c
                    int joyProfileAlt2; // +0x630
                    int joyProfileAlt3; // +0x634
                };
                struct {
                    int joyPause[4]; // +0x618
                    int joyProfile[4]; // +0x628
                };
            };
        };
        int menuKeys[4][4]; // +0x5f8: rows pause, profile, joyPause, joyProfile
    };
};

struct Account {   // 0x40d0 bytes; at 0xa913c0, 0xd2d620
    char id[7]; // +0x0
    char name[30]; // +0x7
    char password[16]; // +0x25
    char pad_35[0x1]; // +0x35
    SysDate created; // +0x36
    char pad_46[0x2]; // +0x46
    __int64 bestLevelTime; // +0x48
    __int64 bestMeteorstormTime; // +0x50
    union {
        struct {
            int highScoreLo; // +0x58
            int highScoreHi; // +0x5c
        };
        struct {
            __int64 highScore; // +0x58
        };
    };
    union {
        struct {
            int meteorstormHighScoreLo; // +0x60
            int meteorstormHighScoreHi; // +0x64
        };
        struct {
            __int64 meteorstormHighScore; // +0x60
        };
    };
    union {
        struct {
            int timeTrialHighScoreLo; // +0x68
            int timeTrialHighScoreHi; // +0x6c
        };
        struct {
            __int64 timeTrialHighScore; // +0x68
        };
    };
    double highestMoney; // +0x70
    int highestRank; // +0x78
    int highestLevel; // +0x7c
    int totalLevelsPlayed; // +0x80
    int gamesPlayed; // +0x84
    union {
        struct {
            int playTimeLo; // +0x88
            int playTimeHi; // +0x8c
        };
        struct {
            __int64 playTimeRaw; // +0x88
        };
        struct {
            __int64 playTime; // +0x88
        };
    };
    union {
        char levelDoneBytes[50]; // +0x90
        bool levelDone[50]; // +0x90
    };
    char pad_c2[0x2]; // +0xc2
    int unusedC4; // +0xc4
    int shotsFired; // +0xc8
    int shotsHit; // +0xcc
    int secretsInOneGame; // +0xd0
    int bonusLevelsPlayed; // +0xd4
    int perfectBonusLevels; // +0xd8
    int unusedDc; // +0xdc
    int medals; // +0xe0
    int bestHitPctAbove25; // +0xe4
    int voiceIndex; // +0xe8
    int version; // +0xec
    int completionRank; // +0xf0
    int unusedF4; // +0xf4
    int unlockedRank; // +0xf8
    int medalStep; // +0xfc
    int lives; // +0x100
    int unused104; // +0x104
    union {
        struct {
            int level100HighScoreLo; // +0x108
            int level100HighScoreHi; // +0x10c
        };
        struct {
            __int64 level100HighScore; // +0x108
        };
    };
    float unused110; // +0x110
    float unused114; // +0x114
    float unused118; // +0x118
    union {
        unsigned char easyByte; // +0x11c
        bool easy; // +0x11c
    };
    unsigned char unused11d; // +0x11d
    unsigned char unused11e; // +0x11e
    unsigned char unused11f; // +0x11f
    unsigned char unused120; // +0x120
    Name255 names[20]; // +0x121
    char pad_150d[0x3]; // +0x150d
    Settings settings; // +0x1510
    short medalOrder[7]; // +0x1b48
    char pad_1b56[0x2000]; // +0x1b56
    union {
        unsigned char gameCompletedByte; // +0x3b56
        bool gameCompleted; // +0x3b56
    };
    char pad_3b57[0xd]; // +0x3b57
    int cheatDetected; // +0x3b64
    char pad_3b68[0x50]; // +0x3b68
    __int64 ownerStamp; // +0x3bb8
    char pad_3bc0[0x48]; // +0x3bc0
    __int64 saveIdHistory[50]; // +0x3c08
    char pad_3d98[0x320]; // +0x3d98
    __int64 lastSaveId; // +0x40b8
    char pad_40c0[0x10]; // +0x40c0
};

struct AccountV0 {   // 0x1b48-byte legacy score layout; at 0xd5a410 during DecodeAccount
    char pad_0[0x58]; // +0x0
    double highScore; // +0x58
    double meteorstormHighScore; // +0x60
    double timeTrialHighScore; // +0x68
    char pad_70[0x98]; // +0x70
    double level100HighScore; // +0x108
    char pad_110[0x1a38]; // +0x110
};

struct SettingsV2 {   // 0x630 bytes; at 0xafd610[1]; the old (0x3c00-byte file) layout of Settings
    char title[30];
    char pad_1e[0x2]; // +0x1e
    int unused020;
    int gamesPlayed;
    int particlesOn;
    char borderMode;
    char pad_2d[0x3]; // +0x2d
    int unused030;
    char musicFormat;
    char pad_35[0x3]; // +0x35
    int unused038;
    int unused03c;
    int musicVolume;
    int sfxVol;
    int fps;
    float numStars;
    char unused050;
    char pad_51[0x3]; // +0x51
    int unused054;
    int sfxOn;
    int unused05c;
    int collisionDetail;
    int bgStars;
    int unused068;
    char bulletIntensity;
    char pad_6d[0x3]; // +0x6d
    __int64 playTime;
    int device0;
    int device1;
    int device2;
    int device3;
    char pad_88[0x70]; // +0x88
    int difficulty;
    int bgEnabled;
    int bgTint;
    int unused104;
    float sparks;
    int profileSel;
    int joyFireAlt0;
    int joyFireAlt1;
    int joyFireAlt2;
    int joyFireAlt3;
    int joyRocketAlt0;
    int joyRocketAlt1;
    int joyRocketAlt2;
    int joyRocketAlt3;
    int joyBtn2Alt0;
    int joyBtn2Alt1;
    int joyBtn2Alt2;
    int joyBtn2Alt3;
    int voice;
    int musicVol;
    int version;
    char pad_14c[0x49f]; // +0x14c
    unsigned char checkVersion;
    unsigned char renderer;
    unsigned char shuffleByte;
    unsigned char windowedByte;
    char pad_5ef[0x21]; // +0x5ef
    int joyPauseAlt0;
    int joyPauseAlt1;
    int joyPauseAlt2;
    int joyPauseAlt3;
    int joyProfileAlt0;
    int joyProfileAlt1;
    int joyProfileAlt2;
    int joyProfileAlt3;
};

struct AccountV2 {   // 0x3c00 bytes; at 0xafc100
    char id[7]; // +0x0
    char name[30]; // +0x7
    char password[16]; // +0x25
    char pad_35[0x1]; // +0x35
    SysDate created; // +0x36
    char pad_46[0x2]; // +0x46
    __int64 bestLevelTime; // +0x48
    __int64 bestMeteorstormTime; // +0x50
    __int64 highScore; // +0x58
    __int64 meteorstormHighScore; // +0x60
    __int64 timeTrialHighScore; // +0x68
    double highestMoney; // +0x70
    int highestRank; // +0x78
    int highestLevel; // +0x7c
    int totalLevelsPlayed; // +0x80
    int gamesPlayed; // +0x84
    __int64 playTimeRaw; // +0x88
    char levelDoneBytes[50]; // +0x90
    char pad_c2[0x2]; // +0xc2
    int unusedC4; // +0xc4
    int shotsFired; // +0xc8
    int shotsHit; // +0xcc
    int secretsInOneGame; // +0xd0
    int bonusLevelsPlayed; // +0xd4
    int perfectBonusLevels; // +0xd8
    int unusedDc; // +0xdc
    int medals; // +0xe0
    int bestHitPctAbove25; // +0xe4
    int voiceIndex; // +0xe8
    int version; // +0xec
    int completionRank; // +0xf0
    int unusedF4; // +0xf4
    int unlockedRank; // +0xf8
    int medalStep; // +0xfc
    int lives; // +0x100
    int unused104; // +0x104
    __int64 level100HighScore; // +0x108
    float unused110; // +0x110
    float unused114; // +0x114
    float unused118; // +0x118
    unsigned char easyByte; // +0x11c
    unsigned char unused11d; // +0x11d
    unsigned char unused11e; // +0x11e
    unsigned char unused11f; // +0x11f
    unsigned char unused120; // +0x120
    Name255 names[20]; // +0x121
    char pad_150d[0x3]; // +0x150d
    SettingsV2 settings; // +0x1510
    short medalOrder[7]; // +0x1b40
    char pad_1b4e[0x2000]; // +0x1b4e
    unsigned char gameCompletedByte; // +0x3b4e
    char pad_3b4f[0xd]; // +0x3b4f
    int cheatDetected; // +0x3b5c
    char pad_3b60[0x50]; // +0x3b60
    __int64 ownerStamp; // +0x3bb0
    char pad_3bb8[0x48]; // +0x3bb8
};

struct Beam {   // 0x34 bytes; at 0x802c00[12]
    int active; // +0x0
    char pad_4[0x8]; // +0x4
    float xoff; // +0xc
    float xvel; // +0x10
    char pad_14[0x4]; // +0x14
    int r; // +0x18
    int g; // +0x1c
    int b; // +0x20
    float alpha; // +0x24
    float fade; // +0x28
    float yoff; // +0x2c
    float yvel; // +0x30
};

struct Rect16 {   // 0x10 bytes; at 0xf61a68[1], 0xf4e808[1], 0x848600[6], 0xd2d5c0[6], 0xab2468[50], 0x849c0c[1], 0x848738[1], 0xb04a8c[1], 0xaf7e94[1], 0xd5e348[1], 0x849b98[1], 0x847384, 0xb49d80[1], 0x8473cc[1]
    union {
        struct {
            int x1; // +0x0
            int y1; // +0x4
            int x2; // +0x8
            int y2; // +0xc
        };
        struct {
            int v[4]; // +0x0
        };
    };
};

struct BlitItem {   // 0x20 bytes; at 0xf61a68, 0xf4e808
    Rect16 src; // +0x0
    float destX; // +0x10
    float destY; // +0x14
    Image *graphic; // +0x18
    char pad_1c[0x4]; // +0x1c
};

struct Bonus {   // 0x64 bytes; at 0xb04a50[150]
    int active; // +0x0
    int alive; // +0x4
    int srcX; // +0x8
    int srcY; // +0xc
    int h; // +0x10
    int w; // +0x14
    int frameCount; // +0x18
    float frame; // +0x1c
    float frameDelay; // +0x20
    float frameTimer; // +0x24
    int type; // +0x28
    Image *gfx; // +0x2c
    void *hma; // +0x30
    int hmaW; // +0x34
    int hmaH; // +0x38
    union {
        struct {
            int left; // +0x3c
            int top; // +0x40
            int right; // +0x44
            int bottom; // +0x48
        };
        struct {
            Rect16 rect; // +0x3c
        };
    };
    float x; // +0x4c
    float y; // +0x50
    float vx; // +0x54
    float vy; // +0x58
    float timer; // +0x5c
    union {
        struct {
            unsigned char fastFall; // +0x60
        };
        struct {
            int unusedI60; // +0x60
        };
    };
};

struct BonusStats {   // 0x60 bytes; at 0xcde5c0
    int cashRemaining; // +0x0
    int maxCashFlag; // +0x4
    int rankRemaining; // +0x8
    int perfectsRemaining; // +0xc
    int hitPercent; // +0x10
    int hitPercentTarget; // +0x14
    __int64 total; // +0x18
    __int64 cash; // +0x20
    __int64 maxCashBonus; // +0x28
    __int64 perfectsBonus; // +0x30
    __int64 perfects; // +0x38
    __int64 rankBonus; // +0x40
    __int64 rank; // +0x48
    __int64 hitBonus; // +0x50
    __int64 unusedTickCount; // +0x58
};

struct FallingHazard {   // 0x58 bytes; at 0xb49d48[30]
    int active; // +0x0
    int sx; // +0x4
    int sy; // +0x8
    int h; // +0xc
    int w; // +0x10
    char pad_14[0x4]; // +0x14
    float animTimer; // +0x18
    char pad_1c[0x4]; // +0x1c
    int type; // +0x20
    int vol; // +0x24
    void *graphic; // +0x28
    void *hma; // +0x2c
    int hmaW; // +0x30
    int hmaH; // +0x34
    union {
        struct {
            int x1_38; // +0x38
            int y1_3c; // +0x3c
            int x2_40; // +0x40
            int y2_44; // +0x44
        };
        struct {
            Rect16 hitBox; // +0x38
        };
    };
    float x; // +0x48
    float y; // +0x4c
    float vx; // +0x50
    float vy; // +0x54
};

struct Box {   // 0x10 bytes; at 0x7d0318, 0x7d03b8
    int x0; // +0x0
    int y0; // +0x4
    int x1; // +0x8
    int y1; // +0xc
};

struct Card {   // 0x18 bytes; at 0x846d80
    int active; // +0x0
    int open; // +0x4
    int unusedF08; // +0x8
    int x; // +0xc
    int y; // +0x10
    int type; // +0x14
};

struct FadeColors {   // 0x78 bytes; at 0x8ff638, 0xcdb160, 0xe0c0d8
    float step0; // +0x0
    char pad_4[0x74]; // +0x4
};

struct EndRect {   // 0x10 bytes
    int x; // +0x0
    int y; // +0x4
    int w; // +0x8
    int h; // +0xc
};

struct FrameSet {   // 0x20 bytes; at 0x803bd0[6], 0x8040c0[6], 0x849bec[1]
    union {
        struct {
            int bLeft; // +0x0
            int bTop; // +0x4
            int bWidth; // +0x8
            int bHeight; // +0xc
            int aLeft; // +0x10
            int aTop; // +0x14
            int aWidth; // +0x18
            int aHeight; // +0x1c
        };
        struct {
            int raw[8]; // +0x0
        };
    };
};

struct Enemy {   // 0x3a8 bytes; at 0x849a48[600]
    int unused00; // +0x0
    int active; // +0x4
    float x; // +0x8
    float y; // +0xc
    float homeX; // +0x10
    float homeY; // +0x14
    float velX; // +0x18
    float velY; // +0x1c
    float accelX; // +0x20
    float accelY; // +0x24
    float speedScale; // +0x28
    float patternTimer; // +0x2c
    float hoverX; // +0x30
    float hoverY; // +0x34
    float speedX; // +0x38
    float speedY; // +0x3c
    float offsetX; // +0x40
    float offsetY; // +0x44
    int oscillateMul; // +0x48
    float oscillateAccel; // +0x4c
    float oscillatePhase; // +0x50
    int ownerPlayer; // +0x54
    int beamOffsetX; // +0x58
    int beamSide; // +0x5c
    int homing; // +0x60
    int settled; // +0x64
    int altFireActive; // +0x68
    int pairedEnemyIdx; // +0x6c
    char pad_70[0x4]; // +0x70
    int savedFireDelay; // +0x74
    float savedHp; // +0x78
    float unusedF7c; // +0x7c
    float unusedF80; // +0x80
    float debrisVelX; // +0x84
    float debrisVelY; // +0x88
    float attackStaggerTimer; // +0x8c
    float turnTimer; // +0x90
    char pad_94[0x8]; // +0x94
    float dirStepTimer; // +0x9c
    char pad_a0[0xc]; // +0xa0
    int type; // +0xac
    int patternId; // +0xb0
    char pad_b4[0x8]; // +0xb4
    int hazardType; // +0xbc
    int srcX; // +0xc0
    int srcY; // +0xc4
    int facing; // +0xc8
    int turnState; // +0xcc
    int zigzagTimer; // +0xd0
    int unusedAlpha; // +0xd4
    float zigzagVelX; // +0xd8
    float zigzagAccel; // +0xdc
    float descendVelY; // +0xe0
    float descendAccelY; // +0xe4
    float descendVelX; // +0xe8
    float descendAccelX; // +0xec
    float turnTimer2; // +0xf0
    float turnTimer2Max; // +0xf4
    float animFrame; // +0xf8
    float animDelay; // +0xfc
    float animTimer; // +0x100
    int animReverse; // +0x104
    int useDirRemap; // +0x108
    int altFrameCounter; // +0x10c
    char pad_110[0x24]; // +0x110
    Image *gfxA; // +0x134
    Image *gfxB; // +0x138
    Image *hitFlashGfxA; // +0x13c
    Image *hitFlashGfxB; // +0x140
    void *shotFrame; // +0x144
    int shotGfxW; // +0x148
    int shotGfxH; // +0x14c
    Rect16 srcRect; // +0x150
    int hitFlashTimer; // +0x160
    float hp; // +0x164
    float maxHp; // +0x168
    char pad_16c[0x4]; // +0x16c
    __int64 score; // +0x170
    char pad_178[0x4]; // +0x178
    int attackDelay; // +0x17c
    int attackDelayStep; // +0x180
    int fireDelay; // +0x184
    int fireDelayStep; // +0x188
    char pad_18c[0x4]; // +0x18c
    int patternEndAction; // +0x190
    char pad_194[0x10]; // +0x194
    FrameSet frameSet; // +0x1a4
    union {
        struct {
            int unused1c4; // +0x1c4
            int unused1c8; // +0x1c8
            int unused1cc; // +0x1cc
            int unused1d0; // +0x1d0
        };
        struct {
            Rect16 hitRect; // +0x1c4
        };
    };
    char pad_1d4[0x28]; // +0x1d4
    int groupIndex; // +0x1fc
    int bonusGroupIndex; // +0x200
    int patternStep; // +0x204
    float animSpeedDivisor; // +0x208
    int unusedSpawnDelay; // +0x20c
    float animStepTime; // +0x210
    int animFrameCount; // +0x214
    int animPingPong; // +0x218
    int forcedDir; // +0x21c
    int bossGunASlot[10]; // +0x220
    int bossGunBSlot[10]; // +0x248
    int bossGunCSlot[10]; // +0x270
    float bossGunAX[10]; // +0x298
    float bossGunAY[10]; // +0x2c0
    float bossGunBX[10]; // +0x2e8
    float bossGunBY[10]; // +0x310
    float bossGunCX[10]; // +0x338
    float bossGunCY[10]; // +0x360
    int deathExplosionGfx; // +0x388
    int deathExplosionLife; // +0x38c
    int deathExplosionR; // +0x390
    int deathExplosionG; // +0x394
    int deathExplosionB; // +0x398
    unsigned char locked; // +0x39c
    unsigned char fixedFireDelay; // +0x39d
    unsigned char flashActive; // +0x39e
    char pad_39f[0x1]; // +0x39f
    float flashTimer; // +0x3a0
    char pad_3a4[0x4]; // +0x3a4
};

struct Explosion {   // 0x4c bytes; at 0x847720[50]
    int active; // +0x0
    float x; // +0x4
    float y; // +0x8
    int type; // +0xc
    int frame; // +0x10
    float delay; // +0x14
    float timer; // +0x18
    int spin; // +0x1c
    int writeOnlyGfxIndex; // +0x20
    float writeOnlyCenterX; // +0x24
    float writeOnlyCenterY; // +0x28
    int writeOnlyColorR; // +0x2c
    int writeOnlyColorG; // +0x30
    int writeOnlyColorB; // +0x34
    int alpha; // +0x38
    float scale; // +0x3c
    float scaleMul; // +0x40
    float angle; // +0x44
    float angleVel; // +0x48
};

struct FPair {   // 0x8 bytes; at 0x7d01e0
    float lo; // +0x0
    float hi; // +0x4
};

struct Flash {   // 0x28 bytes; at 0x803640[10]
    int active; // +0x0
    int x; // +0x4
    int y; // +0x8
    int size; // +0xc
    Image *graphic; // +0x10
    int r; // +0x14
    int g; // +0x18
    int b; // +0x1c
    float alpha; // +0x20
    float fade; // +0x24
};

struct Flash34 {   // 0x34 bytes; at 0xd58d88[49]
    int active; // +0x0
    float x; // +0x4
    float y; // +0x8
    float vx; // +0xc
    float vy; // +0x10
    float size; // +0x14
    int angle; // +0x18
    int r; // +0x1c
    int g; // +0x20
    int b; // +0x24
    float alpha; // +0x28
    float fade; // +0x2c
    float life; // +0x30
};

struct PatternPt {   // 0x14 bytes; at 0x7f048c[150], 0xad00d4[151]
    int x; // +0x0
    int y; // +0x4
    int type; // +0x8
    int uParam; // +0xc
    int tParam; // +0x10
};

struct Pattern {   // 0xbe0 bytes; at 0x7f0468, 0xad00b0
    int unused0; // +0x0
    int unused1; // +0x4
    int unused2; // +0x8
    int unused3; // +0xc
    int unused4; // +0x10
    int unused5; // +0x14
    int unused6; // +0x18
    int unused7; // +0x1c
    int unused8; // +0x20
    PatternPt entries[150]; // +0x24
    int count; // +0xbdc
};

struct HiscoreEntry {   // 0x68 bytes; at 0xc39388[100], 0xb4d890[20], 0xb4afe8[100]
    char name[0x20]; // +0x0
    int power; // +0x20
    int unused24; // +0x24
    __int64 score; // +0x28
    int rank; // +0x30
    int level; // +0x34
    int highlight; // +0x38
    union {
        struct {
            unsigned short year; // +0x3c
            unsigned short month; // +0x3e
            unsigned short dow; // +0x40
            unsigned short day; // +0x42
            unsigned short hour; // +0x44
            unsigned short minute; // +0x46
            unsigned short second; // +0x48
            unsigned short milliseconds; // +0x4a
        };
        SysDate date; // +0x3c
    };
    char pad_4c[0x4]; // +0x4c
    __int64 duration; // +0x50
    int shots; // +0x58
    int hits; // +0x5c
    __int64 ownerStamp; // +0x60
};

#pragma pack(push, 4)   // size 0x30d4 is not a multiple of 8: the original was 4-aligned
struct HiscoreData {   // 0x30d4 bytes; at 0xc39380, 0xb4afe0
    char tag[5]; // +0x0
    char pad_5[0x3]; // +0x5
    union {
        HiscoreEntry table[5][20]; // +0x8
        struct {
            HiscoreEntry table0[20]; // +0x8
            HiscoreEntry table1[20]; // +0x828
            HiscoreEntry table2[20]; // +0x1048
            HiscoreEntry table3[20]; // +0x1868
            HiscoreEntry table4[20]; // +0x2088
        };
    };
    char pad_28a8[0x8]; // +0x28a8
    HiscoreEntry table5[20]; // +0x28b0
    int count; // +0x30d0
};
#pragma pack(pop)

struct IntPair {   // 0x8 bytes; at 0xe1108c[6]
    Image *gfx1; // +0x0
    Image *gfx2; // +0x4
};

struct LvSub {   // 0x20 bytes; at 0xa95cb0[50]
    int xOffset; // +0x0
    int yOffset; // +0x4
    int type; // +0x8
    int hp; // +0xc
    int fireRateMin; // +0x10
    int fireRateMax; // +0x14
    int fireDelay; // +0x18
    int pathId; // +0x1c
};

struct LvGrp {   // 0x664 bytes; at 0xa95c8c[25]
    int spawnX; // +0x0
    int spawnY; // +0x4
    int spawnDelay; // +0x8
    int spawnStep; // +0xc
    int count; // +0x10
    float velX; // +0x14
    float velY; // +0x18
    int groupId; // +0x1c
    int kind; // +0x20
    LvSub sub[50]; // +0x24
};

struct LvHdr {   // 0x14 bytes; at 0xa95c28[5]
    int count; // +0x0
    int type; // +0x4
    int hp; // +0x8
    int fireRateMin; // +0xc
    int fireRateMax; // +0x10
};

struct LvObj {   // 0x14 bytes; at 0xa9fd0c[3750]
    float pathX; // +0x0
    float pathY; // +0x4
    int cmd; // +0x8
    int unusedD; // +0xc
    int holdTime; // +0x10
};

struct Level {   // 0x1cb98 bytes; at 0xa95c20
    int type; // +0x0
    int count; // +0x4
    LvHdr hdr[5]; // +0x8
    LvGrp grp[25]; // +0x6c
    int objCount[25]; // +0xa030
    char name1[36]; // +0xa094
    char name2[52]; // +0xa0b8
    LvObj obj[25][150]; // +0xa0ec
    char gfx[6][51]; // +0x1c5e4
    char mask[6][51]; // +0x1c716
    Rect16 aux[50]; // +0x1c848
    int w[6]; // +0x1cb68
    int h[6]; // +0x1cb80
};

struct LevelObj {   // 0x8c bytes; at 0xaf7e80[100]
    Image *gfxA; // +0x0
    Image *gfxB; // +0x4
    void *hma; // +0x8
    int hmaW; // +0xc
    int hmaH; // +0x10
    Rect16 rect; // +0x14
    int active; // +0x24
    float f28; // +0x28
    int blink; // +0x2c
    int flip; // +0x30
    float speed; // +0x34
    int frame; // +0x38
    int player; // +0x3c
    float fuse; // +0x40
    int row; // +0x44
    float frameDelay; // +0x48
    float frameTimer; // +0x4c
    float turnDelay; // +0x50
    float turnTimer; // +0x54
    float soundDelay; // +0x58
    float soundTimer; // +0x5c
    float unusedTimer; // +0x60
    int type; // +0x64
    int hitOffsetX; // +0x68
    int hitOffsetY; // +0x6c
    int w; // +0x70
    int h; // +0x74
    float x; // +0x78
    float y; // +0x7c
    float vx; // +0x80
    float vy; // +0x84
    char pad_88[0x4]; // +0x88
};

struct LvRawSub {   // 0x20 bytes; at 0xd0da40[50]
    int xOffset; // +0x0
    int yOffset; // +0x4
    int type; // +0x8
    int hp; // +0xc
    int fireRateMin; // +0x10
    int fireRateMax; // +0x14
    int fireDelay; // +0x18
    int pathId; // +0x1c
};

struct LvRawGrp {   // 0x664 bytes; at 0xd0da1c[25]
    int spawnX; // +0x0
    int spawnY; // +0x4
    int spawnDelay; // +0x8
    int spawnStep; // +0xc
    int count; // +0x10
    int velX; // +0x14
    int velY; // +0x18
    int groupId; // +0x1c
    int kind; // +0x20
    LvRawSub sub[50]; // +0x24
};

struct LvRawHdr {   // 0x14 bytes; at 0xd0d9b8[5]
    int count; // +0x0
    int type; // +0x4
    int hp; // +0x8
    int fireRateMin; // +0xc
    int fireRateMax; // +0x10
};

struct LvRawObj {   // 0x14 bytes; at 0xd17a9c[3750]
    int pathX; // +0x0
    int pathY; // +0x4
    int cmd; // +0x8
    int unusedD; // +0xc
    int holdTime; // +0x10
};

struct LvRawQ {   // 0x10 bytes; at 0xd2a1f8[50]
    int x1; // +0x0
    int y1; // +0x4
    int x2; // +0x8
    int y2; // +0xc
};

struct LevelRaw {   // 0x1cb98 bytes; at 0xd0d9b0
    int type; // +0x0
    int count; // +0x4
    LvRawHdr hdr[5]; // +0x8
    LvRawGrp grp[25]; // +0x6c
    int objCount[25]; // +0xa030
    char name1[36]; // +0xa094
    char name2[52]; // +0xa0b8
    LvRawObj obj[25][150]; // +0xa0ec
    char gfx[6][51]; // +0x1c5e4
    char mask[6][51]; // +0x1c716
    LvRawQ aux[50]; // +0x1c848
    int w[6]; // +0x1cb68
    int h[6]; // +0x1cb80
};

struct LevelRec {   // 0x20 bytes; at 0x8d2c08[4000], 0xc3e4d8[20000]
    union {
        struct {
            __int64 score; // +0x0
            int money; // +0x8
            int shots; // +0xc
            char verify0; // +0x10
            unsigned char verify1; // +0x11
            char verify2; // +0x12
            unsigned char livesGainedByte; // +0x13
            unsigned char deathsByte; // +0x14
            unsigned char armourAddedByte; // +0x15
            unsigned char verify3; // +0x16
            char verify4; // +0x17
            char rank; // +0x18
            unsigned char verify5; // +0x19
            unsigned char verify6; // +0x1a
            unsigned char frameBucket; // +0x1b
        };
        struct {
            char data[32]; // +0x0
        };
    };
};

struct LinkArea {   // 0x14 bytes; at 0x7f1048
    int x1; // +0x0
    int y1; // +0x4
    int x2; // +0x8
    int y2; // +0xc
    int id; // +0x10
};

struct MapObj {   // 0xa0 bytes; at 0xd5e338[100]
    Image *gfx; // +0x0
    void *drawnA; // +0x4
    int drawnB; // +0x8
    int drawnC; // +0xc
    Rect16 srcRect; // +0x10
    int active; // +0x20
    int unusedFlag14; // +0x24
    int player; // +0x28
    int type; // +0x2c
    float animT; // +0x30
    float dmg; // +0x34
    int state; // +0x38
    int writeOnlyType; // +0x3c
    int link1; // +0x40
    int link2; // +0x44
    short cost; // +0x48
    char pad_4a[0x2]; // +0x4a
    int laser; // +0x4c
    int unusedA; // +0x50
    int unusedB; // +0x54
    int w; // +0x58
    int stepY; // +0x5c
    float x; // +0x60
    float y; // +0x64
    float vx; // +0x68
    float vy; // +0x6c
    char pad_70[0x4]; // +0x70
    float speed; // +0x74
    int frame; // +0x78
    int enemy; // +0x7c
    float life; // +0x80
    int row; // +0x84
    float animDelay; // +0x88
    float animCnt; // +0x8c
    float turnDelay; // +0x90
    float turnCnt; // +0x94
    float unusedC; // +0x98
    float unusedD; // +0x9c
};

struct MenuEntry {   // 0x6c bytes; at 0xd5bf58[85]
    int active; // +0x0
    int hover; // +0x4
    int visible; // +0x8
    int unused0c; // +0xc
    int x; // +0x10
    int y; // +0x14
    int w; // +0x18
    int h; // +0x1c
    int segs; // +0x20
    int group; // +0x24
    int parent; // +0x28
    int page; // +0x2c
    char text[52]; // +0x30
    int style; // +0x64
    int width; // +0x68
};

struct ScorePopup {   // 0x48 bytes; at 0xe0c240
    int active; // +0x0
    float life; // +0x4
    __int64 value; // +0x8
    Image *glyphGfx; // +0x10
    float x; // +0x14
    float y; // +0x18
    float vy; // +0x1c
    char text[30]; // +0x20
    bool big; // +0x3e
    char pad_3f[0x1]; // +0x3f
    int blinkCounter; // +0x40
    int blinkPhase; // +0x44
};

struct EnemySet {   // 0x22470 bytes; at 0x849a4c
    Enemy enemies[150]; // +0x0
};

struct Particle {   // 0x54 bytes; at 0xac5c98[500]
    int active; // +0x0
    float x; // +0x4
    float y; // +0x8
    float vx; // +0xc
    float vy; // +0x10
    float ax; // +0x14
    float ay; // +0x18
    float frameSpeed; // +0x1c
    float frameF; // +0x20
    int frameBase; // +0x24
    int frame; // +0x28
    int ageFrame; // +0x2c
    int unusedFlickerTimer; // +0x30
    int unusedFlickerReset; // +0x34
    float size; // +0x38
    float alpha; // +0x3c
    float unusedFadeMul; // +0x40
    int gfx; // +0x44
    int r; // +0x48
    int g; // +0x4c
    int b; // +0x50
};

struct FreeParticle {   // 0x68 bytes; at 0xb30690
    int active; // +0x0
    float x; // +0x4
    float y; // +0x8
    float vx; // +0xc
    float vy; // +0x10
    int unused14; // +0x14
    int unused18; // +0x18
    float size; // +0x1c
    float sizeVel; // +0x20
    float angle; // +0x24
    float angleVel; // +0x28
    int r; // +0x2c
    int g; // +0x30
    int b; // +0x34
    float alpha; // +0x38
    float alphaStep; // +0x3c
    float speed; // +0x40
    float gravity; // +0x44
    float life; // +0x48
    int spawn; // +0x4c
    int dir; // +0x50
    int mode; // +0x54
    Image *graphic; // +0x58
    int *xref; // +0x5c
    int *kill; // +0x60
    unsigned char flag; // +0x64
    char pad_65[0x3]; // +0x65
};

struct Player {   // 0x4d8 bytes; at 0x8486e8[4]
    int inputDevice; // +0x0
    float x; // +0x4
    float y; // +0x8
    float mirrorX; // +0xc
    int extraLetterE; // +0x10
    float speed; // +0x14
    float bank; // +0x18
    float writeOnlyF1c; // +0x1c
    char pad_20[0x20]; // +0x20
    Image *gfx; // +0x40
    void *hitMask; // +0x44
    int hitMaskParamA; // +0x48
    int hitMaskParamB; // +0x4c
    Rect16 box; // +0x50
    int shots; // +0x60
    int hits; // +0x64
    int lives; // +0x68
    int deaths; // +0x6c
    int collisionsTaken; // +0x70
    int extraLetterR; // +0x74
    __int64 score; // +0x78
    int autofire; // +0x80
    int autofireUnlocked; // +0x84
    int turretTrackingReduction; // +0x88
    char pad_8c[0x4]; // +0x8c
    union {
        struct {
            int meteorBonus; // +0x90
        };
        struct {
            __int64 bonusRoundScore; // +0x90
        };
    };
    int gemCounterCollected; // +0x98
    char pad_9c[0x4]; // +0x9c
    __int64 bonusHighScore; // +0xa0
    int superAuto; // +0xa8
    int money; // +0xac
    int rank; // +0xb0
    int bestRank; // +0xb4
    float bonusRoundCount; // +0xb8
    int gems; // +0xbc
    int bombPickups; // +0xc0
    float marks; // +0xc4
    int extraLetterX; // +0xc8
    unsigned char color; // +0xcc
    char pad_cd[0x3]; // +0xcd
    int secretBirdHits; // +0xd0
    int level; // +0xd4
    int gemSeqB; // +0xd8
    int gemSeqA; // +0xdc
    int pickupCount; // +0xe0
    char extraProgress; // +0xe4
    char artxeProgress; // +0xe5
    char pad_e6[0x2]; // +0xe6
    int displayLevel; // +0xe8
    short unusedScoreThreshold; // +0xec
    char pad_ee[0x6]; // +0xee
    int extraLetterA; // +0xf4
    int msMultiplierActive; // +0xf8
    int writeOnlyMirrorFlag; // +0xfc
    int armour; // +0x100
    int freezeTimer; // +0x104
    int extraLetterT; // +0x108
    int scoreMult2Timer; // +0x10c
    int scoreMult5Timer; // +0x110
    int mirrorTime; // +0x114
    unsigned int drunkModeTimer; // +0x118
    unsigned int scoopTimer; // +0x11c
    unsigned int shieldTimer; // +0x120
    int blueMoneyActive; // +0x124
    int unusedInvulnBlinkFrame; // +0x128
    int unused12c; // +0x12c
    int unusedInvulnBlinkTick; // +0x130
    int autofireTimer; // +0x134
    int autofireInterval; // +0x138
    int shieldL; // +0x13c
    int shieldLIdx; // +0x140
    int shieldR; // +0x144
    int shieldRIdx; // +0x148
    union {
        struct {
            short weapon; // +0x14c
            short bullets; // +0x14e
        };
        struct {
            int weaponAmmoPacked; // +0x14c
        };
    };
    short energy; // +0x150
    short unused152; // +0x152
    float bulletSpeedMult; // +0x154
    int buffDuration; // +0x158
    char pad_15c[0x4]; // +0x15c
    int dead; // +0x160
    int unused164; // +0x164
    unsigned int respawnTime; // +0x168
    int time; // +0x16c
    int shopVisits; // +0x170
    float shieldHitFlashSpeed; // +0x174
    int enemyHpBonusRoll; // +0x178
    int hurryupComboCount; // +0x17c
    int rows; // +0x180
    int cols; // +0x184
    int rocketsFired; // +0x188
    int memoryGridUpgradeStreak; // +0x18c
    __int64 memoryBonus; // +0x190
    int gemPickups; // +0x198
    int unused19c; // +0x19c
    int shieldGlowG; // +0x1a0
    int shieldGlowB; // +0x1a4
    int unused1a8; // +0x1a8
    int shieldGlowGStep; // +0x1ac
    int shieldGlowBStep; // +0x1b0
    int unusedF1b4_5; // +0x1b4
    int unusedF1b8_1; // +0x1b8
    int placeSlot; // +0x1bc
    float flame; // +0x1c0
    int shipDestroyedThisLevel; // +0x1c4
    int trackKillsFlag; // +0x1c8
    int bonusRoundEnded; // +0x1cc
    int levelEnemyDataCount; // +0x1d0
    int unusedF1d4; // +0x1d4
    int ship; // +0x1d8
    int unusedF1dc; // +0x1dc
    float enemySwayX; // +0x1e0
    float enemySwayY; // +0x1e4
    char pad_1e8[0x8]; // +0x1e8
    float enemySwayVelX; // +0x1f0
    float enemySwayAccel; // +0x1f4
    float enemySwayMax; // +0x1f8
    float enemySwayMin; // +0x1fc
    char pad_200[0x8]; // +0x200
    float unusedF208; // +0x208
    float unusedF20c; // +0x20c
    float unusedF210; // +0x210
    float unusedF214; // +0x214
    int moneyMax; // +0x218
    int moneyMaxShown; // +0x21c
    int unusedF220; // +0x220
    int unusedF224; // +0x224
    int primaryEnemyCount; // +0x228
    int secondaryEnemyCount; // +0x22c
    int totalEnemies; // +0x230
    int spawnReserve; // +0x234
    int killed; // +0x238
    int escaped; // +0x23c
    int done; // +0x240
    int doneTime; // +0x244
    __int64 chainBonusValue; // +0x248
    int started; // +0x250
    float starVelX; // +0x254
    float starSpeed; // +0x258
    float starVelZ; // +0x25c
    float savedRaceStarSpeed; // +0x260
    float hyperspaceOutTimer; // +0x264
    float hyperspaceMidTimer; // +0x268
    float hyperspaceInTimer; // +0x26c
    float hyperspaceInDuration; // +0x270
    float hyperspaceFade; // +0x274
    float savedHyperspaceFade; // +0x278
    float savedHyperspaceInDuration; // +0x27c
    float savedHyperspaceInTimer; // +0x280
    float savedHyperspaceOutTimer; // +0x284
    float savedStarSpeed; // +0x288
    float savedStarVelZ; // +0x28c
    float savedScrollSpeedY; // +0x290
    float savedHyperspaceMidTimer; // +0x294
    float scrollSpeedY; // +0x298
    int levelFinished; // +0x29c
    int levelTransitioning; // +0x2a0
    int effectDuration; // +0x2a4
    int levelWarpPending; // +0x2a8
    int keyLatchFire; // +0x2ac
    int keyLatchRocket; // +0x2b0
    int keyLatchPause; // +0x2b4
    int keyLatchProfile; // +0x2b8
    int bonusTally; // +0x2bc
    int bonusTallyTick; // +0x2c0
    float bonusTallyDelay; // +0x2c4
    float perfectTextSpacing; // +0x2c8
    float perfectTextSpacingVel; // +0x2cc
    int perfectDone; // +0x2d0
    int bonusResultsInitDone; // +0x2d4
    int bonusKilled; // +0x2d8
    int gemCounterPicks; // +0x2dc
    int gemCounterUnlocked; // +0x2e0
    int highScoreMilestone; // +0x2e4
    int blueMoneyPicks; // +0x2e8
    int blueMoneyUnlocked; // +0x2ec
    int multiplierPicks; // +0x2f0
    int multiplierUnlocked; // +0x2f4
    int weaponFloorAtOne; // +0x2f8
    union {
        struct {
            int secretFound01; // +0x2fc
            int secretFound02; // +0x300
            int secretFound03; // +0x304
            int secretFound04; // +0x308
            int secretFound05; // +0x30c
            int secretFound06; // +0x310
            int secretFound07; // +0x314
            int secretFound08; // +0x318
            int secretFound09; // +0x31c
            int secretFound10; // +0x320
            int secretFound11; // +0x324
            int secretFound12; // +0x328
            int secretFound13; // +0x32c
            int secretFound14; // +0x330
            int secretFound15; // +0x334
            int secretFound16; // +0x338
            int secretFound17; // +0x33c
            int secretFound18; // +0x340
            int secretFound19; // +0x344
            int secretFound20; // +0x348
            int secretFound21; // +0x34c
            int secretFound22; // +0x350
            int secretFound23; // +0x354
            int secretFound24; // +0x358
            int secretFound25; // +0x35c
            int secretFound26; // +0x360
            int secretFound27; // +0x364
            int secretFound28; // +0x368
            int secretFound29; // +0x36c
            int secretFound30; // +0x370
            int secretFound31to50[20]; // +0x374
            int secretSeen[50]; // +0x3c4
        };
        struct {
            int secretFlags[100]; // +0x2fc
        };
    };
    int secretCount; // +0x48c
    __int64 bonusRoundPoints; // +0x490
    int perfectStreak; // +0x498
    int drunkStreak; // +0x49c
    int rockets; // +0x4a0
    unsigned char alienLock; // +0x4a4
    char pad_4a5[0x3]; // +0x4a5
    int tries; // +0x4a8
    unsigned char levelMilestoneHandled; // +0x4ac
    char pad_4ad[0x3]; // +0x4ad
    int secretBirdCounter; // +0x4b0
    int secretBirdTick; // +0x4b4
    int raceDistance; // +0x4b8
    int gameSpeedSetting; // +0x4bc
    unsigned char maxRankReached; // +0x4c0
    char pad_4c1[0x3]; // +0x4c1
    int bulkLevelsCooldown; // +0x4c4
    int nextShotSnd; // +0x4c8
    char pad_4cc[0x4]; // +0x4cc
    __int64 sessionPlayTime; // +0x4d0
};

struct QuadItem {   // 0x2c bytes; at 0xf7a2c8
    float dx; // +0x0
    float dy; // +0x4
    float dw; // +0x8
    float dh; // +0xc
    float sx; // +0x10
    float sy; // +0x14
    float sw; // +0x18
    float sh; // +0x1c
    Image *graphic; // +0x20
    char pad_24[0x8]; // +0x24
};

struct Ring {   // 0x30 bytes; at 0x8ff578[4]
    int active; // +0x0
    int x; // +0x4
    int y; // +0x8
    int radius; // +0xc
    float speed; // +0x10
    int unused14; // +0x14
    int r; // +0x18
    int g; // +0x1c
    int b; // +0x20
    float alpha; // +0x24
    int unused28; // +0x28
    Image *gfx; // +0x2c
};

struct SaveData {   // 0x1370 bytes; at 0x8486d8
    char sig[8]; // +0x0
    __int64 saveId; // +0x8
    Player players[4]; // +0x10
};

struct ShipDef {   // 0x24 bytes
    int minEnergy; // +0x0
    int cost; // +0x4
    int extraLives; // +0x8
    int maxEnergy; // +0xc
    int baseArmour; // +0x10
    int armourStep; // +0x14
    int maxArmourBonus; // +0x18
    int gemBase; // +0x1c
    int gemStep; // +0x20
};

struct BurstSpark {   // 0x34 bytes; at 0x7e3948[1000]
    int active; // +0x0
    float x; // +0x4
    float y; // +0x8
    float pos; // +0xc
    float vel; // +0x10
    float accel; // +0x14
    float r; // +0x18
    float g; // +0x1c
    float b; // +0x20
    float dr; // +0x24
    float dg; // +0x28
    float db; // +0x2c
    int angle; // +0x30
};

struct PulseFx {   // 0x34 bytes; at 0xcde8c0
    int active; // +0x0
    float alpha; // +0x4
    float scale; // +0x8
    float speed; // +0xc
    float unusedTimer10; // +0x10
    char pad_14[0x8]; // +0x14
    Image *gfx; // +0x1c
    float x; // +0x20
    float y; // +0x24
    union {
        struct {
            unsigned char r; // +0x28
        };
        struct {
            int rInit; // +0x28
        };
    };
    union {
        struct {
            unsigned char g; // +0x2c
        };
        struct {
            int gInit; // +0x2c
        };
    };
    union {
        struct {
            unsigned char b; // +0x30
        };
        struct {
            int bInit; // +0x30
        };
    };
};

struct AlienGfxSlot {   // 0x54c bytes; at 0x803c98[200]
    short id; // +0x0
    short _p002; // +0x2
    int owner; // +0x4
    void *src; // +0x8
    int srcLen; // +0xc
    unsigned char loaded[6]; // +0x10
    char pad_16[0x2]; // +0x16
    int count[6]; // +0x18
    Image *gfx[6]; // +0x30
    Image *gfx2[6]; // +0x48
    void *hma[6]; // +0x60
    char name1[6][51]; // +0x78
    char name2[6][51]; // +0x1aa
    char name3[6][51]; // +0x2dc
    char pad_40e[0x2]; // +0x40e
    int key[6]; // +0x410
    FrameSet blk[6]; // +0x428
    int frameCount; // +0x4e8
    int frameW[6]; // +0x4ec
    int frameH[6]; // +0x504
    int unusedI51c[6]; // +0x51c
    int unusedI534[6]; // +0x534
};

struct SoundQueueEntry {   // 0x10 bytes; at 0xab27d8[10]
    AudioHandle sample; // +0x0
    int length; // +0x4
    unsigned int time; // +0x8
    int vol; // +0xc
};

struct Spark {   // 0x60 bytes; at 0xcdeac8[2000]
    int active; // +0x0
    float x; // +0x4
    float y; // +0x8
    float vx; // +0xc
    float vy; // +0x10
    int moving; // +0x14
    char pad_18[0x10]; // +0x18
    int type; // +0x28
    char pad_2c[0x8]; // +0x2c
    int r; // +0x34
    int g; // +0x38
    int b; // +0x3c
    int size; // +0x40
    int delay; // +0x44
    int trailInterval; // +0x48
    int trailTimer; // +0x4c
    float alpha; // +0x50
    float fade; // +0x54
    char pad_58[0x8]; // +0x58
};

struct FallingSprite {   // 0x54 bytes; at 0x847398
    int active; // +0x0
    int type; // +0x4
    int sy; // +0x8
    int h; // +0xc
    int w; // +0x10
    int frame; // +0x14
    float animDelay; // +0x18
    float animTimer; // +0x1c
    char pad_20[0x8]; // +0x20
    void *hma; // +0x28
    int hmaW; // +0x2c
    int hmaH; // +0x30
    union {
        struct {
            int r1; // +0x34
            int r2; // +0x38
            int r3; // +0x3c
            int r4; // +0x40
        };
        struct {
            Rect16 hitRect; // +0x34
        };
    };
    float x; // +0x44
    float y; // +0x48
    char pad_4c[0x4]; // +0x4c
    float vy; // +0x50
};

struct ScoopTrail {   // 0x1c bytes; at 0xd622d4[15]
    float pos; // +0x0
    int spawnDelay; // +0x4
    float vel; // +0x8
    int unusedF0c; // +0xc
    int sx; // +0x10
    int timer; // +0x14
    int unusedF18; // +0x18
};

struct HyperspaceStar {   // 0x18 bytes; at 0x7f12a0[3000]
    float x; // +0x0
    float y; // +0x4
    float z; // +0x8
    char pad_c[0xc]; // +0xc
};

struct StretchItemF {   // 0x1c bytes; at 0xf7b6a0
    Image *graphic; // +0x0
    float x1; // +0x4
    float y1; // +0x8
    float x2; // +0xc
    float y2; // +0x10
    unsigned char r; // +0x14
    unsigned char g; // +0x15
    unsigned char b; // +0x16
    unsigned char a; // +0x17
    unsigned char flag; // +0x18
    char pad_19[0x3]; // +0x19
};

struct StretchItemI {   // 0x18 bytes; at 0xf79280
    Image *graphic; // +0x0
    int x1; // +0x4
    int y1; // +0x8
    int x2; // +0xc
    int y2; // +0x10
    unsigned char r; // +0x14
    unsigned char g; // +0x15
    unsigned char b; // +0x16
    unsigned char a; // +0x17
};

struct StretchItemRot {   // 0x20 bytes; at 0xf71580, 0xf7b420
    Image *graphic; // +0x0
    float x1; // +0x4
    float y1; // +0x8
    float x2; // +0xc
    float y2; // +0x10
    unsigned char r; // +0x14
    unsigned char g; // +0x15
    unsigned char b; // +0x16
    unsigned char a; // +0x17
    unsigned char flag; // +0x18
    char pad_19[0x3]; // +0x19
    float angle; // +0x1c
};

struct QuadImageItem {   // 0x28 bytes; at 0xd62544[150]
    int destX; // +0x0
    int destY; // +0x4
    int destW; // +0x8
    int destH; // +0xc
    int srcX; // +0x10
    int srcY; // +0x14
    int srcW; // +0x18
    int srcH; // +0x1c
    int unused20; // +0x20
    void *graphic; // +0x24
};

struct ButtonItem {   // 0x11c bytes; at 0xd63cb8[10]
    unsigned char hover; // +0x0
    char pad_1[0x3]; // +0x1
    int x; // +0x4
    int y; // +0x8
    int srcX; // +0xc
    int srcY; // +0x10
    int w; // +0x14
    int h; // +0x18
    char text[256]; // +0x1c
};

struct ImageRectItem {   // 0x18 bytes; at 0xd647d4[5]
    int x; // +0x0
    int y; // +0x4
    int w; // +0x8
    int h; // +0xc
    int unused10; // +0x10
    void *graphic; // +0x14
};

struct LinkItem {   // 0x218 bytes; at 0xd64850[10]
    int x; // +0x0
    int y; // +0x4
    char text1[255]; // +0x8
    char text2[255]; // +0x107
    char pad_206[0x2]; // +0x206
    int color; // +0x208
    int hoverColor; // +0x20c
    union {
        struct {
            unsigned char hilite; // +0x210
        };
        struct {
            int unusedHilite210; // +0x210
        };
    };
    int id; // +0x214
};

struct TextItem {   // 0x10c bytes; at 0xd65d44[50]
    int x; // +0x0
    int y; // +0x4
    char text[256]; // +0x8
    int color; // +0x108
};

struct MenuItem {   // 0x6c bytes; at 0xd691a0[40]
    int unused00; // +0x0
    int state; // +0x4
    int unused08; // +0x8
    unsigned char checked; // +0xc
    unsigned char hilite; // +0xd
    char pad_e[0x2]; // +0xe
    int x; // +0x10
    int y; // +0x14
    int w; // +0x18
    int h; // +0x1c
    int len; // +0x20
    int id; // +0x24
    int unused28; // +0x28
    int unused2c; // +0x2c
    char text[52]; // +0x30
    int param; // +0x64
    int unused68; // +0x68
};

struct ToggleItem {   // 0x138 bytes; at 0xd6a288[50]
    int unused00; // +0x0
    int value; // +0x4
    int unused08; // +0x8
    int state; // +0xc
    int flag2; // +0x10
    int x; // +0x14
    int y; // +0x18
    int w; // +0x1c
    int h; // +0x20
    int index; // +0x24
    int unused28; // +0x28
    int unused2c; // +0x2c
    int unused30; // +0x30
    int unused34; // +0x34
    char text[256]; // +0x38
};

struct EditItem {   // 0x11c bytes; at 0xd6df7c[30]
    int x; // +0x0
    int y; // +0x4
    int len; // +0x8
    int cursor; // +0xc
    int index; // +0x10
    unsigned char masked; // +0x14
    unsigned char focused; // +0x15
    char buf[258]; // +0x16
    int param; // +0x118
};

struct Window {   // 0xdbc0 bytes; at 0xd62510[10]
    int active; // +0x0
    int unused04; // +0x4
    int unused08; // +0x8
    int visible; // +0xc
    int index; // +0x10
    int x; // +0x14
    int y; // +0x18
    int w; // +0x1c
    int h; // +0x20
    int mode; // +0x24
    float slideX; // +0x28
    float slideY; // +0x2c
    int nA; // +0x30
    QuadImageItem quadImages[150]; // +0x34
    int nB; // +0x17a4
    ButtonItem buttons[10]; // +0x17a8
    int nC; // +0x22c0
    ImageRectItem imageRects[5]; // +0x22c4
    int nD; // +0x233c
    LinkItem links[10]; // +0x2340
    int nE; // +0x3830
    TextItem texts[50]; // +0x3834
    int nF; // +0x6c8c
    MenuItem menuItems[40]; // +0x6c90
    int selF; // +0x7d70
    int nG; // +0x7d74
    ToggleItem toggles[50]; // +0x7d78
    int nH; // +0xba68
    EditItem edits[30]; // +0xba6c
    int firstH; // +0xdbb4
    float blinkTimer; // +0xdbb8
    bool blinkOn; // +0xdbbc
    char pad_dbbd[0x3]; // +0xdbbd
};

// Sizes, as laid out by the compiler. The structs that hold pointers (C_ASSERT_32) have the
// original's size only in a 32-bit build; none of them is written to a file as it is.
C_ASSERT(sizeof(Account) == 0x40d0);
C_ASSERT(sizeof(AccountV0) == 0x1b48);
C_ASSERT(sizeof(AccountV2) == 0x3c00);
C_ASSERT(sizeof(Beam) == 0x34);
C_ASSERT_32(sizeof(BlitItem) == 0x20);
C_ASSERT_32(sizeof(Bonus) == 0x64);
C_ASSERT(sizeof(BonusStats) == 0x60);
C_ASSERT_32(sizeof(FallingHazard) == 0x58);
C_ASSERT(sizeof(Box) == 0x10);
C_ASSERT(sizeof(Card) == 0x18);
C_ASSERT(sizeof(FadeColors) == 0x78);
C_ASSERT(sizeof(EndRect) == 0x10);
C_ASSERT_32(sizeof(Enemy) == 0x3a8);
C_ASSERT(sizeof(Explosion) == 0x4c);
C_ASSERT(sizeof(FPair) == 0x8);
C_ASSERT_32(sizeof(Flash) == 0x28);
C_ASSERT(sizeof(Flash34) == 0x34);
C_ASSERT(sizeof(Pattern) == 0xbe0);
C_ASSERT(sizeof(PatternPt) == 0x14);
C_ASSERT(sizeof(FrameSet) == 0x20);
C_ASSERT(sizeof(HiscoreData) == 0x30d4);
C_ASSERT(sizeof(HiscoreEntry) == 0x68);
C_ASSERT_32(sizeof(IntPair) == 0x8);
C_ASSERT(sizeof(Level) == 0x1cb98);
C_ASSERT_32(sizeof(LevelObj) == 0x8c);
C_ASSERT(sizeof(LevelRaw) == 0x1cb98);
C_ASSERT(sizeof(LevelRec) == 0x20);
C_ASSERT(sizeof(LinkArea) == 0x14);
C_ASSERT(sizeof(LvGrp) == 0x664);
C_ASSERT(sizeof(LvHdr) == 0x14);
C_ASSERT(sizeof(LvObj) == 0x14);
C_ASSERT(sizeof(LvRawGrp) == 0x664);
C_ASSERT(sizeof(LvRawHdr) == 0x14);
C_ASSERT(sizeof(LvRawObj) == 0x14);
C_ASSERT(sizeof(LvRawQ) == 0x10);
C_ASSERT(sizeof(LvRawSub) == 0x20);
C_ASSERT(sizeof(LvSub) == 0x20);
C_ASSERT_32(sizeof(MapObj) == 0xa0);
C_ASSERT(sizeof(MenuEntry) == 0x6c);
C_ASSERT(sizeof(Name255) == 0xff);
C_ASSERT_32(sizeof(ScorePopup) == 0x48);
C_ASSERT_32(sizeof(EnemySet) == 0x22470);
C_ASSERT(sizeof(Particle) == 0x54);
C_ASSERT_32(sizeof(FreeParticle) == 0x68);
C_ASSERT_32(sizeof(Player) == 0x4d8);
C_ASSERT_32(sizeof(QuadItem) == 0x2c);
C_ASSERT(sizeof(Rect16) == 0x10);
C_ASSERT_32(sizeof(Ring) == 0x30);
C_ASSERT_32(sizeof(SaveData) == 0x1370);
C_ASSERT(sizeof(Settings) == 0x638);
C_ASSERT(sizeof(SettingsV2) == 0x630);
C_ASSERT(sizeof(ShipDef) == 0x24);
C_ASSERT(sizeof(BurstSpark) == 0x34);
C_ASSERT_32(sizeof(PulseFx) == 0x34);
C_ASSERT_32(sizeof(AlienGfxSlot) == 0x54c);
C_ASSERT(sizeof(SoundQueueEntry) == 0x10);
C_ASSERT(sizeof(Spark) == 0x60);
C_ASSERT_32(sizeof(FallingSprite) == 0x54);
C_ASSERT(sizeof(ScoopTrail) == 0x1c);
C_ASSERT(sizeof(HyperspaceStar) == 0x18);
C_ASSERT_32(sizeof(StretchItemF) == 0x1c);
C_ASSERT_32(sizeof(StretchItemI) == 0x18);
C_ASSERT_32(sizeof(StretchItemRot) == 0x20);
C_ASSERT_32(sizeof(QuadImageItem) == 0x28);
C_ASSERT(sizeof(ButtonItem) == 0x11c);
C_ASSERT_32(sizeof(ImageRectItem) == 0x18);
C_ASSERT(sizeof(LinkItem) == 0x218);
C_ASSERT(sizeof(TextItem) == 0x10c);
C_ASSERT(sizeof(MenuItem) == 0x6c);
C_ASSERT(sizeof(ToggleItem) == 0x138);
C_ASSERT(sizeof(EditItem) == 0x11c);
C_ASSERT_32(sizeof(Window) == 0xdbc0);

// Member offsets the code depends on (moved here from the source files).
C_ASSERT(offsetof(Account, settings) == 0x1510);
C_ASSERT(offsetof(Account, saveIdHistory) == 0x3c08);
C_ASSERT(offsetof(Account, lastSaveId) == 0x40b8);
C_ASSERT(offsetof(Account, medalOrder) == 0x1b48);
C_ASSERT(offsetof(Account, names) == 0x121);
C_ASSERT(offsetof(AccountV2, medalOrder) == 0x1b40);
C_ASSERT_32(offsetof(Player, gameSpeedSetting) == 0x4bc);
C_ASSERT_32(offsetof(Player, bonusResultsInitDone) == 0x2d4);
C_ASSERT_32(offsetof(Player, scrollSpeedY) == 0x298);
C_ASSERT_32(offsetof(Player, ship) == 0x1d8);
