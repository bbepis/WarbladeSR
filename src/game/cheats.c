// cheats.c: Old-school cheat code (not in the original). Typing CHEAT_CODE during play
// toggles cheats on/off; while they're on, the number keys 1-9 (top row or numpad) apply
// one cheat each to the current player:
//   1 extra life          4 best weapon (WAR.I.PLASMA, max bullets)   7 shield
//   2 money +1000         5 full armour                               8 smart bomb
//   3 skip level (warp)   6 max rockets + super autofire              9 god mode on/off
#include <stdio.h>
#include "globals.h"
#include "game.h"

// Only letters that aren't in-game hotkeys (B E F I M N P S U V W Z), so typing it doesn't
// also flip settings.
#define CHEAT_CODE "GALAGA"

enum {
    CHEAT_CODE_TIMEOUT_MS = 2000,   // max gap between two letters of the code
    CHEAT_MONEY           = 1000,
    CHEAT_MAX_BULLETS     = 50,     // the EXTRA BULLET pickup's cap
    CHEAT_MAX_ROCKETS     = 50,     // the shop's ROCKET PACK cap
    CHEAT_SUPER_AUTO_RATE = 0x19,   // the shop's SUPER AUTOFIRE interval
};

bool g_cheatsOn;
bool g_cheatGodMode;

#define PL g_save.players[g_curPlayer]

#define SHIP g_shipDefs[PL.ship]

static unsigned char s_letterLatch[26];
static unsigned char s_digitLatch[10];
static int s_codePos;
static unsigned int s_lastLetterTime;

static void CheatMsg(const char *msg)
{
    sprintf(g_optionMsg, "%s", msg);
    g_msgTime = g_time + MSG_DURATION_MS;
}

// Returns the letter ('A'-'Z') newly pressed this frame, or 0.
static char NewLetterPressed()
{
    char pressed = 0;
    for (int i = 0; i < 26; i++) {
        if (KeyDown((enum EKeyboardLayout)(K_VK_A + i))) {
            if (s_letterLatch[i] == 0 && pressed == 0)
                pressed = (char)('A' + i);
            s_letterLatch[i] = 1;
        } else {
            s_letterLatch[i] = 0;
        }
    }
    return pressed;
}

// Returns the digit (1-9, top row or numpad) newly pressed this frame, or 0.
static int NewDigitPressed()
{
    int pressed = 0;
    for (int d = 1; d <= 9; d++) {
        if (KeyDown((enum EKeyboardLayout)(K_VK_0 + d)) || KeyDown((enum EKeyboardLayout)(K_VK_NUM0 + d))) {
            if (s_digitLatch[d] == 0 && pressed == 0)
                pressed = d;
            s_digitLatch[d] = 1;
        } else {
            s_digitLatch[d] = 0;
        }
    }
    return pressed;
}

static void ApplyCheat(int n)
{
    switch (n) {
    case 1:  // extra life, same as the EXTRA LIFE pickup minus its armour/score fallbacks
        if (PL.lives < SHIP->minEnergy + SHIP->maxEnergy) {
            PL.lives += SHIP->cost;
            if (PL.lives > SHIP->minEnergy + SHIP->maxEnergy)
                PL.lives = SHIP->minEnergy + SHIP->maxEnergy;
            SoundQueueAdd(g_sfxExtraLife, 50, 0);
            CheatMsg("CHEAT : EXTRA LIFE");
        } else {
            CheatMsg("CHEAT : LIVES ARE FULL");
        }
        break;

    case 2:  // money, up to the wallet size
        PL.money += CHEAT_MONEY;
        if (PL.money > PL.moneyMax)
            PL.money = PL.moneyMax;
        if (PL.money > g_moneyMax)
            g_moneyMax = PL.money;
        SoundQueueAdd(g_sfxMoney, 50, 0);
        CheatMsg("CHEAT : MONEY");
        break;

    case 3:  // skip level: the WARP pickup (once; each warp also raises enemy toughness)
        if (PL.levelFinished)
            return;
        Pickup(ITEM_WARP);
        CheatMsg("CHEAT : SKIP LEVEL");
        break;

    case 4:  // best weapon
        PL.weapon = WEAPON_WAR_PLASMA;
        PL.bullets = CHEAT_MAX_BULLETS;
        CheatMsg("CHEAT : WAR.I.PLASMA");
        break;

    case 5:  // full armour
        PL.armour = SHIP->baseArmour + SHIP->maxArmourBonus;
        SoundQueueAdd(g_sfxArmour, 50, 0);
        CheatMsg("CHEAT : FULL ARMOUR");
        break;

    case 6:  // rockets + super autofire, as bought in the shop
        PL.rockets = CHEAT_MAX_ROCKETS;
        PL.superAuto = 1;
        PL.autofire = 1;
        PL.autofireInterval = CHEAT_SUPER_AUTO_RATE;
        CheatMsg("CHEAT : ROCKETS + SUPER AUTOFIRE");
        break;

    case 7:  // the SHIELD pickup
        Pickup(ITEM_SHIELD);
        CheatMsg("CHEAT : SHIELD");
        break;

    case 8:  // smart bomb: the GEM BOMB pickup
        Pickup(ITEM_GEM_BOMB);
        CheatMsg("CHEAT : SMART BOMB");
        break;

    case 9:  // god mode: enemy shots and level objects pass through (see BulletsVsPlayer)
        g_cheatGodMode = !g_cheatGodMode;
        CheatMsg(g_cheatGodMode ? "CHEAT : GOD MODE ON" : "CHEAT : GOD MODE OFF");
        break;
    }
    PlayClick();
}

// Polled from Hotkeys() every gameplay frame: watches for CHEAT_CODE being typed (toggling
// cheats) and, while cheats are on, applies the cheat for a newly pressed number key. Only
// in real play (not the demo or autoplay), with no window open and the player alive.
void CheatHotkeys()
{
    char letter;
    int digit;

    letter = NewLetterPressed();
    digit = NewDigitPressed();
    if (g_playerUpdateFn == StateDemo || g_autoplay || AnyWindowActive())
        return;

    // ---- the code: each letter must follow the previous one within the timeout ----
    if (letter != 0) {
        if (g_time - s_lastLetterTime > CHEAT_CODE_TIMEOUT_MS)
            s_codePos = 0;
        s_lastLetterTime = g_time;
        if (letter == CHEAT_CODE[s_codePos])
            s_codePos++;
        else
            s_codePos = letter == CHEAT_CODE[0] ? 1 : 0;

        if (CHEAT_CODE[s_codePos] == '\0') {
            s_codePos = 0;
            g_cheatsOn = !g_cheatsOn;
            if (!g_cheatsOn)
                g_cheatGodMode = false;
            SoundPlay(g_sfxFanfare, -1, 0xff, 0.0f, 0xdf, g_sndFlags);
            CheatMsg(g_cheatsOn ? "CHEATS ENABLED : KEYS 1-9" : "CHEATS DISABLED");
        }
    }

    // ---- number keys: god mode toggles any time (respawning, "get ready"), the others need
    //      normal play and the ship alive ----
    if (digit != 0 && g_cheatsOn && (digit == 9 || (g_state == STATE_PLAYING && PL.dead == 0)))
        ApplyCheat(digit);
}
