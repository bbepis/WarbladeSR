// settings.c: Settings (WarBlade.inf), difficulty tuning.
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "globals.h"
#include "game.h"

enum {
    DEFAULT_SCREEN_W      = 800,
    DEFAULT_SCREEN_H      = 600,
    FPS_CLAMP_MIN         = 50,
    FPS_CLAMP_MAX         = 90,
    ALIEN_BUFFER_MIN      = 5,
    MIN_SPARKS            = 5,
    FIRE_DELAY_BIAS_FLOOR = -500,
    CFG_FILE_SIZE         = 0x638,  // sizeof(Config), WarBlade.inf's size
    CFG_TITLE_LEN         = 27      // Config::title[]
};


// Applies the fixed per-difficulty tuning (game speed, fire-rate bias/floor, enemy
// bullet speed) for difficulties 0-3. For the "auto/rising" difficulty (default case),
// instead replays the level-milestone ramp (every 100 levels past level 5) up to the
// current level to rebuild the cumulative bias/HP bonus/speed state.
void SetupDifficulty()
{
    int i = 0;
    float f = 0.0f;
    g_diffHpBonusA = 0.0f;
    g_diffHpBonusB = 0.0f;
    switch (g_cfg.difficulty) {
    case DIFF_EASY:
        g_gameSpeedMul = 1.0f;
        g_fireDelayBiasA = 400;
        g_fireDelayBiasB = 400;
        g_fireDelayMin = 300;
        g_enemyFireRateMin = 300;
        g_enemyBulletSpeed = 3.5f;
        break;

    case DIFF_NORMAL:
        g_gameSpeedMul = 1.0f;
        g_fireDelayBiasA = 200;
        g_fireDelayBiasB = 200;
        g_fireDelayMin = 200;
        g_enemyFireRateMin = 200;
        g_enemyBulletSpeed = 4.3f;
        break;

    case DIFF_HARD:
        g_gameSpeedMul = 1.1666666f;
        g_fireDelayBiasA = -50;
        g_fireDelayBiasB = -50;
        g_fireDelayMin = 190;
        g_enemyFireRateMin = 190;
        g_enemyBulletSpeed = 5.0f;
        break;

    case DIFF_ACE:
        g_gameSpeedMul = 1.3333334f;
        g_fireDelayBiasA = -200;
        g_fireDelayBiasB = -200;
        g_fireDelayMin = 180;
        g_enemyFireRateMin = 180;
        g_enemyBulletSpeed = 5.5f;
        break;

    default:
        // ---- replay the level-milestone ramp up to the current level ----
        g_save.players[g_curPlayer].levelMilestoneHandled = 0;
        do {
            if ((i - 1) % 100 == 0 && i > 5) {
                if (!g_save.players[g_curPlayer].levelMilestoneHandled) {
                    g_bonusWeight[36] = 60;
                    g_save.players[g_curPlayer].levelMilestoneHandled = 1;
                }

                g_fireDelayBiasA = g_fireDelayBiasA - 50;
                if (g_fireDelayBiasA < FIRE_DELAY_BIAS_FLOOR)
                    g_fireDelayBiasA = FIRE_DELAY_BIAS_FLOOR;
                g_fireDelayBiasB = g_fireDelayBiasB - 50;
                if (g_fireDelayBiasB < FIRE_DELAY_BIAS_FLOOR)
                    g_fireDelayBiasB = FIRE_DELAY_BIAS_FLOOR;
                g_diffHpBonusA = g_diffHpBonusA + 1.0;
                g_diffHpBonusB = g_diffHpBonusB + 5.0;
                g_enemyBulletSpeed = g_enemyBulletSpeed * (double)1.025f;
                g_gameSpeedMul = g_gameSpeedMul + (double)0.12f;
                g_save.players[g_curPlayer].gameSpeedSetting =
                    g_save.players[g_curPlayer].gameSpeedSetting + g_diffScoreBonus;
            }
            i++;
        } while (i < g_save.players[g_curPlayer].level);
        EmptyPostTransitionHook();
    }
}

