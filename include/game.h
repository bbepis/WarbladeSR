#pragma once
// Prototypes of every game function the decompiled code defines or calls, grouped
// by the file that defines it (address order).
#include "types.h"
#include "constants.h"
#include "macros.h"

// ---- util.c (0x52ea50)
void SeedRand(unsigned int seed);
int StrLen(char *s);
int StrHash(char *s);
void InitTrigTables();
void DrawStarField(int count, float *xs, float *ys, float *zs, int minY, int maxY, int minX, int maxX, float zNear, float zFar, float speed, bool bright);
void ClearSlots();
void SpawnSlots(float x, float y, int n, int r, int g, int b);
int RandRange(int lo, int hi);
unsigned int XorShift();
float RandFloat(float lo, float hi);
void UpdateSlots(float dt);
void DrawSlots(int minX, int maxX, int minY, int maxY);
char ToLower(char c);
void StrToLowerN(const char *src, char *dst, int max);
Image *LoadGraphic(char *name, bool maskAlpha, bool hiQuality);
Image *LoadGraphic2(char *name, bool maskAlpha, bool hiQuality);

// ---- resources.c (0x5300a0)
void *LoadHma(const char *name, int w, int h);
void DrawImage(Image *g, int x1, int y1, int x2, int y2, unsigned char r, unsigned char gr, unsigned char b, unsigned char a, int unused, float angle);
char *Concat3(const char *a, const char *b, const char *c);
bool VoiceExists(int voice);
char *VoicePath(const char *name);
char *SamplePath(const char *name);

// ---- music.c (0x5308f0)
void FreePlaylist();
void LoadPlaylist();
int ParsePlaylist();
void BackslashToSlash(char *s);
char *StrLower(char *s);
bool OpenNextSong();
int PlayRandomPlaylistSong();
char *StrContains(char *s, const char *sub);
void StopStream(AudioHandle h);
void PlayNextMusic();
void InitSampleTable();
void SetSfxVolume(int vol);
void ApplyMusicVolume();
void SetMusicVolTable(int vol);

// (0x531900)
void BuildRampTables();

// (0x531a50)
void StartMusic();

// ---- sound.c (0x532a10)
void SoundResetQueue();
int SampleLengthMs(AudioHandle sample);
void SoundQueueAdd(AudioHandle sample, int delay, int vol);
int SoundStop(AudioHandle ch);
void SoundPlayPending();
void SoundPlay(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2);
void SoundPlayNoFade(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2);
void SoundPlaySlide(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2, enum AudioAttrib attrib, float value, int time);
void SoundPlay2(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2);
void SoundPlayVoice(AudioHandle sample, int freq, int vol, float pan, int unused1, int unused2);
AudioHandle SoundPlayChannel(AudioHandle prev, AudioHandle sample, int freq, int vol, float pan, int a6);
void SoundQueueUpdate();
void SoundStopAll();
void SoundRestart();
void SoundPause();
void SoundResume();

// (0x533f30)
AudioHandle LoadSample(const char *name, AudioHandle max);
AudioHandle LoadSampleLoop(const char *name, AudioHandle max);

// (0x534250)
void FreeSamples();

// (0x5351d0)
AudioHandle LoadVoiceSample(const char *name, int mode);
void LoadVoices();
void InitSound();

// (0x536ca0)
void SoundShutdown();

// ---- savegame.c (0x537c80)
void SaveProfile(int profile);
void AutoSaveProfile(int profile);
void SetupDifficulty();

// (0x5384f0)
void LoadSuspended(int profile);

// (0x539aa0)
bool ProfileValid(int profile);
void DeleteProfile(int profile);
void MakeProfilesDir();

// ---- savefile.c (not in the original: the explicit suspended-game format)
bool SaveGameToFile(const char *path);
bool SaveFileValid(const char *path);
bool LoadGameFromFile(const char *path);
void MakeGameDir();
__int64 MakeRandomId();
int Rand7f();
int Randff();
int Rand1ff();
void WinCloseAll();
void WinInit();
int WinFindFree();
int WinOpen(int x, int y, int w, int h, int mode);
void WinHideAll();
void WinClose(int win);

