// engine_audio.c: The real engine's audio (src/core/sdl_audio.c, BASS-shaped, on SDL3_mixer):
// handles, voices, channel attributes. Most run on SDL's dummy audio driver; those that check
// what is heard record the output with the disk driver (raw 32-bit float stereo at 44100 Hz).
#include "engine_util.h"

static char s_dir[512];

// A WAV file in the temp folder: `frames` frames at `rate` Hz, every sample `value`.
static const char *Wav(const char *name, int rate, int frames, short value)
{
    static char path[4][1024];
    static int next;
    char *p = path[next++ % 4];
    if (!s_dir[0])
        MakeTempDir(s_dir, sizeof s_dir);
    snprintf(p, 1024, "%s/%s", s_dir, name);
    WriteWav(p, rate, frames, value);
    return p;
}

// A looping DC sample of 0.5 (1 s at 44100 Hz).
static AudioHandle LoopSample(int maxVoices)
{
    AudioHandle s = SampleLoad(Wav("loop.wav", 44100, 44100, 16384), maxVoices, AUDIO_SAMPLE_LOOP);
    CHECK(s != 0);
    return s;
}

static void StartAudio(void)
{
    SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
    CHECK_MSG(AudioInit(), "AudioInit: %s", SDL_GetError());
    CHECK_EQ_INT(AudioError(), 0);
}

static void Done(void)
{
    AudioShutdown();
    if (s_dir[0])
        RmTree(s_dir);
}

// ---------------------------------------------------------------------------------------------
// Recording the output
// ---------------------------------------------------------------------------------------------

static char s_capture[1100];

static void StartRecording(void)
{
    if (!s_dir[0])
        MakeTempDir(s_dir, sizeof s_dir);
    snprintf(s_capture, sizeof s_capture, "%s/out.raw", s_dir);
    SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "disk", SDL_HINT_OVERRIDE);
    SDL_SetHint(SDL_HINT_AUDIO_DISK_OUTPUT_FILE, s_capture);
    CHECK_MSG(AudioInit(), "AudioInit: %s", SDL_GetError());
}

typedef struct Capture {
    float *f;           // left, right, left, ...
    size_t frames;
    size_t first, last; // the first and last frame that isn't silent (last = 0: none)
    size_t loud;        // frames that aren't silent
} Capture;

static bool Silent(const float *f)
{
    return fabsf(f[0]) < 1e-4f && fabsf(f[1]) < 1e-4f;
}

// Waits `ms`, then stops the output and reads what it played.
static Capture StopRecording(int ms)
{
    Capture c;
    size_t size = 0, i;
    SDL_Delay(ms);
    AudioShutdown();
    c.f = SDL_LoadFile(s_capture, &size);
    CHECK_MSG(c.f != NULL, "no recording");
    c.frames = size / 8;
    c.first = c.last = c.loud = 0;
    for (i = 0; i < c.frames; i++) {
        if (!Silent(c.f + i * 2)) {
            if (!c.loud)
                c.first = i;
            c.last = i;
            c.loud++;
        }
    }
    return c;
}

// The last `n` loud frames are (l, r).
static void CheckTail(const Capture *c, size_t n, float l, float r)
{
    size_t i;
    CHECK_MSG(c->loud >= n, "only %zu loud frames of %zu", c->loud, c->frames);
    for (i = c->last + 1 - n; i <= c->last; i++)
        CHECK_MSG(fabsf(c->f[i * 2] - l) < 0.01f && fabsf(c->f[i * 2 + 1] - r) < 0.01f,
                  "frame %zu is (%g, %g), expected (%g, %g)", i, c->f[i * 2], c->f[i * 2 + 1], l, r);
}

// Plays a voice of a looping DC 0.5 sample, after `setup` changed it, for 200 ms.
static Capture PlayLoop(void (*setup)(AudioHandle voice))
{
    AudioHandle v;
    StartRecording();
    v = SampleGetVoice(LoopSample(1));
    CHECK(v != 0);
    if (setup)
        setup(v);
    CHECK(ChanPlay(v, true));
    return StopRecording(200);
}

// ---------------------------------------------------------------------------------------------
// Output, samples, handles
// ---------------------------------------------------------------------------------------------

TEST(engine_AudioInit_and_AudioShutdown)
{
    StartAudio();
    CHECK(LoopSample(1) != 0);
    AudioShutdown();
    AudioShutdown();                        // twice: nothing happens
    CHECK_EQ_INT(SampleLoad(Wav("a.wav", 44100, 100, 0), 1, 0), 0);
    CHECK_EQ_INT(MusicLoad(Wav("b.wav", 44100, 100, 0)), 0);
    // And again.
    StartAudio();
    CHECK(LoopSample(1) != 0);
    Done();
}