// Sets every difficulty-scaled tuning global from g_cfg.difficulty:
// game speed, fire rates, enemy HP/speed bonuses, bonus spawn timing, etc.
void SetDifficulty()
{
    switch (g_cfg.difficulty) {
    case DIFF_EASY: // easy
        g_gameSpeedMul = 1.0f;
        g_diffScoreBonus = 2;
        g_speedMin = 15;

        g_fireDelayBiasA = 400;
        g_fireDelayBiasB = 400;
        g_fireDelayMin = 300;
        g_enemyFireRateMin = 300;
        g_enemyBulletSpeed = 3.5f;

        g_diffShotFuseBase = 200;
        g_diffShotFuseRange = 200;
        g_hurryUpInterval = 170;
        g_diffShotSpeedMin = 2.4f;
        g_diffShotSpeedMax = 3.2f;
        g_diffTurretTrackChance = 50.0f;

        g_speedBase = 4.2f;
        g_speedStep = 0.8f;

        g_bonusDuration = 50000;
        g_bonusSpawnRampRate = 0.0014f;
        g_bonusThresholdBase = 12090;
        g_bonusRareChance = 7;
        g_diffBonusDropRoll = 18.0f;

        g_diffEnemyHpBonus = 10;
        g_moneySuckerBaseHp = 300;
        g_hurryUpHpBonus = 75;
        g_diffHurryUpSpeedMax = 3.0f;
        g_eliteHpBonus = 1500;
        g_diffEnemyFireChance = 4;
        g_diffEnemyTimerMul = 3.0f;
        break;
    case DIFF_NORMAL: // normal
        g_gameSpeedMul = 1.0f;
        g_diffScoreBonus = 3;
        g_speedMin = 10;

        g_fireDelayBiasA = 200;
        g_fireDelayBiasB = 200;
        g_fireDelayMin = 200;
        g_enemyFireRateMin = 200;
        g_enemyBulletSpeed = 4.3f;

        g_diffShotFuseBase = 200;
        g_diffShotFuseRange = 225;
        g_hurryUpInterval = 130;
        g_diffShotSpeedMin = 3.1f;
        g_diffShotSpeedMax = 3.8f;
        g_diffTurretTrackChance = 40.0f;

        g_speedBase = 4.0f;
        g_speedStep = 0.7f;

        g_bonusDuration = 40000;
        g_bonusSpawnRampRate = 0.00145f;
        g_bonusThresholdBase = 14000;
        g_bonusRareChance = 6;
        g_diffBonusDropRoll = 28.0f;

        g_diffEnemyHpBonus = 16;
        g_moneySuckerBaseHp = 350;
        g_hurryUpHpBonus = 100;
        g_diffHurryUpSpeedMax = 4.0f;
        g_eliteHpBonus = 1750;
        g_diffEnemyFireChance = 6;
        g_diffEnemyTimerMul = 2.2f;
        break;
    case DIFF_HARD: // hard
        g_gameSpeedMul = 1.1666666f;
        g_diffScoreBonus = 3;
        g_speedMin = 5;

        g_fireDelayBiasA = -50;
        g_fireDelayBiasB = -50;
        g_fireDelayMin = 190;
        g_enemyFireRateMin = 190;
        g_enemyBulletSpeed = 5.0f;

        g_diffShotFuseBase = 210;
        g_diffShotFuseRange = 230;
        g_hurryUpInterval = 115;
        g_diffShotSpeedMin = 3.3f;
        g_diffShotSpeedMax = 4.3f;
        g_diffTurretTrackChance = 30.0f;

        g_speedBase = 3.5f;
        g_speedStep = 0.6f;

        g_bonusDuration = 30000;
        g_bonusSpawnRampRate = 0.0015f;
        g_bonusThresholdBase = 15540;
        g_bonusRareChance = 5;
        g_diffBonusDropRoll = 38.0f;

        g_diffEnemyHpBonus = 20;
        g_moneySuckerBaseHp = 450;
        g_hurryUpHpBonus = 125;
        g_diffHurryUpSpeedMax = 5.0f;
        g_eliteHpBonus = 2000;
        g_diffEnemyFireChance = 10;
        g_diffEnemyTimerMul = 2.0f;
        break;
    case DIFF_ACE: // ace (hardest; see g_gfxBorderAce)
        g_gameSpeedMul = 1.3333334f;
        g_diffScoreBonus = 2;
        g_speedMin = 5;

        g_fireDelayBiasA = -200;
        g_fireDelayBiasB = -200;
        g_fireDelayMin = 180;
        g_enemyFireRateMin = 180;
        g_enemyBulletSpeed = 5.5f;

        g_diffShotFuseBase = 220;
        g_diffShotFuseRange = 235;
        g_hurryUpInterval = 95;
        g_diffShotSpeedMin = 3.5f;
        g_diffShotSpeedMax = 4.8f;
        g_diffTurretTrackChance = 20.0f;

        g_speedBase = 3.0f;
        g_speedStep = 0.5f;

        g_bonusDuration = 20000;
        g_bonusSpawnRampRate = 0.0017f;
        g_bonusThresholdBase = 17000;
        g_bonusRareChance = 4;
        g_diffBonusDropRoll = 48.0f;

        g_diffEnemyHpBonus = 25;
        g_moneySuckerBaseHp = 600;
        g_hurryUpHpBonus = 150;
        g_diffHurryUpSpeedMax = 6.0f;
        g_eliteHpBonus = 2500;
        g_diffEnemyFireChance = 15;
        g_diffEnemyTimerMul = 1.8f;
        break;
    }
}

