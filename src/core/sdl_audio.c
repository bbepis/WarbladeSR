// sdl_audio.c: The audio half of the engine interface (include/sdlhelp.h) on SDL3_mixer,
// shaped like BASS 2.4, which the game was written for:
// - A sample has a fixed number of voices (MIX_Tracks, made when first needed). When all are
//   busy, SampleGetVoice takes over the quietest one (BASS_SAMPLE_OVER_VOL), and the handle of
//   the sound that was playing there goes stale.
// - Volume is applied per sample frame in a mixer callback, so the game's short fade-ins
//   (ChanSet(vol, 0) + ChanSlide(vol, v, 8 ms)) are as smooth as BASS's. Frequency and pan
//   slides are stepped from the game thread (AudioTick, called by SysProcessEvents).
// - AudioPause/AudioStart pause and resume the output device; AudioStop also stops every
//   channel.
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "sdlhelp.h"

void AudioTick(void);

#define MIX_RATE    44100
#define MAX_SAMPLES 1024        // loaded samples at once
#define MAX_MUSIC   16          // loaded modules and streams at once

typedef struct Slide {
    bool on;
    float from, to;
    Uint64 start;               // SDL_GetTicks
    Uint64 ms;
} Slide;

typedef struct Chan {
    MIX_Track *track;
    unsigned gen;               // part of the handle; changes when the voice is handed out again
    bool reserved;              // handed out by SampleGetVoice, not played yet
    bool loop;
    int rate;                   // the audio's own sample rate: AUDIO_FREQ's reference
    // Volume, applied by VolumeCallback (read on the mixer thread: change under the mixer lock).
    float vol, volTarget, volStep;
    Sint64 volFrames;           // frames left in the volume slide
    float pan;
    Slide freqSlide, panSlide;
    Sint64 startFrame;          // ChanSetPos on a channel that isn't playing: where play starts
} Chan;

typedef struct Sample {
    bool used;
    unsigned gen;
    MIX_Audio *audio;
    int rate;
    bool loop;
    int maxVoices;
    Chan *voices;
} Sample;

typedef struct Music {          // a module or a stream
    bool used;
    unsigned gen;
    MIX_Audio *audio;
    Chan chan;
} Music;

static MIX_Mixer        *s_mixer;
static SDL_AudioDeviceID s_device;
static Sample            s_samples[MAX_SAMPLES];
static Music             s_music[MAX_MUSIC];
static unsigned          s_lastGen;
static int               s_error;       // BASS-style error code of the last failure

enum { ERR_FILEOPEN = 2, ERR_HANDLE = 5, ERR_START = 9, ERR_NOCHAN = 18, ERR_MEM = 1, ERR_ILLPARAM = 20 };

// Handles: type (2 bits) | generation (10) | object index (12) | voice index (8).
enum { H_SAMPLE = 1, H_VOICE = 2, H_MUSIC = 3 };

static AudioHandle MakeHandle(unsigned type, unsigned gen, unsigned index, unsigned voice)
{
    return (AudioHandle)((type << 30) | (gen << 20) | (index << 8) | voice);
}

#define H_TYPE(h)  ((unsigned)((h) >> 30) & 3)
#define H_GEN(h)   ((unsigned)((h) >> 20) & 0x3ff)
#define H_INDEX(h) ((unsigned)((h) >> 8) & 0xfff)
#define H_VOICE(h) ((unsigned)(h) & 0xff)

static unsigned NextGen(void)
{
    s_lastGen = s_lastGen % 0x3ff + 1;      // 1-1023, never 0
    return s_lastGen;
}

static Sample *SampleFromHandle(AudioHandle h)
{
    Sample *s;
    if (H_TYPE(h) != H_SAMPLE && H_TYPE(h) != H_VOICE)
        return NULL;
    if (H_INDEX(h) >= MAX_SAMPLES)
        return NULL;
    s = &s_samples[H_INDEX(h)];
    if (!s->used || (H_TYPE(h) == H_SAMPLE && s->gen != H_GEN(h)))
        return NULL;
    return s;
}

