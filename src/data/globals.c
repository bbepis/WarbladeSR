// The definition of every game global, first generated from include/globals.h and the
// initial values in warblade.exe; on sdl_base it is edited by hand (tools/rmglobal.py).
//
// In address order, each in a section named after its address (.data$g<addr>,
// .bss$g<addr>): the linker sorts grouped sections by name, so linked with
// /INCREMENTAL:NO the game's data keeps the exe's order.  Nothing relies on that order any
// more: the suspended game and the hiscore file have explicit formats (savefile.c,
// hiscore.c), and arrays the original read past their end are defined at their full size.
// (A 64-bit build pads and aligns differently anyway.)
// Bytes that no game code names are kept as g_unref_<addr> (non-zero) and g_pad_<addr> (zero).
// The globals with dynamic initializers are defined in static_init.c, in the same
// kind of section.
#include "globals.h"
#include "game.h"

#pragma data_seg(".data$g7cd000")
_Alignas(16) unsigned char g_unref_7cd000[1] = {1};   // not referenced by game code
#pragma data_seg(".data$g7cd001")
char g_hiscoresCleared = 1;
#pragma data_seg(".data$g7cd002")
unsigned char g_introInit = 1;
#pragma data_seg(".data$g7cd004")
int g_endPic = 1;
#pragma data_seg(".data$g7cd008")
int g_unref_7cd008[3] = {-1, -1, -1};   // not referenced by game code
#pragma data_seg(".data$g7cd014")
int g_aiTimer = 5;
#pragma data_seg(".data$g7cd018")
int g_unref_7cd018[1] = {10};   // not referenced by game code
#pragma data_seg(".data$g7cd01c")
int g_viewTransitionFlag = 10;
#pragma data_seg(".data$g7cd020")
int g_unref_7cd020[1] = {4};   // not referenced by game code
#pragma data_seg(".data$g7cd024")
int g_shopEntryFlag = 100;
#pragma data_seg(".data$g7cd028")
float g_joyCenter = 32767.0f;
#pragma data_seg(".data$g7cd02c")
unsigned char g_pad_7cd02c[1] = {0};
#pragma data_seg(".data$g7cd02d")
unsigned char g_unref_7cd02d = 255;
#pragma data_seg(".data$g7cd02e")
unsigned char g_unref_7cd02e = 127;
#pragma data_seg(".data$g7cd02f")
unsigned char g_unref_7cd02f = 71;
#pragma data_seg(".data$g7cd030")
float g_joyDead = 8000.0f;
#pragma data_seg(".data$g7cd034")
float g_joystickSpeedMul = 1.0f;
#pragma data_seg(".data$g7cd038")
int g_prevMouseX = 400;
#pragma data_seg(".data$g7cd03c")
int g_prevMouseY = 300;
#pragma data_seg(".data$g7cd040")
int g_unref_7cd040[2] = {-1, -1};   // not referenced by game code
#pragma data_seg(".data$g7cd048")
int g_mouseX = 400;
#pragma data_seg(".data$g7cd04c")
int g_mouseY = 300;
#pragma data_seg(".data$g7cd050")
int g_unref_7cd050[2] = {3, 2};   // not referenced by game code
#pragma data_seg(".data$g7cd058")
int g_clicked = -1;
#pragma data_seg(".data$g7cd05c")
int g_pressed = -1;
#pragma data_seg(".data$g7cd060")
int g_shopHoverItem = -1;
#pragma data_seg(".data$g7cd064")
int g_unref_7cd064[1] = {32};   // not referenced by game code
#pragma data_seg(".data$g7cd068")
int g_bgIndex = 4;
#pragma data_seg(".data$g7cd06c")
float g_angle1 = 130.0f;
#pragma data_seg(".data$g7cd070")
float g_angle2 = 241.0f;
#pragma data_seg(".data$g7cd074")
int g_endSequenceActive = 5;
#pragma data_seg(".data$g7cd078")
int g_unref_7cd078[8] = {1, 0, 1, 1065353216, 14, -1, 1, 1};   // not referenced by game code
#pragma data_seg(".data$g7cd098")
unsigned int g_animInterval = 90u;
#pragma data_seg(".data$g7cd09c")
int g_pulseHoldCount = 10;
#pragma data_seg(".data$g7cd0a0")
int g_pulseCycleTimer = 100;
#pragma data_seg(".data$g7cd0a4")
float g_offX = 800.0f;
#pragma data_seg(".data$g7cd0a8")
float g_shopSlideVelX = 40.0f;
#pragma data_seg(".data$g7cd0ac")
float g_offY = -600.0f;
#pragma data_seg(".data$g7cd0b0")
float g_shopSlideVelY = 40.0f;
#pragma data_seg(".data$g7cd0b4")
float g_shopBounceY = -30.0f;
#pragma data_seg(".data$g7cd0b8")
int g_unref_7cd0b8[1] = {1045220557};   // not referenced by game code
#pragma data_seg(".data$g7cd0bc")
int g_grabZoneHit = 1;
#pragma data_seg(".data$g7cd0c0")
unsigned int g_blinkRate = 450u;
#pragma data_seg(".data$g7cd0c4")
unsigned int g_cursorBlinkRate = 50u;
#pragma data_seg(".data$g7cd0c8")
int g_textAutoY = 10000;
#pragma data_seg(".data$g7cd0cc")
int g_unref_7cd0cc[2] = {400, 1};   // not referenced by game code
#pragma data_seg(".data$g7cd0d4")
float g_hiscoreScrollX = 100.0f;
#pragma data_seg(".data$g7cd0d8")
float g_hiscoreScrollY = 1.0f;
#pragma data_seg(".data$g7cd0dc")
int g_logoSplashTimer = 3000;
#pragma data_seg(".data$g7cd0e0")
unsigned int g_idleTimeoutMs = 60000u;
#pragma data_seg(".data$g7cd0e4")
int g_screenDelayMs = 30000;
#pragma data_seg(".data$g7cd0e8")
int g_unref_7cd0e8[2] = {1, 1};   // not referenced by game code
#pragma data_seg(".data$g7cd0f0")
float g_chargeMax = 15.0f;
#pragma data_seg(".data$g7cd0f4")
float g_chargeRate = 0.1f;
#pragma data_seg(".data$g7cd0f8")
float g_dischargeRate = 0.18f;
#pragma data_seg(".data$g7cd0fc")
int g_unref_7cd0fc[1] = {1050253722};   // not referenced by game code
#pragma data_seg(".data$g7cd10c")
float g_starPulseAlphaStep = 2.0f;
#pragma data_seg(".data$g7cd110")
int g_objSrcX[70] = {32, 48, 32, 48, 80, 80, 32, 64, 48, 112, 48, 112, 32, 64, 96, 128, 96, 128, 176, 0, 144, 144, 240, 256, 272, 304, 336, 368, 400, 432, 304, 320, 336, 352, 368, 176, 192, 208, 80, 112, 128, 128, 48, 32, 64, 96, 128, 48, 32, 208, 288, 464, 512, 560, 608, 32, 48, 224, 304, 320, 336, 352, 368, 80, 80, 80, 48, 464, 496, 50};
#pragma data_seg(".data$g7cd228")
int g_objSrcY[70] = {0, 0, 15, 15, 0, 15, 31, 31, 31, 0, 46, 15, 46, 46, 0, 0, 15, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 52, 52, 52, 52, 52, 46, 46, 46, 46, 46, 46, 46, 76, 76, 76, 31, 31, 61, 61, 0, 0, 0, 0, 0, 0, 68, 68, 46, 52, 52, 52, 52, 52, 0, 0, 0, 0, 0, 0, 255};
#pragma data_seg(".data$g7cd340")
int g_objW[70] = {4, 4, 4, 4, 6, 6, 10, 10, 8, 4, 8, 4, 9, 9, 8, 8, 8, 8, 22, 32, 32, 32, 16, 16, 16, 21, 21, 21, 22, 22, 11, 11, 11, 11, 11, 8, 8, 8, 11, 16, 16, 16, 4, 5, 5, 4, 4, 12, 2, 22, 16, 48, 48, 48, 48, 2, 12, 8, 11, 11, 11, 11, 11, 6, 6, 6, 4, 26, 26, 50};
#pragma data_seg(".data$g7cd458")
int g_objH[70] = {10, 10, 10, 10, 10, 10, 12, 12, 12, 10, 12, 10, 11, 12, 10, 10, 10, 10, 41, 78, 78, 78, 100, 100, 100, 50, 51, 50, 52, 52, 25, 25, 25, 25, 25, 50, 50, 50, 20, 39, 39, 39, 6, 6, 6, 5, 5, 5, 5, 41, 100, 66, 66, 66, 66, 5, 5, 50, 25, 25, 25, 25, 25, 10, 10, 10, 10, 68, 68, 2};
#pragma data_seg(".data$g7cd570")
float g_debrisVySpeedFactor[69] = {-5.5f, -5.5f, -5.5f, -5.5f, -7.6f, -6.5f, -6.5f, -6.5f, -7.5f, -6.5f, -6.5f, -6.5f, -5.5f, -5.5f, -5.5f, -5.5f, -5.5f, -5.5f, -19.0f, -25.0f, -25.0f, -25.0f, 0, 0, 0, -7.0f, -8.0f, -6.0f, -7.0f, -8.0f, -8.0f, -9.0f, -6.0f, -6.0f, -6.0f, 0, 0, -22.0f, -23.0f, -24.0f, -24.4f, -24.3f, -6.0f, -5.0f, -5.0f, -5.0f, -5.0f, -4.0f, -4.0f, -19.0f, -6.0f, 0, 0, 0, 0, -4.0f, -4.0f, 0, -8.0f, -7.0f, -7.0f, -7.0f, -7.0f, -7.5f, -7.6f, -7.5f, -5.5f, -20.0f, -20.0f};
#pragma data_seg(".data$g7cd684")
float g_starXMin = -5000.0f;
#pragma data_seg(".data$g7cd688")
float g_debrisVxFactor[69] = {0, 0, 0, 0, 0.06f, 0, -2.0f, 2.0f, 0, 0, 0, 0, -2.0f, 2.0f, -1.0f, 1.0f, -2.0f, 2.0f, 0, 165.0f, 163.0f, 163.0f, 0, 0, 0, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 190.0f, 190.0f, 190.0f, 190.0f, 190.0f, 0, 0, 165.0f, 165.0f, 165.0f, 163.0f, 163.0f, 0, -1.0f, 1.0f, -1.0f, 1.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 200.0f, 200.0f, 200.0f, 200.0f, 200.0f, 0.2f, -0.06f, -0.2f};
#pragma data_seg(".data$g7cd79c")
float g_starXMax = 5000.0f;
#pragma data_seg(".data$g7cd7a0")
int g_debrisLinkType1[69] = {0, 66, 0, 0, 63, 0, 0, 0, 6, 14, 12, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 0, 26, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 40, 0, 0, 43, 0, 0, 46, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65};
#pragma data_seg(".data$g7cd8b4")
float g_starYMin = -5000.0f;
#pragma data_seg(".data$g7cd8b8")
int g_debrisLinkType2[69] = {0, 0, 0, 0, 64, 0, 0, 0, 7, 15, 13, 0, 0, 0, 0, 0, 0, 0, 0, 21, 0, 0, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 0, 0, 44};
#pragma data_seg(".data$g7cd9cc")
float g_starYMax = 5000.0f;
#pragma data_seg(".data$g7cd9d0")
int g_debrisYOffset[69] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 5, 5, 5, 86, 86, 86, 29, 29, 29, 29, 29, 15, 15, 15, 15, 15, 50, 50, 50, 10, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 20, 86, 70, 70, 70, 70, 0, 0, 50, 10, 10, 10, 10, 10, -2, 0, -2};
#pragma data_seg(".data$g7cdae4")
float g_starZFar = 2000.0f;
#pragma data_seg(".data$g7cdae8")
int g_debrisXOffset[70] = {0, -8, 0, 0, -5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, 5, 15, 8, 0, 0, 1047233823};
#pragma data_seg(".data$g7cdc00")
int g_debrisNextType[69] = {2, 3, 0, 1, 5, 4, 12, 13, 10, 11, 8, 9, 6, 7, 16, 17, 14, 15, 49, 20, 19, 19, 23, 24, 50, 26, 27, 28, 29, 25, 31, 32, 33, 34, 30, 36, 37, 57, 38, 40, 39, 39, 42, 43, 44, 45, 46, 56, 55, 18, -1, 52, 53, 54, -1, 48, 47, -1, 59, 60, 61, 62, 58, 5, 5, 5, 3, 68, 67};
#pragma data_seg(".data$g7cdd14")
float g_bulletSpeedMax = 2.0f;
#pragma data_seg(".data$g7cdd18")
unsigned char g_pad_7cdd18[120] = {0};
#pragma data_seg(".data$g7cdd90")
int g_unref_7cdd90[5] = {1, 1, 1, 1, 1};   // not referenced by game code
#pragma data_seg(".data$g7cdda4")
unsigned char g_pad_7cdda4[4] = {0};
#pragma data_seg(".data$g7cdda8")
unsigned char g_pad_7cdda8[132] = {0};
#pragma data_seg(".data$g7cde2c")
int g_levelEnemyColorR = 255;
#pragma data_seg(".data$g7cde30")
int g_debrisLaserFlag[69] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1};
#pragma data_seg(".data$g7cdf44")
int g_levelEnemyColorG = 255;
#pragma data_seg(".data$g7cdf48")
int g_debrisParticleSize[69] = {50, 50, 1, 1, 40, 1, 1, 1, 70, 60, 1, 1, 1, 1, 1, 1, 1, 1, 150, 150, 1, 1, 120, 110, 100, 80, 70, 60, 1, 1, 50, 50, 1, 1, 1, 80, 75, 70, 80, 100, 1, 1, 40, 1, 1, 40, 1, 35, 30, 1, 1, 1, 1, 1, 1, 1, 1, 1, 60, 1, 1, 1, 1, 40, 40, 40, 50, 150, 150};
#pragma data_seg(".data$g7ce05c")
int g_levelEnemyColorB = 255;
#pragma data_seg(".data$g7ce060")
int g_debrisParticleColorR[69] = {255, 0, 1, 1, 255, 1, 1, 1, 255, 64, 1, 1, 1, 1, 1, 1, 1, 1, 255, 255, 1, 1, 128, 128, 128, 255, 255, 255, 1, 1, 255, 255, 1, 1, 1, 128, 128, 128, 255, 255, 1, 1, 255, 1, 1, 64, 1, 0, 255, 1, 1, 1, 1, 1, 1, 1, 1, 1, 255, 1, 1, 1, 1, 255, 255, 255, 0, 80, 80};
#pragma data_seg(".data$g7ce174")
float g_shopTransition = 500.0f;
#pragma data_seg(".data$g7ce178")
int g_debrisParticleColorG[70] = {255, 255, 1, 1, 200, 1, 1, 1, 90, 255, 1, 1, 1, 1, 1, 1, 1, 1, 100, 255, 1, 1, 255, 255, 255, 255, 230, 128, 1, 1, 240, 200, 1, 1, 1, 255, 255, 255, 100, 255, 1, 1, 80, 1, 1, 255, 1, 255, 255, 1, 1, 1, 1, 1, 1, 1, 1, 1, 200, 1, 1, 1, 1, 200, 200, 200, 255, 255, 255, 50};
#pragma data_seg(".data$g7ce290")
int g_debrisParticleColorB[69] = {255, 0, 1, 1, 50, 1, 1, 1, 255, 255, 1, 1, 1, 1, 1, 1, 1, 1, 200, 255, 1, 1, 255, 255, 255, 20, 0, 0, 1, 1, 0, 64, 1, 1, 1, 255, 255, 255, 200, 255, 1, 1, 255, 1, 1, 255, 1, 0, 255, 1, 1, 1, 1, 1, 1, 1, 1, 1, 20, 1, 1, 1, 1, 50, 50, 50, 0, 80, 80};
#pragma data_seg(".data$g7ce3a4")
int g_demoSteer = 50;
#pragma data_seg(".data$g7ce3a8")
int g_debrisParticleAlpha[69] = {255, 300, 1, 1, 255, 1, 1, 1, 255, 500, 1, 1, 1, 1, 1, 1, 1, 1, 255, 255, 1, 1, 400, 300, 200, 300, 255, 255, 1, 1, 255, 200, 1, 1, 1, 400, 300, 200, 255, 255, 1, 1, 300, 1, 1, 300, 1, 200, 255, 1, 1, 1, 1, 1, 1, 1, 1, 1, 255, 1, 1, 1, 1, 255, 255, 255, 300, 400, 400};
#pragma data_seg(".data$g7ce4bc")
float g_starSpeedX = 10.0f;
#pragma data_seg(".data$g7ce4c0")
int g_unref_7ce4c0[69] = {100, 100, 1, 1, 80, 1, 1, 1, 60, 100, 1, 1, 1, 1, 1, 1, 1, 1, 90, 80, 1, 1, 90, 90, 90, 90, 80, 90, 1, 1, 100, 80, 1, 1, 1, 90, 90, 90, 90, 80, 1, 1, 90, 1, 1, 90, 1, 90, 100, 1, 1, 1, 1, 1, 1, 1, 1, 1, 80, 1, 1, 1, 1, 80, 80, 80, 100, 100, 100};   // not referenced by game code
#pragma data_seg(".data$g7ce5d4")
float g_starSpeedY = 10.0f;
#pragma data_seg(".data$g7ce5d8")
int g_objR[69] = {255, 0, 255, 64, 255, 255, 255, 255, 255, 0, 255, 0, 255, 255, 0, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255, 0, 0, 0, 255, 255, 255, 0, 0, 0, 255, 255, 0, 1, 1, 1, 1, 1, 1, 0, 255, 255, 255, 200, 255, 255, 255, 255, 0, 0, 80};
#pragma data_seg(".data$g7ce6ec")
float g_starSpeedZ = 10.0f;
#pragma data_seg(".data$g7ce6f0")
int g_objG[69] = {128, 255, 0, 255, 0, 0, 0, 0, 0, 128, 64, 0, 64, 64, 128, 128, 0, 0, 0, 255, 200, 128, 0, 0, 0, 128, 200, 128, 200, 0, 120, 60, 0, 100, 0, 0, 0, 0, 0, 128, 255, 200, 0, 0, 0, 0, 0, 255, 128, 64, 128, 1, 1, 1, 1, 1, 1, 0, 128, 64, 0, 0, 200, 0, 0, 0, 255, 255, 255};
#pragma data_seg(".data$g7ce804")
unsigned int g_tallyDelay = 1000u;
#pragma data_seg(".data$g7ce808")
int g_objB[69] = {0, 0, 0, 64, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255, 0, 40, 0, 0, 0, 0, 0, 0, 64, 0, 255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 1, 1, 1, 1, 1, 1, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 80};
#pragma data_seg(".data$g7ce91c")
int g_guardCount = 1;
#pragma data_seg(".data$g7ce920")
int g_objFlameOn[69] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
#pragma data_seg(".data$g7cea34")
int g_bonusIconFrame1 = 1;
#pragma data_seg(".data$g7cea38")
int g_bonusIconFrame2 = 2;
#pragma data_seg(".data$g7cea3c")
int g_bonusIconFrame3 = 3;
#pragma data_seg(".data$g7cea40")
int g_bonusIconFrame4 = 4;
#pragma data_seg(".data$g7cea44")
int g_bonusIconFrame5 = 5;
#pragma data_seg(".data$g7cea48")
int g_objFlameGfx[74] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2, 3, 4, 5};
#pragma data_seg(".data$g7ceb70")
int g_objFlameAngle[69] = {0, 0, 0, 0, 2, 0, 340, 20, 0, 0, 0, 0, 340, 20, 345, 15, 345, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 345, 15, 350, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 358, 356};
#pragma data_seg(".data$g7cec84")
float g_bonusIconAnimTimer = 4.0f;
#pragma data_seg(".data$g7cec88")
int g_objFlameHalfW[70] = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 48, 48, 48, 64, 64, 64, 64, 64, 48, 48, 48, 48, 48, 24, 24, 24, 32, 32, 32, 32, 32, 32, 32, 32, 32, 64, 32, 64, 48, 64, 64, 64, 64, 64, 64, 24, 32, 32, 32, 32, 32, 64, 64, 64, 64, 80, 80, 1};
#pragma data_seg(".data$g7ceda0")
int g_objFlameHalfH[70] = {24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 64, 80, 80, 80, 400, 400, 400, 64, 64, 64, 64, 64, 48, 48, 48, 48, 48, 400, 400, 400, 32, 32, 32, 32, 20, 20, 20, 20, 20, 24, 20, 64, 400, 24, 24, 24, 24, 24, 24, 400, 32, 32, 32, 32, 32, 24, 24, 24, 24, 90, 90, 2};
#pragma data_seg(".data$g7ceeb8")
int g_objFlameOffY[69] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 10, -8, -8, -8, -350, -350, -350, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, -350, -350, -350, 0, 0, 0, 0, 2, 2, 2, 2, 2, 4, 2, 10, -350, 4, 4, 4, 4, 4, 4, -350, 0, 0, 0, 0, 0, 4, 4, 4, 4};
#pragma data_seg(".data$g7cefcc")
int g_malfunctionBeepCount = 3;
#pragma data_seg(".data$g7cefd0")
int g_shotType[18] = {0, 1, 9, 4, 8, 18, 25, 22, 19, 67, 35, 38, 39, 42, 45, 47, 48, 58};
#pragma data_seg(".data$g7cf018")
float g_shotSpeed[18] = {1.0f, 2.0f, 3.0f, 2.5f, 4.0f, 5.5f, 6.0f, 10.0f, 15.0f, 3.0f, 5.0f, 3.0f, 5.0f, 4.0f, 1.0f, 1.0f, 1.0f, 3.0f};
#pragma data_seg(".data$g7cf060")
int g_explSrcX[18] = {0, 45, 90, 135, 180, 225, 270, 315, 360, 405, 450, 495, 540, 585};
#pragma data_seg(".data$g7cf0a8")
int g_explW[18] = {45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45};
#pragma data_seg(".data$g7cf0f0")
int g_explH[18] = {45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45};
#pragma data_seg(".data$g7cf138")
int g_expl2SrcX[20] = {0, 32, 64, 96, 128, 0, 32, 64, 96, 128, 0, 32, 64, 96, 128, 0, 32, 64, 96, 128};
#pragma data_seg(".data$g7cf188")
int g_expl2SrcY[391] = {0, 0, 0, 0, 0, 32, 32, 32, 32, 32, 64, 64, 64, 64, 64, 128, 128, 128, 128, 128, 1, 1, -2, 0, -1, -2, 0, 1, 1, -2, 1, 1, -2, 2, 0, -1, -2, 2, 0, 2, -2, 0, 1, -2, 2, -1, -1, -1, 2, 1, 0, -1, 0, 2, -1, 2, 2, 0, 3, 1, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 34, 60, 84, 114, 138, 162, 190, 213, 224, 243, 267, 289, 327, 354, 390, 416, 449, 475, 499, 522, 550, 583, 1, 30, 59, 85, 114, 131, 156, 181, 208, 232, 260, 285, 314, 342, 354, 365, 376, 397, 410, 438, 463, 491, 1101004800, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35};
#pragma data_seg(".data$g7cf7a4")
float g_tiltStep = 0.5f;
#pragma data_seg(".data$g7cf7a8")
int g_unref_7cf7a8[45] = {34, 26, 24, 30, 24, 25, 29, 23, 12, 19, 24, 23, 38, 28, 36, 26, 34, 25, 23, 24, 29, 34, 42, 30, 29, 26, 29, 18, 25, 25, 27, 24, 29, 25, 30, 29, 10, 10, 11, 21, 10, 26, 24, 26, 25};   // not referenced by game code
#pragma data_seg(".data$g7cf85c")
char g_perfect[33] = "PERFECT BONUS     10.000  POINTS";
#pragma data_seg(".data$g7cf87d")
bool g_meterDirUp = true;
#pragma data_seg(".data$g7cf87e")
unsigned char g_windowRenderer = 1;
#pragma data_seg(".data$g7cf87f")
unsigned char g_unref_7cf87f = 153;
#pragma data_seg(".data$g7cf880")
int g_unref_7cf880[45] = {34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 34, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38};   // not referenced by game code
#pragma data_seg(".data$g7cf934")
char g_perfectBlank[11] = "    10.000";
#pragma data_seg(".data$g7cf93f")
unsigned char g_unref_7cf93f = 213;
#pragma data_seg(".data$g7cf940")
unsigned char g_pad_7cf940[4] = {0};
#pragma data_seg(".data$g7cf944")
int g_unref_7cf944[1] = {16};   // not referenced by game code
#pragma data_seg(".data$g7cf948")
int g_unref_7cf948[43] = {29, 41, 56, 68, 80, 94, 105, 111, 120, 132, 144, 162, 176, 194, 207, 224, 237, 249, 260, 274, 291, 312, 326, 341, 353, 368, 377, 389, 402, 415, 427, 441, 454, 468, 483, 489, 495, 501, 511, 517, 531, 544, 558};   // not referenced by game code
#pragma data_seg(".data$g7cf9f4")
int g_secretPicCur = -1;
#pragma data_seg(".data$g7cf9f8")
int g_unref_7cf9f8[45] = {75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75};   // not referenced by game code
#pragma data_seg(".data$g7cfaac")
int g_secretPicPrev = -1;
#pragma data_seg(".data$g7cfab0")
int g_unref_7cfab0[45] = {17, 14, 13, 16, 13, 13, 15, 12, 7, 10, 13, 13, 19, 15, 19, 14, 18, 13, 13, 12, 15, 17, 21, 15, 15, 13, 16, 10, 13, 14, 14, 13, 15, 13, 15, 15, 7, 7, 7, 11, 6, 14, 14, 14, 13};   // not referenced by game code
#pragma data_seg(".data$g7cfb64")
int g_secretPic = -1;
#pragma data_seg(".data$g7cfb68")
int g_unref_7cfb68[45] = {18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19};   // not referenced by game code
#pragma data_seg(".data$g7cfc1c")
int g_shopPic = -1;
#pragma data_seg(".data$g7cfc20")
unsigned char g_pad_7cfc20[4] = {0};
#pragma data_seg(".data$g7cfc24")
int g_unref_7cfc24[1] = {16};   // not referenced by game code
#pragma data_seg(".data$g7cfc28")
int g_unref_7cfc28[89] = {29, 41, 56, 68, 80, 94, 105, 111, 120, 132, 144, 162, 176, 194, 207, 224, 237, 249, 260, 274, 291, 312, 326, 341, 353, 368, 377, 389, 402, 415, 427, 441, 454, 468, 483, 489, 495, 501, 511, 517, 531, 544, 558, -1, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96};   // not referenced by game code
#pragma data_seg(".data$g7cfd8c")
int g_secretPicPick = -1;
#pragma data_seg(".data$g7cfd90")
int g_unref_7cfd90[45] = {17, 14, 13, 16, 13, 13, 15, 12, 7, 10, 13, 13, 19, 15, 19, 14, 18, 13, 13, 12, 15, 17, 21, 15, 15, 13, 16, 10, 13, 14, 14, 13, 15, 13, 15, 15, 7, 7, 7, 11, 6, 14, 14, 14, 13};   // not referenced by game code
#pragma data_seg(".data$g7cfe44")
int g_numLevels = 30;
#pragma data_seg(".data$g7cfe48")
int g_unref_7cfe48[45] = {18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19};   // not referenced by game code
#pragma data_seg(".data$g7cff00")
unsigned char g_pad_7cff00[4] = {0};
#pragma data_seg(".data$g7cff04")
int g_unref_7cff04[1] = {16};   // not referenced by game code
#pragma data_seg(".data$g7cff08")
int g_unref_7cff08[43] = {29, 41, 56, 68, 80, 94, 105, 111, 120, 132, 144, 162, 176, 194, 207, 224, 237, 249, 260, 274, 291, 312, 326, 341, 353, 368, 377, 389, 402, 415, 427, 441, 454, 468, 483, 489, 495, 501, 511, 517, 531, 544, 558};   // not referenced by game code
#pragma data_seg(".data$g7cffb8")
int g_unref_7cffb8[45] = {117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117, 117};   // not referenced by game code
#pragma data_seg(".data$g7d0070")
int g_unref_7d0070[45] = {17, 14, 13, 16, 13, 13, 15, 12, 7, 10, 13, 13, 19, 15, 19, 14, 18, 13, 13, 12, 15, 17, 21, 15, 15, 13, 16, 10, 13, 14, 14, 13, 15, 13, 15, 15, 7, 7, 7, 11, 6, 14, 14, 14, 13};   // not referenced by game code
#pragma data_seg(".data$g7d0124")
int g_speedPctCache = -1;
#pragma data_seg(".data$g7d0128")
int g_unref_7d0128[45] = {18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19};   // not referenced by game code
#pragma data_seg(".data$g7d01dc")
int g_animTickA = 1;
#pragma data_seg(".data$g7d01e0")
FPair g_dirSlopeRange[9] = {{-10000.0f, -5.02734f}, {-5.02734f, -1.4966f}, {-1.4966f, -0.66818f}, {-0.66818f, -0.19891f}, {-0.19891f, 0.19891f}, {0.19891f, 0.66818f}, {0.66818f, 1.4966f}, {1.4966f, 5.02734f}, {5.02734f, 10000.0f}};
#pragma data_seg(".data$g7d0228")
int g_dirRemap[16] = {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};
#pragma data_seg(".data$g7d0268")
int g_rot16SrcX[16] = {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};
#pragma data_seg(".data$g7d02a8")
int g_rot16AltSrcX[6] = {0, 32, 64, 0, 32, 64};
#pragma data_seg(".data$g7d02c0")
int g_rot16SrcY[16] = {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480};
#pragma data_seg(".data$g7d0300")
int g_rot16AltSrcY[6] = {512, 512, 512, 544, 544, 544};
#pragma data_seg(".data$g7d0318")
Box g_boxesA[10] = {{2, 11, 4, 24}, {34, 11, 37, 24}, {5, 16, 34, 23}, {16, 6, 23, 17}, {19, 1, 20, 5}, {13, 9, 15, 17}, {9, 13, 12, 17}, {24, 10, 27, 17}, {28, 14, 31, 17}, {-1, -1, -1, -1}};
#pragma data_seg(".data$g7d03b8")
Box g_boxesB[9] = {{3, 2, 7, 22}, {31, 2, 35, 22}, {11, 6, 28, 21}, {9, 22, 30, 25}, {13, 4, 26, 5}, {15, 2, 24, 3}, {17, 1, 21, 2}, {-1, -1, -1, -1}, {-1, -1, -1, -1}};
#pragma data_seg(".data$g7d0448")
int g_unref_7d0448[3] = {-1, -1, -1};   // not referenced by game code
#pragma data_seg(".data$g7d0454")
float g_dirVecX[33] = {0.0f /* exe: 0xffffffff */, 0, 0.19509032f, 0.38268343f, 0.55557024f, 0.70710677f, 0.8314696f, 0.9238795f, 0.98078525f, 1.0f, 0.98078525f, 0.9238795f, 0.8314696f, 0.70710677f, 0.55557024f, 0.38268343f, 0.19509032f, 0, -0.19509032f, -0.38268343f, -0.55557024f, -0.70710677f, -0.8314696f, -0.9238795f, -0.98078525f, -1.0f, -0.98078525f, -0.9238795f, -0.8314696f, -0.70710677f, -0.55557024f, -0.38268343f, -0.19509032f};   // frame 1..32; [32] was g_dirVecY[0]
#pragma data_seg(".data$g7d04d4")
float g_dirVecY[33] = {-0.19509032f, -1.0f, -0.98078525f, -0.9238795f, -0.8314696f, -0.70710677f, -0.55557024f, -0.38268343f, -0.19509032f, 0, 0.19509032f, 0.38268343f, 0.55557024f, 0.70710677f, 0.8314696f, 0.9238795f, 0.98078525f, 1.0f, 0.98078525f, 0.98078525f, 0.9238795f, 0.8314696f, 0.70710677f, 0.55557024f, 0.38268343f, 0.19509032f, 0, -0.19509032f, -0.38268343f, -0.55557024f, -0.70710677f, -0.8314696f, -0.9238795f};
#pragma data_seg(".data$g7d0558")
float g_dirVecX2[40] = {0, 0.156434f, 0.309017f, 0.453991f, 0.587785f, 0.707107f, 0.809017f, 0.891007f, 0.951057f, 0.987688f, 1.0f, 0.987688f, 0.951057f, 0.891007f, 0.809017f, 0.707107f, 0.587785f, 0.45399f, 0.309017f, 0.156434f, 0, -0.156435f, -0.309017f, -0.453991f, -0.587785f, -0.707107f, -0.809017f, -0.891007f, -0.951057f, -0.987688f, -1.0f, -0.987688f, -0.951056f, -0.891006f, -0.809017f, -0.707107f, -0.587785f, -0.45399f, -0.309017f, -0.156434f};
#pragma data_seg(".data$g7d05f8")
float g_dirVecY2[40] = {-1.0f, -0.987688f, -0.951057f, -0.891007f, -0.809017f, -0.707107f, -0.587785f, -0.45399f, -0.309017f, -0.156434f, 0, 0.156434f, 0.309017f, 0.453991f, 0.587785f, 0.707107f, 0.809017f, 0.891007f, 0.951057f, 0.987688f, 1.0f, 0.987688f, 0.951057f, 0.891006f, 0.809017f, 0.707107f, 0.587785f, 0.45399f, 0.309017f, 0.156434f, 0, -0.156435f, -0.309017f, -0.453991f, -0.587785f, -0.707107f, -0.809017f, -0.891007f, -0.951057f, -0.987688f};
#pragma data_seg(".data$g7d0698")
int g_animTickB = 2;
#pragma data_seg(".data$g7d069c")
int g_animTickC = 3;
#pragma data_seg(".data$g7d06a0")
int g_shopUpEdge = 1;
#pragma data_seg(".data$g7d06a4")
int g_shopDownEdge = 1;
#pragma data_seg(".data$g7d06a8")
int g_unref_7d06a8[1] = {1};   // not referenced by game code
#pragma data_seg(".data$g7d06ac")
int g_jukeboxKeyLatch = 1;
#pragma data_seg(".data$g7d06b0")
int g_screenshotKeyEdge = 1;
#pragma data_seg(".data$g7d06b4")
int g_shopKeyLatchD = 1;
#pragma data_seg(".data$g7d06b8")
int g_f9KeyLatch = 1;
#pragma data_seg(".data$g7d06c4")
int g_shopFireEdge = 1;
#pragma data_seg(".data$g7d06c8")
int g_unref_7d06c8[7] = {1, 3, 1, 3, 4, 1117126656, 1065353216};   // not referenced by game code
#pragma data_seg(".data$g7d06e4")
int g_shopPrevItem = -1;
#pragma data_seg(".data$g7d06e8")
int g_shopActiveTimer = 4;
#pragma data_seg(".data$g7d06ec")
int g_unref_7d06ec[2] = {8, 1};   // not referenced by game code
#pragma data_seg(".data$g7d06f4")
int g_musicMode = -1;
#pragma data_seg(".data$g7d06f8")
int g_playlistIdx = -1;
#pragma data_seg(".data$g7d06fc")
int g_fadeOnce = 4;
#pragma data_seg(".data$g7d0700")
int g_bonusWeight[37] = {45, 45, 45, 45, 45, 111, 53, 90, 90, 130, 130, 65, 80, 80, 80, 20, 140, 20, 60, 35, 50, 25, 35, 35, 35, 5, 25, 10, 15, 300, 150, 75, 30, 8, 10, 15, 20};
#pragma data_seg(".data$g7d0794")
float g_coinVySpeedTable[7] = {1.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 10.0f};
#pragma data_seg(".data$g7d07b0")
int g_itemBonusSrcY[37] = {60, 80, 100, 120, 140, 20, 460, 160, 180, 280, 300, 40, 340, 320, 360, 240, 380, 400, 260, 420, 0, 500, 520, 540, 560, 660, 440, 200, 480, 600, 580, 620, 640, 220, 700, 680, 720};
#pragma data_seg(".data$g7d0844")
int g_coinSrcYTable[7] = {100, 80, 60, 40, 20, 0, 120};
#pragma data_seg(".data$g7d0860")
int g_itemHeightTable[37] = {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
#pragma data_seg(".data$g7d08f4")
int g_coinTypeTable[7] = {48, 49, 50, 51, 52, 53, 64};
#pragma data_seg(".data$g7d0910")
int g_itemWidthTable[37] = {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
#pragma data_seg(".data$g7d09a4")
int g_noSpritesDrawn = 1;
#pragma data_seg(".data$g7d09a8")
int g_itemFrameCountTable[37] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
#pragma data_seg(".data$g7d0a3c")
int g_malfunctionTimer = 22000;
#pragma data_seg(".data$g7d0a40")
int g_itemTypePool[38] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 750};
#pragma data_seg(".data$g7d0ad8")
float g_scoopYScale = 2.0f;
#pragma data_seg(".data$g7d0adc")
int g_unref_7d0adc[3] = {1040791372, 1, 1092616192};   // not referenced by game code
#pragma data_seg(".data$g7d0ae8")
float g_meterY = 550.0f;
#pragma data_seg(".data$g7d0aec")
int g_dragWin = -1;
#pragma data_seg(".data$g7d0af0")
int g_pressWin = -1;
#pragma data_seg(".data$g7d0af4")
int g_pressItem = -1;
#pragma data_seg(".data$g7d0af8")
int g_pressLink = -1;
#pragma data_seg(".data$g7d0afc")
int g_clickWin = -1;
#pragma data_seg(".data$g7d0b00")
int g_clickItem = -1;
#pragma data_seg(".data$g7d0b04")
int g_clickLink = -1;
#pragma data_seg(".data$g7d0b08")
int g_maxSparks = 30;
#pragma data_seg(".data$g7d0b0c")
int g_livesGainedCount = 20;
#pragma data_seg(".data$g7d0b10")
int g_bonusRoundTypePool[35] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38};
#pragma data_seg(".data$g7d0b9c")
int g_hsEntryIndex = -1;
#pragma data_seg(".data$g7d0ba0")
int g_weights[34] = {50, 200, 200, 200, 100, 50, 20, 150, 100, 50, 150, 50, 80, 80, 80, 80, 80, 80, 200, 200, 120, 120, 120, 120, 120, 120, 120, 100, 100, 100, 100, 100, 200, 50};
#pragma data_seg(".data$g7d0c28")
int g_memorySecretBirdSnapshot = 15;
#pragma data_seg(".data$g7d0c2c")
int g_hsRank[4] = {-1, -1, -1, -1};
#pragma data_seg(".data$g7d0c3c")
int g_hsPlayer[4] = {-1, -1, -1, -1};
#pragma data_seg(".data$g7d0c4c")
int g_hsRank2[4] = {-1, -1, -1, -1};
#pragma data_seg(".data$g7d0c5c")
int g_hsPlayer2[4] = {-1, -1, -1, -1};
#pragma data_seg(".data$g7d0c6c")
float g_bonusSpawnTarget = 20.0f;
#pragma data_seg(".data$g7d0c70")
int g_meteorSrcX[46] = {0, 64, 64, 96, 96, 128, 160, 224, 256, 384, 0, 160, 256, 160, 0, 128, 192, 288, 400, 480, 0, 176, 272, 400, 432, 176, 240, 288, 352, 424, 0, 148, 276, 424, 0, 128, 208, 256, 320, 384, 384, 320, 224, 128, 0, 144};
#pragma data_seg(".data$g7d0d28")
int g_bonusSrcY[46] = {0, 0, 26, 0, 28, 0, 0, 0, 0, 0, 52, 52, 72, 136, 193, 193, 187, 127, 195, 195, 276, 277, 255, 291, 291, 349, 349, 344, 344, 333, 410, 410, 409, 424, 515, 514, 514, 510, 510, 510, 550, 588, 553, 576, 616, 616};
#pragma data_seg(".data$g7d0de0")
int g_bonusGfxW[46] = {64, 32, 32, 32, 32, 32, 64, 32, 128, 160, 160, 96, 96, 64, 128, 64, 96, 96, 80, 144, 176, 96, 128, 32, 32, 64, 48, 64, 48, 96, 148, 128, 148, 96, 128, 80, 48, 48, 64, 48, 32, 144, 96, 32, 144, 80};
#pragma data_seg(".data$g7d0e98")
int g_bonusGfxH[46] = {53, 26, 21, 28, 25, 28, 36, 18, 72, 193, 141, 85, 54, 49, 82, 42, 67, 117, 95, 136, 132, 71, 88, 39, 31, 45, 59, 43, 46, 90, 104, 90, 100, 73, 99, 63, 38, 24, 77, 36, 16, 71, 113, 25, 99, 55};
#pragma data_seg(".data$g7d0f50")
float g_maxFallingGems = 1.0f;
#pragma data_seg(".data$g7d0f54")
float g_maxFallingGemsInc = 0.002f;
#pragma data_seg(".data$g7d0f58")
int g_maxBuffered = 10;
#pragma data_seg(".data$g7d0f5c")
int g_unref_7d0f5c[1] = {1};   // not referenced by game code
#pragma data_seg(".data$g7d0f60")
int g_levelLoadingFlag = 1;
#pragma data_seg(".data$g7d0f64")
int g_bossIdx = -1;
#pragma data_seg(".data$g7d0f68")
int g_memHoverRow = -1;
#pragma data_seg(".data$g7d0f6c")
int g_memHoverCol = -1;
#pragma data_seg(".data$g7d0f70")
int g_memCountdownStage = 11;
#pragma data_seg(".data$g7d0f74")
int g_raceCountdownStage = 3;
#pragma data_seg(".data$g7d0f78")
int g_memPendingCardType = -1;
#pragma data_seg(".data$g7d0f7c")
int g_selProfile = -1;
#pragma data_seg(".data$g7d0f80")
int g_profileIndex = -1;
#pragma data_seg(".data$g7d0f84")
int g_profileWin = -1;
#pragma data_seg(".data$g7d0f88")
int g_quitGameWin = -1;
#pragma data_seg(".data$g7d0f8c")
int g_quitWinWin = -1;
#pragma data_seg(".data$g7d0f90")
int g_shopItems = 83;
#pragma data_seg(".data$g7d0f94")
int g_version = 120;
#pragma data_seg(".data$g7d0f98")
int g_rankSprY[34] = {0, 13, 26, 39, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 65, 78, 91, 104, 104, 104, 104, 117, 130, 143, 156, 169, 182, 195, 208, 221, 234, 247, 260, 7835052};
#pragma data_seg(".data$g7d1020")
int g_weaponRow[33] = {0, 10, 20, 30, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 50, 60, 70, 80, 80, 80, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200};
#pragma data_seg(".data$g7d10a8")
char g_rankNames[33][0x22] = {"ENSIGN", "LIEUTENANT", "COMMANDER", "CAPTAIN", "ADMIRAL", "ADMIRAL 1 BRONZE STAR", "ADMIRAL 2 BRONZE STARS", "ADMIRAL 3 BRONZE STARS", "ADMIRAL 1 SILVER STAR", "ADMIRAL 2 SILVER STARS", "ADMIRAL 3 SILVER STARS", "ADMIRAL 1 GOLD STAR", "ADMIRAL 2 GOLD STARS", "ADMIRAL 3 GOLD STARS", "WARBLADE KNIGHT", "WARBLADE LORD", "WARBLADE OVERLORD", "WARBLADE GRANDMASTER", "WARBLADE GRANDMASTER 1 GOLD STAR", "WARBLADE GRANDMASTER 2 GOLD STARS", "WARBLADE GRANDMASTER 3 GOLD STARS", "WARBLADE CHAMPION", "WARBLADE GOD", "WARBLADE GOD  PLUTO RANK", "WARBLADE GOD  NEPTUNE RANK", "WARBLADE GOD  URANUS RANK", "WARBLADE GOD  SATURN RANK", "WARBLADE GOD  JUPITER RANK", "WARBLADE GOD  MARS RANK", "WARBLADE GOD  TELLUS RANK", "WARBLADE GOD  VENUS RANK", "WARBLADE GOD  MERCURY RANK", "WARBLADE GOD  SOL RANK"};
#pragma data_seg(".data$g7d150a")
unsigned char g_unref_7d150a = 1;
#pragma data_seg(".data$g7d150b")
unsigned char g_freshStart = 1;
#pragma data_seg(".data$g7d150c")
int g_pressedKey = -1;
#pragma data_seg(".data$g7d1510")
int g_unref_7d1510[1] = {1};   // not referenced by game code
#pragma data_seg(".data$g7d1518")
int g_unref_7d1518[1] = {2};   // not referenced by game code
#pragma data_seg(".data$g7d151c")
unsigned char g_playerBroke = 1;
#pragma data_seg(".data$g7d151e")
unsigned char g_pad_7d151e = 0;
#pragma data_seg(".data$g7d151f")
unsigned char g_pad_7d151f = 0;
#pragma data_seg(".data$g7d1520")
float g_gameSpeedMul = 1.0f;
#pragma data_seg(".data$g7d1524")
int g_shipStats0[9] = {26, 4, 12, 20, 123, 5, 10, 452, 8};
#pragma data_seg(".data$g7d1548")
int g_foundSlot = -1;
#pragma data_seg(".data$g7d154c")
int g_foundChan = -1;
#pragma data_seg(".data$g7d1550")
char g_shopPics[24][30] = {"shop_speed.jpg", "shop_bullet.jpg", "shop_doubleshot.jpg", "shop_lessspeed.jpg", "shop_tripleshot.jpg", "shop_quad.jpg", "shop_autofire.jpg", "shop_supertriple.jpg", "shop_armour.jpg", "shop_plasma.jpg", "shop_extralife.jpg", "shop_fireballs.jpg", "shop_secret.jpg", "shop_rank.jpg", "shop_extratime.jpg", "shop_laser.jpg", "shop_wariplasma.jpg", "shop_rocketpack.jpg", "shop_alienlock.jpg", "shop_autofire_super.jpg", "shop_rank.jpg", "shop_rank.jpg", "shop_rank.jpg"};
#pragma data_seg(".data$g7d1820")
char g_secretPics[32][30] = {"secret_01.jpg", "secret_02.jpg", "secret_03.jpg", "secret_04.jpg", "secret_05.jpg", "secret_06.jpg", "secret_07.jpg", "secret_08.jpg", "secret_09.jpg", "secret_10.jpg", "secret_11.jpg", "secret_12.jpg", "secret_13.jpg", "secret_14.jpg", "secret_15.jpg", "secret_16.jpg", "secret_17.jpg", "secret_18.jpg", "secret_19.jpg", "secret_20.jpg", "secret_21.jpg", "secret_22.jpg", "secret_23.jpg", "secret_24.jpg", "secret_25.jpg", "secret_26.jpg", "secret_27.jpg", "secret_28.jpg", "secret_29.jpg", "secret_30.jpg", "secret_31.jpg", "secret_32.jpg"};
#pragma data_seg(".data$g7d1be0")
int g_armourAddedCount = 10;
#pragma data_seg(".data$g7d1be4")
int g_shipStats6[9] = {3, 13, 39, 65, 621, 21, 42, 34, 5};
#pragma data_seg(".data$g7d1c08")
int g_deathsCount = 50;
#pragma data_seg(".data$g7d1c0c")
int g_engineFreqMax = 30000;
#pragma data_seg(".data$g7d1c10")
int g_engineFreqMin = 15000;
#pragma data_seg(".data$g7d1c14")
float g_targetItemX = -1.0f;
#pragma data_seg(".data$g7d1c18")
float g_targetItemY = -1.0f;
#pragma data_seg(".data$g7d1c1c")
float g_targetObjX = -1.0f;
#pragma data_seg(".data$g7d1c20")
float g_targetObjY = -1.0f;
#pragma data_seg(".data$g7d1c24")
float g_target847X = -1.0f;
#pragma data_seg(".data$g7d1c28")
float g_targetB49X = -1.0f;
#pragma data_seg(".data$g7d1c2c")
float g_targetB49Y = -1.0f;
#pragma data_seg(".data$g7d1c30")
int g_ringSize[10] = {470, 290, 300, 320, 300, 390, 290, 330, 330, 260};
#pragma data_seg(".data$g7d1c58")
int g_ringR[10] = {255, 200, 80, 50, 200, 200, 200, 120, 120, 130};
#pragma data_seg(".data$g7d1c80")
int g_ringG[10] = {100, 160, 100, 160, 150, 100, 100, 120, 120, 130};
#pragma data_seg(".data$g7d1ca8")
int g_ringB[11] = {0, 0, 140, 210, 0, 0, 0, 150, 220, 130, 128};
#pragma data_seg(".data$g7d1cd4")
int g_streakColorR = 255;
#pragma data_seg(".data$g7d1cd8")
int g_streakColorG = 255;
#pragma data_seg(".data$g7d1cdc")
float g_streakHalfSize = 400.0f;
#pragma data_seg(".data$g7d1ce0")
float g_streakY = 230.0f;
#pragma data_seg(".data$g7d1ce4")
float g_streakAlphaStep = 6.375f;
#pragma data_seg(".data$g7d1ce8")
float g_streakSpinSpeed0 = 4.2f;
#pragma data_seg(".data$g7d1cec")
float g_streakSpinSpeed1 = 2.2f;
#pragma data_seg(".data$g7d1cf0")
float g_streakSpinSpeed2 = 1.2f;
#pragma data_seg(".data$g7d1cf4")
int g_bannerY = 50;
#pragma data_seg(".data$g7d1cf8")
int g_bannerCenterX = 400;
#pragma data_seg(".data$g7d1cfc")
int g_bannerCenterY = 163;
#pragma data_seg(".data$g7d1d00")
int g_bannerHalfW = 400;
#pragma data_seg(".data$g7d1d04")
int g_bannerHalfH = 200;
#pragma data_seg(".data$g7d1d08")
int g_logoAnchorX = 400;
#pragma data_seg(".data$g7d1d0c")
int g_logoAnchorY = 350;
#pragma data_seg(".data$g7d1d10")
int g_bannerShrink = 600;
#pragma data_seg(".data$g7d1d14")
float g_leftWingY = 1000.0f;
#pragma data_seg(".data$g7d1d18")
float g_leftWingSize = 20.0f;
#pragma data_seg(".data$g7d1d1c")
float g_leftWingAngleBase = 50.0f;
#pragma data_seg(".data$g7d1d20")
float g_leftWingAngleDecay = 90.0f;
#pragma data_seg(".data$g7d1d24")
float g_rightWingY = 1000.0f;
#pragma data_seg(".data$g7d1d28")
float g_rightWingSize = 20.0f;
#pragma data_seg(".data$g7d1d2c")
float g_rightWingAngleBase = 310.0f;
#pragma data_seg(".data$g7d1d30")
float g_rightWingAngleDecay = 90.0f;
#pragma data_seg(".data$g7d1d34")
float g_wordmarkShrinkW = 1000.0f;
#pragma data_seg(".data$g7d1d38")
float g_wordmarkShrinkH = 500.0f;
#pragma data_seg(".data$g7d1d3c")
float g_centerScale = 1.0f;
#pragma data_seg(".data$g7d1d40")
float g_starRayAngle1 = 23.0f;
#pragma data_seg(".data$g7d1d44")
float g_starRayAngle2 = 123.0f;
#pragma data_seg(".data$g7d1d48")
float g_starRayAngle3 = 223.0f;
#pragma data_seg(".data$g7d1d4c")
float g_starRayAngle4 = 45.0f;
#pragma data_seg(".data$g7d1d50")
float g_starRayAngle5 = 80.0f;
#pragma data_seg(".data$g7d1d54")
int g_starYOffset = -50;
#pragma data_seg(".data$g7d1d58")
int g_shipStats7[9] = {2, 6, 18, 30, 341, 21, 42, 45, 14};
#pragma data_seg(".data$g7d1d7c")
int g_shipStats9[9] = {14, 6, 18, 30, 12, 8, 16, 52, 13};
#pragma data_seg(".data$g7d1da0")
int g_shipStats2[10] = {5, 2, 6, 10, 42, 7, 14, 51, 2, 1};
#pragma data_seg(".data$g7d1dc8")
float g_wobbleSpeed = 0.01f;
#pragma data_seg(".data$g7d1dcc")
float g_endDR = 0.1f;
#pragma data_seg(".data$g7d1dd0")
float g_endDG = 0.2f;
#pragma data_seg(".data$g7d1dd4")
float g_endDB = 0.05f;
#pragma data_seg(".data$g7d1dd8")
int g_shipStats1[9] = {45, 2, 6, 10, 34, 16, 32, 895, 43};
#pragma data_seg(".data$g7d1dfc")
int g_speedBarP0 = 1;
#pragma data_seg(".data$g7d1e00")
int g_smallFontWidths[74] = {4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 5, 5, 4, 4, 4, 4, 4, 5, 4, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 1, 5, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 2, 2, 4, 1, 2, 5, 4, 2, 2, 2, 4};
#pragma data_seg(".data$g7d1f28")
int g_ammoBarP0 = 1;
#pragma data_seg(".data$g7d1f2c")
int g_timeBarP0 = 1;
#pragma data_seg(".data$g7d1f30")
int g_fireRateBarP0 = 1;
#pragma data_seg(".data$g7d1f34")
int g_speedBarP1 = 1;
#pragma data_seg(".data$g7d1f38")
int g_ammoBarP1 = 1;
#pragma data_seg(".data$g7d1f3c")
int g_timeBarP1 = 1;
#pragma data_seg(".data$g7d1f40")
int g_fireRateBarP1 = 1;
#pragma data_seg(".data$g7d1f44")
int g_speedBarP2 = 1;
#pragma data_seg(".data$g7d1f48")
int g_ammoBarP2 = 1;
#pragma data_seg(".data$g7d1f4c")
int g_timeBarP2 = 1;
#pragma data_seg(".data$g7d1f50")
int g_fireRateBarP2 = 1;
#pragma data_seg(".data$g7d1f54")
int g_speedBarP3 = 1;
#pragma data_seg(".data$g7d1f58")
int g_ammoBarP3 = 1;
#pragma data_seg(".data$g7d1f5c")
int g_timeBarP3 = 1;
#pragma data_seg(".data$g7d1f60")
int g_fireRateBarP3 = 1;
#pragma data_seg(".data$g7d1f64")
int g_shipStats5[9] = {72, 8, 24, 40, 511, 16, 32, 622, 5};
#pragma data_seg(".data$g7d1f88")
int g_flareCnt0 = 1000;
#pragma data_seg(".data$g7d1f8c")
int g_flareCnt1 = 1;
#pragma data_seg(".data$g7d1f90")
int g_flareCnt2 = 1000;
#pragma data_seg(".data$g7d1f94")
int g_flareCnt3 = 1;
#pragma data_seg(".data$g7d1f98")
int g_flareOfs[11][11] = {{7, 7, 6, 5, 4, 3, 3, 2, 2, 2, 2}, {37, 37, 37, 37, 36, 36, 35, 35, 34, 33, 33}, {19, 19, 19, 19, 20, 20, 21, 21, 21, 23, 23}, {8, 8, 7, 6, 5, 5, 5, 4, 5, 5, 5}, {33, 34, 34, 34, 34, 33, 33, 32, 31, 31, 30}, {8, 8, 7, 6, 5, 5, 5, 5, 5, 5, 5}, {33, 33, 34, 33, 33, 33, 33, 32, 32, 31, 31}, {4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2}, {2, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5}, {21, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23}, {23, 23, 22, 22, 22, 22, 21, 21, 21, 21, 21}};
#pragma data_seg(".data$g7d217c")
int g_flareSize = 5;
#pragma data_seg(".data$g7d2180")
int g_digitX[11] = {326, 35, 54, 88, 121, 157, 191, 224, 258, 291, 361};
#pragma data_seg(".data$g7d21ac")
int g_digitW[11] = {33, 16, 33, 32, 35, 33, 32, 33, 32, 33, 7};
#pragma data_seg(".data$g7d21d8")
float g_bonusFxX = 100.0f;
#pragma data_seg(".data$g7d21dc")
float g_bonusFxVel = 14.0f;
#pragma data_seg(".data$g7d21e0")
int g_unref_7d21e0[1] = {10};   // not referenced by game code
#pragma data_seg(".data$g7d21e4")
int g_shipStats3[18] = {215, 8, 24, 40, 551, 4, 8, 34, 7, 105, 6, 18, 30, 671, 51, 102, 82, 4};
#pragma data_seg(".data$g7d222c")
float g_bossAnimStep = 1.0f;
#pragma data_seg(".data$g7d2230")
int g_shipStats4[9] = {415, 10, 30, 50, 731, 18, 36, 621, 71};
#pragma data_seg(".data$g7d2254")
int g_shipStats8[1065] = {225, 9, 27, 45, 56, 4, 8, 631, 58};
#pragma data_seg(".data$g7d32f8")
unsigned int g_screenW = 800u;
#pragma data_seg(".data$g7d32fc")
unsigned int g_screenH = 600u;
#pragma data_seg(".data$g7d3304")
float g_worldZoomInit = 1.0f;
#pragma data_seg(".data$g7d3308")
float g_worldZoom = 1.0f;
#pragma bss_seg(".bss$g7e3940")
_Alignas(16) unsigned char g_pad_7e3940[4];
#pragma bss_seg(".bss$g7e3944")
unsigned int g_exitSoundStartTime;
#pragma bss_seg(".bss$g7e3948")
BurstSpark g_slots[1000];
#pragma bss_seg(".bss$g7f0468")
Pattern g_pattern;
#pragma bss_seg(".bss$g7f1048")
LinkArea g_links[30];
#pragma bss_seg(".bss$g7f12a0")
HyperspaceStar g_stars[3000];
#pragma bss_seg(".bss$g802be0")
unsigned int g_pauseLastTick;
#pragma bss_seg(".bss$g802be4")
int g_hazard0GfxH;
#pragma bss_seg(".bss$g802be8")
void *g_scratchPoolD;
#pragma bss_seg(".bss$g802bec")
int g_pend5Freq;
#pragma bss_seg(".bss$g802bf0")
int g_moneyShipGfxW;
#pragma bss_seg(".bss$g802bf4")
float g_msgPanX;
#pragma bss_seg(".bss$g802bf8")
int g_msgColor;
#pragma bss_seg(".bss$g802bfc")
int g_hazard4GfxW;
#pragma bss_seg(".bss$g802c00")
Beam g_beams[12];
#pragma bss_seg(".bss$g803640")
Flash g_sparkleFlashes[10];
#pragma bss_seg(".bss$g8037d0")
int g_bonusKillCombo[255];
#pragma bss_seg(".bss$g803bcc")
int g_ship2GfxParamA;
#pragma bss_seg(".bss$g803bd0")
FrameSet g_frames[6];
#pragma bss_seg(".bss$g803c90")
int g_rocketGfxW;
#pragma bss_seg(".bss$g803c94")
unsigned char g_pad_803c94[4];
#pragma bss_seg(".bss$g803c98")
AlienGfxSlot g_alienGfxSlots[200];
#pragma bss_seg(".bss$g845ff8")
Image *g_gfxRankPlanets;
#pragma bss_seg(".bss$g845ffc")
int g_hazard5GfxW;
#pragma bss_seg(".bss$g846038")
short g_perfectColorTable[512];
#pragma bss_seg(".bss$g846438")
int g_sampleVol[10];
#pragma bss_seg(".bss$g846460")
unsigned char g_pad_846460[16];
#pragma bss_seg(".bss$g846470")
float g_grabZoneWidthTable[100];
#pragma bss_seg(".bss$g846600")
unsigned char g_pad_846600[1920];
#pragma bss_seg(".bss$g846d80")
Card g_cards[8][8];
#pragma bss_seg(".bss$g847380")
int g_portalGfxW;
#pragma bss_seg(".bss$g847384")
Rect16 g_blitSrc;
#pragma bss_seg(".bss$g847394")
int g_pend1Freq;
#pragma bss_seg(".bss$g847398")
FallingSprite g_fallingGems[10];
#pragma bss_seg(".bss$g8476e0")
int g_accSize;
#pragma bss_seg(".bss$g8476e4")
unsigned char g_pad_8476e4[4];
#pragma bss_seg(".bss$g8476e8")
VoidFn g_bulletsVsPlayerFn;
#pragma bss_seg(".bss$g8476ec")
float g_pend1Pan;
#pragma bss_seg(".bss$g847704")
VoidFn g_drawHudFn;
#pragma bss_seg(".bss$g847708")
float g_enemyVelXScratch;
#pragma bss_seg(".bss$g84770c")
unsigned char g_pad_84770c[8];
#pragma bss_seg(".bss$g847714")
char g_speedPctText[12];
#pragma bss_seg(".bss$g847720")
Explosion g_explosions[50];
#pragma bss_seg(".bss$g8485f8")
unsigned char g_pad_8485f8[4];
#pragma bss_seg(".bss$g8485fc")
int g_hazard2GfxH;
#pragma bss_seg(".bss$g848600")
Rect16 g_rectsA[6];
#pragma bss_seg(".bss$g848660")
unsigned char g_pad_848660[12];
#pragma bss_seg(".bss$g84866c")
int g_animFrameCount;
#pragma bss_seg(".bss$g848670")
unsigned char g_pad_848670[100];
#pragma bss_seg(".bss$g8486d4")
float g_fxColorSpeedR;
#pragma bss_seg(".bss$g8486d8")
SaveData g_save;
#pragma bss_seg(".bss$g849a48")
Enemy g_enemies[2][150];
#pragma bss_seg(".bss$g88e328")
unsigned char g_pad_88e328[280800];
#pragma bss_seg(".bss$g8d2c08")
LevelRec g_levelRecs[4000];
#pragma bss_seg(".bss$g8f2008")
int g_curLevelNum;
#pragma bss_seg(".bss$g8f200c")
int g_levelDataLoaded;
#pragma bss_seg(".bss$g8f2010")
int g_loadedLevel;
#pragma bss_seg(".bss$g8f2014")
int g_warpLevelR;
#pragma bss_seg(".bss$g8f2018")
int g_warpLevelL;
#pragma bss_seg(".bss$g8f201c")
unsigned char g_marksBonusGiven;
#pragma bss_seg(".bss$g8f201d")
unsigned char g_enemyAimAtPlayer;
#pragma bss_seg(".bss$g8f201e")
unsigned char g_fastEnemyBullets;
#pragma bss_seg(".bss$g8f201f")
unsigned char g_pad_8f201f;
#pragma bss_seg(".bss$g8f2020")
int g_diffEnemyFireChance;
#pragma bss_seg(".bss$g8f2024")
int g_diffShotFuseBase;
#pragma bss_seg(".bss$g8f2028")
int g_diffShotFuseRange;
#pragma bss_seg(".bss$g8f202c")
int g_hurryUpInterval;
#pragma bss_seg(".bss$g8f2030")
float g_diffShotSpeedMin;
#pragma bss_seg(".bss$g8f2034")
float g_diffShotSpeedMax;
#pragma bss_seg(".bss$g8f2038")
float g_diffTurretTrackChance;
#pragma bss_seg(".bss$g8f203c")
float g_diffBonusDropRoll;
#pragma bss_seg(".bss$g8f2040")
int g_curPlayer;
#pragma bss_seg(".bss$g8f2044")
int g_shopCurPlayer;
#pragma bss_seg(".bss$g8f2048")
int g_promoPlayer;
#pragma bss_seg(".bss$g8f204c")
int g_bonusStagePlayer;
#pragma bss_seg(".bss$g8f2050")
int g_vsTurnPlayer;
#pragma bss_seg(".bss$g8f2054")
int g_saveVersion;
#pragma bss_seg(".bss$g8f2058")
int g_diffScoreBonus;
#pragma bss_seg(".bss$g8f205c")
float g_defaultObjAlpha;
#pragma bss_seg(".bss$g8f2060")
float g_enemyBulletSpeed;
#pragma bss_seg(".bss$g8f2064")
float g_playerStartSpeed;
#pragma bss_seg(".bss$g8f2068")
float g_diffEnemyTimerMul;
#pragma bss_seg(".bss$g8f206c")
int g_diffEnemyHpBonus;
#pragma bss_seg(".bss$g8f2070")
int g_moneySuckerBaseHp;
#pragma bss_seg(".bss$g8f2074")
float g_diffHurryUpSpeedMax;
#pragma bss_seg(".bss$g8f2078")
int g_eliteHpBonus;
#pragma bss_seg(".bss$g8f207c")
int g_hurryUpHpBonus;
#pragma bss_seg(".bss$g8f2080")
float g_speedBase;
#pragma bss_seg(".bss$g8f2084")
float g_speedStep;
#pragma bss_seg(".bss$g8f2088")
float g_maxSpeedMul;
#pragma bss_seg(".bss$g8f208c")
float g_speedMax;
#pragma bss_seg(".bss$g8f2090")
int g_bonusDuration;
#pragma bss_seg(".bss$g8f2094")
float g_bonusSpawnRampRate;
#pragma bss_seg(".bss$g8f2098")
int g_bonusThresholdBase;
#pragma bss_seg(".bss$g8f209c")
int g_bonusRareChance;
#pragma bss_seg(".bss$g8f20a0")
int g_timeMax;
#pragma bss_seg(".bss$g8f20a4")
int g_speedMin;
#pragma bss_seg(".bss$g8f20a8")
int g_lastEliteSpawnLevel;
#pragma bss_seg(".bss$g8f20ac")
int g_fireDelayMin;
#pragma bss_seg(".bss$g8f20b0")
int g_enemyFireRateMin;
#pragma bss_seg(".bss$g8f20b4")
int g_fireDelayBiasA;
#pragma bss_seg(".bss$g8f20b8")
int g_fireDelayBiasB;
#pragma bss_seg(".bss$g8f20bc")
unsigned char g_pad_8f20bc[4];
#pragma bss_seg(".bss$g8f20c0")
__int64 g_resumeTimeOffset;
#pragma bss_seg(".bss$g8f20c8")
int g_comboLevel;
#pragma bss_seg(".bss$g8f20cc")
int g_comboStep;
#pragma bss_seg(".bss$g8f20d0")
int g_sessionScore;
#pragma bss_seg(".bss$g8f20d4")
int g_hits;
#pragma bss_seg(".bss$g8f20d8")
int g_gameMode;
#pragma bss_seg(".bss$g8f20dc")
int g_pendingGameMode;
#pragma bss_seg(".bss$g8f20e0")
int g_hofMode;
#pragma bss_seg(".bss$g8f20e4")
int g_savedDifficultySave;
#pragma bss_seg(".bss$g8f20e8")
int g_perfectCount;
#pragma bss_seg(".bss$g8f20ec")
int g_killCount;
#pragma bss_seg(".bss$g8f20f0")
char *g_playlistLines[10000];
#pragma bss_seg(".bss$g8fbd30")
float g_sinTable[3600];
#pragma bss_seg(".bss$g8ff574")
unsigned char g_pad_8ff574[4];
#pragma bss_seg(".bss$g8ff578")
Ring g_flash[4];
#pragma bss_seg(".bss$g8ff638")
FadeColors g_colR[3];
#pragma bss_seg(".bss$g8ff7a0")
int g_exitSoundElapsed;
#pragma bss_seg(".bss$g8ff7a4")
unsigned char g_pad_8ff7a4[8];
#pragma bss_seg(".bss$g8ff7ac")
int g_fadeFreq2;
#pragma bss_seg(".bss$g8ff7b0")
unsigned char g_pad_8ff7b0[4];
#pragma bss_seg(".bss$g8ff7b4")
int g_frameAX1;
#pragma bss_seg(".bss$g8ff7b8")
float g_starX[3000];
#pragma bss_seg(".bss$g902698")
char g_hiscorePackedOld[972504];
#pragma bss_seg(".bss$g9efd70")
unsigned int g_rngT;
#pragma bss_seg(".bss$g9efd74")
float g_pend2Pan;
#pragma bss_seg(".bss$g9efd78")
char g_levelName[80];
#pragma bss_seg(".bss$g9efdc8")
__int64 g_timeStampA;
#pragma bss_seg(".bss$g9efdd0")
unsigned char g_pad_9efdd0[148];
#pragma bss_seg(".bss$g9efe64")
int g_pend5Vol;
#pragma bss_seg(".bss$g9efe68")
char g_hiscoreBuf[660824];
#pragma bss_seg(".bss$ga913c0")
Account g_acc;
#pragma bss_seg(".bss$ga95490")
const char *g_songName;
#pragma bss_seg(".bss$ga95494")
unsigned char g_pad_a95494[4];
#pragma bss_seg(".bss$ga95498")
__int64 g_pauseStartStamp;
#pragma bss_seg(".bss$ga954a0")
unsigned char g_pad_a954a0[1920];
#pragma bss_seg(".bss$ga95c20")
Level g_curLevelData;
#pragma bss_seg(".bss$gab27b8")
VoidFn g_frameFunc;
#pragma bss_seg(".bss$gab27bc")
unsigned int g_time;
#pragma bss_seg(".bss$gab27d4")
int g_attractScreen;
#pragma bss_seg(".bss$gab27d8")
SoundQueueEntry g_soundQueue[10];
#pragma bss_seg(".bss$gab2878")
int g_diamondGfxH;
#pragma bss_seg(".bss$gab287c")
unsigned char g_pad_ab287c[4];
#pragma bss_seg(".bss$gab2880")
char g_songNames[256][260];
#pragma bss_seg(".bss$gac2c80")
char g_strE[260];
#pragma bss_seg(".bss$gac2d84")
int g_screenTimerStart;
#pragma bss_seg(".bss$gac2d88")
__int64 g_scoreMul[4];
#pragma bss_seg(".bss$gac2da8")
float g_starY[3000];
#pragma bss_seg(".bss$gac5c88")
unsigned char g_pad_ac5c88[4];
#pragma bss_seg(".bss$gac5c8c")
int g_ship1GfxParamB;
#pragma bss_seg(".bss$gac5c90")
unsigned char g_pad_ac5c90[8];
#pragma bss_seg(".bss$gac5c98")
Particle g_explosionParticles[500];
#pragma bss_seg(".bss$gad00a8")
int g_lastFrameTick;
#pragma bss_seg(".bss$gad00ac")
int g_fadeFreq1;
#pragma bss_seg(".bss$gad00b0")
Pattern g_patterns[50];
#pragma bss_seg(".bss$gaf5270")
float g_sinDeg[360];
#pragma bss_seg(".bss$gaf5810")
unsigned char g_pad_af5810[4];
#pragma bss_seg(".bss$gaf5814")
VoidFn g_stateFn;
#pragma bss_seg(".bss$gaf5818")
char g_scoreBuf[1024];
#pragma bss_seg(".bss$gaf5c18")
int g_sfxVolTable[256];
#pragma bss_seg(".bss$gaf602c")
float g_pend4Pan;
#pragma bss_seg(".bss$gaf6030")
int g_pend1Vol;
#pragma bss_seg(".bss$gaf6034")
int g_pend0Freq;
#pragma bss_seg(".bss$gaf6038")
unsigned char g_pad_af6038[8];
#pragma bss_seg(".bss$gaf6040")
__int64 g_playTimeFt;
#pragma bss_seg(".bss$gaf6048")
// BuildRampTables fills one entry per screen column; the decompile had 319, and the rest spilled
// into the padding after it.
float g_panTable[800];   // the screen width (g_screenW)
#pragma bss_seg(".bss$gaf6544")
float g_pan;
#pragma bss_seg(".bss$gaf6548")
unsigned char g_pad_af6548[2820];
#pragma bss_seg(".bss$gaf704c")
int g_meteorsGfxH;
#pragma bss_seg(".bss$gaf7050")
unsigned char g_pad_af7050[4];
#pragma bss_seg(".bss$gaf7054")
float g_pend3Pan;
#pragma bss_seg(".bss$gaf7058")
unsigned char g_pad_af7058[4];
#pragma bss_seg(".bss$gaf705c")
int g_pend4Vol;
#pragma bss_seg(".bss$gaf7830")
short g_animColScratch;
#pragma bss_seg(".bss$gaf7832")
unsigned char g_pad_af7832;
#pragma bss_seg(".bss$gaf7833")
unsigned char g_pad_af7833;
#pragma bss_seg(".bss$gaf7834")
unsigned char g_pad_af7834[4];
#pragma bss_seg(".bss$gaf7838")
void *g_scratchPoolA;
#pragma bss_seg(".bss$gaf783c")
int g_pend2Freq;
#pragma bss_seg(".bss$gaf7840")
int g_fadeFreq4;
#pragma bss_seg(".bss$gaf7844")
int g_guardWidth;
#pragma bss_seg(".bss$gaf7848")
Settings g_cfg;
#pragma bss_seg(".bss$gaf7e80")
LevelObj g_levelObj[100];
#pragma bss_seg(".bss$gafb530")
char g_msgBuf[1024];
#pragma bss_seg(".bss$gafb930")
int g_flashOverlayActive;
#pragma bss_seg(".bss$gafb934")
int g_diamondGfxW;
#pragma bss_seg(".bss$gafb938")
int g_state;
#pragma bss_seg(".bss$gafb93c")
unsigned char g_pad_afb93c[4];
#pragma bss_seg(".bss$gafb940")
char g_strD[260];
#pragma bss_seg(".bss$gafba44")
unsigned char g_pad_afba44[4];
#pragma bss_seg(".bss$gafba48")
__int64 g_pausedDuration;
#pragma bss_seg(".bss$gafba50")
int g_hiscoreSize;
#pragma bss_seg(".bss$gafba54")
unsigned char g_pad_afba54[4];
#pragma bss_seg(".bss$gafba58")
char g_concatBuf[264];
#pragma bss_seg(".bss$gafbb60")
int g_levelStarted;
#pragma bss_seg(".bss$gafbb64")
int g_cursorBlinkOn;
#pragma bss_seg(".bss$gafbb68")
unsigned int g_raceHoldTime;
#pragma bss_seg(".bss$gafbb6c")
int g_moneySuckerWidth;
#pragma bss_seg(".bss$gafbb70")
void *g_scratchPoolE;
#pragma bss_seg(".bss$gafbb74")
unsigned char g_pad_afbb74[4];
#pragma bss_seg(".bss$gafbb78")
char g_alertMsg[120];
#pragma bss_seg(".bss$gafbbf0")
float g_fxColorG;
#pragma bss_seg(".bss$gafbbf4")
void (*g_playerUpdateFn)();
#pragma bss_seg(".bss$gafbbf8")
char g_songPath[1024];
#pragma bss_seg(".bss$gafbff8")
char g_strH[260];
#pragma bss_seg(".bss$gafc0fc")
int g_hazard4GfxH;
#pragma bss_seg(".bss$gafc100")
AccountV2 g_accV2;
#pragma bss_seg(".bss$gaffd00")
unsigned char g_pad_affd00[20];
#pragma bss_seg(".bss$gaffd14")
int g_rocketGfxH;
#pragma bss_seg(".bss$gaffd18")
char g_buf[16384];
#pragma bss_seg(".bss$gb03d18")
int g_tallyStep;
#pragma bss_seg(".bss$gb03d1c")
unsigned char g_pad_b03d1c[4];
#pragma bss_seg(".bss$gb03d20")
int g_bonusGfxArea[46];
#pragma bss_seg(".bss$gb03dd8")
__int64 g_perfTimerStart;
#pragma bss_seg(".bss$gb03de0")
unsigned char g_pad_b03de0[44];
#pragma bss_seg(".bss$gb03e0c")
unsigned int g_lastTick;
#pragma bss_seg(".bss$gb04210")
unsigned int g_rngZ;
#pragma bss_seg(".bss$gb04214")
int g_coinGfxH;
#pragma bss_seg(".bss$gb04218")
int g_diamondBigGfxH;
#pragma bss_seg(".bss$gb0421c")
unsigned char g_pad_b0421c[4];
#pragma bss_seg(".bss$gb04220")
int g_ship1GfxParamA;
#pragma bss_seg(".bss$gb04224")
int g_frameBX1;
#pragma bss_seg(".bss$gb04228")
int g_gfxWeaponsBigW;
#pragma bss_seg(".bss$gb0422c")
void *g_scratchPoolC;
#pragma bss_seg(".bss$gb04230")
unsigned int g_fps;
#pragma bss_seg(".bss$gb04234")
VoidFn g_drawLevelObjectsFn;
#pragma bss_seg(".bss$gb04238")
VoidFn g_itemsVsPlayerFn;
#pragma bss_seg(".bss$gb0423c")
int g_frameBX2;
#pragma bss_seg(".bss$gb04240")
unsigned char g_pad_b04240[4];
#pragma bss_seg(".bss$gb04244")
int g_ship2GfxParamB;
#pragma bss_seg(".bss$gb04248")
int g_groupKillCount[255];
#pragma bss_seg(".bss$gb04644")
int g_lockOnLevelInitParam;
#pragma bss_seg(".bss$gb04648")
int g_musVolTable[256];
#pragma bss_seg(".bss$gb04a48")
int g_frameAX2;   // was declared as a Bonus view at 8 bytes before g_items
#pragma bss_seg(".bss$gb04a4c")
int g_warpMsgBlinkOn;
#pragma bss_seg(".bss$gb04a50")
Bonus g_items[150];
#pragma bss_seg(".bss$gb084e8")
unsigned char g_pad_b084e8[16];
#pragma bss_seg(".bss$gb0855c")
unsigned int g_rngY;
#pragma bss_seg(".bss$gb08560")
unsigned char g_pad_b08560[153600];
#pragma bss_seg(".bss$gb2dd60")
int g_coinGfxW;
#pragma bss_seg(".bss$gb2dd64")
float g_sfxVolume;
#pragma bss_seg(".bss$gb2dd68")
char g_moneyBuf1[260];
#pragma bss_seg(".bss$gb2de6c")
int g_frameDeltaMs;
#pragma bss_seg(".bss$gb2de70")
unsigned char g_pad_b2de70[256];
#pragma bss_seg(".bss$gb2df70")
int g_keyLatch[256];
#pragma bss_seg(".bss$gb2e370")
unsigned char g_pad_b2e370[3072];
#pragma bss_seg(".bss$gb2ef70")
AudioHandle g_samples[4][150];
#pragma bss_seg(".bss$gb2f8d0")
Image *g_gfxBird;
#pragma bss_seg(".bss$gb2f8d4")
int g_pend0Vol;
#pragma bss_seg(".bss$gb2f8d8")
char g_bonusNumBuf[20];
#pragma bss_seg(".bss$gb2f8ec")
float g_fxGreenVel;
#pragma bss_seg(".bss$gb2f8f0")
int g_rampB[769];
#pragma bss_seg(".bss$gb304f4")
int g_sfxFreq;
#pragma bss_seg(".bss$gb304f8")
int g_sampleRate[10];
#pragma bss_seg(".bss$gb30520")
char g_optionMsg[80];
#pragma bss_seg(".bss$gb30570")
int g_frameBY1;
#pragma bss_seg(".bss$gb30574")
unsigned int g_lastFpsTime;
#pragma bss_seg(".bss$gb30578")
char g_strM[260];
#pragma bss_seg(".bss$gb3067c")
unsigned char g_pad_b3067c[4];
#pragma bss_seg(".bss$gb30680")
__int64 g_hiscore;
#pragma bss_seg(".bss$gb30688")
unsigned char g_pad_b30688[8];
#pragma bss_seg(".bss$gb30690")
FreeParticle g_particles[1000];
#pragma bss_seg(".bss$gb49cd0")
int g_hazard3GfxW;
#pragma bss_seg(".bss$gb49cd4")
VoidFn g_fnPtr;
#pragma bss_seg(".bss$gb49cd8")
int g_portalGfxW2;
#pragma bss_seg(".bss$gb49cdc")
unsigned int g_prevFps;
#pragma bss_seg(".bss$gb49ce0")
char g_getReadyText[80];
#pragma bss_seg(".bss$gb49d44")
int g_mothershipMaskH;
#pragma bss_seg(".bss$gb49d48")
FallingHazard g_bonusMeteors[30];
#pragma bss_seg(".bss$gb4a798")
int g_frameBY2;
#pragma bss_seg(".bss$gb4a79c")
float g_fadePan2;
#pragma bss_seg(".bss$gb4a7a0")
char g_moneyBuf2[320];
#pragma bss_seg(".bss$gb4a8e0")
float g_fxRed;
#pragma bss_seg(".bss$gb4a8e4")
int g_guardHeight;
#pragma bss_seg(".bss$gb4a8e8")
char g_strP[260];
#pragma bss_seg(".bss$gb4a9ec")
int g_hazard3GfxH;
#pragma bss_seg(".bss$gb4a9f0")
// One per ship type (Player::ship, 0-9); the decompile had split it into g_shipDefs[1] and
// nine separate pointers, which only lined up under MSVC's section ordering.
ShipDef *g_shipDefs[NUM_SHIPS];
#pragma bss_seg(".bss$gb4aa18")
char g_moneyBuf3[280];
#pragma bss_seg(".bss$gb4ab30")
float g_fxBlueVel;
#pragma bss_seg(".bss$gb4ab48")
VoidFn g_drawBordersFn;
#pragma bss_seg(".bss$gb4ab4c")
char g_newName[40];
#pragma bss_seg(".bss$gb4ab74")
int g_bonusItemGfxH;
#pragma bss_seg(".bss$gb4ab78")
int g_moneyShipGfxH;
#pragma bss_seg(".bss$gb4ab7c")
unsigned char g_pad_b4ab7c[40];
#pragma bss_seg(".bss$gb4aba4")
int g_savedState;
#pragma bss_seg(".bss$gb4aba8")
int g_meteorBonusesGfxW;
#pragma bss_seg(".bss$gb4abac")
void *g_scratchPoolB;
#pragma bss_seg(".bss$gb4abb0")
__int64 g_pauseEndStamp;
#pragma bss_seg(".bss$gb4abb8")
char g_staleErrBuf[1024];
#pragma bss_seg(".bss$gb4afb8")
int g_gfxWeaponsBigH;
#pragma bss_seg(".bss$gb4afd8")
int g_pend3Freq;
#pragma bss_seg(".bss$gb4afdc")
int g_tallyState;
#pragma bss_seg(".bss$gc386b8")
Image *g_gfxTable[5];
#pragma bss_seg(".bss$gc386cc")
Image *g_gfxSparkA;
#pragma bss_seg(".bss$gc386d0")
Image *g_gfxExplodeDebris;
#pragma bss_seg(".bss$gc386d4")
Image *g_debrisParticleGfx;
#pragma bss_seg(".bss$gc386d8")
Image *g_gfxSpark;
#pragma bss_seg(".bss$gc386dc")
Image *g_gfxMissileSpark;
#pragma bss_seg(".bss$gc386e0")
Image *g_gfxBossBurst;
#pragma bss_seg(".bss$gc386e4")
unsigned char g_pad_c386e4[4];
#pragma bss_seg(".bss$gc386e8")
Image *g_gfxObjExplode;
#pragma bss_seg(".bss$gc386ec")
unsigned char g_pad_c386ec[4];
#pragma bss_seg(".bss$gc386f0")
unsigned char g_pad_c386f0[144];
#pragma bss_seg(".bss$gc38780")
char g_strN[260];
#pragma bss_seg(".bss$gc38884")
unsigned char g_pad_c38884[4];
#pragma bss_seg(".bss$gc38888")
int g_hazard0GfxW;
#pragma bss_seg(".bss$gc3888c")
unsigned char g_pad_c3888c[8];
#pragma bss_seg(".bss$gc38898")
unsigned char g_pad_c38898[1632];
#pragma bss_seg(".bss$gc38ef8")
char g_gfxPath[1024];
#pragma bss_seg(".bss$gc392f8")
__int64 g_timeStampB;
#pragma bss_seg(".bss$gc39300")
unsigned char g_pad_c39300[128];
#pragma bss_seg(".bss$gc39380")
HiscoreData g_hiscoreMagic;
#pragma bss_seg(".bss$gc3e4d8")
LevelRec g_replayRecs[5][4000];
#pragma bss_seg(".bss$gcda8d8")
char g_strS[260];
#pragma bss_seg(".bss$gcda9dc")
float g_fadeVolume1;
#pragma bss_seg(".bss$gcda9e0")
unsigned char g_pad_cda9e0[1920];
#pragma bss_seg(".bss$gcdb160")
FadeColors g_colG[3];
#pragma bss_seg(".bss$gcdb2c8")
float g_starZ[3000];
#pragma bss_seg(".bss$gcde1a8")
int g_meterValue;
#pragma bss_seg(".bss$gcde1bc")
unsigned char g_pad_cde1bc[4];
#pragma bss_seg(".bss$gcde1c0")
int g_typeKillCombo[255];
#pragma bss_seg(".bss$gcde5bc")
int g_pend3Vol;
#pragma bss_seg(".bss$gcde5c0")
BonusStats g_bonusTally[1];
#pragma bss_seg(".bss$gcde620")
unsigned char g_pad_cde620[24];
#pragma bss_seg(".bss$gcde638")
__int64 g_p2Bonus;
#pragma bss_seg(".bss$gcde640")
unsigned char g_pad_cde640[640];
#pragma bss_seg(".bss$gcde8c0")
PulseFx g_fx[10];
#pragma bss_seg(".bss$gcdeac8")
Spark g_sparks[2000];
#pragma bss_seg(".bss$gd0d8c8")
int g_bonusItemField34;
#pragma bss_seg(".bss$gd0d8cc")
float g_savedEnemyY;
#pragma bss_seg(".bss$gd0d8d0")
int g_lastActivityTime;
#pragma bss_seg(".bss$gd0d8d4")
int g_diamondBigGfxW;
#pragma bss_seg(".bss$gd0d8d8")
char g_warpMalfunctionMsg[108];
#pragma bss_seg(".bss$gd0d944")
float g_enemyVelYScratch;
#pragma bss_seg(".bss$gd0d948")
unsigned char g_pad_d0d948[8];
#pragma bss_seg(".bss$gd0d950")
VoidFn g_grabEnemyFn;
#pragma bss_seg(".bss$gd0d954")
int g_joyCount;
#pragma bss_seg(".bss$gd0d958")
int g_sampleHandle[10];
#pragma bss_seg(".bss$gd0d980")
float g_levelDist;
#pragma bss_seg(".bss$gd0d984")
char g_newPass[40];
#pragma bss_seg(".bss$gd0d9ac")
float g_fxBlueColorLevel;
#pragma bss_seg(".bss$gd0d9b0")
LevelRaw g_levelRaw;
#pragma bss_seg(".bss$gd2a548")
int g_portalGfxH;
#pragma bss_seg(".bss$gd2a54c")
int g_tallyTime;
#pragma bss_seg(".bss$gd2a550")
Image *g_starGfx[3100];
#pragma bss_seg(".bss$gd2d5c0")
Rect16 g_rectsB[6];
#pragma bss_seg(".bss$gd2d620")
Account g_accBuf[10];
#pragma bss_seg(".bss$gd55e40")
int g_hazard5GfxH;
#pragma bss_seg(".bss$gd55e44")
unsigned char g_pad_d55e44[4];
#pragma bss_seg(".bss$gd55e48")
float g_cosDeg[360];
#pragma bss_seg(".bss$gd563e8")
int g_mothershipMaskW;
#pragma bss_seg(".bss$gd563ec")
unsigned char g_pad_d563ec[4];
#pragma bss_seg(".bss$gd563f0")
int g_logoBirdFlareGfxH;
#pragma bss_seg(".bss$gd563f4")
unsigned int g_cursorBlinkTime;
#pragma bss_seg(".bss$gd563f8")
unsigned char g_pad_d563f8[1920];
#pragma bss_seg(".bss$gd56b78")
int g_pend4Freq;
#pragma bss_seg(".bss$gd56b7c")
unsigned char g_pad_d56b7c[4];
#pragma bss_seg(".bss$gd56b84")
int g_uiBlink;
#pragma bss_seg(".bss$gd56b88")
int g_triggerX[20];
#pragma bss_seg(".bss$gd56bd8")
unsigned int g_uiBlinkTime;
#pragma bss_seg(".bss$gd56bdc")
float g_savedEnemyX;
#pragma bss_seg(".bss$gd58570")
int g_moneySuckerHeight;
#pragma bss_seg(".bss$gd58574")
int g_hazard2GfxW;
#pragma bss_seg(".bss$gd58578")
int g_frameCount;
#pragma bss_seg(".bss$gd58d60")
int g_accSizes[10];
#pragma bss_seg(".bss$gd58d88")
Flash34 g_logoFlashes[49];
#pragma bss_seg(".bss$gd5977c")
unsigned char g_pad_d5977c[52];
#pragma bss_seg(".bss$gd597b0")
char g_name[32];
#pragma bss_seg(".bss$gd597d0")
short g_animRowScratch;
#pragma bss_seg(".bss$gd597d2")
unsigned char g_pad_d597d2;
#pragma bss_seg(".bss$gd597d3")
unsigned char g_pad_d597d3;
#pragma bss_seg(".bss$gd597d4")
unsigned char g_pad_d597d4[4];
#pragma bss_seg(".bss$gd597d8")
unsigned char g_pad_d597d8[1920];
#pragma bss_seg(".bss$gd59f58")
unsigned int g_rngW;
#pragma bss_seg(".bss$gd59f5c")
int g_hazard1GfxW;
#pragma bss_seg(".bss$gd59f60")
Image *g_gfxSword;
#pragma bss_seg(".bss$gd59f64")
float g_pend0Pan;
#pragma bss_seg(".bss$gd59f68")
char g_levelBannerText[80];
#pragma bss_seg(".bss$gd59fb8")
int g_meteorsGfxW;
#pragma bss_seg(".bss$gd59fbc")
unsigned char g_pad_d59fbc[4];
#pragma bss_seg(".bss$gd59fc0")
int g_groupEnemyCount[255];
#pragma bss_seg(".bss$gd5a3bc")
float g_fadeVolume0;
#pragma bss_seg(".bss$gd5a3c0")
char g_bannerMsg[80];
#pragma bss_seg(".bss$gd5a410")
AccountV0 g_accV0;
#pragma bss_seg(".bss$gd5bf58")
MenuEntry g_menuEntries[85];
#pragma bss_seg(".bss$gd5e334")
float g_sfxPanIdx;
#pragma bss_seg(".bss$gd5e338")
MapObj g_mapObjs[100];
#pragma bss_seg(".bss$gd621b8")
unsigned char g_pad_d621b8[4];
#pragma bss_seg(".bss$gd621bc")
unsigned long g_bltFastFlags;
#pragma bss_seg(".bss$gd621c0")
char g_pathBuf[256];
#pragma bss_seg(".bss$gd622c0")
int g_patternCount;
#pragma bss_seg(".bss$gd622c4")
int g_hiscorePackedSize;
#pragma bss_seg(".bss$gd622c8")
unsigned char g_pad_d622c8[4];
#pragma bss_seg(".bss$gd622cc")
int g_curWin;
#pragma bss_seg(".bss$gd622d0")
unsigned char g_pad_d622d0[4];
#pragma bss_seg(".bss$gd622d4")
ScoopTrail g_scoop[15];
#pragma bss_seg(".bss$gd62478")
unsigned char g_pad_d62478[12];
#pragma bss_seg(".bss$gd62484")
int g_frameAY2;
#pragma bss_seg(".bss$gd62488")
unsigned char g_pad_d62488[128];
#pragma bss_seg(".bss$gd62508")
unsigned int g_raceStartTime;
#pragma bss_seg(".bss$gd6250c")
unsigned int g_rngX;
#pragma bss_seg(".bss$gd62510")
Window g_windows[10];
#pragma bss_seg(".bss$gdeba90")
unsigned char g_pad_deba90[117656];
#pragma bss_seg(".bss$ge08628")
char g_strT[260];
#pragma bss_seg(".bss$ge0872c")
unsigned int g_frameTime;
#pragma bss_seg(".bss$ge08730")
float g_starA[3000];
#pragma bss_seg(".bss$ge0b610")
char g_logBuf[1024];
#pragma bss_seg(".bss$ge0ba10")
int g_hazard1GfxH;
#pragma bss_seg(".bss$ge0ba14")
VoidFn g_shipHudFn;
#pragma bss_seg(".bss$ge0ba18")
char g_strI[260];
#pragma bss_seg(".bss$ge0bb1c")
int g_portalGfxH2;
#pragma bss_seg(".bss$ge0bb20")
short g_enemyDamageStage[2][150];
#pragma bss_seg(".bss$ge0bd78")
unsigned char g_pad_e0bd78[600];
#pragma bss_seg(".bss$ge0bfd0")
char g_strB[260];
#pragma bss_seg(".bss$ge0c0d4")
int g_meteorBonusesGfxH;
#pragma bss_seg(".bss$ge0c0d8")
FadeColors g_colB[3];
#pragma bss_seg(".bss$ge0c240")
ScorePopup g_popups[10];
#pragma bss_seg(".bss$ge0c510")
float g_fadeVolume3;
#pragma bss_seg(".bss$ge0c514")
unsigned char g_pad_e0c514[4];
#pragma bss_seg(".bss$ge0c518")
float g_cosTable[3600];
#pragma bss_seg(".bss$ge0fd58")
int g_pend2Vol;
#pragma bss_seg(".bss$ge0fd5c")
float g_fadePan0;
#pragma bss_seg(".bss$ge10530")
char g_moneyBuf0[260];
#pragma bss_seg(".bss$ge10634")
int g_frameAY1;
#pragma bss_seg(".bss$ge10638")
float g_raceDistanceLeft;
#pragma bss_seg(".bss$ge1063c")
unsigned char g_pad_e1063c[4];
#pragma bss_seg(".bss$ge10658")
int g_logoBirdFlareGfxW;
#pragma bss_seg(".bss$ge1065c")
unsigned char g_pad_e1065c[4];
#pragma bss_seg(".bss$ge10660")
float g_pend5Pan;
#pragma bss_seg(".bss$ge10664")
unsigned char g_pad_e10664[4];
#pragma bss_seg(".bss$ge10668")
int g_bonusVolume[46];
#pragma bss_seg(".bss$ge107b4")
unsigned char g_pad_e107b4[4];
#pragma bss_seg(".bss$ge107b8")
unsigned char g_pad_e107b8[1920];
#pragma bss_seg(".bss$ge10f38")
unsigned char g_autoplay;
#pragma bss_seg(".bss$ge10f39")
unsigned char g_fileWriteErrorFlag;
#pragma bss_seg(".bss$ge10f3a")
bool g_linkHover;
#pragma bss_seg(".bss$ge10f3b")
unsigned char g_mirrorLevel;
#pragma bss_seg(".bss$ge10f3c")
int g_voiceSearchTries;
#pragma bss_seg(".bss$ge10f40")
unsigned char g_pad_e10f40[4];
#pragma bss_seg(".bss$ge10f44")
void *g_levelBufA;
#pragma bss_seg(".bss$ge10f48")
unsigned char g_pad_e10f48[4];
#pragma bss_seg(".bss$ge10f4c")
void *g_levelBufB;
#pragma bss_seg(".bss$ge10f50")
unsigned char g_pad_e10f50[4];
#pragma bss_seg(".bss$ge10f58")
int g_mouseDown;
#pragma bss_seg(".bss$ge10f5c")
int g_mouseClick;
#pragma bss_seg(".bss$ge10f60")
int g_rightButton;
#pragma bss_seg(".bss$ge10f64")
unsigned char g_pad_e10f64[4];
#pragma bss_seg(".bss$ge10f68")
int g_cursor;
#pragma bss_seg(".bss$ge10f6c")
int g_editing;
#pragma bss_seg(".bss$ge10f70")
unsigned char g_pad_e10f70[36];
#pragma bss_seg(".bss$ge10f94")
unsigned char g_autoplayCanFire;
#pragma bss_seg(".bss$ge10f95")
unsigned char g_mouseFlag;
#pragma bss_seg(".bss$ge10f96")
unsigned char g_presets;
#pragma bss_seg(".bss$ge10f97")
unsigned char g_restartNeeded;
#pragma bss_seg(".bss$ge10f98")
int g_pauseCount;
#pragma bss_seg(".bss$ge10f9c")
unsigned char g_pad_e10f9c[12];
#pragma bss_seg(".bss$ge10fa8")
int g_idleFrames;
#pragma bss_seg(".bss$ge10fac")
unsigned int g_splashEnd;
#pragma bss_seg(".bss$ge10fb0")
unsigned int g_splashMinEnd;
#pragma bss_seg(".bss$ge10fb4")
unsigned char g_pad_e10fb4[8];
#pragma bss_seg(".bss$ge10fbc")
int g_rankFanfarePlayed;
#pragma bss_seg(".bss$ge10fc0")
int g_buttonsOn;
#pragma bss_seg(".bss$ge10fc4")
int g_volumeSnapClick;
#pragma bss_seg(".bss$ge10fc8")
int g_mouseClickHandled;
#pragma bss_seg(".bss$ge10fcc")
unsigned char g_joy0;
#pragma bss_seg(".bss$ge10fcd")
unsigned char g_joy1;
#pragma bss_seg(".bss$ge10fce")
unsigned char g_windowed;
#pragma bss_seg(".bss$ge10fd0")
unsigned char g_pad_e10fd0[8];
#pragma bss_seg(".bss$ge10fd8")
int g_saveMagic;
#pragma bss_seg(".bss$ge10fdc")
unsigned char g_pad_e10fdc[16];
#pragma bss_seg(".bss$ge10fec")
void *g_ship1Hma;
#pragma bss_seg(".bss$ge10ff0")
void *g_ship2Hma;
#pragma bss_seg(".bss$ge10ff4")
void *g_hmaWeaponsBig;
#pragma bss_seg(".bss$ge10ff8")
void *g_alienGfxMem[6];
#pragma bss_seg(".bss$ge11010")
void *g_shutdownPtrA;
#pragma bss_seg(".bss$ge11014")
void *g_shutdownPtrB;
#pragma bss_seg(".bss$ge11018")
void *g_hmaDiamond;
#pragma bss_seg(".bss$ge1101c")
void *g_hmaDiamondBig;
#pragma bss_seg(".bss$ge11020")
unsigned char g_pad_e11020[4];
#pragma bss_seg(".bss$ge11024")
void *g_hmaBonuses;
#pragma bss_seg(".bss$ge11028")
void *g_hmaRocket;
#pragma bss_seg(".bss$ge1102c")
void *g_hmaMarks;
#pragma bss_seg(".bss$ge11030")
void *g_hmaMothership;
#pragma bss_seg(".bss$ge11034")
void *g_hmaMoneySucker;
#pragma bss_seg(".bss$ge11038")
void *g_hmaMoneyShip;
#pragma bss_seg(".bss$ge1103c")
void *g_hmaGuard;
#pragma bss_seg(".bss$ge11040")
void *g_hmaLogoBirdFlare;
#pragma bss_seg(".bss$ge11044")
void *g_hmaMeteors;
#pragma bss_seg(".bss$ge11048")
void *g_hmaMeteorBonuses;
#pragma bss_seg(".bss$ge11050")
Image *g_gfxFighter1;
#pragma bss_seg(".bss$ge11054")
Image *g_gfxFighter2;
#pragma bss_seg(".bss$ge11058")
Image *g_tinyFont;
#pragma bss_seg(".bss$ge1105c")
Image *g_smallFont;
#pragma bss_seg(".bss$ge11060")
Image *g_digitFont;
#pragma bss_seg(".bss$ge11064")
Image *g_fontGfx;
#pragma bss_seg(".bss$ge11068")
Image *g_gfxSparks;
#pragma bss_seg(".bss$ge1106c")
Image *g_gfxLogos;
#pragma bss_seg(".bss$ge11070")
Image *g_gfxLogo3;
#pragma bss_seg(".bss$ge11074")
Image *g_gfxLogo3Glow;
#pragma bss_seg(".bss$ge11078")
Image *g_gfxGameOver;
#pragma bss_seg(".bss$ge1107c")
Image *g_gfxSkull;
#pragma bss_seg(".bss$ge11080")
Image *g_gfxWeaponsBig;
#pragma bss_seg(".bss$ge11084")
Image *g_gfxFighterFire2;
#pragma bss_seg(".bss$ge11088")
unsigned char g_pad_e11088[4];
#pragma bss_seg(".bss$ge1108c")
IntPair g_alienGfxCache[6];
#pragma bss_seg(".bss$ge110bc")
Image *g_gfxDiamond;
#pragma bss_seg(".bss$ge110c0")
Image *g_gfxDiamondBig;
#pragma bss_seg(".bss$ge110c4")
unsigned char g_pad_e110c4[4];
#pragma bss_seg(".bss$ge110c8")
Image *g_gfxPause;
#pragma bss_seg(".bss$ge110cc")
Image *g_gfxBonus;
#pragma bss_seg(".bss$ge110d0")
Image *g_gfxRocket;
#pragma bss_seg(".bss$ge110d4")
Image *g_gfxCoins;
#pragma bss_seg(".bss$ge110d8")
Image *g_gfxMedals;
#pragma bss_seg(".bss$ge110dc")
Image *g_gfxRanks;
#pragma bss_seg(".bss$ge110e0")
Image *g_gfxRankIcons;
#pragma bss_seg(".bss$ge110e4")
Image *g_gfxMemoryBlocks;
#pragma bss_seg(".bss$ge110e8")
Image *g_gfxMothership;
#pragma bss_seg(".bss$ge110ec")
Image *g_gfxMothershipMask;
#pragma bss_seg(".bss$ge110f0")
Image *g_gfxMoneySucker;
#pragma bss_seg(".bss$ge110f4")
Image *g_gfxMoneySuckerMask;
#pragma bss_seg(".bss$ge110f8")
Image *g_gfxMoneyShip;
#pragma bss_seg(".bss$ge110fc")
Image *g_gfxMoneyShipMask;
#pragma bss_seg(".bss$ge11100")
Image *g_gfxBorderEasy;
#pragma bss_seg(".bss$ge11104")
Image *g_gfxBorderNormal;
#pragma bss_seg(".bss$ge11108")
Image *g_gfxBorderHard;
#pragma bss_seg(".bss$ge1110c")
Image *g_gfxBorderAce;
#pragma bss_seg(".bss$ge11110")
Image *g_gfxGuard;
#pragma bss_seg(".bss$ge11114")
Image *g_gfxGuardMask;
#pragma bss_seg(".bss$ge11118")
Image *g_gfxBeam;
#pragma bss_seg(".bss$ge1111c")
Image *g_explGfx;
#pragma bss_seg(".bss$ge11120")
Image *g_explGfx2;
#pragma bss_seg(".bss$ge11124")
Image *g_numbersGfx;
#pragma bss_seg(".bss$ge11128")
Image *g_gfxShopBg;
#pragma bss_seg(".bss$ge1112c")
unsigned char g_pad_e1112c[4];
#pragma bss_seg(".bss$ge11130")
Image *g_gfxSecretScreen;
#pragma bss_seg(".bss$ge11134")
Image *g_gfxSecretPic;
#pragma bss_seg(".bss$ge11138")
Image *g_creditPic;
#pragma bss_seg(".bss$ge1113c")
Image *g_splash;
#pragma bss_seg(".bss$ge11140")
unsigned char g_pad_e11140[4];
#pragma bss_seg(".bss$ge11144")
Image *g_endImg;
#pragma bss_seg(".bss$ge11148")
unsigned char g_pad_e11148[8];
#pragma bss_seg(".bss$ge11150")
Image *g_gfxShopItemPic;
#pragma bss_seg(".bss$ge11154")
Image *g_bgGraphic;
#pragma bss_seg(".bss$ge11158")
Image *g_gfxMeteorMeter;
#pragma bss_seg(".bss$ge1115c")
Image *g_gfxMeteorBonuses;
#pragma bss_seg(".bss$ge11160")
Image *g_gfxFlare1;
#pragma bss_seg(".bss$ge11164")
Image *g_gfxFlare2;
#pragma bss_seg(".bss$ge11168")
Image *g_gfxFlare3;
#pragma bss_seg(".bss$ge1116c")
Image *g_gfxFlare4;
#pragma bss_seg(".bss$ge11170")
Image *g_gfxFlare5;
#pragma bss_seg(".bss$ge11174")
Image *g_gfxFlare6;
#pragma bss_seg(".bss$ge11178")
Image *g_gfxFlare7;
#pragma bss_seg(".bss$ge1117c")
Image *g_gfxFlare8;
#pragma bss_seg(".bss$ge11180")
Image *g_gfxFlare9;
#pragma bss_seg(".bss$ge11184")
Image *g_gfxFlare10;
#pragma bss_seg(".bss$ge11188")
Image *g_gfxFlare11;
#pragma bss_seg(".bss$ge1118c")
Image *g_gfxFlare12;
#pragma bss_seg(".bss$ge11190")
Image *g_gfxFlare13;
#pragma bss_seg(".bss$ge11194")
Image *g_gfxFlare14;
#pragma bss_seg(".bss$ge11198")
Image *g_gfxFlare15;
#pragma bss_seg(".bss$ge1119c")
Image *g_gfxFlare16;
#pragma bss_seg(".bss$ge111a0")
Image *g_gfxFlare17;
#pragma bss_seg(".bss$ge111a4")
Image *g_gfxFlare18;
#pragma bss_seg(".bss$ge111a8")
Image *g_gfxFlare19;
#pragma bss_seg(".bss$ge111ac")
Image *g_gfxFlare20;
#pragma bss_seg(".bss$ge111b0")
Image *g_gfxFlare21;
#pragma bss_seg(".bss$ge111b4")
Image *g_gfxFlare23;
#pragma bss_seg(".bss$ge111b8")
Image *g_gfxFlare24;
#pragma bss_seg(".bss$ge111bc")
Image *g_gfxFlare25;
#pragma bss_seg(".bss$ge111c0")
Image *g_gfxFlare26;
#pragma bss_seg(".bss$ge111c4")
Image *g_gfxFlare27;
#pragma bss_seg(".bss$ge111c8")
Image *g_gfxFlare28;
#pragma bss_seg(".bss$ge111cc")
Image *g_gfxFlare29;
#pragma bss_seg(".bss$ge111d0")
Image *g_gfxFlare30;
#pragma bss_seg(".bss$ge111d4")
Image *g_gfxFlare31;
#pragma bss_seg(".bss$ge111d8")
Image *g_gfxFlare32;
#pragma bss_seg(".bss$ge111dc")
Image *g_gfxFlare33;
#pragma bss_seg(".bss$ge111e0")
Image *g_gfxFlare34;
#pragma bss_seg(".bss$ge111e4")
Image *g_gfxStar1;
#pragma bss_seg(".bss$ge111e8")
Image *g_starSprite;
#pragma bss_seg(".bss$ge111ec")
Image *g_gfxStar3;
#pragma bss_seg(".bss$ge111f0")
Image *g_gfxLogoFighter;
#pragma bss_seg(".bss$ge111f4")
Image *g_gfxLogoFighterShadow;
#pragma bss_seg(".bss$ge111f8")
Image *g_gfxLogoBird;
#pragma bss_seg(".bss$ge111fc")
Image *g_gfxLogoSword;
#pragma bss_seg(".bss$ge11200")
Image *g_gfxLogoSwordGlow;
#pragma bss_seg(".bss$ge11204")
Image *g_gfxShieldNew;
#pragma bss_seg(".bss$ge11208")
Image *g_gfxFlareSpark;
#pragma bss_seg(".bss$ge1120c")
Image *g_gfxFlareBomb;
#pragma bss_seg(".bss$ge11210")
Image *g_gfxFlareBomb2;
#pragma bss_seg(".bss$ge11214")
Image *g_gfxFlareBomb3;
#pragma bss_seg(".bss$ge11218")
Image *g_gfxFlareAtmos;
#pragma bss_seg(".bss$ge1121c")
Image *g_flashGfx;
#pragma bss_seg(".bss$ge11220")
Image *g_gfxFlareStreakBig;
#pragma bss_seg(".bss$ge11224")
Image *g_gfxFlareLaser;
#pragma bss_seg(".bss$ge11228")
Image *g_gfxFlareStreakGuard;
#pragma bss_seg(".bss$ge1122c")
Image *g_sparkGfx;
#pragma bss_seg(".bss$ge11230")
Image *g_gfxFlareScoop;
#pragma bss_seg(".bss$ge11234")
Image *g_gfxLogoBirdFlare;
#pragma bss_seg(".bss$ge11238")
Image *g_beamSparkGfxP0;
#pragma bss_seg(".bss$ge1123c")
Image *g_beamSparkGfxP1;
#pragma bss_seg(".bss$ge11240")
Image *g_gfxFlarePlanet;
#pragma bss_seg(".bss$ge11244")
Image *g_bg1;
#pragma bss_seg(".bss$ge11248")
Image *g_bg2;
#pragma bss_seg(".bss$ge1124c")
Image *g_bg3;
#pragma bss_seg(".bss$ge11250")
Image *g_bg4;
#pragma bss_seg(".bss$ge11254")
Image *g_bg5;
#pragma bss_seg(".bss$ge11258")
Image *g_winGfx;
#pragma bss_seg(".bss$ge1125c")
unsigned char g_pad_e1125c[4];
#pragma bss_seg(".bss$ge11260")
float g_bgScroll;
#pragma bss_seg(".bss$ge11264")
float g_angle0;
#pragma bss_seg(".bss$ge11268")
unsigned char g_keysChanged;
#pragma bss_seg(".bss$ge11269")
bool g_showFps;
#pragma bss_seg(".bss$ge1126a")
unsigned char g_enemyActedThisFrame;
#pragma bss_seg(".bss$ge1126b")
unsigned char g_pad_e1126b;
#pragma bss_seg(".bss$ge1126c")
int g_blitErrors;
#pragma bss_seg(".bss$ge11270")
int g_profileCollisions;
#pragma bss_seg(".bss$ge11274")
float g_frameDt;
#pragma bss_seg(".bss$ge11278")
unsigned char g_pad_e11278[64];
#pragma bss_seg(".bss$ge112b8")
int g_skipLogoFlag;
#pragma bss_seg(".bss$ge112bc")
unsigned char g_pad_e112bc[4];
#pragma bss_seg(".bss$ge112c0")
unsigned int g_soundStealCooldown;
#pragma bss_seg(".bss$ge112c4")
unsigned int g_nextSpark;
#pragma bss_seg(".bss$ge112c8")
unsigned int g_flameTimerP0;
#pragma bss_seg(".bss$ge112cc")
unsigned int g_flameTimerP1;
#pragma bss_seg(".bss$ge112d0")
unsigned int g_scoopTimerP0;
#pragma bss_seg(".bss$ge112d4")
unsigned int g_scoopTimerP1;
#pragma bss_seg(".bss$ge112d8")
unsigned int g_flameTimerP0Alt;
#pragma bss_seg(".bss$ge112dc")
unsigned int g_flameTimerP1Alt;
#pragma bss_seg(".bss$ge112e0")
unsigned int g_scoopTimerP0Alt;
#pragma bss_seg(".bss$ge112e4")
unsigned int g_scoopTimerP1Alt;
#pragma bss_seg(".bss$ge112e8")
int g_rocketRepeatTimer;
#pragma bss_seg(".bss$ge112ec")
unsigned int g_statSubmitCooldown;
#pragma bss_seg(".bss$ge112f0")
int g_debug;
#pragma bss_seg(".bss$ge112f4")
int g_inputCooldown;
#pragma bss_seg(".bss$ge112f8")
float g_fxSpinAngle;
#pragma bss_seg(".bss$ge112fc")
unsigned char g_pad_e112fc[4];
#pragma bss_seg(".bss$ge11300")
float g_fxAngleStep;
#pragma bss_seg(".bss$ge11304")
int g_shopClosing;
#pragma bss_seg(".bss$ge11308")
int g_levelIdleCounter;
#pragma bss_seg(".bss$ge1130c")
int g_altFireToggle;
#pragma bss_seg(".bss$ge11310")
int g_hiscoreEntryReset;
#pragma bss_seg(".bss$ge11314")
unsigned char g_pad_e11314[4];
#pragma bss_seg(".bss$ge11318")
int g_tallyDonePending;
#pragma bss_seg(".bss$ge1131c")
int g_titleResetPending;
#pragma bss_seg(".bss$ge11320")
int g_advanceScreenFlag;
#pragma bss_seg(".bss$ge11324")
int g_textCursorY;
#pragma bss_seg(".bss$ge11328")
int g_curY;
#pragma bss_seg(".bss$ge1132c")
int g_cursorX;
#pragma bss_seg(".bss$ge11330")
int g_textStartX;
#pragma bss_seg(".bss$ge11334")
unsigned char g_pad_e11334[4];
#pragma bss_seg(".bss$ge11338")
int g_resetFlag;
#pragma bss_seg(".bss$ge1133c")
unsigned char g_pad_e1133c[4];
#pragma bss_seg(".bss$ge11340")
float g_hiscoreEntryScale;
#pragma bss_seg(".bss$ge11344")
unsigned char g_levelStartLatch;
#pragma bss_seg(".bss$ge11345")
unsigned char g_boostReleased;
#pragma bss_seg(".bss$ge11346")
unsigned char g_isBossLevel;
#pragma bss_seg(".bss$ge11347")
unsigned char g_isWaveLevel;
#pragma bss_seg(".bss$ge11348")
unsigned char g_pad_e11348[8];
#pragma bss_seg(".bss$ge11350")
float g_boostCharge;
#pragma bss_seg(".bss$ge11354")
float g_baseSpeedRamp;
#pragma bss_seg(".bss$ge11358")
int g_fastFrameCount;
#pragma bss_seg(".bss$ge1135c")
int g_slowFrameCount;
#pragma bss_seg(".bss$ge11360")
float g_speedPct;
#pragma bss_seg(".bss$ge11364")
int g_bgTint;
#pragma bss_seg(".bss$ge11368")
int g_fadeStep;
#pragma bss_seg(".bss$ge1136c")
int g_fadeColorSet;
#pragma bss_seg(".bss$ge11370")
int g_explSrcY[18];
#pragma bss_seg(".bss$ge113b8")
unsigned char g_flag;
#pragma bss_seg(".bss$ge113b9")
unsigned char g_drunk;
#pragma bss_seg(".bss$ge113bc")
int g_beamLevel;
#pragma bss_seg(".bss$ge113c0")
int g_ringCount;
#pragma bss_seg(".bss$ge113c4")
unsigned char g_pad_e113c4[12];
#pragma bss_seg(".bss$ge113d0")
float g_starPulseAlpha;
#pragma bss_seg(".bss$ge113d4")
unsigned char g_pad_e113d4[4];
#pragma bss_seg(".bss$ge113d8")
int g_clipTop;
#pragma bss_seg(".bss$ge113dc")
int g_clipBottom;
#pragma bss_seg(".bss$ge113e0")
int g_clipLeft;
#pragma bss_seg(".bss$ge113e4")
int g_clipRight;
#pragma bss_seg(".bss$ge113e8")
float g_starZNear;
#pragma bss_seg(".bss$ge113ec")
unsigned char g_pad_e113ec[8];
#pragma bss_seg(".bss$ge113f4")
int g_bossBurstTimer;
#pragma bss_seg(".bss$ge113f8")
float g_diffHpBonusA;
#pragma bss_seg(".bss$ge113fc")
float g_diffHpBonusB;
#pragma bss_seg(".bss$ge11400")
int g_bossGunCountA;
#pragma bss_seg(".bss$ge11404")
int g_bossGunCountB;
#pragma bss_seg(".bss$ge11408")
int g_bossGunCountC;
#pragma bss_seg(".bss$ge1140c")
int g_type3EnemySpawnCount;
#pragma bss_seg(".bss$ge11410")
int g_alienAttackTimer;
#pragma bss_seg(".bss$ge11414")
int g_alienAttackTimerStep;
#pragma bss_seg(".bss$ge11418")
int g_guardTriggerFlag;
#pragma bss_seg(".bss$ge1141c")
float g_transitionRate;
#pragma bss_seg(".bss$ge11420")
int g_numLevels2;
#pragma bss_seg(".bss$ge11424")
unsigned char g_pad_e11424[2];
#pragma bss_seg(".bss$ge11426")
char g_musicPlaying;
#pragma bss_seg(".bss$ge11427")
unsigned char g_loginWinOpen;
#pragma bss_seg(".bss$ge11428")
unsigned char g_pad_e11428[4];
#pragma bss_seg(".bss$ge1142c")
unsigned int g_meterShowUntil;
#pragma bss_seg(".bss$ge11430")
float g_angVelX;
#pragma bss_seg(".bss$ge11434")
float g_angX;
#pragma bss_seg(".bss$ge11438")
float g_angVelY;
#pragma bss_seg(".bss$ge1143c")
float g_angY;
#pragma bss_seg(".bss$ge11440")
float g_angVelZ;
#pragma bss_seg(".bss$ge11444")
float g_angZ;
#pragma bss_seg(".bss$ge11448")
unsigned char g_pad_e11448[4];
#pragma bss_seg(".bss$ge1144c")
float g_targetX;
#pragma bss_seg(".bss$ge11450")
float g_targetY;
#pragma bss_seg(".bss$ge11454")
unsigned int g_menuIdleTimeout;
#pragma bss_seg(".bss$ge11458")
unsigned int g_timeTrialDeadline;
#pragma bss_seg(".bss$ge1145c")
int g_timeTrialLocked;
#pragma bss_seg(".bss$ge11460")
unsigned char g_pad_e11460[4];
#pragma bss_seg(".bss$ge11464")
int g_lastEventTime;
#pragma bss_seg(".bss$ge11468")
unsigned int g_timerB;
#pragma bss_seg(".bss$ge1146c")
unsigned int g_timerA;
#pragma bss_seg(".bss$ge11470")
int g_getReadyFlagA;
#pragma bss_seg(".bss$ge11474")
int g_memoryDone;
#pragma bss_seg(".bss$ge11478")
int g_memoryBonusPending;
#pragma bss_seg(".bss$ge1147c")
int g_memoryIntro;
#pragma bss_seg(".bss$ge11480")
int g_raceActive;
#pragma bss_seg(".bss$ge11484")
int g_gemDropIntroActive;
#pragma bss_seg(".bss$ge11488")
int g_hiscoreSkipGateActive;
#pragma bss_seg(".bss$ge1148c")
int g_rankMsgActive;
#pragma bss_seg(".bss$ge11490")
unsigned int g_rankLockUntil;
#pragma bss_seg(".bss$ge11494")
unsigned int g_rankPopupMinTime;
#pragma bss_seg(".bss$ge11498")
int g_resultsScreenEndTime;
#pragma bss_seg(".bss$ge1149c")
unsigned int g_memoryIntroTimer;
#pragma bss_seg(".bss$ge114a0")
unsigned int g_memoryStationDeadline;
#pragma bss_seg(".bss$ge114a4")
int g_gemDropIntroTimer;
#pragma bss_seg(".bss$ge114a8")
unsigned char g_pad_e114a8[4];
#pragma bss_seg(".bss$ge114ac")
int g_deathSeqActive;
#pragma bss_seg(".bss$ge114b0")
int g_transitionLockUntil;
#pragma bss_seg(".bss$ge114b4")
int g_transitionLock;
#pragma bss_seg(".bss$ge114b8")
int g_malfunctionAlarmTime;
#pragma bss_seg(".bss$ge114bc")
unsigned char g_pad_e114bc[4];
#pragma bss_seg(".bss$ge114c0")
unsigned int g_playerStallTime;
#pragma bss_seg(".bss$ge114c4")
unsigned char g_pad_e114c4[4];
#pragma bss_seg(".bss$ge114c8")
int g_inRandom;
#pragma bss_seg(".bss$ge114cc")
float g_borderScroll;
#pragma bss_seg(".bss$ge114d0")
unsigned int g_levelBannerTime;
#pragma bss_seg(".bss$ge114d4")
unsigned int g_msgTimer;
#pragma bss_seg(".bss$ge114d8")
unsigned int g_alertTextTime;
#pragma bss_seg(".bss$ge114dc")
unsigned int g_msgTime;
#pragma bss_seg(".bss$ge114e0")
unsigned int g_bannerMsgTime;
#pragma bss_seg(".bss$ge114e4")
int g_warpMsgBlinkTime;
#pragma bss_seg(".bss$ge114e8")
unsigned int g_bonusResultsTime;
#pragma bss_seg(".bss$ge114ec")
unsigned int g_perfectCheckDelay;
#pragma bss_seg(".bss$ge114f0")
int g_secretShown;
#pragma bss_seg(".bss$ge114f4")
unsigned char g_pad_e114f4[4];
#pragma bss_seg(".bss$ge114f8")
int g_animFrame;
#pragma bss_seg(".bss$ge11530")
int g_activePlayerBlink;
#pragma bss_seg(".bss$ge11534")
unsigned int g_activePlayerBlinkTime;
#pragma bss_seg(".bss$ge11538")
int g_colorPhase1;
#pragma bss_seg(".bss$ge1153c")
int g_colorPhase2;
#pragma bss_seg(".bss$ge11540")
int g_colorPhase3;
#pragma bss_seg(".bss$ge11544")
int g_introDone;
#pragma bss_seg(".bss$ge11548")
int g_levelStartFlag;
#pragma bss_seg(".bss$ge1154c")
int g_shopSelItem;
#pragma bss_seg(".bss$ge11554")
int g_musicRestartTime;
#pragma bss_seg(".bss$ge11558")
unsigned char g_pad_e11558[4];
#pragma bss_seg(".bss$ge1155c")
int g_songCount;
#pragma bss_seg(".bss$ge11560")
int g_songIndex;
#pragma bss_seg(".bss$ge11564")
AudioHandle g_songLengthMs;
#pragma bss_seg(".bss$ge11568")
int g_songEndTime;
#pragma bss_seg(".bss$ge1156c")
int g_soundEnabled;
#pragma bss_seg(".bss$ge11570")
int g_soundQueueCount;
#pragma bss_seg(".bss$ge11574")
unsigned int g_soundQueueNext;
#pragma bss_seg(".bss$ge11578")
int g_sndFlags;
#pragma bss_seg(".bss$ge1157c")
int g_sndFlags2;
#pragma bss_seg(".bss$ge11580")
AudioHandle g_chanScopeHum;
#pragma bss_seg(".bss$ge11584")
AudioHandle g_chanShieldHum;
#pragma bss_seg(".bss$ge11588")
AudioHandle g_musicPos;
#pragma bss_seg(".bss$ge1158c")
AudioHandle g_musicHandle;
#pragma bss_seg(".bss$ge11590")
AudioHandle g_musicStream;
#pragma bss_seg(".bss$ge11594")
int g_fadeQueueSample1;
#pragma bss_seg(".bss$ge11598")
int g_fadeQueueSample2;
#pragma bss_seg(".bss$ge1159c")
int g_fadeQueueSample3;
#pragma bss_seg(".bss$ge115a0")
int g_fadeQueueSample4;
#pragma bss_seg(".bss$ge115a4")
AudioHandle g_sfxAlienShoot1;
#pragma bss_seg(".bss$ge115a8")
AudioHandle g_sfxAlienShoot2;
#pragma bss_seg(".bss$ge115ac")
AudioHandle g_sfxAlienShoot3;
#pragma bss_seg(".bss$ge115b0")
AudioHandle g_sfxAlienShoot4;
#pragma bss_seg(".bss$ge115b4")
AudioHandle g_sndAlienShoot5;
#pragma bss_seg(".bss$ge115b8")
AudioHandle g_sfxAlienShoot6;
#pragma bss_seg(".bss$ge115bc")
AudioHandle g_sfxAlienShoot7;
#pragma bss_seg(".bss$ge115c0")
AudioHandle g_sfxAlienShoot8;
#pragma bss_seg(".bss$ge115c4")
AudioHandle g_sfxAlienShoot9;
#pragma bss_seg(".bss$ge115c8")
AudioHandle g_sndAlienShoot10;
#pragma bss_seg(".bss$ge115cc")
AudioHandle g_sfxAlienShoot11;
#pragma bss_seg(".bss$ge115d0")
AudioHandle g_sndAlienShoot12;
#pragma bss_seg(".bss$ge115d4")
AudioHandle g_sfxAlienShoot12Alt;
#pragma bss_seg(".bss$ge115d8")
AudioHandle g_sfxAlienShoot13;
#pragma bss_seg(".bss$ge115dc")
AudioHandle g_sfxAlienShoot14;
#pragma bss_seg(".bss$ge115e0")
AudioHandle g_sfxAlienShoot15;
#pragma bss_seg(".bss$ge115e4")
AudioHandle g_sfxAlienShoot16;
#pragma bss_seg(".bss$ge115e8")
AudioHandle g_sfxAlienShoot17;
#pragma bss_seg(".bss$ge115ec")
AudioHandle g_sfxAlienShoot18;
#pragma bss_seg(".bss$ge115f0")
AudioHandle g_sfxTast;
#pragma bss_seg(".bss$ge115f4")
AudioHandle g_sfxBell1;
#pragma bss_seg(".bss$ge115f8")
AudioHandle g_sfxBell2;
#pragma bss_seg(".bss$ge115fc")
AudioHandle g_sfxBell3;
#pragma bss_seg(".bss$ge11600")
AudioHandle g_sfxBigFire;
#pragma bss_seg(".bss$ge11604")
AudioHandle g_sfxBigSmall;
#pragma bss_seg(".bss$ge11608")
AudioHandle g_sfxHit1;
#pragma bss_seg(".bss$ge1160c")
AudioHandle g_sfxHit2;
#pragma bss_seg(".bss$ge11610")
AudioHandle g_sfxHit3;
#pragma bss_seg(".bss$ge11614")
AudioHandle g_sfxHit4;
#pragma bss_seg(".bss$ge11618")
AudioHandle g_sfxClickGeneric;
#pragma bss_seg(".bss$ge1161c")
AudioHandle g_sfxOver;
#pragma bss_seg(".bss$ge11620")
AudioHandle g_sfxFoundIt;
#pragma bss_seg(".bss$ge11624")
AudioHandle g_sfxBing;
#pragma bss_seg(".bss$ge11628")
AudioHandle g_sfxBirth;
#pragma bss_seg(".bss$ge1162c")
AudioHandle g_sfxChaching;
#pragma bss_seg(".bss$ge11630")
AudioHandle g_sfxCash;
#pragma bss_seg(".bss$ge11634")
AudioHandle g_sfxExplo1;
#pragma bss_seg(".bss$ge11638")
AudioHandle g_sfxExplo2;
#pragma bss_seg(".bss$ge1163c")
AudioHandle g_sfxExplo3;
#pragma bss_seg(".bss$ge11640")
AudioHandle g_sfxExplo4;
#pragma bss_seg(".bss$ge11644")
AudioHandle g_sfxExplo5;
#pragma bss_seg(".bss$ge11648")
AudioHandle g_sfxGuit;
#pragma bss_seg(".bss$ge1164c")
AudioHandle g_sfxFanfare;
#pragma bss_seg(".bss$ge11650")
AudioHandle g_sfxFanfare1;
#pragma bss_seg(".bss$ge11654")
AudioHandle g_sfxLaser1;
#pragma bss_seg(".bss$ge11658")
AudioHandle g_sfxLaser2;
#pragma bss_seg(".bss$ge1165c")
AudioHandle g_sfxOrkHit;
#pragma bss_seg(".bss$ge11660")
AudioHandle g_sfxShot1;
#pragma bss_seg(".bss$ge11664")
AudioHandle g_sfxShot2;
#pragma bss_seg(".bss$ge11668")
AudioHandle g_sfxWaom;
#pragma bss_seg(".bss$ge1166c")
AudioHandle g_sfxWooing;
#pragma bss_seg(".bss$ge11670")
AudioHandle g_sfxWaauw;
#pragma bss_seg(".bss$ge11674")
AudioHandle g_sfxZuzk;
#pragma bss_seg(".bss$ge11678")
AudioHandle g_sfxJangle;
#pragma bss_seg(".bss$ge1167c")
AudioHandle g_sfxHarpGliss1;
#pragma bss_seg(".bss$ge11680")
AudioHandle g_sfxMachine;
#pragma bss_seg(".bss$ge11684")
AudioHandle g_sfxRocket;
#pragma bss_seg(".bss$ge11688")
AudioHandle g_sfxMouww;
#pragma bss_seg(".bss$ge1168c")
AudioHandle g_sfxPing;
#pragma bss_seg(".bss$ge11690")
AudioHandle g_sfxWarp;
#pragma bss_seg(".bss$ge11694")
AudioHandle g_sfxWarp2;
#pragma bss_seg(".bss$ge11698")
AudioHandle g_sfxWarp3;
#pragma bss_seg(".bss$ge1169c")
AudioHandle g_sfxWheommm;
#pragma bss_seg(".bss$ge116a0")
AudioHandle g_sfxScopeHum;
#pragma bss_seg(".bss$ge116a4")
AudioHandle g_sfxShieldHum;
#pragma bss_seg(".bss$ge116a8")
AudioHandle g_sfxMotherLoop;
#pragma bss_seg(".bss$ge116ac")
AudioHandle g_sfxGuardLoop;
#pragma bss_seg(".bss$ge116b0")
AudioHandle g_sfxBossLoop;
#pragma bss_seg(".bss$ge116b4")
AudioHandle g_sfxShipHumLoop;
#pragma bss_seg(".bss$ge116b8")
AudioHandle g_sfxAlienAttack;
#pragma bss_seg(".bss$ge116bc")
AudioHandle g_sfxAlienAttack2;
#pragma bss_seg(".bss$ge116c0")
AudioHandle g_sfxAlienAttack3;
#pragma bss_seg(".bss$ge116c4")
AudioHandle g_sfxAlienAttack4;
#pragma bss_seg(".bss$ge116c8")
AudioHandle g_sfxAlienAttack5;
#pragma bss_seg(".bss$ge116cc")
AudioHandle g_sfxMeteorPass;
#pragma bss_seg(".bss$ge116d0")
AudioHandle g_sfxDeath;
#pragma bss_seg(".bss$ge116d4")
AudioHandle g_sfxCapture;
#pragma bss_seg(".bss$ge116d8")
AudioHandle g_sfxSlide;
#pragma bss_seg(".bss$ge116dc")
AudioHandle g_sfxKanganang;
#pragma bss_seg(".bss$ge116e0")
AudioHandle g_sfxJingles;
#pragma bss_seg(".bss$ge116e4")
AudioHandle g_sfxHahaha;
#pragma bss_seg(".bss$ge116e8")
AudioHandle g_sfxMothership;
#pragma bss_seg(".bss$ge116ec")
AudioHandle g_sfxPing2;
#pragma bss_seg(".bss$ge116f0")
AudioHandle g_sfxCoin;
#pragma bss_seg(".bss$ge116f4")
AudioHandle g_sfxCurrent;
#pragma bss_seg(".bss$ge116f8")
AudioHandle g_sfxRollover;
#pragma bss_seg(".bss$ge116fc")
AudioHandle g_sfxBuzzer;
#pragma bss_seg(".bss$ge11700")
AudioHandle g_sfxBuzzer2;
#pragma bss_seg(".bss$ge11704")
AudioHandle g_sfxMoneyBomb;
#pragma bss_seg(".bss$ge11708")
AudioHandle g_sfxGemBomb;
#pragma bss_seg(".bss$ge1170c")
AudioHandle g_sfxWindow;
#pragma bss_seg(".bss$ge11710")
AudioHandle g_sfxMinimize;
#pragma bss_seg(".bss$ge11714")
AudioHandle g_sfxButtonClick;
#pragma bss_seg(".bss$ge11718")
AudioHandle g_sfxZoom;
#pragma bss_seg(".bss$ge1171c")
AudioHandle g_sfxMetal;
#pragma bss_seg(".bss$ge11720")
AudioHandle g_sfxSword;
#pragma bss_seg(".bss$ge11724")
AudioHandle g_sfxThump;
#pragma bss_seg(".bss$ge11728")
AudioHandle g_sfxThumpBig;
#pragma bss_seg(".bss$ge1172c")
AudioHandle g_sfxSwoosh;
#pragma bss_seg(".bss$ge11730")
AudioHandle g_sfxComing;
#pragma bss_seg(".bss$ge11734")
AudioHandle g_sfxWhip;
#pragma bss_seg(".bss$ge11738")
AudioHandle g_sfxFalling;
#pragma bss_seg(".bss$ge1173c")
AudioHandle g_sfxSingleShot;
#pragma bss_seg(".bss$ge11740")
AudioHandle g_sfxBonus;
#pragma bss_seg(".bss$ge11744")
AudioHandle g_sfxExtraBullet;
#pragma bss_seg(".bss$ge11748")
AudioHandle g_sfxExtraLife;
#pragma bss_seg(".bss$ge1174c")
AudioHandle g_sfxExtraSpeed;
#pragma bss_seg(".bss$ge11750")
AudioHandle g_sfxExtraTime;
#pragma bss_seg(".bss$ge11754")
AudioHandle g_sfxGetReady;
#pragma bss_seg(".bss$ge11758")
AudioHandle g_sfxGetReady2;
#pragma bss_seg(".bss$ge1175c")
AudioHandle g_sfxGetReady3;
#pragma bss_seg(".bss$ge11760")
AudioHandle g_sfxHurryUp1;
#pragma bss_seg(".bss$ge11764")
AudioHandle g_sfxHurryUp2;
#pragma bss_seg(".bss$ge11768")
AudioHandle g_sfxMoney;
#pragma bss_seg(".bss$ge1176c")
AudioHandle g_sfxScoop;
#pragma bss_seg(".bss$ge11770")
AudioHandle g_sfxShield;
#pragma bss_seg(".bss$ge11774")
AudioHandle g_sfxSingleShotVoice;
#pragma bss_seg(".bss$ge11778")
AudioHandle g_sfxDoubleShot;
#pragma bss_seg(".bss$ge1177c")
AudioHandle g_sfxSucker;
#pragma bss_seg(".bss$ge11780")
AudioHandle g_sfxSucker2;
#pragma bss_seg(".bss$ge11784")
AudioHandle g_sfxSucker3;
#pragma bss_seg(".bss$ge11788")
AudioHandle g_sfxSuperTripleShot;
#pragma bss_seg(".bss$ge1178c")
AudioHandle g_sfxTripleShot;
#pragma bss_seg(".bss$ge11790")
AudioHandle g_sfxQuadShot;
#pragma bss_seg(".bss$ge11794")
AudioHandle g_sfxRankEnsign;
#pragma bss_seg(".bss$ge11798")
AudioHandle g_sfxRankLieutenant;
#pragma bss_seg(".bss$ge1179c")
AudioHandle g_sfxRankCommander;
#pragma bss_seg(".bss$ge117a0")
AudioHandle g_sfxRankCaptain;
#pragma bss_seg(".bss$ge117a4")
AudioHandle g_sfxRankAdmiral;
#pragma bss_seg(".bss$ge117a8")
AudioHandle g_sfxAlright;
#pragma bss_seg(".bss$ge117ac")
AudioHandle g_sfxCongratulations;
#pragma bss_seg(".bss$ge117b0")
AudioHandle g_sfxArmour;
#pragma bss_seg(".bss$ge117b4")
AudioHandle g_sfxBonusUnused;
#pragma bss_seg(".bss$ge117b8")
AudioHandle g_sfxMeteorStorm;
#pragma bss_seg(".bss$ge117bc")
AudioHandle g_sfxMemoryStation;
#pragma bss_seg(".bss$ge117c0")
AudioHandle g_sfxOops;
#pragma bss_seg(".bss$ge117c4")
AudioHandle g_sfxPlayer1;
#pragma bss_seg(".bss$ge117c8")
AudioHandle g_sfxPlayer2;
#pragma bss_seg(".bss$ge117cc")
AudioHandle g_sfxWarning;
#pragma bss_seg(".bss$ge117d0")
AudioHandle g_sfxWarpMalfunction;
#pragma bss_seg(".bss$ge117d4")
AudioHandle g_sfxWelcome;
#pragma bss_seg(".bss$ge117d8")
AudioHandle g_sfxVoiceLetterE;
#pragma bss_seg(".bss$ge117dc")
AudioHandle g_sfxVoiceLetterX;
#pragma bss_seg(".bss$ge117e0")
AudioHandle g_sfxVoiceLetterT;
#pragma bss_seg(".bss$ge117e4")
AudioHandle g_sfxVoiceLetterR;
#pragma bss_seg(".bss$ge117e8")
AudioHandle g_sfxVoiceLetterA;
#pragma bss_seg(".bss$ge117ec")
AudioHandle g_sampleSecret;
#pragma bss_seg(".bss$ge117f0")
AudioHandle g_sfxGameOver;
#pragma bss_seg(".bss$ge117f4")
AudioHandle g_sfxTimes2;
#pragma bss_seg(".bss$ge117f8")
AudioHandle g_sfxTimes5;
#pragma bss_seg(".bss$ge117fc")
AudioHandle g_sfxPerfect;
#pragma bss_seg(".bss$ge11800")
AudioHandle g_sfxGoodbye;
#pragma bss_seg(".bss$ge11804")
AudioHandle g_sfxFreeze;
#pragma bss_seg(".bss$ge11808")
AudioHandle g_sfxGemDrop;
#pragma bss_seg(".bss$ge1180c")
AudioHandle g_sfxPlayer;
#pragma bss_seg(".bss$ge11810")
AudioHandle g_sfxPlayers;
#pragma bss_seg(".bss$ge11814")
AudioHandle g_sfxAutofire;
#pragma bss_seg(".bss$ge11818")
AudioHandle g_sfxDrunk;
#pragma bss_seg(".bss$ge1181c")
AudioHandle g_sfxMirror;
#pragma bss_seg(".bss$ge11820")
AudioHandle g_sfxShop1;
#pragma bss_seg(".bss$ge11824")
AudioHandle g_sfxShop2;
#pragma bss_seg(".bss$ge11828")
AudioHandle g_sfxShop3;
#pragma bss_seg(".bss$ge1182c")
AudioHandle g_sfxShop4;
#pragma bss_seg(".bss$ge11830")
AudioHandle g_sfxShop5;
#pragma bss_seg(".bss$ge11834")
AudioHandle g_sfxVoiceOne;
#pragma bss_seg(".bss$ge11838")
AudioHandle g_sfxVoiceTwo;
#pragma bss_seg(".bss$ge1183c")
AudioHandle g_sfxVoiceThree;
#pragma bss_seg(".bss$ge11840")
AudioHandle g_sfxVoiceFour;
#pragma bss_seg(".bss$ge11844")
AudioHandle g_sfxVoiceFive;
#pragma bss_seg(".bss$ge11848")
AudioHandle g_sfxVoiceSix;
#pragma bss_seg(".bss$ge1184c")
AudioHandle g_sfxVoiceSeven;
#pragma bss_seg(".bss$ge11850")
AudioHandle g_sfxVoiceEight;
#pragma bss_seg(".bss$ge11854")
AudioHandle g_sfxVoiceNine;
#pragma bss_seg(".bss$ge11858")
AudioHandle g_sfxVoiceTen;
#pragma bss_seg(".bss$ge1185c")
AudioHandle g_sfxRankKnight;
#pragma bss_seg(".bss$ge11860")
AudioHandle g_sfxRankLord;
#pragma bss_seg(".bss$ge11864")
AudioHandle g_sfxRankOverlord;
#pragma bss_seg(".bss$ge11868")
AudioHandle g_sfxRankGrandmaster;
#pragma bss_seg(".bss$ge1186c")
AudioHandle g_sfxRankChampion;
#pragma bss_seg(".bss$ge11870")
AudioHandle g_sfxRankGod;
#pragma bss_seg(".bss$ge11874")
AudioHandle g_sfxStar;
#pragma bss_seg(".bss$ge11878")
AudioHandle g_sfxStars;
#pragma bss_seg(".bss$ge1187c")
AudioHandle g_sfxWarblade;
#pragma bss_seg(".bss$ge11880")
AudioHandle g_sfxRankBronze;
#pragma bss_seg(".bss$ge11884")
AudioHandle g_sfxRankSilver;
#pragma bss_seg(".bss$ge11888")
AudioHandle g_sfxRankGold;
#pragma bss_seg(".bss$ge1188c")
AudioHandle g_sfxOhNo;
#pragma bss_seg(".bss$ge11890")
AudioHandle g_sfxBomb;
#pragma bss_seg(".bss$ge11894")
AudioHandle g_sfxRankMarker;
#pragma bss_seg(".bss$ge11898")
AudioHandle g_sfxGotcha;
#pragma bss_seg(".bss$ge1189c")
AudioHandle g_sfxSpeed;
#pragma bss_seg(".bss$ge118a0")
AudioHandle g_sfxPlanetPluto;
#pragma bss_seg(".bss$ge118a4")
AudioHandle g_sfxPlanetNeptune;
#pragma bss_seg(".bss$ge118a8")
AudioHandle g_sfxPlanetUranus;
#pragma bss_seg(".bss$ge118ac")
AudioHandle g_sfxPlanetSaturn;
#pragma bss_seg(".bss$ge118b0")
AudioHandle g_sfxPlanetJupiter;
#pragma bss_seg(".bss$ge118b4")
AudioHandle g_sfxPlanetMars;
#pragma bss_seg(".bss$ge118b8")
AudioHandle g_sfxPlanetTellus;
#pragma bss_seg(".bss$ge118bc")
AudioHandle g_sfxPlanetVenus;
#pragma bss_seg(".bss$ge118c0")
AudioHandle g_sfxPlanetMercury;
#pragma bss_seg(".bss$ge118c4")
AudioHandle g_sfxPlanetSol;
#pragma bss_seg(".bss$ge118c8")
AudioHandle g_sfxUltimateRank;
#pragma bss_seg(".bss$ge118cc")
AudioHandle g_sfxRank;
#pragma bss_seg(".bss$ge118d0")
AudioHandle g_sfxAvailable;
#pragma bss_seg(".bss$ge118d4")
AudioHandle g_sfxPlanet;
#pragma bss_seg(".bss$ge118d8")
AudioHandle g_sfxNew;
#pragma bss_seg(".bss$ge118dc")
AudioHandle g_sfxYouAreThe;
#pragma bss_seg(".bss$ge118e0")
unsigned char g_pad_e118e0[12];
#pragma bss_seg(".bss$ge118ec")
unsigned char g_warpWhooshPlayed;
#pragma bss_seg(".bss$ge118ed")
unsigned char g_bonusMeterStarted;
#pragma bss_seg(".bss$ge118ee")
unsigned char g_gadgetHitLatch;
#pragma bss_seg(".bss$ge118ef")
unsigned char g_mouseOverTextItem;
#pragma bss_seg(".bss$ge118f0")
unsigned char g_pad_e118f0[4];
#pragma bss_seg(".bss$ge118f4")
int g_coinSrcX[7];
#pragma bss_seg(".bss$ge11910")
int g_itemBonusSrcX[37];
#pragma bss_seg(".bss$ge119a4")
int g_warpCheckCount;
#pragma bss_seg(".bss$ge119a8")
int g_warpMalfunctionCount;
#pragma bss_seg(".bss$ge119ac")
float g_scoopRange;
#pragma bss_seg(".bss$ge119b0")
float g_scoopTrailScale;
#pragma bss_seg(".bss$ge119b4")
float g_meterV;
#pragma bss_seg(".bss$ge119b8")
int g_nextId;
#pragma bss_seg(".bss$ge119bc")
unsigned char g_pad_e119bc[4];
#pragma bss_seg(".bss$ge119c0")
int g_lastLeft;
#pragma bss_seg(".bss$ge119c4")
int g_lastRight;
#pragma bss_seg(".bss$ge119c8")
int g_lastTop;
#pragma bss_seg(".bss$ge119cc")
int g_lastBottom;
#pragma bss_seg(".bss$ge119d0")
int g_last2Left;
#pragma bss_seg(".bss$ge119d4")
int g_last2Right;
#pragma bss_seg(".bss$ge119d8")
int g_last2Top;
#pragma bss_seg(".bss$ge119dc")
int g_last2Bottom;
#pragma bss_seg(".bss$ge119e0")
unsigned char g_pad_e119e0[4];
#pragma bss_seg(".bss$ge119e4")
int g_nextX;
#pragma bss_seg(".bss$ge119e8")
unsigned char g_buttonHitLatch;
#pragma bss_seg(".bss$ge119e9")
unsigned char g_winDragActive;
#pragma bss_seg(".bss$ge119ea")
unsigned char g_pad_e119ea;
#pragma bss_seg(".bss$ge119eb")
bool g_newGameOnClose;
#pragma bss_seg(".bss$ge119ec")
int g_dragDX;
#pragma bss_seg(".bss$ge119f0")
int g_dragDY;
#pragma bss_seg(".bss$ge119f4")
int g_linkCount;
#pragma bss_seg(".bss$ge119f8")
int g_enemyFrameCounter;
#pragma bss_seg(".bss$ge119fc")
int g_savedDifficultyTT;
#pragma bss_seg(".bss$ge11a00")
int g_cfgBackup;
#pragma bss_seg(".bss$ge11a04")
int g_hudY;
#pragma bss_seg(".bss$ge11a08")
__int64 g_bestScore;
#pragma bss_seg(".bss$ge11a10")
int g_hiscoreInsertGate;
#pragma bss_seg(".bss$ge11a14")
int g_hsTable[4];
#pragma bss_seg(".bss$ge11a24")
int g_nameLen;
#pragma bss_seg(".bss$ge11a28")
unsigned char g_pad_e11a28[4];
#pragma bss_seg(".bss$ge11a2c")
char g_hiscoreTransitionFlag;
#pragma bss_seg(".bss$ge11a2d")
unsigned char g_superGemDrop;
#pragma bss_seg(".bss$ge11a2e")
unsigned char g_noProfilesError;
#pragma bss_seg(".bss$ge11a2f")
unsigned char g_profileReadOnly;
#pragma bss_seg(".bss$ge11a30")
int g_bonusItemCount;
#pragma bss_seg(".bss$ge11a34")
int g_pickupCount;
#pragma bss_seg(".bss$ge11a38")
int g_bossGunActiveA;
#pragma bss_seg(".bss$ge11a3c")
int g_bossGunActiveB;
#pragma bss_seg(".bss$ge11a40")
int g_bossGunActiveC;
#pragma bss_seg(".bss$ge11a44")
int g_alienGfxBufferedCount;
#pragma bss_seg(".bss$ge11a48")
int g_gfxLoaded;
#pragma bss_seg(".bss$ge11a4c")
int g_gfx2Loaded;
#pragma bss_seg(".bss$ge11a50")
int g_hmaLoaded;
#pragma bss_seg(".bss$ge11a54")
int g_alienGfxFreedA;
#pragma bss_seg(".bss$ge11a58")
int g_alienGfxFreedB;
#pragma bss_seg(".bss$ge11a5c")
int g_alienGfxFreedM;
#pragma bss_seg(".bss$ge11a60")
int g_freedA;
#pragma bss_seg(".bss$ge11a64")
int g_freedB;
#pragma bss_seg(".bss$ge11a68")
int g_freedC;
#pragma bss_seg(".bss$ge11a6c")
int g_numMalfunction;
#pragma bss_seg(".bss$ge11a70")
unsigned char g_pad_e11a70[8];
#pragma bss_seg(".bss$ge11a78")
int g_gridX;
#pragma bss_seg(".bss$ge11a7c")
int g_gridY;
#pragma bss_seg(".bss$ge11a80")
int g_memSelValid;
#pragma bss_seg(".bss$ge11a84")
int g_memSelRow;
#pragma bss_seg(".bss$ge11a88")
int g_memSelCol;
#pragma bss_seg(".bss$ge11a8c")
int g_memCursorFrame;
#pragma bss_seg(".bss$ge11a90")
unsigned int g_gridAnimTime;
#pragma bss_seg(".bss$ge11a94")
unsigned int g_memStageDeadline;
#pragma bss_seg(".bss$ge11a98")
unsigned int g_memPickResolveTime;
#pragma bss_seg(".bss$ge11a9c")
int g_pairsTickTime;
#pragma bss_seg(".bss$ge11aa0")
int g_pauseShiftedTimerE;
#pragma bss_seg(".bss$ge11aa4")
int g_toggleOnCount;
#pragma bss_seg(".bss$ge11aa8")
int g_toggleOffCount;
#pragma bss_seg(".bss$ge11aac")
int g_profileCount;
#pragma bss_seg(".bss$ge11ab0")
unsigned char g_profileWinOpen;
#pragma bss_seg(".bss$ge11ab1")
unsigned char g_quitGameWinOpen;
#pragma bss_seg(".bss$ge11ab2")
bool g_quitToWindowsWinOpen;
#pragma bss_seg(".bss$ge11ab3")
bool g_dialogWinOpen;
#pragma bss_seg(".bss$ge11ab4")
unsigned char g_itemSteered;
#pragma bss_seg(".bss$ge11ab6")
unsigned char g_loggedIn;
#pragma bss_seg(".bss$ge11ab7")
bool g_endWobbleActive;
#pragma bss_seg(".bss$ge11ab8")
__int64 g_shownStat;
#pragma bss_seg(".bss$ge11ac0")
int g_newGamePending;
#pragma bss_seg(".bss$ge11ac4")
int g_bonusFlag;
#pragma bss_seg(".bss$ge11ac8")
int g_moneyBlinkTimer;
#pragma bss_seg(".bss$ge11acc")
int g_hudSpeedBarTick;
#pragma bss_seg(".bss$ge11ad8")
bool g_endWobbleEnabled;
#pragma bss_seg(".bss$ge11ad9")
unsigned char g_customSongs;
#pragma bss_seg(".bss$ge11ada")
unsigned char g_soundPaused;
#pragma bss_seg(".bss$ge11adb")
bool g_extraLifeGranted;
#pragma bss_seg(".bss$ge11aec")
int g_slotCount;
#pragma bss_seg(".bss$ge11af0")
int g_curSong;
#pragma bss_seg(".bss$ge11af4")
char *g_playlistBuf;
#pragma bss_seg(".bss$ge11af8")
int g_playlistSize;
#pragma bss_seg(".bss$ge11afc")
int g_playlistCount;
#pragma bss_seg(".bss$ge11b00")
AudioHandle g_curStream;
#pragma bss_seg(".bss$ge11b04")
AudioHandle g_pend0;
#pragma bss_seg(".bss$ge11b08")
unsigned char g_pad_e11b08[4];
#pragma bss_seg(".bss$ge11b0c")
AudioHandle g_pend1;
#pragma bss_seg(".bss$ge11b10")
unsigned char g_pad_e11b10[4];
#pragma bss_seg(".bss$ge11b14")
AudioHandle g_pend2;
#pragma bss_seg(".bss$ge11b18")
unsigned char g_pad_e11b18[4];
#pragma bss_seg(".bss$ge11b1c")
AudioHandle g_pend3;
#pragma bss_seg(".bss$ge11b20")
unsigned char g_pad_e11b20[4];
#pragma bss_seg(".bss$ge11b24")
AudioHandle g_pend4;
#pragma bss_seg(".bss$ge11b28")
unsigned char g_pad_e11b28[4];
#pragma bss_seg(".bss$ge11b2c")
AudioHandle g_pend5;
#pragma bss_seg(".bss$ge11b30")
unsigned char g_pad_e11b30[4];
#pragma bss_seg(".bss$ge11b34")
float g_scanY;
#pragma bss_seg(".bss$ge11b38")
unsigned char g_pad_e11b38[8];
#pragma bss_seg(".bss$ge11b40")
int g_newGameResetVal;
#pragma bss_seg(".bss$ge11b51")
bool g_playTimeAdded;
#pragma bss_seg(".bss$ge11b52")
bool g_profilePlayTimeAdded;
#pragma bss_seg(".bss$ge11b53")
unsigned char g_pad_e11b53;
#pragma bss_seg(".bss$ge11b54")
int g_shownRank;
#pragma bss_seg(".bss$ge11b58")
int g_introGateScratch;
#pragma bss_seg(".bss$ge11b5c")
int g_memoryStationExitFlag;
#pragma bss_seg(".bss$ge11b60")
float g_dt;
#pragma bss_seg(".bss$ge11b64")
int g_skipRender;
#pragma bss_seg(".bss$ge11b68")
unsigned char g_pad_e11b68[4];
#pragma bss_seg(".bss$ge11b6c")
bool g_saveMsgToggle;
#pragma bss_seg(".bss$ge11b6d")
bool g_hasLives;
#pragma bss_seg(".bss$ge11b6e")
unsigned char g_promoRingActive;
#pragma bss_seg(".bss$ge11b6f")
unsigned char g_promoSpecialRank;
#pragma bss_seg(".bss$ge11b70")
int g_lives;
#pragma bss_seg(".bss$ge11b74")
int g_lastLevelBuffered;
#pragma bss_seg(".bss$ge11b78")
float g_shopShake;
#pragma bss_seg(".bss$ge11b7c")
int g_engineDroneFreq;
#pragma bss_seg(".bss$ge11b80")
int g_engineDroneFreqStep;
#pragma bss_seg(".bss$ge11b84")
int g_completePopupYOffset;
#pragma bss_seg(".bss$ge11b88")
unsigned int g_moneySuckerCooldown;
#pragma bss_seg(".bss$ge11b8c")
int g_aiRight;
#pragma bss_seg(".bss$ge11b90")
int g_aiLeft;
#pragma bss_seg(".bss$ge11b94")
unsigned int g_sndTHit3;
#pragma bss_seg(".bss$ge11b98")
unsigned int g_sndTHit4;
#pragma bss_seg(".bss$ge11b9c")
unsigned int g_sndTHit1;
#pragma bss_seg(".bss$ge11ba0")
unsigned int g_sndTHit2;
#pragma bss_seg(".bss$ge11ba4")
unsigned int g_sndTCurrent;
#pragma bss_seg(".bss$ge11ba8")
unsigned int g_sndTExplo1;
#pragma bss_seg(".bss$ge11bac")
unsigned int g_sndTExplo2;
#pragma bss_seg(".bss$ge11bb0")
unsigned int g_sndTExplo4;
#pragma bss_seg(".bss$ge11bb4")
unsigned int g_sndTExplo5;
#pragma bss_seg(".bss$ge11bb8")
unsigned int g_sndTMouww;
#pragma bss_seg(".bss$ge11bbc")
int g_bindingsBannerUntil;
#pragma bss_seg(".bss$ge11bc0")
int g_memUntil;
#pragma bss_seg(".bss$ge11bc4")
int g_hiscoreStarCount;
#pragma bss_seg(".bss$ge11bc8")
int g_hiscoreStarBrightness1;
#pragma bss_seg(".bss$ge11bcc")
int g_hiscoreStarBrightness2;
#pragma bss_seg(".bss$ge11bd0")
int g_hiscoreStarBrightness3;
#pragma bss_seg(".bss$ge11bd4")
int g_tipIndex;
#pragma bss_seg(".bss$ge11bd8")
int g_promoRingIndex;
#pragma bss_seg(".bss$ge11bdc")
int g_streakColorB;
#pragma bss_seg(".bss$ge11be0")
float g_streakX;
#pragma bss_seg(".bss$ge11be4")
float g_streakAlpha;
#pragma bss_seg(".bss$ge11be8")
unsigned char g_streakActive;
#pragma bss_seg(".bss$ge11be9")
unsigned char g_introStageCenter;
#pragma bss_seg(".bss$ge11bea")
unsigned char g_introStageLeftWing;
#pragma bss_seg(".bss$ge11beb")
unsigned char g_introStageRightWing;
#pragma bss_seg(".bss$ge11bec")
unsigned char g_pad_e11bec[4];
#pragma bss_seg(".bss$ge11bf0")
float g_bannerShrinkSpeed;
#pragma bss_seg(".bss$ge11bf4")
float g_leftWingX;
#pragma bss_seg(".bss$ge11bf8")
float g_rightWingX;
#pragma bss_seg(".bss$ge11bfc")
float g_wordmarkOffsetX;
#pragma bss_seg(".bss$ge11c00")
float g_convergeFlash;
#pragma bss_seg(".bss$ge11c04")
unsigned char g_introStageWordmark;
#pragma bss_seg(".bss$ge11c05")
unsigned char g_introStageBanner;
#pragma bss_seg(".bss$ge11c06")
unsigned char g_introStageFlash;
#pragma bss_seg(".bss$ge11c07")
unsigned char g_introStageFinal;
#pragma bss_seg(".bss$ge11c08")
float g_starScale;
#pragma bss_seg(".bss$ge11c0c")
float g_starAlpha;
#pragma bss_seg(".bss$ge11c10")
float g_starAlphaStep;
#pragma bss_seg(".bss$ge11c14")
unsigned char g_introSoundPlayed[9];
#pragma bss_seg(".bss$ge11c1d")
unsigned char g_introBlockedByWindow;
#pragma bss_seg(".bss$ge11c1e")
unsigned char g_autoConfirmName;
#pragma bss_seg(".bss$ge11c1f")
bool g_promoContinueBlink;
#pragma bss_seg(".bss$ge11c20")
unsigned char g_pad_e11c20[8];
#pragma bss_seg(".bss$ge11c58")
float g_wobble;
#pragma bss_seg(".bss$ge11c5c")
int g_moneyW0;
#pragma bss_seg(".bss$ge11c60")
int g_moneyW1;
#pragma bss_seg(".bss$ge11c64")
int g_moneyW2;
#pragma bss_seg(".bss$ge11c68")
int g_moneyW3;
#pragma bss_seg(".bss$ge11c6c")
unsigned int g_promoBlinkTimer;
#pragma bss_seg(".bss$ge11c70")
int g_debrisSpawnedCount;
#pragma bss_seg(".bss$ge11c74")
int g_secretBirdHitCount;
#pragma bss_seg(".bss$ge11c78")
int g_flare0;
#pragma bss_seg(".bss$ge11c7c")
int g_flare1;
#pragma bss_seg(".bss$ge11c80")
int g_flare2;
#pragma bss_seg(".bss$ge11c84")
int g_flare3;
#pragma bss_seg(".bss$ge11c88")
int g_shipX;
#pragma bss_seg(".bss$ge11c8c")
int g_shipY;
#pragma bss_seg(".bss$ge11c90")
float g_shieldAngle;
#pragma bss_seg(".bss$ge11c94")
int g_flameXP0;
#pragma bss_seg(".bss$ge11c98")
int g_flameXP1;
#pragma bss_seg(".bss$ge11c9c")
int g_scoopXP0;
#pragma bss_seg(".bss$ge11ca0")
int g_scoopXP1;
#pragma bss_seg(".bss$ge11ca4")
int g_flameXP0Alt;
#pragma bss_seg(".bss$ge11ca8")
int g_flameXP1Alt;
#pragma bss_seg(".bss$ge11cac")
int g_scoopXP0Alt;
#pragma bss_seg(".bss$ge11cb0")
int g_scoopXP1Alt;
#pragma bss_seg(".bss$ge11cb4")
int g_perfectAny;
#pragma bss_seg(".bss$ge11cb8")
int g_perfectAwardedP0;
#pragma bss_seg(".bss$ge11cbc")
int g_perfectAwardedP1;
// g_endG: defined in static_init.c (dynamic initializer), section .bss$ge11cc0
// g_endR: defined in static_init.c (dynamic initializer), section .bss$ge11cc4
// g_endB: defined in static_init.c (dynamic initializer), section .bss$ge11cc8
// g_floorY: defined in static_init.c (dynamic initializer), section .bss$ge11ccc
// g_saveMsgBlinkRate: defined in static_init.c (dynamic initializer), section .bss$ge11cd0
#pragma bss_seg(".bss$ge11cd4")
unsigned char g_pad_e11cd4[4];
#pragma bss_seg(".bss$ge11d3c")
unsigned char g_pad_e11d3c[4];
#pragma bss_seg(".bss$ge12028")
unsigned char g_pad_e12028[1296328];
#pragma bss_seg(".bss$gf4e7f0")
int g_ovX1;
#pragma bss_seg(".bss$gf4e7f4")
int g_ovY1;
#pragma bss_seg(".bss$gf4e7f8")
int g_ovX2;
#pragma bss_seg(".bss$gf4e7fc")
int g_ovY2;
#pragma bss_seg(".bss$gf4e800")
__int64 g_timeMarkC;
#pragma bss_seg(".bss$gf4e808")
BlitItem g_blit2[2000];
#pragma bss_seg(".bss$gf5e208")
__int64 g_timerEnd1;
#pragma bss_seg(".bss$gf5e210")
float g_sinTableFine[3600];
#pragma bss_seg(".bss$gf61a50")
SysDate g_sysTime;
#pragma bss_seg(".bss$gf61a60")
__int64 g_timerMin2;
#pragma bss_seg(".bss$gf61a68")
BlitItem g_blit[2000];
#pragma bss_seg(".bss$gf71468")
__int64 g_timerStart2;
#pragma bss_seg(".bss$gf71470")
char g_numBuf[260];
#pragma bss_seg(".bss$gf71574")
int g_ovW;
#pragma bss_seg(".bss$gf71578")
int g_offYB;
#pragma bss_seg(".bss$gf7157c")
int g_idxA;
#pragma bss_seg(".bss$gf71580")
StretchItemRot g_stretchRot[1000];
#pragma bss_seg(".bss$gf79280")
StretchItemI g_stretchI[150];
#pragma bss_seg(".bss$gf7a090")
int g_offXB;
#pragma bss_seg(".bss$gf7a094")
int g_ovH;
#pragma bss_seg(".bss$gf7a198")
char g_keyName[260];
#pragma bss_seg(".bss$gf7a29c")
int g_idxB;
#pragma bss_seg(".bss$gf7a2a8")
__int64 g_timeD;
#pragma bss_seg(".bss$gf7a2b0")
__int64 g_timeE;
#pragma bss_seg(".bss$gf7a2b8")
unsigned char g_pad_f7a2b8[4];
#pragma bss_seg(".bss$gf7a2bc")
int g_pendingLevelsPlayed;
#pragma bss_seg(".bss$gf7a2c8")
QuadItem g_quad[100];
#pragma bss_seg(".bss$gf7b3f8")
__int64 g_timerMin1;
#pragma bss_seg(".bss$gf7b400")
int g_offXA;
#pragma bss_seg(".bss$gf7b408")
__int64 g_timerStart1;
#pragma bss_seg(".bss$gf7b410")
__int64 g_timeMarkB;
#pragma bss_seg(".bss$gf7b418")
int g_offYA;
#pragma bss_seg(".bss$gf7b41c")
int g_moneyMax;
#pragma bss_seg(".bss$gf7b420")
StretchItemRot g_stretchRot2[20];
#pragma bss_seg(".bss$gf7b6a0")
StretchItemF g_stretchF[1500];
#pragma bss_seg(".bss$gf85ab0")
__int64 g_timeA;
#pragma bss_seg(".bss$gf85ab8")
__int64 g_timerEnd2;
#pragma bss_seg(".bss$gf85ac8")
float g_cosTableFine[3600];
#pragma bss_seg(".bss$gf89308")
int g_screenHInit;
#pragma bss_seg(".bss$gf8930c")
int g_offsetXi;
#pragma bss_seg(".bss$gf89310")
int g_offsetYi;
#pragma bss_seg(".bss$gf89314")
float g_offsetX;
#pragma bss_seg(".bss$gf89318")
float g_offsetY;
#pragma bss_seg(".bss$gf8931c")
float g_worldViewRotation;
#pragma bss_seg(".bss$gf89320")
float g_worldViewX;
#pragma bss_seg(".bss$gf89324")
float g_worldViewY;
#pragma bss_seg(".bss$gf89328")
int g_stretchICount;
#pragma bss_seg(".bss$gf8932c")
unsigned char g_pad_f8932c[4];
#pragma bss_seg(".bss$gf89330")
int g_stretchFCount;
#pragma bss_seg(".bss$gf89334")
unsigned char g_pad_f89334[4];
#pragma bss_seg(".bss$gf89338")
int g_stretchRotCount;
#pragma bss_seg(".bss$gf8933c")
unsigned char g_pad_f8933c[4];
#pragma bss_seg(".bss$gf89340")
int g_stretchRot2Count;
#pragma bss_seg(".bss$gf89344")
unsigned char g_pad_f89344[4];
#pragma bss_seg(".bss$gf89348")
int g_blitCount;
#pragma bss_seg(".bss$gf8934c")
int g_blit2Count;
#pragma bss_seg(".bss$gf89350")
int g_blit3Count;
#pragma bss_seg(".bss$gf89354")
int g_quadCount;
#pragma bss_seg(".bss$gf89358")
int g_quadCountMax;
#pragma data_seg()
#pragma bss_seg()
