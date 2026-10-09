// Tests for src/profile/settings.c: factory defaults, WarBlade.inf round trip, difficulty tuning.
#include <stdio.h>
#include <sys/stat.h>
#include "support.h"

enum { CFG_SIZE = 0x638 };

static const char *InfPath(void) { return FakeUserPath("warblade\\WarBlade.inf"); }

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

// A WarBlade.inf holding `cfg` as it is (not through WriteSettings).
static void WriteInf(const Settings *cfg)
{
    MakeGameDir();
    WriteHostFile(InfPath(), cfg, CFG_SIZE);
}

// Defaults, but with a few values that differ from them.
static Settings CustomSettings(void)
{
    DefaultSettings();
    Settings c = g_cfg;
    c.sfxVol = 0x40;
    c.musicVol = 0x80;
    c.musicVolume = 0x22;
    c.difficulty = DIFF_ACE;
    c.sparks = 40.0f;
    c.alienBuffer = 9;
    c.windowedByte = 0;
    c.fps = 75;
    c.collisionDetail = COLLISION_SIMPLE;
    c.best = 123456789012LL;
    c.left[0] = 0x21;
    c.profileSel = 3;
    return c;
}

// ---------------------------------------------------------------- DefaultSettings

TEST(Profile_DefaultSettings_sets_audio_video_defaults)
{
    memset(&g_cfg, 0x55, sizeof g_cfg);
    g_presets = 0;
    g_screenW = g_screenH = 0;
    DefaultSettings();
    CHECK_EQ_INT(g_cfg.version, 120);
    CHECK_EQ_INT(g_presets, 1);
    CHECK_EQ_INT(g_cfg.borderMode, BORDER_ON);
    CHECK_EQ_INT(g_cfg.sfxVol, 0xcc);
    CHECK_EQ_INT(g_cfg.musicVolume, 0xb3);
    CHECK_EQ_INT(g_cfg.musicVol, 0xff);
    CHECK_EQ_INT(g_cfg.fps, 60);
    CHECK_NEAR(g_cfg.numStars, 600.0, 0);
    CHECK_EQ_INT(g_cfg.sfxOn, 1);
    CHECK_EQ_INT(g_cfg.collisionDetail, COLLISION_NORMAL);
    CHECK_EQ_INT(g_cfg.bgStars, 1);
    CHECK_EQ_INT(g_cfg.particlesOn, 1);
    CHECK_EQ_INT(g_cfg.best, 0);
    CHECK_EQ_INT(g_cfg.bgEnabled, 0);
    CHECK_EQ_INT(g_cfg.bgTint, 0x37);
    CHECK_EQ_INT(g_cfg.profileSel, -1);
    CHECK_EQ_INT(g_cfg.voice, 1);
    CHECK_EQ_INT(g_cfg.shuffleByte, 1);
    CHECK_EQ_INT(g_cfg.checkVersion, 1);
    CHECK_EQ_INT(g_cfg.alienBuffer, 5);
    CHECK_EQ_INT(g_cfg.renderer, RENDERER_AUTO);
    CHECK_EQ_INT(g_cfg.vsyncOff, 0);
    CHECK_EQ_INT(g_cfg.interpolation, SYS_INTERP_AUTO);
    CHECK_EQ_INT(g_cfg.windowedByte, 1);
    // set to MOD early on, then overridden at the end
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_MP3);
    CHECK_EQ_INT(g_screenW, 800);
    CHECK_EQ_INT(g_screenH, 600);
}

TEST(Profile_DefaultSettings_sparks_and_max_sparks)
{
    g_maxSparks = 0;
    DefaultSettings();
    CHECK_NEAR(g_cfg.sparks, 20.0, 0);
    CHECK_EQ_INT(g_maxSparks, 10);
}

