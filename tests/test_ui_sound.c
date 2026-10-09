// Tests for src/audio/sound.c (sound effects, the delayed voice queue, pending slots, voice
// packs, volume tables) and src/audio/music.c (the playlist, track selection and switching).
// Plays are observed through the fake engine (FakePlayCount, FakeSampleName).
#include <unistd.h>
#include <sys/stat.h>
#include "support.h"

static AudioHandle Sample(const char *name)
{
    char path[128];
    snprintf(path, sizeof path, "data\\samples\\%s.wav", name);
    return SampleLoad(path, 4, 0);
}

static void SoundOn(void)
{
    g_soundEnabled = 1;
    g_cfg.sfxOn = 1;
    SetSfxVolume(255);
    SetMusicVolTable(255);
}

// ---- playing sounds ----

TEST(ui_SoundPlay_plays_a_new_voice_of_the_sample)
{
    SoundOn();
    AudioHandle hit = Sample("hit1");
    SoundPlay(hit, 22050, 200, 0.5f, 0, 0);
    SoundPlay(hit, -1, 200, 0.0f, 0, 0);
    CHECK_EQ_INT(FakePlayCount("hit1"), 2);
    CHECK_EQ_INT(g_fake.voicesStarted, 2);
    CHECK_STR(FakeSampleName(g_fake.lastPlayed), "data\\samples\\hit1.wav");
}

TEST(ui_SoundPlay_is_silent_without_sound_or_sample)
{
    SoundOn();
    AudioHandle hit = Sample("hit1");
    SoundPlay(0, -1, 200, 0, 0, 0);
    g_soundEnabled = 0;
    SoundPlay(hit, -1, 200, 0, 0, 0);
    SoundPlayNoFade(hit, -1, 200, 0, 0, 0);
    SoundPlaySlide(hit, -1, 200, 0, 0, 0, AUDIO_PAN, 1.0f, 100);
    SoundPlay2(hit, -1, 200, 0, 0, 0);
    SoundPlayVoice(hit, -1, 200, 0, 0, 0);
    CHECK_EQ_INT(SoundPlayChannel(0, hit, -1, 200, 0, 0), 0);
    CHECK_EQ_INT(g_fake.chanPlays, 0);
}

TEST(ui_SoundPlay_variants_each_play_once)
{
    SoundOn();
    AudioHandle a = Sample("a"), b = Sample("b"), c = Sample("c"), d = Sample("d");
    SoundPlayNoFade(a, 30000, 100, 0, 0, 0);
    SoundPlaySlide(b, -1, 100, 0, 0, 0, AUDIO_FREQ, 44100, 300);
    SoundPlay2(c, -1, 100, 0, 0, 0);
    SoundPlayVoice(d, -1, 100, 0, 0, 0);
    CHECK_EQ_INT(FakePlayCount("a.wav"), 1);
    CHECK_EQ_INT(FakePlayCount("b.wav"), 1);
    CHECK_EQ_INT(FakePlayCount("c.wav"), 1);
    CHECK_EQ_INT(FakePlayCount("d.wav"), 1);
}

TEST(ui_SoundPlayChannel_returns_the_new_channel)
{
    SoundOn();
    AudioHandle hum = Sample("hum");
    AudioHandle ch = SoundPlayChannel(0, hum, -1, 200, 0, 0);
    CHECK_NE_INT(ch, 0);
    CHECK_STR(FakeSampleName(ch), "data\\samples\\hum.wav");
    AudioHandle ch2 = SoundPlayChannel(ch, hum, -1, 200, 0, 0);
    CHECK_NE_INT(ch2, ch);
    CHECK_EQ_INT(FakePlayCount("hum"), 2);
    CHECK_EQ_INT(SoundStop(ch2), 0);
}

