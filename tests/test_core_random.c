// Tests for src/core/random.c (the xorshift generator, ranges, random ids),
// src/core/mathutil.c (trig tables, CRT wrappers, screen clamps), src/data/static_init.c and
// src/core/stubs.c.
#include <stdlib.h>
#include "support.h"

// The generator's state, set directly.
static void SetRng(unsigned x, unsigned y, unsigned z, unsigned w, unsigned t)
{
    g_rngX = x;
    g_rngY = y;
    g_rngZ = z;
    g_rngW = w;
    g_rngT = t;
}

// A model of XorShift on a copy of the state (to predict values without advancing the game's).
typedef struct Rng {
    unsigned x, y, z, w, t;
} Rng;

static Rng GetRng(void)
{
    return (Rng){g_rngX, g_rngY, g_rngZ, g_rngW, g_rngT};
}

static unsigned RefNext(Rng *r)
{
    r->t -= (r->x << 11) ^ r->x;
    r->x = r->y;
    r->y = r->z;
    r->z = r->w;
    r->w = (r->w >> 19) ^ r->w ^ ((r->t >> 8) ^ r->t);
    return r->w;
}

// ---------------------------------------------------------------- SeedRand

TEST(core_SeedRand_derives_xyz_from_the_crt_rng)
{
    srand(1234);
    unsigned a = (unsigned)rand(), b = (unsigned)rand(), c = (unsigned)rand();
    SetRng(0, 0, 0, 77, 99);
    SeedRand(1234);
    CHECK_EQ_INT(g_rngX, a);
    CHECK_EQ_INT(g_rngY, (unsigned)(b * a));
    CHECK_EQ_INT(g_rngZ, (unsigned)(c * a + b * a));
    // W and T are left alone
    CHECK_EQ_INT(g_rngW, 77);
    CHECK_EQ_INT(g_rngT, 99);
}

TEST(core_SeedRand_same_seed_same_sequence)
{
    unsigned first[8], second[8];
    SeedRand(42);
    for (int i = 0; i < 8; i++)
        first[i] = XorShift();
    SetRng(1, 1, 1, 0, 0);
    SeedRand(42);
    for (int i = 0; i < 8; i++)
        second[i] = XorShift();
    CHECK_MEM(first, second, sizeof first);
    SetRng(1, 1, 1, 0, 0);
    SeedRand(43);
    CHECK_NE_INT(XorShift(), first[0]);
}

// ---------------------------------------------------------------- XorShift

TEST(core_XorShift_known_sequence)
{
    SetRng(1, 2, 3, 4, 5);
    CHECK_EQ_INT(XorShift(), 0xff0007f8u);
    CHECK_EQ_INT(XorShift(), 0xff2u);
    CHECK_EQ_INT(XorShift(), 0xff003fc2u);
    CHECK_EQ_INT(XorShift(), 0x7076u);
}

TEST(core_XorShift_shifts_the_state)
{
    SetRng(1, 2, 3, 4, 5);
    unsigned w = XorShift();
    CHECK_EQ_INT(g_rngX, 2);
    CHECK_EQ_INT(g_rngY, 3);
    CHECK_EQ_INT(g_rngZ, 4);
    CHECK_EQ_INT(g_rngW, w);
    CHECK_EQ_INT(g_rngT, (unsigned)(5 - 2049));
}

TEST(core_XorShift_matches_model_over_many_steps)
{
    SeedRand(2024);
    g_rngW = 0x12345678;
    g_rngT = 0x9abcdef0;
    Rng r = GetRng();
    for (int i = 0; i < 1000; i++)
        CHECK_EQ_INT(XorShift(), RefNext(&r));
}

TEST(core_XorShift_all_zero_state_stays_zero)
{
    // the program's initial state: the generator gives 0 until seeded
    for (int i = 0; i < 5; i++)
        CHECK_EQ_INT(XorShift(), 0);
}

// ---------------------------------------------------------------- RandRange

TEST(core_RandRange_empty_range_is_zero_without_advancing)
{
    SetRng(1, 2, 3, 4, 5);
    CHECK_EQ_INT(RandRange(5, 5), 0);
    CHECK_EQ_INT(RandRange(-7, -7), 0);
    CHECK_EQ_INT(g_rngW, 4);
    CHECK_EQ_INT(g_rngT, 5);
}

TEST(core_RandRange_is_xorshift_mod_width_plus_lo)
{
    SetRng(1, 2, 3, 4, 5);
    // 0xff0007f8 % 10 + 10, 0xff2 % 10 + 10
    CHECK_EQ_INT(RandRange(10, 20), 0xff0007f8u % 10 + 10);
    CHECK_EQ_INT(RandRange(10, 20), 0xff2u % 10 + 10);
    CHECK_EQ_INT(RandRange(-100, 100), (int)(0xff003fc2u % 200) - 100);
}

