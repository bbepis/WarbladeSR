// engine_input.c: The real engine's (src/core/sdl.c) input: KeyDown's PTK key table, the mouse
// in back-buffer coordinates and joysticks. Keys and the mouse are fed through SDL's own
// (internal) event functions, as a video driver would; joysticks are SDL virtual joysticks.
#include "engine_util.h"

// The scancode each PTK key reads on a US layout, in key-table order.
static const SDL_Scancode s_us[K_VK_ERROR] = {
    SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE,
    SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RETURN, SDL_SCANCODE_RCTRL,
    SDL_SCANCODE_LCTRL,
    SDL_SCANCODE_F1, SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, SDL_SCANCODE_F5,
    SDL_SCANCODE_F6, SDL_SCANCODE_F7, SDL_SCANCODE_F8, SDL_SCANCODE_F9, SDL_SCANCODE_F10,
    SDL_SCANCODE_F11, SDL_SCANCODE_F12,
    SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_TAB, SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_A, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_D, SDL_SCANCODE_E,
    SDL_SCANCODE_F, SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_I, SDL_SCANCODE_J,
    SDL_SCANCODE_K, SDL_SCANCODE_L, SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O,
    SDL_SCANCODE_P, SDL_SCANCODE_Q, SDL_SCANCODE_R, SDL_SCANCODE_S, SDL_SCANCODE_T,
    SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X, SDL_SCANCODE_Y,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_0, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9,
    SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_9,
    SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_MINUS,
    SDL_SCANCODE_KP_PERIOD, SDL_SCANCODE_KP_DIVIDE,
    SDL_SCANCODE_CLEAR, SDL_SCANCODE_LALT, SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI,
    SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_SCROLLLOCK,
    SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_EQUALS, SDL_SCANCODE_COMMA, SDL_SCANCODE_MINUS,
    SDL_SCANCODE_PERIOD, SDL_SCANCODE_SLASH, SDL_SCANCODE_GRAVE, SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_BACKSLASH, SDL_SCANCODE_RIGHTBRACKET, SDL_SCANCODE_APOSTROPHE,
    SDL_SCANCODE_END, SDL_SCANCODE_HOME, SDL_SCANCODE_DELETE, SDL_SCANCODE_INSERT,
    SDL_SCANCODE_PRINTSCREEN, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_PAGEDOWN,
};

static void Key(SDL_Scancode sc, bool down)
{
    SDL_SendKeyboardKey(0, 0, 0, sc, down);
}

// The PTK keys that read as down, as a list "k1 k2 ...".
static const char *KeysDown(void)
{
    static char buf[512];
    int k, n = 0;
    buf[0] = 0;
    for (k = 0; k < K_VK_ERROR; k++)
        if (KeyDown((EKeyboardLayout)k))
            n += snprintf(buf + n, sizeof buf - n, "%s%d", n ? " " : "", k);
    return buf;
}

TEST(engine_KeyDown_reads_every_key_of_the_table)
{
    char want[16];
    int k;
    OpenWindow(64, 48);
    CHECK_STR(KeysDown(), "");
    for (k = 0; k < K_VK_ERROR; k++) {
        if (k == K_VK_MENU)                 // Left Alt; engine_KeyDown_menu_is_either_alt
            continue;
        Key(s_us[k], true);
        snprintf(want, sizeof want, "%d", k);
        CHECK_MSG(strcmp(KeysDown(), want) == 0, "scancode %d (%s) reads as keys \"%s\", expected %d",
                  s_us[k], SDL_GetScancodeName(s_us[k]), KeysDown(), k);
        Key(s_us[k], false);
    }
}

TEST(engine_KeyDown_menu_is_either_alt)
{
    OpenWindow(64, 48);
    Key(SDL_SCANCODE_LALT, true);
    CHECK_STR(KeysDown(), "77");
    Key(SDL_SCANCODE_LALT, false);
    Key(SDL_SCANCODE_RALT, true);
    CHECK_STR(KeysDown(), "77");
    Key(SDL_SCANCODE_RALT, false);
    CHECK_STR(KeysDown(), "");
}

