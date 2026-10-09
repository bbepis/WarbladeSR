// engine_sys.c: The real engine's (src/core/sdl.c) paths, folders and clocks.
#include "engine_util.h"
#include <time.h>

// ---------------------------------------------------------------------------------------------
// SysPath
// ---------------------------------------------------------------------------------------------

// A temp folder holding Data/Samples/Boom.WAV.
static void MakeTree(char *dir, size_t size)
{
    char p[1024];
    MakeTempDir(dir, size);
    snprintf(p, sizeof p, "%s/Data", dir);
    CHECK(mkdir(p, 0755) == 0);
    snprintf(p, sizeof p, "%s/Data/Samples", dir);
    CHECK(mkdir(p, 0755) == 0);
    snprintf(p, sizeof p, "%s/Data/Samples/Boom.WAV", dir);
    WriteBytes(p, "x", 1);
}

TEST(engine_SysPath_turns_backslashes_into_slashes)
{
    CHECK_STR(SysPath("nothere1\\nothere2\\file.txt"), "nothere1/nothere2/file.txt");
    CHECK_STR(SysPath("\\abs_nothere\\x"), "/abs_nothere/x");
    CHECK_STR(SysPath("nothere/mixed\\sep"), "nothere/mixed/sep");
}

TEST(engine_SysPath_collapses_repeated_and_trailing_separators)
{
    CHECK_STR(SysPath("nothere1\\\\a//b\\"), "nothere1/a/b");
    CHECK_STR(SysPath("//nothere_root"), "/nothere_root");
    CHECK_STR(SysPath(""), "");
}

TEST(engine_SysPath_matches_each_existing_component_ignoring_case)
{
    char dir[512], in[1024], want[1024];
    MakeTree(dir, sizeof dir);

    snprintf(in, sizeof in, "%s\\DATA\\samples\\boom.wav", dir);
    snprintf(want, sizeof want, "%s/Data/Samples/Boom.WAV", dir);
    CHECK_STR(SysPath(in), want);
    // A component spelled right is kept.
    snprintf(in, sizeof in, "%s/Data/sAmPlEs", dir);
    snprintf(want, sizeof want, "%s/Data/Samples", dir);
    CHECK_STR(SysPath(in), want);
    RmTree(dir);
}

TEST(engine_SysPath_keeps_missing_components_as_written)
{
    char dir[512], in[1024], want[1024];
    MakeTree(dir, sizeof dir);

    // The existing part is matched, the rest is kept: a file about to be created.
    snprintf(in, sizeof in, "%s\\data\\NewFolder\\samples\\New.Dat", dir);
    snprintf(want, sizeof want, "%s/Data/NewFolder/samples/New.Dat", dir);
    CHECK_STR(SysPath(in), want);
    snprintf(in, sizeof in, "%s\\data\\samples\\boom.wav.bak", dir);
    snprintf(want, sizeof want, "%s/Data/Samples/boom.wav.bak", dir);
    CHECK_STR(SysPath(in), want);
    RmTree(dir);
}

TEST(engine_SysPath_prefers_the_exact_spelling)
{
    char dir[512], in[1024], want[1024];
    MakeTempDir(dir, sizeof dir);
    snprintf(in, sizeof in, "%s/abc", dir);
    CHECK(mkdir(in, 0755) == 0);
    snprintf(in, sizeof in, "%s/ABC", dir);
    CHECK(mkdir(in, 0755) == 0);

    snprintf(in, sizeof in, "%s\\abc", dir);
    snprintf(want, sizeof want, "%s/abc", dir);
    CHECK_STR(SysPath(in), want);
    snprintf(in, sizeof in, "%s\\ABC", dir);
    snprintf(want, sizeof want, "%s/ABC", dir);
    CHECK_STR(SysPath(in), want);
    RmTree(dir);
}

