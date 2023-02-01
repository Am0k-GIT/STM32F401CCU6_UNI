## Конфигурирование GRBL HAL для работы с платой STM32F401CCU6_UNI

Добавляем файл определения пинов: `..\Inc\blackpill_map.h`

В файлe `..\Inc\my_machine.h` проверяем определение:

```C
#define BOARD_BLACKPILL
```

В файл `..\platformio.ini` добавляем определение окружения:

```C
[env:blackpill_f401cc]
board = blackpill_f401cc
board_build.ldscript = STM32F401CCUX_FLASH.ld
build_flags = ${common.build_flags}
  # See Inc/my_machine.h for options
  -D BOARD_BLACKPILL=
  -D USB_SERIAL_CDC=1 
  # Uncomment to enable Spindle PWM output on the SpinDir pin
  #-D PROTONEER_SPINDLE_PWM=
lib_deps = ${common.lib_deps}
lib_extra_dirs = ${common.lib_extra_dirs}
# Alternatively, place the .pio/build/<env name>/firmware.bin on the NODE_F4xxRE drive
; change MCU frequency
upload_protocol = dfu 
```

Конфигурируем файл `..\grbl\config.h`:

```C
#define N_AXIS 4
#define N_SPINDLE 1
#define DEFAULT_SOFT_LIMIT_ENABLE
#define DEFAULT_HARD_LIMIT_ENABLE
#define DEFAULT_INVERT_PROBE_BIT
#define DEFAULT_HOMING_ENABLE
#define DEFAULT_HOMING_ALLOW_MANUAL
#define DEFAULT_HOMING_INIT_LOCK
#define HOMING_CYCLE_0 (Z_AXIS_BIT)
#define HOMING_CYCLE_1 (X_AXIS_BIT|Y_AXIS_BIT)
#define HOMING_FORCE_SET_ORIGIN     1

#define DEFAULT_ACCELERATION (10.0f * 60.0f * 60.0f)
#define DEFAULT_X_STEPS_PER_MM 6400.0f
#define DEFAULT_Y_STEPS_PER_MM 6400.0f
#define DEFAULT_Z_STEPS_PER_MM 1600.0f
#define DEFAULT_X_MAX_RATE 600.0f
#define DEFAULT_Y_MAX_RATE 600.0f
#define DEFAULT_Z_MAX_RATE 400.0f
#define DEFAULT_X_ACCELERATION (12.0f * 60.0f * 60.0f)
#define DEFAULT_Y_ACCELERATION (12.0f * 60.0f * 60.0f)
#define DEFAULT_Z_ACCELERATION (10.0f * 60.0f * 60.0f)
#define DEFAULT_X_MAX_TRAVEL 360.0f
#define DEFAULT_Y_MAX_TRAVEL 233.0f
#define DEFAULT_Z_MAX_TRAVEL 90.0f
#define DEFAULT_SPINDLE_PWM_FREQ 5000
#define DEFAULT_SPINDLE_PWM_OFF_VALUE 0.0f
#define DEFAULT_SPINDLE_PWM_MIN_VALUE 0.0f
#define DEFAULT_SPINDLE_PWM_MAX_VALUE 100.0f
#define DEFAULT_SPINDLE_RPM_MAX 12000.0
#define DEFAULT_SPINDLE_RPM_MIN 2000.0
#define DEFAULT_STEP_PULSE_MICROSECONDS 10.0f                       //$0
#define DEFAULT_STEP_PULSE_DELAY 0.0f                               //$29
#define DEFAULT_STEPPING_INVERT_MASK 0                              //$2
#define DEFAULT_DIRECTION_INVERT_MASK 1                             //$3
#define DEFAULT_STEPPER_IDLE_LOCK_TIME 255                          //$1
#define INVERT_LIMIT_BIT_MASK 0                                     //$5
#define DEFAULT_SOFT_LIMIT_ENABLE 1                                 //$20
#define DEFAULT_HARD_LIMIT_ENABLE 1                                 //$21
#define DEFAULT_INVERT_PROBE_BIT 1                                  //$6
#define DEFAULT_JUNCTION_DEVIATION 0.01f                            //$11

#define DEFAULT_A_STEPS_PER_MM 8.88888889f
#define DEFAULT_A_MAX_RATE (1.0f * 360.0f * 60.0f)
#define DEFAULT_A_ACCELERATION (180.0f * 60.0f * 60.0f)
#define DEFAULT_A_MAX_TRAVEL 0.0f
#define DEFAULT_HOMING_DIR_MASK 7
#define DEFAULT_HOMING_FEED_RATE 25.0f
#define DEFAULT_HOMING_SEEK_RATE 300.0f
#define DEFAULT_HOMING_DEBOUNCE_DELAY 200
#define DEFAULT_HOMING_PULLOFF 1.0f
```

Прошиваем выбрав окружение `env:blackpill_f401cc`. 
После прошивки и запуска GRBL стираем и восстанавливает настройки до значений по умолчанию командой `$RST=$`.
Проверяем настройки командой `$$`.