TEST(engine_SampleLoad_finds_the_file_ignoring_case)
{
    char p[1024], q[1100];
    AudioHandle a, b;
    StartAudio();
    Wav("x", 8000, 10, 0);                  // makes the folder
    snprintf(p, sizeof p, "%s/Sounds", s_dir);
    CHECK(mkdir(p, 0755) == 0);
    snprintf(q, sizeof q, "%s/Boom.WAV", p);
    WriteWav(q, 22050, 100, 0);
    snprintf(q, sizeof q, "%s\\SOUNDS\\boom.wav", s_dir);
    a = SampleLoad(q, 2, 0);
    CHECK(a != 0);
    b = SampleLoad(q, 2, 0);
    CHECK(b != 0);
    CHECK(a != b);
    CHECK_EQ_INT(AudioError(), 0);
    Done();
}

TEST(engine_SampleLoad_fails_for_a_missing_file)
{
    char p[1024];
    StartAudio();
    Wav("x", 8000, 10, 0);
    snprintf(p, sizeof p, "%s/missing.wav", s_dir);
    CHECK_EQ_INT(SampleLoad(p, 1, 0), 0);
    CHECK_EQ_INT(AudioError(), 2);
    Done();
}

TEST(engine_ChanLength_in_seconds)
{
    AudioHandle s, v, m;
    StartAudio();
    s = SampleLoad(Wav("half.wav", 22050, 11025, 100), 1, 0);
    CHECK(s != 0);
    CHECK_NEAR(ChanLength(s), 0.5, 1e-9);
    v = SampleGetVoice(s);
    CHECK_NEAR(ChanLength(v), 0.5, 1e-9);
    m = MusicLoad(Wav("music.wav", 8000, 2000, 100));
    CHECK(m != 0);
    CHECK_NEAR(ChanLength(m), 0.25, 1e-9);
    CHECK_NEAR(ChanLength(0), -1, 0);
    Done();
}

TEST(engine_SampleFree_makes_its_handles_stale)
{
    AudioHandle s, v, s2;
    StartAudio();
    s = LoopSample(1);
    v = SampleGetVoice(s);
    CHECK(v != 0);
    SampleFree(s);
    CHECK_NEAR(ChanLength(s), -1, 0);
    CHECK_NEAR(ChanLength(v), -1, 0);
    CHECK(!ChanPlay(v, true));
    CHECK_EQ_INT(AudioError(), 5);
    CHECK_EQ_INT(SampleGetVoice(s), 0);
    SampleFree(s);                          // twice: nothing happens
    // A new sample in the same place: the old handles stay stale.
    s2 = LoopSample(1);
    CHECK(s2 != s);
    CHECK_NEAR(ChanLength(s), -1, 0);
    CHECK_EQ_INT(SampleGetVoice(s), 0);
    CHECK(SampleGetVoice(s2) != 0);
    Done();
}

TEST(engine_SampleFree_ignores_voice_handles)
{
    AudioHandle s, v;
    StartAudio();
    s = LoopSample(1);
    v = SampleGetVoice(s);
    SampleFree(v);
    CHECK_NEAR(ChanLength(s), 1.0, 1e-9);
    CHECK(ChanPlay(v, true));
    Done();
}

TEST(engine_SampleGetVoice_rejects_other_handles)
{
    AudioHandle s, v, m;
    StartAudio();
    s = LoopSample(1);
    v = SampleGetVoice(s);
    m = MusicLoad(Wav("m.wav", 8000, 100, 0));
    CHECK_EQ_INT(SampleGetVoice(v), 0);
    CHECK_EQ_INT(AudioError(), 5);
    CHECK_EQ_INT(SampleGetVoice(m), 0);
    CHECK_EQ_INT(SampleGetVoice(0), 0);
    Done();
}

TEST(engine_ChanPlay_needs_a_channel)
{
    AudioHandle s;
    StartAudio();
    s = LoopSample(1);
    CHECK(!ChanPlay(s, true));              // a sample isn't a channel
    CHECK_EQ_INT(AudioError(), 5);
    CHECK(!ChanPlay(0, true));
    Done();
}

