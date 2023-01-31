/*
  motors/trinamic.c - Trinamic stepper driver plugin

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

#ifdef ARDUINO
#include "../driver.h"
#else
#include "driver.h"
#endif

#if TRINAMIC_ENABLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "trinamic.h"
#if TRINAMIC_I2C
#include "../trinamic/tmc_i2c_interface.h"
#endif

#include "../grbl/nvs_buffer.h"
#include "../grbl/protocol.h"
#include "../grbl/state_machine.h"
#include "../grbl/report.h"
#include "../grbl/platform.h"

static bool warning = false, is_homing = false, settings_loaded = false;
static volatile uint_fast16_t diag1_poll = 0;
static char sbuf[65]; // string buffer for reports
static uint_fast8_t n_motors = 0;
static float current_homing_rate = 0.0f;
static const tmchal_t *stepper[TMC_N_MOTORS_MAX];
static motor_map_t *motor_map;
static axes_signals_t homing = {0}, otpw_triggered = {0}, driver_enabled = {0};
#if TMC_POLL_STALLED
static limits_get_state_ptr limits_get_state = NULL;
#endif
static limits_enable_ptr limits_enable = NULL;
static stepper_pulse_start_ptr hal_stepper_pulse_start = NULL;
static nvs_address_t nvs_address;
static on_realtime_report_ptr on_realtime_report;
static on_report_options_ptr on_report_options;
static driver_setup_ptr driver_setup;
static settings_changed_ptr settings_changed;
static user_mcode_ptrs_t user_mcode;
static trinamic_driver_if_t driver_if = {0};
static trinamic_settings_t trinamic;

static struct {
    bool raw;
    bool sg_status_enable;
    volatile bool sg_status;
    bool sfilt;
    uint32_t sg_status_motor;
    axes_signals_t sg_status_motormask;
    uint32_t msteps;
} report = {0};

#if TRINAMIC_I2C

static stepper_enable_ptr stepper_enable = NULL;

TMCI2C_enable_dgr_t dgr_enable = {
    .addr.value = TMC_I2CReg_ENABLE
};

TMCI2C_monitor_status_dgr_t dgr_monitor = {
    .addr.value = TMC_I2CReg_MON_STATE
};
#endif

static void write_debug_report (uint_fast8_t axes);

// Wrapper for initializing physical interface
void trinamic_if_init (trinamic_driver_if_t *driver)
{
    memcpy(&driver_if, driver, sizeof(trinamic_driver_if_t));
}

#if 1 // Region settings

static void trinamic_drivers_init (axes_signals_t axes);
static void trinamic_settings_load (void);
static void trinamic_settings_restore (void);

static status_code_t set_axis_setting (setting_id_t setting, uint_fast16_t value);
static uint32_t get_axis_setting (setting_id_t setting);
static status_code_t set_axis_setting_float (setting_id_t setting, float value);
static float get_axis_setting_float (setting_id_t setting);
#if TRINAMIC_MIXED_DRIVERS
static status_code_t set_driver_enable (setting_id_t id, uint_fast16_t value);
static uint32_t get_driver_enable (setting_id_t setting);
#endif

static const setting_detail_t trinamic_settings[] = {
#if TRINAMIC_MIXED_DRIVERS
    { Setting_TrinamicDriver, Group_MotorDriver, "Trinamic driver", NULL, Format_AxisMask, NULL, NULL, NULL, Setting_NonCoreFn, set_driver_enable, get_driver_enable, NULL },
#endif
    { Setting_TrinamicHoming, Group_MotorDriver, "Sensorless homing", NULL, Format_AxisMask, NULL, NULL, NULL, Setting_NonCore, &trinamic.homing_enable.mask, NULL, NULL },
    { Setting_AxisStepperCurrent, Group_Axis0, "?-axis motor current", "mA", Format_Integer, "###0", NULL, NULL, Setting_NonCoreFn, set_axis_setting, get_axis_setting, NULL },
    { Setting_AxisMicroSteps, Group_Axis0, "?-axis microsteps", "steps", Format_Integer, "###0", NULL, NULL, Setting_NonCoreFn, set_axis_setting, get_axis_setting, NULL },
#if TMC_STALLGUARD == 4
    { Setting_AxisExtended0, Group_Axis0, "?-axis StallGuard4 fast threshold", NULL, Format_Decimal, "##0", "0", "255", Setting_NonCoreFn, set_axis_setting_float, get_axis_setting_float, NULL },
#else
    { Setting_AxisExtended0, Group_Axis0, "?-axis StallGuard2 fast threshold", NULL, Format_Decimal, "-##0", "-64", "63", Setting_NonCoreFn, set_axis_setting_float, get_axis_setting_float, NULL },
#endif
    { Setting_AxisExtended1, Group_Axis0, "?-axis hold current", "%", Format_Int8, "##0", "5", "100", Setting_NonCoreFn, set_axis_setting, get_axis_setting, NULL },
#if TMC_STALLGUARD == 4
    { Setting_AxisExtended2, Group_Axis0, "?-axis StallGuard4 slow threshold", NULL, Format_Decimal, "##0", "0", "255", Setting_NonCoreFn, set_axis_setting_float, get_axis_setting_float, NULL },
#else
    { Setting_AxisExtended2, Group_Axis0, "?-axis stallGuard2 slow threshold", NULL, Format_Decimal, "-##0", "-64", "63", Setting_NonCoreFn, set_axis_setting_float, get_axis_setting_float, NULL },
#endif
};

#ifndef NO_SETTINGS_DESCRIPTIONS

static const setting_descr_t trinamic_settings_descr[] = {
#if TRINAMIC_MIXED_DRIVERS
    { Setting_TrinamicDriver, "Enable SPI or UART controlled Trinamic drivers for axes." },
#endif
    { Setting_TrinamicHoming, "Enable sensorless homing for axis. Requires SPI controlled Trinamic drivers." },
    { Setting_AxisStepperCurrent, "Motor current in mA (RMS)." },
    { Setting_AxisMicroSteps, "Microsteps per fullstep." },
    { Setting_AxisExtended0, "StallGuard threshold for fast (seek) homing phase." },
    { Setting_AxisExtended1, "Motor current at standstill as a percentage of full current.\n"
                             "NOTE: if grblHAL is configured to disable motors on standstill this setting has no use."
    },
    { Setting_AxisExtended2, "StallGuard threshold for slow (feed) homing phase." },
};

#endif

static void trinamic_settings_save (void)
{
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&trinamic, sizeof(trinamic_settings_t), true);
}

static setting_details_t settings_details = {
    .settings = trinamic_settings,
    .n_settings = sizeof(trinamic_settings) / sizeof(setting_detail_t),
#ifndef NO_SETTINGS_DESCRIPTIONS
    .descriptions = trinamic_settings_descr,
    .n_descriptions = sizeof(trinamic_settings_descr) / sizeof(setting_descr_t),
#endif
    .load = trinamic_settings_load,
    .save = trinamic_settings_save,
    .restore = trinamic_settings_restore
};

static void trinamic_drivers_setup (void)
{
    if(driver_if.on_drivers_init) {

        uint8_t n_enabled = 0, motor = n_motors;

        do {
            if(bit_istrue(trinamic.driver_enable.mask, bit(motor_map[--motor].axis)))
                n_enabled++;
        } while(motor);

        driver_if.on_drivers_init(n_enabled, trinamic.driver_enable);
    }

    trinamic_drivers_init(trinamic.driver_enable);
}

#if TRINAMIC_MIXED_DRIVERS

static status_code_t set_driver_enable (setting_id_t id, uint_fast16_t value)
{
    if(trinamic.driver_enable.mask != (uint8_t)value) {

        driver_enabled.mask = 0;
        trinamic.driver_enable.mask = (uint8_t)value;

        trinamic_drivers_setup();
    }

    return Status_OK;
}

static uint32_t get_driver_enable (setting_id_t setting)
{
    return trinamic.driver_enable.mask;
}

#endif

// Parse and set driver specific parameters
static status_code_t set_axis_setting (setting_id_t setting, uint_fast16_t value)
{
    uint_fast8_t axis, motor = n_motors;
    status_code_t status = Status_OK;

    switch(settings_get_axis_base(setting, &axis)) {

        case Setting_AxisStepperCurrent:
            trinamic.driver[axis].current = (uint16_t)value;
            do {
                motor--;
                if(stepper[motor] && stepper[motor]->get_config(motor)->motor.axis == axis)
                    stepper[motor]->set_current(motor, trinamic.driver[axis].current, trinamic.driver[axis].hold_current_pct);
            } while(motor);
            break;

        case Setting_AxisExtended1: // Hold current percentage
            if(value > 100)
                value = 100;
            trinamic.driver[axis].hold_current_pct = (uint16_t)value;
            do {
                motor--;
                if(stepper[motor] && stepper[motor]->get_config(motor)->motor.axis == axis)
                    stepper[motor]->set_current(motor, trinamic.driver[axis].current, trinamic.driver[axis].hold_current_pct);
            } while(motor);
            break;

        case Setting_AxisMicroSteps:
            do {
                motor--;
                if(stepper[motor] && stepper[motor]->get_config(motor)->motor.axis == axis) {
                    if(stepper[motor]->microsteps_isvalid(motor, (uint16_t)value)) {
                        trinamic.driver[axis].microsteps = value;
                        stepper[motor]->set_microsteps(motor, trinamic.driver[axis].microsteps);
                        if(report.sg_status_motormask.mask & bit(axis))
                            report.msteps = trinamic.driver[axis].microsteps;
                    } else {
                        status = Status_InvalidStatement;
                        break;
                    }
                }
            } while(motor);
            break;

        default:
            status = Status_Unhandled;
            break;
    }

    return status;
}

static uint32_t get_axis_setting (setting_id_t setting)
{
    uint32_t value = 0;
    uint_fast8_t idx;

    switch(settings_get_axis_base(setting, &idx)) {

        case Setting_AxisStepperCurrent:
            value = trinamic.driver[idx].current;
            break;

        case Setting_AxisExtended1: // Hold current percentage
            value = trinamic.driver[idx].hold_current_pct;
            break;

        case Setting_AxisMicroSteps:
            value = trinamic.driver[idx].microsteps;
            break;

        default: // for stopping compiler warning
            break;
    }

    return value;
}

// Parse and set driver specific parameters
static status_code_t set_axis_setting_float (setting_id_t setting, float value)
{
    status_code_t status = Status_OK;

    uint_fast8_t idx;

    switch(settings_get_axis_base(setting, &idx)) {

        case Setting_AxisExtended0:
            trinamic.driver[idx].homing_seek_sensitivity = (int16_t)value;
            break;

        case Setting_AxisExtended2:
            trinamic.driver[idx].homing_feed_sensitivity = (int16_t)value;
            break;

        default:
            status = Status_Unhandled;
            break;
    }

    return status;
}

static float get_axis_setting_float (setting_id_t setting)
{
    float value = 0.0f;

    uint_fast8_t idx;

    switch(settings_get_axis_base(setting, &idx)) {

        case Setting_AxisExtended0:
            value = (float)trinamic.driver[idx].homing_seek_sensitivity;
            break;

        case Setting_AxisExtended2:
            value = (float)trinamic.driver[idx].homing_feed_sensitivity;
            break;

        default: // for stopping compiler warning
            break;
    }

    return value;
}

// Initialize default EEPROM settings
static void trinamic_settings_restore (void)
{
    uint_fast8_t idx = N_AXIS;

    trinamic.driver_enable.mask = driver_enabled.mask = 0;
    trinamic.homing_enable.mask = 0;

    do {

        switch(--idx) {

            case X_AXIS:
#if TMC_X_STEALTHCHOP
                trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                trinamic.driver_enable.x = TMC_X_ENABLE;
                trinamic.driver[idx].current = TMC_X_CURRENT;
                trinamic.driver[idx].hold_current_pct = TMC_X_HOLD_CURRENT_PCT;
                trinamic.driver[idx].microsteps = TMC_X_MICROSTEPS;
                trinamic.driver[idx].r_sense = TMC_X_R_SENSE;
                trinamic.driver[idx].homing_seek_sensitivity = TMC_X_HOMING_SEEK_SGT;
                trinamic.driver[idx].homing_feed_sensitivity = TMC_X_HOMING_FEED_SGT;
                break;

            case Y_AXIS:
#if TMC_Y_STEALTHCHOP
                trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                trinamic.driver_enable.y = TMC_Y_ENABLE;
                trinamic.driver[idx].current = TMC_Y_CURRENT;
                trinamic.driver[idx].hold_current_pct = TMC_Y_HOLD_CURRENT_PCT;
                trinamic.driver[idx].microsteps = TMC_Y_MICROSTEPS;
                trinamic.driver[idx].r_sense = TMC_Y_R_SENSE;
                trinamic.driver[idx].homing_seek_sensitivity = TMC_Y_HOMING_SEEK_SGT;
                trinamic.driver[idx].homing_feed_sensitivity = TMC_Y_HOMING_FEED_SGT;
                break;

            case Z_AXIS:
#if TMC_Z_STEALTHCHOP
                trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                trinamic.driver_enable.z = TMC_Z_ENABLE;
                trinamic.driver[idx].current = TMC_Z_CURRENT;
                trinamic.driver[idx].hold_current_pct = TMC_Z_HOLD_CURRENT_PCT;
                trinamic.driver[idx].microsteps = TMC_Z_MICROSTEPS;
                trinamic.driver[idx].r_sense = TMC_Z_R_SENSE;
                trinamic.driver[idx].homing_seek_sensitivity = TMC_Z_HOMING_SEEK_SGT;
                trinamic.driver[idx].homing_feed_sensitivity = TMC_Z_HOMING_FEED_SGT;
                break;

#ifdef A_AXIS
            case A_AXIS:
#if TMC_A_STEALTHCHOP
                trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                trinamic.driver_enable.z = TMC_A_ENABLE;
                trinamic.driver[idx].current = TMC_A_CURRENT;
                trinamic.driver[idx].hold_current_pct = TMC_A_HOLD_CURRENT_PCT;
                trinamic.driver[idx].microsteps = TMC_A_MICROSTEPS;
                trinamic.driver[idx].r_sense = TMC_A_R_SENSE;
                trinamic.driver[idx].homing_seek_sensitivity = TMC_A_HOMING_SEEK_SGT;
                trinamic.driver[idx].homing_feed_sensitivity = TMC_A_HOMING_FEED_SGT;
                break;
#endif

#ifdef B_AXIS
            case B_AXIS:
#if TMC_B_STEALTHCHOP
                trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                trinamic.driver_enable.z = TMC_B_ENABLE;
                trinamic.driver[idx].current = TMC_B_CURRENT;
                trinamic.driver[idx].hold_current_pct = TMC_B_HOLD_CURRENT_PCT;
                trinamic.driver[idx].microsteps = TMC_B_MICROSTEPS;
                trinamic.driver[idx].r_sense = TMC_B_R_SENSE;
                trinamic.driver[idx].homing_seek_sensitivity = TMC_B_HOMING_SEEK_SGT;
                trinamic.driver[idx].homing_feed_sensitivity = TMC_B_HOMING_FEED_SGT;
                break;
#endif

#ifdef C_AXIS
            case C_AXIS:
#if TMC_C_STEALTHCHOP
                trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                trinamic.driver_enable.z = TMC_C_ENABLE;
                trinamic.driver[idx].current = TMC_C_CURRENT;
                trinamic.driver[idx].hold_current_pct = TMC_C_HOLD_CURRENT_PCT;
                trinamic.driver[idx].microsteps = TMC_C_MICROSTEPS;
                trinamic.driver[idx].r_sense = TMC_C_R_SENSE;
                trinamic.driver[idx].homing_seek_sensitivity = TMC_C_HOMING_SEEK_SGT;
                trinamic.driver[idx].homing_feed_sensitivity = TMC_C_HOMING_FEED_SGT;
                break;
#endif
        }
    } while(idx);

    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&trinamic, sizeof(trinamic_settings_t), true);

    if(settings_loaded)
        trinamic_drivers_setup();
}

static void trinamic_settings_load (void)
{
    if(hal.nvs.memcpy_from_nvs((uint8_t *)&trinamic, nvs_address, sizeof(trinamic_settings_t), true) != NVS_TransferResult_OK)
        trinamic_settings_restore();
    else {
        uint_fast8_t idx = N_AXIS;
        do {
#if TMC_STALLGUARD == 4
            if(trinamic.driver[--idx].homing_seek_sensitivity < 0)
                trinamic.driver[idx].homing_seek_sensitivity = 0;
            if(trinamic.driver[idx].homing_feed_sensitivity  < 0)
                trinamic.driver[idx].homing_feed_sensitivity  = 0;
#else
            if(trinamic.driver[--idx].homing_seek_sensitivity > 64)
                trinamic.driver[idx].homing_seek_sensitivity = 0;
            if(trinamic.driver[idx].homing_feed_sensitivity  > 64)
                trinamic.driver[idx].homing_feed_sensitivity  = 0;
#endif
// Until $-setting is added set from mode from defines
            switch(idx) {
                case X_AXIS:
#if TMC_X_STEALTHCHOP
                    trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                    trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                    break;
                case Y_AXIS:
#if TMC_Y_STEALTHCHOP
                    trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                    trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif
                    break;
                case Z_AXIS:
#if TMC_Z_STEALTHCHOP
                    trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                    trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif

                    break;
#ifdef A_AXIS
                case A_AXIS:
#if TMC_A_STEALTHCHOP
                    trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                    trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif

                    break;
#endif
#ifdef B_AXIS
                case B_AXIS:
#if TMC_B_STEALTHCHOP
                    trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                    trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif

                    break;
#endif
#ifdef C_AXIS
                case C_AXIS:
#if TMC_C_STEALTHCHOP
                    trinamic.driver[idx].mode = TMCMode_StealthChop;
#else
                    trinamic.driver[idx].mode = TMCMode_CoolStep;
#endif

                    break;
#endif
            }
//
        } while(idx);
    }

#if !TRINAMIC_MIXED_DRIVERS
    trinamic.driver_enable.mask = AXES_BITMASK;
#endif

    settings_loaded = true;
}

static void on_settings_changed (settings_t *settings)
{
    static bool init_ok = false;
    static float steps_per_mm[N_AXIS];

    uint_fast8_t idx = N_AXIS;

    settings_changed(settings);

    if(init_ok) {
        do {
            idx--;
            if(steps_per_mm[idx] != settings->axis[idx].steps_per_mm) {
                steps_per_mm[idx] = settings->axis[idx].steps_per_mm;
#if PWM_THRESHOLD_VELOCITY > 0
                uint8_t motor = n_motors;
                do {
                    motor--;
                    if(bit_istrue(driver_enabled.mask, bit(idx)) && idx == motor_map[motor].axis)
                        stepper[motor]->set_tpwmthrs(motor, (float)PWM_THRESHOLD_VELOCITY / 60.0f, steps_per_mm[idx]);
                } while(motor);
#endif
            }
        } while(idx);
    } else {
        init_ok = true;
        do {
            idx--;
            steps_per_mm[idx] = settings->axis[idx].steps_per_mm;
        } while(idx);
    }
}

#endif // End region settings

static void pos_failed (sys_state_t state)
{
    report_message("Could not communicate with stepper driver!", Message_Warning);
}

static bool trinamic_driver_config (motor_map_t motor, uint8_t seq)
{
    bool ok = false;
    trinamic_driver_config_t cfg = {
        .address = motor.id,
        .settings = &trinamic.driver[motor.axis]
    };

    if(driver_if.on_driver_preinit)
        driver_if.on_driver_preinit(motor, &cfg);

    #if TRINAMIC_ENABLE == 2209
        ok = (stepper[motor.id] = TMC2209_AddMotor(motor, cfg.address, cfg.settings->current, cfg.settings->microsteps, cfg.settings->r_sense)) != NULL;
    #elif TRINAMIC_ENABLE == 2130
        ok = (stepper[motor.id] = TMC2130_AddMotor(motor, cfg.settings->current, cfg.settings->microsteps, cfg.settings->r_sense)) != NULL;
    #elif TRINAMIC_ENABLE == 5160
        ok = (stepper[motor.id] = TMC5160_AddMotor(motor, cfg.settings->current, cfg.settings->microsteps, cfg.settings->r_sense)) != NULL;
    #endif

    if(!ok) {
        protocol_enqueue_rt_command(pos_failed);
    //    system_raise_alarm(Alarm_SelftestFailed);
        return false;
    }

    stepper[motor.id]->get_config(motor.id)->motor.seq = seq; //

    driver_enabled.mask |= bit(motor.axis);

    switch(motor.axis) {

        case X_AXIS:
          #ifdef TMC_X_ADVANCED
            TMC_X_ADVANCED(motor.id)
          #endif
          #if TRINAMIC_I2C && TMC_X_MONITOR
            dgr_enable.reg.monitor.x = TMC_X_MONITOR;
          #endif
            break;

        case Y_AXIS:
          #ifdef TMC_Y_ADVANCED
            TMC_Y_ADVANCED(motor.id)
          #endif
          #if TRINAMIC_I2C && TMC_Y_MONITOR
            dgr_enable.reg.monitor.y = TMC_Y_MONITOR;
          #endif
            break;

        case Z_AXIS:
          #ifdef TMC_Z_ADVANCED
            TMC_Z_ADVANCED(motor.id)
          #endif
          #if TRINAMIC_I2C && TMC_Z_MONITOR
            dgr_enable.reg.monitor.z = TMC_Z_MONITOR;
          #endif
            break;

#ifdef A_AXIS
        case A_AXIS:
          #ifdef TMC_A_ADVANCED
            TMC_A_ADVANCED(motor.id)
          #endif
          #if TRINAMIC_I2C && TMC_A_MONITOR
            dgr_enable.reg.monitor.a = TMC_A_MONITOR;
          #endif
            break;
#endif

#ifdef B_AXIS
        case B_AXIS:
          #ifdef TMC_B_ADVANCED
            TMC_B_ADVANCED(motor.id)
          #endif
          #if TRINAMIC_I2C && TMC_B_MONITOR
            dgr_enable.reg.monitor.b = TMC_B_MONITOR;
          #endif
            break;
#endif

#ifdef C_AXIS
        case C_AXIS:
          #ifdef TMC_C_ADVANCED
            TMC_C_ADVANCED(motor.id)
          #endif
          #if TRINAMIC_I2C && TMC_C_MONITOR
            dgr_enable.reg.monitor.c = TMC_C_MONITOR;
          #endif
            break;
#endif
    }

    stepper[motor.id]->stealthChop(motor.id, cfg.settings->mode == TMCMode_StealthChop);

#if PWM_THRESHOLD_VELOCITY > 0
    stepper[motor.id]->set_tpwmthrs(motor.id, (float)PWM_THRESHOLD_VELOCITY / 60.0f, cfg.settings->steps_per_mm);
#endif
    stepper[motor.id]->set_current(motor.id, cfg.settings->current, cfg.settings->hold_current_pct);
    stepper[motor.id]->set_microsteps(motor.id, cfg.settings->microsteps);
#if TRINAMIC_I2C
    tmc_spi_write((trinamic_motor_t){0}, (TMC_spi_datagram_t *)&dgr_enable);
#endif

    if(driver_if.on_driver_postinit)
        driver_if.on_driver_postinit(motor, stepper[motor.id]);

    return true;
}

static void trinamic_drivers_init (axes_signals_t axes)
{
    bool ok = axes.value != 0;
    uint_fast8_t motor = n_motors, n_enabled = 0, seq = 0;

    memset(stepper, 0, sizeof(stepper));

    do {
        if(bit_istrue(axes.mask, bit(motor_map[--motor].axis)))
            seq++;
    } while(motor);

    motor = n_motors;
    do {
        if(bit_istrue(axes.mask, bit(motor_map[--motor].axis))) {
            if((ok = trinamic_driver_config(motor_map[motor], --seq)))
                n_enabled++;
        }
    } while(ok && motor);

    tmc_motors_set(ok ? n_enabled : 0);

    if(!ok) {
        driver_enabled.mask = 0;
        memset(stepper, 0, sizeof(stepper));
    }
}

// Add warning info to next realtime report when warning flag set by drivers
static void trinamic_realtime_report (stream_write_ptr stream_write, report_tracking_flags_t report)
{
    if(warning) {
        warning = false;
#if TRINAMIC_I2C
        TMC_spi_status_t status = tmc_spi_read((trinamic_motor_t){0}, (TMC_spi_datagram_t *)&dgr_monitor);
        otpw_triggered.mask |= dgr_monitor.reg.otpw.mask;
        sprintf(sbuf, "|TMCMON:%d:%d:%d:%d:%d", status, dgr_monitor.reg.ot.mask, dgr_monitor.reg.otpw.mask, dgr_monitor.reg.otpw_cnt.mask, dgr_monitor.reg.error.mask);
        stream_write(sbuf);
#endif
    }

    if(on_realtime_report)
        on_realtime_report(stream_write, report);
}

// Return pointer to end of string
static char *append (char *s)
{
    while(*s) s++;

    return s;
}

// Write CRLF terminated string to current stream
static void write_line (char *s)
{
    strcat(s, ASCII_EOL);
    hal.stream.write(s);
}

//
static void report_sg_status (sys_state_t state)
{
    hal.stream.write("[SG:");
    hal.stream.write(uitoa(stepper[report.sg_status_motor]->get_sg_result(report.sg_status_motor)));
    hal.stream.write("]" ASCII_EOL);
}

static void stepper_pulse_start (stepper_t *motors)
{
    static uint32_t step_count = 0;

    hal_stepper_pulse_start(motors);

    if(motors->step_outbits.mask & report.sg_status_motormask.mask) {
        uint32_t ms = hal.get_elapsed_ticks();
        if(ms - step_count >= 20) {
            step_count = ms;
            protocol_enqueue_rt_command(report_sg_status);
        }
/*        step_count++;
        if(step_count >= report.msteps * 4) {
            step_count = 0;
            protocol_enqueue_rt_command(report_sg_status);
        } */
    }
}