TEST(ui_PlaySample_and_PlayClick)
{
    BootGame();
    int min = FakePlayCount("minimize"), tast = FakePlayCount("tast");
    PlaySample();
    CHECK_EQ_INT(FakePlayCount("minimize"), min + 1);
    g_time = 5000;
    PlayClick();
    CHECK_EQ_INT(FakePlayCount("tast"), tast + 1);
    CHECK_EQ_INT(g_menuIdleTimeout, 65000);
}

TEST(ui_SoundPlayPending_plays_and_clears_the_slots)
{
    SoundOn();
    AudioHandle a = Sample("a"), b = Sample("b");
    g_pend0 = a;
    g_pend0Freq = -1;
    g_pend0Vol = 100;
    g_pend3 = b;
    g_pend3Freq = 20000;
    g_pend3Vol = 50;
    g_pend5 = a;
    g_pend5Freq = -1;
    g_pend5Vol = -1 + 1;
    SoundPlayPending();
    CHECK_EQ_INT(FakePlayCount("a.wav"), 2);
    CHECK_EQ_INT(FakePlayCount("b.wav"), 1);
    CHECK_EQ_INT(g_pend0, 0);
    CHECK_EQ_INT(g_pend3, 0);
    CHECK_EQ_INT(g_pend5, 0);
    SoundPlayPending();
    CHECK_EQ_INT(g_fake.chanPlays, 3);
}

TEST(ui_SoundResetQueue_clears_queue_and_slots)
{
    g_soundQueueCount = 3;
    g_pend0 = g_pend1 = g_pend2 = g_pend3 = g_pend4 = g_pend5 = 7;
    SoundResetQueue();
    CHECK_EQ_INT(g_soundQueueCount, 0);
    CHECK_EQ_INT(g_pend0 + g_pend1 + g_pend2 + g_pend3 + g_pend4 + g_pend5, 0);
}

TEST(ui_SoundStopAll_clears_the_channel_grid)
{
    SoundOn();
    g_samples[0][0] = 5;
    g_samples[3][149] = 6;
    g_chanShieldHum = 7;
    g_chanScopeHum = 8;
    SoundStopAll();
    CHECK_EQ_INT(g_samples[0][0], 0);
    CHECK_EQ_INT(g_samples[3][149], 0);
    CHECK_EQ_INT(g_chanShieldHum, 0);
    CHECK_EQ_INT(g_chanScopeHum, 0);
}

TEST(ui_SoundPause_and_SoundResume)
{
    SoundOn();
    SoundPause();
    CHECK_EQ_INT(g_soundPaused, 1);
    SoundResume();
    CHECK_EQ_INT(g_soundPaused, 0);
    g_soundEnabled = 0;
    SoundPause();
    CHECK_EQ_INT(g_soundPaused, 0);
}

// ---- the delayed voice queue ----

TEST(ui_SoundQueueAdd_queues_one_voice_at_a_time)
{
    SoundOn();
    AudioHandle a = Sample("a"), b = Sample("b");
    g_time = 1000;
    g_soundQueueNext = 0;
    SoundQueueAdd(a, 50, 0);
    CHECK_EQ_INT(g_soundQueueCount, 1);
    CHECK_EQ_INT(g_soundQueue[0].time, 1000);
    CHECK_EQ_INT(g_soundQueue[0].length, 1000);   // the fake's samples last a second
    CHECK_EQ_INT(g_soundQueueNext, 1000 + 1000 + 50);
    SoundQueueAdd(b, 50, 0);                       // busy: dropped
    CHECK_EQ_INT(g_soundQueueCount, 1);
    SoundQueueAdd(b, 20, 1);                       // vol > 0: queued after the first
    CHECK_EQ_INT(g_soundQueueCount, 2);
    CHECK_EQ_INT(g_soundQueue[1].sample, b);
    CHECK_EQ_INT(g_soundQueue[1].time, 2050);
    CHECK_EQ_INT(g_soundQueueNext, 2050 + 1000 + 20);
    g_time = 5000;                                 // past the end, but voices still queued
    SoundQueueAdd(a, 0, 0);
    CHECK_EQ_INT(g_soundQueueCount, 2);
}

