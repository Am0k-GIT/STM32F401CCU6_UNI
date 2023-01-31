/*

  coolant.c - plugin for for handling laser coolant

  Part of grblHAL

  Copyright (c) 2020-2022 Terje Io

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

#include "driver.h"

#if LASER_COOLANT_ENABLE

#include <string.h>
#include <math.h>

#ifdef ARDUINO
#include "../grbl/hal.h"
#include "../grbl/protocol.h"
#include "../grbl/nvs_buffer.h"
#else
#include "grbl/hal.h"
#include "grbl/protocol.h"
#include "grbl/nvs_buffer.h"
#endif

typedef union {
    uint8_t value;
    struct {
        uint8_t enable : 1;
    };
} coolant_options_t;

typedef struct {
    coolant_options_t options;
    float min_temp;
    float max_temp;
    float on_delay;
    float off_delay;
    uint8_t coolant_ok_port;
    uint8_t coolant_temp_port;
} coolant_settings_t;

static uint32_t coolant_off, coolant_off_delay = 0;
static uint8_t coolant_ok_port, coolant_temp_port;
static bool coolant_on = false, monitor_on = false, can_monitor = false;
static on_report_options_ptr on_report_options;
static on_realtime_report_ptr on_realtime_report;
static on_execute_realtime_ptr on_execute_realtime;
static coolant_ptrs_t on_coolant_changed;
static nvs_address_t nvs_address;
static coolant_settings_t coolant_settings;
static uint8_t n_ain, n_din;
static char max_aport[4], max_dport[4];

static void coolant_lost_handler (uint8_t port, bool state)
{
    if(coolant_on && !coolant_off_delay)
        system_set_exec_alarm(Alarm_AbortCycle);
}

// Start/stop tube coolant, wait for ok signal on start if delay is configured.
static void coolantSetState (coolant_state_t mode)
{
    static bool irq_checked = false;

    bool changed = mode.flood != hal.coolant.get_state().flood || (mode.flood && coolant_off_delay);

    if(changed && !mode.flood) {

        if(coolant_settings.off_delay > 0.0f && !sys.reset_pending) {
            mode.flood = On;
            coolant_off = hal.get_elapsed_ticks();
            coolant_off_delay = (uint32_t)(coolant_settings.off_delay * 60.0f) * 1000;
            on_coolant_changed.set_state(mode);
            return;
        }

        coolant_on = false;
    }

    on_coolant_changed.set_state(mode);

    if(changed && mode.flood) {
        coolant_off_delay = 0;
        if(coolant_settings.on_delay > 0.0f && hal.port.wait_on_input(Port_Digital, coolant_ok_port, WaitMode_High, coolant_settings.on_delay) != 1) {
            mode.flood = Off;
            coolant_on = false;
            on_coolant_changed.set_state(mode);
            system_set_exec_alarm(Alarm_AbortCycle);
        }
        coolant_on = true;
    }

    if(!irq_checked) {

        irq_checked = true;

        if(hal.port.get_pin_info) {
            xbar_t *port = hal.port.get_pin_info(Port_Digital, Port_Input, coolant_ok_port);
            if(port && (port->cap.irq_mode & IRQ_Mode_Falling))
                hal.port.register_interrupt_handler(coolant_ok_port, IRQ_Mode_Falling, coolant_lost_handler);
        }
    }

    monitor_on = mode.flood && (coolant_settings.min_temp + coolant_settings.max_temp) > 0.0f;
}

static void coolant_poll_realtime (sys_state_t state)
{
    on_execute_realtime(state);

    if(coolant_off_delay && hal.get_elapsed_ticks() - coolant_off > coolant_off_delay) {

        coolant_state_t mode = hal.coolant.get_state();
        mode.flood = Off;
        on_coolant_changed.set_state(mode);
        coolant_on = false;
        coolant_off_delay = 0;
        sys.report.coolant = On; // Set to report change immediately
    }
}

static void onRealtimeReport (stream_write_ptr stream_write, report_tracking_flags_t report)
{
    static float coolant_temp_prev = 0.0f;

    char buf[20] = "";

    *buf = '\0';

    if(can_monitor) {

        float coolant_temp = (float)hal.port.wait_on_input(Port_Analog, coolant_temp_port, WaitMode_Immediate, 0.0f) / 10.0f;

        if(coolant_temp_prev != coolant_temp || report.all) {
            strcat(buf, "|TCT:");
            strcat(buf, ftoa(coolant_temp, 1));
            coolant_temp_prev = coolant_temp;
        }

        if(monitor_on && coolant_temp > coolant_settings.max_temp)
            system_set_exec_alarm(Alarm_AbortCycle);
    }

    if(*buf != '\0')
        stream_write(buf);

    if(on_realtime_report)
        on_realtime_report(stream_write, report);
}

static bool is_setting_available (const setting_detail_t *setting)
{
    return (setting->id == Setting_CoolantMaxTemp || setting->id == Setting_CoolantTempPort) && n_ain > 0;
}

static const setting_detail_t plugin_settings[] = {
    { Setting_CoolantOnDelay, Group_Coolant, "Laser coolant on delay", "seconds", Format_Decimal, "#0.0", "0.0", "30.0", Setting_NonCore, &coolant_settings.on_delay, NULL, NULL },
    { Setting_CoolantOffDelay, Group_Coolant, "Laser coolant off delay", "minutes", Format_Decimal, "#0.0", "0.0", "30.0", Setting_NonCore, &coolant_settings.off_delay, NULL, NULL },
//    { Setting_CoolantMinTemp, Group_Coolant, "Laser coolant min temp", "deg", Format_Decimal, "#0.0", "0.0", "30.0", Setting_NonCore, &coolant_settings.min_temp, NULL, NULL, false },
    { Setting_CoolantMaxTemp, Group_Coolant, "Laser coolant max temp", "deg", Format_Decimal, "#0.0", "0.0", "30.0", Setting_NonCore, &coolant_settings.max_temp, NULL, is_setting_available },
    { Setting_CoolantTempPort, Group_AuxPorts, "Coolant temperature port", NULL, Format_Int8, "#0", "0", max_aport, Setting_NonCore, &coolant_settings.coolant_temp_port, NULL, is_setting_available, { .reboot_required = On } },
    { Setting_CoolantOkPort, Group_AuxPorts, "Coolant ok port", NULL, Format_Int8, "#0", "0", max_dport, Setting_NonCore, &coolant_settings.coolant_ok_port, NULL, NULL, { .reboot_required = On } }
};

#ifndef NO_SETTINGS_DESCRIPTIONS

static const setting_descr_t plugin_settings_descr[] = {
    { Setting_CoolantOnDelay, "" },
    { Setting_CoolantOffDelay, "" },
    { Setting_CoolantMaxTemp, "" },
    { Setting_CoolantTempPort, "Aux port number to use for coolant temperature monitoring." },
    { Setting_CoolantOkPort, "Aux port number to use for coolant ok signal." },
};

#endif

static void coolant_settings_save (void)
{
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&coolant_settings, sizeof(coolant_settings_t), true);
}

static void coolant_settings_restore (void)
{
    coolant_settings.min_temp =
    coolant_settings.max_temp =
    coolant_settings.on_delay =
    coolant_settings.off_delay = 0.0f;

    if(ioport_can_claim_explicit()) {
        coolant_settings.coolant_temp_port = n_ain ? n_ain - 1 : 0;
        coolant_settings.coolant_ok_port= n_din - 1;
    }

    coolant_settings_save();
}

static void coolant_settings_load (void)
{
    bool ok = true;

    if(hal.nvs.memcpy_from_nvs((uint8_t *)&coolant_settings, nvs_address, sizeof(coolant_settings_t), true) != NVS_TransferResult_OK)
        coolant_settings_restore();

    if(ioport_can_claim_explicit()) {

        // Sanity checks
        if(coolant_settings.coolant_temp_port > n_ain)
            coolant_settings.coolant_temp_port = n_ain ? n_ain - 1 : 0;
        if(coolant_settings.coolant_ok_port > n_din)
            coolant_settings.coolant_ok_port = n_din - 1;

        coolant_temp_port = coolant_settings.coolant_temp_port;
        coolant_ok_port = coolant_settings.coolant_ok_port;

        if(n_ain > 0)
            ok = (can_monitor = ioport_claim(Port_Analog, Port_Input, &coolant_temp_port, "Coolant temperature"));

        ok &= ioport_claim(Port_Digital, Port_Input, &coolant_ok_port, "Coolant ok");
    }

    if(ok) {

        on_realtime_report = grbl.on_realtime_report;
        grbl.on_realtime_report = onRealtimeReport;

        on_execute_realtime = grbl.on_execute_realtime;
        grbl.on_execute_realtime = coolant_poll_realtime;

        memcpy(&on_coolant_changed, &hal.coolant, sizeof(coolant_ptrs_t));
        hal.coolant.set_state = coolantSetState;
    }
}

static setting_details_t setting_details = {
    .settings = plugin_settings,
    .n_settings = sizeof(plugin_settings) / sizeof(setting_detail_t),
#ifndef NO_SETTINGS_DESCRIPTIONS
    .descriptions = plugin_settings_descr,
    .n_descriptions = sizeof(plugin_settings_descr) / sizeof(setting_descr_t),
#endif
    .save = coolant_settings_save,
    .load = coolant_settings_load,
    .restore = coolant_settings_restore,
};

static void report_options (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        hal.stream.write("[PLUGIN:Laser coolant v0.04]" ASCII_EOL);
}

static void warning_msg (uint_fast16_t state)
{
    report_message("Laser coolant plugin failed to initialize!", Message_Warning);
}

void laser_coolant_init (void)
{
    bool ok;

    n_ain = ioports_available(Port_Analog, Port_Input);
    n_din = ioports_available(Port_Digital, Port_Input);
    ok = n_din >= 1;

    if(ok) {

        if(!ioport_can_claim_explicit()) {

            // Driver does not support explicit port claiming, claim the highest numbered ports instead.

            if((ok = (nvs_address = nvs_alloc(sizeof(coolant_settings_t))))) {
                if(hal.port.num_analog_in > 0) {
                    coolant_temp_port = hal.port.num_analog_in - 1;
                    can_monitor = ioport_claim(Port_Analog, Port_Input, &coolant_temp_port, "Coolant temperature");
                }
                coolant_ok_port = hal.port.num_digital_in - 1;
                ioport_claim(Port_Digital, Port_Input, &coolant_ok_port, "Coolant ok");
            }

        } else
            ok = (nvs_address = nvs_alloc(sizeof(coolant_settings_t)));
    }

    if(ok) {

        if(n_ain)
            strcpy(max_aport, uitoa(n_ain - 1));
        strcpy(max_dport, uitoa(n_din - 1));

        on_report_options = grbl.on_report_options;
        grbl.on_report_options = report_options;

        settings_register(&setting_details);

    } else
        protocol_enqueue_rt_command(warning_msg);
}

#endif
