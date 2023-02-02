#define N_AXIS                                             4
#define N_SPINDLE                                          1
#define BUILD_INFO                               "Am0k .cfg"
//#define MASLOW_ROUTER
//#define WALL_PLOTTER
//#define COREXY
#define ACCELERATION_TICKS_PER_SECOND                    100
#define REPORT_ECHO_LINE_RECEIVED                        Off 
#define TOOL_LENGTH_OFFSET_AXIS                           -1 
#define MINIMUM_JUNCTION_SPEED                          0.0f 
#define MINIMUM_FEED_RATE                               1.0f 
#define N_ARC_CORRECTION                                  12
#define SEGMENT_BUFFER_SIZE                               10

#define DEFAULT_STEP_PULSE_MICROSECONDS                10.0f          //$0
#define DEFAULT_STEPPER_IDLE_LOCK_TIME                   255          //$1
#define DEFAULT_STEP_SIGNALS_INVERT_MASK          0b00000000          //$2
#define DEFAULT_DIR_SIGNALS_INVERT_MASK           0b00000001          //$3
#define DEFAULT_ENABLE_SIGNALS_INVERT_MASK        0b00001111          //$4
#define DEFAULT_LIMIT_SIGNALS_INVERT_MASK         0b00000000          //$5
#define DEFAULT_PROBE_SIGNAL_INVERT                       On          //$6
#define DEFAULT_SPINDLE_ENABLE_OFF_WITH_ZERO_SPEED        On          //$9
#define DEFAULT_REPORT_MACHINE_POSITION                   On          //$10 (bit 0)
#define DEFAULT_REPORT_BUFFER_STATE                       On          //$10 (bit 1)
#define DEFAULT_REPORT_LINE_NUMBERS                       On          //$10 (bit 2) 
#define DEFAULT_REPORT_CURRENT_FEED_SPEED                 On          //$10 (bit 3) 
#define DEFAULT_REPORT_PIN_STATE                          On          //$10 (bit 4)
#define DEFAULT_REPORT_WORK_COORD_OFFSET                  On          //$10 (bit 5)
#define DEFAULT_REPORT_OVERRIDES                          On          //$10 (bit 6)
#define DEFAULT_REPORT_PROBE_COORDINATES                  On          //$10 (bit 7)
#define DEFAULT_REPORT_SYNC_ON_WCO_CHANGE                 On          //$10 (bit 8)
#define DEFAULT_REPORT_PARSER_STATE                      Off          //$10 (bit 9)
#define DEFAULT_REPORT_ALARM_SUBSTATE                    Off          //$10 (bit 10)
#define DEFAULT_REPORT_RUN_SUBSTATE                      Off          //$10 (bit 11)
#define DEFAULT_JUNCTION_DEVIATION                     0.01f          //$11
#define DEFAULT_ARC_TOLERANCE                         0.002f          //$12
#define DEFAULT_REPORT_INCHES                            Off          //$13
#define DEFAULT_CONTROL_SIGNALS_INVERT_MASK       0b00000000          //$14
#define DEFAULT_INVERT_COOLANT_FLOOD_PIN                 Off          //$15
#define DEFAULT_INVERT_COOLANT_MIST_PIN                  Off          //$15
#define DEFAULT_INVERT_SPINDLE_ENABLE_PIN                Off          //$16
#define DEFAULT_INVERT_SPINDLE_CCW_PIN                   Off          //$16
#define DEFAULT_INVERT_SPINDLE_PWM_PIN                   Off          //$16
#define DEFAULT_DISABLE_CONTROL_PINS_PULL_UP_MASK 0b00000000          //$17
#define DEFAULT_LIMIT_SIGNALS_PULLUP_DISABLE_MASK 0b00000000          //$18
#define DEFAULT_PROBE_SIGNAL_DISABLE_PULLUP              Off          //$19
#define DEFAULT_SOFT_LIMIT_ENABLE                         On          //$20
#define DEFAULT_HARD_LIMIT_ENABLE                         On          //$21
#define DEFAULT_HOMING_ENABLE                             On          //$22 (bit 0)
#define HOMING_SINGLE_AXIS_COMMANDS                      Off          //$22 (bit 1)
#define DEFAULT_HOMING_INIT_LOCK                          On          //$22 (bit 2)
#define HOMING_FORCE_SET_ORIGIN                          Off          //$22 (bit 3)
#define DEFAULT_HOMING_ALLOW_MANUAL                       On          //$22 (bit 4)
#define DEFAULT_HOMING_OVERRIDE_LOCKS                     On          //$22 (bit 5)
#define DEFAULT_HOMING_KEEP_STATUS_ON_RESET              Off          //$22 (bit 6)
#define DEFAULT_HOMING_DIR_MASK                   0b00000111          //$23
#define DEFAULT_HOMING_FEED_RATE                       25.0f          //$24
#define DEFAULT_HOMING_SEEK_RATE                      300.0f          //$25
#define DEFAULT_HOMING_DEBOUNCE_DELAY                    200          //$26
#define DEFAULT_HOMING_PULLOFF                          1.0f          //$27
#define DEFAULT_G73_RETRACT                             0.1f          //$28
#define DEFAULT_STEP_PULSE_DELAY                        0.0f          //$29
#define DEFAULT_SPINDLE_RPM_MAX                     12000.0f          //$30
#define DEFAULT_SPINDLE_RPM_MIN                      2000.0f          //$31
#define DEFAULT_LASER_MODE                               Off          //$32
#define DEFAULT_LATHE_MODE                               Off          //$32
#define DEFAULT_SPINDLE_PWM_FREQ                        5000          //$33
#define DEFAULT_SPINDLE_PWM_OFF_VALUE                   0.0f          //$34
#define DEFAULT_SPINDLE_PWM_MIN_VALUE                   0.0f          //$35
#define DEFAULT_SPINDLE_PWM_MAX_VALUE                 100.0f          //$36
#define DEFAULT_STEPPER_DEENERGIZE_MASK                    0          //$37
#define DEFAULT_SPINDLE_PPR                                0          //$38
#define DEFAULT_LEGACY_RTCOMMANDS                         On          //$39
#define DEFAULT_JOG_LIMIT_ENABLE                         Off          //$40
#define DEFAULT_PARKING_ENABLE                           Off          //$41 (bit 0)
#define DEFAULT_DEACTIVATE_PARKING_UPON_INIT             Off          //$41 (bit 1)
#define DEFAULT_ENABLE_PARKING_OVERRIDE_CONTROL          Off          //$41 (bit 2)
#define DEFAULT_PARKING_AXIS                      0b00000100          //$42
#define DEFAULT_N_HOMING_LOCATE_CYCLE                      1          //$43
#define DEFAULT_HOMING_CYCLE_0                    0b00000100          //$44
#define DEFAULT_HOMING_CYCLE_1                    0b00000011          //$45
#define DEFAULT_HOMING_CYCLE_2                             0          //$46
#define DEFAULT_HOMING_CYCLE_3                             0          //$47
#define DEFAULT_HOMING_CYCLE_4                             0          //$48
#define DEFAULT_HOMING_CYCLE_5                             0          //$49
#define DEFAULT_PARKING_PULLOUT_INCREMENT               5.0f          //$56
#define DEFAULT_PARKING_PULLOUT_RATE                  100.0f          //$57
#define DEFAULT_PARKING_TARGET                         -5.0f          //$58
#define DEFAULT_PARKING_RATE                          500.0f          //$59
#define DEFAULT_RESET_OVERRIDES                          Off          //$60
#define DEFAULT_DOOR_IGNORE_WHEN_IDLE                    Off          //$61
#define DEFAULT_SLEEP_ENABLE                             Off          //$62
#define DEFAULT_ENABLE_LASER_DURING_HOLD                 Off          //$63
#define DEFAULT_RESTORE_AFTER_FEED_HOLD                   On          //$63
#define DEFAULT_FORCE_INITIALIZATION_ALARM               Off          //$64
#define DEFAULT_ALLOW_FEED_OVERRIDE_DURING_PROBE_CYCLES  Off          //$65
#define DEFAULT_X_STEPS_PER_MM                       6400.0f          //$100
#define DEFAULT_Y_STEPS_PER_MM                       6400.0f          //$101
#define DEFAULT_Z_STEPS_PER_MM                       1600.0f          //$102
#define DEFAULT_A_STEPS_PER_MM                   8.88888889f          //$103
#define DEFAULT_X_MAX_RATE                            600.0f          //$110
#define DEFAULT_Y_MAX_RATE                            600.0f          //$111
#define DEFAULT_Z_MAX_RATE                            400.0f          //$112
#define DEFAULT_A_MAX_RATE           (1.0f * 360.0f * 60.0f)          //$113
#define DEFAULT_X_ACCELERATION       (12.0f * 60.0f * 60.0f)          //$120
#define DEFAULT_Y_ACCELERATION       (12.0f * 60.0f * 60.0f)          //$121
#define DEFAULT_Z_ACCELERATION       (10.0f * 60.0f * 60.0f)          //$122
#define DEFAULT_A_ACCELERATION      (180.0f * 60.0f * 60.0f)          //$123
#define DEFAULT_X_MAX_TRAVEL                          360.0f          //$130
#define DEFAULT_Y_MAX_TRAVEL                          233.0f          //$131
#define DEFAULT_Z_MAX_TRAVEL                           90.0f          //$132
#define DEFAULT_A_MAX_TRAVEL                            0.0f          //$133
#define DEFAULT_SPINDLE_AT_SPEED_TOLERANCE              0.0f          //$340
#define DEFAULT_TOOLCHANGE_MODE                            0          //$341
#define DEFAULT_TOOLCHANGE_PROBING_DISTANCE               30          //$342
#define DEFAULT_TOOLCHANGE_FEED_RATE                   25.0f          //$343
#define DEFAULT_TOOLCHANGE_SEEK_RATE                  200.0f          //$344
#define DEFAULT_TOOLCHANGE_PULLOFF_RATE               200.0f          //$345
#define DEFAULT_TOOLCHANGE_NO_RESTORE_POSITION           Off          //$346
#define DEFAULT_AXIS_ROTATIONAL_MASK                       0          //$376
#define DEFAULT_DISABLE_G92_PERSISTENCE                  Off          //$384
#define DEFAULT_PLANNER_BUFFER_BLOCKS                     35          //$398
#define DEFAULT_AUTOREPORT_INTERVAL                        0          //$481
#define DEFAULT_TIMEZONE_OFFSET                         0.0f          //$482