#if TRINAMIC_DEV

static void report_sg_params (void)
{
    sprintf(sbuf, "[SGPARAMS:%ld:%d:%d:%d]" ASCII_EOL, report.sg_status_motor, stepper[report.sg_status_motor].coolconf.reg.sfilt, stepper[report.sg_status_motor].coolconf.reg.semin, stepper[report.sg_status_motor].coolconf.reg.semax);
    hal.stream.write(sbuf);
}

#endif

static char *get_axisname (motor_map_t motor)
{
    static char axisname[3] = {0};

    axisname[0] = axis_letter[motor.axis][0];
    axisname[1] = motor.id == motor.axis ? '\0' : '2';

    return axisname;
}

// Validate M-code axis parameters
// Sets value to NAN (Not A Number) and returns false if driver not installed
static bool check_params (parser_block_t *gc_block)
{
    static const parameter_words_t wordmap[] = {
       { .x = On },
       { .y = On },
       { .z = On }
#if N_AXIS > 3
     , { .a = On },
       { .b = On },
       { .c = On }
#endif
    };

    uint_fast8_t n_found = 0, n_ok = 0, idx = N_AXIS;

    do {
        idx--;
        if(gc_block->words.mask & wordmap[idx].mask) {
            n_found++;
            if(bit_istrue(driver_enabled.mask, bit(idx)) && !isnanf(gc_block->values.xyz[idx])) {
                n_ok++;
                gc_block->words.mask &= ~wordmap[idx].mask;
            }
        } else
            gc_block->values.xyz[idx] = NAN;
    } while(idx);

    return n_ok > 0 && n_ok == n_found;
}

