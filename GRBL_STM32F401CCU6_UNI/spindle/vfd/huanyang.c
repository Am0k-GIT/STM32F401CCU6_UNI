/*

  huanyang.c - Huanyang VFD spindle support

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

#include "../shared.h"

#if VFD_ENABLE == SPINDLE_ALL || VFD_ENABLE == SPINDLE_HUANYANG1 || VFD_ENABLE == SPINDLE_HUANYANG2

#include <math.h>
#include <string.h>

#include "spindle.h"

static spindle_id_t v1_spindle_id = -1, v2_spindle_id = -1;
static bool v1_active = false,  v2_active = false;
static float rpm_programmed = -1.0f;
static spindle_state_t vfd_state = {0};
static spindle_data_t spindle_data = {0};
static on_report_options_ptr on_report_options;
static on_spindle_select_ptr on_spindle_select;
static driver_reset_ptr driver_reset;
static uint32_t rpm_max = 0;
static float amps_max = 0;
static float amps = 0;

static void rx_exception (uint8_t code, void *context);
static spindle_data_t *spindleGetData (spindle_data_request_t request);

#if VFD_ENABLE == SPINDLE_HUANYANG1 || VFD_ENABLE == SPINDLE_ALL

static float rpm_max50 = 3000;
static void v1_rx_packet (modbus_message_t *msg);

// Testing Huanyang VFDs (with other devices on the bus) has shown failure to respond if silent period is < 6ms
static const modbus_silence_timeout_t v1_silence =
{
    .b2400   = 16,
    .b4800   = 8,
    .b9600   = 6,
    .b19200  = 6,
    .b38400  = 6,
    .b115200 = 6
};

static const modbus_callbacks_t v1_callbacks = {
    .on_rx_packet = v1_rx_packet,
    .on_rx_exception = rx_exception
};

// Read maximum configured RPM from spindle, value is used later for calculating current RPM
// In the case of the original Huanyang protocol, the value is the configured RPM at 50Hz
static void v1_spindleGetMaxRPM (void)
{
    modbus_message_t cmd = {
        .context = (void *)VFD_GetMaxRPM50,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadCoils,
        .adu[2] = 0x03,
        .adu[3] = 0x90, // PD144
        .adu[4] = 0x00,
        .adu[5] = 0x00,
        .tx_length = 8,
        .rx_length = 8
    };

    modbus_set_silence(&v1_silence);
    modbus_send(&cmd, &v1_callbacks, true);
}

// Read maximum configured current from spindle, value is used later for calculating spindle load
static void v1_spindleGetMaxAmps (void)
{
    modbus_message_t cmd = {
        .context = (void *)VFD_GetMaxAmps,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadCoils,
        .adu[2] = 0x03,
        .adu[3] = 0x8E, // PD142
        .adu[4] = 0x00,
        .adu[5] = 0x00,
        .tx_length = 8,
        .rx_length = 8
    };

    modbus_set_silence(&v1_silence);
    modbus_send(&cmd, &v1_callbacks, true);
}

static void v1_spindleSetRPM (float rpm, bool block)
{
    if (rpm != rpm_programmed) {

        uint32_t data = lroundf(rpm * 5000.0f / (float)rpm_max50); // send Hz * 10  (Ex:1500 RPM = 25Hz .... Send 2500)

        modbus_message_t rpm_cmd = {
            .context = (void *)VFD_SetRPM,
            .crc_check = false,
            .adu[0] = vfd_config.modbus_address,
            .adu[1] = ModBus_WriteCoil,
            .adu[2] = 0x02,
            .adu[3] = data >> 8,
            .adu[4] = data & 0xFF,
            .tx_length = 7,
            .rx_length = 6
        };

        vfd_state.at_speed = false;

        modbus_send(&rpm_cmd, &v1_callbacks, block);

        if(settings.spindle.at_speed_tolerance > 0.0f) {
            spindle_data.rpm_low_limit = rpm / (1.0f + settings.spindle.at_speed_tolerance);
            spindle_data.rpm_high_limit = rpm * (1.0f + settings.spindle.at_speed_tolerance);
        }
        rpm_programmed = rpm;
    }
}

static void v1_spindleUpdateRPM (float rpm)
{
    v1_spindleSetRPM(rpm, false);
}

// Start or stop spindle
static void v1_spindleSetState (spindle_state_t state, float rpm)
{
    modbus_message_t mode_cmd = {
        .context = (void *)VFD_SetStatus,
        .crc_check = false,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadHoldingRegisters,
        .adu[2] = 0x01,
        .adu[3] = (!state.on || rpm == 0.0f) ? 0x08 : (state.ccw ? 0x11 : 0x01),
        .tx_length = 6,
        .rx_length = 6
    };

    if(vfd_state.ccw != state.ccw)
        rpm_programmed = 0.0f;

    vfd_state.on = state.on;
    vfd_state.ccw = state.ccw;

    if(modbus_send(&mode_cmd, &v1_callbacks, true))
        v1_spindleSetRPM(rpm, true);
}

static spindle_data_t *v1_spindleGetData (spindle_data_request_t request)
{
    return &spindle_data;
}

// Returns spindle state in a spindle_state_t variable
static spindle_state_t v1_spindleGetState (void)
{
    modbus_message_t rpm_cmd = {
        .context = (void *)VFD_GetRPM,
        .crc_check = false,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadInputRegisters,
        .adu[2] = 0x03,
        .adu[3] = 0x01,
        .tx_length = 8,
        .rx_length = 8
    };

    modbus_send(&rpm_cmd, &v1_callbacks, false); // TODO: add flag for not raising alarm?

    modbus_message_t amps_cmd = {
        .context = (void *)VFD_GetAmps,
        .crc_check = false,
        .adu[0] = VFD_ADDRESS,
        .adu[1] = ModBus_ReadInputRegisters,
        .adu[2] = 0x03,
        .adu[3] = 0x02,     // Output amps * 10
        .tx_length = 8,
        .rx_length = 8
    };
    modbus_send(&amps_cmd, &v1_callbacks, false); // TODO: add flag for not raising alarm?

    // Get the actual RPM from spindle encoder input when available.
    if(hal.spindle.get_data && hal.spindle.get_data != v1_spindleGetData) {
        float rpm = hal.spindle.get_data(SpindleData_RPM)->rpm;
        vfd_state.at_speed = settings.spindle.at_speed_tolerance <= 0.0f || (rpm >= spindle_data.rpm_low_limit && rpm <= spindle_data.rpm_high_limit);
    }

    return vfd_state; // return previous state as we do not want to wait for the response
}

static void v1_rx_packet (modbus_message_t *msg)
{
    if(!(msg->adu[0] & 0x80)) {

        switch((vfd_response_t)msg->context) {

            case VFD_GetRPM:
                spindle_data.rpm = (float)((msg->adu[4] << 8) | msg->adu[5]) * (float)rpm_max50 / 5000.0f;
                vfd_state.at_speed = settings.spindle.at_speed_tolerance <= 0.0f || (spindle_data.rpm >= spindle_data.rpm_low_limit && spindle_data.rpm <= spindle_data.rpm_high_limit);
                break;

            case VFD_GetMaxRPM:
                rpm_max = (msg->adu[4] << 8) | msg->adu[5];
                break;

            case VFD_GetMaxRPM50:
                rpm_max50 = (msg->adu[4] << 8) | msg->adu[5];
                break;

            case VFD_GetMaxAmps:
                amps_max = (float)((msg->adu[4] << 8) | msg->adu[5]) / 10.0f;
                break;

            case VFD_GetAmps:
                amps = (float)((msg->adu[4] << 8) | msg->adu[5]) / 10.0f;
                break;

            default:
                break;
        }
    }
}

bool v1_spindle_config (void)
{
    static bool init_ok = false;

    if(!modbus_isup())
        return false;

    if(!init_ok) {
        init_ok = true;
        v1_spindleGetMaxRPM();
        v1_spindleGetMaxAmps();
    }

    return true;
}

#endif // VFD_ENABLE == SPINDLE_HUANYANG1 || VFD_ENABLE == SPINDLE_ALL

#if VFD_ENABLE == SPINDLE_HUANYANG2 || VFD_ENABLE == SPINDLE_ALL

static void v2_rx_packet (modbus_message_t *msg);

static const modbus_callbacks_t v2_callbacks = {
    .on_rx_packet = v2_rx_packet,
    .on_rx_exception = rx_exception
};

// Read maximum configured RPM from spindle, value is used later for calculating current RPM
// In the case of the original Huanyang protocol, the value is the configured RPM at 50Hz
static void v2_spindleGetMaxRPM (void)
{
    modbus_message_t cmd = {
        .context = (void *)VFD_GetMaxRPM,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadHoldingRegisters,
        .adu[2] = 0xB0,
        .adu[3] = 0x05,
        .adu[4] = 0x00,
        .adu[5] = 0x02,
        .tx_length = 8,
        .rx_length = 8
    };

    modbus_set_silence(NULL);
    modbus_send(&cmd, &v2_callbacks, true);
}

static void v2_spindleSetRPM (float rpm, bool block)
{
    if (rpm != rpm_programmed) {

        uint16_t data = (uint32_t)(rpm) * 10000UL / rpm_max;

        modbus_message_t rpm_cmd = {
            .context = (void *)VFD_SetRPM,
            .crc_check = false,
            .adu[0] = vfd_config.modbus_address,
            .adu[1] = ModBus_WriteRegister,
            .adu[2] = 0x10,
            .adu[4] = data >> 8,
            .adu[5] = data & 0xFF,
            .tx_length = 8,
            .rx_length = 8
        };

        vfd_state.at_speed = false;

        modbus_send(&rpm_cmd, &v2_callbacks, block);

        if(settings.spindle.at_speed_tolerance > 0.0f) {
            spindle_data.rpm_low_limit = rpm / (1.0f + settings.spindle.at_speed_tolerance);
            spindle_data.rpm_high_limit = rpm * (1.0f + settings.spindle.at_speed_tolerance);
        }
        rpm_programmed = rpm;
    }
}

static void v2_spindleUpdateRPM (float rpm)
{
    v2_spindleSetRPM(rpm, false);
}

// Start or stop spindle
static void v2_spindleSetState (spindle_state_t state, float rpm)
{
    modbus_message_t mode_cmd = {
        .context = (void *)VFD_SetStatus,
        .crc_check = false,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_WriteRegister,
        .adu[2] = 0x20,
        .adu[5] = (!state.on || rpm == 0.0f) ? 6 : (state.ccw ? 2 : 1),
        .tx_length = 8,
        .rx_length = 8
    };

    if(vfd_state.ccw != state.ccw)
        rpm_programmed = 0.0f;

    vfd_state.on = state.on;
    vfd_state.ccw = state.ccw;

    if(modbus_send(&mode_cmd, &v2_callbacks, true))
        v2_spindleSetRPM(rpm, true);
}

// Returns spindle state in a spindle_state_t variable
static spindle_state_t v2_spindleGetState (void)
{
    modbus_message_t mode_cmd = {
        .context = (void *)VFD_GetRPM,
        .crc_check = false,
        .adu[0] = vfd_config.modbus_address,
        .adu[1] = ModBus_ReadHoldingRegisters,
        .adu[2] = 0x70,
        .adu[3] = 0x0C,
        .adu[4] = 0x00,
        .adu[5] = 0x02,
        .tx_length = 8,
        .rx_length = 8
    };

    modbus_send(&mode_cmd, &v2_callbacks, false); // TODO: add flag for not raising alarm?

    // Get the actual RPM from spindle encoder input when available.
    if(hal.spindle.get_data && hal.spindle.get_data != spindleGetData) {
        float rpm = hal.spindle.get_data(SpindleData_RPM)->rpm;
        vfd_state.at_speed = settings.spindle.at_speed_tolerance <= 0.0f || (rpm >= spindle_data.rpm_low_limit && rpm <= spindle_data.rpm_high_limit);
    }

    return vfd_state; // return previous state as we do not want to wait for the response
}

static void v2_rx_packet (modbus_message_t *msg)
{
    if(!(msg->adu[0] & 0x80)) {

        switch((vfd_response_t)msg->context) {

            case VFD_GetRPM:
                spindle_data.rpm = (float)((msg->adu[4] << 8) | msg->adu[5]);
                vfd_state.at_speed = settings.spindle.at_speed_tolerance <= 0.0f || (spindle_data.rpm >= spindle_data.rpm_low_limit && spindle_data.rpm <= spindle_data.rpm_high_limit);
                break;

            case VFD_GetMaxRPM:
                rpm_max = (msg->adu[4] << 8) | msg->adu[5];
                break;

            default:
                break;
        }
    }
}

bool v2_spindle_config (void)
{
    static bool init_ok = false;

    if(!modbus_isup())
        return false;

    if(!init_ok) {
        init_ok = true;
        v2_spindleGetMaxRPM();
    }

    return true;
}

#endif // VFD_ENABLE == SPINDLE_HUANYANG2 || VFD_ENABLE == SPINDLE_ALL

static float spindleGetLoad (void)
{
    if (amps_max)
        return ((amps/amps_max) * 100.0f);

    return 0;
}

static spindle_data_t *spindleGetData (spindle_data_request_t request)
{
    return &spindle_data;
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

    if(!newopt) {
#if VFD_ENABLE == SPINDLE_ALL
        hal.stream.write("[PLUGIN:HUANYANG VFD v0.08]" ASCII_EOL);
#elif VFD_ENABLE == SPINDLE_HUANYANG1
        hal.stream.write("[PLUGIN:HUANYANG v1 VFD v0.08]" ASCII_EOL);
#else
        hal.stream.write("[PLUGIN:HUANYANG P2A VFD v0.08]" ASCII_EOL);
#endif
    }
}

static void huanyang_reset (void)
{
    driver_reset();

#if VFD_ENABLE == SPINDLE_HUANYANG1 || VFD_ENABLE == SPINDLE_ALL
    if(v1_active) {
        v1_spindleGetMaxRPM();
        v1_spindleGetMaxAmps();
    }
#endif
#if VFD_ENABLE == SPINDLE_HUANYANG2 || VFD_ENABLE == SPINDLE_ALL
    if(v2_active)
        v2_spindleGetMaxRPM();
#endif
}

static bool huanyang_spindle_select (spindle_id_t spindle_id)
{
    if((v1_active = spindle_id == v1_spindle_id) || (v2_active = spindle_id == v2_spindle_id)) {

        if(settings.spindle.ppr == 0)
            hal.spindle.get_data = spindleGetData;

    } else if(hal.spindle.get_data == spindleGetData)
        hal.spindle.get_data = NULL;

    if(on_spindle_select)
        on_spindle_select(spindle_id);

    return true;
}

void vfd_huanyang_init (void)
{
#if VFD_ENABLE == SPINDLE_HUANYANG1 || VFD_ENABLE == SPINDLE_ALL
    static const vfd_spindle_ptrs_t v1_spindle = {
        .spindle.type = SpindleType_VFD,
        .spindle.cap.variable = On,
        .spindle.cap.at_speed = On,
        .spindle.cap.direction = On,
        .spindle.config = v1_spindle_config,
        .spindle.set_state = v1_spindleSetState,
        .spindle.get_state = v1_spindleGetState,
        .spindle.update_rpm = v1_spindleUpdateRPM,
        .vfd.get_load = spindleGetLoad
    };

    v1_spindle_id = vfd_register(&v1_spindle, "Huanyang v1");
#endif

#if VFD_ENABLE == SPINDLE_HUANYANG2 || VFD_ENABLE == SPINDLE_ALL
    static const vfd_spindle_ptrs_t v2_spindle = {
        .spindle.type = SpindleType_VFD,
        .spindle.cap.variable = On,
        .spindle.cap.at_speed = On,
        .spindle.cap.direction = On,
        .spindle.config = v2_spindle_config,
        .spindle.set_state = v2_spindleSetState,
        .spindle.get_state = v2_spindleGetState,
        .spindle.update_rpm = v2_spindleUpdateRPM
    };

    v2_spindle_id = vfd_register(&v2_spindle, "Huanyang P2A");
#endif

    if(v1_spindle_id != -1 || v2_spindle_id != -1) {

        on_spindle_select = grbl.on_spindle_select;
        grbl.on_spindle_select = huanyang_spindle_select;

        on_report_options = grbl.on_report_options;
        grbl.on_report_options = onReportOptions;

        driver_reset = hal.driver_reset;
        hal.driver_reset = huanyang_reset;
    }
}

#endif
