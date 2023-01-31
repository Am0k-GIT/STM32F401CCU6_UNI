## Keypad plugin

This plugin can be used for jogging, controlling feedhold, cycle start etc. via single character commands.
The command characters can be delivered either via an I2C or an UART port.

The plugin is enabled in _my_machine.h_ by removing the comment from the `#define KEYPAD_ENABLE` line and changing it if required:  
`#define KEYPAD_ENABLE 1` enables I2C mode, an additional strobe pin is required to signal keypresses.  
`#define KEYPAD_ENABLE 2` enables UART mode.

See the [core wiki](https://github.com/grblHAL/core/wiki/MPG-and-DRO-interfaces#keypad-plugin) for more details.

[Settings](https://github.com/terjeio/grblHAL/wiki/Additional-or-extended-settings#jogging) are provided for jog speed and distance for step, slow and fast jogging.

Character to action map:

|Character | Action                                     |
|----------|--------------------------------------------|
| `!`      | Enter feed hold                            |
| `~`      | Cycle start                                |
| `0x8B`   | Enable MPG full control<sup>1</sup>        |
| `0x85`   | Cancel jog motion<sup>2</sup>              |
| `0`      | Enable step mode jogging                   |
| `1`      | Enable slow jogging mode                   |
| `2`      | Enable fast jogging mode                   |
| `h`      | Select next jog mode                       |
| `H`      | Home machine                               |
| `R`      | Continuous jog X+                          |
| `L`      | Continuous jog X-                          |
| `F`      | Continuous jog Y+                          |
| `B`      | Continuous jog Y-                          |
| `U`      | Continuous jog Z+                          |
| `D`      | Continuous jog Z-                          |
| `r`      | Continuous jog X+Y+                        |
| `q`      | Continuous jog X+Y-                        |
| `s`      | Continuous jog X-Y+                        |
| `t`      | Continuous jog X-Y-                        |
| `w`      | Continuous jog X+Z+                        |
| `v`      | Continuous jog X+Z-                        |
| `u`      | Continuous jog X-Z+                        |
| `x`      | Continuous jog X-Z-                        |
| `0x84`   | Toggle safety door open status             |
| `0x88`   | Toggle optional stop mode                  |
| `0x89`   | Toggle single block execution mode         |
| `0x8A`   | Toggle Fan 0 output<sup>3</sup>            |
| `I`      | Restore feed override value to 100%        |
| `0x90`   | Restore feed override value to 100%        |
| `i`      | Increase feed override value 10%           |
| `0x91`   | Increase feed override value 10%           |
| `j`      | Decrease feed override value 10%           |
| `0x92`   | Decrease feed override value 10%           |
| `0x93`   | Increase feed override value 1%            |
| `0x94`   | Decrease feed override value 1%            |
| `0x95`   | Restore rapids override value to 100%      |
| `0x96`   | Set rapids override to 50%                 |
| `0x97`   | Set rapids override to 25%                 |
| `K`      | Restore spindle RPM override value to 100% |
| `0x99`   | Restore spindle RPM override value to 100% |
| `k`      | Increase spindle RPM override value 10%    |
| `0x9A`   | Increase spindle RPM override value 10%    |
| `z`      | Decrease spindle RPM override value 10%    |
| `0x9B`   | Decrease spindle RPM override value 10%    |
| `0x9C`   | Increase spindle RPM override value 1%     |
| `0x9D`   | Decrease spindle RPM override value 1%     |
| `0x9E`   | Toggle spindle stop override<sup>4</sup>   |
| `C`      | Toggle flood coolant output                |
| `0xA0`   | Toggle flood coolant output                |
| `M`      | Toggle mist coolant output                 |
| `0xA1`   | Toggle mist coolant output                 |

<sup>1</sup> Only available if MPG mode is enabled. Build 20220105 or later is required.  
<sup>2</sup> Only available in UART mode, it is recommended to send this on all key up events. In I2C mode the strobe line going high is used to signal jog cancel.  
<sup>3</sup> The [fans plugin](https://github.com/grblHAL/Plugin_fans) is required.  
<sup>4</sup> Only available when the machine is in _Hold_ state.  

---

Dependencies:

An app providing input such as [this implementation](https://github.com/terjeio/I2C-interface-for-4x4-keyboard).

Driver (and app) must support I2C communication and a keypad strobe interrupt signal or have a free UART port depending on the mode selected.

---
2022-01-08