TEST(engine_SampleGetVoice_gives_each_voice_its_own_handle)
{
    AudioHandle s, v[3];
    int i;
    StartAudio();
    s = LoopSample(3);
    for (i = 0; i < 3; i++) {
        v[i] = SampleGetVoice(s);
        CHECK(v[i] != 0);
        CHECK(ChanPlay(v[i], true));
    }
    CHECK(v[0] != v[1] && v[1] != v[2] && v[0] != v[2]);
    for (i = 0; i < 3; i++)
        CHECK_MSG(ChanPlay(v[i], true), "voice %d went stale", i);
    Done();
}

TEST(engine_SampleGetVoice_takes_over_the_quietest_when_all_play)
{
    AudioHandle s, v[3], w;
    int i;
    StartAudio();
    s = LoopSample(3);
    for (i = 0; i < 3; i++) {
        v[i] = SampleGetVoice(s);
        CHECK(ChanPlay(v[i], true));
    }
    ChanSet(v[0], AUDIO_VOL, 0.8f);
    ChanSet(v[1], AUDIO_VOL, 0.2f);
    ChanSet(v[2], AUDIO_VOL, 0.5f);
    w = SampleGetVoice(s);
    CHECK(w != 0);
    CHECK(w != v[1]);
    CHECK(!ChanPlay(v[1], true));
    CHECK(ChanPlay(v[0], true));
    CHECK(ChanPlay(v[2], true));
    CHECK(ChanPlay(w, true));
    // The next: v[2] at 0.5 is now the quietest (w is at 1).
    w = SampleGetVoice(s);
    CHECK(!ChanPlay(v[2], true));
    CHECK(ChanPlay(v[0], true));
    Done();
}

TEST(engine_SampleGetVoice_keeps_handed_out_voices_while_others_are_free)
{
    AudioHandle s, a, b, c;
    StartAudio();
    s = LoopSample(2);
    a = SampleGetVoice(s);                  // handed out, not played yet
    b = SampleGetVoice(s);
    CHECK(a != b);
    CHECK(ChanPlay(a, true));
    CHECK(ChanPlay(b, true));
    // Both play: the next takes over one.
    c = SampleGetVoice(s);
    CHECK(c != 0);
    Done();
}

TEST(engine_SampleGetVoice_takes_back_an_unplayed_voice_last)
{
    AudioHandle s, a, b, c;
    StartAudio();
    s = LoopSample(2);
    a = SampleGetVoice(s);
    b = SampleGetVoice(s);
    CHECK(ChanPlay(b, true));
    ChanSet(b, AUDIO_VOL, 0.5f);
    // a was never played; it goes before the playing b, though b is quieter.
    c = SampleGetVoice(s);
    CHECK(!ChanPlay(a, true));
    CHECK(ChanPlay(b, true));
    CHECK(ChanPlay(c, true));
    Done();
}

TEST(engine_SampleGetVoice_reuses_a_stopped_voice)
{
    AudioHandle s, a, b;
    StartAudio();
    s = LoopSample(2);
    a = SampleGetVoice(s);
    CHECK(ChanPlay(a, true));
    ChanStop(a);
    b = SampleGetVoice(s);
    CHECK(b != 0);
    // The stopped voice was free: a is stale, and the second voice is still unused.
    CHECK(!ChanPlay(a, true));
    CHECK(ChanPlay(b, true));
    Done();
}

TEST(engine_ChanSetPos_before_play_sets_the_start)
{
    AudioHandle s, v;
    unsigned long p;
    StartAudio();
    s = SampleLoad(Wav("long.wav", 22050, 44100, 1000), 1, 0);
    v = SampleGetVoice(s);
    ChanSetPos(v, 22050);
    CHECK(ChanPlay(v, true));
    p = ChanGetPos(v);
    CHECK_MSG(p >= 22050 && p < 30000, "ChanGetPos() == %lu", p);
    // A voice handed out again starts at 0.
    ChanStop(v);
    v = SampleGetVoice(s);
    CHECK(ChanPlay(v, true));
    p = ChanGetPos(v);
    CHECK_MSG(p < 8000, "ChanGetPos() == %lu", p);
    Done();
}

TEST(engine_ChanSetPos_while_playing_seeks)
{
    AudioHandle s, v;
    unsigned long p;
    StartAudio();
    s = SampleLoad(Wav("long.wav", 22050, 44100, 1000), 1, 0);
    v = SampleGetVoice(s);
    CHECK(ChanPlay(v, true));
    ChanSetPos(v, 30000);
    p = ChanGetPos(v);
    CHECK_MSG(p >= 30000 && p < 38000, "ChanGetPos() == %lu", p);
    CHECK_EQ_INT(ChanGetPos(0), 0);
    Done();
}