TEST(Profile_DefaultSettings_player_one_bindings)
{
    memset(&g_cfg, 0x55, sizeof g_cfg);
    DefaultSettings();
    CHECK_EQ_INT(g_cfg.device[0], DEVICE_KEYBOARD);
    CHECK_EQ_INT(g_cfg.left[0], 0);
    CHECK_EQ_INT(g_cfg.right[0], 3);
    CHECK_EQ_INT(g_cfg.up[0], 1);
    CHECK_EQ_INT(g_cfg.down[0], 2);
    CHECK_EQ_INT(g_cfg.fire[0], 9);
    CHECK_EQ_INT(g_cfg.rocket[0], 5);
    CHECK_EQ_INT(g_cfg.pause[0], 0x28);
    CHECK_EQ_INT(g_cfg.profile[0], 0x17);
    CHECK_EQ_INT(g_cfg.key6[0], 0x30);
    CHECK_EQ_INT(g_cfg.joyFire[0], 0);
    CHECK_EQ_INT(g_cfg.joyRocket[0], 1);
    CHECK_EQ_INT(g_cfg.joyBtn2[0], 2);
    CHECK_EQ_INT(g_cfg.joyPause[0], 3);
    CHECK_EQ_INT(g_cfg.joyProfile[0], 4);
}

TEST(Profile_DefaultSettings_player_two_bindings)
{
    memset(&g_cfg, 0x55, sizeof g_cfg);
    DefaultSettings();
    CHECK_EQ_INT(g_cfg.device[1], DEVICE_KEYBOARD);
    CHECK_EQ_INT(g_cfg.left[1], 0x41);
    CHECK_EQ_INT(g_cfg.right[1], 0x43);
    CHECK_EQ_INT(g_cfg.up[1], 0x45);
    CHECK_EQ_INT(g_cfg.down[1], 0x3f);
    CHECK_EQ_INT(g_cfg.fire[1], 8);
    CHECK_EQ_INT(g_cfg.rocket[1], 6);
    CHECK_EQ_INT(g_cfg.pause[1], 0x28);
    CHECK_EQ_INT(g_cfg.profile[1], 0x17);
    CHECK_EQ_INT(g_cfg.key6[1], 7);
    CHECK_EQ_INT(g_cfg.joyFire[1], 0);
    CHECK_EQ_INT(g_cfg.joyRocket[1], 1);
    CHECK_EQ_INT(g_cfg.joyBtn2[1], 2);
    CHECK_EQ_INT(g_cfg.joyPause[1], 3);
    CHECK_EQ_INT(g_cfg.joyProfile[1], 4);
}

TEST(Profile_DefaultSettings_picks_normal_difficulty_tuning)
{
    g_cfg.difficulty = DIFF_ACE;
    DefaultSettings();
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_NORMAL);
    CHECK_EQ_INT(g_diffScoreBonus, 3);
    CHECK_EQ_INT(g_bonusDuration, 40000);
    CHECK_NEAR(g_enemyBulletSpeed, 4.3, 1e-6);
}

// ---------------------------------------------------------------- SetDifficulty

TEST(Profile_SetDifficulty_easy)
{
    g_cfg.difficulty = DIFF_EASY;
    SetDifficulty();
    CHECK_NEAR(g_gameSpeedMul, 1.0, 0);
    CHECK_EQ_INT(g_diffScoreBonus, 2);
    CHECK_EQ_INT(g_speedMin, 15);
    CHECK_EQ_INT(g_fireDelayBiasA, 400);
    CHECK_EQ_INT(g_fireDelayBiasB, 400);
    CHECK_EQ_INT(g_fireDelayMin, 300);
    CHECK_EQ_INT(g_enemyFireRateMin, 300);
    CHECK_NEAR(g_enemyBulletSpeed, 3.5, 1e-6);
    CHECK_EQ_INT(g_diffShotFuseBase, 200);
    CHECK_EQ_INT(g_diffShotFuseRange, 200);
    CHECK_EQ_INT(g_hurryUpInterval, 170);
    CHECK_NEAR(g_diffShotSpeedMin, 2.4, 1e-6);
    CHECK_NEAR(g_diffShotSpeedMax, 3.2, 1e-6);
    CHECK_NEAR(g_diffTurretTrackChance, 50.0, 0);
    CHECK_NEAR(g_speedBase, 4.2, 1e-6);
    CHECK_NEAR(g_speedStep, 0.8, 1e-6);
    CHECK_EQ_INT(g_bonusDuration, 50000);
    CHECK_NEAR(g_bonusSpawnRampRate, 0.0014, 1e-8);
    CHECK_EQ_INT(g_bonusThresholdBase, 12090);
    CHECK_EQ_INT(g_bonusRareChance, 7);
    CHECK_NEAR(g_diffBonusDropRoll, 18.0, 0);
    CHECK_EQ_INT(g_diffEnemyHpBonus, 10);
    CHECK_EQ_INT(g_moneySuckerBaseHp, 300);
    CHECK_EQ_INT(g_hurryUpHpBonus, 75);
    CHECK_NEAR(g_diffHurryUpSpeedMax, 3.0, 0);
    CHECK_EQ_INT(g_eliteHpBonus, 1500);
    CHECK_EQ_INT(g_diffEnemyFireChance, 4);
    CHECK_NEAR(g_diffEnemyTimerMul, 3.0, 0);
}

