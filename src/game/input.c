// input.c: Player input: per-action input tests (keyboard, joystick, autoplay AI) and the
// autoplay target finders.
#include <stdio.h>
#include "globals.h"
#include "game.h"

// A keyboard player also takes the same control from their gamepad (pad p, SysPadState); pad 0
// includes the browser page's touch controls. Not in the original.
#define PAD(p, bit) ((SysPadState(p) & (bit)) != 0)


// Finds the lowest (highest y) alive/active item within the horizontal play field and above
// the current player, for auto-targeting; result in g_targetItemX/g_targetItemY, or (-1, -1)
// if none found.
void FindTargetItem()
{
    int best;
    float maxY;
    int i;

    g_targetItemX = -1.0f;
    g_targetItemY = -1.0f;
    best = -1;
    maxY = 0.0f;
    for (i = 0; i < MAX_ITEMS; i++) {
        if (g_items[i].alive != 0 && g_items[i].active != 0 && g_items[i].x > 70.0) {
            if (g_items[i].x < g_screenW - 70 &&
                g_items[i].y < g_save.players[g_curPlayer].y + 32 &&
                g_items[i].y > maxY) {
                maxY = g_items[i].y;
                best = i;
            }
        }
    }
    if (best != -1) {
        g_targetItemX = g_items[best].x;
        g_targetItemY = g_items[best].y;
    }
}

// Finds the lowest active enemy (excluding captured/type 8, above y=500, within the
// horizontal play field) for the current player, for auto-targeting; result in
// g_targetObjX/g_targetObjY, or -1 if none found.
void FindTargetObj()
{
    int best;
    float maxY;
    int i;

    g_targetObjX = -1.0f;
    best = -1;
    maxY = 0.0f;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[g_curPlayer][i].active == 1 &&
            g_enemies[g_curPlayer][i].type != ENEMY_CAPTURED &&
            g_enemies[g_curPlayer][i].y < 500.0 &&
            g_enemies[g_curPlayer][i].x > 70.0) {
            if (g_enemies[g_curPlayer][i].x < g_screenW - 70 &&
                g_enemies[g_curPlayer][i].y > maxY) {
                maxY = g_enemies[g_curPlayer][i].y;
                best = i;
            }
        }
    }
    if (best != -1) {
        g_targetObjX = g_enemies[g_curPlayer][best].x;
        g_targetObjY = g_enemies[g_curPlayer][best].y;
    }
}

// Finds the lowest active falling gem (within the horizontal play field, above y=550), for
// auto-targeting in the Gem Drop stage; result (center x) in g_target847X, or -1 if none found.
void FindTargetGem()
{
    int best;
    float maxY;
    int i;

    g_target847X = -1.0f;
    best = -1;
    maxY = 0.0f;
    for (i = 0; i < MAX_FALLING_GEMS; i++) {
        if (g_fallingGems[i].active != 0 && g_fallingGems[i].y < 550.0 &&
            g_fallingGems[i].x + (g_fallingGems[i].w >> 1) > 70.0) {
            if (g_fallingGems[i].x + (g_fallingGems[i].w >> 1) < g_screenW - 70 &&
                g_fallingGems[i].y > maxY) {
                maxY = g_fallingGems[i].y;
                best = i;
            }
        }
    }
    if (best != -1)
        g_target847X = g_fallingGems[best].x + (g_fallingGems[best].w >> 1);
}

// Finds the lowest active bonus meteor (within the horizontal play field, above y=550), for
// auto-targeting in the meteor-storm round; result (center x, bottom y) in
// g_targetB49X/g_targetB49Y, or -1 if none found.
void FindTargetMeteor()
{
    int best;
    float maxY;
    int i;
    float cx;

    g_targetB49X = -1.0f;
    best = -1;
    maxY = 0.0f;
    for (i = 0; i < MAX_BONUS_METEORS; i++) {
        if (g_bonusMeteors[i].active != 0 && g_bonusMeteors[i].y + g_bonusMeteors[i].h < 550.0) {
            cx = g_bonusMeteors[i].x + g_bonusMeteors[i].w / 2.0f;
            if (cx > 70.0 && cx < g_screenW - 70 &&
                g_bonusMeteors[i].y + g_bonusMeteors[i].h > maxY) {
                maxY = g_bonusMeteors[i].y + g_bonusMeteors[i].h;
                best = i;
            }
        }
    }
    if (best != -1) {
        g_targetB49X = g_bonusMeteors[best].x + g_bonusMeteors[best].w / 2.0f;
        g_targetB49Y = g_bonusMeteors[best].y + g_bonusMeteors[best].h;
    }
}

