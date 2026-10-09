// Tests for src/game/promotion.c: the rank-promotion fanfare (PlayRankPromotionSounds), the
// promotion screen (DrawRankPromoHud) and the in-game promotion banner (DrawRankPromoBanner).
#include "support.h"

// ---------------------------------------------------------------------------------------
// PlayRankPromotionSounds
// ---------------------------------------------------------------------------------------

enum {
    S_CONGRATS = 1, S_RANK, S_LIEUTENANT, S_COMMANDER, S_CAPTAIN, S_ADMIRAL, S_ONE, S_TWO,
    S_THREE, S_BRONZE, S_SILVER, S_GOLD, S_STAR, S_STARS, S_WARBLADE, S_KNIGHT, S_LORD,
    S_OVERLORD, S_GRANDMASTER, S_CHAMPION, S_GOD, S_PLUTO, S_NEPTUNE, S_URANUS, S_SATURN,
    S_JUPITER, S_MARS, S_TELLUS, S_VENUS, S_MERCURY, S_SOL, S_ULTIMATE
};

// Sound on, every sample a distinct fake handle, music left alone (an external playlist).
static void SoundSetUp(void)
{
    g_soundEnabled = 1;
    g_cfg.sfxOn = 1;
    g_cfg.musicFormat = MUSIC_FMT_PLAYLIST;
    g_playlistCount = 1;
    g_time = 5000;
    g_sfxCongratulations = S_CONGRATS;
    g_sfxRank = S_RANK;
    g_sfxRankLieutenant = S_LIEUTENANT;
    g_sfxRankCommander = S_COMMANDER;
    g_sfxRankCaptain = S_CAPTAIN;
    g_sfxRankAdmiral = S_ADMIRAL;
    g_sfxVoiceOne = S_ONE;
    g_sfxVoiceTwo = S_TWO;
    g_sfxVoiceThree = S_THREE;
    g_sfxRankBronze = S_BRONZE;
    g_sfxRankSilver = S_SILVER;
    g_sfxRankGold = S_GOLD;
    g_sfxStar = S_STAR;
    g_sfxStars = S_STARS;
    g_sfxWarblade = S_WARBLADE;
    g_sfxRankKnight = S_KNIGHT;
    g_sfxRankLord = S_LORD;
    g_sfxRankOverlord = S_OVERLORD;
    g_sfxRankGrandmaster = S_GRANDMASTER;
    g_sfxRankChampion = S_CHAMPION;
    g_sfxRankGod = S_GOD;
    g_sfxPlanetPluto = S_PLUTO;
    g_sfxPlanetNeptune = S_NEPTUNE;
    g_sfxPlanetUranus = S_URANUS;
    g_sfxPlanetSaturn = S_SATURN;
    g_sfxPlanetJupiter = S_JUPITER;
    g_sfxPlanetMars = S_MARS;
    g_sfxPlanetTellus = S_TELLUS;
    g_sfxPlanetVenus = S_VENUS;
    g_sfxPlanetMercury = S_MERCURY;
    g_sfxPlanetSol = S_SOL;
    g_sfxUltimateRank = S_ULTIMATE;
}

// Promotes player 2 to `rank` and checks the queued samples and the gap (ms) after each:
// `seq` is sample, delay, sample, delay, ..., 0. Every fake sample lasts 1000 ms.
static void CheckFanfare(int rank, const int *seq)
{
    SoundSetUp();
    g_soundQueueCount = 0;
    g_soundQueueNext = 0;
    g_promoPlayer = 2;
    g_save.players[2].rank = rank;
    PlayRankPromotionSounds();
    int n = 0;
    while (seq[2 * n])
        n++;
    CHECK_MSG(g_soundQueueCount == n, "rank %d: %d sounds queued, expected %d", rank,
              g_soundQueueCount, n);
    for (int i = 0; i < n; i++) {
        unsigned next = i + 1 < n ? g_soundQueue[i + 1].time : g_soundQueueNext;
        CHECK_MSG((int)g_soundQueue[i].sample == seq[2 * i],
                  "rank %d, sound %d: sample %d, expected %d", rank, i,
                  (int)g_soundQueue[i].sample, seq[2 * i]);
        CHECK_MSG((int)(next - g_soundQueue[i].time - 1000) == seq[2 * i + 1],
                  "rank %d, sound %d: delay %d, expected %d", rank, i,
                  (int)(next - g_soundQueue[i].time - 1000), seq[2 * i + 1]);
    }
}

TEST(Stats_Promotion_ensign_gets_only_the_congratulations)
{
    static const int seq[] = {S_CONGRATS, 100, 0};
    CheckFanfare(RANK_ENSIGN, seq);
}

