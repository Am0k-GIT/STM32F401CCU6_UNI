/*
  fs_fatfs.c - VFS mount for FatFs

  Part of grblHAL

  Copyright (c) 2022 Terje Io

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined(ARDUINO)
#include "../driver.h"
#include "../grbl/platform.h"
#include "../grbl/vfs.h"
#else
#include "driver.h"
#include "grbl/platform.h"
#include "grbl/vfs.h"
#endif

#if SDCARD_ENABLE

#if defined(ESP_PLATFORM)
#include "esp_vfs_fat.h"
#elif defined(__LPC176x__) || defined(__MSP432E401Y__)
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#elif defined(ARDUINO_SAMD_MKRZERO)
#include "../../ff.h"
#include "../../diskio.h"
#elif defined(STM32_PLATFORM) || defined(__LPC17XX__) || defined(__IMXRT1062__) || defined(RP2040)
#include "ff.h"
#include "diskio.h"
#else
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef ESP_PLATFORM
#define FF_DIR DIR
#endif

#if FF_USE_LFN
//#define _USE_LFN FF_USE_LFN
#define _MAX_LFN FF_MAX_LFN
#endif

static inline char *get_name (FILINFO *file)
{
#if _USE_LFN
    return *file->lfname == '\0' ? file->fname : file->lfname;
#else
    return file->fname;
#endif
}

static vfs_file_t *fs_open (const char *filename, const char *mode)
{
    BYTE flags = 0;
    vfs_file_t *file = malloc(sizeof(vfs_file_t) + sizeof(FIL));

    if(file) {

        while (*mode != '\0') {
            if (*mode == 'r')
                flags |= FA_READ;
            else if (*mode == 'w')
                flags |= FA_WRITE | FA_CREATE_ALWAYS;
            mode++;
        }

        if((vfs_errno = f_open((FIL *)&file->handle, filename, flags)) != FR_OK) {
            free(file);
            file = NULL;
        } else
            file->size = f_size((FIL *)&file->handle);
    }

    return file;
}

static void fs_close (vfs_file_t *file)
{
    f_close((FIL *)&file->handle);
    free(file);
}

static size_t fs_read (void *buffer, size_t size, size_t count, vfs_file_t *file)
{
    UINT bytesread;
    vfs_errno = f_read((FIL *)&file->handle, buffer, size * count, &bytesread);

    return (size_t)bytesread;
}

static size_t fs_write (const void *buffer, size_t size, size_t count, vfs_file_t *file)
{
    UINT byteswritten = 0;

#if FF_FS_READONLY == 0
    if ((vfs_errno = f_write((FIL *)&file->handle, buffer, size * count, &byteswritten)) != FR_OK)
        byteswritten = 0;
#endif

    return byteswritten;
}

static size_t fs_tell (vfs_file_t *file)
{
    return f_tell((FIL *)&file->handle);
}

static int fs_seek (vfs_file_t *file, size_t offset)
{
    return f_lseek((FIL *)&file->handle, offset);
}

static bool fs_eof (vfs_file_t *file)
{
    return f_eof((FIL *)&file->handle) != 0;
}

static int fs_rename (const char *from, const char *to)
{
#if FF_FS_READONLY
    return -1;
#else
    return f_rename(from, to);
#endif
}

static int fs_unlink (const char *filename)
{
#if FF_FS_READONLY
    return -1;
#else
    return f_unlink(filename);
#endif
}

static int fs_mkdir (const char *path)
{
#if FF_FS_READONLY
    return -1;
#else
    return f_mkdir(path);
#endif
}

static int fs_chdir (const char *path)
{
#if FF_FS_RPATH
    return f_chdir(path);
#else
    return -1;
#endif
}

static char *fs_getcwd (char *buf, size_t size)
{
    static char cwd[255];
#if FF_FS_RPATH
    if ((vfs_errno = f_getcwd(cwd, 255)) == FR_OK) {
        char *s1, *s2;
        // Strip drive information
        if((s2 = strchr(cwd, ':'))) {
            s1 = cwd;
            s2++;
            while(*s2)
                *s1++ = *s2++;
            *s1 = '\0';
        }
    }
#else
    *cwd = '\0'; // TODO: return mount path?
#endif

    return cwd;
}

static vfs_dir_t *fs_opendir (const char *path)
{
    vfs_dir_t *dir = malloc(sizeof(vfs_dir_t) + sizeof(FF_DIR));

    if (dir && (vfs_errno = f_opendir((FF_DIR *)&dir->handle, path)) != FR_OK)
    {
        free(dir);
        dir = NULL;
    }

    return dir;
}

static char *fs_readdir (vfs_dir_t *dir, vfs_dirent_t *dirent)
{
    static FILINFO fi;

#if _USE_LFN
    fi.lfname = NULL;
#endif
#if x_USE_LFN
    static TCHAR lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    *dirent->name = '\0';

    if ((vfs_errno = f_readdir((FF_DIR *)&dir->handle, &fi)) != FR_OK || *fi.fname == '\0')
        return NULL;

    if(!strcmp(fi.fname, "System Volume Information") && ((vfs_errno = f_readdir((FF_DIR *)&dir->handle, &fi)) != FR_OK || *fi.fname == '\0'))
        return NULL;

    if(fi.fname && *fi.fname != '\0')
        strcpy(dirent->name, fi.fname);

    dirent->size = fi.fsize;
    dirent->st_mode.mode = fi.fattrib;

    return fi.fname;
}

static void fs_closedir (vfs_dir_t *dir)
{
    if (dir) {
        vfs_errno = f_closedir((FF_DIR *)&dir->handle);
//#if defined(__MSP432E401Y__) || defined(ESP_PLATFORM)
//    f_closedir(&dir);
//#endif

        free(dir);
    }
}

static int fs_stat (const char *filename, vfs_stat_t *st)
{
    FILINFO f;
#if _USE_LFN
    f.lfname = NULL;
#endif

    if ((vfs_errno = f_stat(filename, &f)) == FR_OK) {
        st->st_size = f.fsize;
        st->st_mode.mode = f.fattrib;
        struct tm tm  = {
            .tm_sec  = (f.ftime & 0x1f) << 1,
            .tm_min  = (f.ftime >> 5) & 0x3f,
            .tm_hour = (f.ftime >> 11) & 0x1f,
            .tm_mday = f.fdate & 0x1f,
            .tm_mon  = ((f.fdate >> 5) & 0xf) - 1,
            .tm_year = 80 + ((f.fdate >> 9) & 0x7f),
        };
#ifdef ESP_PLATFORM
        st->st_mtim = mktime(&tm);
#else
        st->st_mtime = mktime(&tm);
#endif
    } else
        return -1;

    return 0;
}

static int fs_utime (const char *filename, struct tm *modified)
{
#if FF_FS_READONLY == 0 && FF_USE_CHMOD == 1

    FILINFO fno;

    fno.fdate = (WORD)(((modified->tm_year - 80) * 512U) | (modified->tm_mon + 1) * 32U | modified->tm_mday);
    fno.ftime = (WORD)(modified->tm_hour * 2048U | modified->tm_min * 32U | modified->tm_sec / 2U);

    return f_utime(filename, &fno);
#else
    return -1;
#endif
}

static bool fs_getfree (vfs_free_t *free)
{
#if FF_FS_READONLY
    return false;
#else
    FATFS *fs;
    DWORD fre_clust, tot_sect;

    if((vfs_errno = f_getfree("", &fre_clust, &fs)) == FR_OK) {
        tot_sect = (fs->n_fatent - 2) * fs->csize;
        free->size = tot_sect << 9; // assuming 512 byte sector size
        free->used = (tot_sect - fre_clust * fs->csize) << 9;
    }

    return vfs_errno == FR_OK;
#endif
}

#if FF_FS_READONLY == 0 && FF_USE_MKFS == 1
static int fs_format (void)
{
    void *work = malloc(FF_MAX_SS);

    FRESULT res = f_mkfs("/", FM_ANY, 0, work, work ? FF_MAX_SS : 0);

    if(work)
        free(work);

    return res;
}
#endif

void fs_fatfs_mount (const char *path)
{
    static const vfs_t fs = {
        .fs_name = "FatFs",
#if FF_FS_READONLY
        .mode.read_only = true,
#endif
        .fopen = fs_open,
        .fclose = fs_close,
        .fread = fs_read,
        .fwrite = fs_write,
        .ftell = fs_tell,
        .fseek = fs_seek,
        .feof = fs_eof,
        .frename = fs_rename,
        .funlink = fs_unlink,
        .fmkdir = fs_mkdir,
        .fchdir = fs_chdir,
        .frmdir = fs_unlink,
        .fopendir = fs_opendir,
        .readdir = fs_readdir,
        .fclosedir = fs_closedir,
        .fstat = fs_stat,
        .futime = fs_utime,
        .fgetcwd = fs_getcwd,
        .fgetfree = fs_getfree,
#if FF_FS_READONLY == 0 && FF_USE_MKFS == 1
        .format = fs_format
#endif
    };

    vfs_mount(path, &fs);
}

#endif