// ---- window.c (0x53ae00)
void WinAddText(int x, int y, int win, char *text, int color);
void WinAddTextPair(int x, int y, int win, int id, char *text1, char *text2, int a, int b);
void WinAddRect(int a, int b, int c, int d, int win, Image *e);
void WinAddItemA(int a, int b, int c, int d, int e, int f, int g, int h, int win, void *j);
void WinAddItemB(int a, int b, int c, int d, int e, int f, char *text, int win);
void EmptyWindowStub();
void WinAddMenuItem(int x, int y, int win, int id, char *text, int param);
void WinSetSelected(int win, int sel);
void WinCheckMenuItem(int win, int id);
void WinClearMenuChecks();
void WinAddToggle(int x, int y, int win, bool value, char *text, bool flag2);
void WinAddEdit(int x, int y, int win, int len, unsigned char c, int param, unsigned char d);
bool AnyWindowActive();
bool AnyWindowHasEdit();

// (0x53c630)
void WinFocusNextEdit(int win);

// (0x53c870)
void WinDraw(int win);

// (0x540400)
float FabsWindow(float x);
float FabsF(float x);
void WinUpdateAll();
bool ReturnTrueNet();
void DoNothing();

// ---- account.c (0x540fb0)
void EmptyPostTransitionHook();
void EmptyViewChangeHook();
void SaveSetPro();
void DeleteSetPro();

// (0x541280)
void LoadProfile();

// (0x5421a0)
void UpdateMusicPos();
void DrawFps();
void DrawRow(int x, int y, int col, int count);
void PlaySample();
void ActivateProfile(int slot);
#ifdef __EMSCRIPTEN__
void WebNormalizeAccount(void);
void WebProcessAccountQueue(void);
void WebOpenLogin(int create);
int WebQueueCredentials(const char *username, const char *password, int create);
void WebQueueSignOut(void);
int WebLoginMode(void);
int WebAccountStatus(void);
int WebCanPlay(void);
int WebIsGuest(void);
int WebPlayGuest(void);
#endif
void ClearAccount();
void ResetAccount();
int NextAccountReset();
void SaveAccount(int profile);
void PackAccount(int profile);
void UnpackAccount(int profile);
__int64 LoadBackupAccount(int profile);
__int64 GetAccountTime(int profile);
int LoadAccount(int profile);

// (0x5430f0)
bool DecodeAccount(void *buf, int profile, int len);

// (0x5447e0)
void ScanProfiles();
void MergeSettings(int slot);
void GetProfileName(int slot);
bool GetProfileEasyFlag(int slot);
bool GetProfileCfgFlag(int slot);
int GetProfileVoiceIndex(int slot);
void SetProfileVoiceIndex(int slot, int v);
void SetProfileLastSaveId(int slot, __int64 v);
__int64 GetProfileLastSaveId(int slot);
void DecProfileLives(int slot);
void IncProfileLives(int slot);
int GetProfileLives(int slot);
void ResetProfileLives(int slot);
bool ProfileHistHas(int slot, __int64 v);
void ProfileHistPush(int slot, __int64 v);

// ---- stats.c (0x546080)
void ShowHighScore(int profile);
void UpdateHighScore(int profile, __int64 v);
void ShowMarathonScore(int profile);
void UpdateMarathonScore(int profile, __int64 v);
void ShowMeteorStormScore(int profile);
void UpdateMeteorStormScore(int profile, __int64 v);
void ShowTimeTrialScore(int profile);
void UpdateTimeTrialScore(int profile, __int64 v);
void ShowPerfectAttempts(int profile);
void ShowPerfectCount(int profile);
void AddStats(int profile, int a, int b);
void ShowRatio(int profile);
void ShowPlayTime(int profile);
void AddPlayTime(int profile, __int64 a, __int64 b, __int64 c);
void SetStat(int profile, int v);
int GetStat(int profile);

// (0x546eb0)
int GetRank(int slot);
void IncrementRank(int slot);
int GetMedalOrder(int slot, int idx);
void AwardMedal(int slot, int medal);
int GetMedals(int slot);
void ClearMedalOrder(int slot);
void ClearMedalsMask(int slot, int mask);
void ClearLevelsDone(int slot);
void ClearRecordStat(int slot);
void RescaleRatioStat(int slot);
bool HasAllMedals(int slot);
void CheckRatioMedal(int slot);
void CheckAllLevelsMedal(int slot);
void FormatBestTime(int slot);
void UpdateBestTime(int slot, __int64 t);
void AddScoreStat(int slot, int n);