TEST(ui_SoundQueueAdd_needs_the_announcer_on)
{
    SoundOn();
    g_cfg.sfxOn = 0;
    SoundQueueAdd(Sample("a"), 0, 1);
    CHECK_EQ_INT(g_soundQueueCount, 0);
    g_cfg.sfxOn = 1;
    SoundQueueAdd(0, 0, 1);
    CHECK_EQ_INT(g_soundQueueCount, 0);
}

TEST(ui_SoundQueueAdd_waits_for_the_last_voice_to_end)
{
    SoundOn();
    g_time = 1000;
    g_soundQueueNext = 1500;
    SoundQueueAdd(Sample("a"), 0, 0);
    CHECK_EQ_INT(g_soundQueueCount, 0);
    g_time = 1500;
    SoundQueueAdd(Sample("a"), 0, 0);
    CHECK_EQ_INT(g_soundQueueCount, 0);
    g_time = 1501;
    SoundQueueAdd(Sample("a"), 0, 0);
    CHECK_EQ_INT(g_soundQueueCount, 1);
}

TEST(ui_SoundQueueAdd_holds_ten)
{
    SoundOn();
    AudioHandle a = Sample("a");
    for (int i = 0; i < 12; i++)
        SoundQueueAdd(a, 0, 1);
    CHECK_EQ_INT(g_soundQueueCount, MAX_SOUND_QUEUE);
}

TEST(ui_SoundQueueUpdate_plays_the_head_when_due)
{
    SoundOn();
    AudioHandle a = Sample("a"), b = Sample("b");
    g_time = 1000;
    SoundQueueAdd(a, 0, 1);
    SoundQueueAdd(b, 0, 1);
    SoundQueueUpdate();        // not past its time yet
    CHECK_EQ_INT(g_fake.chanPlays, 0);
    g_time = 1001;
    SoundQueueUpdate();
    CHECK_EQ_INT(FakePlayCount("a.wav"), 1);
    CHECK_EQ_INT(g_soundQueueCount, 1);
    CHECK_EQ_INT(g_soundQueue[0].sample, b);
    g_time = 2000;
    SoundQueueUpdate();
    CHECK_EQ_INT(FakePlayCount("b.wav"), 0);
    g_time = 2001;
    SoundQueueUpdate();
    CHECK_EQ_INT(FakePlayCount("b.wav"), 1);
    CHECK_EQ_INT(g_soundQueueCount, 0);
}

TEST(ui_SoundQueueUpdate_drops_voices_while_the_announcer_is_off)
{
    SoundOn();
    g_time = 1000;
    SoundQueueAdd(Sample("a"), 0, 1);
    g_cfg.sfxOn = 0;
    g_time = 1001;
    SoundQueueUpdate();
    CHECK_EQ_INT(g_soundQueueCount, 0);
    CHECK_EQ_INT(g_fake.chanPlays, 0);
}

TEST(ui_FadeLoopSamples_plays_every_other_call)
{
    SoundOn();
    BuildRampTables();
    AudioHandle a = Sample("a"), b = Sample("b");
    g_fadeOnce = 0;
    g_fadeQueueSample1 = a;
    g_fadeVolume0 = 100;
    g_fadeQueueSample4 = b;
    g_fadeVolume3 = 100;
    FadeLoopSamples();
    CHECK_EQ_INT(FakePlayCount("a.wav"), 1);
    CHECK_EQ_INT(FakePlayCount("b.wav"), 1);
    CHECK_EQ_INT(g_fadeQueueSample1, 0);
    CHECK_EQ_INT(g_fadeQueueSample4, 0);
    g_fadeQueueSample1 = a;
    FadeLoopSamples();        // the gate is closed this time
    CHECK_EQ_INT(FakePlayCount("a.wav"), 1);
    FadeLoopSamples();
    CHECK_EQ_INT(FakePlayCount("a.wav"), 2);
}

// ---- tables ----

