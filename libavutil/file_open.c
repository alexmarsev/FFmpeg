/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config.h"
#include "internal.h"
#include "mem.h"
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_IO_H
#include <io.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32CE__)
#undef open
#undef lseek
#undef stat
#undef fstat
#include <windows.h>
#include <share.h>
#include <errno.h>

static int win32_open(const char *filename_utf8, int oflag, int pmode)
{
    int fd;
    int num_chars;
    wchar_t *filename_w;

    HANDLE file_handle;

    const DWORD flags_access = (oflag & O_RDONLY) ? GENERIC_READ :
                               (oflag & O_WRONLY) ? GENERIC_WRITE :
                               GENERIC_READ | GENERIC_READ;

    const DWORD flags_share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    const DWORD create_type = (oflag & (O_CREAT | O_TRUNC)) ? CREATE_ALWAYS :
                              (oflag & (O_CREAT | O_EXCL)) ? CREATE_NEW :
                              (oflag & O_TRUNC) ? TRUNCATE_EXISTING :
                              (oflag & O_CREAT) ? OPEN_ALWAYS :
                              OPEN_EXISTING;

    /* convert UTF-8 to wide chars */
    num_chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename_utf8, -1, NULL, 0);
    if (num_chars <= 0)
        goto fallback;
    filename_w = av_mallocz(sizeof(wchar_t) * num_chars);
    if (!filename_w) {
        errno = ENOMEM;
        return -1;
    }
    MultiByteToWideChar(CP_UTF8, 0, filename_utf8, -1, filename_w, num_chars);

    file_handle = CreateFileW(filename_w, flags_access, flags_share, 0, create_type, 0, 0);
    av_freep(&filename_w);
    fd = _open_osfhandle((intptr_t)file_handle, oflag);
    if (fd == -1)
        CloseHandle(file_handle);

    if (fd != -1 || (oflag & O_CREAT))
        return fd;

fallback:
    /* filename may be in CP_ACP */
    file_handle = CreateFileA(filename_utf8, flags_access, flags_share, 0, create_type, 0, 0);
    fd = _open_osfhandle((intptr_t)file_handle, oflag);
    if (fd == -1)
        CloseHandle(file_handle);
    return fd;
}
#define open win32_open
#endif

int avpriv_open(const char *filename, int flags, ...)
{
    int fd;
    unsigned int mode = 0;
    va_list ap;

    va_start(ap, flags);
    if (flags & O_CREAT)
        mode = va_arg(ap, unsigned int);
    va_end(ap);

#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif

    fd = open(filename, flags, mode);
#if HAVE_FCNTL
    if (fd != -1) {
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
            av_log(NULL, AV_LOG_DEBUG, "Failed to set close on exec\n");
    }
#endif

    return fd;
}

FILE *av_fopen_utf8(const char *path, const char *mode)
{
    int fd;
    int access;
    const char *m = mode;

    switch (*m++) {
    case 'r': access = O_RDONLY; break;
    case 'w': access = O_CREAT|O_WRONLY|O_TRUNC; break;
    case 'a': access = O_CREAT|O_WRONLY|O_APPEND; break;
    default :
        errno = EINVAL;
        return NULL;
    }
    while (*m) {
        if (*m == '+') {
            access &= ~(O_RDONLY | O_WRONLY);
            access |= O_RDWR;
        } else if (*m == 'b') {
#ifdef O_BINARY
            access |= O_BINARY;
#endif
        } else if (*m) {
            errno = EINVAL;
            return NULL;
        }
        m++;
    }
    fd = avpriv_open(path, access, 0666);
    if (fd == -1)
        return NULL;
    return fdopen(fd, mode);
}
