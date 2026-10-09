// Tests for src/core/strutil.c: string helpers and number formatting.
#include "support.h"

TEST(StrLen_counts_characters)
{
    CHECK_EQ_INT(StrLen(""), 0);
    CHECK_EQ_INT(StrLen("warblade"), 8);
    CHECK_EQ_INT(StrLenPlat("warblade"), 8);
    CHECK_EQ_INT(StrLenPlat(""), 0);
}

TEST(StrHash_is_the_31_polynomial)
{
    CHECK_EQ_INT(StrHash(""), 0);
    CHECK_EQ_INT(StrHash("a"), 'a');
    CHECK_EQ_INT(StrHash("abc"), ('a' * 31 + 'b') * 31 + 'c');
}

TEST(ToLower_only_changes_capitals)
{
    CHECK_EQ_INT(ToLower('A'), 'a');
    CHECK_EQ_INT(ToLower('Z'), 'z');
    CHECK_EQ_INT(ToLower('a'), 'a');
    CHECK_EQ_INT(ToLower('@'), '@');
    CHECK_EQ_INT(ToLower('['), '[');
}

TEST(StrToLowerN_stops_at_max)
{
    char out[16];
    memset(out, 'x', sizeof out);
    StrToLowerN("HeLLo World", out, 5);
    CHECK_STR(out, "hello");
    StrToLowerN("AB", out, 10);
    CHECK_STR(out, "ab");
}

TEST(StrToLowerN_leaves_dst_alone_for_empty_src)
{
    char out[8] = "keep";
    StrToLowerN("", out, 5);
    CHECK_STR(out, "keep");
}

TEST(Concat3_joins_three_strings)
{
    CHECK_STR(Concat3("data\\", "level", ".lvl"), "data\\level.lvl");
    CHECK_STR(Concat3("", "x", ""), "x");
}

TEST(Concat3_with_null_middle_returns_previous)
{
    Concat3("a", "b", "c");
    CHECK_STR(Concat3("q", NULL, "r"), "abc");
}

TEST(BackslashToSlash_converts_every_backslash)
{
    char s[] = "data\\samples\\x.mp3";
    BackslashToSlash(s);
    CHECK_STR(s, "data/samples/x.mp3");
}

TEST(StrLower_and_StrUpper)
{
    char s[] = "MiXeD 123";
    CHECK_STR(StrLower(s), "mixed 123");
    StrUpper(s);
    CHECK_STR(s, "MIXED 123");
}

TEST(StrContains_finds_substrings)
{
    char s[] = "warblade.pac";
    CHECK(StrContains(s, ".pac") == s + 8);
    CHECK(StrContains(s, "zip") == NULL);
}

TEST(CopyBytes_copies_and_returns_size)
{
    char src[] = "abcdef", dst[8] = {0};
    CHECK_EQ_INT(CopyBytes(src, dst, 4), 4);
    CHECK_STR(dst, "abcd");
}

TEST(CopyBytesAt_and_CopyStrAt_write_at_offset)
{
    char dst[16] = "0123456789";
    CopyBytesAt(dst, "ab", 3, 2);
    CHECK_STR(dst, "012ab56789");
    CopyStrAt(dst, "XY", 5, 2);
    CHECK_STR(dst, "012abXY");
}

TEST(ParseInt_reads_numbers_and_skips_separators)
{
    char buf[] = "  640x480\r\n7";
    char *p = buf;
    CHECK_EQ_INT(ParseInt(&p, -1), 640);
    CHECK_STR(p, "480\r\n7");     // the separators after a number are skipped too
    CHECK_EQ_INT(ParseInt(&p, -1), 480);
    CHECK_EQ_INT(ParseInt(&p, -1), 7);
    CHECK_EQ_INT(*p, 0);
}

TEST(ParseInt_returns_default_without_digits)
{
    char buf[] = "  abc";
    char *p = buf;
    CHECK_EQ_INT(ParseInt(&p, 42), 42);
    CHECK_EQ_INT(*p, 'a');
}

TEST(Int64ToStrGrouped_puts_dots_between_thousands)
{
    char out[32];
    CHECK_EQ_INT(Int64ToStrGrouped(0, out), 1);
    CHECK_STR(out, "0");
    Int64ToStrGrouped(999, out);
    CHECK_STR(out, "999");
    CHECK_EQ_INT(Int64ToStrGrouped(1000, out), 5);
    CHECK_STR(out, "1.000");
    Int64ToStrGrouped(1234567, out);
    CHECK_STR(out, "1.234.567");
    Int64ToStrGrouped(9999999999999LL, out);
    CHECK_STR(out, "9.999.999.999.999");
}

TEST(Int64ToStr_writes_plain_digits)
{
    char out[32];
    CHECK_EQ_INT(Int64ToStr(0, out), 1);
    CHECK_STR(out, "0");
    CHECK_EQ_INT(Int64ToStr(1234567, out), 7);
    CHECK_STR(out, "1234567");
    Int64ToStr(10, out);
    CHECK_STR(out, "10");
    Int64ToStr(1000000000000LL, out);
    CHECK_STR(out, "1000000000000");
}