TEST(engine_SysPath_resolves_relative_paths_from_the_current_folder)
{
    char dir[512];
    MakeTree(dir, sizeof dir);
    CHECK(chdir(dir) == 0);
    CHECK_STR(SysPath("data\\SAMPLES\\BOOM.wav"), "Data/Samples/Boom.WAV");
    CHECK_STR(SysPath("DATA"), "Data");
    RmTree(dir);
}

TEST(engine_SysPath_rotates_four_buffers)
{
    const char *p[5];
    int i, j;
    p[0] = SysPath("nothere\\one");
    p[1] = SysPath("nothere\\two");
    p[2] = SysPath("nothere\\three");
    p[3] = SysPath("nothere\\four");
    for (i = 0; i < 4; i++)
        for (j = 0; j < i; j++)
            CHECK_MSG(p[i] != p[j], "calls %d and %d share a buffer", j, i);
    // Four results stay valid together.
    CHECK_STR(p[0], "nothere/one");
    CHECK_STR(p[1], "nothere/two");
    CHECK_STR(p[2], "nothere/three");
    CHECK_STR(p[3], "nothere/four");
    // The fifth overwrites the first.
    p[4] = SysPath("nothere\\five");
    CHECK(p[4] == p[0]);
    CHECK_STR(p[0], "nothere/five");
    CHECK_STR(p[1], "nothere/two");
}

// ---------------------------------------------------------------------------------------------
// SysFileExists, SysMakeDir, SysAppPath, SysUserFolder
// ---------------------------------------------------------------------------------------------

TEST(engine_SysFileExists_finds_files_and_folders_ignoring_case)
{
    char dir[512], p[1024];
    MakeTree(dir, sizeof dir);
    snprintf(p, sizeof p, "%s\\data\\samples\\BOOM.WAV", dir);
    CHECK(SysFileExists(p));
    snprintf(p, sizeof p, "%s\\DATA\\Samples", dir);
    CHECK(SysFileExists(p));
    snprintf(p, sizeof p, "%s\\data\\samples\\boom2.wav", dir);
    CHECK(!SysFileExists(p));
    snprintf(p, sizeof p, "%s\\nothing\\boom.wav", dir);
    CHECK(!SysFileExists(p));
    RmTree(dir);
}

TEST(engine_SysMakeDir_creates_the_folder_at_the_matched_path)
{
    char dir[512], p[1024];
    MakeTree(dir, sizeof dir);
    snprintf(p, sizeof p, "%s\\data\\Profiles", dir);
    CHECK(SysMakeDir(p));
    snprintf(p, sizeof p, "%s/Data/Profiles", dir);
    CHECK_MSG(IsDir(p), "%s wasn't created", p);
    // No folder with a backslash or a second "data" in its name.
    snprintf(p, sizeof p, "%s/data", dir);
    CHECK(!IsDir(p));
    snprintf(p, sizeof p, "%s\\data\\Profiles", dir);
    CHECK(SysFileExists(p));
    RmTree(dir);
}

TEST(engine_SysMakeDir_fails_without_a_parent)
{
    char dir[512], p[1024];
    MakeTempDir(dir, sizeof dir);
    WriteBytes(strcat(strcpy(p, dir), "/file"), "x", 1);
    // A folder can't be made inside a file.
    snprintf(p, sizeof p, "%s\\file\\sub", dir);
    CHECK(!SysMakeDir(p));
    RmTree(dir);
}

TEST(engine_SysAppPath_is_relative_to_the_program_folder)
{
    char exe[1024], want[1100];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    CHECK(n > 0);
    exe[n] = 0;
    *strrchr(exe, '/') = 0;
    snprintf(want, sizeof want, "%s/Jukebox.exe", exe);
    CHECK_STR(SysAppPath("Jukebox.exe"), want);
    snprintf(want, sizeof want, "%s/data\\warblade.pac", exe);
    CHECK_STR(SysAppPath("data\\warblade.pac"), want);
}