TEST(Stats_Promotion_plain_ranks_announce_the_rank)
{
    static const int lieutenant[] = {S_CONGRATS, 100, S_LIEUTENANT, 50, S_RANK, 50, 0};
    static const int commander[] = {S_CONGRATS, 100, S_COMMANDER, 50, S_RANK, 50, 0};
    static const int captain[] = {S_CONGRATS, 100, S_CAPTAIN, 50, S_RANK, 50, 0};
    static const int admiral[] = {S_CONGRATS, 100, S_ADMIRAL, 50, S_RANK, 50, 0};
    CheckFanfare(RANK_LIEUTENANT, lieutenant);
    CheckFanfare(RANK_COMMANDER, commander);
    CheckFanfare(RANK_CAPTAIN, captain);
    CheckFanfare(RANK_ADMIRAL, admiral);
}

TEST(Stats_Promotion_admiral_tiers_count_their_stars)
{
    static const int voices[3] = {S_ONE, S_TWO, S_THREE};
    static const int stars[3] = {S_STAR, S_STARS, S_STARS};
    static const int medals[3] = {S_BRONZE, S_SILVER, S_GOLD};
    for (int tier = 0; tier < 3; tier++) {
        for (int n = 0; n < 3; n++) {
            int seq[] = {S_CONGRATS, 100, S_ADMIRAL, 100, S_RANK, 50, voices[n], 5,
                         medals[tier], 5, stars[n], 0, 0};
            CheckFanfare(RANK_ADMIRAL_1_1 + tier * 3 + n, seq);
        }
    }
}

TEST(Stats_Promotion_warblade_ranks_play_the_sting)
{
    static const int knight[] = {S_CONGRATS, 100, S_WARBLADE, 20, S_KNIGHT, 50, S_RANK, 50, 0};
    static const int lord[] = {S_CONGRATS, 100, S_WARBLADE, 20, S_LORD, 50, S_RANK, 50, 0};
    static const int overlord[] = {S_CONGRATS, 100, S_WARBLADE, 20, S_OVERLORD, 50, S_RANK, 50, 0};
    static const int gm[] = {S_CONGRATS, 100, S_WARBLADE, 20, S_GRANDMASTER, 50, S_RANK, 50, 0};
    CheckFanfare(RANK_KNIGHT, knight);
    CheckFanfare(RANK_LORD, lord);
    CheckFanfare(RANK_OVERLORD, overlord);
    CheckFanfare(RANK_GRANDMASTER, gm);
}

TEST(Stats_Promotion_grandmaster_tiers_count_their_gold_stars)
{
    static const int voices[3] = {S_ONE, S_TWO, S_THREE};
    static const int stars[3] = {S_STAR, S_STARS, S_STARS};
    for (int n = 0; n < 3; n++) {
        int seq[] = {S_CONGRATS, 100, S_WARBLADE, 20, S_GRANDMASTER, 20, S_RANK, 50,
                     voices[n], 5, S_GOLD, 5, stars[n], 0, 0};
        CheckFanfare(RANK_GRANDMASTER_1 + n, seq);
    }
}

TEST(Stats_Promotion_champion_and_god)
{
    static const int champion[] = {S_CONGRATS, 100, S_WARBLADE, 40, S_CHAMPION, 50, S_RANK, 50, 0};
    static const int god[] = {S_CONGRATS, 100, S_WARBLADE, 40, S_GOD, 50, S_RANK, 50, 0};
    CheckFanfare(RANK_CHAMPION, champion);
    CheckFanfare(RANK_GOD, god);
}

TEST(Stats_Promotion_god_planet_ranks_name_the_planet)
{
    static const int planets[9] = {S_PLUTO, S_NEPTUNE, S_URANUS, S_SATURN, S_JUPITER,
                                   S_MARS, S_TELLUS, S_VENUS, S_MERCURY};
    for (int i = 0; i < 9; i++) {
        int seq[] = {S_CONGRATS, 100, S_WARBLADE, 40, S_GOD, 300, planets[i], 40,
                     S_RANK, 50, 0};
        CheckFanfare(RANK_GOD_PLUTO + i, seq);
    }
}

TEST(Stats_Promotion_sol_is_the_ultimate_rank)
{
    static const int sol[] = {S_CONGRATS, 100, S_ULTIMATE, 150, S_WARBLADE, 100, S_GOD, 100,
                              S_SOL, 40, S_RANK, 50, 0};
    CheckFanfare(RANK_GOD_SOL, sol);
}

TEST(Stats_Promotion_uses_the_promoted_players_rank)
{
    static const int seq[] = {S_CONGRATS, 100, S_CAPTAIN, 50, S_RANK, 50, 0};
    g_curPlayer = 0;
    g_save.players[0].rank = RANK_KNIGHT;
    CheckFanfare(RANK_CAPTAIN, seq);
}

