// shop.c: The shop between levels, its pictures and sounds, sparkle flashes, the profile bonus.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// g_shopItems unlock gates: the run's shown stat (or "special rank" bonus flag) raises the
// item-list cap as it climbs, unlocking a few more (pricier) items each tier.
enum {
    SHOP_ITEMS_BASE       = 0x42,
    SHOP_ITEMS_TIER70     = 0x54,
    SHOP_ITEMS_TIER80     = 0x55,
    SHOP_ITEMS_TIER90     = 0x56,
    SHOP_ITEMS_BONUS_RANK = 0x57,
};

// Shop item prices, in shop-item order (index 0 = the exit row, always free).
static const int g_prices[25] = {
    0, 50, 75, 100, 150, 200, 300, 400, 500, 600, 750, 800, 990, 1000, 1250, 1500, 2000, 3000,
    5000, 15000, 30000, 500000, 0, 0, 0
};

// Score bonus for the all-marks-collected / rank-promotion milestones below.
enum { RANK_UP_SCORE_BONUS = 1000000 };
// Score bonus for reaching or exceeding RANK_GOD.
static const __int64 GOD_RANK_SCORE_BONUS = 50000000;


// Allocates a sparkle-flash in the first free slot of g_sparkleFlashes (10 max): a single fading/growing
// glow sprite, distinct from the g_sparks particle system.
void AddSparkleFlash(Image *graphic, int x, int y, int size, int r, int g, int b,
                     int alpha, int fade)
{
    for (int i = 0; i < MAX_SPARKLE_FLASHES; i++) {
        if (g_sparkleFlashes[i].active == 0) {
            g_sparkleFlashes[i].active = 1;
            g_sparkleFlashes[i].graphic = graphic;
            g_sparkleFlashes[i].x = x;
            g_sparkleFlashes[i].y = y;
            g_sparkleFlashes[i].size = size;
            g_sparkleFlashes[i].r = r;
            g_sparkleFlashes[i].g = g;
            g_sparkleFlashes[i].b = b;
            g_sparkleFlashes[i].alpha = (float)alpha;
            g_sparkleFlashes[i].fade = (float)fade;
            break;
        }
    }
}

// Per-frame update and draw for every active sparkle-flash: draws it rotated/stretched to its bounding
// box, fades its alpha and deactivates once it reaches zero.
void UpdateSparkleFlashes()
{
    for (int i = 0; i < MAX_SPARKLE_FLASHES; i++) {
        if (g_sparkleFlashes[i].active != 0) {
            int a = (int)g_sparkleFlashes[i].alpha;
            if (a > 255)
                a = 255;
            QueueStretchRot(g_sparkleFlashes[i].graphic,
                                   (float)(g_sparkleFlashes[i].x - (g_sparkleFlashes[i].size >> 1)) + g_offX,
                                   (float)(g_sparkleFlashes[i].y - (g_sparkleFlashes[i].size >> 1)) + g_offY,
                                   (float)(g_sparkleFlashes[i].x + (g_sparkleFlashes[i].size >> 1)) + g_offX,
                                   (float)(g_sparkleFlashes[i].y + (g_sparkleFlashes[i].size >> 1)) + g_offY,
                                   g_sparkleFlashes[i].r, g_sparkleFlashes[i].g, g_sparkleFlashes[i].b,
                                   a, 0, 1.0f);
            g_sparkleFlashes[i].alpha -= (double)g_sparkleFlashes[i].fade;
            if (g_sparkleFlashes[i].alpha < 0.0)
                g_sparkleFlashes[i].active = 0;
        }
    }
}

#ifdef __EMSCRIPTEN__
// Whether this shop visit's game has been saved (the browser build, AutoSaveShop).
static bool g_shopAutoSaved;

// The browser build has no F1/F2 (a tab is closed, not quit): the profile's game is saved once
// per shop visit instead, as soon as the shop is fully open. A visit starts when the shop is
// drawn again after a gap (it is drawn every frame while open, paused or not).
static void AutoSaveShop()
{
    static unsigned lastFrame;

    if (g_time - lastFrame > 1000)
        g_shopAutoSaved = false;
    lastFrame = g_time;
    if (!g_shopAutoSaved && (int)g_shopTransition == 500 && g_profileIndex != -1 &&
        g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo && !g_autoplay) {
        AutoSaveProfile(g_profileIndex);
        g_shopAutoSaved = true;
    }
}
#endif

// Draws the blinking "press F1/F2 to save" pause-menu prompt, alternating between the two save options
// each blink and slowing the blink rate to 4x while the alternate message is shown.
void DrawSavePrompt()
{
    if (g_time - g_uiBlinkTime > g_saveMsgBlinkRate) {
        g_uiBlinkTime = g_time;
        g_uiBlink = g_uiBlink == 0;
        if (g_uiBlink != 0)
            g_saveMsgToggle = !g_saveMsgToggle;
        if (g_uiBlink != 0)
            g_saveMsgBlinkRate = g_blinkRate * 4;
        else
            g_saveMsgBlinkRate = g_blinkRate;
    }
    if (g_uiBlink != 0) {
#ifdef __EMSCRIPTEN__
        // The browser build saves by itself at each shop visit (AutoSaveShop).
        if (!g_shopAutoSaved)
            return;
        sprintf(g_logBuf, "GAME SAVED: CONTINUE FROM YOUR PROFILE NEXT TIME");
#else
        if (g_saveMsgToggle)
            sprintf(g_logBuf, "PRESS F1 TO SAVE GAME AND EXIT TO WINDOWS      %d SAVES LEFT", g_lives);
        else
            sprintf(g_logBuf, "PRESS F2 TO SAVE GAME AND EXIT TO MENUSCREEN   %d SAVES LEFT", g_lives);
#endif
        DrawMixedCaseText(g_logBuf, (int)g_offX + 0x82, (int)(g_offY + g_shopBounceY) + 0x216, 1);
    }
}

// Updates the active save profile's bonus/extra-life state for the shop screen: sets
// g_bonusFlag when the profile has rank 1 or 2 with all medals, grants a free life every
// 250 levels if the player is broke, and refreshes the cached life count/flag. Only applies
// to a real saved profile in classic mode outside the demo (g_profileIndex/g_gameMode/
// g_playerUpdateFn checks). Always returns 1.
int CheckProfileBonus()
{
    g_bonusFlag = 0;
    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
        if (GetRank(g_profileIndex) == 1) {
            if (HasAllMedals(g_profileIndex))
                g_bonusFlag = 1;
        }
        if (GetRank(g_profileIndex) == 2) {
            if (HasAllMedals(g_profileIndex))
                g_bonusFlag = 1;
        }
    }

    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
        // Every 250 levels, grant one extra life if the player has no money; the granted
        // flag latches until the level counter moves off a multiple of 250 again.
        if ((g_save.players[g_shopCurPlayer].level + 1) % 250 == 0 && g_playerBroke && !g_extraLifeGranted) {
            IncProfileLives(g_profileIndex);
            g_extraLifeGranted = 1;
            SoundPlay(g_sfxFanfare, -1, 0xff, 0.0f, 0xdf, g_sndFlags);
        }
        if ((g_save.players[g_shopCurPlayer].level + 1) % 250 != 0)
            g_extraLifeGranted = 0;
    }

    g_hasLives = 0;
    g_lives = GetProfileLives(g_profileIndex);
    if (g_lives > 0)
        g_hasLives = 1;
    return 1;
}

// Frees graphic `g` (if loaded) and nulls it out; same shape as FREE_NULL in init.c and
// FREE_SAMPLE in sound.c. #undef'd after ReloadSecret below.
#define FREE_GFX(g)          \
    if (g != 0) {            \
        ImgFreePicture(g);   \
        ImgFree(g);          \
        g = 0;                \
    }

// Frees the shop background picture and resets the frame's draw-call counters.
void FreeShopBgGfx()
{
    FREE_GFX(g_gfxShopBg)
    CLEAR_DRAW_COUNTERS()
}

// Frees the shop item preview picture and resets the frame's draw-call counters.
void FreeShopItemGfx()
{
    FREE_GFX(g_gfxShopItemPic)
    CLEAR_DRAW_COUNTERS()
}

// Loads and shows the shop item picture for item `idx` (index into g_shopPics), replacing
// whatever was loaded before. idx == -1 means "no selection" and leaves the current picture.
void LoadShopPic(int idx)
{
    if (idx == -1)
        return;
    FREE_GFX(g_gfxShopItemPic)
    g_gfxShopItemPic = LoadGraphic(g_shopPics[idx], 1, 0);
    CLEAR_DRAW_COUNTERS()
    g_shopPic = idx;
}

