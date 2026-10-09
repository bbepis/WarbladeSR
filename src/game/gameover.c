// gameover.c: End of a game: the post-game sequence, bonus tally, game-over layers, the
// completion sequence.
#include "globals.h"
#include "game.h"

// Cash-tally "odometer" chunk sizes (biggest that still fits, checked biggest to smallest) and
// the score awarded per unit of cash drained that way.
enum {
    CASH_CHUNK_10000000 = 10000000,
    CASH_CHUNK_1000000  = 1000000,
    CASH_CHUNK_100000   = 100000,
    CASH_CHUNK_10000    = 10000,
    CASH_CHUNK_1000     = 1000,
    CASH_CHUNK_100      = 100,
    CASH_TO_SCORE_MULT  = 100,     // score credited per unit of cash the tally drains
    MAX_CASH_BONUS_MULT = 5,       // maxCashBonus = MAX_CASH_BONUS_MULT * CASH_CHUNK_10000000
    PERFECT_BONUS_SCORE = 100000,  // tally bonus per perfect bonus-round clear
    HIT_PERCENT_BONUS   = 1000,    // tally bonus per hit-percentage point
};

// Rank bonus by g_bonusTally[].rank (rank is incremented before it indexes this table, so
// index 0 is never read).
static const __int64 g_rankBonusTable[33] = {
    10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000,
    200000, 300000, 400000, 500000, 600000, 700000, 800000, 1000000,
    2000000, 3000000, 4000000, 5000000,
    10000000, 10000000, 10000000, 10000000, 10000000, 10000000, 10000000, 10000000, 10000000, 10000000,
    50000000
};

// Advances the end-of-level results-screen tally by one tick for each of `count` players/slots: drains
// remaining cash (in decreasing chunk sizes so it counts down with a satisfying "odometer" effect), then
// rank bonus, then perfect-run bonus, then hit-percentage, one category at a time in priority order.
// Sets g_tallyDelay to the shortest per-category tick delay needed this frame; returns whether anything
// changed (the tally screen keeps calling this until it returns 0, i.e. everything is settled).
int TallyStep(int count)
{
    int changed;
    int i;

    changed = 0;
    g_tallyDelay = 1000;
    for (i = 0; i < count; i++) {
        if (g_bonusTally[i].cashRemaining > 0) {
            changed = 1;
            // Largest chunk size that still fits in cashRemaining, checked from biggest to smallest.
            if (g_bonusTally[i].cashRemaining >= CASH_CHUNK_10000000) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining
                    - CASH_CHUNK_10000000;
                g_bonusTally[i].cash = g_bonusTally[i].cash
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_10000000;
                g_bonusTally[i].total = g_bonusTally[i].total
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_10000000;

            } else if (g_bonusTally[i].cashRemaining >= CASH_CHUNK_1000000) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining
                    - CASH_CHUNK_1000000;
                g_bonusTally[i].cash = g_bonusTally[i].cash
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_1000000;
                g_bonusTally[i].total = g_bonusTally[i].total
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_1000000;

            } else if (g_bonusTally[i].cashRemaining >= CASH_CHUNK_100000) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining
                    - CASH_CHUNK_100000;
                g_bonusTally[i].cash = g_bonusTally[i].cash
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_100000;
                g_bonusTally[i].total = g_bonusTally[i].total
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_100000;

            } else if (g_bonusTally[i].cashRemaining >= CASH_CHUNK_10000) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining
                    - CASH_CHUNK_10000;
                g_bonusTally[i].cash = g_bonusTally[i].cash
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_10000;
                g_bonusTally[i].total = g_bonusTally[i].total
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_10000;

            } else if (g_bonusTally[i].cashRemaining >= CASH_CHUNK_1000) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining
                    - CASH_CHUNK_1000;
                g_bonusTally[i].cash = g_bonusTally[i].cash
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_1000;
                g_bonusTally[i].total = g_bonusTally[i].total
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_1000;

            } else if (g_bonusTally[i].cashRemaining >= CASH_CHUNK_100) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining
                    - CASH_CHUNK_100;
                g_bonusTally[i].cash = g_bonusTally[i].cash
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_100;
                g_bonusTally[i].total = g_bonusTally[i].total
                    + CASH_TO_SCORE_MULT * CASH_CHUNK_100;

            } else if (g_bonusTally[i].cashRemaining >= 10) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining - 10;
                g_bonusTally[i].cash = g_bonusTally[i].cash + CASH_TO_SCORE_MULT * 10;
                g_bonusTally[i].total = g_bonusTally[i].total + CASH_TO_SCORE_MULT * 10;

            } else if (g_bonusTally[i].cashRemaining >= 1) {
                g_bonusTally[i].cashRemaining = g_bonusTally[i].cashRemaining - 1;
                g_bonusTally[i].cash = g_bonusTally[i].cash + CASH_TO_SCORE_MULT;
                g_bonusTally[i].total = g_bonusTally[i].total + CASH_TO_SCORE_MULT;
            }
            if (g_bonusTally[i].cashRemaining == 0)
                g_tallyDelay = g_tallyDelay < 300 ? g_tallyDelay : 300;
            else
                g_tallyDelay = g_tallyDelay < 100 ? g_tallyDelay : 100;

        } else if (g_bonusTally[i].maxCashFlag > 0 && g_bonusTally[i].maxCashFlag != -1) {
            changed = 1;
            g_bonusTally[i].maxCashFlag = g_bonusTally[i].maxCashFlag - 1;
            g_bonusTally[i].maxCashBonus = (__int64)CASH_CHUNK_10000000 * MAX_CASH_BONUS_MULT;
            g_bonusTally[i].total = g_bonusTally[i].total + (__int64)CASH_CHUNK_10000000 * MAX_CASH_BONUS_MULT;

        } else if (g_bonusTally[i].rankRemaining > 0) {
            changed = 1;
            g_bonusTally[i].rank++;
            g_bonusTally[i].rankRemaining = g_bonusTally[i].rankRemaining - 1;
            if (g_bonusTally[i].rankRemaining == 0)
                g_tallyDelay = g_tallyDelay < 300 ? g_tallyDelay : 300;
            else
                g_tallyDelay = g_tallyDelay < 300 ? g_tallyDelay : 300;
            g_bonusTally[i].rankBonus = g_bonusTally[i].rankBonus
                + g_rankBonusTable[g_bonusTally[i].rank];
            g_bonusTally[i].total = g_bonusTally[i].total
                + g_rankBonusTable[g_bonusTally[i].rank];

        } else if (g_bonusTally[i].perfectsRemaining > 0) {
            changed = 1;
            g_bonusTally[i].perfects++;
            g_bonusTally[i].perfectsRemaining = g_bonusTally[i].perfectsRemaining - 1;
            if (g_bonusTally[i].perfectsRemaining == 0)
                g_tallyDelay = g_tallyDelay < 300 ? g_tallyDelay : 300;
            else
                g_tallyDelay = g_tallyDelay < 150 ? g_tallyDelay : 150;
            g_bonusTally[i].perfectsBonus = g_bonusTally[i].perfectsBonus + PERFECT_BONUS_SCORE;
            g_bonusTally[i].total = g_bonusTally[i].total + PERFECT_BONUS_SCORE;

        } else if (g_bonusTally[i].hitPercent != g_bonusTally[i].hitPercentTarget) {
            changed = 1;
            g_bonusTally[i].unusedTickCount++;
            g_bonusTally[i].hitPercent = g_bonusTally[i].hitPercent + 1;
            if (g_bonusTally[i].hitPercent == g_bonusTally[i].hitPercentTarget)
                g_tallyDelay = g_tallyDelay < 300 ? g_tallyDelay : 300;
            else
                g_tallyDelay = g_tallyDelay < 75 ? g_tallyDelay : 75;
            g_bonusTally[i].hitBonus = g_bonusTally[i].hitBonus + HIT_PERCENT_BONUS;
            g_bonusTally[i].total = g_bonusTally[i].total + HIT_PERCENT_BONUS;

        } else {
            g_tallyDelay = g_tallyDelay < 500 ? g_tallyDelay : 500;
        }
    }
    return changed;
}