TEST(Stats_Promotion_starts_the_promoted_music)
{
    g_cfg.musicFormat = 1;
    PlayRankPromotionSounds();
    CHECK_STR(g_songName, "promoted");
    CHECK_EQ_INT(g_musicMode, MUSIC_PROMOTED);
}

TEST(Stats_Promotion_music_without_playlist_songs)
{
    g_cfg.musicFormat = MUSIC_FMT_PLAYLIST;
    g_playlistCount = 0;
    PlayRankPromotionSounds();
    CHECK_STR(g_songName, "promoted");
}

TEST(Stats_Promotion_keeps_the_playlist_playing)
{
    g_cfg.musicFormat = MUSIC_FMT_PLAYLIST;
    g_playlistCount = 3;
    g_musicMode = 1;
    PlayRankPromotionSounds();
    CHECK(g_songName == NULL);
    CHECK_EQ_INT(g_musicMode, 1);
}

// ---------------------------------------------------------------------------------------
// DrawRankPromoHud / DrawRankPromoBanner (drawn after the real start-up, for the clip rect
// and the graphics)
// ---------------------------------------------------------------------------------------

static void HudSetUp(int rank)
{
    BootGame();
    g_hiscoreSkipGateActive = 1;
    g_promoPlayer = 1;
    g_save.players[1].rank = rank;
    g_angle0 = g_angle1 = g_angle2 = 0;
    g_blitCount = 0;
}

// Blits of `img` queued since HudSetUp; the first one's source x in *sx.
static int BlitsOf(Image *img, int *sx, float *x)
{
    int n = 0;
    for (int i = 0; i < g_blitCount; i++) {
        if (g_blit[i].graphic == img) {
            if (n == 0) {
                if (sx)
                    *sx = g_blit[i].src.x1;
                if (x)
                    *x = g_blit[i].destX;
            }
            n++;
        }
    }
    return n;
}

TEST(Stats_PromoHud_draws_nothing_before_the_screen_shows)
{
    HudSetUp(RANK_CHAMPION);
    g_hiscoreSkipGateActive = 0;
    g_promoSpecialRank = 1;
    g_promoRingActive = 1;
    DrawRankPromoHud();
    CHECK_EQ_INT(g_promoSpecialRank, 0);
    CHECK_EQ_INT(g_promoRingActive, 0);
    CHECK_NEAR(g_angle0, 0, 0);
    CHECK_EQ_INT(g_blitCount, 0);
}

TEST(Stats_PromoHud_champion_spins_the_flares)
{
    HudSetUp(RANK_CHAMPION);
    DrawRankPromoHud();
    CHECK_EQ_INT(g_promoSpecialRank, 1);
    CHECK_EQ_INT(g_promoRingActive, 0);
    CHECK_NEAR(g_angle0, 0.1, 1e-5);
    CHECK_NEAR(g_angle1, 359.7, 1e-3);
    CHECK_NEAR(g_angle2, 0.8, 1e-5);
}

TEST(Stats_PromoHud_flare_angles_wrap_at_360)
{
    HudSetUp(RANK_GOD);
    g_angle0 = 359.95f;
    g_angle1 = 10.0f;
    g_angle2 = 359.5f;
    DrawRankPromoHud();
    CHECK_NEAR(g_angle0, 0.05, 1e-3);
    CHECK_NEAR(g_angle1, 9.7, 1e-3);
    CHECK_NEAR(g_angle2, 0.3, 1e-3);
}

TEST(Stats_PromoHud_planet_ranks_spin_the_ring)
{
    HudSetUp(RANK_GOD_URANUS);
    DrawRankPromoHud();
    CHECK_EQ_INT(g_promoSpecialRank, 1);
    CHECK_EQ_INT(g_promoRingActive, 1);
    CHECK_EQ_INT(g_promoRingIndex, 7);
    CHECK_NEAR(g_angle0, 0.15, 1e-5);
    CHECK_NEAR(g_angle1, 0.3, 1e-5);
    CHECK_NEAR(g_angle2, 0.6, 1e-5);

    HudSetUp(RANK_GOD_SOL);
    DrawRankPromoHud();
    CHECK_EQ_INT(g_promoRingIndex, 0);
    HudSetUp(RANK_GOD_PLUTO);
    g_angle0 = 359.9f;
    g_angle1 = 359.9f;
    g_angle2 = 359.9f;
    DrawRankPromoHud();
    CHECK_EQ_INT(g_promoRingIndex, 9);
    CHECK_NEAR(g_angle0, 0.05, 1e-3);
    CHECK_NEAR(g_angle1, 0.2, 1e-3);
    CHECK_NEAR(g_angle2, 0.5, 1e-3);
}