TEST(engine_KeyDown_return_includes_keypad_enter)
{
    OpenWindow(64, 48);
    Key(SDL_SCANCODE_KP_ENTER, true);
    CHECK_STR(KeysDown(), "7");
    Key(SDL_SCANCODE_KP_ENTER, false);
    CHECK_STR(KeysDown(), "");
}

TEST(engine_KeyDown_is_false_outside_the_table)
{
    OpenWindow(64, 48);
    Key(SDL_SCANCODE_LEFT, true);
    Key(SDL_SCANCODE_A, true);
    CHECK(KeyDown(K_VK_LEFT));
    CHECK(!KeyDown(K_VK_ERROR));
    CHECK(!KeyDown((EKeyboardLayout)-1));
    CHECK(!KeyDown((EKeyboardLayout)1000));
}

TEST(engine_KeyDown_follows_the_keyboard_layout)
{
    SDL_Keymap *km;
    OpenWindow(64, 48);
    // AZERTY: the key in QWERTY's A place types Q, and so on; ';' is on the M key.
    km = SDL_CreateKeymap(false);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_A, SDL_KMOD_NONE, SDLK_Q);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_Q, SDL_KMOD_NONE, SDLK_A);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_W, SDL_KMOD_NONE, SDLK_Z);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_Z, SDL_KMOD_NONE, SDLK_W);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_M, SDL_KMOD_NONE, SDLK_SEMICOLON);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_SEMICOLON, SDL_KMOD_NONE, SDLK_M);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_1, SDL_KMOD_NONE, SDLK_7);
    SDL_SetKeymapEntry(km, SDL_SCANCODE_7, SDL_KMOD_NONE, SDLK_1);
    SDL_SetKeymap(km, false);

    Key(SDL_SCANCODE_Q, true);
    CHECK(KeyDown(K_VK_A));
    CHECK(!KeyDown(K_VK_Q));
    Key(SDL_SCANCODE_Q, false);
    Key(SDL_SCANCODE_W, true);
    CHECK(KeyDown(K_VK_Z));
    Key(SDL_SCANCODE_W, false);
    Key(SDL_SCANCODE_M, true);
    CHECK(KeyDown(K_VK_OEM_1));
    CHECK(!KeyDown(K_VK_M));
    Key(SDL_SCANCODE_M, false);
    Key(SDL_SCANCODE_1, true);
    CHECK(KeyDown(K_VK_7));
    Key(SDL_SCANCODE_1, false);
    // The other keys are by position.
    Key(SDL_SCANCODE_UP, true);
    CHECK(KeyDown(K_VK_UP));
    Key(SDL_SCANCODE_UP, false);
    Key(SDL_SCANCODE_KP_1, true);
    CHECK(KeyDown(K_VK_NUM1));
}

// ---------------------------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------------------------

// The window resized; the flips let the software renderer take the new size.
static void Resize(int w, int h)
{
    SysFlip();
    SDL_SetWindowSize(TheWindow(), w, h);
    SysFlip();
}

// The software renderer reports the render target's size as its output size, and between flips
// the target is the back buffer; GPU renderers report the window's. These read the mouse with
// the window as the target, as the engine sees it on those.
static SDL_Texture *WindowTarget(void)
{
    SDL_Texture *t = SDL_GetRenderTarget(TheRenderer());
    SDL_SetRenderTarget(TheRenderer(), NULL);
    return t;
}

static int MX(void)
{
    SDL_Texture *t = WindowTarget();
    int v = MouseX();
    SDL_SetRenderTarget(TheRenderer(), t);
    return v;
}

static int MY(void)
{
    SDL_Texture *t = WindowTarget();
    int v = MouseY();
    SDL_SetRenderTarget(TheRenderer(), t);
    return v;
}