// Initializes the end-of-round bonus tally for HUD `slot` from `player`'s saved stats
// (cash, perfects, rank, hit percent), ready for the tally screen's count-up animation.
void InitPlayerStats(int slot, int player)
{
    int pct;

    g_tallyTime = g_time;
    g_bonusTally[slot].cashRemaining = g_save.players[player].money;
    g_bonusTally[slot].maxCashFlag = -1;
    g_bonusTally[slot].cash = 0;
    g_bonusTally[slot].maxCashBonus = 0;
    g_bonusTally[slot].perfectsRemaining = (int)g_save.players[player].bonusRoundCount;
    g_bonusTally[slot].perfects = 0;
    g_bonusTally[slot].perfectsBonus = 0;
    g_bonusTally[slot].rankRemaining = g_save.players[player].rank;
    if (g_save.players[player].shots == 0)
        g_save.players[player].shots = 1;
    pct = (int)((double)g_save.players[player].hits / g_save.players[player].shots * 100.0);
    if (pct > 100)
        pct = 100;
    g_bonusTally[slot].hitPercent = 0;
    g_bonusTally[slot].hitPercentTarget = pct;
    g_bonusTally[slot].rank = 0;
    g_bonusTally[slot].rankBonus = 0;
    g_bonusTally[slot].total = 0;
    g_bonusTally[slot].hitBonus = 0;
}

