/*

  vfd/spindle.c - Top level functions for VFD spindle handling

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

#include "../shared.h"

#if VFD_ENABLE

#include <math.h>
#include <string.h>

#ifdef ARDUINO
#include "../../grbl/nvs_buffer.h"
#else
#include "grbl/nvs_buffer.h"
#endif

#include "spindle.h"

#if VFD_ENABLE == SPINDLE_ALL && N_SPINDLE == 1
#warning Increase N_SPINDLE in grbl/config.h to a value high enough to accomodate all spindles.
#endif

typedef struct {
    spindle_id_t id;
    const vfd_spindle_ptrs_t *vfd;
} vfd_spindle_t;

static uint8_t n_spindle = 0;
static bool spindle_changed = false;
static vfd_ptrs_t vfd_spindle = {0};
static vfd_spindle_t vfd_spindles[N_SPINDLE];
static nvs_address_t nvs_address = 0;

static on_spindle_select_ptr on_spindle_select;
static on_realtime_report_ptr on_realtime_report = NULL;

vfd_settings_t vfd_config;

static void vfd_realtime_report (stream_write_ptr stream_write, report_tracking_flags_t report)
{
    static float load = -1.0f;

    if(on_realtime_report)
        on_realtime_report(stream_write, report);

    if(vfd_spindle.get_load) {
        float new_load = vfd_spindle.get_load();
        if(load != new_load || spindle_changed || report.all) {
            load = new_load;
            spindle_changed = false;
            stream_write("|Sl:");
            stream_write(ftoa(load, 1));
        }
    }
}

spindle_id_t vfd_register (const vfd_spindle_ptrs_t *vfd, const char *name)
{
    spindle_id_t spindle_id = -1;

    if(n_spindle < N_SPINDLE && (spindle_id = spindle_register(&vfd->spindle, name)) != -1) {
        vfd_spindles[n_spindle].id = spindle_id;
        vfd_spindles[n_spindle++].vfd = vfd;

        if(vfd->vfd.get_load && on_realtime_report == NULL) {
            on_realtime_report = grbl.on_realtime_report;
            grbl.on_realtime_report = vfd_realtime_report;
        }
    }

    return spindle_id;
}

// Add info about our settings for $help and enumerations.
// Potentially used by senders for settings UI.

static const setting_group_detail_t vfd_groups [] = {
    {Group_Root, Group_VFD, "VFD"}
};

static const setting_detail_t vfd_settings[] = {
     { Setting_VFD_ModbusAddress, Group_VFD, "ModBus address", NULL, Format_Integer, "########0", NULL, NULL, Setting_NonCore, &vfd_config.modbus_address, NULL, NULL },
#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_GS20 || VFD_ENABLE == SPINDLE_YL620A
     { Setting_VFD_RPM_Hz, Group_VFD, "RPM per Hz", "", Format_Integer, "####0", "1", "3000", Setting_NonCore, &vfd_config.vfd_rpm_hz, NULL, NULL },
#endif
#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_MODVFD
     { Setting_VFD_10, Group_VFD, "Run/Stop Register (decimal)", NULL, Format_Integer, "########0", NULL, NULL, Setting_NonCore, &vfd_config.runstop_reg, NULL, NULL },
     { Setting_VFD_11, Group_VFD, "Set Frequency Register (decimal)", "", Format_Integer, "########0", NULL, NULL, Setting_NonCore, &vfd_config.set_freq_reg, NULL, NULL },
     { Setting_VFD_12, Group_VFD, "Get Frequency Register (decimal)", NULL, Format_Integer, "########0", NULL, NULL, Setting_NonCore, &vfd_config.get_freq_reg, NULL, NULL },
     { Setting_VFD_13, Group_VFD, "Run CW Command (decimal)", "", Format_Integer, "########0", NULL, NULL, Setting_NonCore, &vfd_config.run_cw_cmd, NULL, NULL },
     { Setting_VFD_14, Group_VFD, "Run CCW Command (decimal)", NULL, Format_Integer, "########0", NULL, NULL, Setting_NonCore, &vfd_config.run_ccw_cmd, NULL, NULL },
     { Setting_VFD_15, Group_VFD, "Stop Command (decimal)", "", Format_Integer, "########0", NULL, NULL, Setting_NonCore, &vfd_config.stop_cmd, NULL, NULL },
     { Setting_VFD_16, Group_VFD, "RPM input Multiplier", "", Format_Decimal, "########0", NULL, NULL, Setting_NonCore, &vfd_config.in_multiplier, NULL, NULL },
     { Setting_VFD_17, Group_VFD, "RPM input Divider", "", Format_Decimal, "########0", NULL, NULL, Setting_NonCore, &vfd_config.in_divider, NULL, NULL },
     { Setting_VFD_18, Group_VFD, "RPM output Multiplier", "", Format_Decimal, "########0", NULL, NULL, Setting_NonCore, &vfd_config.out_multiplier, NULL, NULL },
     { Setting_VFD_19, Group_VFD, "RPM output Divider", "", Format_Decimal, "########0", NULL, NULL, Setting_NonCore, &vfd_config.out_divider, NULL, NULL },
#endif
};

#ifndef NO_SETTINGS_DESCRIPTIONS
static const setting_descr_t vfd_settings_descr[] = {
    { Setting_VFD_ModbusAddress, "VFD ModBus address" },
#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_GS20 || VFD_ENABLE == SPINDLE_YL620A
    { Setting_VFD_RPM_Hz, "RPM/Hz value for GS20 and YL620A" },
#endif
#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_MODVFD
    { Setting_VFD_10, "MODVFD Register for Run/stop" },
    { Setting_VFD_11, "MODVFD Set Frequency Register" },
    { Setting_VFD_12, "MODVFD Get Frequency Register" },
    { Setting_VFD_13, "MODVFD Command word for CW" },
    { Setting_VFD_14, "MODVFD Command word for CCW" },
    { Setting_VFD_15, "MODVFD Command word for stop" },
    { Setting_VFD_16, "MODVFD RPM value multiplier for programming RPM" },
    { Setting_VFD_17, "MODVFD RPM value divider for programming RPM" },
    { Setting_VFD_18, "MODVFD RPM value multiplier for reading RPM" },
    { Setting_VFD_19, "MODVFD RPM value divider for reading RPM" },
#endif
};
#endif

static void vfd_settings_save (void)
{
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&vfd_config, sizeof(vfd_settings_t), true);
}

static void vfd_settings_restore (void)
{
    vfd_config.modbus_address = VFD_ADDRESS;
//MODVFD settings below are defaulted to values for GS20 VFD
    vfd_config.vfd_rpm_hz = 60;
    vfd_config.runstop_reg = 8192; //0x2000
    vfd_config.set_freq_reg = 8193; //0x2001
    vfd_config.get_freq_reg = 8451; //0x2103
    vfd_config.run_cw_cmd = 18; //0x12
    vfd_config.run_ccw_cmd = 34; //0x22
    vfd_config.stop_cmd = 1; //0x02
    vfd_config.in_multiplier = 50;
    vfd_config.in_divider = 60;
    vfd_config.out_multiplier = 60;
    vfd_config.out_divider = 100;

    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&vfd_config, sizeof(vfd_settings_t), true);
}

static void vfd_settings_load (void)
{
    if(nvs_address != 0) {
        if((hal.nvs.memcpy_from_nvs((uint8_t *)&vfd_config, nvs_address, sizeof(vfd_settings_t), true) != NVS_TransferResult_OK))
            vfd_settings_restore();
    }
}

static setting_details_t vfd_setting_details = {
    .groups = vfd_groups,
    .n_groups = sizeof(vfd_groups) / sizeof(setting_group_detail_t),
    .settings = vfd_settings,
    .n_settings = sizeof(vfd_settings) / sizeof(setting_detail_t),
#ifndef NO_SETTINGS_DESCRIPTIONS
    .descriptions = vfd_settings_descr,
    .n_descriptions = sizeof(vfd_settings_descr) / sizeof(setting_descr_t),
#endif
    .load = vfd_settings_load,
    .restore = vfd_settings_restore,
    .save = vfd_settings_save
};

static bool vfd_spindle_select (spindle_id_t spindle_id)
{
    uint_fast8_t idx = n_spindle;

    spindle_changed = true;
    memset(&vfd_spindle, 0, sizeof(vfd_ptrs_t));;
    if(n_spindle) do {
        if(vfd_spindles[--idx].id == spindle_id) {
            memcpy(&vfd_spindle, &vfd_spindles[idx].vfd->vfd, sizeof(vfd_ptrs_t));
            break;
        }
    } while(idx);

    if(on_spindle_select)
        on_spindle_select(spindle_id);

    return true;
}

const vfd_ptrs_t *vfd_get_active (void)
{
    return &vfd_spindle;
}

void vfd_init (void)
{
    if(modbus_enabled() && (nvs_address = nvs_alloc(sizeof(vfd_settings_t)))) {

        on_spindle_select = grbl.on_spindle_select;
        grbl.on_spindle_select = vfd_spindle_select;

        settings_register(&vfd_setting_details);

#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_HUANYANG1 || VFD_ENABLE == SPINDLE_HUANYANG2
        extern void vfd_huanyang_init (void);
        vfd_huanyang_init();
#endif

#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_GS20
        extern void vfd_gs20_init (void);
        vfd_gs20_init();
#endif

#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_YL620A
        extern void vfd_yl620_init (void);
        vfd_yl620_init();
#endif

#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_MODVFD
        extern void vfd_modvfd_init (void);
        vfd_modvfd_init();
#endif

#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_H100
        extern void vfd_h100_init (void);
        vfd_h100_init();
#endif
    }
}

#endif