TEST(engine_ChanGetPos_advances_while_playing)
{
    AudioHandle v;
    unsigned long a, b;
    StartAudio();
    v = SampleGetVoice(LoopSample(1));
    CHECK(ChanPlay(v, true));
    a = ChanGetPos(v);
    SDL_Delay(150);
    b = ChanGetPos(v);
    CHECK_MSG(b > a, "position %lu, then %lu", a, b);
    Done();
}

TEST(engine_ChanSet_pan_outside_minus_1_to_1_is_an_error)
{
    AudioHandle v;
    StartAudio();
    v = SampleGetVoice(LoopSample(1));
    ChanSet(v, AUDIO_PAN, -1);
    ChanSet(v, AUDIO_PAN, 1);
    ChanSet(v, AUDIO_PAN, 0.5f);
    ChanSet(v, AUDIO_VOL, 3);
    ChanSet(v, AUDIO_FREQ, 1);
    CHECK_EQ_INT(AudioError(), 0);
    ChanSet(v, AUDIO_PAN, 1.01f);
    CHECK_EQ_INT(AudioError(), 20);
    Done();
}

TEST(engine_ChanSlide_pan_outside_minus_1_to_1_is_an_error)
{
    AudioHandle v;
    StartAudio();
    v = SampleGetVoice(LoopSample(1));
    ChanSlide(v, AUDIO_PAN, -1, 100);
    ChanSlide(v, AUDIO_PAN, 1, 0);
    CHECK_EQ_INT(AudioError(), 0);
    ChanSlide(v, AUDIO_PAN, -128, 100);
    CHECK_EQ_INT(AudioError(), 20);
    Done();
}

TEST(engine_Music_handles)
{
    AudioHandle m, st;
    StartAudio();
    m = MusicLoad(Wav("m.wav", 8000, 800, 0));
    st = StreamLoad(Wav("s.wav", 8000, 1600, 0));
    CHECK(m != 0 && st != 0 && m != st);
    CHECK_NEAR(ChanLength(st), 0.2, 1e-9);
    CHECK(ChanPlay(m, true));
    StreamFree(st);
    CHECK_NEAR(ChanLength(st), -1, 0);
    CHECK(!ChanPlay(st, true));
    CHECK_EQ_INT(AudioError(), 5);
    MusicFree(m);
    CHECK(!ChanPlay(m, true));
    CHECK_NEAR(ChanLength(m), -1, 0);
    CHECK_EQ_INT(MusicLoad("/nonexistent/x.wav"), 0);
    CHECK_EQ_INT(AudioError(), 2);
    Done();
}

// ---------------------------------------------------------------------------------------------
// What is heard
// ---------------------------------------------------------------------------------------------

TEST(engine_voice_plays_at_volume_1_centred)
{
    Capture c = PlayLoop(NULL);
    CHECK(c.loud > 2000);
    CheckTail(&c, 2000, 0.5f, 0.5f);
    SDL_free(c.f);
    Done();
}

static void HalfVolume(AudioHandle v) { ChanSet(v, AUDIO_VOL, 0.5f); }

TEST(engine_ChanSet_volume_scales_the_output)
{
    Capture c = PlayLoop(HalfVolume);
    CheckTail(&c, 2000, 0.25f, 0.25f);
    SDL_free(c.f);
    Done();
}

static void NegativeVolume(AudioHandle v) { ChanSet(v, AUDIO_VOL, -1); }

TEST(engine_ChanSet_negative_volume_is_silence)
{
    Capture c = PlayLoop(NegativeVolume);
    CHECK_EQ_INT(c.loud, 0);
    SDL_free(c.f);
    Done();
}

static void PanLeft(AudioHandle v) { ChanSet(v, AUDIO_PAN, -0.5f); }
static void PanRight(AudioHandle v) { ChanSet(v, AUDIO_PAN, 0.75f); }

TEST(engine_ChanSet_pan_turns_down_the_far_side)
{
    Capture c = PlayLoop(PanLeft);
    CheckTail(&c, 2000, 0.5f, 0.25f);
    SDL_free(c.f);
    c = PlayLoop(PanRight);
    CheckTail(&c, 2000, 0.125f, 0.5f);
    SDL_free(c.f);
    Done();
}