TEST(Profile_SetDifficulty_normal)
{
    g_cfg.difficulty = DIFF_NORMAL;
    SetDifficulty();
    CHECK_NEAR(g_gameSpeedMul, 1.0, 0);
    CHECK_EQ_INT(g_diffScoreBonus, 3);
    CHECK_EQ_INT(g_speedMin, 10);
    CHECK_EQ_INT(g_fireDelayBiasA, 200);
    CHECK_EQ_INT(g_fireDelayBiasB, 200);
    CHECK_EQ_INT(g_fireDelayMin, 200);
    CHECK_EQ_INT(g_enemyFireRateMin, 200);
    CHECK_NEAR(g_enemyBulletSpeed, 4.3, 1e-6);
    CHECK_EQ_INT(g_diffShotFuseBase, 200);
    CHECK_EQ_INT(g_diffShotFuseRange, 225);
    CHECK_EQ_INT(g_hurryUpInterval, 130);
    CHECK_NEAR(g_diffShotSpeedMin, 3.1, 1e-6);
    CHECK_NEAR(g_diffShotSpeedMax, 3.8, 1e-6);
    CHECK_NEAR(g_diffTurretTrackChance, 40.0, 0);
    CHECK_NEAR(g_speedBase, 4.0, 0);
    CHECK_NEAR(g_speedStep, 0.7, 1e-6);
    CHECK_EQ_INT(g_bonusDuration, 40000);
    CHECK_NEAR(g_bonusSpawnRampRate, 0.00145, 1e-8);
    CHECK_EQ_INT(g_bonusThresholdBase, 14000);
    CHECK_EQ_INT(g_bonusRareChance, 6);
    CHECK_NEAR(g_diffBonusDropRoll, 28.0, 0);
    CHECK_EQ_INT(g_diffEnemyHpBonus, 16);
    CHECK_EQ_INT(g_moneySuckerBaseHp, 350);
    CHECK_EQ_INT(g_hurryUpHpBonus, 100);
    CHECK_NEAR(g_diffHurryUpSpeedMax, 4.0, 0);
    CHECK_EQ_INT(g_eliteHpBonus, 1750);
    CHECK_EQ_INT(g_diffEnemyFireChance, 6);
    CHECK_NEAR(g_diffEnemyTimerMul, 2.2, 1e-6);
}

