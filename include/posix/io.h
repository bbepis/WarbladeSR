#pragma once
// io.h for builds other than Windows (CMakeLists.txt puts include/posix on the include path
// there only). The game uses the Microsoft C runtime's low-level file calls with Windows paths
// and MSVC's flag values (some written as numbers, e.g. 0x302); these map them onto POSIX,
// with the path going through SysPath.
#include <fcntl.h>
#include <stdio.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

const char *SysPath(const char *path);

// MSVC's values (the game's numeric flags assume them).
#define _O_RDONLY 0x0000
#define _O_WRONLY 0x0001
#define _O_RDWR   0x0002
#define _O_APPEND 0x0008
#define _O_CREAT  0x0100
#define _O_TRUNC  0x0200
#define _O_EXCL   0x0400
#define _O_TEXT   0x4000
#define _O_BINARY 0x8000
#define _S_IREAD  0x0100
#define _S_IWRITE 0x0080

static inline int CompatOpen(const char *path, int msFlags, int msMode)
{
    int flags = (msFlags & 3) == _O_WRONLY ? O_WRONLY : (msFlags & 3) == _O_RDWR ? O_RDWR : O_RDONLY;
    mode_t mode = 0644;
    if (msFlags & _O_APPEND) flags |= O_APPEND;
    if (msFlags & _O_CREAT)  flags |= O_CREAT;
    if (msFlags & _O_TRUNC)  flags |= O_TRUNC;
    if (msFlags & _O_EXCL)   flags |= O_EXCL;
    if (!(msMode & _S_IWRITE) && (msFlags & _O_CREAT))
        mode = 0444;
    return open(SysPath(path), flags, mode);
}

static inline long CompatFileLength(int fd)
{
    struct stat st;
    return fstat(fd, &st) == 0 ? (long)st.st_size : -1L;
}

static inline int CompatRemove(const char *path) { return remove(SysPath(path)); }
static inline int CompatRename(const char *from, const char *to)
{
    char f[1024];
    snprintf(f, sizeof(f), "%s", SysPath(from));
    return rename(f, SysPath(to));
}

#define _open(path, flags, mode) CompatOpen(path, flags, mode)
#define _read(fd, buf, n)        ((int)read(fd, buf, n))
#define _write(fd, buf, n)       ((int)write(fd, buf, n))
#define _close(fd)               close(fd)
#define _lseek(fd, off, whence)  ((long)lseek(fd, off, whence))
#define _filelength(fd)          CompatFileLength(fd)
#define _set_fmode(mode)         ((void)(mode), 0)   // files are always binary here
#define _strnicmp(a, b, n)       strncasecmp(a, b, n)
#define remove(path)             CompatRemove(path)
#define rename(from, to)         CompatRename(from, to)
