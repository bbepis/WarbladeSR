// engine_gamepad.c: The real SDL gamepad adapter in src/core/sdl.c, driven by virtual pads.
#include "engine_util.h"

static SDL_JoystickID AttachGamepad(void)
{
    SDL_VirtualJoystickDesc desc;
    SDL_JoystickID id;

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
    desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    desc.name = "Warblade test gamepad";
    id = SDL_AttachVirtualJoystick(&desc);
    CHECK_MSG(id != 0, "SDL_AttachVirtualJoystick: %s", SDL_GetError());
    return id;
}

static SDL_Joystick *OpenVirtualJoystick(SDL_JoystickID id)
{
    SDL_Joystick *joystick = SDL_OpenJoystick(id);
    CHECK_MSG(joystick != NULL, "SDL_OpenJoystick: %s", SDL_GetError());
    return joystick;
}

static void PumpGamepadEvents(void)
{
    SDL_PumpEvents();
    SysProcessEvents();
}

static void Axis(SDL_Joystick *joystick, SDL_GamepadAxis axis, Sint16 value)
{
    CHECK(SDL_SetJoystickVirtualAxis(joystick, axis, value));
    PumpGamepadEvents();
}

static void Button(SDL_Joystick *joystick, SDL_GamepadButton button, bool down)
{
    CHECK(SDL_SetJoystickVirtualButton(joystick, button, down));
    PumpGamepadEvents();
}

static void DetachGamepad(SDL_JoystickID id, SDL_Joystick *joystick)
{
    SDL_CloseJoystick(joystick);
    CHECK(SDL_DetachVirtualJoystick(id));
    PumpGamepadEvents();
}

TEST(engine_SysPadState_applies_stick_dead_zones_to_each_axis)
{
    SDL_JoystickID id;
    SDL_Joystick *joystick;

    OpenWindow(64, 48);
    id = AttachGamepad();
    PumpGamepadEvents();
    joystick = OpenVirtualJoystick(id);
    CHECK_EQ_INT(SysPadState(0), 0);

    Axis(joystick, SDL_GAMEPAD_AXIS_LEFTX, -12000);
    CHECK_EQ_INT(SysPadState(0), 0);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFTX, -12001);
    CHECK_EQ_INT(SysPadState(0), PAD_LEFT);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 12000);
    CHECK_EQ_INT(SysPadState(0), 0);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 12001);
    CHECK_EQ_INT(SysPadState(0), PAD_RIGHT);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 0);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFTY, -12001);
    CHECK_EQ_INT(SysPadState(0), PAD_UP);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFTY, 12001);
    CHECK_EQ_INT(SysPadState(0), PAD_DOWN);
    DetachGamepad(id, joystick);
}

TEST(engine_SysPadState_maps_dpad_actions_triggers_start_and_back)
{
    SDL_JoystickID id;
    SDL_Joystick *joystick;

    OpenWindow(64, 48);
    id = AttachGamepad();
    PumpGamepadEvents();
    joystick = OpenVirtualJoystick(id);

    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    CHECK_EQ_INT(SysPadState(0), PAD_LEFT);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    CHECK_EQ_INT(SysPadState(0), PAD_RIGHT);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, false);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    CHECK_EQ_INT(SysPadState(0), PAD_UP);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
    CHECK_EQ_INT(SysPadState(0), PAD_DOWN);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);

    Button(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    CHECK_EQ_INT(SysPadState(0), PAD_FIRE);
    Button(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false);
    Button(joystick, SDL_GAMEPAD_BUTTON_EAST, true);
    CHECK_EQ_INT(SysPadState(0), PAD_ROCKET);
    Button(joystick, SDL_GAMEPAD_BUTTON_EAST, false);
    Button(joystick, SDL_GAMEPAD_BUTTON_WEST, true);
    CHECK_EQ_INT(SysPadState(0), PAD_ROCKET);
    Button(joystick, SDL_GAMEPAD_BUTTON_WEST, false);
    Button(joystick, SDL_GAMEPAD_BUTTON_START, true);
    CHECK_EQ_INT(SysPadState(0), PAD_PAUSE);
    Button(joystick, SDL_GAMEPAD_BUTTON_START, false);
    Button(joystick, SDL_GAMEPAD_BUTTON_BACK, true);
    CHECK_EQ_INT(SysPadState(0), PAD_PROFILE);
    Button(joystick, SDL_GAMEPAD_BUTTON_BACK, false);

    Axis(joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
    CHECK_EQ_INT(SysPadState(0), PAD_FIRE);
    Axis(joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, -32768);
    CHECK_EQ_INT(SysPadState(0), 0);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767);
    CHECK_EQ_INT(SysPadState(0), PAD_ROCKET);
    Axis(joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, -32768);
    CHECK_EQ_INT(SysPadState(0), 0);
    DetachGamepad(id, joystick);
}

TEST(engine_SysPadState_routes_players_and_clears_disconnected_pads)
{
    SDL_JoystickID firstId, secondId, replacementId;
    SDL_Joystick *first, *second, *replacement;

    OpenWindow(64, 48);
    firstId = AttachGamepad();
    PumpGamepadEvents();
    first = OpenVirtualJoystick(firstId);
    Button(first, SDL_GAMEPAD_BUTTON_SOUTH, true);
    CHECK_EQ_INT(SysPadState(0), PAD_FIRE);

    secondId = AttachGamepad();
    PumpGamepadEvents();
    second = OpenVirtualJoystick(secondId);
    Button(second, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
    CHECK_EQ_INT(SysPadState(0), PAD_FIRE);
    CHECK_EQ_INT(SysPadState(1), PAD_RIGHT);

    DetachGamepad(firstId, first);
    CHECK_EQ_INT(SysPadState(0), 0);
    CHECK_EQ_INT(SysPadState(1), PAD_RIGHT);

    replacementId = AttachGamepad();
    PumpGamepadEvents();
    replacement = OpenVirtualJoystick(replacementId);
    Button(replacement, SDL_GAMEPAD_BUTTON_BACK, true);
    CHECK_EQ_INT(SysPadState(0), PAD_PROFILE);
    CHECK_EQ_INT(SysPadState(1), PAD_RIGHT);
    DetachGamepad(secondId, second);
    CHECK_EQ_INT(SysPadState(0), PAD_PROFILE);
    CHECK_EQ_INT(SysPadState(1), 0);
    DetachGamepad(replacementId, replacement);
    CHECK_EQ_INT(SysPadState(0), 0);
}

TEST(engine_SysPadState_ORs_touch_controls_with_player_one_gamepad)
{
    SDL_JoystickID id;
    SDL_Joystick *joystick;

    OpenWindow(64, 48);
    id = AttachGamepad();
    PumpGamepadEvents();
    joystick = OpenVirtualJoystick(id);

    SysSetVirtualPad(PAD_UP | PAD_FIRE);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    CHECK_EQ_INT(SysPadState(0), PAD_UP | PAD_LEFT | PAD_FIRE);
    CHECK_EQ_INT(SysPadState(1), 0);
    SysSetVirtualPad(0);
    CHECK_EQ_INT(SysPadState(0), PAD_LEFT);
    Button(joystick, SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
    DetachGamepad(id, joystick);
}