TEST(Profile_SetDifficulty_hard)
{
    g_cfg.difficulty = DIFF_HARD;
    SetDifficulty();
    CHECK_NEAR(g_gameSpeedMul, 1.1666666, 1e-6);
    CHECK_EQ_INT(g_diffScoreBonus, 3);
    CHECK_EQ_INT(g_speedMin, 5);
    CHECK_EQ_INT(g_fireDelayBiasA, -50);
    CHECK_EQ_INT(g_fireDelayBiasB, -50);
    CHECK_EQ_INT(g_fireDelayMin, 190);
    CHECK_EQ_INT(g_enemyFireRateMin, 190);
    CHECK_NEAR(g_enemyBulletSpeed, 5.0, 0);
    CHECK_EQ_INT(g_diffShotFuseBase, 210);
    CHECK_EQ_INT(g_diffShotFuseRange, 230);
    CHECK_EQ_INT(g_hurryUpInterval, 115);
    CHECK_NEAR(g_diffShotSpeedMin, 3.3, 1e-6);
    CHECK_NEAR(g_diffShotSpeedMax, 4.3, 1e-6);
    CHECK_NEAR(g_diffTurretTrackChance, 30.0, 0);
    CHECK_NEAR(g_speedBase, 3.5, 0);
    CHECK_NEAR(g_speedStep, 0.6, 1e-6);
    CHECK_EQ_INT(g_bonusDuration, 30000);
    CHECK_NEAR(g_bonusSpawnRampRate, 0.0015, 1e-8);
    CHECK_EQ_INT(g_bonusThresholdBase, 15540);
    CHECK_EQ_INT(g_bonusRareChance, 5);
    CHECK_NEAR(g_diffBonusDropRoll, 38.0, 0);
    CHECK_EQ_INT(g_diffEnemyHpBonus, 20);
    CHECK_EQ_INT(g_moneySuckerBaseHp, 450);
    CHECK_EQ_INT(g_hurryUpHpBonus, 125);
    CHECK_NEAR(g_diffHurryUpSpeedMax, 5.0, 0);
    CHECK_EQ_INT(g_eliteHpBonus, 2000);
    CHECK_EQ_INT(g_diffEnemyFireChance, 10);
    CHECK_NEAR(g_diffEnemyTimerMul, 2.0, 0);
}

TEST(Profile_SetDifficulty_ace)
{
    g_cfg.difficulty = DIFF_ACE;
    SetDifficulty();
    CHECK_NEAR(g_gameSpeedMul, 1.3333334, 1e-6);
    CHECK_EQ_INT(g_diffScoreBonus, 2);
    CHECK_EQ_INT(g_speedMin, 5);
    CHECK_EQ_INT(g_fireDelayBiasA, -200);
    CHECK_EQ_INT(g_fireDelayBiasB, -200);
    CHECK_EQ_INT(g_fireDelayMin, 180);
    CHECK_EQ_INT(g_enemyFireRateMin, 180);
    CHECK_NEAR(g_enemyBulletSpeed, 5.5, 0);
    CHECK_EQ_INT(g_diffShotFuseBase, 220);
    CHECK_EQ_INT(g_diffShotFuseRange, 235);
    CHECK_EQ_INT(g_hurryUpInterval, 95);
    CHECK_NEAR(g_diffShotSpeedMin, 3.5, 0);
    CHECK_NEAR(g_diffShotSpeedMax, 4.8, 1e-6);
    CHECK_NEAR(g_diffTurretTrackChance, 20.0, 0);
    CHECK_NEAR(g_speedBase, 3.0, 0);
    CHECK_NEAR(g_speedStep, 0.5, 0);
    CHECK_EQ_INT(g_bonusDuration, 20000);
    CHECK_NEAR(g_bonusSpawnRampRate, 0.0017, 1e-8);
    CHECK_EQ_INT(g_bonusThresholdBase, 17000);
    CHECK_EQ_INT(g_bonusRareChance, 4);
    CHECK_NEAR(g_diffBonusDropRoll, 48.0, 0);
    CHECK_EQ_INT(g_diffEnemyHpBonus, 25);
    CHECK_EQ_INT(g_moneySuckerBaseHp, 600);
    CHECK_EQ_INT(g_hurryUpHpBonus, 150);
    CHECK_NEAR(g_diffHurryUpSpeedMax, 6.0, 0);
    CHECK_EQ_INT(g_eliteHpBonus, 2500);
    CHECK_EQ_INT(g_diffEnemyFireChance, 15);
    CHECK_NEAR(g_diffEnemyTimerMul, 1.8, 1e-6);
}