#define Trinamic_StallGuardParams 123
#define Trinamic_WriteRegister 124

// Check if given M-code is handled here
static user_mcode_t trinamic_MCodeCheck (user_mcode_t mcode)
{
#if TRINAMIC_DEV
    if(mcode == Trinamic_StallGuardParams || mcode == Trinamic_WriteRegister)
        return mcode;
#endif

    return mcode == Trinamic_DebugReport ||
            (driver_enabled.mask && (mcode == Trinamic_StepperCurrent || mcode == Trinamic_ReportPrewarnFlags || mcode == Trinamic_ClearPrewarnFlags ||
                                      mcode == Trinamic_HybridThreshold || mcode == Trinamic_HomingSensitivity))
              ? mcode
              : (user_mcode.check ? user_mcode.check(mcode) : UserMCode_Ignore);
}

// Validate driver specific M-code parameters
static status_code_t trinamic_MCodeValidate (parser_block_t *gc_block, parameter_words_t *deprecated)
{
    status_code_t state = Status_GcodeValueWordMissing;

    switch(gc_block->user_mcode) {

#if TRINAMIC_DEV

        case Trinamic_StallGuardParams:
            state = Status_OK;
            break;

        case Trinamic_WriteRegister:
            if(gc_block->words.r && gc_block->words.q) {
                if(isnanf(gc_block->values.r) || isnanf(gc_block->values.q))
                    state = Status_BadNumberFormat;
                else {
                    reg_ptr = TMC5160_GetRegPtr(&stepper[report.sg_status_motor], (tmc5160_regaddr_t)gc_block->values.r);
                    state = reg_ptr == NULL ? Status_GcodeValueOutOfRange : Status_OK;
                    gc_block->words.r = gc_block->words.q = Off;
                }
            }
            break;

#endif

        case Trinamic_DebugReport:
            state = Status_OK;

            if(gc_block->words.h && gc_block->values.h > 1)
                state = Status_BadNumberFormat;

            if(gc_block->words.q && isnanf(gc_block->values.q))
                state = Status_BadNumberFormat;

            if(gc_block->words.s && isnanf(gc_block->values.s))
                state = Status_BadNumberFormat;

            gc_block->words.h = gc_block->words.i = gc_block->words.q = gc_block->words.s =
             gc_block->words.x = gc_block->words.y = gc_block->words.z = Off;

#ifdef A_AXIS
            gc_block->words.a = Off;
#endif
#ifdef B_AXIS
            gc_block->words.b = Off;
#endif
#ifdef C_AXIS
            gc_block->words.c = Off;
#endif
//            gc_block->user_mcode_sync = true;
            break;

        case Trinamic_StepperCurrent:
            if(check_params(gc_block)) {
                state = Status_OK;
                gc_block->user_mcode_sync = true;
                if(!gc_block->words.q)
                    gc_block->values.q = NAN;
                else // TODO: add range check?
                    gc_block->words.q = Off;
            }
            break;

        case Trinamic_ReportPrewarnFlags:
        case Trinamic_ClearPrewarnFlags:
            state = Status_OK;
            break;

        case Trinamic_HybridThreshold:
            if(check_params(gc_block)) {
                state = Status_OK;
                gc_block->user_mcode_sync = true;
            }
            break;

        case Trinamic_HomingSensitivity:
            if(check_params(gc_block)) {
                uint_fast8_t idx = N_AXIS;
                state = gc_block->words.i && (isnanf(gc_block->values.ijk[0]) || gc_block->values.ijk[0] != 1.0f) ? Status_BadNumberFormat : Status_OK;
                gc_block->words.i = Off;
                if(state == Status_OK) do {
                    idx--;
#if TMC_STALLGUARD == 4
                    if(!isnanf(gc_block->values.xyz[idx]) && (gc_block->values.xyz[idx] < 0.0f || gc_block->values.xyz[idx] > 255.0f))
                        state = Status_BadNumberFormat;
#else
                    if(!isnanf(gc_block->values.xyz[idx]) && (gc_block->values.xyz[idx] < -64.0f || gc_block->values.xyz[idx] > 63.0f))
                        state = Status_BadNumberFormat;
#endif
                } while(idx && state == Status_OK);
            }
            break;

        default:
            state = Status_Unhandled;
            break;
    }

    return state == Status_Unhandled && user_mcode.validate ? user_mcode.validate(gc_block, deprecated) : state;
}