TEST(core_RandRange_stays_in_bounds_and_reaches_both_ends)
{
    SeedRand(7);
    int seen[7] = {0};
    for (int i = 0; i < 5000; i++) {
        int v = RandRange(-3, 4);
        CHECK_MSG(v >= -3 && v < 4, "RandRange(-3, 4) gave %d", v);
        seen[v + 3]++;
    }
    for (int i = 0; i < 7; i++)
        CHECK_MSG(seen[i] > 0, "value %d never came up", i - 3);
}

// ---------------------------------------------------------------- RandFloat

TEST(core_RandFloat_scales_by_2_pow_minus_32)
{
    SetRng(1, 2, 3, 4, 5);
    CHECK_NEAR(RandFloat(0, 1), 0xff0007f8u / 4294967296.0, 1e-6);
    CHECK_NEAR(RandFloat(10, 20), 10 + 10 * (0xff2u / 4294967296.0), 1e-5);
    CHECK_NEAR(RandFloat(-2, 2), -2 + 4 * (0xff003fc2u / 4294967296.0), 1e-5);
}

TEST(core_RandFloat_stays_in_range)
{
    SeedRand(99);
    float lo = 1e9f, hi = -1e9f;
    for (int i = 0; i < 5000; i++) {
        float v = RandFloat(2.0f, 5.0f);
        CHECK_MSG(v >= 2.0f && v <= 5.0f, "RandFloat(2, 5) gave %g", v);
        if (v < lo)
            lo = v;
        if (v > hi)
            hi = v;
    }
    CHECK(lo < 2.01f);
    CHECK(hi > 4.99f);
}

// ---------------------------------------------------------------- Rand7f / Randff / Rand1ff

TEST(core_Rand_masks_take_the_low_bits)
{
    SetRng(1, 2, 3, 4, 5);
    CHECK_EQ_INT(Rand7f(), 0xff0007f8u & 0x7f);
    CHECK_EQ_INT(Randff(), 0xff2u & 0xff);
    CHECK_EQ_INT(Rand1ff(), 0xff003fc2u & 0x1ff);
}

TEST(core_Rand_masks_cover_their_full_range)
{
    SeedRand(5);
    int max7 = 0, maxff = 0, max1ff = 0;
    for (int i = 0; i < 20000; i++) {
        int a = Rand7f(), b = Randff(), c = Rand1ff();
        CHECK(a >= 0 && a <= 0x7f);
        CHECK(b >= 0 && b <= 0xff);
        CHECK(c >= 0 && c <= 0x1ff);
        if (a > max7)
            max7 = a;
        if (b > maxff)
            maxff = b;
        if (c > max1ff)
            max1ff = c;
    }
    CHECK_EQ_INT(max7, 0x7f);
    CHECK_EQ_INT(maxff, 0xff);
    CHECK_EQ_INT(max1ff, 0x1ff);
}

// ---------------------------------------------------------------- MakeRandomId

TEST(core_MakeRandomId_is_in_range_for_many_seeds)
{
    for (int s = 1; s <= 40; s++) {
        SeedRand(s * 7919);
        g_mouseX = s;
        g_mouseY = 3 * s;
        __int64 id = MakeRandomId();
        CHECK_MSG(id > 0 && id <= 9999999999999LL, "seed %d: id %lld", s, (long long)id);
    }
}

static __int64 IdWith(int mouseX, int mouseY, unsigned short ms)
{
    SetRng(11, 22, 33, 44, 55);
    g_mouseX = mouseX;
    g_mouseY = mouseY;
    g_fake.localDate.milliseconds = ms;
    return MakeRandomId();
}

TEST(core_MakeRandomId_is_deterministic)
{
    __int64 a = IdWith(100, 200, 500);
    __int64 b = IdWith(100, 200, 500);
    CHECK_EQ_INT(a, b);
}

// (The mouse position seeds the accumulator, but the shifts in the loop wash it out: it
// rarely changes the result.)
TEST(core_MakeRandomId_depends_on_rng_and_clock)
{
    __int64 base = IdWith(100, 200, 500);
    CHECK_NE_INT(IdWith(100, 200, 501), base);
    CHECK_NE_INT(IdWith(100, 200, 0), base);
    // another generator state (the mouse at 0, 0 so only the generator differs)
    __int64 zero = IdWith(0, 0, 500);
    SetRng(12, 22, 33, 44, 55);
    CHECK_NE_INT(MakeRandomId(), zero);
}

// ---------------------------------------------------------------- trig tables

TEST(core_InitTrigTables_tenth_degree_steps)
{
    InitTrigTables();
    CHECK_NEAR(g_sinTable[0], 0.0, 1e-6);
    CHECK_NEAR(g_cosTable[0], 1.0, 1e-6);
    CHECK_NEAR(g_sinTable[300], 0.5, 1e-5);         // 30 degrees
    CHECK_NEAR(g_cosTable[600], 0.5, 1e-5);         // 60 degrees
    CHECK_NEAR(g_sinTable[900], 1.0, 1e-5);
    CHECK_NEAR(g_cosTable[900], 0.0, 1e-5);
    CHECK_NEAR(g_cosTable[1800], -1.0, 1e-5);
    CHECK_NEAR(g_sinTable[2700], -1.0, 1e-5);
    CHECK_NEAR(g_sinTable[1], sin(0.1 * M_PI / 180), 1e-7);
    CHECK_NEAR(g_sinTable[3599], sin(359.9 * M_PI / 180), 1e-5);
    CHECK_NEAR(g_cosTable[3599], cos(359.9 * M_PI / 180), 1e-5);
}