TEST(Profile_SetDifficulty_out_of_range_changes_nothing)
{
    g_cfg.difficulty = NUM_DIFFICULTIES;
    g_gameSpeedMul = 7.0f;
    g_diffScoreBonus = 77;
    g_bonusDuration = 777;
    g_fireDelayBiasA = 7;
    SetDifficulty();
    CHECK_NEAR(g_gameSpeedMul, 7.0, 0);
    CHECK_EQ_INT(g_diffScoreBonus, 77);
    CHECK_EQ_INT(g_bonusDuration, 777);
    CHECK_EQ_INT(g_fireDelayBiasA, 7);
}

// ---------------------------------------------------------------- SetupDifficulty

typedef struct Fixed {
    int diff;
    float speedMul;
    int bias, delayMin;
    float bullet;
} Fixed;

TEST(Profile_SetupDifficulty_fixed_tuning_for_each_difficulty)
{
    static const Fixed want[] = {
        {DIFF_EASY, 1.0f, 400, 300, 3.5f},
        {DIFF_NORMAL, 1.0f, 200, 200, 4.3f},
        {DIFF_HARD, 1.1666666f, -50, 190, 5.0f},
        {DIFF_ACE, 1.3333334f, -200, 180, 5.5f},
    };
    for (int i = 0; i < 4; i++) {
        g_cfg.difficulty = want[i].diff;
        g_diffHpBonusA = 9.0f;
        g_diffHpBonusB = 9.0f;
        g_gameSpeedMul = 0;
        SetupDifficulty();
        CHECK_MSG(g_gameSpeedMul == want[i].speedMul, "difficulty %d: speed %g", i, g_gameSpeedMul);
        CHECK_MSG(g_fireDelayBiasA == want[i].bias, "difficulty %d: biasA %d", i, g_fireDelayBiasA);
        CHECK_MSG(g_fireDelayBiasB == want[i].bias, "difficulty %d: biasB %d", i, g_fireDelayBiasB);
        CHECK_MSG(g_fireDelayMin == want[i].delayMin, "difficulty %d: delayMin %d", i, g_fireDelayMin);
        CHECK_MSG(g_enemyFireRateMin == want[i].delayMin, "difficulty %d: rateMin %d", i,
                  g_enemyFireRateMin);
        CHECK_MSG(g_enemyBulletSpeed == want[i].bullet, "difficulty %d: bullet %g", i,
                  g_enemyBulletSpeed);
        CHECK_NEAR(g_diffHpBonusA, 0, 0);
        CHECK_NEAR(g_diffHpBonusB, 0, 0);
    }
}

// The "rising" difficulty (anything past ACE) replays a milestone at levels 101, 201, ...
static void SetUpRising(int level)
{
    g_cfg.difficulty = NUM_DIFFICULTIES;
    g_curPlayer = 0;
    g_save.players[0].level = level;
    g_save.players[0].levelMilestoneHandled = 1;
    g_save.players[0].gameSpeedSetting = 60;
    g_diffScoreBonus = 3;
    g_fireDelayBiasA = 0;
    g_fireDelayBiasB = 10;
    g_enemyBulletSpeed = 4.0f;
    g_gameSpeedMul = 1.0f;
    g_diffHpBonusA = 9.0f;
    g_diffHpBonusB = 9.0f;
    g_bonusWeight[36] = 7;
}

TEST(Profile_SetupDifficulty_rising_replays_two_milestones_at_level_250)
{
    SetUpRising(250);
    SetupDifficulty();
    float bullet = 4.0f, speed = 1.0f;
    for (int i = 0; i < 2; i++) {
        bullet = bullet * (double)1.025f;
        speed = speed + (double)0.12f;
    }
    CHECK_EQ_INT(g_fireDelayBiasA, -100);
    CHECK_EQ_INT(g_fireDelayBiasB, -90);
    CHECK_NEAR(g_diffHpBonusA, 2.0, 0);
    CHECK_NEAR(g_diffHpBonusB, 10.0, 0);
    CHECK_NEAR(g_enemyBulletSpeed, bullet, 1e-6);
    CHECK_NEAR(g_gameSpeedMul, speed, 1e-6);
    CHECK_EQ_INT(g_save.players[0].gameSpeedSetting, 66);
    CHECK_EQ_INT(g_bonusWeight[36], 60);
    CHECK_EQ_INT(g_save.players[0].levelMilestoneHandled, 1);
}