// Execute driver specific M-code
static void trinamic_MCodeExecute (uint_fast16_t state, parser_block_t *gc_block)
{
    bool handled = true;
    uint_fast8_t motor = n_motors;

    switch(gc_block->user_mcode) {

#if TRINAMIC_DEV

        case Trinamic_StallGuardParams:
            report_sg_params();
            break;

        case Trinamic_WriteRegister:
            reg_ptr->payload.value = (uint32_t)gc_block->values.q;
            TMC5160_WriteRegister(&stepper[report.sg_status_motor], reg_ptr);
            break;

#endif

        case Trinamic_DebugReport:
            {
                if(driver_enabled.mask != trinamic.driver_enable.mask) {
                    if(gc_block->words.i)
                        trinamic_drivers_init(trinamic.driver_enable);
                    else
                        pos_failed(state_get());
                    return;
                }

                if(!trinamic.driver_enable.mask) {
                    hal.stream.write("TMC driver(s) not enabled, enable with $338 setting." ASCII_EOL);
                    return;
                }

                axes_signals_t axes = {0};
                bool write_report = !(gc_block->words.i || gc_block->words.s || gc_block->words.h || gc_block->words.q);

                if(!write_report) {

                    if(gc_block->words.i)
                        trinamic_drivers_init(driver_enabled);

                    if(gc_block->words.h)
                        report.sfilt = gc_block->values.h != 0.0f;

                    if(gc_block->words.q)
                        report.raw = gc_block->values.q != 0.0f;

                    if(gc_block->words.s)
                        report.sg_status_enable = gc_block->values.s != 0.0f;
                }

                axes.x = gc_block->words.x;
                axes.y = gc_block->words.y;
                axes.z = gc_block->words.z;
    #ifdef A_AXIS
                axes.a = gc_block->words.a;
    #endif
    #ifdef B_AXIS
                axes.b = gc_block->words.b;
    #endif
    #ifdef C_AXIS
                axes.c = gc_block->words.c;
    #endif
                axes.mask &= driver_enabled.mask;

                if(!write_report) {

                    uint_fast16_t axis;

                    do {
                        motor--;
                        if(stepper[motor] && (axis = motor_map[motor].axis) == report.sg_status_motor) {
                            if(trinamic.driver[axis].mode == TMCMode_StealthChop)
                                stepper[motor]->stealthchop_enable(motor);
                            else if(trinamic.driver[motor].mode == TMCMode_CoolStep)
                                stepper[motor]->coolstep_enable(motor);
                        }
                    } while(motor);

                    if(axes.mask) {
                        uint_fast16_t mask = axes.mask;
                        axis = 0;
                        while(mask) {
                            if(mask & 0x01) {
                                report.sg_status_motor = axis;
                                break;
                            }
                            axis++;
                            mask >>= 1;
                        }
                    }

                    if(report.sg_status_enable) {

                        report.sg_status_motormask.mask = 1 << report.sg_status_motor;
                        report.msteps = trinamic.driver[report.sg_status_motor].microsteps;
                        if(hal_stepper_pulse_start == NULL) {
                            hal_stepper_pulse_start = hal.stepper.pulse_start;
                            hal.stepper.pulse_start = stepper_pulse_start;
                        }

                        motor = n_motors;
                        do {
                            if((axis = motor_map[--motor].axis) == report.sg_status_motor) {
                                stepper[motor]->stallguard_enable(motor, settings.homing.feed_rate, settings.axis[axis].steps_per_mm, trinamic.driver[motor_map[motor].axis].homing_seek_sensitivity);
                                stepper[motor]->sg_filter(motor, report.sfilt);
                                if(stepper[motor]->set_thigh_raw) // TODO: TMC2209 do not have this...
                                    stepper[motor]->set_thigh_raw(motor, 0);
                            }
                        } while(motor);
                    } else if(hal_stepper_pulse_start != NULL) {
                        hal.stepper.pulse_start = hal_stepper_pulse_start;
                        hal_stepper_pulse_start = NULL;
                    }

                } else
                    write_debug_report(axes.mask ? axes.mask : driver_enabled.mask);
            }
            break;

        case Trinamic_StepperCurrent:
            do {
                if(!isnanf(gc_block->values.xyz[motor_map[--motor].axis]))
                    stepper[motor]->set_current(motor, (uint16_t)gc_block->values.xyz[motor_map[motor].axis],
                                                      isnanf(gc_block->values.q) ? trinamic.driver[motor_map[motor].axis].hold_current_pct : (uint8_t)gc_block->values.q);
            } while(motor);
            break;

        case Trinamic_ReportPrewarnFlags:
            {
                TMC_drv_status_t status;
                strcpy(sbuf, "[TMCPREWARN:");
                for(motor = 0; motor < n_motors; motor++) {
                    if(bit_istrue(driver_enabled.mask, bit(motor_map[motor].axis))) {
                        status = stepper[motor]->get_drv_status(motor);
                        strcat(sbuf, "|");
                        strcat(sbuf, get_axisname(motor_map[motor]));
                        strcat(sbuf, ":");
                        if(status.driver_error)
                            strcat(sbuf, "E");
                        else if(status.ot)
                            strcat(sbuf, "O");
                        else if(status.otpw)
                            strcat(sbuf, "W");
                    }
                }
                hal.stream.write(sbuf);
                hal.stream.write("]" ASCII_EOL);
            }
            break;

        case Trinamic_ClearPrewarnFlags:
            otpw_triggered.mask = 0;
#if TRINAMIC_DEV
            stallGuard_enable(report.sg_status_motor, true); // TODO: (re)move this...
#endif
            break;

        case Trinamic_HybridThreshold:
            {
                uint_fast8_t axis;
                do {
                    axis = motor_map[--motor].axis;
                    if(!isnanf(gc_block->values.xyz[axis])) // mm/min
                        stepper[motor]->set_tpwmthrs(motor, gc_block->values.xyz[axis] / 60.0f, settings.axis[axis].steps_per_mm);
                } while(motor);
            }
            break;

        case Trinamic_HomingSensitivity:
            {
                uint_fast8_t axis;
                do {
                    axis = motor_map[--motor].axis;
                    if(!isnanf(gc_block->values.xyz[axis])) {
                        trinamic.driver[axis].homing_seek_sensitivity = (int16_t)gc_block->values.xyz[axis];
                        stepper[motor]->sg_filter(motor, report.sfilt);
                        stepper[motor]->sg_stall_value(motor, trinamic.driver[axis].homing_seek_sensitivity);
                    }
                } while(motor);
            }
            break;

        default:
            handled = false;
            break;
    }

    if(!handled && hal.user_mcode.execute)
        hal.user_mcode.execute(state, gc_block);
}

