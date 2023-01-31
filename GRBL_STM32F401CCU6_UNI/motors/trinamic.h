/*
  motors/trinamic.h - Trinamic stepper driver plugin

  Part of grblHAL

  Copyright (c) 2018-2021 Terje Io

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

#ifndef _TRINAMIC_H_
#define _TRINAMIC_H_

#ifdef ARDUINO
#include "../driver.h"
#else
#include "driver.h"
#endif

#if TRINAMIC_ENABLE

#if TRINAMIC_ENABLE == 2130
#define R_SENSE 110
#include "../trinamic/tmc2130hal.h"
#endif
#if TRINAMIC_ENABLE == 2209
#define R_SENSE 110
#include "../trinamic/tmc2209hal.h"
#endif
#if TRINAMIC_ENABLE == 5160
#define R_SENSE 75
#include "../trinamic/tmc5160hal.h"
#endif

#ifndef TMC_POLL_STALLED
#if TRINAMIC_I2C
#define TMC_POLL_STALLED 1
#else
#define TMC_POLL_STALLED 0
#endif
#endif

#ifndef PWM_THRESHOLD_VELOCITY
#define PWM_THRESHOLD_VELOCITY 0 // mm/min - 0 to disable, should be set > homing seek rate when enabled (use M913 to set at run time)
#endif

#ifndef TMC_STEALTHCHOP
#define TMC_STEALTHCHOP 1    // 0 = CoolStep, 1 = StealthChop
#endif

#if TRINAMIC_ENABLE == 2209
#define TMC_STALLGUARD 4 // Do not change!
#else
#define TMC_STALLGUARD 2 // Do not change!
#endif

//#define TMC_HOMING_ACCELERATION 50.0f // NOT tested... Reduce acceleration during homing to avoid falsely triggering DIAG output

static const TMC_coolconf_t coolconf = { .semin = 5, .semax = 2, .sedn = 1 };
static const TMC_chopper_timing_t chopper_timing = { .hstrt = 1, .hend = -1, .tbl = 1 };

// General
#if TRINAMIC_MIXED_DRIVERS
#define TMC_X_ENABLE 0
#else
#define TMC_X_ENABLE 1 // Do not change
#endif
#define TMC_X_MONITOR 1
#define TMC_X_MICROSTEPS 16
#define TMC_X_R_SENSE R_SENSE   // mOhm

#ifndef DEFAULT_X_CURRENT
#define TMC_X_CURRENT 500       // mA RMS
#else
#define TMC_X_CURRENT DEFAULT_X_CURRENT       // mA RMS
#endif 

#define TMC_X_HOLD_CURRENT_PCT 50
#define TMC_X_HOMING_SEEK_SGT 22
#define TMC_X_HOMING_FEED_SGT 22
#define TMC_X_STEALTHCHOP TMC_STEALTHCHOP

#define TMC_X_ADVANCED(motor) \
stepper[motor]->sg_filter(motor, 1); \
stepper[motor]->coolconf(motor, coolconf); \
stepper[motor]->chopper_timing(motor, chopper_timing);

#if TRINAMIC_MIXED_DRIVERS
#define TMC_Y_ENABLE 0
#else
#define TMC_Y_ENABLE 1 // Do not change
#endif
#define TMC_Y_MONITOR 1
#define TMC_Y_MICROSTEPS 16
#define TMC_Y_R_SENSE R_SENSE   // mOhm

#ifndef DEFAULT_Y_CURRENT
#define TMC_Y_CURRENT 500       // mA RMS
#else
#define TMC_Y_CURRENT DEFAULT_Y_CURRENT       // mA RMS
#endif

#define TMC_Y_HOLD_CURRENT_PCT 50
#define TMC_Y_HOMING_SEEK_SGT 22
#define TMC_Y_HOMING_FEED_SGT 22
#define TMC_Y_STEALTHCHOP TMC_STEALTHCHOP

#define TMC_Y_ADVANCED(motor) \
stepper[motor]->sg_filter(motor, 1); \
stepper[motor]->coolconf(motor, coolconf); \
stepper[motor]->chopper_timing(motor, chopper_timing);

#if TRINAMIC_MIXED_DRIVERS
#define TMC_Z_ENABLE 0
#else
#define TMC_Z_ENABLE 1 // Do not change
#endif
#define TMC_Z_MONITOR 1
#define TMC_Z_MICROSTEPS 16
#define TMC_Z_R_SENSE R_SENSE   // mOhm

#ifndef DEFAULT_Z_CURRENT
#define TMC_Z_CURRENT 500       // mA RMS
#else
#define TMC_Z_CURRENT DEFAULT_Z_CURRENT       // mA RMS
#endif

#define TMC_Z_HOLD_CURRENT_PCT 50
#define TMC_Z_HOMING_SEEK_SGT 22
#define TMC_Z_HOMING_FEED_SGT 22
#define TMC_Z_STEALTHCHOP TMC_STEALTHCHOP

#define TMC_Z_ADVANCED(motor) \
stepper[motor]->sg_filter(motor, 1); \
stepper[motor]->coolconf(motor, coolconf); \
stepper[motor]->chopper_timing(motor, chopper_timing);

#ifdef A_AXIS

#if TRINAMIC_MIXED_DRIVERS
#define TMC_A_ENABLE 0
#else
#define TMC_A_ENABLE 1 // Do not change
#endif
#define TMC_A_MONITOR 1
#define TMC_A_MICROSTEPS 16
#define TMC_A_R_SENSE R_SENSE   // mOhm

#ifndef DEFAULT_A_CURRENT
#define TMC_A_CURRENT 500       // mA RMS
#else
#define TMC_A_CURRENT DEFAULT_A_CURRENT       // mA RMS
#endif 

#define TMC_A_HOLD_CURRENT_PCT 50
#define TMC_A_HOMING_SEEK_SGT 22
#define TMC_A_HOMING_FEED_SGT 22
#define TMC_A_STEALTHCHOP TMC_STEALTHCHOP

#define TMC_A_ADVANCED(motor) \
stepper[motor]->sg_filter(motor, 1); \
stepper[motor]->coolconf(motor, coolconf); \
stepper[motor]->chopper_timing(motor, chopper_timing);

#endif

#ifdef B_AXIS

#if TRINAMIC_MIXED_DRIVERS
#define TMC_B_ENABLE 0
#else
#define TMC_B_ENABLE 1 // Do not change
#endif
#define TMC_B_MONITOR 1
#define TMC_B_MICROSTEPS 16
#define TMC_B_R_SENSE R_SENSE   // mOhm

#ifndef DEFAULT_B_CURRENT
#define TMC_B_CURRENT 500       // mA RMS
#else
#define TMC_B_CURRENT DEFAULT_B_CURRENT       // mA RMS
#endif 

#define TMC_B_HOLD_CURRENT_PCT 50
#define TMC_B_HOMING_SEEK_SGT 22
#define TMC_B_HOMING_FEED_SGT 22
#define TMC_B_STEALTHCHOP TMC_STEALTHCHOP

#define TMC_B_ADVANCED(motor) \
stepper[motor]->sg_filter(motor, 1); \
stepper[motor]->coolconf(motor, coolconf); \
stepper[motor]->chopper_timing(motor, chopper_timing);

#endif

#ifdef C_AXIS

#if TRINAMIC_MIXED_DRIVERS
#define TMC_C_ENABLE 0
#else
#define TMC_C_ENABLE 1 // Do not change
#endif
#define TMC_C_MONITOR 1
#define TMC_C_MICROSTEPS 16
#define TMC_C_R_SENSE R_SENSE   // mOhm

#ifndef DEFAULT_C_CURRENT
#define TMC_C_CURRENT 500       // mA RMS
#else
#define TMC_C_CURRENT DEFAULT_C_CURRENT       // mA RMS
#endif 

#define TMC_C_HOLD_CURRENT_PCT 50
#define TMC_C_HOMING_SEEK_SGT 22
#define TMC_C_HOMING_FEED_SGT 22
#define TMC_C_STEALTHCHOP TMC_STEALTHCHOP

#define TMC_C_ADVANCED(motor) \
stepper[motor]->sg_filter(motor, 1); \
stepper[motor]->coolconf(motor, coolconf); \
stepper[motor]->chopper_timing(motor, chopper_timing);

#endif

//

typedef struct {
    uint16_t current; // mA
    uint8_t hold_current_pct;
    uint16_t r_sense; // mOhm
    uint16_t microsteps;
    trinamic_mode_t mode;
    int16_t homing_seek_sensitivity;
    int16_t homing_feed_sensitivity;
} motor_settings_t;

typedef struct {
    axes_signals_t driver_enable;
    axes_signals_t homing_enable;
    motor_settings_t driver[N_AXIS];
} trinamic_settings_t;

typedef struct {
    uint8_t address;            // slave address, for UART Single Wire Interface drivers - can be overridden by driver interface
    motor_settings_t *settings; // for info only, do not modify
} trinamic_driver_config_t;

typedef void (*trinamic_on_drivers_init_ptr)(uint8_t n_motors, axes_signals_t enabled);
typedef void (*trinamic_on_driver_preinit_ptr)(motor_map_t motor, trinamic_driver_config_t *config);
typedef void (*trinamic_on_driver_postinit_ptr)(motor_map_t motor, const tmchal_t *driver);

typedef struct {
    trinamic_on_drivers_init_ptr on_drivers_init;
    trinamic_on_driver_preinit_ptr on_driver_preinit;
    trinamic_on_driver_postinit_ptr on_driver_postinit;
} trinamic_driver_if_t;

bool trinamic_init (void);
void trinamic_fault_handler (void);
void trinamic_warn_handler (void);
void trinamic_if_init (trinamic_driver_if_t *driver);

#endif // TRINAMIC_ENABLE

#endif
