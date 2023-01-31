/*
  sdcard.h - SDCard plugin for FatFs

  Part of grblHAL

  Copyright (c) 2018-2022 Terje Io

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

#ifndef _SDCARD_H_
#define _SDCARD_H_

#if defined(ARDUINO)
#include "../driver.h"
#include "../grbl/hal.h"
#include "../grbl/platform.h"
#else
#include "driver.h"
#include "grbl/hal.h"
#include "grbl/platform.h"
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

typedef char *(*on_mount_ptr)(FATFS **fs);
typedef bool (*on_unmount_ptr)(FATFS **fs);

typedef struct {
    on_mount_ptr on_mount;
    on_unmount_ptr on_unmount;
} sdcard_events_t;

typedef struct
{
    char name[50];
    size_t size;
    size_t pos;
    uint32_t line;
} sdcard_job_t;

sdcard_events_t *sdcard_init (void);
bool sdcard_busy (void);
FATFS *sdcard_getfs (void);
sdcard_job_t *sdcard_get_job_info (void);
status_code_t stream_file (sys_state_t state, char *fname);

#endif // SDCARD_ENABLE

#endif