static void PanOutOfRange(AudioHandle v)
{
    ChanSet(v, AUDIO_PAN, -0.5f);
    ChanSet(v, AUDIO_PAN, 200);             // a screen x, as the game passes: ignored
    ChanSlide(v, AUDIO_PAN, 3, 10);
}

TEST(engine_ChanSet_pan_outside_minus_1_to_1_keeps_the_old_pan)
{
    Capture c;
    StartRecording();
    {
        AudioHandle v = SampleGetVoice(LoopSample(1));
        PanOutOfRange(v);
        CHECK(ChanPlay(v, true));
        // Slides step from the game thread.
        for (int i = 0; i < 10; i++) {
            AudioUpdate();
            SDL_Delay(10);
        }
    }
    c = StopRecording(100);
    CheckTail(&c, 2000, 0.5f, 0.25f);
    SDL_free(c.f);
    Done();
}

static void Spoil(AudioHandle v)
{
    ChanSet(v, AUDIO_VOL, 0.1f);
    ChanSet(v, AUDIO_PAN, 1);
    ChanSet(v, AUDIO_FREQ, 88200);
    ChanSetPos(v, 2000);
}

TEST(engine_SampleGetVoice_resets_the_voice)
{
    AudioHandle s, v;
    Capture c;
    StartRecording();
    s = SampleLoad(Wav("once.wav", 44100, 4410, 16384), 1, 0);
    v = SampleGetVoice(s);
    Spoil(v);
    ChanStop(v);
    v = SampleGetVoice(s);
    CHECK(ChanPlay(v, true));
    c = StopRecording(250);
    CheckTail(&c, 1000, 0.5f, 0.5f);
    CHECK_MSG(c.loud > 4300 && c.loud < 4500, "%zu frames heard", c.loud);
    SDL_free(c.f);
    Done();
}

// Plays a 0.1 s sample (4410 frames at 44100 Hz) once and counts the frames heard.
static size_t FramesHeard(float freq, int slideMs)
{
    AudioHandle v;
    Capture c;
    size_t n;
    StartRecording();
    v = SampleGetVoice(SampleLoad(Wav("once.wav", 44100, 4410, 16384), 1, 0));
    CHECK(v != 0);
    if (slideMs > 0) {
        ChanSlide(v, AUDIO_FREQ, freq, slideMs);
        for (int i = 0; i < slideMs / 5 + 4; i++) {
            SysProcessEvents();
            SDL_Delay(10);
        }
    } else if (freq >= 0) {
        ChanSet(v, AUDIO_FREQ, freq);
    }
    CHECK(ChanPlay(v, true));
    c = StopRecording(300);
    n = c.loud;
    SDL_free(c.f);
    return n;
}

TEST(engine_sample_plays_once_without_the_loop_flag)
{
    size_t n = FramesHeard(-1, 0);
    CHECK_MSG(n > 4380 && n < 4440, "%zu frames heard", n);
    Done();
}

TEST(engine_ChanSet_freq_changes_the_pitch)
{
    size_t n = FramesHeard(88200, 0);
    CHECK_MSG(n > 2150 && n < 2260, "%zu frames heard at twice the rate", n);
    n = FramesHeard(22050, 0);
    CHECK_MSG(n > 8760 && n < 8880, "%zu frames heard at half the rate", n);
    // 0: the sample's own rate.
    n = FramesHeard(0, 0);
    CHECK_MSG(n > 4380 && n < 4440, "%zu frames heard at rate 0", n);
    Done();
}

TEST(engine_ChanSlide_freq_steps_with_the_game_loop)
{
    size_t n;
    InitVideo();
    n = FramesHeard(88200, 30);
    CHECK_MSG(n > 2150 && n < 2260, "%zu frames heard after the slide", n);
    Done();
}

TEST(engine_ChanSlide_volume_ramps_to_the_target)
{
    AudioHandle v;
    Capture c;
    size_t i, ramp = 0;
    StartRecording();
    v = SampleGetVoice(LoopSample(1));
    CHECK(ChanPlay(v, true));
    SDL_Delay(60);
    ChanSlide(v, AUDIO_VOL, 0.25f, 50);
    c = StopRecording(200);
    CheckTail(&c, 2000, 0.125f, 0.125f);
    // About 50 ms of frames between the two.
    for (i = c.first; i <= c.last; i++)
        if (c.f[i * 2] < 0.49f && c.f[i * 2] > 0.13f)
            ramp++;
    CHECK_MSG(ramp > 2000 && ramp < 2500, "%zu frames of ramp", ramp);
    SDL_free(c.f);
    Done();
}