TEST(Profile_SetupDifficulty_rising_milestone_boundary)
{
    // level 101: i runs to 100, so no milestone yet; the handled flag is cleared
    SetUpRising(101);
    SetupDifficulty();
    CHECK_EQ_INT(g_fireDelayBiasA, 0);
    CHECK_NEAR(g_diffHpBonusA, 0, 0);
    CHECK_NEAR(g_diffHpBonusB, 0, 0);
    CHECK_EQ_INT(g_bonusWeight[36], 7);
    CHECK_EQ_INT(g_save.players[0].levelMilestoneHandled, 0);
    CHECK_EQ_INT(g_save.players[0].gameSpeedSetting, 60);

    // level 102 reaches i = 101: one milestone
    SetUpRising(102);
    SetupDifficulty();
    CHECK_EQ_INT(g_fireDelayBiasA, -50);
    CHECK_NEAR(g_diffHpBonusA, 1.0, 0);
    CHECK_EQ_INT(g_save.players[0].gameSpeedSetting, 63);

    // low levels never count (i > 5)
    SetUpRising(5);
    SetupDifficulty();
    CHECK_EQ_INT(g_fireDelayBiasA, 0);
    CHECK_NEAR(g_diffHpBonusA, 0, 0);
}

TEST(Profile_SetupDifficulty_rising_bias_floor_is_minus_500)
{
    SetUpRising(1000);   // milestones at 101, 201, ..., 901: 9 of them
    g_fireDelayBiasA = -380;
    g_fireDelayBiasB = -480;
    SetupDifficulty();
    CHECK_EQ_INT(g_fireDelayBiasA, -500);
    CHECK_EQ_INT(g_fireDelayBiasB, -500);
    CHECK_NEAR(g_diffHpBonusA, 9.0, 0);
    CHECK_NEAR(g_diffHpBonusB, 45.0, 0);
}

// ---------------------------------------------------------------- WriteSettings / LoadSettings

TEST(Profile_WriteSettings_writes_whole_config_with_title)
{
    MakeGameDir();
    DefaultSettings();
    memset(g_cfg.title, 'x', sizeof g_cfg.title);
    WriteSettings();
    CHECK_STR(g_cfg.title, "Warblade v2.0 Information");
    CHECK_EQ_INT(HostFileSize(InfPath()), CFG_SIZE);

    Settings onDisk;
    FILE *f = fopen(InfPath(), "rb");
    CHECK(f != NULL);
    CHECK_EQ_INT(fread(&onDisk, 1, CFG_SIZE, f), CFG_SIZE);
    fclose(f);
    CHECK_MEM(&onDisk, &g_cfg, CFG_SIZE);
}

TEST(Profile_WriteSettings_LoadSettings_round_trip)
{
    MakeGameDir();
    Settings c = CustomSettings();
    g_cfg = c;
    WriteSettings();
    c = g_cfg;   // with the title stamped

    memset(&g_cfg, 0, sizeof g_cfg);
    g_presets = 1;
    g_windowed = 1;
    g_screenW = 1024;
    LoadSettings();
    CHECK_MEM(&g_cfg, &c, CFG_SIZE);
    CHECK_EQ_INT(g_presets, 0);
    CHECK_EQ_INT(g_windowed, 0);
    CHECK_EQ_INT(g_screenW, 800);
    CHECK_EQ_INT(g_maxBuffered, 9);
    CHECK_EQ_INT(g_maxSparks, 20);
}

TEST(Profile_LoadSettings_applies_volumes)
{
    Settings c = CustomSettings();
    WriteInf(&c);
    LoadSettings();
    CHECK_EQ_INT(g_sfxVolTable[255], 0x40);
    CHECK_EQ_INT(g_musVolTable[255], 0x80);
}

