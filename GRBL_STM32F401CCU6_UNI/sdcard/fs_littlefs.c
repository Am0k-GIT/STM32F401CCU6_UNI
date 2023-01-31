/*
  fs_littlefs.c - VFS wrapper/mount for littlefs

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
#else
#include "driver.h"
#endif

#if LITTLEFS_ENABLE

#if defined(ARDUINO)
#include "../grbl/platform.h"
#include "../grbl/vfs.h"
#else
#include "../grbl/platform.h"
#include "../grbl/vfs.h"
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "../littlefs/lfs.h"
#include "../littlefs/lfs_util.h"

#define ATTR_TIMESTAMP 0x74 // 't'

typedef struct time_file {
    lfs_file_t file;
    bool modified;
    time_t timestamp;
    struct lfs_attr attrs[1];
    struct lfs_file_config cfg;
} time_file_t;

static lfs_t lfs = {0};
static const struct lfs_config *lfs_config;

static vfs_file_t *fs_open (const char *filename, const char *mode)
{
    int flags = 0;
    vfs_file_t *file = malloc(sizeof(vfs_file_t) + sizeof(time_file_t));

    if(file) {

        time_file_t *f = (time_file_t *)&file->handle;

        // set up description of timestamp attribute
        f->modified = false;
        f->timestamp = 0;
        f->attrs[0].type = ATTR_TIMESTAMP;
        f->attrs[0].buffer = &f->timestamp;
        f->attrs[0].size = sizeof(time_t);

        // set up config to indicate file has custom attributes
        memset(&f->cfg, 0, sizeof(struct lfs_file_config));
        f->cfg.attrs = f->attrs;
        f->cfg.attr_count = 1;

        while (*mode != '\0') {
            if (*mode == 'r')
                flags |= LFS_O_RDONLY;
            else if (*mode == 'w') {
                flags |= LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC;
                if(hal.rtc.get_datetime) {
                    struct tm dt;
                    if(hal.rtc.get_datetime(&dt))
                        f->timestamp = mktime(&dt);
                }
            } else if (*mode == 'a')
                flags |= LFS_O_APPEND;
            mode++;
        }

        if((vfs_errno = lfs_file_opencfg(&lfs, &f->file, filename, flags, &f->cfg)) != LFS_ERR_OK) {
            free(file);
            file = NULL;
        } else
            file->size = lfs_file_size(&lfs, &f->file);
    }

    return file;
}

static void fs_close (vfs_file_t *file)
{
    time_file_t *f = (time_file_t *)&file->handle;

    if(f->modified && hal.rtc.get_datetime) {
        struct tm dt;
        if(hal.rtc.get_datetime(&dt))
            f->timestamp = mktime(&dt);
    }

    lfs_file_close(&lfs, &f->file);
    free(file);
}

static size_t fs_read (void *buffer, size_t size, size_t count, vfs_file_t *file)
{
    return lfs_file_read(&lfs, &((time_file_t *)&file->handle)->file, buffer, size * count);
}

static size_t fs_write (const void *buffer, size_t size, size_t count, vfs_file_t *file)
{
    time_file_t *f = (time_file_t *)&file->handle;

    f->modified = true;

    return lfs_file_write(&lfs, &f->file, buffer, size * count);
}

static size_t fs_tell (vfs_file_t *file)
{
    return lfs_file_tell(&lfs, &((time_file_t *)&file->handle)->file);
}

static int fs_seek (vfs_file_t *file, size_t offset)
{
    return lfs_file_seek(&lfs, &((time_file_t *)&file->handle)->file, offset, LFS_SEEK_SET);
}

static bool fs_eof (vfs_file_t *file)
{
    return lfs_file_tell(&lfs, &((time_file_t *)&file->handle)->file) == file->size;
}

static int fs_rename (const char *from, const char *to)
{
    return lfs_rename(&lfs, from, to);
}

static int fs_unlink (const char *filename)
{
    return lfs_remove(&lfs, filename);
}

static int fs_mkdir (const char *path)
{
    int res;

    if((res = lfs_mkdir(&lfs, path)) == LFS_ERR_OK) {
        struct tm dt;
        if(hal.rtc.get_datetime && hal.rtc.get_datetime(&dt)) {
            time_t t = mktime(&dt);
            lfs_setattr(&lfs, path, ATTR_TIMESTAMP, &t, sizeof(time_t));
        }
    }

    return res;
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
    vfs_dir_t *dir = malloc(sizeof(vfs_dir_t) + sizeof(lfs_dir_t));

    if (dir && (vfs_errno = lfs_dir_open(&lfs, (lfs_dir_t *)&dir->handle, path)) != LFS_ERR_OK)
    {
        free(dir);
        dir = NULL;
    }

    return dir;
}

static char *fs_readdir (vfs_dir_t *dir, vfs_dirent_t *dirent)
{
    static struct lfs_info f;

    *dirent->name = '\0';

    if ((vfs_errno = lfs_dir_read(&lfs, (lfs_dir_t *)&dir->handle, &f)) <= 0)
        return NULL;

    if(!strcmp(f.name, ".") && (vfs_errno = lfs_dir_read(&lfs, (lfs_dir_t *)&dir->handle, &f)) <= 0)
        return NULL;

    if(!strcmp(f.name, "..") && (vfs_errno = lfs_dir_read(&lfs, (lfs_dir_t *)&dir->handle, &f)) <= 0)
        return NULL;

    if(f.name && *f.name != '\0')
        strcpy(dirent->name, f.name);

    vfs_errno = 0;
    dirent->size = f.size;
    dirent->st_mode.mode = 0;
    dirent->st_mode.directory = f.type == LFS_TYPE_DIR;

    return *f.name ? dirent->name : NULL;
}

static void fs_closedir (vfs_dir_t *dir)
{
    if (dir) {
        vfs_errno = lfs_dir_close(&lfs, (lfs_dir_t *)&dir->handle);
        free(dir);
    }
}

static int fs_stat (const char *filename, vfs_stat_t *st)
{
    struct lfs_info f;

    if ((vfs_errno = lfs_stat(&lfs, filename, &f)) == LFS_ERR_OK) {
        st->st_size = f.size;
        st->st_mode.mode = 0;
        st->st_mode.directory = f.type == LFS_TYPE_DIR;
#if ESP_PLATFORM
        if(lfs_getattr(&lfs, filename, ATTR_TIMESTAMP, &st->st_mtim, sizeof(time_t)) != sizeof(time_t))
            st->st_mtim = (time_t)0;
#else
        if(lfs_getattr(&lfs, filename, ATTR_TIMESTAMP, &st->st_mtime, sizeof(time_t)) != sizeof(time_t))
            st->st_mtime = (time_t)0;
#endif
    } else
        return -1;

    return 0;
}

static int fs_utime (const char *filename, struct tm *modified)
{
    time_t t = mktime(modified);

    return lfs_setattr(&lfs, filename, ATTR_TIMESTAMP, &t, sizeof(time_t));
}

static bool fs_getfree (vfs_free_t *free)
{
    free->size = lfs_config->block_count * lfs_config->block_size;
    free->used = lfs_fs_size(&lfs) * lfs_config->block_size;

    return true;
}

static int fs_format (void)
{
    int ret = lfs_format(&lfs, lfs_config);
    lfs_mount(&lfs, lfs_config);

    return ret;
}

void fs_littlefs_mount (const char *path, const struct lfs_config *config)
{
    static const vfs_t littlefs = {
        .fs_name = "littlefs",
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
        .format = fs_format
    };

    if((lfs_config = config) == NULL)
        return;

    if (lfs_mount(&lfs, config) != LFS_ERR_OK)
        lfs_format(&lfs, config);

    if (lfs_mount(&lfs, config) == LFS_ERR_OK)
        vfs_mount(path, &littlefs);
}

#endif // LITTLEFS_ENABLE