TEST(ui_SetSfxVolume_scales_the_table)
{
    SetSfxVolume(128);
    CHECK_EQ_INT(g_sfxVolTable[0], 0);
    CHECK_EQ_INT(g_sfxVolTable[255], 128);
    CHECK_EQ_INT(g_sfxVolTable[100], (int)(100 * (128 / 255.0f)));
    CHECK_EQ_INT(g_sfxVolTable[1], 0);   // truncated, not rounded
    SetMusicVolTable(51);
    CHECK_EQ_INT(g_musVolTable[255], 51);
    CHECK_EQ_INT(g_musVolTable[3], 0);
    CHECK_EQ_INT(g_musVolTable[200], 40);
}

TEST(ui_BuildRampTables_pan_and_brightness)
{
    g_screenW = 800;
    g_screenH = 600;
    BuildRampTables();
    CHECK_NEAR(g_panTable[0], 0, 0);
    CHECK_NEAR(g_panTable[400], 127.5, 0.01);
    CHECK_NEAR(g_panTable[799], 255.0 / 800 * 799, 0.01);
    CHECK_EQ_INT(g_rampB[0], 100);
    CHECK_EQ_INT(g_rampB[250], (int)(250 * (155.0 / 500) + 100));
    CHECK_EQ_INT(g_rampB[499], (int)(499 * (155.0 / 500) + 100));
    CHECK_EQ_INT(g_rampB[500], 255);
    CHECK_EQ_INT(g_rampB[599], 255);
}

TEST(ui_InitSampleTable_alien_shot_pitches)
{
    g_sfxAlienShoot2 = 22;
    g_sfxAlienShoot4 = 44;
    InitSampleTable();
    CHECK_EQ_INT(g_sampleHandle[1], 22);
    CHECK_EQ_INT(g_sampleRate[1], 25000);
    CHECK_EQ_INT(g_sampleVol[1], 180);
    CHECK_EQ_INT(g_sampleHandle[9], 44);
    CHECK_EQ_INT(g_sampleRate[9], 20000);
    CHECK_EQ_INT(g_sampleVol[9], 200);
    CHECK_EQ_INT(g_sampleRate[5], 17500);
}

// ---- loading ----

TEST(ui_LoadSample_tries_wav_first)
{
    AudioHandle h = LoadSample("zoom", 2);
    CHECK_STR(FakeSampleName(h), "data\\samples\\zoom.wav");
    h = LoadSampleLoop("hum", 1);
    CHECK_STR(FakeSampleName(h), "data\\samples\\hum.wav");
    g_fake.sampleLoadFails = true;
    CHECK_EQ_INT(LoadSample("zoom", 2), 0);
}

TEST(ui_VoicePath_uses_the_configured_pack)
{
    g_profileIndex = -1;
    g_cfg.voice = 4;
    CHECK_STR(VoicePath("bonus.wav"), "data\\samples\\voices\\4\\bonus.wav");
    CHECK_STR(SamplePath("zoom.wav"), "data\\samples\\zoom.wav");
}

TEST(ui_LoadVoices_loads_from_the_voice_pack)
{
    g_profileIndex = -1;
    g_cfg.voice = 2;
    LoadVoices();
    CHECK_STR(FakeSampleName(g_sfxBonus), "data\\samples\\voices\\2\\bonus.wav");
    CHECK_STR(FakeSampleName(g_sfxYouAreThe), "data\\samples\\voices\\2\\youarethe.wav");
    CHECK_STR(FakeSampleName(g_sfxRankBronze), "data\\samples\\voices\\2\\Bronze.wav");
    int before = g_fake.samplesFreed;
    LoadVoices();   // frees the previous set first
    CHECK(g_fake.samplesFreed - before >= 100);
}