TEST(Profile_LoadSettings_missing_file_gives_defaults)
{
    MakeGameDir();
    memset(&g_cfg, 0x55, sizeof g_cfg);
    g_presets = 0;
    LoadSettings();
    CHECK_EQ_INT(g_presets, 1);
    CHECK_EQ_INT(g_cfg.version, 120);
    CHECK_EQ_INT(g_cfg.sfxVol, 0xcc);
    CHECK_EQ_INT(g_maxBuffered, 5);
    CHECK_EQ_INT(g_sfxVolTable[255], 0xcc);
    CHECK(!FakeFileExists(InfPath()));
}

TEST(Profile_LoadSettings_short_file_gives_defaults)
{
    Settings c = CustomSettings();
    MakeGameDir();
    WriteHostFile(InfPath(), &c, CFG_SIZE - 1);
    LoadSettings();
    CHECK_EQ_INT(g_presets, 1);
    CHECK_EQ_INT(g_cfg.sfxVol, 0xcc);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_NORMAL);
}

TEST(Profile_LoadSettings_other_version_resets_to_defaults)
{
    Settings c = CustomSettings();
    c.version = 119;
    WriteInf(&c);
    g_presets = 1;
    LoadSettings();
    CHECK_EQ_INT(g_cfg.version, 120);
    CHECK_EQ_INT(g_cfg.sfxVol, 0xcc);
    CHECK_EQ_INT(g_cfg.difficulty, DIFF_NORMAL);
    CHECK_EQ_INT(g_cfg.alienBuffer, 5);
    // unlike a missing file, the presets flag is off
    CHECK_EQ_INT(g_presets, 0);
    CHECK_EQ_INT(g_windowed, 1);
}

TEST(Profile_LoadSettings_clamps_fps_to_50_90)
{
    int cases[][2] = {{49, 60}, {50, 50}, {90, 90}, {91, 60}, {75, 75}};
    for (int i = 0; i < 5; i++) {
        Settings c = CustomSettings();
        c.fps = cases[i][0];
        WriteInf(&c);
        LoadSettings();
        CHECK_MSG(g_cfg.fps == cases[i][1], "fps %d loads as %d, expected %d", cases[i][0],
                  g_cfg.fps, cases[i][1]);
    }
}

TEST(Profile_LoadSettings_collision_detail_simple_or_normal)
{
    int cases[][2] = {{COLLISION_SIMPLE, COLLISION_SIMPLE}, {COLLISION_NORMAL, COLLISION_NORMAL},
                      {1, COLLISION_NORMAL}, {2, COLLISION_NORMAL}, {7, COLLISION_NORMAL}};
    for (int i = 0; i < 5; i++) {
        Settings c = CustomSettings();
        c.collisionDetail = cases[i][0];
        WriteInf(&c);
        LoadSettings();
        CHECK_MSG(g_cfg.collisionDetail == cases[i][1], "collision %d loads as %d", cases[i][0],
                  g_cfg.collisionDetail);
    }
}

TEST(Profile_LoadSettings_alien_buffer_at_least_5)
{
    Settings c = CustomSettings();
    c.alienBuffer = 4;
    WriteInf(&c);
    LoadSettings();
    CHECK_EQ_INT(g_cfg.alienBuffer, 5);
    CHECK_EQ_INT(g_maxBuffered, 5);

    c.alienBuffer = 6;
    WriteInf(&c);
    LoadSettings();
    CHECK_EQ_INT(g_cfg.alienBuffer, 6);
    CHECK_EQ_INT(g_maxBuffered, 6);
}

TEST(Profile_LoadSettings_max_sparks_is_half_at_least_5)
{
    Settings c = CustomSettings();
    c.sparks = 9.0f;
    WriteInf(&c);
    LoadSettings();
    CHECK_EQ_INT(g_maxSparks, 5);

    c.sparks = 13.0f;
    WriteInf(&c);
    LoadSettings();
    CHECK_EQ_INT(g_maxSparks, 6);
}