TEST(Stats_PromoHud_plain_rank_shows_name_and_badge)
{
    HudSetUp(RANK_GRANDMASTER_3);
    g_cfg.particlesOn = 0;
    int lines = g_fake.lines;
    DrawRankPromoHud();
    CHECK_EQ_INT(g_promoSpecialRank, 0);
    CHECK_STR(g_logBuf, "WARBLADE GRANDMASTER 3 GOLD STARS");
    CHECK_EQ_INT(g_fake.lines - lines, 60);
    CHECK_EQ_INT(BlitsOf(g_gfxRanks, NULL, NULL), 1);
    for (int i = 0; i < g_blitCount; i++)
        if (g_blit[i].graphic == g_gfxRanks)
            CHECK_EQ_INT(g_blit[i].src.y1, g_rankSprY[RANK_GRANDMASTER_3]);
    CHECK_NEAR(g_angle0, 0, 0);
}

TEST(Stats_PromoHud_sparks_with_particles_on)
{
    HudSetUp(RANK_CAPTAIN);
    g_cfg.particlesOn = 1;
    g_time = 7000;
    g_nextSpark = 0;
    int lines = g_fake.lines;
    DrawRankPromoHud();
    CHECK_EQ_INT(g_nextSpark, 7008);
    CHECK_EQ_INT(g_fake.lines, lines);
    g_time = 7008;
    DrawRankPromoHud();
    CHECK_EQ_INT(g_nextSpark, 7008);
}

static int PipsFor(int rank, int *sx, float *x)
{
    HudSetUp(rank);
    DrawRankPromoHud();
    return BlitsOf(g_gfxLogos, sx, x);
}

TEST(Stats_PromoHud_star_pips_per_rank)
{
    int sx = -1;
    float x = 0;
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL, &sx, &x), 0);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_1_1, &sx, &x), 1);
    CHECK_EQ_INT(sx, 0);
    CHECK_NEAR(x, (g_screenW >> 1) + 40, 0);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_1_2, &sx, &x), 2);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_1_3, &sx, &x), 3);
    CHECK_EQ_INT(sx, 0);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_2_1, &sx, &x), 1);
    CHECK_EQ_INT(sx, 16);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_2_2, &sx, &x), 2);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_2_3, &sx, &x), 3);
    CHECK_EQ_INT(sx, 16);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_3_1, &sx, &x), 1);
    CHECK_EQ_INT(sx, 32);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_3_2, &sx, &x), 2);
    CHECK_EQ_INT(PipsFor(RANK_ADMIRAL_3_3, &sx, &x), 3);
    CHECK_EQ_INT(PipsFor(RANK_KNIGHT, &sx, &x), 0);
    CHECK_EQ_INT(PipsFor(RANK_GRANDMASTER, &sx, &x), 0);
    CHECK_EQ_INT(PipsFor(RANK_GRANDMASTER_1, &sx, &x), 1);
    CHECK_EQ_INT(sx, 32);
    CHECK_EQ_INT(PipsFor(RANK_GRANDMASTER_2, &sx, &x), 2);
    CHECK_EQ_INT(PipsFor(RANK_GRANDMASTER_3, &sx, &x), 3);
    CHECK_EQ_INT(sx, 32);
}

TEST(Stats_PromoHud_blinks_press_fire)
{
    HudSetUp(RANK_CAPTAIN);
    g_rankMsgActive = 1;
    g_promoBlinkTimer = 0;
    g_promoContinueBlink = false;
    g_time = 1000;
    DrawRankPromoHud();
    CHECK(g_promoContinueBlink);
    CHECK_EQ_INT(g_promoBlinkTimer, 1400);
    DrawRankPromoHud();
    CHECK(g_promoContinueBlink);
    g_time = 1401;
    DrawRankPromoHud();
    CHECK(!g_promoContinueBlink);
    CHECK_EQ_INT(g_promoBlinkTimer, 1801);

    g_rankMsgActive = 0;
    g_time = 5000;
    DrawRankPromoHud();
    CHECK(!g_promoContinueBlink);
}

TEST(Stats_PromoBanner_names_the_current_players_rank)
{
    BootGame();
    g_curPlayer = 1;
    g_save.players[0].rank = RANK_CAPTAIN;
    g_save.players[1].rank = RANK_KNIGHT;
    g_blitCount = 0;
    DrawRankPromoBanner();
    CHECK_STR(g_logBuf, "WARBLADE KNIGHT");
    int n = 0;
    for (int i = 0; i < g_blitCount; i++) {
        if (g_blit[i].graphic == g_gfxRanks) {
            n++;
            CHECK_EQ_INT(g_blit[i].src.y1, g_rankSprY[RANK_KNIGHT]);
            CHECK_NEAR(g_blit[i].destY, 480, 0);
        }
    }
    CHECK_EQ_INT(n, 1);
}