// Sets up the 10 fading/receding FX layers used on the game-over screen: a skull (layer 0)
// in front of white game-over graphics that shrink in speed and alpha, layer by layer.
void InitLayers()
{
    int alpha;
    float speed;
    int i;

    alpha = 255;
    speed = 20.0f;

    for (i = 0; i < MAX_FX; i++) {
        g_fx[i].active = 1;
        g_fx[i].gfx = g_gfxGameOver;
        g_fx[i].scale = 1.0f;
        g_fx[i].speed = speed;
        g_fx[i].x = g_screenW / 2.0f;
        g_fx[i].y = g_screenH / 2.0f;
        g_fx[i].alpha = alpha;
        g_fx[i].unusedTimer10 = 0;
        g_fx[i].rInit = RandRange(0, 255);
        g_fx[i].gInit = RandRange(0, 255);
        g_fx[i].bInit = RandRange(0, 255);

        // The first sprite is the skull, drawn full white and full alpha; the second
        // sprite is also forced to white but keeps its own alpha fade.
        if (i == 0) {
            g_fx[i].gfx = g_gfxSkull;
            g_fx[i].rInit = 255;
            g_fx[i].gInit = 255;
            g_fx[i].bInit = 255;
            g_fx[i].alpha = 255.0f;
        }
        if (i == 1) {
            g_fx[i].rInit = 255;
            g_fx[i].gInit = 255;
            g_fx[i].bInit = 255;
        }
        if ((alpha -= 25, speed /= 1.5, alpha) < 0)
            alpha = 0;
    }
}