// Writes g_cfg (with its title field stamped) to WarBlade.inf in the user's data folder.
void WriteSettings()
{
    int fd;
    int res;

    fd = 0;
    char tit[CFG_TITLE_LEN] = "Warblade v2.0 Information";
    unsigned int i;
    char path[512];
    _set_fmode(_O_BINARY);
    sprintf(path, "%s\\warblade\\WarBlade.inf", SysUserFolder());
    fd = _open(path, _O_CREAT | _O_RDWR, _S_IREAD | _S_IWRITE);
    if (fd != -1)
    {
        for (i = 0; i < CFG_TITLE_LEN; i++)
            g_cfg.title[i] = tit[i];
        res = _write(fd, &g_cfg, CFG_FILE_SIZE);
        if (res == -1)
        {
            g_fileWriteErrorFlag = 1;
            SysMessageBox("ERROR", "Could not open/create WarBlade information file");
            g_fileWriteErrorFlag = 0;
        }
        _close(fd);
    }
}

// Resets g_cfg to factory-default settings: video/audio, both players' key/joystick
// bindings, difficulty, screen size, and misc gameplay tunables.
void DefaultSettings()
{
    g_cfg.version = g_version;
    g_presets = 1;
    g_cfg.borderMode = BORDER_ON;
    g_cfg.sfxVol = 0xcc;
    g_cfg.musicVolume = 0xb3;
    g_cfg.musicVol = 0xff;
    g_cfg.unused030 = 0;
    g_cfg.fps = 0x3c;
    g_cfg.musicFormat = MUSIC_FMT_MOD;
    g_cfg.numStars = 600.0f;
    g_cfg.sfxOn = 1;
    g_cfg.unused050 = 1;
    g_cfg.unused05c = 0;
    g_cfg.collisionDetail = COLLISION_NORMAL;
    g_cfg.bgStars = 1;
    g_cfg.unused068 = 1;
    g_cfg.bulletIntensity = BULLETS_NORMAL;
    g_cfg.particlesOn = 1;
    g_cfg.best = 0;

    // ---- player 1's key/joystick bindings ----
    g_cfg.device[0] = DEVICE_KEYBOARD;
    g_cfg.left[0] = 0;
    g_cfg.right[0] = 3;
    g_cfg.up[0] = 1;
    g_cfg.down[0] = 2;
    g_cfg.fire[0] = 9;
    g_cfg.rocket[0] = 5;
    g_cfg.pause[0] = 0x28;
    g_cfg.profile[0] = 0x17;
    g_cfg.key6[0] = 0x30;
    g_cfg.joyFire[0] = 0;
    g_cfg.joyRocket[0] = 1;
    g_cfg.joyBtn2[0] = 2;
    g_cfg.joyPause[0] = 3;
    g_cfg.joyProfile[0] = 4;

    // ---- player 2's key/joystick bindings ----
    g_cfg.device[1] = DEVICE_KEYBOARD;
    g_cfg.left[1] = 0x41;
    g_cfg.right[1] = 0x43;
    g_cfg.up[1] = 0x45;
    g_cfg.down[1] = 0x3f;
    g_cfg.fire[1] = 8;
    g_cfg.rocket[1] = 6;
    g_cfg.pause[1] = 0x28;
    g_cfg.profile[1] = 0x17;
    g_cfg.key6[1] = 7;
    g_cfg.joyFire[1] = 0;
    g_cfg.joyRocket[1] = 1;
    g_cfg.joyBtn2[1] = 2;
    g_cfg.joyPause[1] = 3;
    g_cfg.joyProfile[1] = 4;

    // ---- difficulty, screen size, misc gameplay tunables ----
    g_cfg.difficulty = DIFF_NORMAL;
    SetDifficulty();
    g_screenW = DEFAULT_SCREEN_W;
    g_screenH = DEFAULT_SCREEN_H;
    g_cfg.fps = 0x3c;
    DoNothing();
    g_cfg.bgEnabled = 0;
    g_cfg.bgTint = 0x37;
    g_cfg.sparks = 20.0f;
    g_maxSparks = ((int)g_cfg.sparks >> 1 < MIN_SPARKS) ? MIN_SPARKS : (int)g_cfg.sparks >> 1;
    g_cfg.profileSel = -1;
    g_cfg.voice = 1;
    g_cfg.shuffleByte = 1;
    g_cfg.netMode = 0;

    // ---- unused/reserved fields and remaining flags ----
    g_cfg.unusedStr1b6[0] = 0;
    g_cfg.unusedStr1f5[0][0] = 0;
    g_cfg.unusedStr1f5[1][0] = 0;
    g_cfg.unusedStr1f5[2][0] = 0;
    g_cfg.unusedStr1f5[3][0] = 0;
    g_cfg.unusedF14c[0] = 0;
    g_cfg.unusedF14c[1] = 0;
    g_cfg.unusedF14c[2] = 0;
    g_cfg.unusedF14c[3] = 0;
    g_cfg.unusedF14c[4] = 0;
    g_cfg.checkVersion = 1;
    g_cfg.alienBuffer = ALIEN_BUFFER_MIN;
    g_cfg.renderer = RENDERER_AUTO;
    g_cfg.vsyncOff = 0;
    g_cfg.interpolation = SYS_INTERP_AUTO;
    g_cfg.windowedByte = 1;
    g_cfg.musicFormat = MUSIC_FMT_MP3;
}