static void SlideNow(AudioHandle v)
{
    ChanSlide(v, AUDIO_VOL, 0.5f, 0);
    ChanSlide(v, AUDIO_PAN, -1, 0);
}

TEST(engine_ChanSlide_of_0_ms_sets_now)
{
    // Without the game loop stepping slides.
    Capture c = PlayLoop(SlideNow);
    CheckTail(&c, 2000, 0.25f, 0);
    CHECK_MSG(c.f[c.first * 2] < 0.26f, "first frame %g", c.f[c.first * 2]);
    SDL_free(c.f);
    Done();
}

TEST(engine_ChanSlide_pan_steps_with_the_game_loop)
{
    AudioHandle v;
    Capture c;
    StartRecording();
    v = SampleGetVoice(LoopSample(1));
    ChanSlide(v, AUDIO_PAN, -1, 20);
    for (int i = 0; i < 6; i++) {
        AudioUpdate();
        SDL_Delay(10);
    }
    CHECK(ChanPlay(v, true));
    c = StopRecording(150);
    CheckTail(&c, 2000, 0.5f, 0.0f);
    SDL_free(c.f);
    Done();
}

TEST(engine_ChanStop_silences)
{
    AudioHandle v;
    Capture c;
    StartRecording();
    v = SampleGetVoice(LoopSample(1));
    CHECK(ChanPlay(v, true));
    SDL_Delay(100);
    ChanStop(v);
    c = StopRecording(250);
    CHECK(c.loud > 1000);
    CHECK_MSG(c.frames - c.last > 4000, "sound until frame %zu of %zu", c.last, c.frames);
    SDL_free(c.f);
    Done();
}

TEST(engine_AudioStop_stops_every_channel)
{
    AudioHandle a, b, m;
    Capture c;
    StartRecording();
    a = SampleGetVoice(LoopSample(1));
    b = SampleGetVoice(SampleLoad(Wav("loop2.wav", 44100, 44100, 8192), 1, AUDIO_SAMPLE_LOOP));
    m = MusicLoad(Wav("music.wav", 44100, 4410, 4096));
    CHECK(ChanPlay(a, true) && ChanPlay(b, true) && ChanPlay(m, true));
    SDL_Delay(100);
    AudioStop();
    AudioStart();
    c = StopRecording(250);
    CHECK(c.loud > 1000);
    CHECK_MSG(c.frames - c.last > 4000, "sound until frame %zu of %zu", c.last, c.frames);
    SDL_free(c.f);
    Done();
}

TEST(engine_AudioStart_resumes_the_output)
{
    AudioHandle v;
    Capture c;
    StartRecording();
    AudioStop();
    AudioStart();
    v = SampleGetVoice(LoopSample(1));
    CHECK(ChanPlay(v, true));
    c = StopRecording(150);
    CHECK_MSG(c.loud > 2000, "%zu frames heard", c.loud);
    CHECK(c.last + 2048 >= c.frames);
    SDL_free(c.f);
    Done();
}

TEST(engine_MusicLoad_loops)
{
    AudioHandle m;
    Capture c;
    StartRecording();
    // 0.05 s, played for 0.25.
    m = MusicLoad(Wav("short.wav", 44100, 2205, 16384));
    CHECK(ChanPlay(m, true));
    c = StopRecording(250);
    CHECK_MSG(c.loud > 6000, "%zu frames heard", c.loud);
    CheckTail(&c, 1000, 0.5f, 0.5f);
    SDL_free(c.f);
    Done();
}

TEST(engine_SampleLoad_loop_flag_loops)
{
    AudioHandle v;
    Capture c;
    StartRecording();
    v = SampleGetVoice(SampleLoad(Wav("short.wav", 44100, 2205, 16384), 1, AUDIO_SAMPLE_LOOP));
    CHECK(ChanPlay(v, true));
    c = StopRecording(250);
    CHECK_MSG(c.loud > 6000, "%zu frames heard", c.loud);
    SDL_free(c.f);
    Done();
}

TEST(engine_ChanSet_on_music)
{
    AudioHandle m;
    Capture c;
    StartRecording();
    m = MusicLoad(Wav("short.wav", 44100, 2205, 16384));
    ChanSet(m, AUDIO_VOL, 0.5f);
    ChanSet(m, AUDIO_PAN, 0.5f);
    CHECK(ChanPlay(m, true));
    c = StopRecording(200);
    CheckTail(&c, 2000, 0.125f, 0.25f);
    SDL_free(c.f);
    Done();
}
