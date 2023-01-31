## Spindle plugins

This plugin adds support for VFD spindles via ModBus (RS485).

VFD spindle support is enabled in _my_machine.h_ by uncommenting `\\#define VFD_SPINDLE` and changing the VFD spindle number to the desired type or `-1` for all.  
A list of supported VFDs and associated spindle numbers can be found [here](./shared.h).

If all spindles or dual spindle is enabled the active spindle is configured by setting `$395`.  
Note that the setting value not the same as the VFD spindle number used in _my_machine.h_, either use a sender that supports grblHAL setting enumerations
 for configuration or output a list with the `$$=395` command to find the correct setting value.

Switching between the configured VFD spindle and PWM output is possible via a M-code, currently `M104`: `M104P0` for PWM (laser) and `M104P1` for VFD.  
Available when `\\#define DUAL_SPINDLE` is uncommented in _my_machine.h_ for drivers that has support.

If all spindles are enabled `M104Q<n>` can be used to switch between them without changing the configuration. `<n>` is a spindle number as listed by the `$$=395` command.  

The MODVFD spindle uses different register values for the control and RPM functions. The functionality is similar to the [VFDMOD](https://github.com/aekhv/vfdmod) component from LinuxCNC.

__NOTE:__ `M104` does not change the `$32` setting for machine mode, the setting will still be used to select the spindle at startup.  
__NOTE:__ currently all VFD spindles use the same ModBus address, configured by setting `$360` \(default 1\).  

---
2022-06-18
