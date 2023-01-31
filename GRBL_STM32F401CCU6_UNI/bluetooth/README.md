## Bluetooth plugins

This plugin currently adds support for the [HC-05 Bluetooth module](https://duckduckgo.com/?t=ffsb&q=HC-05+Bluetooth+module.&ia=web).

It has an auto-configure mode to set the module device name and baud rate.
It also supports automatic switching of the active stream to Bluetooth and back on connect/disconnect if a native USB port or another UART port is the default.
The grblHAL welcome message will be sent to the Bluetooth stream when a connection is established so that senders requiring this is kept happy.

To achieve this an UART port and an interrupt capable auxillary input pin is required - the input pin has to be connected to the module `STATE` pin.
The plugin will claim the highest numbered _Aux input_ pin, this can be found by either checking the board map file or
listing the available pins with the `$pins` system command. E.g. for the [iMXRT1062/Teensy 4.1 T41U5XBB](https://github.com/grblHAL/iMXRT1062/blob/master/grblHAL_Teensy4/src/T41U5XBB_map.h) board this will normally be pin 35:

To enable this plugin uncomment the `#define BLUETOOTH_ENABLE 1` line in the driver _machine.h_ file and recompile.

```
...
[PIN:22,Z limit min]
[PIN:36,Aux input 0]
[PIN:30,Aux input 1]
[PIN:33,Aux input 2]
[PIN:35,Aux input 3]
[PIN:2,X step]
...
```

__NOTE:__ Pins may be claimed by other plugins so it might not always be the highets numbered pin.  
__NOTE:__ The claimed pin is no longer available for `M62`-`M66` commands.

#### Auto configure

Ensure the `$377` setting is set to `0` \(it can be found in the _Bluetooth_ group if your sender supports grouping of settings\).
Power up the controller and the module while pressing down the AT-mode switch on the module.
This will change the controller baud rate to 38400 and it will then send the AT commands required for configuration.
Reports will be sent to the default com port indicating success or failure. Success is also indicated by `$377` beeing set to `1`. 
After auto configuration is completed cycle power to both the module and the controller to start normal operation.

#### Manual configure
If the module is already configured for correct operation \(baud rate set to 115200 baud\) then automatic switching can be enabled by setting `$377` to `1`.

__NOTE:__ This is a preview version.

---

Available for some board maps for the [iMXRT1062](https://github.com/grblHAL/iMXRT1062) \(Teensy 4.1\), [STM32F4xx](https://github.com/grblHAL/STM32F4xx), [STM32F7xx](https://github.com/grblHAL/STM32F7xx) and [SAM3X8E](https://github.com/grblHAL/SAM3X8E) \(Arduino Due\) drivers.  
Support for further drivers may be added on request.

---
2022-07-19