// Runs the game-completion sequence: scrolling credits text over a slideshow of "ending_*.jpg"
// pictures with cross-fades, plus an optional screen-wobble effect. Blocks in its own loop,
// polling input each frame, until the player presses space/escape/ctrl (or the credits reach
// the end). Resets round state and returns to the caller when done.
void EndSequence()
{
    unsigned int startTime;
    int mode;                  // unused/vestigial (write-only)
    char t1[] =
"|Congratulations\n"
        "\n"
        "\n"
        "Our mission has been a success!\n"
        "\n"
        "In an invasion from far away, the known enemies of earth\n"
        "have been defeated.  As we embark on our long journey home,\n"
        "we must contemplate our next mission.\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "Once in warp we are sucked into a worm hole,\n"
        "it is as though only minutes have passed--\n"
        "\n"
        "\n"
        "\n"
        "We exit to find we are in an uncharted galaxy..\n"

        "\n"
        "It has been days rather than minutes since we entered the worm hole--\n"
        "\n"
        "This galaxy being unfamiliar,\n"
        "we wait for the unexpected...\n"
        "\n"
        "\n"
        "\n"
        "We are lost...\n"
        "\n"
        "\n"
        "\n"
        "The joy of our victory is dampened as we proceed with caution.\n"
        "In the distance we pick up something on scanners..\n"
        "\n"
        "\n"
        "We wait, what is approaching?\n"
        "\n"

        "\n"
        "In a sudden flash we are surrounded!!\n"
        "\n"
        "New enemies everywhere\n"
        "\n"
        "Guns are blasting\n"
        "\n"
        "We must regroup... we must begin again!!\n"
        "\n"
        "The war rages on.   May the fate of the gods be with you!\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"

        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "- - -  CREDITS  - - -\n"
        "\n"
        "\n"
        "Game design, programming, graphics, soundeffects, level design\n"
        "\n"
        "by\n"
        "\n"
        "Edgar M Vigdal\n"
        "\n"
        "\n"
        "\n"

        "\n"
        "\n"
        "Additional game design\n"
        "\n"
        "by\n"
        "\n"
        "Simon Quincey, Joshua Wayne, SR\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "Additional level design\n"
        "\n"
        "by\n"
        "\n"
        "Simon Quincey, Joshua Wayne\n"
        "\n"

        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "Vocals and vocoder voice effects\n"
        "\n"
        "By\n"
        "\n"
        "Simon Quincey and Vanessa Quincey\n"
        "\n"
        "Camren Harm and Angela Strauss\n"
        "\n"
        "Violette and Yannis Brown\n"
        "\n"
        "\n"
        "\n"

        "\n"
        "\n"
        "\n"
        "\n"
        "MOD music\n"
        "\n"
        "By\n"
        "\n"
        "Yannis Brown\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "MP3 music\n"
        "\n"

        "By\n"
        "\n"
        "VANDARK\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "Other MOD music credits\n"
        "\n"
        "Memory station music : DRAX\n"
        "\n"
        "Promoted music : MANIAC\n"
        "\n"
        "Boss music : SETH PEELLE\n"
        "\n"

        "End game music : Skaven of Future Crew\n"
        "Peter Hajba\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "- - -  Beta Testers  - - -\n"
        "\n"
        "Simon Quincey, Joshua Wayne, Odd Rune Olsen\n"
        "\n"
        "Sharlene A Wade, Scott Bell, Roland Meinhard\n"
        "\n"
        "\n"
        "\n"
        "\n"

        "\n"
        "- - -  End Story  - - -\n"
        "\n"
        "Sharlene A Wade\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "- - -  Greetings  - - -\n"
        "\n"
        "\n"
        "I (Edgar) would like to thank the following people for all their help:\n"

        "\n"
        "Siv Lise : For holding out through years of late night coding and almost\n"
        "\n"
        "never seeing me...  I love you!!\n"
        "\n"
        "\n"
        "To Marta and Rebekka for giving my life a meaning!\n"
        "\n"
        "\n"
        "Hitm4n : Thank you for all your good critique! And for the very good levels!!\n"
        "\n"
        "Mrs. Hitm4n : For lending me your voice\n"
        "\n"
        "Karth : Thanks for the many ideas and excellent levels\n"
        "\n"
        "Olsen : For beta testing and help on both my galaga games!\n"
        "\n"
        "To my colleagues at DataPart\n"

        "\n"
        "\n"
        "...and to all that believed in me and supported me through the whole\n"
        "\n"
        "development period. The game would not be, if not for your support!\n"
        "\n"
        "\n"
        "Thank you all!!\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "Greetings from Hitm4n\n"
        "\n"
        "Hitm4n has to say massive love and respect to Mrs.Hitm4n, for she\n"
        "\n"

        "lets me play and test Warblade while the pots stay unwashed and\n"
        "\n"
        "the room stays untidied. I love ya babes...\n"
        "\n"
        "\n"
        "Big hugs and kisses to my kids Neo and Lila (I hope you love this\n"
        "\n"
        "game as much as me when you grow up). Love daddy.\n"
        "\n"
        "\n"
        "Sorry Ed, had to put family first ;-) Massive respect to Edgar\n"
        "\n"
        "for this fine game, for letting me be a beta tester, level\n"
        "\n"
        "maker and ideas man. (I hope you all liked my levels btw)\n"
        "\n"
        "\n"
        "Thanks to everyone in the forum for the help, support and patience\n"

        "\n"
        "throughout the development times, and to those that helped the\n"
        "\n"
        "newbies find their feet. For bug hunting, testing and giving\n"
        "\n"
        "us your ideas.\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "Greetings from Joshua Wayne (aka Karth the Insult Comic Bishounen)\n"
        "\n"
        "Its been a long road.  And I havent been around for the whole thing.\n"
        "\n"
        "But seriously, its been a fun time the folks at the forums are great,\n"
        "\n"

        "and I could prove my low post count meant nothing.  Ask me later.\n"
        "\n"
        "\n"
        "Special Thanks:\n"
        "\n"
        "S:  What can I say?  He introduced me to Warblade!  If not for him, then\n"
        "\n"
        "I would not be in these credits!\n"
        "\n"
        "Edgar:  For making this nostalgia-inducing game which has taken many\n"
        "\n"
        "countless hours of my time away, and obviously for him putting many countless\n"
        "\n"
        "hours into making this game.\n"
        "\n"
        "Hitman:  For being around when the going got rough.\n"
        "\n"
        "Mark Lenzo:  For appreciating my artwork, my creative vision...\n"

        "\n"
        "you know what I mean.\n"
        "\n"
        "Michael MacDonald:  For being the only person who can measure up to my\n"
        "\n"
        "innate weirdness.\n"
        "\n"
        "So, until next mission, keep watching the pies!  ...Wait, I mean skies!\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "Greetings from Sharlene Wade\n"
        "\n"
        "To Edgar: For allowing me to help and be a part of this, Tusen Takk!!\n"
        "\n"

        "To S: For being so kind to me and all the help you have given me,\n"
        "\n"
        "and the wonderful friend you have become.  And to the whole family -- Love!!!\n"
        "\n"
        "To Karth: For all the help you have given me, and the fun we have had.\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"

        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "This game is dedicated to\n"
        "Siv Lise, Marta and Rebekka\n"
        "\n"
        "\n"
        "\n"
        "\n"

        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "HAVE FUN!!!\n"
        "\n"

        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"
        "\n"

        "\0";
    float textY;
    EndRect src;
    // NOTE: unusedTimer/unusedRate/unusedPos/unusedSpeed/unusedMax/unusedAngle/unusedRadius are
    // set (once, or every frame for unusedAngle) but never read anywhere in this function; kept
    // because the original binary has them.
    float unusedTimer;
    float unusedRate;
    float unusedPos;
    float unusedSpeed;
    float unusedMax;
    float unusedAngle;

    float alpha;
    float alphaSpeed;
    float maxAlpha;
    bool fadeDone;
    unsigned int nextTime;
    bool orbits;

    int r;
    int g;
    int b;
    int cy;
    int unusedRadius;
    bool done;
    unsigned int helpUntil;
    float ang;
    int size;
    int cx;

    // ---- setup ----
    g_state = STATE_END_SEQUENCE;
    g_endR = RandRange(210, 252);
    g_endG = RandRange(210, 252);
    g_endB = RandRange(210, 252);
    g_endDR = 0.1f;
    g_endDG = 0.2f;
    g_endDB = 0.05f;
    g_endWobbleActive = g_endWobbleEnabled;
    SoundStopAll();
    g_clipLeft = 0;
    g_clipRight = g_screenW;
    textY = g_screenH + 10.0;

    g_time = SysMillis();
    if (g_time == 0)
        g_time = SysMillis();
    startTime = g_time;
    mode = 4;
    unusedTimer = 0;
    unusedRate = 4.0f;
    g_endSequenceActive = 1;

    src.y = 0;
    src.x = 0;
    src.w = g_screenW;
    src.h = g_screenH;
    SetViewHud();

    unusedPos = 0;
    unusedSpeed = 0.25f;
    unusedMax = 30.0f;
    unusedAngle = 0;
    alpha = 0;
    alphaSpeed = 1.0f;
    maxAlpha = 200.0f;
    fadeDone = true;
    nextTime = g_time;
    orbits = false;
    cy = 300;
    unusedRadius = 500;
    r = 255;
    g = 255;
    b = 255;
    done = false;
    g_endImg = LoadGraphic("ending_5.jpg", true, true);
    helpUntil = g_time + 8000;

    do {
        g_time = SysMillis();
        if (g_time == 0)
            g_time = SysMillis();

        // ---- input/exit ----
        if ((KeyDown(K_VK_SPACE) == true || KeyDown(K_VK_ESCAPE) == true ||
            KeyDown(K_VK_L_CONTROL) == true)
            && !AnyWindowHasEdit()) {
            WaitForKeyRelease(K_VK_SPACE);
            WaitForKeyRelease(K_VK_ESCAPE);
            WaitForKeyRelease(K_VK_L_CONTROL);
            g_menuIdleTimeout = g_time + MENU_IDLE_MS;
            ResetObjectsKeep();
            g_save.players[0].energy = 0;
            g_save.players[1].energy = 0;
            g_save.players[2].energy = 0;
            g_save.players[3].energy = 0;
            EmptyViewChangeHook();
            g_clipLeft = 64;
            g_clipRight = g_screenW - 64;
            CLEAR_DRAW_COUNTERS()
            return;
        }

        // ---- wobble ----
        unusedAngle = 0;
        if (g_endWobbleActive) {
            ang = g_wobble;
            if (ang < 0.0)
                ang = ang + 360.0;
            DrawRect(0, 0, g_screenW, g_screenH, 0, 0, 0, 1.0f);
            ImgBlitAlphaRectFx(g_endImg, 0, 0, 800.0f, 600.0f, 0, 0, 0, 1.0f, alpha / 255.0, false, false, 0, 0);

            g_wobble = g_wobble + g_wobbleSpeed;
            if (g_wobble > 1.0 || g_wobble < -1.0)
                g_wobbleSpeed = 0 - g_wobbleSpeed;
            if (g_wobble > -0.01f && g_wobble < 0.01f) {
                if (Rand7f() < 30) {
                    g_wobble = 0;
                    g_wobbleSpeed = 0;
                }
            }
            if (g_wobble == 0.0) {
                if (Rand7f() < 4)
                    g_wobbleSpeed = RandRange(0, 10) / 300.0;
            }

            g_endR = g_endR + g_endDR;
            if (g_endR < 220.0) {
                g_endR = 220.0f;
                g_endDR = 0 - g_endDR;
            }
            if (g_endR > 253.0) {
                g_endR = 253.0f;
                g_endDR = 0 - g_endDR;
            }

            g_endG = g_endG + g_endDG;
            if (g_endG < 220.0) {
                g_endG = 220.0f;
                g_endDG = 0 - g_endDG;
            }
            if (g_endG > 253.0) {
                g_endG = 253.0f;
                g_endDG = 0 - g_endDG;
            }

            g_endB = g_endB + g_endDB;
            if (g_endB < 220.0) {
                g_endB = 220.0f;
                g_endDB = 0 - g_endDB;
            }
            if (g_endB > 253.0) {
                g_endB = 253.0f;
                g_endDB = 0 - g_endDB;
            }
        }

        // ---- fade in ----
        if (!fadeDone && !g_endWobbleActive) {
            DrawRect(0, 0, g_screenW, g_screenH, 0, 0, 0, 1.0f);
            ImgBlitAlphaRectFx(g_endImg, 0, 0, 800.0f, 600.0f, 0, 0, 0, 1.0f, alpha / 255.0, false, false, 0, 0);
            alpha = alpha + alphaSpeed;
            if (alpha > maxAlpha) {
                alpha = maxAlpha;
                if (g_time > nextTime)
                    alphaSpeed = -5.0f;
            }
            if (alpha < 0.0) {
                alpha = 0;
                alphaSpeed = 1.0f;
                fadeDone = true;
            }
        }

        // ---- slideshow advance ----
        if (g_time > nextTime && fadeDone && !g_endWobbleActive) {
            nextTime = g_time + 15000;
            g_endPic++;
            if (g_endPic == 13) {
                g_endPic = 13;
                nextTime = g_time + 86400000;
            }
            ImgFreePicture(g_endImg);
            ImgFree(g_endImg);
            g_endImg = 0;
            CLEAR_DRAW_COUNTERS()

            switch (g_endPic) {
            case 1:
                g_endImg = LoadGraphic("ending_5.jpg", true, true);
                maxAlpha = 220.0f;
                break;
            case 2:
                g_endImg = LoadGraphic("ending_4.jpg", true, true);
                maxAlpha = 200.0f;
                break;
            case 3:
                g_endImg = LoadGraphic("ending_6.jpg", true, true);
                maxAlpha = 200.0f;
                break;
            case 4:
                g_endImg = LoadGraphic("ending_3.jpg", true, true);
                maxAlpha = 200.0f;
                break;
            case 5:
                g_endImg = LoadGraphic("ending_0.jpg", true, true);
                maxAlpha = 200.0f;
                break;
            case 6:
                g_endImg = LoadGraphic("ending_1.jpg", true, true);
                maxAlpha = 200.0f;
                break;

            case 7:
                g_endImg = LoadGraphic("ending_9.jpg", true, true);
                maxAlpha = 200.0f;
                break;
            case 8:
                g_endImg = LoadGraphic("ending_7.jpg", true, true);
                maxAlpha = 160.0f;
                break;
            case 9:
                g_endImg = LoadGraphic("ending_8.jpg", true, true);
                maxAlpha = 150.0f;
                break;
            case 10:
                g_endImg = LoadGraphic("ending_2.jpg", true, true);
                maxAlpha = 150.0f;
                break;

            case 11:
                g_endImg = LoadGraphic("ending_10.jpg", true, true);
                maxAlpha = 170.0f;
                break;
            case 12:
                g_endImg = LoadGraphic("ending_11.jpg", true, true);
                maxAlpha = 170.0f;
                break;
            case 13:
                g_endImg = LoadGraphic("ending_12.jpg", true, true);
                maxAlpha = 50.0f;
                orbits = true;
                break;
            }
            unusedPos = 0;
            alpha = 0;
            fadeDone = false;
        }

        // ---- final-picture flares ----
        if (orbits) {
            size = 500;
            cx = cy;
            DrawImage(g_gfxFlare11, 400 - size, cx - size, 400 + size, cx + size, r, g, b, 0x40, 0, g_angle0);
            size = 700;
            DrawImage(g_gfxFlare16, 400 - (size >> 1), cx - (size >> 1), 400 + (size >> 1), cx + (size >> 1),
                r, g, b, 0x20, 0, g_angle1);
            size = 900;
            DrawImage(g_gfxFlare16, 400 - (size >> 1), cx - (size >> 1), 400 + (size >> 1), cx + (size >> 1),
                r, g, b, 0x10, 0, g_angle2);
            g_angle0 = g_angle0 + 0.1f;
            if (g_angle0 > 360.0)
                g_angle0 = g_angle0 - 360.0;
            g_angle1 = g_angle1 - 0.3f;
            if (g_angle1 < 0.0)
                g_angle1 = g_angle1 + 360.0;
            g_angle2 = g_angle2 + 0.8f;
            if (g_angle2 > 360.0)
                g_angle2 = g_angle2 - 360.0;
        }

        // ---- credits, help and present ----
        DrawBigText(-1, (int)textY, t1);
        if (g_time < helpUntil) {
            DrawMixedCaseText("LEFT MOUSEBUTTON TO PAUSE", 20, 20, 0);
            DrawMixedCaseText("RIGHT MOUSEBUTTON TO SPEED UP", 20, 30, 0);
            DrawMixedCaseText("ESC, SPACE OR FIRE TO CONTINUE", 20, 40, 0);
        }
        g_mouseDown = MouseLeft();
        g_rightButton = MouseRight();
        if (g_rightButton)
            textY = textY - 4.0;
        else if (!g_mouseDown)
            textY = textY - 0.5;
        if (g_textCursorY < 0)
            textY = g_screenH + 10.0;
        FlushBlit(0);
        SysFlip();
        SysProcessEvents();
    } while (!done);
}