#if TRINAMIC_I2C

static void trinamic_stepper_enable (axes_signals_t enable)
{
    enable.mask ^= settings.steppers.enable_invert.mask;

    dgr_enable.reg.enable.mask = enable.mask & driver_enabled.mask;

    tmc_spi_write((trinamic_motor_t){0}, (TMC_spi_datagram_t *)&dgr_enable);
}

#endif

#if TMC_POLL_STALLED

// hal.limits.get_state is redirected here when homing
static limit_signals_t trinamic_limits (void)
{
    limit_signals_t signals = limits_get_state(); // read from switches first

    signals.min.mask &= ~homing.mask;
    signals.min2.mask &= ~homing.mask;

    if(hal.clear_bits_atomic(&diag1_poll, 0)) {
        // TODO: read I2C bridge status register instead of polling drivers when using I2C comms
        uint_fast8_t motor = n_motors;
        do {
            if(bit_istrue(homing.mask, bit(motor_map[--motor].axis))) {
                if(stepper[motor]->get_drv_status(motor).stallguard) {
                    if(motor == motor_map[motor].axis)
                        bit_true(signals.min.mask, motor_map[motor].axis);
                    else
                        bit_true(signals.min2.mask, motor_map[motor].axis);
                }
            }
        } while(motor);
    }

    return signals;
}