// One bit of GetFlagMask's mask: if joystick `joy` button `bit` is held, set it in `flags`.
#define JOY_BIT(bit) \
    if (JoyButton(joy, bit) == true) \
        flags |= (bit);

// Builds a 32-bit mask of which of joystick `joy`'s buttons (bits 0-31) are currently held.
unsigned int GetFlagMask(unsigned char joy)
{
    unsigned int flags = 0;
    JOY_BIT(0x1)
    JOY_BIT(0x2)
    JOY_BIT(0x4)
    JOY_BIT(0x8)
    JOY_BIT(0x10)
    JOY_BIT(0x20)
    JOY_BIT(0x40)
    JOY_BIT(0x80)

    JOY_BIT(0x100)
    JOY_BIT(0x200)
    JOY_BIT(0x400)
    JOY_BIT(0x800)
    JOY_BIT(0x1000)
    JOY_BIT(0x2000)
    JOY_BIT(0x4000)
    JOY_BIT(0x8000)

    JOY_BIT(0x10000)
    JOY_BIT(0x20000)
    JOY_BIT(0x40000)
    JOY_BIT(0x80000)
    JOY_BIT(0x100000)
    JOY_BIT(0x200000)
    JOY_BIT(0x400000)
    JOY_BIT(0x800000)

    JOY_BIT(0x1000000)
    JOY_BIT(0x2000000)
    JOY_BIT(0x4000000)
    JOY_BIT(0x8000000)
    JOY_BIT(0x10000000)
    JOY_BIT(0x20000000)
    JOY_BIT(0x40000000)
    JOY_BIT(0x80000000)
    return flags;
}
#undef JOY_BIT

// Return legacy joystick axes explicitly; callers must not depend on register leftovers.
long GetJoyX(unsigned char joy)
{
    return JoyX(joy, 0);
}

long GetJoyY(unsigned char joy)
{
    return JoyY(joy, 0);
}

// True if player p is pressing "move left" this frame: keyboard, joystick axis (setting
// g_joystickSpeedMul from how far the stick is pushed), or, in autoplay, the AI's target
// logic for the current game state.
// NOTE: the Gem Drop (STATE_GEM_DROP) and race (STATE_BONUS_RACE) branches each end in an unconditional
// return, so the braced block and final AI-timer check below them are dead code.
int InputLeft(int p)
{
    if (g_autoplay) {
        if (g_state == STATE_GEM_DROP) {
            FindTargetGem();
            if (g_target847X != -1.0 && g_target847X < g_save.players[p].x)
                return 1;
            return 0;
        }
        if (g_state == STATE_BONUS_RACE) {
            if (g_aiLeft > 0) {
                g_aiLeft--;
                return 1;
            }
            if (RandRange(0, 100) < 15 || g_aiRight == 0) {
                FindTargetMeteor();
                if (g_targetB49X > g_save.players[p].x) {
                    g_aiLeft = RandRange(5, 50);
                    return 1;
                }
            }
            return 0;
        }

        {
            FindTargetItem();
            FindTargetObj();
            if (g_targetItemY > g_targetObjY && g_targetItemX != -1.0 &&
                g_targetItemX < g_save.players[p].x)
                return 1;
            if (g_targetObjX != -1.0 && g_targetObjX < g_save.players[p].x)
                return 1;
            return 0;
        }

        // NOTE: unreachable (both branches above return unconditionally); kept as in the original.
        if (g_aiTimer > 0) {
            g_aiTimer--;
            return 1;
        }
        if (RandRange(0, 100) < 2) {
            g_aiTimer = RandRange(5, 40);
            return 1;
        }
    }

    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        g_joystickSpeedMul = 1.0f;
        if (KeyDown((enum EKeyboardLayout)g_cfg.left[p]) == true || PAD(p, PAD_LEFT))
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetJoyX(0) < g_joyCenter - g_joyDead) {
            if (g_joyCenter == 0.0)
                g_joyCenter = 1.0f;
            g_joystickSpeedMul = (g_joyCenter - GetJoyX(0)) / g_joyCenter;
            return 1;
        } else {
            g_joystickSpeedMul = 1.0f;
            return 0;
        }
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetJoyX(1) < g_joyCenter - g_joyDead) {
            if (g_joyCenter == 0.0)
                g_joyCenter = 1.0f;
            g_joystickSpeedMul = (g_joyCenter - GetJoyX(1)) / g_joyCenter;
            return 1;
        } else {
            g_joystickSpeedMul = 1.0f;
            return 0;
        }
    }
    return 0;
}