static Music *MusicFromHandle(AudioHandle h)
{
    Music *m;
    if (H_TYPE(h) != H_MUSIC || H_INDEX(h) >= MAX_MUSIC)
        return NULL;
    m = &s_music[H_INDEX(h)];
    return m->used && m->gen == H_GEN(h) ? m : NULL;
}

// The voice, module or stream `h` names, or NULL if it is stale or not a channel.
static Chan *ChanFromHandle(AudioHandle h)
{
    if (H_TYPE(h) == H_VOICE) {
        Sample *s = SampleFromHandle(h);
        Chan *c;
        if (s == NULL || (int)H_VOICE(h) >= s->maxVoices)
            return NULL;
        c = &s->voices[H_VOICE(h)];
        return c->track && c->gen == H_GEN(h) ? c : NULL;
    }
    if (H_TYPE(h) == H_MUSIC) {
        Music *m = MusicFromHandle(h);
        return m ? &m->chan : NULL;
    }
    return NULL;
}

// Mixer thread: the channel's volume (with its slide), per sample frame.
static void SDLCALL VolumeCallback(void *userdata, MIX_Track *track, const SDL_AudioSpec *spec,
                                   float *pcm, int samples)
{
    Chan *c = (Chan *)userdata;
    int channels = spec->channels;
    int frames = samples / channels;
    int f, k;
    float vol = c->vol;

    (void)track;
    if (c->volFrames == 0 && vol == 1.0f)
        return;
    for (f = 0; f < frames; f++) {
        if (c->volFrames > 0) {
            vol += c->volStep;
            if (--c->volFrames == 0)
                vol = c->volTarget;
        }
        for (k = 0; k < channels; k++)
            pcm[f * channels + k] *= vol;
    }
    c->vol = vol;
}

static int AudioRate(MIX_Audio *audio)
{
    SDL_AudioSpec spec;
    if (MIX_GetAudioFormat(audio, &spec) && spec.freq > 0)
        return spec.freq;
    return MIX_RATE;
}

static bool InitChan(Chan *c, MIX_Audio *audio, int rate, bool loop)
{
    memset(c, 0, sizeof(*c));
    c->track = MIX_CreateTrack(s_mixer);
    if (c->track == NULL)
        return false;
    MIX_SetTrackAudio(c->track, audio);
    MIX_SetTrackCookedCallback(c->track, VolumeCallback, c);
    c->rate = rate;
    c->loop = loop;
    c->vol = c->volTarget = 1.0f;
    return true;
}

static void SetVolume(Chan *c, float vol)
{
    MIX_LockMixer(s_mixer);
    c->vol = c->volTarget = SDL_max(vol, 0.0f);
    c->volFrames = 0;
    MIX_UnlockMixer(s_mixer);
}

static void SetFreq(Chan *c, float hz)
{
    float ratio = hz > 0 ? hz / c->rate : 1.0f;
    MIX_SetTrackFrequencyRatio(c->track, SDL_clamp(ratio, 0.01f, 100.0f));
}

// BASS's pan is a balance: the far side is turned down, the near side stays at full volume.
static void SetPan(Chan *c, float pan)
{
    MIX_StereoGains g;
    pan = SDL_clamp(pan, -1.0f, 1.0f);
    c->pan = pan;
    g.left = pan > 0 ? 1.0f - pan : 1.0f;
    g.right = pan < 0 ? 1.0f + pan : 1.0f;
    MIX_SetTrackStereo(c->track, &g);
}

// A voice as SampleGetVoice hands it out: stopped, volume 1, centred, normal pitch.
static void ResetVoice(Chan *c)
{
    MIX_StopTrack(c->track, 0);
    SetVolume(c, 1.0f);
    SetPan(c, 0);
    MIX_SetTrackFrequencyRatio(c->track, 1.0f);
    c->freqSlide.on = false;
    c->panSlide.on = false;
    c->startFrame = 0;
    c->gen = NextGen();
    c->reserved = true;
}

// ---------------------------------------------------------------------------------------------
// Output
// ---------------------------------------------------------------------------------------------