// HOME is `home` and $XDG_CONFIG_HOME is unset, so SDL reads <home>/.config/user-dirs.dirs.
static void SetHome(const char *home)
{
    setenv("HOME", home, 1);
    unsetenv("XDG_CONFIG_HOME");
}

static void WriteUserDirs(const char *home, const char *docs)
{
    char p[1024], text[256];
    snprintf(p, sizeof p, "%s/.config", home);
    CHECK(mkdir(p, 0755) == 0);
    snprintf(p, sizeof p, "%s/.config/user-dirs.dirs", home);
    snprintf(text, sizeof text, "XDG_DOCUMENTS_DIR=\"$HOME/%s\"\n", docs);
    WriteBytes(p, text, strlen(text));
}

TEST(engine_SysUserFolder_is_home_without_a_documents_folder)
{
    char home[512];
    MakeTempDir(home, sizeof home);
    SetHome(home);
    CHECK_STR(SysUserFolder(), home);       // no trailing separator
    RmTree(home);
}

TEST(engine_SysUserFolder_is_the_documents_folder_when_it_exists)
{
    char home[512], docs[1024];
    MakeTempDir(home, sizeof home);
    SetHome(home);
    WriteUserDirs(home, "MyDocs");
    snprintf(docs, sizeof docs, "%s/MyDocs", home);
    CHECK(mkdir(docs, 0755) == 0);
    CHECK_STR(SysUserFolder(), docs);
    // Kept for the rest of the run, even if the folder goes.
    CHECK(rmdir(docs) == 0);
    CHECK_STR(SysUserFolder(), docs);
    RmTree(home);
}

TEST(engine_SysUserFolder_skips_a_documents_folder_that_is_missing)
{
    char home[512];
    MakeTempDir(home, sizeof home);
    SetHome(home);
    WriteUserDirs(home, "MyDocs");          // configured but not created
    CHECK_STR(SysUserFolder(), home);
    RmTree(home);
}

// ---------------------------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------------------------

TEST(engine_SysFileTimeNow_counts_100ns_from_1601)
{
    long long ft = SysFileTimeNow();
    time_t now = time(NULL);
    long long secs = ft / 10000000 - 11644473600LL;
    CHECK_MSG(llabs(secs - (long long)now) <= 2, "FILETIME %lld is %lld s, time() %lld", ft, secs,
              (long long)now);
    // Sub-second resolution: two readings 20 ms apart differ by about 200000 units.
    {
        long long a = SysFileTimeNow(), b;
        SDL_Delay(20);
        b = SysFileTimeNow();
        CHECK_MSG(b - a >= 150000 && b - a < 2000000, "20 ms is %lld units", b - a);
    }
}

static void CheckDate(long long ft, int y, int mo, int d, int dow, int h, int mi, int s, int ms)
{
    SysDate t;
    memset(&t, 0xcc, sizeof t);
    SysFileTimeToDate(ft, &t);
    CHECK_MSG(t.year == y && t.month == mo && t.day == d && t.dayOfWeek == dow && t.hour == h &&
              t.minute == mi && t.second == s && t.milliseconds == ms,
              "%lld: %04d-%02d-%02d (%d) %02d:%02d:%02d.%03d, expected %04d-%02d-%02d (%d) "
              "%02d:%02d:%02d.%03d", ft, t.year, t.month, t.day, t.dayOfWeek, t.hour, t.minute,
              t.second, t.milliseconds, y, mo, d, dow, h, mi, s, ms);
}

TEST(engine_SysFileTimeToDate_converts_known_dates)
{
    CheckDate(125911584000000000LL, 2000, 1, 1, 6, 0, 0, 0, 0);
    CheckDate(125911583990000000LL, 1999, 12, 31, 5, 23, 59, 59, 0);
    CheckDate(133536879302500000LL, 2024, 2, 29, 4, 13, 45, 30, 250);
    CheckDate(133536879302509999LL, 2024, 2, 29, 4, 13, 45, 30, 250);
    CheckDate(94406044280000000LL, 1900, 3, 1, 4, 6, 7, 8, 0);
    CheckDate(157784543999990000LL, 2100, 12, 31, 5, 23, 59, 59, 999);
}