// Frees the "secret unlocked" picture and resets the frame's draw-call counters.
void FreeSecretPicGfx()
{
    FREE_GFX(g_gfxSecretPic)
    CLEAR_DRAW_COUNTERS()
}

// Loads and shows the secret picture for index `idx` (into g_secretPics), replacing whatever
// was loaded before. Always returns 1.
int LoadSecretPic(int idx)
{
    FREE_GFX(g_gfxSecretPic)
    g_gfxSecretPic = LoadGraphic(g_secretPics[idx], 1, 0);
    CLEAR_DRAW_COUNTERS()
    g_secretPic = idx;
    return 1;
}

// Frees the secret background screen picture.
void FreeSecretScreenGfx()
{
    if (g_gfxSecretScreen != 0) {
        // NOTE: unlike the other Free*Gfx helpers here, this skips ImgFreePicture() before
        // ImgFree(); kept from the original.
        ImgFree(g_gfxSecretScreen);
        g_gfxSecretScreen = 0;
    }
}

// Frees and reloads the "secret screen" graphic, then resets the frame draw-call counters.
int ReloadSecret()
{
    FREE_GFX(g_gfxSecretScreen)
    g_gfxSecretScreen = LoadGraphic("secretscreen_new.tga", true, true);
    CLEAR_DRAW_COUNTERS()
    return 1;
}

#undef FREE_GFX

// Bumps the current shop player's visit count and plays the cash register sound.
void AddCash()
{
    g_save.players[g_shopCurPlayer].shopVisits = g_save.players[g_shopCurPlayer].shopVisits + 1;
    SoundPlay(g_sfxCash, -1, 100, 0.0f, 0xdf, g_sndFlags);
}

// Plays the "buzzer" (invalid action / denial) sound effect.
void PlayBuzzerSfx()
{
    SoundPlay(g_sfxBuzzer, -1, 200, 0.0f, 0xdf, g_sndFlags);
}

// Plays the alternate "buzzer" sound effect.
void PlayBuzzer2Sfx()
{
    SoundPlay(g_sfxBuzzer2, -1, 200, 0.0f, 0xdf, g_sndFlags);
}

// Plays the UI "slide" sound effect.
void PlaySlideSfx()
{
    SoundPlay(g_sfxSlide, 20000, 200, 0.0f, 0xdf, g_sndFlags);
}

// Deducts the price of the currently selected shop item from the current shop player.
#define PAY P.money = P.money - g_prices[g_shopSelItem]

// Fixed size of the secret-screen panel graphic.
enum { SECRET_PANEL_W = 0x242, SECRET_PANEL_H = 0x1b1 };

// P: the player currently shopping. SX/SY: the shop panel's current slide offset, as
// ints. BX/BY: the secret-screen panel's centred top-left corner.
#define P g_save.players[g_shopCurPlayer]

#define SX ((int)g_offX)

#define SY ((int)g_offY)

#define BX ((g_screenW - SECRET_PANEL_W) / 2)

#define BY ((g_screenH - SECRET_PANEL_H) / 2)

// Queues one drifting sparkle flare (a glint effect) around the secret-screen panel at
// panel-relative (xo, yo), sized randomly in [zlo, zhi] and slid with the panel transition.
#define FX(zlo, zhi, xo, yo)                                                                       \
    ImgBlitAlphaRectFx(g_gfxFlare5,                                                                 \
        0, 0, ImgWidth(g_gfxFlare5), ImgHeight(g_gfxFlare5),                                        \
        (short)(0 - g_shopTransition + xo - ImgWidth(g_gfxFlare5) / 2.0),                           \
        (short)(yo - ImgHeight(g_gfxFlare5) / 2.0), RandFloat(0, 360),                              \
        RandFloat(zlo, zhi), 1, false, false, 0, 0)

// Adds `add` to the current shop player's score.
#define SCOREADD(add) ADD_PLAYER_SCORE(P.score, g_shopCurPlayer, add)

// Leaves the shop and starts the next level.
#define LEAVESHOP            \
    g_buttonsOn = 0;            \
    HidePointer();    \
    g_state = STATE_PLAYING;            \
    StartNextLevel();             \
    EmptyViewChangeHook()

// In 2-player vs. mode, hands the shop to player 2 (with a fresh slide-in animation) if
// they can still afford to shop and have lives left; otherwise (or outside vs. mode)
// leaves the shop via LEAVESHOP.
#define NEXTPLAYER                                                                                  \
    if (g_gameMode == MODE_DUAL) {                                                                   \
        if (g_shopCurPlayer == 0) {                                                                        \
            if (g_save.players[1].money >= SHOP_MIN_MONEY &&                                            \
                g_save.players[1].lives > g_shipDefs[g_save.players[1].ship]->minEnergy) {  \
                g_shopCurPlayer = 1;                                                                       \
                g_shopSelItem = 0;                                                                       \
                g_offX = 0;                                                                       \
                g_shopSlideVelX = 0;                                                                       \
                g_offY = -600;                                                                    \
                g_shopSlideVelY = 40;                                                                      \
                g_shopBounceY = -30;                                                                     \
                CheckProfileBonus();                                                                        \
                PlayShopMusic();                                                                        \
                return;                                                                             \
            } else {                                                                                \
                LEAVESHOP;                                                                          \
            }                                                                                       \
        } else {                                                                                    \
            LEAVESHOP;                                                                              \
        }                                                                                           \
    } else {                                                                                        \
        LEAVESHOP;                                                                                  \
    }

// Grants mark `bit` if the player doesn't have it yet and no other mark was already granted
// this purchase. Not used for the first mark checked (MARK_6): there `gotMark` is always
// still 0, so the original omits that redundant check.
#define TRY_GRANT_MARK(bit)                       \
    if (!((short)P.marks & (bit)) && gotMark == 0) { \
        P.marks = (short)P.marks | (bit);            \
        gotMark = 1;                                  \
    }

// Buys weapon `w` if not already equipped, otherwise plays the "already own this" buzzer.
#define BUY_WEAPON(w)            \
    if (P.weapon != (w)) {          \
        P.weapon = (w);              \
        AddCash();                   \
        PAY;                         \
    } else                          \
        PlayBuzzer2Sfx();

// Closes the shop (starting the slide-out) once the current player can no longer afford it.
#define CHECK_BROKE()            \
    if (P.money < SHOP_MIN_MONEY) { \
        g_playerBroke = 1;          \
        g_shopClosing = 1;          \
        g_shopSlideVelX = 0;        \
        g_shopSlideVelY = 4;        \
        PlaySlideSfx();              \
    }