// Advances from the post-round screen to the next state once the screen's timeout
// (g_screenDelayMs, measured from g_screenTimerStart) has elapsed.
void PostRoundIdleTimeout()
{
    if ((unsigned int)(g_time - g_screenTimerStart) > (unsigned int)g_screenDelayMs)
        g_state = STATE_PLAYING;
}

// Per-frame driver for the post-game screen sequence (bonus tally -> game-over/versus-result
// screen -> hiscore check -> name entry, or back to the title screen). Advances g_tallyStep on
// SPACE/FIRE or on screen timeout, tallies bonus into the score once all tally steps are done,
// and on the last step decides whether any player made a hiscore table (CheckHiscore) and
// either starts name entry (EnterHiscore) or resets to the title/intro.
void UpdateGameOverSequence()
{
    int flags;
    int unused;
    int sel;
    int i;

    // ---- screenshot ----
    if (KeyDown(K_VK_F7) == true) {
        if (g_screenshotKeyEdge != 0) {
            TakeScreenshot();
            g_screenshotKeyEdge = 0;
        }
    } else {
        g_screenshotKeyEdge = 1;
    }

    // ---- advance tally ----
    if (g_gameMode == MODE_TIME_TRIAL && g_tallyStep == 0) {
    } else {
        if (KeyDown(K_VK_SPACE) == true && g_transitionLock == 0) {
            if (g_save.players[0].keyLatchFire != 0) {
                do {
                    switch (g_gameMode) {
                    case MODE_SINGLE:
                        g_tallyState = TallyStep(1);
                        break;
                    case MODE_TWO_PLAYER:
                        g_tallyState = TallyStep(2);
                        break;
                    case MODE_DUAL:
                        g_tallyState = TallyStep(2);
                        break;
                    case MODE_TEAM:
                        g_tallyState = TallyStep(2);
                        break;

                    case MODE_UNUSED_4:
                        break;

                    case MODE_TIME_TRIAL:
                        g_tallyState = TallyStep(1);
                        break;
                    }
                } while (g_tallyState != 0);
                g_advanceScreenFlag = 1;
                g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                g_transitionLock = 1;
                g_save.players[0].keyLatchFire = 0;
            }
        } else {
            g_save.players[0].keyLatchFire = 1;
        }
    }

    // ---- tally done ----
    if (g_tallyDonePending != 0) {
        g_hiscoreTransitionFlag = 0;
        g_attractScreen = ATTRACT_HALL_OF_FAME;
        g_idleTimeoutMs = 5000;
        g_lastActivityTime = g_time - 10000;
        g_hiscoreInsertGate = 0;
        g_hofMode = g_cfg.difficulty;
        if (g_gameMode == MODE_TIME_TRIAL)
            g_hofMode = HOF_TIME_TRIAL;

        flags = 0;
        if (!g_autoplay) {
            switch (g_gameMode) {
            case MODE_SINGLE:
                flags |= CheckHiscore(0);
                break;

            case MODE_TWO_PLAYER:
                flags |= CheckHiscore(1);
                flags |= CheckHiscore(0);
                break;

            case MODE_DUAL:
                flags |= CheckHiscore(1);
                flags |= CheckHiscore(0);
                break;

            case MODE_TEAM:
                flags |= CheckHiscore(1);
                flags |= CheckHiscore(0);
                break;

            case MODE_UNUSED_4:
                break;

            case MODE_ACE_TOURNAMENT:
                break;

            case MODE_TIME_TRIAL:
                flags |= CheckHiscore(0);
                break;
            }
        }

        if (flags != 0 && g_pendingGameMode == -1) {
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo)
                UpdateHighScore(g_profileIndex, g_save.players[g_curPlayer].score);

            DecompressHiscores();
            unused = 0;
            ClearMoneyHiscoreHighlight(0);
            ClearMoneyHiscoreHighlight(1);
            ClearMoneyHiscoreHighlight(2);
            ClearMoneyHiscoreHighlight(3);
            if (g_gameMode == MODE_TIME_TRIAL) {
                ClearHiscoreHighlightTT(0);
            } else {
                ClearHiscoreHighlight(0);
                ClearHiscoreHighlight(1);
                ClearHiscoreHighlight(2);
                ClearHiscoreHighlight(3);
            }
            CompressHiscores();

            // ---- hiscore selection ----
            sel = -1;
            if (g_hsPlayer[0] != -1 && sel == -1) {
                g_curPlayer = g_hsPlayer[0];
                sel = g_hsPlayer[0];
            }
            if (g_hsPlayer[1] != -1 && sel == -1) {
                g_curPlayer = g_hsPlayer[1];
                sel = g_hsPlayer[1];
            }
            if (g_hsPlayer[2] != -1 && sel == -1) {
                g_curPlayer = g_hsPlayer[2];
                sel = g_hsPlayer[2];
            }
            if (g_hsPlayer[3] != -1 && sel == -1) {
                g_curPlayer = g_hsPlayer[3];
                sel = g_hsPlayer[3];
            }

            if (sel == -1) {
                if (g_hsPlayer2[0] != -1 && sel == -1) {
                    g_curPlayer = g_hsPlayer2[0];
                    sel = g_hsPlayer2[0];
                }
                if (g_hsPlayer2[1] != -1 && sel == -1) {
                    g_curPlayer = g_hsPlayer2[1];
                    sel = g_hsPlayer2[1];
                }
                if (g_hsPlayer2[2] != -1 && sel == -1) {
                    g_curPlayer = g_hsPlayer2[2];
                    sel = g_hsPlayer2[2];
                }
                if (g_hsPlayer2[3] != -1 && sel == -1) {
                    g_curPlayer = g_hsPlayer2[3];
                    sel = g_hsPlayer2[3];
                }
            }

            for (i = 0; i < NAME_LEN; i++)
                g_name[i] = '_';
            g_name[NAME_LEN] = 0;
            g_state = STATE_ENTER_HISCORE;
            g_frameFunc = EnterHiscore;
            CLEAR_DRAW_COUNTERS()

            // ---- stat awards ----
            if (g_profileIndex != -1 && (g_gameMode == MODE_SINGLE || g_gameMode == MODE_TIME_TRIAL) &&
                g_playerUpdateFn != StateDemo) {
                if (!g_profilePlayTimeAdded)
                    AddPlayTime(g_profileIndex, g_timeStampB, g_timeStampA,
                        g_pausedDuration);
                if (g_gameMode == MODE_SINGLE)
                    UpdateHighScore(g_profileIndex, g_save.players[g_curPlayer].score);
                UpdateMeteorStormScore(g_profileIndex, g_save.players[g_curPlayer].bonusHighScore);
                if (g_gameMode == MODE_TIME_TRIAL)
                    UpdateTimeTrialScore(g_profileIndex, g_save.players[g_curPlayer].score);
                AddStats(g_profileIndex, g_perfectCount, (&g_perfectCount)[1]);
                CheckRatioMedal(g_profileIndex);
                if (g_gameMode == MODE_SINGLE)
                    UpdateBestTime(g_profileIndex, g_timerMin1);
                if (g_gameMode == MODE_SINGLE)
                    UpdateFastestMeteorStorm(g_profileIndex, g_timerMin2);

                AddScoreStat(g_profileIndex, g_sessionScore);
                g_sessionScore = 0;
                AddHitsStat(g_profileIndex, g_hits);
                g_hits = 0;

                if (g_gameMode == MODE_SINGLE) {
                    UpdateHighestLevel(g_profileIndex, g_save.players[g_curPlayer].level);
                    AddLevelsPlayed(g_profileIndex, g_pendingLevelsPlayed);
                    g_pendingLevelsPlayed = 0;
                    if (g_save.players[g_curPlayer].level > 1)
                        IncrementGamesPlayed(g_profileIndex);
                    if (g_save.players[g_curPlayer].level > 25)
                        UpdateHitPctAbove25(g_profileIndex,
                            (int)((double)g_save.players[g_curPlayer].hits /
                                g_save.players[g_curPlayer].shots * 100.0));
                    UpdateHighestRank(g_profileIndex, g_save.players[g_curPlayer].rank);
                }

                UpdateHighestMoney(g_profileIndex, g_moneyMax);
            }

        } else {
            g_state = STATE_TITLE;
            ClearPlayers();
            g_introInit = 1;
            DoNothing();
            g_frameFunc = IntroFrame;
            ClipCursorOn();
            CLEAR_DRAW_COUNTERS()
            InitStarRotation();
            g_titleResetPending = 1;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
        }

        ClearHiscores();
        return;
    }

    // ---- advance screen ----
    if (g_advanceScreenFlag != 0) {
        g_advanceScreenFlag = 0;
        g_titleResetPending = 0;
        InitStarRotation();
        g_screenTimerStart = g_time;
        InitLayers();
        g_tallyStep++;
        if (g_tallyStep > 2) {
            switch (g_gameMode) {
            case MODE_SINGLE:
                g_save.players[0].score += g_bonusTally[0].total;
                break;
            case MODE_TWO_PLAYER:
                g_save.players[0].score += g_bonusTally[0].total;
                g_save.players[1].score += g_p2Bonus;
                break;

            case MODE_DUAL:
                g_save.players[0].score += g_bonusTally[0].total;
                g_save.players[1].score += g_p2Bonus;
                break;

            case MODE_TEAM:
                g_save.players[0].score += g_bonusTally[0].total;
                g_save.players[1].score += g_p2Bonus;
                break;

            case MODE_UNUSED_4:
                break;

            case MODE_ACE_TOURNAMENT:
                break;

            case MODE_TIME_TRIAL:
                g_save.players[0].score += g_bonusTally[0].total;
                break;
            }

            g_tallyStep = 1;
            g_tallyDonePending = 1;
            g_frameFunc = TallyScreen;
            CLEAR_DRAW_COUNTERS()
            return;
        }
        switch (g_tallyStep) {
        case 1:
            g_screenDelayMs = 5000;
            return;
        case 2:
            g_screenDelayMs = 5000;
            return;
        }
    }

    // ---- timeout and dispatch ----
    if ((unsigned int)(g_time - g_screenTimerStart) > (unsigned int)g_screenDelayMs) {
        g_advanceScreenFlag = 1;
        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
        g_transitionLock = 1;
    }
    CLEAR_DRAW_COUNTERS()

    switch (g_tallyStep) {
    case 0:
        return;

    case 1:
        ResetBlitCounters();
        g_frameFunc = BonusScreen;
        break;

    case 2:
        ResetBlitCounters();
        if (g_gameMode == MODE_DUAL)
            g_frameFunc = VersusResult;
        else
            g_frameFunc = GameOverScreen;
        break;
    }
}
