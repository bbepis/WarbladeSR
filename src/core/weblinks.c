// weblinks.c: Opening web links from the game. The original sites are long gone, so every
// link goes to a Wayback Machine snapshot of the page instead (full timestamps, so the
// archive doesn't redirect).
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <strings.h>
#define _strnicmp strncasecmp
#endif
#include "globals.h"
#include "game.h"

typedef struct ArchivedUrl {
    const char *original;   // normalised: no scheme, no "www.", no trailing '/', lower case
    const char *archive;
} ArchivedUrl;

static const ArchivedUrl s_archivedUrls[] = {
    { "warblade.as",             "https://web.archive.org/web/20061230082208/http://www.warblade.as:80/" },
    { "warblade.as/faq.asp",     "https://web.archive.org/web/20061231212123/http://www.warblade.as:80/faq.asp" },
    { "warblade.as/manual.txt",  "https://web.archive.org/web/20080402231704/http://www.warblade.as:80/manual.txt" },
    { "warblade.as/help.asp",    "https://web.archive.org/web/20061230151009/http://www.warblade.as:80/help.asp" },
    { "warblade.as/halloffame.asp", "https://web.archive.org/web/20061230083019/http://www.warblade.as:80/halloffame.asp?" },
    { "warblade.as/gamenews.asp", "https://web.archive.org/web/20051228161911/http://www.warblade.as:80/gamenews.asp" },
    { "groovyaudio.com",         "https://web.archive.org/web/20061230021822/http://www.groovyaudio.com:80/" },
    { "karthesios.tripod.com",   "https://web.archive.org/web/20030227224434/http://karthesios.tripod.com:80/" },
    { "sbelectronics.com.au",    "https://web.archive.org/web/20080829054615/http://www.sbelectronics.com.au/" },
};

// The archived address for `url`: looked up without scheme, "www.", query string and trailing
// '/'. Any other warblade.as page goes to the site's front page; unknown sites are unchanged.
const char *ArchiveUrl(const char *url)
{
    char key[256];
    const char *p = url;
    int n = 0;
    int i;

    if (_strnicmp(p, "http://", 7) == 0)
        p += 7;
    else if (_strnicmp(p, "https://", 8) == 0)
        p += 8;
    if (_strnicmp(p, "www.", 4) == 0)
        p += 4;
    while (*p && *p != '?' && n < (int)sizeof(key) - 1)
        key[n++] = (char)tolower((unsigned char)*p++);
    while (n > 0 && key[n - 1] == '/')
        n--;
    key[n] = 0;

    for (i = 0; i < (int)(sizeof(s_archivedUrls) / sizeof(s_archivedUrls[0])); i++) {
        if (strcmp(key, s_archivedUrls[i].original) == 0)
            return s_archivedUrls[i].archive;
    }
    if (strncmp(key, "warblade.as", 11) == 0)
        return s_archivedUrls[0].archive;
    return url;
}

// Closes the game UI, flushes hiscores, then launches `url` (its archived snapshot) in the
// default browser and minimizes the game window.
void OpenUrl(const char *url)
{
    g_clickWin = -1;
    g_clickItem = -1;
    WinCloseAll();
    WriteHiscoreFile();
    ClearHiscores();
    SysOpenUrl(ArchiveUrl(url));
    SysMinimize();
}

// No-op hook, called before following a URL/link from the UI.
void BeforeOpenLink()
{
}