#endif

// Configure sensorless homing for enabled axes
static void trinamic_on_homing (axes_signals_t axes, float rate, bool pulloff)
{
    uint_fast8_t motor = n_motors, axis;

    axes.mask = driver_enabled.mask & trinamic.homing_enable.mask;

    if(axes.mask) do {
        axis = motor_map[--motor].axis;
        if(bit_istrue(axes.mask, bit(axis))) {
            if(pulloff) {
                current_homing_rate = 0.0f;
                if(trinamic.driver[axis].mode == TMCMode_StealthChop)
                    stepper[motor]->stealthchop_enable(motor);
                else if(trinamic.driver[axis].mode == TMCMode_CoolStep)
                    stepper[motor]->coolstep_enable(motor);
            } else if(current_homing_rate != rate) {
                if(rate == settings.homing.feed_rate)
                    stepper[motor]->stallguard_enable(motor, rate, settings.axis[axis].steps_per_mm, trinamic.driver[axis].homing_feed_sensitivity);
                else
                    stepper[motor]->stallguard_enable(motor, rate, settings.axis[axis].steps_per_mm, trinamic.driver[axis].homing_seek_sensitivity);
            }
        }
    } while(motor);
}

// Enable/disable sensorless homing
static void trinamic_homing (bool on, bool enable)
{
#ifdef TMC_HOMING_ACCELERATION
    static float accel[N_AXIS];
#endif

    if(limits_enable)
        limits_enable(on, enable);

    homing.mask = driver_enabled.mask & trinamic.homing_enable.mask;

    is_homing = enable;
    enable = enable && homing.mask;

    if(enable) {

        current_homing_rate = 0.0f;
        grbl.on_homing_rate_set = trinamic_on_homing;

#ifdef TMC_HOMING_ACCELERATION
        uint_fast8_t axis = 0, axes = homing.mask;
        while(axes) {
            if(axes & 1 && accel[axis] == 0.0f) {
                accel[axis] = settings.axis[axis].acceleration / (60.0f * 60.0f);
                settings_override_acceleration(axis, min(TMC_HOMING_ACCELERATION, accel[axis]));
            }
            axes >>= 1;
            axis++;
        }
#endif
#if TMC_POLL_STALLED
        if(limits_get_state == NULL) {
            limits_get_state = hal.limits.get_state;
            hal.limits.get_state = trinamic_limits;
        }
        diag1_poll = 0;
#endif
    } else {

        uint_fast8_t motor = n_motors, axis;

        do {
            axis = motor_map[--motor].axis;
            if(bit_istrue(driver_enabled.mask, bit(axis))) {
                if(trinamic.driver[axis].mode == TMCMode_StealthChop)
                    stepper[motor]->stealthchop_enable(motor);
                else if(trinamic.driver[axis].mode == TMCMode_CoolStep)
                    stepper[motor]->coolstep_enable(motor);
#ifdef TMC_HOMING_ACCELERATION
                if(accel[axis] > 0.0f) {
                    settings_override_acceleration(axis, accel[axis]);
                    accel[axis] = 0.0f;
                }
#endif
            }
        } while(motor);
#if TMC_POLL_STALLED
        if(limits_get_state != NULL) {
            hal.limits.get_state = limits_get_state;
            limits_get_state = NULL;
        }
#endif
    }
}

