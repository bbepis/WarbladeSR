// resources.c: Loading graphics (Image) and .hma masks.
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include "globals.h"
#include "game.h"


// Loads `name` (lowercased into g_gfxPath) as a picture with the given alpha-mask/quality
// flags, and enables texture quality + alpha blending. Returns null and logs an error if
// loading fails.
Image *LoadGraphic(char *name, bool maskAlpha, bool hiQuality)
{
    Image *gfx;
    StrToLowerN(name, g_gfxPath, 1024);
    gfx = ImgLoad(g_gfxPath, maskAlpha, hiQuality);
    if (gfx == 0) {
        sprintf(g_msgBuf, "ERROR: Could not load %s file\n", name);
        LogPrint(g_msgBuf);
        return 0;
    }
    ImgSetTextureQuality(gfx, true);
    ImgSetAlphaMode(gfx, 1);
    return gfx;
}

// Same as LoadGraphic() but leaves alpha blending mode untouched (used for graphics
// that don't need setAlphaMode()).
Image *LoadGraphic2(char *name, bool maskAlpha, bool hiQuality)
{
    Image *gfx;
    StrToLowerN(name, g_gfxPath, 1024);
    gfx = ImgLoad(g_gfxPath, maskAlpha, hiQuality);
    if (gfx == 0) {
        sprintf(g_msgBuf, "ERROR: Could not load %s file\n", name);
        LogPrint(g_msgBuf);
        return 0;
    }
    ImgSetTextureQuality(gfx, true);
    return gfx;
}

// Loads a raw `w`x`h` (1 byte/pixel) image from `<name>.hma`. Returns a malloc'd buffer
// the caller owns, or NULL if the file can't be opened.
void *LoadHma(const char *name, int w, int h)
{
    char levelname[512];
    void *buf;
    size_t nameLen;
    size_t maskSize;

    if (!name || w <= 0 || h <= 0 || (size_t)w > SIZE_MAX / (size_t)h)
        return NULL;
    maskSize = (size_t)w * (size_t)h;
    if (maskSize > UINT_MAX)
        return NULL;
    nameLen = strlen(name);
    if (nameLen > sizeof(levelname) - sizeof(".hma"))
        return NULL;

    buf = calloc(maskSize, 1);
    if (buf) {
        StrToLowerN(name, levelname, (int)sizeof(levelname));
        memcpy(levelname + nameLen, ".hma", sizeof(".hma"));
        if (PacRead(levelname, buf, (unsigned)maskSize)) {
            return buf;
        } else {
            free(buf);
            return NULL;
        }
    }
    return NULL;
}