TEST(ui_InitSound_and_SoundShutdown)
{
    InitSound();
    CHECK_EQ_INT(g_soundEnabled, 1);
    CHECK_STR(FakeSampleName(g_sfxTast), "data\\samples\\tast.wav");
    CHECK_STR(FakeSampleName(g_sfxWindow), "data\\samples\\maximize.wav");
    CHECK_STR(FakeSampleName(g_sfxScopeHum), "data\\samples\\scopehum.wav");
    CHECK_EQ_INT(g_sampleHandle[0], g_sfxSingleShot);
    SoundShutdown();
    CHECK_EQ_INT(g_soundEnabled, 0);
    CHECK_EQ_INT(g_sfxTast, 0);
    CHECK_EQ_INT(g_sfxBonus, 0);
    // NOTE: "hahaha" is loaded by InitSound but SoundShutdown never frees it (a leak).
    CHECK_NE_INT(g_sfxHahaha, 0);
    CHECK_EQ_INT(g_fake.samplesFreed, g_fake.samplesLoaded - 1);
}

// Makes voice pack `n` exist under the current folder (VoiceExists looks for its wv.id).
static void MakeVoicePack(int n)
{
    char path[256];
    mkdir("data", 0755);
    mkdir("data/samples", 0755);
    mkdir("data/samples/voices", 0755);
    snprintf(path, sizeof path, "data/samples/voices/%d", n);
    mkdir(path, 0755);
    snprintf(path, sizeof path, "data/samples/voices/%d/wv.id", n);
    FILE *f = fopen(path, "wb");
    if (f)
        fclose(f);
}

TEST(ui_VoiceExists_looks_for_the_pack_id)
{
    CHECK(chdir(g_fake.userFolder) == 0);
    MakeVoicePack(3);
    CHECK(VoiceExists(3));
    CHECK(!VoiceExists(2));
}

TEST(ui_Menu_V_switches_to_the_next_voice_pack)
{
    CHECK(chdir(g_fake.userFolder) == 0);
    MakeVoicePack(1);
    MakeVoicePack(4);
    BootGame();
    WinInit();
    RunFrames(1);
    g_cfg.voice = 1;
    g_cfg.sfxOn = 0;
    TapKey(K_VK_V, 1);
    CHECK_EQ_INT(g_cfg.voice, 4);
    CHECK_EQ_INT(g_cfg.sfxOn, 1);
    CHECK_STR(g_alertMsg, "*  L O A D I N G   V O I C E  *  :4");
    CHECK_STR(FakeSampleName(g_sfxGetReady), "data\\samples\\voices\\4\\getready.wav");
    CHECK_EQ_INT(FakePlayCount("voices\\4\\getready"), 1);
    TapKey(K_VK_V, 1);   // wraps past 99 to pack 1
    CHECK_EQ_INT(g_cfg.voice, 1);
}

// ---- music ----

static void MusicOn(void)
{
    g_soundEnabled = 1;
    g_customSongs = 0;
    g_playlistBuf = NULL;
    g_playlistCount = 0;
    g_state = STATE_TITLE;
}

TEST(ui_StartMusic_loads_the_module)
{
    MusicOn();
    g_cfg.musicFormat = MUSIC_FMT_MOD;
    g_songName = "boss";
    g_time = 3000;
    StartMusic();
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\boss.mus");
    CHECK_EQ_INT(g_musicStream, 0);
    CHECK_EQ_INT(FakePlayCount("boss.mus"), 1);
    CHECK_EQ_INT(g_musicPlaying, 1);
    CHECK_EQ_INT(g_songLengthMs, 1000);
    CHECK_EQ_INT(g_songEndTime, 4000);
}

TEST(ui_StartMusic_streams_mp3)
{
    MusicOn();
    g_cfg.musicFormat = MUSIC_FMT_MP3;
    g_songName = "shop";
    g_time = 100;
    StartMusic();
    CHECK_STR(FakeSampleName(g_musicStream), "data\\music\\shop.mp3");
    CHECK_EQ_INT(g_musicHandle, 0);
    CHECK_EQ_INT(g_songEndTime, 1100);
    g_cfg.musicFormat = MUSIC_FMT_MOD;
    StartMusic();   // switching formats drops the stream
    CHECK_EQ_INT(g_musicStream, 0);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\shop.mus");
}

