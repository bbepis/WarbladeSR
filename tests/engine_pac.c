// engine_pac.c: The data archive (warblade.pac, a tar) of the real engine (src/core/sdl.c):
// PacAddArchive, PacExists, PacRead, and ImgLoad reading from it.
#include "engine_util.h"

// A tar archive built in memory.
typedef struct Tar {
    unsigned char *buf;
    size_t size;
} Tar;

// Adds an entry: `prefix` (ustar) may be NULL, `sizeField` replaces the size field's text
// (else "%011o"), `ustar` false writes an old (v7) header without the magic.
static void TarAddRaw(Tar *t, const char *name, size_t nameLen, const char *prefix, char type,
                      const void *data, size_t size, const char *sizeField, bool ustar)
{
    size_t padded = (size + 511) / 512 * 512;
    unsigned char *h;

    t->buf = realloc(t->buf, t->size + 512 + padded);
    h = t->buf + t->size;
    memset(h, 0, 512 + padded);
    memcpy(h, name, nameLen);
    memcpy(h + 100, "0000644", 8);
    if (sizeField)
        memcpy(h + 124, sizeField, strlen(sizeField));
    else
        snprintf((char *)h + 124, 12, "%011o", (unsigned)size);
    h[156] = (unsigned char)type;
    if (ustar) {
        memcpy(h + 257, "ustar", 6);
        memcpy(h + 263, "00", 2);
    }
    if (prefix)
        memcpy(h + 345, prefix, strlen(prefix));
    if (size)
        memcpy(h + 512, data, size);
    t->size += 512 + padded;
}

static void TarAdd(Tar *t, const char *name, const void *data, size_t size)
{
    TarAddRaw(t, name, strlen(name), NULL, '0', data, size, NULL, true);
}

// Ends the archive (two zero blocks) and writes it to `path`.
static void TarWrite(Tar *t, const char *path)
{
    t->buf = realloc(t->buf, t->size + 1024);
    memset(t->buf + t->size, 0, 1024);
    t->size += 1024;
    WriteBytes(path, t->buf, t->size);
    free(t->buf);
    t->buf = NULL;
    t->size = 0;
}

static char s_dir[512];

// A temp folder (the current folder, so loose files are found there) with warblade.pac made
// from `t`, added with a Windows-style path in other case.
static void AddArchive(Tar *t)
{
    char p[1024];
    MakeTempDir(s_dir, sizeof s_dir);
    CHECK(chdir(s_dir) == 0);
    CHECK(mkdir("Data", 0755) == 0);
    TarWrite(t, "Data/Warblade.pac");
    snprintf(p, sizeof p, "%s\\data\\WARBLADE.PAC", s_dir);
    PacAddArchive(p);
}

static const char s_hello[] = "hello world";

// Three files, one spanning two blocks, between a folder and other entry types.
static void StandardArchive(void)
{
    Tar t = {0};
    static char big[600];
    int i;
    for (i = 0; i < 600; i++)
        big[i] = (char)('a' + i % 26);
    TarAddRaw(&t, "Sounds/", 7, NULL, '5', NULL, 0, NULL, true);
    TarAdd(&t, "Data/Hello.TXT", s_hello, 11);
    TarAdd(&t, "data/big.bin", big, 600);
    TarAddRaw(&t, "link.txt", 8, NULL, '2', NULL, 0, NULL, true);
    TarAdd(&t, "Last.dat", "LAST", 4);
    AddArchive(&t);
}

TEST(engine_PacExists_finds_archive_files_ignoring_case)
{
    StandardArchive();
    CHECK(PacExists("Data/Hello.TXT"));
    CHECK(PacExists("data/hello.txt"));
    CHECK(PacExists("DATA/BIG.BIN"));
    CHECK(PacExists("last.DAT"));
    CHECK(!PacExists("Data/Hello.TX"));
    CHECK(!PacExists("Hello.TXT"));
    RmTree(s_dir);
}