// Write Marlin style driver debug report to output stream (M122)
// NOTE: this output is not in a parse friendly format for grbl senders
static void write_debug_report (uint_fast8_t axes)
{
    typedef struct {
        TMC_chopconf_t chopconf;
        TMC_drv_status_t drv_status;
        uint32_t tstep;
        uint16_t current;
        TMC_ihold_irun_t ihold_irun;
    } debug_report_t;

    uint_fast8_t motor = n_motors;
    bool has_gscaler = false;
    debug_report_t debug_report[6];

    hal.stream.write("[TRINAMIC]" ASCII_EOL);

    do {
        if(bit_istrue(axes, bit(motor_map[--motor].axis))) {
            debug_report[motor].drv_status = stepper[motor]->get_drv_status(motor);
            debug_report[motor].chopconf = stepper[motor]->get_chopconf(motor);
            debug_report[motor].tstep = stepper[motor]->get_tstep(motor);
            debug_report[motor].current = stepper[motor]->get_current(motor);
            debug_report[motor].ihold_irun =  stepper[motor]->get_ihold_irun(motor);
//            TMC5160_ReadRegister(&stepper[motor], (TMC5160_datagram_t *)&stepper[motor]->pwm_scale);
            if(debug_report[motor].drv_status.otpw)
                otpw_triggered.mask |= bit(motor);
            has_gscaler |= !!stepper[motor]->get_global_scaler;
        }
    } while(motor);

    if(report.raw) {

    } else {

        sprintf(sbuf, "%-15s", "");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", get_axisname(motor_map[motor]));
        }

        write_line(sbuf);
        sprintf(sbuf, "%-15s", "Driver");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", stepper[motor]->name);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "Set current");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", stepper[motor]->get_config(motor)->current);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "RMS current");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", debug_report[motor].current);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "Peak current");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8" UINT32SFMT, (uint32_t)((float)debug_report[motor].current * sqrtf(2)));
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "Run current");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%5d/31", debug_report[motor].ihold_irun.irun);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "Hold current");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%5d/31", debug_report[motor].ihold_irun.ihold);
        }
        write_line(sbuf);

        if(has_gscaler) {
            sprintf(sbuf, "%-15s", "Global scaler");
            for(motor = 0; motor < n_motors; motor++) {
                if(bit_istrue(axes, bit(motor)) && stepper[motor]->get_global_scaler)
                    sprintf(append(sbuf), "%4d/256", stepper[motor]->get_global_scaler(motor));
            }
            write_line(sbuf);
        }

        sprintf(sbuf, "%-15s", "CS actual");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%5d/31", debug_report[motor].drv_status.cs_actual);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "PWM scale");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", stepper[motor]->pwm_scale(motor));
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "vsense");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis))) {
                if(stepper[motor]->vsense)
                    sprintf(append(sbuf), "%8s", stepper[motor]->vsense(motor) ? "1=0.180" : "0=0.325");
                else
                    sprintf(append(sbuf), "%8s", "N/A");
            }
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "stealthChop");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", stepper[motor]->get_en_pwm_mode(motor) ? "true" : "false");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "msteps");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", 1 << (8 - debug_report[motor].chopconf.mres));
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "tstep");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8" UINT32SFMT, debug_report[motor].tstep);
        }
        write_line(sbuf);

        hal.stream.write("pwm" ASCII_EOL);

        sprintf(sbuf, "%-15s", "threshold");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8" UINT32SFMT, stepper[motor]->get_tpwmthrs_raw(motor));
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "[mm/s]");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis))) {
                if(stepper[motor]->get_tpwmthrs)
                    sprintf(append(sbuf), "%8" UINT32SFMT, (uint32_t)stepper[motor]->get_tpwmthrs(motor, settings.axis[motor_map[motor].axis].steps_per_mm));
                else
                    sprintf(append(sbuf), "%8s", "-");
            }
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "OT prewarn");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.otpw ? "true" : "false");
        }
        write_line(sbuf);

        hal.stream.write("OT prewarn has" ASCII_EOL);
        sprintf(sbuf, "%-15s", "been triggered");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", bit_istrue(otpw_triggered.mask, bit(motor_map[motor].axis)) ? "true" : "false");
        }
        write_line(sbuf);

