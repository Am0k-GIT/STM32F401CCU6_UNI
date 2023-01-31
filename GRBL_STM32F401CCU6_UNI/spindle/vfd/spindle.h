/*

  vfd/spindle.h - Top level functions for VFD spindle handling

  Part of grblHAL

  Copyright (c) 2022 Andrew Marles
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

#ifndef _VFD_SPINDLE_H_
#define _VFD_SPINDLE_H_

#ifdef ARDUINO
#include "../../grbl/hal.h"
#include "../../grbl/protocol.h"
#include "../../grbl/state_machine.h"
#include "../../grbl/report.h"
#else
#include "grbl/hal.h"
#include "grbl/protocol.h"
#include "grbl/state_machine.h"
#include "grbl/report.h"
#endif

#include "../shared.h"
#include "../modbus.h"

#ifndef VFD_ADDRESS
#define VFD_ADDRESS 0x01
#endif

#define VFD_RETRIES     25
#define VFD_RETRY_DELAY 100

typedef enum {
    VFD_Idle = 0,
    VFD_GetRPM,
    VFD_SetRPM,
    VFD_GetMinRPM,
    VFD_GetMaxRPM,
    VFD_GetMaxRPM50,
    VFD_GetStatus,
    VFD_SetStatus,
    VFD_GetMaxAmps,
    VFD_GetAmps
} vfd_response_t;

typedef struct {
    uint32_t modbus_address;
    uint32_t vfd_rpm_hz;
    uint32_t runstop_reg;
    uint32_t set_freq_reg;
    uint32_t get_freq_reg;
    uint32_t run_cw_cmd;
    uint32_t run_ccw_cmd;
    uint32_t stop_cmd;
    float in_multiplier;
    float in_divider;
    float out_multiplier;
    float out_divider;
} vfd_settings_t;

typedef float (*vfd_get_load_ptr)(void);

typedef struct {
    vfd_get_load_ptr get_load;
} vfd_ptrs_t;

typedef struct {
    const spindle_ptrs_t spindle;
    const vfd_ptrs_t vfd;
} vfd_spindle_ptrs_t;

extern vfd_settings_t vfd_config;

spindle_id_t vfd_register (const vfd_spindle_ptrs_t *vfd, const char *name);
const vfd_ptrs_t *vfd_get_active (void);

#endif