TEST(engine_PacExists_ignores_folders_and_links)
{
    StandardArchive();
    CHECK(!PacExists("Sounds/"));
    CHECK(!PacExists("link.txt"));
    RmTree(s_dir);
}

TEST(engine_PacRead_reads_whole_files)
{
    char buf[700];
    int i;
    StandardArchive();
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("DATA/hello.txt", buf, 11));
    CHECK_MEM(buf, "hello world", 11);
    CHECK(buf[11] == '#');
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("data/big.bin", buf, sizeof buf));
    for (i = 0; i < 600; i++)
        CHECK_MSG(buf[i] == (char)('a' + i % 26), "byte %d is %d", i, buf[i]);
    CHECK(buf[600] == '#');
    // The entry after the two-block file.
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("Last.dat", buf, 4));
    CHECK_MEM(buf, "LAST", 4);
    RmTree(s_dir);
}

TEST(engine_PacRead_reads_only_the_size_asked)
{
    char buf[32];
    StandardArchive();
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("Data/Hello.TXT", buf, 5));
    CHECK_MEM(buf, "hello#", 6);
    RmTree(s_dir);
}

TEST(engine_PacRead_fills_only_a_shorter_files_length)
{
    char buf[32];
    StandardArchive();
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("Data/Hello.TXT", buf, sizeof buf));
    CHECK_MEM(buf, "hello world#####", 16);
    RmTree(s_dir);
}

TEST(engine_PacRead_fails_for_missing_files)
{
    char buf[8];
    StandardArchive();
    memset(buf, '#', sizeof buf);
    CHECK(!PacRead("Data/missing.txt", buf, sizeof buf));
    CHECK_MEM(buf, "########", 8);
    RmTree(s_dir);
}

TEST(engine_Pac_falls_back_to_loose_files)
{
    char buf[16];
    StandardArchive();
    CHECK(mkdir("Data/Levels", 0755) == 0);
    WriteBytes("Data/Levels/Loose.LVD", "loose!", 6);
    CHECK(PacExists("data\\levels\\loose.lvd"));
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("data\\levels\\loose.lvd", buf, sizeof buf));
    CHECK_MEM(buf, "loose!##", 8);
    RmTree(s_dir);
}

TEST(engine_Pac_prefers_the_archive_over_a_loose_file)
{
    char buf[16];
    StandardArchive();
    WriteBytes("Data/Hello.TXT", "loose", 5);
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("Data/Hello.TXT", buf, 11));
    CHECK_MEM(buf, "hello world", 11);
    RmTree(s_dir);
}

TEST(engine_Pac_without_an_archive_reads_loose_files)
{
    char dir[512], buf[8];
    MakeTempDir(dir, sizeof dir);
    CHECK(chdir(dir) == 0);
    PacAddArchive("missing.pac");           // logged, nothing indexed
    WriteBytes("A.txt", "abc", 3);
    CHECK(PacExists("a.TXT"));
    CHECK(!PacExists("b.txt"));
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("a.txt", buf, sizeof buf));
    CHECK_MEM(buf, "abc#", 4);
    RmTree(dir);
}

TEST(engine_PacAddArchive_joins_the_ustar_prefix)
{
    Tar t = {0};
    char buf[8];
    TarAddRaw(&t, "Alien_10.tga", 12, "gfx/aliens", '0', "AL", 2, NULL, true);
    AddArchive(&t);
    CHECK(PacExists("gfx/aliens/alien_10.tga"));
    CHECK(!PacExists("alien_10.tga"));
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("GFX/Aliens/Alien_10.TGA", buf, 2));
    CHECK_MEM(buf, "AL##", 4);
    RmTree(s_dir);
}

TEST(engine_PacAddArchive_ignores_the_prefix_field_of_old_headers)
{
    Tar t = {0};
    // A v7 header has no magic; whatever is where the prefix would be isn't part of the name.
    TarAddRaw(&t, "old.txt", 7, "junk", '0', "OLD", 3, NULL, false);
    AddArchive(&t);
    CHECK(PacExists("old.txt"));
    CHECK(!PacExists("junk/old.txt"));
    RmTree(s_dir);
}