TEST(ui_StartMusic_is_silent_without_sound)
{
    MusicOn();
    g_soundEnabled = 0;
    g_songName = "boss";
    StartMusic();
    CHECK_EQ_INT(g_fake.chanPlays, 0);
}

TEST(ui_Play_track_functions_pick_the_song)
{
    MusicOn();
    g_cfg.musicFormat = MUSIC_FMT_MOD;
    PlayBossMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_BOSS);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\boss.mus");
    PlayShopMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_SHOP);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\shop.mus");
    PlayHiscoreMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_HISCORE);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\hiscore.mus");
    PlayMemoryStationMusic();
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\memory.mus");
    PlayMeteorStormMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_METEOR);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\meteor.mus");
    PlayGemDropMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_GEM_DROP);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\gems.mus");
    PlayEndMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_END);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\end.mus");
    PlayTimeTrialMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_TIMETRIAL);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\timetrial.mus");
    PlayTitleMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_TITLE);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\title.mus");
}

TEST(ui_PlayGameMusic_depends_on_the_mode)
{
    MusicOn();
    g_cfg.musicFormat = MUSIC_FMT_MOD;
    g_gameMode = MODE_SINGLE;
    PlayGameMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_CUSTOM);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\warblade.mus");
    g_gameMode = MODE_TIME_TRIAL;
    PlayGameMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_TIMETRIAL);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\timetrial.mus");
}

TEST(ui_EnsureTitleMusic_only_switches_from_other_tracks)
{
    MusicOn();
    g_cfg.musicFormat = MUSIC_FMT_MOD;
    g_musicMode = MUSIC_TITLE;
    EnsureTitleMusic();
    CHECK_EQ_INT(g_fake.chanPlays, 0);
    g_musicMode = MUSIC_BOSS;
    EnsureTitleMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_TITLE);
    CHECK_EQ_INT(FakePlayCount("title.mus"), 1);
}

TEST(ui_track_functions_leave_an_active_playlist_alone)
{
    MusicOn();
    g_cfg.musicFormat = MUSIC_FMT_PLAYLIST;
    g_playlistCount = 3;
    g_musicMode = MUSIC_TITLE;
    PlayBossMusic();
    PlayGameMusic();
    PlayHiscoreMusic();
    PlayTitleMusic();
    PlayTimeTrialMusic();
    CHECK_EQ_INT(g_musicMode, MUSIC_TITLE);
    CHECK_EQ_INT(g_fake.chanPlays, 0);
}

TEST(ui_StartMusic_custom_song_list)
{
    MusicOn();
    g_customSongs = 1;
    g_musicMode = MUSIC_CUSTOM;
    g_cfg.shuffle = 0;
    g_musicPos = 0;
    g_songCount = 3;
    g_songIndex = 1;
    strcpy(g_songNames[0], "zero.xm");
    strcpy(g_songNames[1], "one.xm");
    strcpy(g_songNames[2], "two.xm");
    StartMusic();
    CHECK_EQ_INT(g_curSong, 2);
    CHECK_STR(FakeSampleName(g_musicHandle), "two.xm");
    StartMusic();
    CHECK_EQ_INT(g_curSong, 0);
    CHECK_STR(FakeSampleName(g_musicHandle), "zero.xm");
    g_musicMode = MUSIC_BOSS;
    strcpy(g_strB, "custom_boss.xm");
    StartMusic();
    CHECK_STR(FakeSampleName(g_musicHandle), "custom_boss.xm");
}

TEST(ui_OpenNextSong_steps_through_the_list)
{
    g_cfg.shuffle = 0;
    g_musicPos = 0;
    g_songCount = 2;
    g_songIndex = 0;
    g_musicMode = MUSIC_CUSTOM;
    CHECK(OpenNextSong());
    CHECK_EQ_INT(g_curSong, 1);
    OpenNextSong();
    CHECK_EQ_INT(g_curSong, 0);
    CHECK_EQ_INT(g_playlistIdx, -1);
    CHECK_EQ_INT(g_musicPlaying, 0);
}