// The between-levels shop screen: called every frame while the shop is open. Slides the
// shop panel in/out, draws the item list, price-gated by the current player's money, and
// the selected item's picture/description; handles keyboard/mouse input to move the
// selection and buy the highlighted item (each of the ~22 items has its own effect,
// switched on `g_shopSelItem - 1`); draws and drives the secret-screen overlay bought via
// the "GAME SECRET" item; and, once the panel finishes sliding shut, awards any pending
// rank promotion or hands off to the next player (2-player vs. mode) before starting the
// next level.
void Shop()
{
    // ---- setup and text tables ----
    // NOTE: always 0 in the original (never assigned elsewhere), so the g_buttonsOn = 1
    // below it never runs; kept for the byte match.
    int buttonsOnRequest = 0;
    bool gotUniquePick = 0; // secret-picture pick loop: true once a usable pick is found
    int pickAttempts = 0;   // secret-picture pick loop: give up and accept any pick past 250 tries
    SysDate sy;
    int monthDayBase; // (month-1)*30, used only to feed dayOfYear
    int dayOfYear;    // NOTE: computed but never printed/used further
    int hour;
    int minute;
    int second;

    int gotMark; // true once any single rank-marker mark was newly set this purchase
    int x;
    int j;
    int i;
    float y;

    char str[25];       // scratch copy of one varer[] row for DrawMenuText (needs a plain char*)

    // The shop's scrolling item list, one price-suffixed row per item (index 0 = exit).
    char varer[25][25] = {
        "      EXIT SHOP      \0",
        "EXTRA SPEED        50\0",
        "EXTRA BULLET       75\0",
        "DOUBLE SHOT       100\0",
        "LESS SPEED        150\0",
        "TRIPLE SHOT       200\0",
        "QUAD SHOT         300\0",
        "AUTO FIRE UNIT    400\0",
        "SUPER TRIPLE      500\0",
        "SHIP ARMOUR       600\0",
        "PLASMA            750\0",
        "EXTRA LIFE        800\0",
        "FIRE BALLS        990\0",
        "GAME SECRET      1000\0",
        "RANK MARKER      1250\0",
        "EXTRA TIME       1500\0",
        "LASER BEAM       2000\0",
        "WAR.I.PLASMA     3000\0",
        "ROCKET PACK      5000\0",
        "ALIEN LOCK      15000\0",
        "SUPER AUTOFIRE  30000\0",
        "CLEAR SHIELDS  500000\0",
    };

    // T1..T23: the full flavour-text description shown for each item (and the exit
    // prompt), indexed indirectly through tekster[] below; T18 is the plain exit prompt,
    // T23 the demo-version exit prompt.

    char T23[] = "E X I T   T H E   S H O P||DEMO VERSION: All items is not available in this demo version|\0";
    char T22[] = "C L E A R   S H I E L D S :||Wipe all your shields for a big bonus score and go for them |again. I can always sell the metal as scrap for now and |supply the new ones when you need them.\0";
    char T21[] = "S U P E R   A U T O F I R E :||This enhanced autofire unit works just like the cheaper unit |but can also increase your rate of fire dramatically. |Any fast firing weapon will become even more powerful with one|of these strapped to it. If you can max out your bullets and |get one of the pricier weapons you will become an almost |unstoppable force.\0";
    char T20[] = "A L I E N   L O C K :||This powerful device can hold captured aliens firmly to your |ship, even through a turbulent warp sequence. A combination of|tractor beam and armour make this unit extremely handy for the |fight ahead. We still don't fully understand this technology and |they cost a fortune to buy in from the Cralti's. |So they aren't gonna be cheap !\0";
    char T19[] = "R O C K E T   P A C K :||Buy 10 fast homing missiles. Each one of these beauties will|destroy whatever it hits although the bosses may be able to|absorb a few of them. |Anything else will be turned to dust though as these are a |very powerful addition to your arsenal. ||Theres 10 in each pack and your ship can carry 50 in total.||The sticker on these says 'Qui Yen Min Sco'. Roughly translated|it means BOOOOOM 'And dead you are'\0\0";
    char T1[] = "W A R . I . P L A S M A :||The most awesome weapon to come out of the big R6 plant on|the remote Eurtix 4. Developed earlier this year it is by far the |most destructive weapon we know of. ||3 high performance plasma emitters are fired through 3 XDFc |lenses and 3 plasma amplifiers.||How it all fits in there i'll never know. ||The 3 emitters can deliver 259.002gw of plasma destruction EACH!!|Very destructive on all lifeforms with a fast rate and perfect spread.|Be feared !!||Rumour has it that it may have been invented by|the mystic Mr. Vida Galdeg.\0";
    char T2[] = "L A S E R   B E A M :||Now we're talking. Hows about this high quality zeta laser. |Up to 46mw output with pinpoint accuracy. Almost unbeatable firing |rate and high pressure impact, combined with kelvin disruptors, ensures |this weapon works like a hot knife through butter. |Your enemies WILL fear you with this weapon, but can you afford it ?||Invented by the now legendary Max And Peeluc duo of the Stohg system\0";
    char T3[] = "E X T R A   T I M E :||This is another ship addon and is pretty complicated to fit which is |why its pricey. We can hook this into many of the ships systems so |that any powerups you might gain while fighting will last you a |bit longer. You can buy more than one, if you can afford it, to give |you even more time. ||Shields and score multipliers work great with these addons.||This one was recently developed by the Trama and Krebake company.\0";
    char T4[] = "R A N K   M A R K E R :||As you battle the enemy you may come across these colourful glass balls.|Well, if you grab all 6, then i'll give you a new rank. |If you only have 5 then i can sell you what you need if no |questions are asked.\0";
    char T5[] = "G A M E   S E C R E T:||I'll let you into a little secret for a fee. |It could be about your foes or your profile, but whatever it is, you |need to learn these beauties. If you really wanna beat those aliens at|their own game, then take a chance and buy one of my random secrets.\0";
    char T6[] = "F I R E   B A L L S :||This weapon is superb. It goes back to old school weaponry but this |does it with added style. This can produce and shoot real fireballs and|with extra upgrades its almost unstoppable. ||This weapon also uses the plasma's XDFa lens but 'Linda Elano' found|a way to modify them to give this weapons unique spread of fire.||One of our best weapons on offer.\0";
    char T7[] = "E X T R A   L I F E :||Want to buy an extra ship in case you get wiped out ? |You don't want to be running out now do you or the battle will be over?|Well these are relatively cheap considering the items that are |sometimes fitted so buy em when you can ! ||Your hangar will hold 4 of these AR-65 ships\0";
    char T8[] = "P L A S M A :||This was the first of a new breed of weapons to come from our friends|on Guldax Gaelea 2.6c. No longer explosive based we step up a gear |into the realms of plasma (of course the price goes up too) ||This uses medium strength plasma emitters with 1 small XDFa crystal |lens to control the spread. ||Fast fire, very powerful and can tame some of the hardest enemies.\0";
    char T9[] = "S H I P   A R M O U R :||Your fighter can carry up to 2 of these devices but each one can only |be used once. When you've installed one its frion sensory trackers can |detect an impending hit and activate the unit. It saves your ship and |will give you a short term shield. ||Once the shield is activated the unit is destroyed.\0";
    char T10[] = "S U P E R   T R I P L E :||Our corporation further developed the standard triple shot into a more|powerful weapon about 30 years ago. Its still a triple and its still|got a wide spread, but boy do those bullets pack a powerful punch now !|Each bullet now carries 5x the power of the standard weapons, which|is why we couldn't lessen the spread at all.|From Sir Venuslib Tritech Corporation.\0\0";
    char T11[] = "A U T O   F I R E   U N I T :||This addon unit can fire your weapon automatically for you. |It will still pause when your weapon needs to reload, but buy enough|extra ammo and your rain of bullets could be awesome. |This makes great use of the pulse modulator technology found on Tellus9.\0";
    char T12[] = "Q U A D   S H O T :||About 50 years after the Triple Shot comes the Quad Shot from the |Talfi's. They gave us the technology after we liberated them from their|ruthless leader t'Gis II. ||This fires 4 bullets in a tight spread and is more powerful than the|Triple Shot. A bargain buy if you can afford enough extra ammo.\0";
    char T13[] = "T R I P L E   S H O T :||Soon after the development of the Double Shot, those Drelfs managed|to get a triple barrel working. This can fire 3 regular bullets but|unfortunately the spread is rather wide. |Still, its 1 more bullet to be blasting with.\0";
    char T14[] = "L E S S   S P E E D :||Ha ha. Can't you handle a fast fighter ? Ok, ok, our engineers can |slow down your lateral thrusters for you, but its gonna cost you extra !\0";
    char T15[] = "D O U B L E   S H O T :||The Double Shot weapon was developed almost 100 years ago now, by the |Drelfs on Froel7. Its obviously old technology now, but its far better|than your ships standard single shot, and its cheap !\0";
    char T16[] = "E X T R A   B U L L E T :||Your weapon can only load so fast, but we can improve the reload|speed on your gun and supply you with extra ammo. ||Buy your bullets here at bargain prices.\0";
    char T17[] = "E X T R A   S P E E D :||Our engineers can make your ships lateral movement faster for a |measly 50 credits. Pay us more and we'll improve it further. |Good or bad speed can make the difference between life or death.\0";
    char T18[] = "E X I T   T H E   S H O P\0";
    char *tekster[] = {T18, T17, T16, T15, T14, T13, T12, T11, T10, T9, T8, T7, T6, T5, T4, T3, T2, T1,
                       T19, T20, T21, T22, T23};

    int nitems;        // last valid item index (g_shopItems - SHOP_ITEMS_BASE): more unlock as the run's stat rises
    int shake;
    int k;
    float step;         // vertical spacing between item rows (380 / (nitems + 1))
    int extraLineCount; // extra stat lines drawn below the weapon/score block (alien lock adds one)
    float hoverStep;    // same formula as `step`, recomputed for hit-testing the mouse against rows
    int hoverItem;      // item index under the mouse, from hoverStep
    unsigned int goodbyeWaitUntil; // tick to stop pumping AudioUpdate() while the goodbye sample plays
    int secretIdx;      // loop index when clearing all levels' secretFlags (CLEAR SHIELDS)

    ResetPlayerTimers();
    FormatHitPctAbove25(g_profileIndex);

    if (g_gameMode != MODE_TIME_TRIAL) {
        if (g_bonusFlag > 0)
            g_shopItems = SHOP_ITEMS_BONUS_RANK;
        else {
            if (g_shownStat >= 70) g_shopItems = SHOP_ITEMS_TIER70;
            if (g_shownStat >= 80) g_shopItems = SHOP_ITEMS_TIER80;
            if (g_shownStat >= 90) g_shopItems = SHOP_ITEMS_TIER90;
        }
    }
    if (g_autoplay && g_shopItems < SHOP_ITEMS_TIER90)
        g_shopItems = SHOP_ITEMS_TIER90;
    nitems = g_shopItems - SHOP_ITEMS_BASE;
    if (buttonsOnRequest)
        g_buttonsOn = 1;
    g_moneyBlinkTimer = 0;

    // ---- panel slide/shake ----
    Frame();
    DrawParticles();
    FlushBlit(0);
    FlushStretchF();
    FlushStretchRot();
    FlushStretchI();
    FlushStretchRot2();
    FlushBlit2(0);
    g_clipLeft = 0;
    g_clipRight = g_screenW;
    EmptyViewChangeHook();

    if (g_offX >= 799.0 && g_shopClosing == 0)
        PlaySlideSfx();
    if (g_offX > 0.0 && g_shopSlideVelX > 0.0) {
        g_offX -= g_shopSlideVelX;
        if (g_offX < 0.0) g_offX = 0;
        g_shopSlideVelX *= 0.95f;
        if (g_shopSlideVelX < 1.0) g_shopSlideVelX = 1;
    }
    if (g_offX < 800.0 && g_shopClosing != 0) {
        g_offX -= g_shopSlideVelX;
        if (g_offX > 800.0) g_offX = 800;
        g_shopSlideVelX *= 1.03f;
    }
    if (g_offY <= -600.0 && g_shopClosing == 0)
        PlaySlideSfx();

    if (g_offY < 0.0 && g_shopSlideVelY > 0.0 && g_shopClosing == 0) {
        g_offY += g_shopSlideVelY;
        if (g_offY > 0.0) {
            g_offY = 0;
            g_shopSlideVelY = 0;
        } else {
            g_shopSlideVelY *= 0.95f;
            if (g_shopSlideVelY < 1.0) g_shopSlideVelY = 1;
        }
    }
    if (g_offY > -600.0 && g_shopClosing != 0) {
        g_offY -= g_shopSlideVelY;
        if (g_offY < -600.0) g_offY = -600;
        g_shopSlideVelY *= 1.05f;
    }
    if (g_offY == 0.0 && g_shopBounceY < 0.0 && g_hasLives) {
        g_shopBounceY += 0.4f;
        if (g_shopBounceY > 0.0) g_shopBounceY = 0;
    }

    if (g_profileIndex != -1) {
        DrawSavePrompt();
        if (g_gfxShopBg)
            Blit((int)g_offX, (int)(g_offY + g_shopBounceY) + 0x20b, 0, g_gfxShopBg,
                        0, 0x20b, 800 - (int)g_offX, 0x16);
        // Occasional "shop keeper is moving" tremor: randomly kicks off a shake, then
        // redraws the two counter-flap strips with a decaying shake offset each frame.
        if (RandRange(0, 100) < 4 && g_shopBounceY >= 0.0)
            g_shopShake = (float)RandRange(0, 15);
        shake = (int)g_shopShake;
        if (shake > 0 && g_gfxShopBg) {
            for (k = 0; k < 10; k++) {
                Blit((int)g_offX + k * 16 + 0x75, (int)(g_offY + g_shopBounceY) + 0x20e, 0,
                            g_gfxShopBg, shake * 16 + 0x75, 0x222, 0x10, 0x12);
                Blit((int)g_offX + 0x1a5 - k * 16, (int)(g_offY + g_shopBounceY) + 0x20e, 0,
                            g_gfxShopBg, shake * 16 + 0x75, 0x222, 0x10, 0x12);
                if (shake > 0) shake--;
            }
        }
        if (g_shopShake > 0.0)
            g_shopShake -= 0.7f;
        else
            g_shopShake = 0;
    }

    if (g_gfxShopBg)
        Blit((int)g_offX, (int)g_offY, 0, g_gfxShopBg, 0, 0, 800 - (int)g_offX, 0x20a);

    // ---- item list and description ----
    // Draw the item list bottom-to-top, colour-coded: selected (9), affordable (8), or
    // too expensive (7).
    x = 0x1d0;
    y = 490.0f;
    i = 0;
    step = 380.0 / (nitems + 1);
    for (i = 0; i < nitems + 1; i++) {
        for (j = 0; j < 25; j++)
            str[j] = varer[i][j];
        if (i == g_shopSelItem)
            DrawMenuText(str, (int)g_offX + x, (int)y + (int)g_offY, 9);
        else if (g_save.players[g_shopCurPlayer].money >= g_prices[i])
            DrawMenuText(str, (int)g_offX + x, (int)y + (int)g_offY, 8);
        else
            DrawMenuText(str, (int)g_offX + x, (int)y + (int)g_offY, 7);
        y -= step;
    }
    if (g_shopSelItem != 0 && P.money >= g_prices[g_shopSelItem] && g_gfxShopItemPic)
        BlitLocal((int)g_offX + 0x4b, (int)g_offY + 0x21, 0, g_gfxShopItemPic, 0, 0, 0x108, 0xc6);

    // Randomly seeds a red/green sparkle flash at each of the shop window's 4 corners
    // (once the panel has finished sliding in and the secret screen isn't showing).
    if (g_secretShown == 0 && (int)g_shopTransition == 500) {
        if (RandRange(0, 50) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x17a, 0x106, 0x14, 0xff, 0, 0, 300, 0x28);
        if (RandRange(0, 100) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x18f, 0x106, 0x14, 0, 0xff, 0, 300, 10);
        if (RandRange(0, 50) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x15d, 0x76, 0x14, 0xff, 0, 0, 300, 0x28);
        if (RandRange(0, 100) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x15d, 0x89, 0x14, 0, 0xff, 0, 300, 10);
        if (RandRange(0, 50) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x1e1, 0x6c, 0x14, 0xff, 0, 0, 300, 0x28);
        if (RandRange(0, 100) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x1f8, 0x6c, 0x14, 0, 0xff, 0, 300, 10);
        if (RandRange(0, 50) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x2a2, 0x56, 0x14, 0xff, 0, 0, 300, 0x28);
        if (RandRange(0, 100) == 2)
            AddSparkleFlash(g_gfxFlare10, 0x2b9, 0x56, 0x14, 0, 0xff, 0, 300, 10);

        FlushBlit2(0);
        UpdateSparkleFlashes();
        FlushBlit(0);
        FlushStretchF();
        FlushStretchRot();
        FlushStretchI();
        FlushStretchRot2();
    }

    if (g_shopSelItem > -1 && P.money >= g_prices[g_shopSelItem])
        DrawMixedCaseText(tekster[g_shopSelItem], (int)g_offX + 0x6c, (int)g_offY + 0x11d, 0);

    // ---- weapon/stat text ----
    switch (P.weapon) {
    case WEAPON_SINGLE: DrawMixedCaseText("CURRENT WEAPON : SINGLE SHOT", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_DOUBLE: DrawMixedCaseText("CURRENT WEAPON : DOUBLE SHOT", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_TRIPLE: DrawMixedCaseText("CURRENT WEAPON : TRIPLE SHOT", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_QUAD: DrawMixedCaseText("CURRENT WEAPON : QUAD SHOT", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_SUPER_TRIPLE: DrawMixedCaseText("CURRENT WEAPON : SUPER TRIPLE SHOT", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_PLASMA: DrawMixedCaseText("CURRENT WEAPON : PLASMA", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_FIREBALLS: DrawMixedCaseText("CURRENT WEAPON : FIREBALLS", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_LASER: DrawMixedCaseText("CURRENT WEAPON : LASER", SX + 0x74, SY + 0x1db, 1); break;
    case WEAPON_WAR_PLASMA: DrawMixedCaseText("CURRENT WEAPON : WAR.I.PLASMA", SX + 0x74, SY + 0x1db, 1); break;
    }

    if (P.autofire != 0 && P.superAuto == 0)
        DrawMixedCaseText("AUTOFIRE ON", SX + 0x128, SY + 0x1db, 1);
    if (P.superAuto != 0)
        DrawMixedCaseText("SUPER AUTOFIRE ON", SX + 0x128, SY + 0x1db, 1);
    Int64ToStrGrouped(P.score / P.level, g_scoreBuf);
    sprintf(g_logBuf, "SCORE PER LEVEL : %s", g_scoreBuf);
    DrawMixedCaseText(g_logBuf, SX + 0x74, SY + 0x1e3, 1);
    if (P.shots > 0) {
        sprintf(g_logBuf, "HIT PERCENTAGE : %d %%", (int)((double)P.hits / P.shots * 100.0));
        DrawMixedCaseText(g_logBuf, SX + 0x128, SY + 0x1e3, 1);
    } else
        DrawMixedCaseText("HIT PERCENTAGE : 0", SX + 0x128, SY + 0x1e3, 1);

    // Total play time (elapsed real time minus paused time) as a duration, then decoded
    // as a calendar date/time purely to read off H:M:S for display.
    StampTimeB();
    g_timeStampA = g_timeA;
    g_timeStampB = g_timeMarkB;
    g_playTimeFt = g_timeStampB - g_timeStampA - g_pausedDuration;
    SysFileTimeToDate(g_playTimeFt, &sy);
    monthDayBase = (sy.month - 1) * 30;
    dayOfYear = sy.day + monthDayBase - 1;
    hour = sy.hour;
    minute = sy.minute;
    second = sy.second;
    sprintf(g_logBuf, "GAME TIME : %1d H  %1d M  %1d Sec", hour, minute, second);
    DrawMixedCaseText(g_logBuf, SX + 0x74, SY + 0x1eb, 1);

    SysLocalDate(&sy);
    monthDayBase = (sy.month - 1) * 30;
    dayOfYear = sy.day + monthDayBase - 1;
    hour = sy.hour;
    minute = sy.minute;
    second = sy.second;
    sprintf(g_logBuf, "CLOCK IS NOW : %02d:%02d:%02d", hour, minute, second);
    DrawMixedCaseText(g_logBuf, SX + 0x128, SY + 0x1eb, 1);

    // Base line count for the stat block below (score/hit%%/time/clock); grows by one
    // when the "ALIEN LOCK ON" line is also shown, to push the rank line down to match.
    extraLineCount = 3;
    if (P.alienLock) {
        DrawMixedCaseText("ALIEN LOCK ON", SX + 0x74, SY + 0x1f3, 1);
        extraLineCount++;
    }
    if (P.rank > P.bestRank)
        P.bestRank = P.rank;
    sprintf(g_logBuf, "THIS GAME HIGHEST RANK : %s", g_rankNames[P.bestRank]);
    DrawMixedCaseText(g_logBuf, SX + 0x74, SY + extraLineCount * 8 + 0x1db, 1);

    // ---- money ----
    if (g_gameMode == MODE_DUAL) {
        sprintf(g_logBuf, "PL%1d:%8d$", g_shopCurPlayer + 1, P.money);
        DrawMenuText(g_logBuf, SX + 0x217, SY + 0x32, 9);
    } else {
        sprintf(g_logBuf, "%8d$", P.money);
        DrawMenuText(g_logBuf, SX + 0x221, SY + 0x32, 9);
    }
    if (g_profileIndex != -1 && g_shopBounceY >= 0.0)
        DrawSavePrompt();

    // ---- mouse hover ----
    // Mouse-over selection: while the cursor is over the item list column, pick the row
    // under it, clamp to range, force it back to the exit row if unaffordable, and play a
    // rollover sound + load the item's picture when the hovered row changes.
    if (g_buttonsOn && g_secretShown == 0 && (int)g_shopTransition == 500 && g_mouseX > SX + x &&
        g_mouseX < x + SX + 0xf0) {
        hoverStep = 380.0 / (nitems + 1);
        hoverItem = (int)((0 - (g_mouseY - 490.0 - 12.0)) / hoverStep);
        g_shopSelItem = hoverItem;
        if (g_shopSelItem < 0) g_shopSelItem = 0;
        if (g_shopSelItem >= nitems + 1) g_shopSelItem = nitems;
        hoverItem = g_shopSelItem;
        if (P.money < g_prices[g_shopSelItem]) {
            g_shopSelItem = 0;
            g_mouseDown = 0;
        } else if (g_shopHoverItem != hoverItem) {
            g_shopHoverItem = hoverItem;
            SoundPlay(g_sfxRollover, -1, 0x9b, 0.0f, 0x7f, g_sndFlags);
            LoadShopPic(g_shopSelItem - 1);
        }
    }

    FlushBlit2(0);
    UpdateSparkleFlashes();
    FlushBlit(0);
    FlushStretchF();
    FlushStretchRot();
    FlushStretchI();
    FlushStretchRot2();

    // ---- input ----
    if (g_shopPrevItem != g_shopSelItem)
        g_shopPrevItem = g_shopSelItem;
    if (g_shopClosing == 0 && g_state != STATE_PAUSED && g_inputCooldown < 1) {
        g_shopActiveTimer = 5;

#ifdef __EMSCRIPTEN__
        AutoSaveShop();
#else
        // F1: save and quit to Windows, after playing the goodbye jingle to completion
        // (pumping AudioUpdate() in a busy-wait since the main loop isn't running).
        if (KeyDown(K_VK_F1) && g_secretShown == 0 && (int)g_shopTransition == 500) {
            if (g_keyLatch[K_VK_F1]) {
                g_keyLatch[K_VK_F1] = 0;
                if (GetProfileLives(g_profileIndex) > 0) {
                    SaveProfile(g_profileIndex);
                    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1);
                    if (g_sfxGoodbye) {
                        AudioStop();
                        AudioStart();
                        g_exitSoundStartTime = g_time;
                        SoundPlay2(g_sfxGoodbye, -1, 0xff, 0.0f, 0xff, g_sndFlags);
                        goodbyeWaitUntil = g_time + 2500;
                        do {
                            g_time = SysMillis();
                            if (g_time == 0)
                                g_time = SysMillis();
                            if (g_soundEnabled)
                                AudioUpdate();
                        } while (goodbyeWaitUntil > g_time);
                    }

                    if (g_state != STATE_TITLE)
                        ResumeGame();
                    PlaySample();
                    WinCloseAll();
                    g_clickWin = -1;
                    g_clickItem = -1;
                    g_quitGameWinOpen = 0;
                    MergeSettings(g_profileIndex);
                    WriteSettings();
                    WriteHiscoreFile();

                    CLEAR_DRAW_COUNTERS()
                    DrawRect(0, 0, (float)g_screenW, (float)g_screenH, 0, 0, 0, 1);
                    SysTerminate();
                }
            }
        } else
            g_keyLatch[K_VK_F1] = 1;

        // F2: save the game, forget the auto-login profile (setpro.dat) and return to the
        // title screen. The profile and its save are kept.
        if (KeyDown(K_VK_F2) && g_secretShown == 0 && (int)g_shopTransition == 500) {
            if (g_keyLatch[K_VK_F2]) {
                g_keyLatch[K_VK_F2] = 0;
                if (GetProfileLives(g_profileIndex) > 0) {
                    SaveProfile(g_profileIndex);
                    MergeSettings(g_profileIndex);
                    DeleteSetPro();
                    ClearAccount();
#ifndef __EMSCRIPTEN__
                    g_profileIndex = -1;
#endif
                    PlaySample();
                    g_quitGameWinOpen = 0;
                    ResetToTitle();
                    g_inputCooldown = 100;
                    WinCloseAll();
                    g_clickWin = -1;
                    g_clickItem = -1;
                    g_profileWinOpen = 0;
                    LoadProfile();
                    StartMusic();
                }
            }
        } else
            g_keyLatch[K_VK_F2] = 1;

#endif

        // F7: take a screenshot.
        if (KeyDown(K_VK_F7)) {
            if (g_screenshotKeyEdge) {
                TakeScreenshot();
                g_screenshotKeyEdge = 0;
                PlayClick();
            }
        } else
            g_screenshotKeyEdge = 1;

        // Attract-mode/demo: pick a random item and weapon each frame instead of
        // reading real input.
        if (g_autoplay) {
            g_shopSelItem = RandRange(0, nitems + 2) - 1;
            P.weapon = RandRange(4, 8);
        }

        // Up/down move the selection by one row, skipping back if the new row is
        // unaffordable; both also dismiss the secret screen if it's showing.
        if (InputUp(g_shopCurPlayer) && g_secretShown == 0 && (int)g_shopTransition == 500) {
            if (g_shopUpEdge) {
                g_buttonsOn = 0;
                HidePointer();
                g_shopSelItem++;
                if (g_shopSelItem > nitems)
                    g_shopSelItem = nitems;
                if (P.money < g_prices[g_shopSelItem])
                    g_shopSelItem--;
                else {
                    g_shopUpEdge = 0;
                    SoundPlay(g_sfxRollover, -1, 0x9b, 0.0f, 0xdf, g_sndFlags);
                    LoadShopPic(g_shopSelItem - 1);
                }

                if (g_secretShown) {
                    g_secretShown = 0;
                    g_shopTransition = 0.84f;
                    g_transitionRate = 1.17f;
                    CHECK_BROKE()
                }
            }
        } else
            g_shopUpEdge = 1;

        if (InputDown(g_shopCurPlayer) && g_secretShown == 0 && (int)g_shopTransition == 500) {
            if (g_shopDownEdge) {
                g_buttonsOn = 0;
                HidePointer();
                if (--g_shopSelItem < 0)
                    g_shopSelItem = 0;
                g_shopDownEdge = 0;
                SoundPlay(g_sfxRollover, -1, 0x9b, 0.0f, 0xdf, g_sndFlags);
                LoadShopPic(g_shopSelItem - 1);
                if (g_secretShown) {
                    g_shopTransition = 0.84f;
                    g_transitionRate = 1.17f;
                    g_secretShown = 0;
                    CHECK_BROKE()
                }
            }
        } else
            g_shopDownEdge = 1;

        // Fire/click/Escape while the secret screen is fully shown dismisses it (and
        // slides the shop closed too if that purchase left the player broke).
        if ((InputFire(g_shopCurPlayer) || (g_mouseDown && g_buttonsOn) ||
             KeyDown(K_VK_ESCAPE)) &&
            g_transitionLock == 0 && g_secretShown && g_shopFireEdge && g_shopTransition == 0.0) {
            g_prevMouseX = Randff();
            g_buttonsOn = 0;
            HidePointer();
            g_secretShown = 0;
            g_shopTransition = 0.84f;
            g_transitionRate = 1.17f;
            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
            g_transitionLock = 1;
            CHECK_BROKE()
        }

        // Same dismiss-secret-screen action, but as an edge-triggered Escape key handler
        // covering the case where the fire button check above didn't catch it.
        if (KeyDown(K_VK_ESCAPE) && g_transitionLock == 0 &&
            (g_secretShown || (int)g_shopTransition == 0)) {
            if (g_keyLatch[K_VK_ESCAPE]) {
                if (g_shopTransition == 0.0) {
                    g_prevMouseX = Randff();
                    g_buttonsOn = 0;
                    HidePointer();
                    g_secretShown = 0;
                    g_shopTransition = 0.84f;
                    g_transitionRate = 1.17f;
                    g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                    g_transitionLock = 1;

                    CHECK_BROKE()
                }
                g_keyLatch[K_VK_ESCAPE] = 0;
            }
        } else
            g_keyLatch[K_VK_ESCAPE] = 1;

        // Escape on the main item list: back out to the exit row if something else is
        // selected, or leave the shop if already on it.
        if (KeyDown(K_VK_ESCAPE) && g_transitionLock == 0 && g_secretShown == 0 &&
            (int)g_shopTransition == 500) {
            if (g_keyLatch[K_VK_ESCAPE]) {
                if (g_shopSelItem != 0) {
                    g_buttonsOn = 0;
                    HidePointer();
                    g_shopSelItem = 0;
                    g_shopDownEdge = 0;
                    LoadShopPic(g_shopSelItem - 1);
                    SoundPlay(g_sfxRollover, -1, 0x9b, 0.0f, 0xdf, g_sndFlags);
                } else if (g_shopSelItem == 0) {
                    g_playerBroke = 1;
                    g_shopClosing = 1;
                    g_shopSlideVelX = 0;
                    g_shopSlideVelY = 4;
                    PlaySlideSfx();
                }
                g_keyLatch[K_VK_ESCAPE] = 0;
            }
        } else
            g_keyLatch[K_VK_ESCAPE] = 1;

        // Fire/click buys the selected item: exits the shop for the exit row, otherwise
        // dispatches to the per-item switch below if it's still affordable.
        if ((InputFire(g_shopCurPlayer) ||
             (g_mouseDown && g_buttonsOn && g_mouseX > 0x1d4 && g_mouseX < 0x2d6)) &&
            g_transitionLock == 0 && (int)g_shopTransition == 500) {
            if (g_shopFireEdge) {
                if (g_mouseDown == 0)
                    g_buttonsOn = 0;
                g_shopFireEdge = 0;

                if (g_shopSelItem == 0) {
                    g_playerBroke = 1;
                    g_shopClosing = 1;
                    g_shopSlideVelX = 0;
                    g_shopSlideVelY = 4;
                    PlaySlideSfx();
                } else {
                    if (g_mouseDown)
                        g_prevMouseX = Randff();

                    if (P.money >= g_prices[g_shopSelItem]) {
                        // One case per purchasable item (index 0 = exit is handled above),
                        // in the same order as `varer`/`tekster`/`g_prices`; each case
                        // applies the item's effect and, on success, AddCash() + PAY the
                        // price, or PlayBuzzer2Sfx() if the item is already maxed out.
                        switch (g_shopSelItem - 1) {
                        case SHOP_CLEAR_SHIELDS:    // (profile-only: resets medals, level-completion
                                    // and per-level secret-found flags for a fresh run at ranking up)
                            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
                                P.secretBirdCounter = 0x14;
                                P.secretBirdTick = 3;
                                P.secretBirdHits = 0;

                                ClearMedalOrder(g_profileIndex);
                                ClearMedalsMask(g_profileIndex, MEDAL_DRUNK_FINISH);
                                ClearMedalsMask(g_profileIndex, MEDAL_SPEED_STREAK);
                                ClearMedalsMask(g_profileIndex, MEDAL_BONUS_RATIO);
                                ClearMedalsMask(g_profileIndex, MEDAL_ALL_LEVELS);
                                ClearMedalsMask(g_profileIndex, MEDAL_OVERALL);
                                ClearMedalsMask(g_profileIndex, MEDAL_EXACT_MONEY);
                                ClearLevelsDone(g_profileIndex);
                                for (secretIdx = 0; secretIdx < g_numLevels; secretIdx++)
                                    P.secretFlags[secretIdx] = 0;
                                P.secretFlags[25] = 1;
                                MarkSecretFound(g_profileIndex, 0x1a);
                                RescaleRatioStat(g_profileIndex);
                                if (GetRank(g_profileIndex) == 1)
                                    IncrementRank(g_profileIndex);
                                AddCash();
                                PAY;
                            }
                            break;

                        case SHOP_SUPER_AUTOFIRE:
                            if (P.superAuto == 0) {
                                P.superAuto = 1;
                                P.autofireInterval = 0x19;
                                P.autofire = 1;
                                AddCash();
                                PAY;
                            } else
                                PlayBuzzer2Sfx();
                            break;

                        case SHOP_ALIEN_LOCK:
                            if (!P.alienLock) {
                                P.alienLock = 1;
                                AddCash();
                                PAY;
                            } else
                                PlayBuzzer2Sfx();
                            break;

                        case SHOP_ROCKET_PACK:
                            if (P.rockets < 50) {
                                P.rockets = P.rockets + 10;
                                if (P.rockets > 50)
                                    P.rockets = 50;
                                AddCash();
                                PAY;
                            } else
                                PlayBuzzer2Sfx();
                            break;

                        case SHOP_WAR_I_PLASMA:
                            BUY_WEAPON(WEAPON_WAR_PLASMA)
                            break;

                        case SHOP_LASER_BEAM:
                            BUY_WEAPON(WEAPON_LASER)
                            break;

                        case SHOP_EXTRA_TIME:
                            if (P.buffDuration < g_timeMax) {
                                P.buffDuration = P.buffDuration + 5;
                                AddCash();
                                PAY;
                            } else
                                PlayBuzzer2Sfx();
                            break;

                        case SHOP_RANK_MARKER:
                            // P.marks is a 6-bit collected-marks mask; sell the player
                            // the cheapest mark they're still missing (MARK_6 first).
                            gotMark = 0;
                            if (!((short)P.marks & MARK_6)) {
                                P.marks = (short)P.marks | MARK_6;
                                gotMark = 1;
                            }
                            TRY_GRANT_MARK(MARK_5)
                            TRY_GRANT_MARK(MARK_4)

                            TRY_GRANT_MARK(MARK_3)
                            TRY_GRANT_MARK(MARK_2)
                            TRY_GRANT_MARK(MARK_1)

                            if (gotMark == 0 && g_profileIndex != -1 && g_gameMode == MODE_SINGLE &&
                                g_playerUpdateFn != StateDemo) {
                                P.secretFlags[8] = 1;
                                MarkSecretFound(g_profileIndex, 9);
                            }
                            // gotMark == 0 here means the player already had all 6 marks:
                            // award the one-time all-marks bonus instead of a new mark.
                            if (gotMark == 0) {
                                if (!g_marksBonusGiven) {
                                    g_marksBonusGiven = 1;
                                    SCOREADD(RANK_UP_SCORE_BONUS);
                                    if (!P.maxRankReached)
                                        P.color = 1;
                                } else {
                                    PlayBuzzer2Sfx();
                                    break;
                                }
                            } else
                                AddCash();
                            PAY;
                            break;

                        case SHOP_GAME_SECRET:
                            if (g_gfxSecretScreen == 0)
                                ReloadSecret();

                            if (GetRank(g_profileIndex) == 2) {
                                // A special-rank account always gets the fixed secret #30.
                                g_secretPicPick = 0x1e;
                                g_secretPicPrev = g_secretPicCur;
                                g_secretPicCur = g_secretPicPick;
                                LoadSecretPic(g_secretPicPick);
                            } else {
                                // Pick a random secret the player hasn't seen yet (tracked
                                // per-profile), falling back to any pick after 250 tries.
                                gotUniquePick = 0;
                                pickAttempts = 0;
                                do {
                                    g_secretPicPick = RandRange(0, g_numLevels);
                                    pickAttempts++;
                                    if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE &&
                                        g_playerUpdateFn != StateDemo) {
                                        if (P.secretFlags[g_secretPicPick] != 0 &&
                                            P.secretSeen[g_secretPicPick] != 0)
                                            gotUniquePick = 0;
                                        else {
                                            gotUniquePick = 1;
                                            P.secretSeen[g_secretPicPick] = 1;
                                        }

                                    } else if (g_secretPicPick != g_secretPicCur &&
                                               g_secretPicPick != g_secretPicPrev)
                                        gotUniquePick = 1;
                                    if (pickAttempts > 250)
                                        gotUniquePick = 1;
                                } while (!gotUniquePick);
                                g_secretPicPrev = g_secretPicCur;
                                g_secretPicCur = g_secretPicPick;
                                LoadSecretPic(g_secretPicPick);
                            }

                            if (g_gfxSecretScreen) {
                                g_secretShown = 1;
                                g_shopTransition = 450.0f;
                                g_transitionRate = 0.871f;
                                AddCash();
                                PAY;
                            } else CHECK_BROKE()
                            break;

                        case SHOP_FIRE_BALLS:
                            BUY_WEAPON(WEAPON_FIREBALLS)
                            break;

                        case SHOP_EXTRA_LIFE:
                            if (P.lives < g_shipDefs[P.ship]->minEnergy + g_shipDefs[P.ship]->maxEnergy) {
                                P.lives = P.lives + g_shipDefs[P.ship]->cost;
                                AddCash();
                                PAY;
                                g_livesGainedCount = g_livesGainedCount + 10;
                            } else
                                PlayBuzzer2Sfx();
                            if (P.lives > g_shipDefs[P.ship]->minEnergy + g_shipDefs[P.ship]->maxEnergy)
                                P.lives = g_shipDefs[P.ship]->minEnergy + g_shipDefs[P.ship]->maxEnergy;
                            break;

                        case SHOP_PLASMA:
                            BUY_WEAPON(WEAPON_PLASMA)
                            break;

                        case SHOP_SHIP_ARMOUR:
                            if (P.armour <
                                g_shipDefs[P.ship]->baseArmour + g_shipDefs[P.ship]->maxArmourBonus) {
                                P.armour = P.armour + g_shipDefs[P.ship]->armourStep;
                                AddCash();
                                PAY;
                                g_armourAddedCount = g_armourAddedCount + 5;
                            } else
                                PlayBuzzer2Sfx();
                            if (P.armour >
                                g_shipDefs[P.ship]->baseArmour + g_shipDefs[P.ship]->maxArmourBonus)
                                P.armour = g_shipDefs[P.ship]->baseArmour +
                                           g_shipDefs[P.ship]->maxArmourBonus;
                            break;

                        case SHOP_SUPER_TRIPLE:
                            BUY_WEAPON(WEAPON_SUPER_TRIPLE)
                            break;

                        case SHOP_AUTO_FIRE:
                            if (P.superAuto == 0) {
                                if (P.autofire != 1) {
                                    P.autofire = 1;
                                    AddCash();
                                    PAY;
                                } else
                                    PlayBuzzer2Sfx();
                            } else
                                PlayBuzzer2Sfx();
                            break;

                        case SHOP_QUAD_SHOT:
                            BUY_WEAPON(WEAPON_QUAD)
                            break;

                        case SHOP_TRIPLE_SHOT:
                            BUY_WEAPON(WEAPON_TRIPLE)
                            break;

                        case SHOP_LESS_SPEED:
                            if (P.speed > g_speedBase) {
                                P.speed = P.speed - g_speedStep;
                                AddCash();
                                PAY;
                            } else
                                PlayBuzzer2Sfx();
                            break;

                        case SHOP_DOUBLE_SHOT:
                            BUY_WEAPON(WEAPON_DOUBLE)
                            break;

                        case SHOP_EXTRA_BULLET:
                            if (P.bullets < 50) {
                                P.bullets++;
                                AddCash();
                                PAY;
                            } else
                                PlayBuzzer2Sfx();
                            break;

                        case SHOP_EXTRA_SPEED:
                            if (P.speed < g_speedStep * g_maxSpeedMul + g_speedBase) {
                                P.speed = P.speed + g_speedStep;
                                if (P.speed > g_speedStep * g_maxSpeedMul + g_speedBase)
                                    P.speed = g_speedStep * g_maxSpeedMul + g_speedBase;
                                AddCash();
                                PAY;
                            } else
                                PlayBuzzer2Sfx();
                            break;
                        }

                        if (P.money < 50.0 && g_secretShown == 0) {
                            g_playerBroke = 1;
                            g_shopClosing = 1;
                            g_shopSlideVelX = 0;
                            g_shopSlideVelY = 4;
                            PlaySlideSfx();
                        }
                    } else {
                        if (g_secretShown == 0)
                            PlayBuzzerSfx();
                        else {
                            g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
                            g_transitionLock = 1;
                        }

                        CHECK_BROKE()
                    }
                }
            }
        } else
            g_shopFireEdge = 1;
    }

    // ---- secret-screen panel ----
    // Draws the secret panel (revealed/hidden while it's animating via g_shopTransition,
    // sliding out from BX along with a title-bar strip) and, when particles are on, a
    // dimming quad plus 4 trailing "ghost" copies of the panel and title bar for a motion
    // blur / afterimage look while it slides.
    if (g_gfxSecretScreen && g_shopClosing == 0 && (g_secretShown || (int)g_shopTransition != 500)) {
        if (g_cfg.particlesOn) {
            ImgSetAlphaMode(g_gfxLogos, 4);
            ImgSetBlitColor(g_gfxLogos, 0, 0, 0, 0.96f);
            QueueQuad(0, 0, 0, g_screenW, g_screenH, g_gfxSecretScreen, 128, 158, 160, 190);
            QueueQuad(0, 0, 0, g_screenW, g_screenH, g_gfxSecretScreen, 128, 158, 160, 190);
            ImgSetBlitColor(g_gfxSecretScreen, 0, 0, 0, 0.2f);

            Blit(BX - (int)g_shopTransition + 20, BY + 20, 0, g_gfxSecretScreen, 0, 0, SECRET_PANEL_W, SECRET_PANEL_H);
            QueueQuad(0, BX - g_shopTransition - 300.0 + 20.0, BY + 13.0 + 20.0, 469, 28,
                      g_gfxSecretScreen, 0, 13, 169, 28);
            Blit(BX - (int)g_shopTransition + 15, BY + 15, 0, g_gfxSecretScreen, 0, 0, SECRET_PANEL_W, SECRET_PANEL_H);
            QueueQuad(0, BX - g_shopTransition - 300.0 + 15.0, BY + 13.0 + 15.0, 469, 28,
                      g_gfxSecretScreen, 0, 13, 169, 28);

            Blit(BX - (int)g_shopTransition + 10, BY + 10, 0, g_gfxSecretScreen, 0, 0, SECRET_PANEL_W, SECRET_PANEL_H);
            QueueQuad(0, BX - g_shopTransition - 300.0 + 10.0, BY + 13.0 + 10.0, 469, 28,
                      g_gfxSecretScreen, 0, 13, 169, 28);
            Blit(BX - (int)g_shopTransition + 5, BY + 5, 0, g_gfxSecretScreen, 0, 0, SECRET_PANEL_W, SECRET_PANEL_H);
            QueueQuad(0, BX - g_shopTransition - 300.0 + 5.0, BY + 13.0 + 5.0, 469, 28,
                      g_gfxSecretScreen, 0, 13, 169, 28);

            FlushBlit2(0);
            UpdateSparkleFlashes();
            FlushBlit(0);
            FlushStretchF();
            FlushStretchRot();
            FlushStretchI();
            FlushStretchRot2();
            FlushQuads(0);
            ImgSetAlphaMode(g_gfxLogos, 1);
            ImgSetBlitColor(g_gfxLogos, 1, 1, 1, 1);
            ImgSetBlitColor(g_gfxSecretScreen, 1, 1, 1, 1);
        }

        Blit(BX - (int)g_shopTransition, BY, 0, g_gfxSecretScreen, 0, 0, SECRET_PANEL_W, SECRET_PANEL_H);
        QueueQuad(0, BX - g_shopTransition - 300.0, BY + 13.0, 469, 28,
                  g_gfxSecretScreen, 0, 13, 169, 28);

        if (g_gfxSecretPic) {
            ImgSetBlitColor(g_gfxSecretPic, 1, 1, 1, RandFloat(0.93f, 1));
            QueueQuad(0, BX - g_shopTransition + 45.0, BY + 147.0, 488, 236,
                      g_gfxSecretPic, 0, 0, 420, 206);
            // "Already seen" checkmark badge in the corner of the secret picture.
            if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo &&
                IsSecretFound(g_profileIndex, g_secretPicPick + 1))
                QueueQuad(0, BX - g_shopTransition + 44.0 + 458.0, BY + 356.0, 16, 18,
                          g_gfxLogos, 21, 81, 9, 7);
        }

        FlushBlit2(0);
        UpdateSparkleFlashes();
        FlushBlit(0);
        FlushStretchF();
        FlushStretchRot();
        FlushQuads(0);
        FlushStretchI();
        FlushStretchRot2();

        // Two sparkle flares (a bigger dim one, a smaller bright one) at each of the 4
        // secret-panel corners.
        if (g_cfg.particlesOn) {
            ImgSetBlitColor(g_gfxFlare5, RandFloat(0.5f, 1), RandFloat(0.5f, 1), RandFloat(0.5f, 1), 1);
            FX(1.2f, 1.9f, 127.0, 209.0);
            FX(1.2f, 1.9f, 673.0, 210.0);
            FX(1.2f, 1.9f, 127.0, 492.0);
            FX(1.2f, 1.9f, 673.0, 492.0);

            ImgSetBlitColor(g_gfxFlare5, RandFloat(0.5f, 1), RandFloat(0.5f, 1), RandFloat(0.5f, 1), 1);
            FX(0.6f, 1, 127.0, 209.0);
            FX(0.6f, 1, 673.0, 210.0);
            FX(0.6f, 1, 127.0, 492.0);
            FX(0.6f, 1, 673.0, 492.0);
            ImgSetBlitColor(g_gfxFlare5, 1, 1, 1, 1);
        }

        // g_shopTransition eases toward 0 (fully hidden) or 500 (fully shown) by a
        // multiplicative rate each frame, then snaps to the target once close enough.
        g_shopTransition *= g_transitionRate;
        if (g_transitionRate < 1.0 && g_shopTransition < 0.5) {
            g_shopTransition = 0;
            g_transitionRate = 1;
        }
        if (g_transitionRate > 1.0 && g_shopTransition > 500.0) {
            g_shopTransition = 500;
            g_transitionRate = 1;
        }
    }

    if (g_state == STATE_PAUSED)
        DrawWarpRing();

    // ---- closing: hand-off, rank bumps, next player ----
    // Once the shop panel has finished sliding all the way up and closed: hand control
    // back to the HUD view, then check for a couple of legendary/end-game rank awards,
    // and finally either promote the player a rank (if all 6 marks are collected and
    // they've hit the next rank's stat requirement) or hand off to the next
    // player/level via NEXTPLAYER.
    if (g_shopClosing && g_offY <= -600.0 && g_state) {
        g_shopClosing = 0;
        g_transitionLockUntil = g_time + TRANSITION_LOCK_MS;
        g_transitionLock = 1;
        RETURN_TO_HUD_VIEW()
        g_shopKeyLatchD = 0;
        g_shopActiveTimer = 0;

        if (g_gameMode == MODE_DUAL) {
            g_save.players[0].levelTransitioning = 0;
            g_save.players[1].levelTransitioning = 0;
        } else
            P.levelTransitioning = 0;

        // Two one-off "legendary" rank stat bumps beyond the normal mark-based
        // promotion below, each gated on a stat value that's only reachable once.
        if (g_profileIndex != -1 && g_gameMode == MODE_SINGLE && g_playerUpdateFn != StateDemo) {
            if (HasAllMedals(g_profileIndex) && P.level > 500 && P.rank == RANK_CHAMPION &&
                P.secretCount == g_numLevels && GetStat(g_profileIndex) < RANK_GOD) {
                P.marks = 0;
                P.gemSeqB = -1;
                P.gemSeqA = -1;
                SCOREADD(RANK_UP_SCORE_BONUS);
                NewRank();
                SetStat(g_profileIndex, RANK_GOD);
                SoundQueueAdd(g_sfxNew, 0x28, 1);
                SoundQueueAdd(g_sfxRank, 0x28, 1);
                SoundQueueAdd(g_sfxAvailable, 0x28, 1);
                g_rankFanfarePlayed = 1;
            }

            if (P.rank == RANK_GRANDMASTER_3 && P.level > 100 && P.perfectStreak >= 24 && GetStat(g_profileIndex) < RANK_CHAMPION) {
                P.marks = 0;
                P.gemSeqB = -1;
                P.gemSeqA = -1;
                NewRank();
                SetStat(g_profileIndex, RANK_CHAMPION);
                SoundQueueAdd(g_sfxNew, 0x28, 1);
                SoundQueueAdd(g_sfxRank, 0x28, 1);
                SoundQueueAdd(g_sfxAvailable, 0x28, 1);
                g_rankFanfarePlayed = 1;
            }
        }

        if (PlayerHasAllMarks(g_shopCurPlayer) && g_rankFanfarePlayed == 0) {
            RETURN_TO_HUD_VIEW()

            if (P.rank < GetStat(g_profileIndex)) {
                g_promoPlayer = g_shopCurPlayer;
                P.marks = 0;
                P.gemSeqB = -1;
                P.gemSeqA = -1;
                SCOREADD(RANK_UP_SCORE_BONUS);
                P.rank = P.rank + 1;
                if (P.rank > P.bestRank)
                    P.bestRank = P.rank;

                // Below the top rank, just note when the promotion popup may show again.
                // At/above the max rank (RANK_CHAMPION+), also flag the run as having reached
                // max rank and award a bonus for every further promotion beyond it.
                if (P.rank < RANK_CHAMPION) {
                    g_rankLockUntil = g_time + 1200000;
                    g_rankPopupMinTime = g_time + 4000;
                    g_rankMsgActive = 0;
                } else {
                    if (P.rank == RANK_CHAMPION) {
                        P.secretFlags[24] = 1;
                        MarkSecretFound(g_profileIndex, 0x19);
                        g_rankLockUntil = g_time + 1200000;
                        g_rankPopupMinTime = g_time + 4000;
                        g_rankMsgActive = 0;
                        P.maxRankReached = 1;
                        P.color = 0;
                    }

                    if (P.rank == RANK_GOD) {
                        if (GetRank(g_profileIndex) == 0 && !GetProfileEasyFlag(g_profileIndex))
                            IncrementRank(g_profileIndex);
                        SCOREADD(GOD_RANK_SCORE_BONUS);
                        g_rankLockUntil = g_time + 1200000;
                        g_rankPopupMinTime = g_time + 4000;
                        g_rankMsgActive = 0;
                        P.maxRankReached = 1;
                        P.color = 0;
                    }

                    if (P.rank > RANK_GOD) {
                        SCOREADD(GOD_RANK_SCORE_BONUS);
                        g_rankLockUntil = g_time + 1200000;
                        g_rankPopupMinTime = g_time + 4000;
                        g_rankMsgActive = 0;
                        P.maxRankReached = 1;
                        P.color = 0;
                    }
                }

                PlayRankPromotionSounds();
                g_state = STATE_SHOP_GATE;
                g_buttonsOn = 0;
                HidePointer();
                EmptyViewChangeHook();
            } else {
                P.marks = 0;
                P.gemSeqB = -1;
                P.gemSeqA = -1;
                SCOREADD(RANK_UP_SCORE_BONUS);
                NEXTPLAYER;
            }
        } else {
            NEXTPLAYER;
        }
    }
}

#undef PAY
#undef P
#undef SX
#undef SY
#undef BX
#undef BY
#undef FX
#undef SCOREADD
#undef LEAVESHOP
#undef NEXTPLAYER
#undef CHECK_BROKE
#undef BUY_WEAPON
#undef TRY_GRANT_MARK