// True if player p is pressing "move right" this frame; mirrors InputLeft.
int InputRight(int p)
{
    if (g_autoplay) {
        if (g_state == STATE_GEM_DROP) {
            FindTargetGem();
            if (g_target847X != -1.0 && g_target847X > g_save.players[p].x)
                return 1;
            return 0;
        }

        if (g_state == STATE_BONUS_RACE) {
            if (g_aiRight > 0) {
                g_aiRight--;
                return 1;
            }
            if (RandRange(0, 100) < 15 || g_aiLeft == 0) {
                FindTargetMeteor();
                if (g_targetB49X < g_save.players[p].x) {
                    g_aiRight = RandRange(5, 50);
                    return 1;
                }
            }
            return 0;
        }
        {
            FindTargetItem();
            FindTargetObj();
            if (g_targetItemY > g_targetObjY && g_targetItemX != -1.0 &&
                g_targetItemX > g_save.players[p].x)
                return 1;
            if (g_targetObjX != -1.0 && g_targetObjX > g_save.players[p].x)
                return 1;
            return 0;
        }
    }

    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.right[p]) == true || PAD(p, PAD_RIGHT))
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetJoyX(0) > g_joyCenter + g_joyDead) {
            if (g_joyCenter == 0.0)
                g_joyCenter = 1.0f;
            g_joystickSpeedMul = (GetJoyX(0) - g_joyCenter) / g_joyCenter;
            return 1;
        } else {
            g_joystickSpeedMul = 1.0f;
            return 0;
        }
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetJoyX(1) > g_joyCenter + g_joyDead) {
            if (g_joyCenter == 0.0)
                g_joyCenter = 1.0f;
            g_joystickSpeedMul = (GetJoyX(1) - g_joyCenter) / g_joyCenter;
            return 1;
        } else {
            g_joystickSpeedMul = 1.0f;
            return 0;
        }
    }
    return 0;
}

// True if player p is pressing "move down" this frame (or, in autoplay, a 50% random press).
int InputDown(int p)
{
    if (g_autoplay && RandRange(0, 100) < 50)
        return 1;
    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.down[p]) == true || PAD(p, PAD_DOWN))
            return 1;
        else
            return 0;
    }
    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetJoyY(0) > g_joyCenter + g_joyDead)
            return 1;
        else
            return 0;
    }
    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetJoyY(1) > g_joyCenter + g_joyDead)
            return 1;
        else
            return 0;
    }
    return 0;
}

// True if player p is pressing "move up" this frame (or, in autoplay, a periodic AI dodge).
int InputUp(int p)
{
    if (g_autoplay) {
        if (g_aiTimer > 0) {
            g_aiTimer--;
            return 1;
        }
        if (RandRange(0, 100) < 2) {
            g_aiTimer = RandRange(5, 40);
            return 1;
        }
    }

    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.up[p]) == true || PAD(p, PAD_UP))
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetJoyY(0) < g_joyCenter - g_joyDead)
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetJoyY(1) < g_joyCenter - g_joyDead)
            return 1;
        else
            return 0;
    }
    return 0;
}