bool AudioInit(void)
{
    SDL_AudioSpec spec;

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO) || !MIX_Init()) {
        SDL_Log("Audio init failed: %s", SDL_GetError());
        s_error = ERR_START;
        return false;
    }
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = MIX_RATE;
    s_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (s_mixer == NULL) {
        SDL_Log("MIX_CreateMixerDevice failed: %s", SDL_GetError());
        s_error = ERR_START;
        return false;
    }
    s_device = (SDL_AudioDeviceID)SDL_GetNumberProperty(MIX_GetMixerProperties(s_mixer),
                                                        MIX_PROP_MIXER_DEVICE_NUMBER, 0);
    return true;
}

void AudioShutdown(void)
{
    int i;
    if (s_mixer == NULL)
        return;
    for (i = 0; i < MAX_SAMPLES; i++) {
        if (s_samples[i].used)
            SampleFree(MakeHandle(H_SAMPLE, s_samples[i].gen, i, 0));
    }
    for (i = 0; i < MAX_MUSIC; i++) {
        if (s_music[i].used)
            MusicFree(MakeHandle(H_MUSIC, s_music[i].gen, i, 0));
    }
    MIX_DestroyMixer(s_mixer);
    s_mixer = NULL;
    s_device = 0;
    MIX_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int  AudioError(void)  { return s_error; }
// Only called from the game's busy-waits (e.g. while a jingle plays out); in a browser those
// must also let the page run.
void AudioUpdate(void)
{
    AudioTick();
#ifdef __EMSCRIPTEN__
    emscripten_sleep(1);
#endif
}

void AudioStart(void)
{
    if (s_device)
        SDL_ResumeAudioDevice(s_device);
}

void AudioPause(void)
{
    if (s_device)
        SDL_PauseAudioDevice(s_device);
}

void AudioStop(void)
{
    int i, j;
    if (s_mixer == NULL)
        return;
    MIX_StopAllTracks(s_mixer, 0);
    for (i = 0; i < MAX_SAMPLES; i++) {
        for (j = 0; s_samples[i].used && j < s_samples[i].maxVoices; j++)
            s_samples[i].voices[j].reserved = false;
    }
    AudioPause();
}

// ---------------------------------------------------------------------------------------------
// Samples, modules, streams
// ---------------------------------------------------------------------------------------------

AudioHandle SampleLoad(const char *file, int maxVoices, int flags)
{
    MIX_Audio *audio;
    Sample *s;
    int i;

    if (s_mixer == NULL)
        return 0;
    for (i = 0; i < MAX_SAMPLES && s_samples[i].used; i++)
        ;
    if (i == MAX_SAMPLES) {
        s_error = ERR_MEM;
        return 0;
    }
    audio = MIX_LoadAudio(s_mixer, SysPath(file), true);
    if (audio == NULL) {
        s_error = ERR_FILEOPEN;
        return 0;
    }
    s = &s_samples[i];
    memset(s, 0, sizeof(*s));
    s->maxVoices = SDL_clamp(maxVoices, 1, 255);
    s->voices = (Chan *)SDL_calloc(s->maxVoices, sizeof(Chan));
    s->used = true;
    s->gen = NextGen();
    s->audio = audio;
    s->rate = AudioRate(audio);
    s->loop = (flags & AUDIO_SAMPLE_LOOP) != 0;
    return MakeHandle(H_SAMPLE, s->gen, i, 0);
}

void SampleFree(AudioHandle sample)
{
    Sample *s = H_TYPE(sample) == H_SAMPLE ? SampleFromHandle(sample) : NULL;
    int j;

    if (s == NULL)
        return;
    for (j = 0; j < s->maxVoices; j++) {
        if (s->voices[j].track)
            MIX_DestroyTrack(s->voices[j].track);
    }
    SDL_free(s->voices);
    MIX_DestroyAudio(s->audio);
    memset(s, 0, sizeof(*s));
}

AudioHandle SampleGetVoice(AudioHandle sample)
{
    Sample *s = H_TYPE(sample) == H_SAMPLE ? SampleFromHandle(sample) : NULL;
    int best = -1;
    int j;

    if (s == NULL) {
        s_error = ERR_HANDLE;
        return 0;
    }
    // A free voice (made now if it doesn't exist yet), else one handed out but never played,
    // else the quietest playing one.
    for (j = 0; j < s->maxVoices && best < 0; j++) {
        Chan *c = &s->voices[j];
        if (c->track == NULL) {
            if (!InitChan(c, s->audio, s->rate, s->loop)) {
                s_error = ERR_NOCHAN;
                return 0;
            }
            best = j;
        } else if (!c->reserved && !MIX_TrackPlaying(c->track)) {
            best = j;
        }
    }
    for (j = 0; j < s->maxVoices && best < 0; j++) {
        if (!MIX_TrackPlaying(s->voices[j].track))
            best = j;
    }
    if (best < 0) {
        best = 0;
        for (j = 1; j < s->maxVoices; j++) {
            if (s->voices[j].vol < s->voices[best].vol)
                best = j;
        }
    }
    ResetVoice(&s->voices[best]);
    return MakeHandle(H_VOICE, s->voices[best].gen, (unsigned)(s - s_samples), best);
}

static AudioHandle LoadMusic(const char *file)
{
    MIX_Audio *audio;
    Music *m;
    int i;

    if (s_mixer == NULL)
        return 0;
    for (i = 0; i < MAX_MUSIC && s_music[i].used; i++)
        ;
    if (i == MAX_MUSIC) {
        s_error = ERR_MEM;
        return 0;
    }
    audio = MIX_LoadAudio(s_mixer, SysPath(file), false);
    if (audio == NULL) {
        s_error = ERR_FILEOPEN;
        return 0;
    }
    m = &s_music[i];
    if (!InitChan(&m->chan, audio, AudioRate(audio), true)) {
        MIX_DestroyAudio(audio);
        s_error = ERR_NOCHAN;
        return 0;
    }
    m->used = true;
    m->gen = NextGen();
    m->audio = audio;
    return MakeHandle(H_MUSIC, m->gen, i, 0);
}

static void FreeMusic(AudioHandle h)
{
    Music *m = MusicFromHandle(h);
    if (m == NULL)
        return;
    MIX_DestroyTrack(m->chan.track);
    MIX_DestroyAudio(m->audio);
    memset(m, 0, sizeof(*m));
}

// Modules always loop to the start: SDL_mixer's module decoder ends at libxmp's first loop
// (all of the game's modules loop to order 0; SDL_PLAN.md Step 0).
AudioHandle MusicLoad(const char *file)    { return LoadMusic(file); }
void        MusicFree(AudioHandle music)   { FreeMusic(music); }
AudioHandle StreamLoad(const char *file)   { return LoadMusic(file); }
void        StreamFree(AudioHandle stream) { FreeMusic(stream); }

// ---------------------------------------------------------------------------------------------
// Channels
// ---------------------------------------------------------------------------------------------

bool ChanPlay(AudioHandle ch, bool restart)
{
    Chan *c = ChanFromHandle(ch);
    SDL_PropertiesID props;
    bool ok;

    if (c == NULL) {
        s_error = ERR_HANDLE;
        return false;
    }
    if (!restart && MIX_TrackPaused(c->track))
        return MIX_ResumeTrack(c->track);
    props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, c->loop ? -1 : 0);
    if (c->startFrame > 0)
        SDL_SetNumberProperty(props, MIX_PROP_PLAY_START_FRAME_NUMBER, c->startFrame);
    ok = MIX_PlayTrack(c->track, props);
    SDL_DestroyProperties(props);
    c->reserved = false;
    c->startFrame = 0;
    if (!ok)
        s_error = ERR_START;
    return ok;
}