TEST(ui_CycleMusicFormat_walks_the_formats)
{
    MusicOn();
    g_cfg.musicFormat = MUSIC_FMT_PLAYLIST;
    CycleMusicFormat();
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_MOD);
    g_cfg.musicFormat = MUSIC_FMT_MP3;   // no playlist: back to modules
    CycleMusicFormat();
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_MOD);
    g_songName = "title";
    CycleMusicFormat();
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_MP3);
    CHECK_STR(FakeSampleName(g_musicStream), "data\\music\\title.mp3");
}

// ---- the playlist ----

static void WritePlaylist(const char *text)
{
    mkdir(FakeUserPath("warblade"), 0755);
    FILE *f = fopen(FakeUserPath("warblade\\playlist.m3u"), "wb");
    fputs(text, f);
    fclose(f);
}

TEST(ui_LoadPlaylist_and_ParsePlaylist_split_lines)
{
    WritePlaylist("#EXTM3U\nfirst.mp3\r\nsecond.ogg\nthird.wav\n");
    LoadPlaylist();
    CHECK(g_playlistBuf != NULL);
    CHECK_EQ_INT(g_playlistSize, 40);
    CHECK_EQ_INT(ParsePlaylist(), 4);   // a line starts after every '\n'
    CHECK(strncmp(g_playlistLines[0], "first.mp3", 9) == 0);
    CHECK(strncmp(g_playlistLines[1], "second.ogg", 10) == 0);
    CHECK(strncmp(g_playlistLines[2], "third.wav", 9) == 0);
    CHECK(g_playlistLines[3] == g_playlistBuf + 40);
    CHECK(g_playlistLines[4] == NULL);
    FreePlaylist();
    CHECK(g_playlistBuf == NULL);
    CHECK_EQ_INT(ParsePlaylist(), 0);
}

TEST(ui_LoadPlaylist_without_a_file)
{
    LoadPlaylist();
    CHECK(g_playlistBuf == NULL);
}

TEST(ui_PlayRandomPlaylistSong_streams_mp3_and_ogg_lines_only)
{
    WritePlaylist("#EXTM3U\nnotes.txt\nsong.OGG\r\n");
    LoadPlaylist();
    g_playlistCount = ParsePlaylist() - 1;
    CHECK_EQ_INT(g_playlistCount, 2);
    g_soundEnabled = 1;
    SeedRand(7);
    g_time = 500;
    PlayRandomPlaylistSong();
    CHECK_STR(g_songPath, "song.ogg");
    CHECK_STR(FakeSampleName(g_musicStream), "song.ogg");
    CHECK_EQ_INT(FakePlayCount("song.ogg"), 1);
    CHECK_EQ_INT(g_songEndTime, 1500);
}

TEST(ui_StartMusic_plays_from_the_playlist)
{
    WritePlaylist("#EXTM3U\nonly.mp3\n");
    LoadPlaylist();
    g_playlistCount = ParsePlaylist() - 1;
    g_soundEnabled = 1;
    g_cfg.musicFormat = MUSIC_FMT_PLAYLIST;
    g_songName = "title";
    StartMusic();
    CHECK_STR(FakeSampleName(g_musicStream), "only.mp3");
    CHECK_EQ_INT(g_musicHandle, 0);
    CycleMusicFormat();   // playlist -> modules
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_MOD);
    CHECK_STR(FakeSampleName(g_musicHandle), "data\\music\\title.mus");
    g_cfg.musicFormat = MUSIC_FMT_MP3;
    CycleMusicFormat();   // mp3 -> playlist, as one is loaded
    CHECK_EQ_INT(g_cfg.musicFormat, MUSIC_FMT_PLAYLIST);
}