TEST(engine_SysFileTimeToDate_converts_durations_near_1601)
{
    CheckDate(0, 1601, 1, 1, 1, 0, 0, 0, 0);
    // 90 minutes, 5.5 seconds: a duration the game turns into a date.
    CheckDate((90 * 60 + 5) * 10000000LL + 5000000, 1601, 1, 1, 1, 1, 30, 5, 500);
    // 59 days: 1601-03-01, a Thursday (1601 isn't a leap year).
    CheckDate(59LL * 86400 * 10000000, 1601, 3, 1, 4, 0, 0, 0, 0);
}

TEST(engine_SysFileTimeToDate_zeroes_negative_times)
{
    SysDate t, zero;
    memset(&t, 0xcc, sizeof t);
    memset(&zero, 0, sizeof zero);
    SysFileTimeToDate(-1, &t);
    CHECK_MEM(&t, &zero, sizeof t);
}

TEST(engine_SysUtcDate_and_SysLocalDate_read_the_clock)
{
    SysDate u, l;
    time_t now;
    struct tm g;

    setenv("TZ", "XST5", 1);                // 5 hours behind UTC, no daylight saving
    tzset();
    now = time(NULL);
    gmtime_r(&now, &g);
    SysUtcDate(&u);
    SysLocalDate(&l);
    CHECK_EQ_INT(u.year, g.tm_year + 1900);
    CHECK_EQ_INT(u.month, g.tm_mon + 1);
    CHECK_EQ_INT(u.day, g.tm_mday);
    CHECK_EQ_INT(u.dayOfWeek, g.tm_wday);
    CHECK_EQ_INT(u.hour, g.tm_hour);
    CHECK(u.milliseconds < 1000);
    CHECK_EQ_INT(l.hour, (u.hour + 24 - 5) % 24);
    CHECK_EQ_INT(l.minute, u.minute);
}

TEST(engine_SysMillis_never_starts_near_0)
{
    unsigned a = SysMillis(), b;
    CHECK_MSG(a >= 86400000u, "SysMillis() == %u", a);
    CHECK(a - 86400000u <= (unsigned)SDL_GetTicks());
    SDL_Delay(30);
    b = SysMillis();
    CHECK_MSG(b - a >= 29 && b - a < 500, "30 ms took %u", b - a);
}

TEST(engine_SysPerfCounter_ticks_at_SysPerfFreq)
{
    long long f = SysPerfFreq(), a, b;
    double s;
    CHECK(f > 0);
    a = SysPerfCounter();
    SDL_Delay(50);
    b = SysPerfCounter();
    s = (double)(b - a) / f;
    CHECK_MSG(s >= 0.045 && s < 0.5, "50 ms measured as %g s", s);
}

TEST(engine_SysPadState_virtual_controls_belong_only_to_player_one)
{
    InitVideo();
    SysSetVirtualPad(PAD_LEFT | PAD_FIRE | PAD_PAUSE);
    CHECK_EQ_INT(SysPadState(0), PAD_LEFT | PAD_FIRE | PAD_PAUSE);
    CHECK_EQ_INT(SysPadState(1), 0);
    CHECK_EQ_INT(SysPadState(-1), 0);
    CHECK_EQ_INT(SysPadState(4), 0);
    SysSetVirtualPad(PAD_RIGHT | PAD_ROCKET);
    CHECK_EQ_INT(SysPadState(0), PAD_RIGHT | PAD_ROCKET);
    SysSetVirtualPad(0);
    CHECK_EQ_INT(SysPadState(0), 0);
}