// (0x5487f0)
void AddHitsStat(int id, int amount);
void ShowFastestMeteorstorm(int id);
void UpdateFastestMeteorStorm(int id, __int64 t);
void ShowSecretsFound(int id);
void ShowSecretsInOneGame(int id);
void SetGameCompleted(int id);
bool IsGameCompleted(int id);
void MarkSecretFound(int id, int level);
bool IsSecretFound(int id, int level);
void ShowHighestLevelReached(int id);
void UpdateHighestLevel(int id, int value);
void ShowTotalLevelsPlayed(int id);
void AddLevelsPlayed(int id, int amount);
void ShowTotalGamesPlayed(int id);
void IncrementGamesPlayed(int id);

// (0x5498e0)
void FormatHitPctAbove25(int profile);
void UpdateHitPctAbove25(int profile, int value);
void FormatTotalHitPct(int profile);
void FormatHighestMoney(int profile);
void UpdateHighestMoney(int profile, int value);
void FormatHighestRank(int profile);
void UpdateHighestRank(int profile, int value);
void QuitGameDialog();
void QuitToWindowsDialog();
int MedalIconOffsetY(int n);

// (0x54a260)
void ProfileWindow(bool noButtons);

// (0x54d440)
void ApplyStatUnlocks();

// ---- render.c (0x54e1f0)
int EmptyEscGateCheck();
void QueueStretchI(Image *graphic, float x1, float y1, float x2, float y2, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void QueueStretchF(Image *graphic, float x1, float y1, float x2, float y2, unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned char flag);
void QueueStretchRot(Image *graphic, float x1, float y1, float x2, float y2, unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned char flag, float angle);
void QueueStretchRot2(Image *graphic, float x1, float y1, float x2, float y2, unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned char flag, float angle);
void QueueBlit(float destX, float destY, Image *graphic, Rect16 *src);
void QueueBlit2(float destX, float destY, Image *graphic, Rect16 *src);
void FlushStretchI();
void FlushStretchF();
void FlushStretchRot();
void FlushStretchRot2();
void FlushBlit(void *dst);
void FlushBlit2(void *dst);
void QueueQuad(Image *unused, float v0, float v1, float v2, float v3, Image *param, float v4, float v5, float v6, float v7);

// (0x54f6d0)
void FlushQuads(void *dst);
void DrawStretch(Image *graphic, int dx, int dy, int dw, int dh, int sx, int sy, int sw, int sh);
void ClearParticles();
int CountParticles();
void AddParticle(Image *graphic, int x, int y, float size, float sizeVel, float angle, float angleVel, int dir, int r, int g, int b, int alpha, float life, float speed, int spawn, float gravity, int mode, int *xref, int *kill, unsigned char flag);
void UpdateParticles();
void DrawParticles();

// (0x550900)
void DrawBackground();

// ---- effects.c (0x552040)
void InitStarRotation();
void CycleMusicFormat();
void PlayClick();
int IsSpecialLevel(int unused);
void SetHurryUpTimer();
int PlayerHasAllMarks(int idx);
unsigned int GetHurryUpTimer();
int IsGameOver();

// (0x5529b0)
int TallyStep(int count);

// (0x553d90)
void SpawnSpark(int x, int y, int r, int g, int b, int type, float speed, int angle, int delay, int fade, int trailInterval, int size);
void UpdateSparks();
void SpawnFirework();
void AddSparkleFlash(Image *graphic, int x, int y, int size, int r, int g, int b, int alpha, int fade);
void UpdateSparkleFlashes();
int ReturnZero();
void ResetPlayerTimers();
void NoOpEffects();
void ClearPlayers();
int ScanLeft(unsigned char *data, int left, int top, int width, int height, int pitch, int unusedImgH);
int ScanTop(unsigned char *data, int left, int top, int width, int height, int pitch, int unusedImgH);
int ScanRight(unsigned char *data, int left, int top, int width, int height, int pitch, int unusedImgH);
int ScanBottom(unsigned char *data, int left, int top, int width, int height, int pitch, int unusedImgH);

// (0x5555f0)
void ScanFrameRects(void *img, int w, int h);
void CacheFrameRect(int i);
void InitFrameRectDefaults(int i);
void CreditKill(int p);
void CreditEscape(int p);
void LevelStallWatchdog();
void StartNextLevel();
void AfterGemPickup();
void DrawMenuPrompt();
void DrawSavePrompt();
void DrawPleaseWait();
int CountTimeTrialLevels();
int CheckTimeTrialAvailable();
int CountClassicLevels();

// ---- level.c (0x557330)
void FreeBuffers();
int CopyBytes(void *src, void *dst, int size);
void ReleaseAlienGfxCache();
bool FindOtherSlotWithKey(int slot, int chan, int key);
void BumpAlienGfxAge(Image *snd);
void DropAlienGfxAge(Image *snd);
void StealOldestAlienGfx(int owner, short id);

// (0x558d60)
void LoadLevelData();

// (0x55aa10)
int PackLevelData(int slot, int level, short mode);

// (0x55cbc0)
void LoadingScreen(int n);
void BufferAllLevels();
void FreeLevelBufs();
void CountMalfunctionLevels();
void SwitchPlayer();
int CheckProfileBonus();
void FreeShopBgGfx();
void FreeShopItemGfx();
void LoadShopPic(int idx);
void FreeSecretPicGfx();
int LoadSecretPic(int idx);
void FreeSecretScreenGfx();

// ---- splash.c (0x55e630)
void ShowLogoSplash();
void ShowTitleSplash();
void ResetF893();
int CenturyLevelFlag();
int ReloadSecret();
int SplashReadyStub();
void EmptySplashStub();
void AddCash();
void PlayBuzzerSfx();
void PlayBuzzer2Sfx();
void PlaySlideSfx();
void DrawFlash();
void Frame();
void NewRank();

// ---- shop.c (0x55f650)
void Shop();

// ---- levelstart.c (0x5674a0)
void ClipCursorOn();
void ClipCursorOff();
int ApplyBgTint();
void PickBgTint();
void FadeLoopSamples();
void LoadPatterns();
int LoadClassicLevel(int n);
int LoadTimeTrialLevel(int n);
int LoadMalfunctionLevel(int n);
void SetDifficulty();
void LoadBestScore();
void InitPlayerStats(int slot, int player);
void UpdateBestScore();
int StatSnapshotStub();

// (0x569260)
void StartLevel();

// ---- items.c (0x56ff10)
void SpawnBonus(float x, float y);
void SpawnItem(float x, float y, unsigned char rare);
void SpawnWeaponItem(int x, int y);
void SpawnPowerup(float x, float y);
void SpawnPowerupBurst(int x, int y, unsigned char preferBlueMoney, unsigned char bigBurst);
void SpawnGem(float x, float y);
void PlayerHit(int p);

// (0x571c60)
void Pickup(int type);

// (0x581250)
void SpawnMoneySucker();
void SpawnEliteFlyby();

// (0x582120)
void WarpMalfunction();

// ---- collide.c (0x5834b0)
void ItemsVsPlayer();
void FindTargetItem();
void FindTargetObj();
void FindTargetGem();
void FindTargetMeteor();
void ItemsVsBothPlayers();
bool BoxOverlap(int type, int x, int y, int left, int top, int right, int bottom);

// (0x5842c0)
void BulletsVsPlayer();

// (0x585620)
void BulletsVsBothPlayers();
int IsEnemyWaveCleared();
bool IsHurryUpEnemyActive();

// (0x585840)
void PlayerShotsHitEnemies();

// (0x58d490)
void ShieldGrabEnemies();
void ShieldGrabEnemiesBothPlayers();

// ---- hurryup.c (0x58e350)
void HurryUp();

// (0x58f550)

// ---- screens.c (0x58fae0)
void AboutScreen();

// (0x591500)
void MissionScreen();

// (0x591880)
void HelpControls();

// (0x593160)
void HelpBonuses();

// (0x5942a0)
void HallOfFame();

// (0x596840)
void TallyScreen();
void GameOverScreen();
void VersusResult();
void EmptyScreenStub();

// (0x596dd0)
void BonusScreen();

// ---- main.c (0x59aac0)
bool GameInit();
void ResetObjects();
void ResetAllObjects();
void ResetObjectsKeep();
void DrawRankPromoBanner();

// (0x59c3a0)

// (0x59d710)
unsigned int GetFlagMask(unsigned char joy);
// Returns the requested legacy joystick axis.
long GetJoyX(unsigned char joy);
long GetJoyY(unsigned char joy);
void OpenCreateProfileWin();
void OnFocusChange(bool focused);

// (0x59f1a0)
int GameMain();

// ---- init.c (0x5a15b0)
float Cos2(float a);
float Cos(float a);
float Sin2(float a);
float Sin(float a);
bool ResetClip();
bool InitWindow(bool windowed);
int RendererChoiceOf(int value);
void ApplyFrameSettings(void);
const char *CycleInterpolation(void);
const char *ToggleVSync(void);
int InitWindowCfg();

// (0x5a1d50)
int LoadGameData();

// (0x5a4cc0)
int InitGame();
int InitFail(const char *msg);
void AddMenuText(int x, int y, const char *text, int page, int id, int parent, int unused, int style);
void AddMenuItem(int x, int y, const char *text, int page, int id, int parent, int unused, int style, int width);

// (0x5a56a0)
void InitMenu();

// ---- intro.c (0x5a67b0)
void HidePageButtons(int owner);
void DrawButtons(int page);
void DrawFrame(int x1, int y1, int x2, int y2, char *title);
void DrawTextBox(int x1, int y1, int x2, int y2, char *text);
void ClearFlashes();
void AddLogoFlash(int x, int y);
void UpdateLogoFlashes();
void DrawFlashes();

// (0x5a82f0)
void IntroFrame();

// ---- gameflow.c (0x5aa9b0)
void ResetToTitle();
void ShowHiscoreTable();
void UpdateGetReadyRespawn();
void UpdateGetReadyNewLevel();
void UpdateMemoryStationIntro();

// (0x5ac450)
void FinishMemoryStation();
void UpdateHiscoreSkipGate();
void UpdateMeteorStormIntroGate();
void UpdateGemDropIntroGate();
void ResumeGame();
void PauseGame();

// (0x5ad490)
void Hotkeys();

// (0x5af5f0)
void InitLayers();
void BankBonusScore();

// (0x5afc50)
void GameFrame();

// (0x5b2f40)
void NewGame(bool resetLevel);

// (0x5b3e10)
void Logout();
char KeyToChar(int key);
void WaitForKeyRelease(int key);
void ResetFlags();
int CurMonth();
int CurYear();

// (0x5b53d0)

// (0x5b6f20)

// ---- menu.c (0x5b7080)
void MenuUpdate(bool moved);

// (0x5bf570)
void SwapKeyBindings();
void TakeScreenshot();

// (0x5bfac0)
void MenuHandler();

// ---- hiscore.c (0x5c6110)
int CheckHiscore(int p);

// (0x5c6940)
void EndSequence();

// (0x5c7cf0)
void PostRoundIdleTimeout();
void EmptyShowHiscoreTableHook();
void ResetBlitCounters();
void ClearMoneyHiscoreHighlight(int p);
void ClearHiscoreHighlightTT(int p);
void ClearHiscoreHighlight(int p);
void UpdateGameOverSequence();

// (0x5c8ee0)
void EnterHiscore();

// ---- controls.c (0x5cb410)
void KeyName(int key);
void ButtonName(int type, int button);
void FixDuplicateKeys(int player);
void TrimSpaces(char *s);
void ControlsText(int player);

// (0x5cce90)
void GetPressedKeyName();

// (0x5cdd40)
void ConfigInputMenu();

// ---- text.c (0x5cfcd0)
void DrawMenuText(const char *text, int x, int y, int row);
void DrawScoreDigits(const char *text, int x, int y, int a, float scale);
void DrawTinyText(const char *text, int x, int y, int row);
void DrawTinyText2(const char *text, int x, int y, int row);

// (0x5d1740)
void DrawMixedCaseText(const char *text, int x, int y, int flags);
void DrawNewsText(char *text, int x, int y, int font);

// (0x5d2560)
void DrawBigText(int x, int y, const char *text);

// (0x5d44c0)
void DrawTextFrame(const char *text, int x, int y, int style);
void DrawTinyTextAlt(const char *text, int x, int y, int row);
void Blit(int x, int y, Image *dst, Image *graphic, int sx, int sy, int w, int h);
void Blit2(int x, int y, void *dst, Image *graphic, int sx, int sy, int w, int h);
void BlitLocal(int x, int y, void *dst, Image *graphic, int sx, int sy, int w, int h);
void BlitLocal2(int x, int y, Image *dst, Image *graphic, int sx, int sy, int w, int h);
void EmptyTextStubA();
void EmptyTextStubB();

// ---- hud.c (0x5d59f0)
void Hud(int p, int x, int meterX);

// (0x5d82c0)
void CompressHiscores();
void ClearHiscores();
void DecompressHiscores();
void DrawHudTimed();
void DrawHud1P();
void DrawHud2P();
void DrawHud2PCoop();
void WriteSettings();
void DefaultSettings();
void LoadSettings();
void WriteHiscoreFile();

// (0x5d9c20)
void ResetHiscores();
void LoadHiscores();
void FlipBuffer(int a);
void SetViewHud();
void SetView();
void RenderGameplayFrame();
void RenderFrame2();

// (0x5dae80)
void DrawRankPromoHud();

// (0x5dc490)
void MemoryStationText();

// (0x5dcd80)
void DrawMeteorStorm();

// ---- fx.c (0x5de1a0)
void DrawGemDropBanner();
void RenderMemoryStationFrame();
void RenderMeteorStormRaceFrame();
void RenderMeteorStormResultsFrame();
void RenderShopGateFrame();
void RenderGemDropFrame();
void SpawnSparks(int count, float x, float y, float spdLo, float spdHi, float accLo, float accHi);
void SpawnSparksRGB(int count, float x, float y, float spdLo, float spdHi, float accLo, float accHi, int r, int g, int b);
int SpawnDebris(float x, float y, int player, int type, float dmg, int countsAsShot, float speed, float push);

// (0x5dfee0)
void SpawnSmall(float x, float y);
void SpawnExplosion(float x, float y, int w, int h, int life, int type, int p20, int r, int g, int b, int r2, int g2, int b2);
void SpawnBigExplosion(float x, float y, int w, int h, int type, int p20, int r, int g, int b);
void SpawnHugeExplosion(float x, float y, int w, int h, int type, int p20, int r, int g, int b, int mode);
void FirePlayer(int level, int countsAsShot);
int AllTypesUnique();
int CountTypePairs();

// (0x5e16c0)
void MemoryBonusUpdate();

// ---- player.c (0x5ea0f0)
bool IsKeyFree(int key);
int InputLeft(int p);
int InputRight(int p);
int InputDown(int p);
int InputUp(int p);
int InputFire(int p);
int InputRocket(int p);
int InputPause(int p);
int InputProfile(int p);
int InputMenuFire(int p);

// (0x5eb550)
void UpdatePlayer();

// (0x5ed9b0)
void UpdatePlayers();

// (0x5eda60)
void StateDemo();

// (0x5ef1b0)
void DrawPlayerShipFx(bool alt);

// (0x5f2c00)
void ShipHud();
void ShipHudAll();
void AddScorePopup(int x, int y, __int64 value, bool big);
void UpdateScorePopups();
void DrawScorePopups();
void DrawNumberRow(const char *s, int x, int y, int row);
void SpawnItems();

// ---- bonusround.c (0x5f4210)
void UpdateItems();
void UpdateStarItems();

// (0x5f4e10)
void UpdateBonusResultsHud();

// (0x5f6fa0)
void DrawSprites();
void DrawWarpRing();
void DrawWarpFlash();
void InitGrid();
void SpawnMeteor(int idx);
void InitMeteorStormLevel();
void InitGemDropLevel();
void PlayGameMusic();
void PlayTitleMusic();
void EmptyMusicStubA();
void PlayTimeTrialMusic();
void EmptyMusicStubB();
void PlayBossMusic();
void EmptyMusicStubC();
void PlayMemoryStationMusic();
void OnMemoryStationLevelEnd();

// (0x5f8c50)
void PlayHiscoreMusic();
void EnsureTitleMusic();
void PlayEndMusic();
void EmptyMusicStubD();
void PlayShopMusic();
void EmptyMusicStubE();
void PlayMeteorStormMusic();
void OnMeteorStormLevelEnd();
void PlayGemDropMusic();
void OnGemDropLevelEnd();

// (0x5f8fe0)
void PlayRankPromotionSounds();

// ---- levelupdate.c (0x5f9f30)
void OnLevelComplete();

// (0x5f9f60)
void MeteorStormUpdate();

// (0x5fc750)
void DrawSpeedMeter();
void DrawMeteors();
void DrawMemoryGrid();
void DrawMemoryCursor();
void DrawFallingGems();

// (0x5fda70)
void MeteorStormCollide();

// (0x6003c0)
void GemDropCollide(int p);

// (0x6014f0)
void GemDropUpdate();

// ---- levelobj.c (0x601cd0)
void UpdateLevelObjects();

// (0x603290)
float Sqrt(float x);
float Sqrtf(float x);

// (0x603360)
void DrawLevelObjectsNormal();

// (0x604b90)
void DrawLevelObjectsBright();

// (0x605ef0)
int NearPlayerFireBoost(int x, int val);

// ---- enemies.c (0x605fe0)
void UpdateEnemies();

// (0x618560)
void DrawEnemies();

// ---- stars.c (0x61b610)
void UpdateHyperspace();

// (0x61ca10)
void UpdateMenuStars();
void DrawStarsPlayer();
void DrawStarsStill();
void DrawDotGlyph(int x, int y, int scale, int count, int *xs, int *ys);
void DrawDotSignature();
void DrawSpriteStars();

// (0x61e700)
void DrawFlightFrame();
void DrawStarsGlow();
void DrawBossBar();
void NoOpStars();
void DrawBorders();

// ---- mapobj.c (0x61fff0)
void UpdateMapObjects();

// (0x6211e0)
void DrawMapObjects();

// ---- explosions.c (0x622150)
void UpdateExplosions();
void DrawExplosions();
void UpdateExplosionDebris();
float FabsExplosion(float x);
void DrawExplosionDebris();
void PlacePlayer(int p);

// (0x623980)
void InitPlayer(int p);

// ---- platform.c (0x624a20)
void SetStateByMode();
void InitTablePtrs();
int ParseInt(char **p, int def);
void Shutdown();
void BuildSinCos();
int Int64ToStrGrouped(__int64 v, char *out);
int Int64ToStr(__int64 v, char *out);
void CopyBytesAt(char *dst, char *src, int off, int n);
void CopyStrAt(char *dst, char *src, int off, int n);
int StrLenPlat(char *s);
int MaskCollide(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned char *maskA, unsigned char *maskB, Rect16 boxA, Rect16 boxB, int pitchA, int pitchB, int hA, int hB, int enable);
int MaskCollide2(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, unsigned char *maskA, unsigned char *maskB, int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2, int pitchA, int pitchB, int hA, int hB, int enable);
void StrUpper(char *s);

// (0x6262d0)
int GetDaySeconds();
void LogInit();
void LogPrint(const char *s);

// (0x626ce0)
void StampTimeA();
void StampTimeB();
void StampTimeC();
void StampTimeD();
void StampTimeE();
void TimerStart1();
void TimerStop1();
void TimerStart2();
void TimerStop2();
void TimerReset1();
void TimerReset2();

// (0x6274d0)
int ClampX(int x);
int ClampY(int y);

// ---- weblinks.c (not in the original: links go to Wayback Machine snapshots)
const char *ArchiveUrl(const char *url);

// static_init.c
void InitStaticGlobals(void);
void OpenUrl(const char *url);
void BeforeOpenLink();

// ---- main.c (not in the original)
void LogCrashReport(const char *reason);

// ---- cheats.c (not in the original)
extern bool g_cheatsOn;
extern bool g_cheatGodMode;
void CheatHotkeys();