void ChanStop(AudioHandle ch)
{
    Chan *c = ChanFromHandle(ch);
    if (c) {
        MIX_StopTrack(c->track, 0);
        c->reserved = false;
    }
}

// BASS rejects a pan outside -1..1 (BASS_ERROR_ILLPARAM) and keeps the old one. The game relies
// on it: its sound effects pass g_panTable's 0..255 screen-x values, so they play centred (only
// the leftmost few pixels' values are in range, and pan slightly right). Clamping them instead
// put nearly every effect in the right ear.
static bool PanValid(float pan)
{
    if (pan >= -1.0f && pan <= 1.0f)
        return true;
    s_error = ERR_ILLPARAM;
    return false;
}

// Sample handles are ignored, as BASS did.
void ChanSet(AudioHandle ch, enum AudioAttrib attrib, float value)
{
    Chan *c = ChanFromHandle(ch);
    if (c == NULL)
        return;
    if (attrib == AUDIO_PAN && !PanValid(value))
        return;
    switch (attrib) {
    case AUDIO_VOL:
        SetVolume(c, value);
        break;
    case AUDIO_FREQ:
        c->freqSlide.on = false;
        SetFreq(c, value);
        break;
    case AUDIO_PAN:
        c->panSlide.on = false;
        SetPan(c, value);
        break;
    }
}

