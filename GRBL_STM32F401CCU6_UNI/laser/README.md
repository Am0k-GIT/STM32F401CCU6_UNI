## Laser related plugins

### Laser PPI

Under development. Adds 3 M-codes for controlling PPI (Pulse Per Inch) mode for lasers.

* `M126 P-` turns PPI mode on or off. The P-word specifies the mode. `0` = off, `1` = on.
* `M127 P-` The P-word specifies the PPI value. Default value on startup is `600`.
* `M128 P-` The P-word specifies the pulse length in microseconds. Default value on startup is `1500`.

__NOTE:__ These M-codes are not standard and may change in a later release. 

A description of what PPI is and how it works can be found [here](https://www.buildlog.net/blog/2011/12/getting-more-power-and-cutting-accuracy-out-of-your-home-built-laser-system/).

I have a customized version of [ioSender](https://github.com/terjeio/Grbl-GCode-Sender) that I use for my CO2-laser, this has not yet been published but I may do so if this is of interest.

Dependencies:

Driver must support pulsing spindle on pin. Only for processors having a FPU that can be used in an interrupt context.

### Laser coolant

Under development. Adds monitoring for \(tube\) coolant controlled by `M8`, configurable by settings.

* `$378` - time in seconds after coolant is turned on before an alarm is raised if the coolant ok signal is not asserted.
* `$379` - time in minutes after program end before coolant is turned off. \(WIP\)
* `$380` - min coolant temperature allowed. \(WIP\)
* `$381` - max coolant temperature allowed. \(WIP\)
* `$382` - input value offset for temperature calculation. \(WIP\)
* `$383` - input value gain factor for temperature calculation. \(WIP\)

WIP - Work In Progress.

Dependencies:

Driver must have at least one [ioports port](../../templates/ioports.c) input available for the coolant ok signal.
An optional analog input port is required for coolant temperature monitoring.

### LightBurn clusters

Under development, experimental.

Add `#define LB_CLUSTERS_ENABLE 1` to your _my_machine.h_ to enable.  

The plugin unpacks the clustered S command from the input stream and deliveres standard gcode to the parser.

---
2022-09-25
