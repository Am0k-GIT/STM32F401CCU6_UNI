/*

  ppi.c - plugin for for laser PPI (Pulses Per Inch) mode

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

#if PPI_ENABLE

#include <math.h>
#include <string.h>

#include "grbl/hal.h"

typedef struct {
    uint_fast16_t ppi;
    float ppi_distance;
    float ppi_pos;
    float next_pos;
    uint_fast16_t pulse_length; // uS
    bool on;
} laser_ppi_t;

static laser_ppi_t laser = {
    .ppi = 600.0f,
    .ppi_distance = 25.4f / 600.0f,
    .pulse_length = 1500,
    .on = false
};

static user_mcode_ptrs_t user_mcode;
static on_report_options_ptr on_report_options;
static void (*stepper_wake_up)(void);
static void (*stepper_pulse_start)(stepper_t *stepper);
static on_spindle_select_ptr on_spindle_select;
static spindle_update_pwm_ptr spindle_update_pwm;
static spindle_update_rpm_ptr spindle_update_rpm;

static void stepperWakeUp (void)
{
    laser.ppi_pos = laser.next_pos = 0.0f;

    stepper_wake_up();
}

static void stepperPulseStartPPI (stepper_t *stepper)
{
    static float mm_per_step;

    if(laser.on) {

        if(stepper->new_block)
            mm_per_step = 1.0f / stepper->exec_block->steps_per_mm;

        if(stepper->step_outbits.mask) {
            laser.ppi_pos += mm_per_step;
            if(laser.ppi_pos >= laser.next_pos) {
                laser.next_pos += laser.ppi_distance;
                hal.spindle.pulse_on(laser.pulse_length);
            }
        }
    }

    stepper_pulse_start(stepper);
}

static void ppiUpdatePWM (uint_fast16_t pwm)
{
    if(!laser.on && pwm > 0)
        laser.ppi_pos = laser.next_pos = 0.0f;

    laser.on = pwm > 0;

    spindle_update_pwm(pwm);
}

static void ppiUpdateRPM (float rpm)
{
    if(!laser.on && rpm > 0.0f)
        laser.ppi_pos = laser.next_pos = 0.0f;

    laser.on = rpm > 0.0f;

    spindle_update_rpm(rpm);
}

static bool enable_ppi (bool on)
{
    if(!gc_laser_ppi_enable(on ? laser.ppi : 0, laser.pulse_length)) {

        if(on && stepper_wake_up == NULL) {
            stepper_wake_up = hal.stepper.wake_up;
            hal.stepper.wake_up = stepperWakeUp;
            stepper_pulse_start = hal.stepper.pulse_start;
            hal.stepper.pulse_start = stepperPulseStartPPI;
        }

        if(!on && stepper_wake_up != NULL) {
            hal.stepper.wake_up = stepper_wake_up;
            stepper_wake_up = NULL;
            hal.stepper.pulse_start = stepper_pulse_start;
            stepper_pulse_start = NULL;
        }
    }

    return on;
}

static user_mcode_t userMCodeCheck (user_mcode_t mcode)
{
    return mcode == LaserPPI_Enable || mcode == LaserPPI_Rate || mcode == LaserPPI_PulseLength
            ? mcode
            : (user_mcode.check ? user_mcode.check(mcode) : UserMCode_Ignore);
}

static status_code_t userMCodeValidate (parser_block_t *gc_block, parameter_words_t *deprecated)
{
    status_code_t state = Status_GcodeValueWordMissing;

    switch(gc_block->user_mcode) {

        case LaserPPI_Enable:
            if(!hal.driver_cap.laser_ppi_mode)
                state = Status_GcodeUnsupportedCommand;
            else if(gc_block->words.p) {
                state = isnan(gc_block->values.p) ? Status_BadNumberFormat : Status_OK;
                gc_block->words.p = Off;
            }
            break;

        case LaserPPI_Rate:
            if(!hal.driver_cap.laser_ppi_mode)
                state = Status_GcodeUnsupportedCommand;
            else if(gc_block->words.p) {
                state = isnan(gc_block->values.p) ? Status_BadNumberFormat : Status_OK;
                gc_block->user_mcode_sync = true;
                gc_block->words.p = Off;
            }
            break;

        case LaserPPI_PulseLength:
            if(!hal.driver_cap.laser_ppi_mode)
                state = Status_GcodeUnsupportedCommand;
            else if(gc_block->words.p) {
                state = isnan(gc_block->values.p) ? Status_BadNumberFormat : Status_OK;
                gc_block->user_mcode_sync = true;
                gc_block->words.p = Off;
            }
            break;

        default:
            state = Status_Unhandled;
            break;
    }

    return state == Status_Unhandled && user_mcode.validate ? user_mcode.validate(gc_block, deprecated) : state;
}

static void userMCodeExecute (uint_fast16_t state, parser_block_t *gc_block)
{
    static bool ppi_on = false;

    bool handled = true;

    if (state != STATE_CHECK_MODE)
      switch(gc_block->user_mcode) {

        case LaserPPI_Enable:
            ppi_on = gc_block->values.p != 0.0f;
            enable_ppi(ppi_on && laser.ppi > 0 && laser.pulse_length > 0);
            break;

        case LaserPPI_Rate:
            if((laser.ppi = (uint_fast16_t)gc_block->values.p) != 0)
                laser.ppi_distance = 25.4f / (float)laser.ppi;
            enable_ppi(ppi_on && laser.ppi > 0 && laser.pulse_length > 0);
            break;

        case LaserPPI_PulseLength:
            laser.pulse_length = (uint16_t)gc_block->values.p;
            enable_ppi(ppi_on && laser.ppi > 0 && laser.pulse_length > 0);
            break;

        default:
            handled = false;
            break;
    }

    if(!handled && user_mcode.execute)
        user_mcode.execute(state, gc_block);
}

static bool onSpindleSelect (spindle_id_t spindle_id)
{
    if((hal.driver_cap.laser_ppi_mode = sys.mode == Mode_Laser && hal.spindle.cap.laser && hal.spindle.pulse_on != NULL)) {

        if(hal.spindle.update_pwm) {
            spindle_update_pwm = hal.spindle.update_pwm;
            hal.spindle.update_pwm = ppiUpdatePWM;
        }
        if(hal.spindle.update_rpm) {
            spindle_update_rpm = hal.spindle.update_rpm;
            hal.spindle.update_rpm = ppiUpdateRPM;
        }
    }

    return on_spindle_select == NULL || on_spindle_select(spindle_id);
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        hal.stream.write("[PLUGIN:Laser PPI v0.04]" ASCII_EOL);
}

void ppi_init (void)
{
    memcpy(&user_mcode, &hal.user_mcode, sizeof(user_mcode_ptrs_t));

    hal.user_mcode.check = userMCodeCheck;
    hal.user_mcode.validate = userMCodeValidate;
    hal.user_mcode.execute = userMCodeExecute;

    on_spindle_select = grbl.on_spindle_select;
    grbl.on_spindle_select = onSpindleSelect;

    on_report_options = grbl.on_report_options;
    grbl.on_report_options = onReportOptions;
}

#endif