// True if player p is pressing fire this frame. In autoplay this also posts the
// "AUTO PLAY GAME" banner and drives state-specific AI firing odds.
int InputFire(int p)
{
    if (g_autoplay) {
        sprintf(g_alertMsg, "A U T O   P L A Y   G A M E");
        g_msgColor = 2;
        g_msgTimer = g_time + 5000;
        if (g_state == STATE_BONUS_RACE && RandRange(0, 100) < 90)
            return 1;
        if (g_state == STATE_MEMORY_STATION && RandRange(0, 100) < 100)
            return 1;
        if (g_state == STATE_SHOP && RandRange(0, 100) < 10)
            return 1;
        if (g_rankMsgActive != 0 && RandRange(0, 100) < 10)
            return 1;
        if (g_state != STATE_SHOP && g_state != STATE_MEMORY_STATION) {
            FindTargetItem();
            FindTargetObj();
            if (g_targetItemY < g_targetObjY || RandRange(0, 100) < 25) {
                if (g_targetObjX != -1.0 && RandRange(0, 100) < 70)
                    return 1;
                return 0;
            }
        }
    }

    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.fire[p]) == true || PAD(p, PAD_FIRE))
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetFlagMask(0) & g_cfg.joyFire[p])
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetFlagMask(1) & g_cfg.joyFire[p])
            return 1;
        else
            return 0;
    }
    return 0;
}

// True if player p is pressing rocket/missile this frame (or, in autoplay, a 4% random press).
int InputRocket(int p)
{
    if (g_autoplay) {
        if (RandRange(0, 100) < 4)
            return 1;
        return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.rocket[p]) == true || PAD(p, PAD_ROCKET))
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetFlagMask(0) & g_cfg.joyRocket[p])
            return 1;
        else
            return 0;
    }

    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetFlagMask(1) & g_cfg.joyRocket[p])
            return 1;
        else
            return 0;
    }
    return 0;
}

// True if player p is pressing pause this frame.
int InputPause(int p)
{
    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.pause[p]) == true || PAD(p, PAD_PAUSE))
            return 1;
        else
            return 0;
    }
    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetFlagMask(0) & g_cfg.joyPause[p])
            return 1;
        else
            return 0;
    }
    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetFlagMask(1) & g_cfg.joyPause[p])
            return 1;
        else
            return 0;
    }
    return 0;
}

// True if player p is pressing the profile-window key this frame.
int InputProfile(int p)
{
    if (g_save.players[p].inputDevice == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.profile[p]) == true || PAD(p, PAD_PROFILE))
            return 1;
        else
            return 0;
    }
    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK1) {
        if (GetFlagMask(0) & g_cfg.joyProfile[p])
            return 1;
        else
            return 0;
    }
    if (g_save.players[p].inputDevice == DEVICE_JOYSTICK2) {
        if (GetFlagMask(1) & g_cfg.joyProfile[p])
            return 1;
        else
            return 0;
    }
    return 0;
}

// True if player p is pressing fire on a menu screen (uses g_cfg.device, not the
// per-game inputDevice); in autoplay it fires 90% of the time once autoplayCanFire is set.
int InputMenuFire(int p)
{
    if (g_autoplay && g_autoplayCanFire && RandRange(0, 100) < 90)
        return 1;
    if (g_cfg.device[p] == DEVICE_KEYBOARD) {
        if (KeyDown((enum EKeyboardLayout)g_cfg.fire[p]) == true || PAD(p, PAD_FIRE))
            return 1;
        else
            return 0;
    }
    if (g_cfg.device[p] == DEVICE_JOYSTICK1) {
        if (GetFlagMask(0) & g_cfg.joyFire[p])
            return 1;
        else
            return 0;
    }
    if (g_cfg.device[p] == DEVICE_JOYSTICK2) {
        if (GetFlagMask(1) & g_cfg.joyFire[p])
            return 1;
        else
            return 0;
    }
    return 0;
}