TEST(core_BuildSinCos_fills_the_fine_tables)
{
    BuildSinCos();
    CHECK_NEAR(g_sinTableFine[0], 0.0, 1e-6);
    CHECK_NEAR(g_cosTableFine[0], 1.0, 1e-6);
    CHECK_NEAR(g_sinTableFine[450], sqrt(0.5), 1e-5);
    CHECK_NEAR(g_sinTableFine[900], 1.0, 1e-5);
    CHECK_NEAR(g_cosTableFine[900], 0.0, 1e-5);
    CHECK_NEAR(g_cosTableFine[1200], -0.5, 1e-5);
    CHECK_NEAR(g_sinTableFine[1], sin(0.1 * M_PI / 180), 1e-7);
    CHECK_NEAR(g_sinTableFine[3599], sin(359.9 * M_PI / 180), 1e-5);
    // the coarse tables are a separate pair
    CHECK_EQ_INT(g_sinTable[900] == 0.0f, 1);
}

// ---------------------------------------------------------------- CRT wrappers

TEST(core_Fabs_wrappers)
{
    CHECK_NEAR(FabsF(-2.5f), 2.5, 0);
    CHECK_NEAR(FabsF(3.25f), 3.25, 0);
    CHECK_NEAR(FabsWindow(-7.0f), 7.0, 0);
    CHECK_NEAR(FabsExplosion(-0.5f), 0.5, 0);
}

TEST(core_Sqrt_wrappers)
{
    CHECK_NEAR(Sqrtf(16.0f), 4.0, 0);
    CHECK_NEAR(Sqrtf(2.0f), 1.41421356, 1e-6);
    CHECK_NEAR(Sqrt(81.0f), 9.0, 0);
}

TEST(core_Sin_Cos_wrappers)
{
    CHECK_NEAR(Sin((float)(M_PI / 2)), 1.0, 1e-6);
    CHECK_NEAR(Sin(0.5f), sin(0.5), 1e-6);
    CHECK_NEAR(Sin2((float)(M_PI / 6)), 0.5, 1e-6);
    CHECK_NEAR(Cos(0.0f), 1.0, 1e-6);
    CHECK_NEAR(Cos(0.5f), cos(0.5), 1e-6);
    CHECK_NEAR(Cos2((float)M_PI), -1.0, 1e-6);
}

// ---------------------------------------------------------------- screen clamps

TEST(core_ClampX_to_screen_width)
{
    CHECK_EQ_INT(ClampX(-5), 0);
    CHECK_EQ_INT(ClampX(0), 0);
    CHECK_EQ_INT(ClampX(400), 400);
    CHECK_EQ_INT(ClampX(799), 799);
    CHECK_EQ_INT(ClampX(800), 799);
    CHECK_EQ_INT(ClampX(100000), 799);
    g_screenW = 1024;
    CHECK_EQ_INT(ClampX(900), 900);
    CHECK_EQ_INT(ClampX(1024), 1023);
}

TEST(core_ClampY_to_screen_height)
{
    CHECK_EQ_INT(ClampY(-1), 0);
    CHECK_EQ_INT(ClampY(0), 0);
    CHECK_EQ_INT(ClampY(599), 599);
    CHECK_EQ_INT(ClampY(600), 599);
    CHECK_EQ_INT(ClampY(700), 599);
    g_screenH = 768;
    CHECK_EQ_INT(ClampY(700), 700);
    CHECK_EQ_INT(ClampY(768), 767);
}

// ---------------------------------------------------------------- static_init.c

TEST(core_static_globals_initial_values)
{
    CHECK_EQ_INT(g_floorY, 550);
    CHECK_EQ_INT(g_saveMsgBlinkRate, 450);
}

TEST(core_InitStaticGlobals_on_initial_rng_is_200)
{
    // the generator's initial (all zero) state gives 0, so every colour is the range's start
    InitStaticGlobals();
    CHECK_NEAR(g_endR, 200, 0);
    CHECK_NEAR(g_endG, 200, 0);
    CHECK_NEAR(g_endB, 200, 0);
}

TEST(core_InitStaticGlobals_rolls_r_g_b_in_order)
{
    SetRng(1, 2, 3, 4, 5);
    InitStaticGlobals();
    CHECK_NEAR(g_endR, 0xff0007f8u % 50 + 200, 0);
    CHECK_NEAR(g_endG, 0xff2u % 50 + 200, 0);
    CHECK_NEAR(g_endB, 0xff003fc2u % 50 + 200, 0);
}

// ---------------------------------------------------------------- stubs.c

TEST(core_stub_constants)
{
    CHECK_EQ_INT(ReturnTrueNet(), 1);
    CHECK_EQ_INT(ReturnZero(), 0);
    CHECK_EQ_INT(EmptyEscGateCheck(), 0);
}