static void StartSlide(Slide *s, float from, float to, int ms)
{
    s->on = true;
    s->from = from;
    s->to = to;
    s->start = SDL_GetTicks();
    s->ms = ms;
}

void ChanSlide(AudioHandle ch, enum AudioAttrib attrib, float value, int ms)
{
    Chan *c = ChanFromHandle(ch);
    if (c == NULL)
        return;
    if (attrib == AUDIO_PAN && !PanValid(value))
        return;
    if (ms <= 0) {
        ChanSet(ch, attrib, value);
        return;
    }
    switch (attrib) {
    case AUDIO_VOL: {
        Sint64 frames = (Sint64)ms * MIX_RATE / 1000;
        MIX_LockMixer(s_mixer);
        c->volTarget = SDL_max(value, 0.0f);
        c->volFrames = SDL_max(frames, 1);
        c->volStep = (c->volTarget - c->vol) / c->volFrames;
        MIX_UnlockMixer(s_mixer);
        break;
    }
    case AUDIO_FREQ:
        StartSlide(&c->freqSlide, MIX_GetTrackFrequencyRatio(c->track) * c->rate, value, ms);
        break;
    case AUDIO_PAN:
        StartSlide(&c->panSlide, c->pan, value, ms);
        break;
    }
}

static bool StepSlide(Slide *s, Uint64 now, float *value)
{
    float t;
    if (!s->on)
        return false;
    t = now - s->start >= s->ms ? 1.0f : (float)(now - s->start) / s->ms;
    *value = s->from + (s->to - s->from) * t;
    if (t >= 1.0f)
        s->on = false;
    return true;
}

static void TickChan(Chan *c, Uint64 now)
{
    float v;
    if (StepSlide(&c->freqSlide, now, &v))
        SetFreq(c, v);
    if (StepSlide(&c->panSlide, now, &v))
        SetPan(c, v);
}

// Steps the frequency and pan slides.
void AudioTick(void)
{
    Uint64 now = SDL_GetTicks();
    int i, j;

    if (s_mixer == NULL)
        return;
    for (i = 0; i < MAX_SAMPLES; i++) {
        for (j = 0; s_samples[i].used && j < s_samples[i].maxVoices; j++) {
            if (s_samples[i].voices[j].track)
                TickChan(&s_samples[i].voices[j], now);
        }
    }
    for (i = 0; i < MAX_MUSIC; i++) {
        if (s_music[i].used)
            TickChan(&s_music[i].chan, now);
    }
}

double ChanLength(AudioHandle ch)
{
    MIX_Audio *audio = NULL;
    Sint64 frames;

    if (H_TYPE(ch) == H_SAMPLE || H_TYPE(ch) == H_VOICE) {
        Sample *s = SampleFromHandle(ch);
        if (s)
            audio = s->audio;
    } else {
        Music *m = MusicFromHandle(ch);
        if (m)
            audio = m->audio;
    }
    if (audio == NULL)
        return -1.0;
    frames = MIX_GetAudioDuration(audio);
    if (frames < 0)
        return -0.001;              // unknown: the game reads it as -1 ms
    return (double)frames / AudioRate(audio);
}

unsigned long ChanGetPos(AudioHandle ch)
{
    Chan *c = ChanFromHandle(ch);
    Sint64 pos = c ? MIX_GetTrackPlaybackPosition(c->track) : 0;
    return pos > 0 ? (unsigned long)pos : 0;
}

void ChanSetPos(AudioHandle ch, unsigned long pos)
{
    Chan *c = ChanFromHandle(ch);
    if (c == NULL)
        return;
    if (MIX_TrackPlaying(c->track) || MIX_TrackPaused(c->track))
        MIX_SetTrackPlaybackPosition(c->track, (Sint64)pos);
    else
        c->startFrame = (Sint64)pos;
}