// Loads g_cfg from WarBlade.inf; resets to defaults if the file is missing, the wrong
// size, or from an older game version (also re-clamping fps/screen size/collision
// detail in that case). Applies the loaded audio/sparks settings afterward.
void LoadSettings()
{
    int fd;
    int n;
    char path[512];

    fd = 0;
    n = 0;
    g_cfg.numStars = 500.0f;
    _set_fmode(_O_BINARY);
    sprintf(path, "%s\\warblade\\WarBlade.inf", SysUserFolder());
    fd = _open(path, 0, 0);
    if (fd != -1)
    {
        n = _read(fd, &g_cfg, CFG_FILE_SIZE);
        _close(fd);
    }
    if (n != CFG_FILE_SIZE)
        DefaultSettings();
    else if (g_cfg.version != g_version)
    {
        DefaultSettings();
        g_presets = 0;
        if (g_cfg.fps < FPS_CLAMP_MIN || g_cfg.fps > FPS_CLAMP_MAX)
            g_cfg.fps = 0x3c;
        g_screenW = DEFAULT_SCREEN_W;
        g_screenH = DEFAULT_SCREEN_H;

        g_windowed = g_cfg.windowedByte;

        if (g_cfg.collisionDetail == COLLISION_SIMPLE || g_cfg.collisionDetail == COLLISION_NORMAL)
        {
        }
        else
            g_cfg.collisionDetail = COLLISION_NORMAL;
    }
    else
    {
        // NOTE: same reclamping as the "old version" branch above, duplicated rather than shared.
        g_presets = 0;
        if (g_cfg.fps < FPS_CLAMP_MIN || g_cfg.fps > FPS_CLAMP_MAX)
            g_cfg.fps = 0x3c;
        g_screenW = DEFAULT_SCREEN_W;
        g_screenH = DEFAULT_SCREEN_H;
        g_windowed = g_cfg.windowedByte;

        if (g_cfg.collisionDetail == COLLISION_SIMPLE || g_cfg.collisionDetail == COLLISION_NORMAL)
        {
        }
        else
            g_cfg.collisionDetail = COLLISION_NORMAL;
    }

    if (g_cfg.alienBuffer < ALIEN_BUFFER_MIN)
        g_cfg.alienBuffer = ALIEN_BUFFER_MIN;
    g_maxBuffered = g_cfg.alienBuffer;
    SetMusicVolTable(g_cfg.musicVol);
    SetSfxVolume(g_cfg.sfxVol);
    ApplyMusicVolume();
    g_maxSparks = ((int)g_cfg.sparks >> 1 < MIN_SPARKS) ? MIN_SPARKS : (int)g_cfg.sparks >> 1;
}