/*
    #if HAS_TMC220x
      TMC_REPORT("pwm scale sum",     TMC_PWM_SCALE_SUM);
      TMC_REPORT("pwm scale auto",    TMC_PWM_SCALE_AUTO);
      TMC_REPORT("pwm offset auto",   TMC_PWM_OFS_AUTO);
      TMC_REPORT("pwm grad auto",     TMC_PWM_GRAD_AUTO);
    #endif
        case TMC_PWM_SCALE_SUM: SERIAL_PRINT(st.pwm_scale_sum(), DEC); break;
        case TMC_PWM_SCALE_AUTO: SERIAL_PRINT(st.pwm_scale_auto(), DEC); break;
        case TMC_PWM_OFS_AUTO: SERIAL_PRINT(st.pwm_ofs_auto(), DEC); break;
        case TMC_PWM_GRAD_AUTO: SERIAL_PRINT(st.pwm_grad_auto(), DEC); break;

        sprintf(sbuf, "%-15s", "pwm autoscale");

        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", stepper[motor]->pwmconf.reg.pwm_autoscale);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "pwm ampl");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", stepper[motor]->pwmconf.reg.pwm_ampl);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "pwm grad");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", stepper[motor]->pwmconf.reg.pwm_grad);
        }
        write_line(sbuf);
*/

        sprintf(sbuf, "%-15s", "off time");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", debug_report[motor].chopconf.toff);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "blank time");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", debug_report[motor].chopconf.tbl);
        }
        write_line(sbuf);

        hal.stream.write("hysteresis" ASCII_EOL);

        sprintf(sbuf, "%-15s", "-end");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", (int)debug_report[motor].chopconf.hend - 3);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "-start");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", debug_report[motor].chopconf.hstrt + 1);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "Stallguard thrs");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", stepper[motor]->get_sg_stall_value(motor));
        }
        write_line(sbuf);

        hal.stream.write("DRIVER STATUS:" ASCII_EOL);

        sprintf(sbuf, "%-15s", "stallguard");
        write_line(sbuf);
        sprintf(sbuf, "%-15s", "sg_result");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8d", debug_report[motor].drv_status.sg_result);
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "fsactive");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.fsactive ? "*" : "");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "stst");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.stst ? "*" : "");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "olb");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.olb ? "*" : "");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "ola");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.ola ? "*" : "");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "s2gb");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.s2gb ? "*" : "");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "s2ga");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.s2ga ? "*" : "");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "otpw");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.otpw ? "*" : "");
        }
        write_line(sbuf);

        sprintf(sbuf, "%-15s", "ot");
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis)))
                sprintf(append(sbuf), "%8s", debug_report[motor].drv_status.ot ? "*" : "");
        }
        write_line(sbuf);

        hal.stream.write("STATUS REGISTERS:" ASCII_EOL);
        for(motor = 0; motor < n_motors; motor++) {
            if(bit_istrue(axes, bit(motor_map[motor].axis))) {
                uint32_t reg = stepper[motor]->get_drv_status_raw(motor);
                sprintf(sbuf, " %s = 0x%02X:%02X:%02X:%02X", get_axisname(motor_map[motor]), (uint8_t)(reg >> 24), (uint8_t)((reg >> 16) & 0xFF), (uint8_t)((reg >> 8) & 0xFF), (uint8_t)(reg & 0xFF));
                write_line(sbuf);
            }
        }
    }
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        hal.stream.write("[PLUGIN:Trinamic v0.09]" ASCII_EOL);
    else if(driver_enabled.mask) {
        hal.stream.write(",TMC=");
        hal.stream.write(uitoa(driver_enabled.mask));
    }
}

static void count_motors (motor_map_t motor)
{
    n_motors++;
}

static void assign_motors (motor_map_t motor)
{
    motor_map[n_motors++].value = motor.value;
}

static bool on_driver_setup (settings_t *settings)
{
    bool ok;

    if((ok = driver_setup(settings))) {
        hal.delay_ms(100, NULL); // Allow time for drivers to boot
        trinamic_drivers_setup();
    }

    return ok;
}

bool trinamic_init (void)
{
    if(hal.stepper.motor_iterator) {
        hal.stepper.motor_iterator(count_motors);
        if((motor_map = malloc(n_motors * sizeof(motor_map_t)))) {
            n_motors = 0;
            hal.stepper.motor_iterator(assign_motors);
        }
    } else {
        motor_map = malloc(N_AXIS * sizeof(motor_map_t));
        if(motor_map) {
            uint_fast8_t idx;
    //        n_motors = N_AXIS;
            for(idx = 0; idx < N_AXIS; idx++) {
                motor_map[idx].id = idx;
                motor_map[idx].axis = idx;
            }
            n_motors = idx;
        }
    }

    if(motor_map && (nvs_address = nvs_alloc(sizeof(trinamic_settings_t)))) {

        memcpy(&user_mcode, &hal.user_mcode, sizeof(user_mcode_ptrs_t));

        hal.user_mcode.check = trinamic_MCodeCheck;
        hal.user_mcode.validate = trinamic_MCodeValidate;
        hal.user_mcode.execute = trinamic_MCodeExecute;

        on_realtime_report = grbl.on_realtime_report;
        grbl.on_realtime_report = trinamic_realtime_report;

        on_report_options = grbl.on_report_options;
        grbl.on_report_options = onReportOptions;

        driver_setup = hal.driver_setup;
        hal.driver_setup = on_driver_setup;

        settings_changed = hal.settings_changed;
        hal.settings_changed = on_settings_changed;

        limits_enable = hal.limits.enable;
        hal.limits.enable = trinamic_homing;

        settings_register(&settings_details);

#if TRINAMIC_I2C
        stepper_enable = hal.stepper.enable;
        hal.stepper.enable = trinamic_stepper_enable;
#endif
    }

    return nvs_address != 0;
}

// Interrupt handler for DIAG1 signal(s)
void trinamic_fault_handler (void)
{
    if(is_homing)
        diag1_poll = 1;
    else {
        limit_signals_t limits = {0};
        limits.min.mask = AXES_BITMASK;
        hal.limits.interrupt_callback(limits);
    }
}

#if TRINAMIC_I2C
// Interrupt handler for warning event from I2C bridge
// Sets flag to add realtime report message
void trinamic_warn_handler (void)
{
    warning = true;
}
#endif

#endif
