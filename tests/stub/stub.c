// A stand-in for the game in the browser tests (tests/README.md). Built with the same
// Emscripten options as warblade.js (CMakeLists.txt), it lets the tests run the page's real
// file-system and IndexedDB code without the Warblade data.
//
// Like the game, it reads data/warblade.pac and keeps its files in /save/warblade: it counts
// its runs in a profile file, prints what it found, and keeps running until the page closes.
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <emscripten.h>

#define PROFILE "/save/warblade/profiles/profile000.acc"

static int s_settingsWrites;

// The page calls this when it's hidden or closed, and every few seconds (web/page.js).
EMSCRIPTEN_KEEPALIVE void WebSaveSettings(void)
{
    FILE *f = fopen("/save/warblade/WarBlade.inf", "w");
    if (f) {
        fprintf(f, "settings %d\n", ++s_settingsWrites);
        fclose(f);
    }
}

EMSCRIPTEN_KEEPALIVE int WebReloadHiscores(void)
{
    return 1;
}

EMSCRIPTEN_KEEPALIVE void SysSetVirtualPad(int bits)
{
    (void)bits;
}

static char username[33], password[257];
static int queued, createAccount, authenticated;
EM_ASYNC_JS(int, authenticate, (const char *name, const char *secret, int create), {
    return await Module.authenticateGame(UTF8ToString(name), UTF8ToString(secret), !!create) ? 1 : 0;
});
EMSCRIPTEN_KEEPALIVE int WebQueueCredentials(const char *name, const char *secret, int create)
{
    if (queued || authenticated) return 0;
    snprintf(username, sizeof(username), "%s", name);
    snprintf(password, sizeof(password), "%s", secret);
    createAccount = create;
    queued = 1;
    return 1;
}
EMSCRIPTEN_KEEPALIVE int WebGameReady(void) { return 1; }
EMSCRIPTEN_KEEPALIVE int WebAccountStatus(void) { return authenticated; }

int main(void)
{
    int restored = EM_ASM_INT({ return !!Module.accountName; });
    for (;;) {
        if (queued || restored) {
            queued = 0;
            authenticated = restored ? 1 : authenticate(username, password, createAccount);
            restored = 0;
            memset(password, 0, sizeof(password));
            if (authenticated) {
                int runs = 0;
                FILE *f = fopen(PROFILE, "r");
                if (f) { fscanf(f, "%d", &runs); fclose(f); }
                mkdir("/save/warblade", 0755);
                mkdir("/save/warblade/profiles", 0755);
                f = fopen(PROFILE, "w");
                if (f) { fprintf(f, "%d\n", runs + 1); fclose(f); }
            }
        }
        emscripten_sleep(100);
    }
}