static void Warp(int x, int y)
{
    SDL_Texture *t = WindowTarget();
    MouseWarp(x, y);
    SDL_SetRenderTarget(TheRenderer(), t);
}

static void MouseAt(float x, float y)
{
    SDL_SendMouseMotion(0, TheWindow(), 0, false, x, y);
}

TEST(engine_Mouse_reports_back_buffer_coordinates)
{
    OpenWindow(64, 48);
    MouseAt(10.5f, 20.7f);
    CHECK_EQ_INT(MX(), 10);
    CHECK_EQ_INT(MY(), 20);
    // Letterboxed in a wider window: 32 pixels of bar on the left.
    Resize(128, 48);
    MouseAt(40, 10);
    CHECK_EQ_INT(MX(), 8);
    CHECK_EQ_INT(MY(), 10);
    MouseAt(10.5f, 10);
    CHECK_EQ_INT(MX(), -22);                // -21.5, rounded down
    MouseAt(127, 47);
    CHECK_EQ_INT(MX(), 95);
    // Twice the size, centred: 16 pixels of bar on top.
    Resize(128, 128);
    MouseAt(65, 50);
    CHECK_EQ_INT(MX(), 32);
    CHECK_EQ_INT(MY(), 17);
    MouseAt(65, 10);
    CHECK_EQ_INT(MY(), -3);
}

// What the engine does with the software renderer itself (see WindowTarget): the window's
// letterbox is missed, and the mouse reads in window pixels.
TEST(engine_Mouse_with_the_software_renderer_misses_the_letterbox)
{
    OpenWindow(64, 48);
    Resize(128, 48);
    MouseAt(40, 10);
    CHECK_EQ_INT(MouseX(), 40);
    CHECK_EQ_INT(MX(), 8);
}

TEST(engine_MouseWarp_takes_back_buffer_coordinates)
{
    float wx = 0, wy = 0;
    OpenWindow(64, 48);
    Resize(128, 128);
    Warp(10, 20);
    SDL_GetMouseState(&wx, &wy);
    CHECK_NEAR(wx, 20, 0.01);
    CHECK_NEAR(wy, 56, 0.01);
    CHECK_EQ_INT(MX(), 10);
    CHECK_EQ_INT(MY(), 20);
}

TEST(engine_Mouse_buttons)
{
    OpenWindow(64, 48);
    MouseAt(5, 5);
    CHECK(!MouseLeft());
    CHECK(!MouseRight());
    SDL_SendMouseButton(0, TheWindow(), 0, SDL_BUTTON_LEFT, true);
    CHECK(MouseLeft());
    CHECK(!MouseRight());
    SDL_SendMouseButton(0, TheWindow(), 0, SDL_BUTTON_RIGHT, true);
    SDL_SendMouseButton(0, TheWindow(), 0, SDL_BUTTON_LEFT, false);
    CHECK(!MouseLeft());
    CHECK(MouseRight());
}

// ---------------------------------------------------------------------------------------------
// Joysticks
// ---------------------------------------------------------------------------------------------

TEST(engine_Joy_without_joysticks)
{
    InitVideo();
    CHECK(!JoyEnable(0));
    CHECK(!JoyEnable(1));
    CHECK(!JoyEnable(2));
    CHECK(!JoyEnable(-1));
    CHECK_EQ_INT(JoyX(0, 0), 0x7fff);
    CHECK_EQ_INT(JoyY(0, 0), 0x7fff);
    CHECK_EQ_INT(JoyX(0, 1), 0x7fff);
    CHECK_EQ_INT(JoyX(5, 0), 0x7fff);
    CHECK(!JoyButton(0, 1));
    CHECK(!JoyButton(-1, 1));
}

TEST(engine_HidePointer_and_ShowPointer)
{
    OpenWindow(64, 48);
    CHECK(SDL_CursorVisible());
    HidePointer();
    CHECK(!SDL_CursorVisible());
    ShowPointer();
    CHECK(SDL_CursorVisible());
}
