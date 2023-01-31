/*

  h100.c - H-100 VFD spindle support

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

#include "../shared.h"

#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_H100

#include <math.h>
#include <string.h>

#include "spindle.h"

static bool spindle_active = false;
static spindle_id_t vfd_spindle_id = -1;
static uint32_t freq_min = 0, freq_max = 0;
static float rpm_programmed = -1.0f, rpm_low_limit = 0.0f, rpm_high_limit = 0.0f;
static spindle_state_t vfd_state = {0};
static spindle_data_t spindle_data = {0};
static on_report_options_ptr on_report_options;
static on_spindle_select_ptr on_spindle_select;
static driver_reset_ptr driver_reset;

static void rx_packet (modbus_message_t *msg);
static void rx_exception (uint8_t code, void *context);

static const modbus_callbacks_t callbacks = {
    .on_rx_packet = rx_packet,
    .on_rx_exception = rx_exception
};

// Read min and max configured frequency from spindle
static void spindleGetRPMLimits (void)
{
    modbus_message_t cmd = {
        .context = (void *)VFD_GetMinRPM,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadHoldingRegisters,
        .adu[2] = 0x00,
        .adu[3] = 0x0B, // PD11
        .adu[4] = 0x00,
        .adu[5] = 0x01,
        .tx_length = 8,
        .rx_length = 7
    };

    if(modbus_send(&cmd, &callbacks, true)) {

        cmd.context = (void *)VFD_GetMaxRPM;
        cmd.adu[3] = 0x05; // PD05

        modbus_send(&cmd, &callbacks, true);
    }
}

static void spindleSetRPM (float rpm, bool block)
{
    if (rpm != rpm_programmed) {

        uint16_t freq = (uint16_t)(rpm * 0.167f); // * 10.0f / 60.0f

        freq = min(max(freq, freq_min), freq_max);

        modbus_message_t rpm_cmd = {
            .context = (void *)VFD_SetRPM,
            .crc_check = false,
            .adu[0] = vfd_config.modbus_address,
            .adu[1] = ModBus_WriteRegister,
            .adu[2] = 0x02,
            .adu[4] = freq >> 8,
            .adu[5] = freq & 0xFF,
            .tx_length = 8,
            .rx_length = 8
        };

        vfd_state.at_speed = false;

        modbus_send(&rpm_cmd, &callbacks, block);

        if(settings.spindle.at_speed_tolerance > 0.0f) {
            rpm_low_limit = rpm / (1.0f + settings.spindle.at_speed_tolerance);
            rpm_high_limit = rpm * (1.0f + settings.spindle.at_speed_tolerance);
        }
        rpm_programmed = rpm;
    }
}

static void spindleUpdateRPM (float rpm)
{
    spindleSetRPM(rpm, false);
}

// Start or stop spindle
static void spindleSetState (spindle_state_t state, float rpm)
{
    modbus_message_t mode_cmd = {
        .context = (void *)VFD_SetStatus,
        .crc_check = false,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_WriteCoil,
        .adu[2] = 0x00,
        .adu[3] = (!state.on || rpm == 0.0f) ? 0x4B : (state.ccw ? 0x4A : 0x49),
        .adu[4] = 0xFF,
        .tx_length = 8,
        .rx_length = 8
    };

    if(vfd_state.ccw != state.ccw)
        rpm_programmed = 0.0f;

    vfd_state.on = state.on;
    vfd_state.ccw = state.ccw;

    if(modbus_send(&mode_cmd, &callbacks, true))
        spindleSetRPM(rpm, true);
}

// Returns spindle state in a spindle_state_t variable
static spindle_state_t spindleGetState (void)
{
    modbus_message_t mode_cmd = {
        .context = (void *)VFD_GetRPM,
        .crc_check = false,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadInputRegisters,
        .adu[2] = 0x00,
        .adu[3] = 0x00,
        .adu[4] = 0x00,
        .adu[5] = 0x02,
        .tx_length = 8,
        .rx_length = 9
    };

    modbus_send(&mode_cmd, &callbacks, false); // TODO: add flag for not raising alarm?

    return vfd_state; // return previous state as we do not want to wait for the response
}

static spindle_data_t *spindleGetData (spindle_data_request_t request)
{
    return &spindle_data;
}

static float f2rpm (uint16_t f)
{
    return (float)f * 6.0f; // * 60.0f / 10.0f
}

static void rx_packet (modbus_message_t *msg)
{
    if(!(msg->adu[0] & 0x80)) {

        switch((vfd_response_t)msg->context) {

            case VFD_GetRPM:
                spindle_data.rpm = f2rpm((msg->adu[3] << 8) | msg->adu[4]);
                vfd_state.at_speed = settings.spindle.at_speed_tolerance <= 0.0f || (spindle_data.rpm >= rpm_low_limit && spindle_data.rpm <= rpm_high_limit);
                break;

            case VFD_GetMinRPM:
                freq_min = (msg->adu[3] << 8) | msg->adu[4];
                break;

            case VFD_GetMaxRPM:
                freq_max = (msg->adu[3] << 8) | msg->adu[4];
                hal.spindle.cap.rpm_range_locked = On;
                hal.spindle.rpm_min = f2rpm(freq_min);
                hal.spindle.rpm_max = f2rpm(freq_max);
                break;

            default:
                break;
        }
    }
}

static void raise_alarm (sys_state_t state)
{
    system_raise_alarm(Alarm_Spindle);
}

static void rx_exception (uint8_t code, void *context)
{
    // Alarm needs to be raised directly to correctly handle an error during reset (the rt command queue is
    // emptied on a warm reset). Exception is during cold start, where alarms need to be queued.
    if(sys.cold_start)
        protocol_enqueue_rt_command(raise_alarm);
    else
        system_raise_alarm(Alarm_Spindle);
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        hal.stream.write("[PLUGIN:H-100 VFD v0.01]" ASCII_EOL);
}

static void spindle_reset (void)
{
    driver_reset();

    if(spindle_active)
        spindleGetRPMLimits();
}

bool vfd_spindle_config (void)
{
    static bool init_ok = false;

    if(!modbus_isup())
        return false;

    if(!init_ok) {
        init_ok = true;
        spindleGetRPMLimits();
    }

    return true;
}

static bool vfd_spindle_select (spindle_id_t spindle_id)
{
    if((spindle_active = spindle_id == vfd_spindle_id)) {

        if(settings.spindle.ppr == 0)
            hal.spindle.get_data = spindleGetData;

    } else if(hal.spindle.get_data == spindleGetData)
        hal.spindle.get_data = NULL;

    if(on_spindle_select)
        on_spindle_select(spindle_id);

    return true;
}

void vfd_h100_init (void)
{
    static const vfd_spindle_ptrs_t spindle = {
        .spindle.type = SpindleType_VFD,
        .spindle.cap.variable = On,
        .spindle.cap.at_speed = On,
        .spindle.cap.direction = On,
        .spindle.config = vfd_spindle_config,
        .spindle.set_state = spindleSetState,
        .spindle.get_state = spindleGetState,
        .spindle.update_rpm = spindleUpdateRPM
    };

    if((vfd_spindle_id = vfd_register(&spindle, "H-100")) != -1) {

        on_spindle_select = grbl.on_spindle_select;
        grbl.on_spindle_select = vfd_spindle_select;

        on_report_options = grbl.on_report_options;
        grbl.on_report_options = onReportOptions;

        driver_reset = hal.driver_reset;
        hal.driver_reset = spindle_reset;
    }
}

#endif