TEST(engine_PacAddArchive_reads_a_full_100_character_name)
{
    Tar t = {0};
    char name[101];
    memset(name, 'n', 100);
    memcpy(name + 96, ".bin", 4);
    name[100] = 0;
    // The name field has no terminator; the mode field follows it.
    TarAddRaw(&t, name, 100, NULL, '0', "N", 1, NULL, true);
    AddArchive(&t);
    CHECK(PacExists(name));
    RmTree(s_dir);
}

TEST(engine_PacAddArchive_parses_octal_sizes)
{
    Tar t = {0};
    char data[600], buf[700];
    int i;
    for (i = 0; i < 600; i++)
        data[i] = (char)i;
    // 01130 = 600, ended by a space as some tars write it; the old type 0 ('\0') is a file.
    TarAddRaw(&t, "sized.bin", 9, NULL, 0, data, 600, "00000001130 ", true);
    TarAdd(&t, "next.txt", "NEXT", 4);
    AddArchive(&t);
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("sized.bin", buf, sizeof buf));
    CHECK_MEM(buf, data, 600);
    CHECK(buf[600] == '#');
    memset(buf, '#', sizeof buf);
    CHECK(PacRead("next.txt", buf, 4));
    CHECK_MEM(buf, "NEXT", 4);
    RmTree(s_dir);
}

TEST(engine_PacAddArchive_indexes_many_files)
{
    Tar t = {0};
    char name[32], buf[8];
    int i;
    for (i = 0; i < 1500; i++) {
        snprintf(name, sizeof name, "f%04d", i);
        TarAdd(&t, name, name, 5);
    }
    AddArchive(&t);
    for (i = 0; i < 1500; i += 101) {
        snprintf(name, sizeof name, "F%04d", i);
        memset(buf, 0, sizeof buf);
        CHECK_MSG(PacRead(name, buf, 5), "%s", name);
        CHECK_MEM(buf + 1, name + 1, 4);
    }
    CHECK(PacExists("f1499"));
    RmTree(s_dir);
}

TEST(engine_PacAddArchive_stops_at_the_end_block)
{
    Tar t = {0};
    TarAdd(&t, "first.txt", "1", 1);
    // A zero block, then an entry after it that isn't part of the archive.
    t.buf = realloc(t.buf, t.size + 512);
    memset(t.buf + t.size, 0, 512);
    t.size += 512;
    TarAdd(&t, "after.txt", "2", 1);
    AddArchive(&t);
    CHECK(PacExists("first.txt"));
    CHECK(!PacExists("after.txt"));
    RmTree(s_dir);
}

TEST(engine_ImgLoad_reads_pictures_from_the_archive)
{
    Tar t = {0};
    Uint32 px[6] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff, 0xff000000, 0x80ff00ff };
    size_t size;
    void *tga;
    Image *img;
    char p[1024];

    OpenWindow(64, 32);
    MakeTempDir(p, sizeof p);
    strcat(p, "/pic.tga");
    WriteTga(p, 3, 2, px);
    tga = SDL_LoadFile(p, &size);
    TarAdd(&t, "gfx/Ship.TGA", tga, size);
    SDL_free(tga);
    *strrchr(p, '/') = 0;
    RmTree(p);
    AddArchive(&t);
    img = ImgLoad("GFX/ship.tga", false, true);
    CHECK(img != NULL);
    CHECK_EQ_INT((int)ImgWidth(img), 3);
    CHECK_EQ_INT((int)ImgHeight(img), 2);
    ImgBlitAlphaRect(img, 0, 0, 3, 2, 10, 20, false, false);
    CHECK_RGB(Rgb(10, 20), 0xff0000, 0);
    CHECK_RGB(Rgb(12, 20), 0x0000ff, 0);
    CHECK_RGB(Rgb(10, 21), 0xffffff, 0);
    ImgFree(img);
    RmTree(s_dir);
}
