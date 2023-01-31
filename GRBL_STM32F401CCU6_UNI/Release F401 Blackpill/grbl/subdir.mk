################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../grbl/alarms.c \
../grbl/coolant_control.c \
../grbl/corexy.c \
../grbl/errors.c \
../grbl/gcode.c \
../grbl/grbllib.c \
../grbl/ioports.c \
../grbl/machine_limits.c \
../grbl/maslow.c \
../grbl/motion_control.c \
../grbl/my_plugin.c \
../grbl/ngc_expr.c \
../grbl/ngc_params.c \
../grbl/nuts_bolts.c \
../grbl/nvs_buffer.c \
../grbl/override.c \
../grbl/pid.c \
../grbl/planner.c \
../grbl/protocol.c \
../grbl/regex.c \
../grbl/report.c \
../grbl/settings.c \
../grbl/sleep.c \
../grbl/spindle_control.c \
../grbl/state_machine.c \
../grbl/stepper.c \
../grbl/stream.c \
../grbl/system.c \
../grbl/tool_change.c \
../grbl/vfs.c \
../grbl/wall_plotter.c 

OBJS += \
./grbl/alarms.o \
./grbl/coolant_control.o \
./grbl/corexy.o \
./grbl/errors.o \
./grbl/gcode.o \
./grbl/grbllib.o \
./grbl/ioports.o \
./grbl/machine_limits.o \
./grbl/maslow.o \
./grbl/motion_control.o \
./grbl/my_plugin.o \
./grbl/ngc_expr.o \
./grbl/ngc_params.o \
./grbl/nuts_bolts.o \
./grbl/nvs_buffer.o \
./grbl/override.o \
./grbl/pid.o \
./grbl/planner.o \
./grbl/protocol.o \
./grbl/regex.o \
./grbl/report.o \
./grbl/settings.o \
./grbl/sleep.o \
./grbl/spindle_control.o \
./grbl/state_machine.o \
./grbl/stepper.o \
./grbl/stream.o \
./grbl/system.o \
./grbl/tool_change.o \
./grbl/vfs.o \
./grbl/wall_plotter.o 

C_DEPS += \
./grbl/alarms.d \
./grbl/coolant_control.d \
./grbl/corexy.d \
./grbl/errors.d \
./grbl/gcode.d \
./grbl/grbllib.d \
./grbl/ioports.d \
./grbl/machine_limits.d \
./grbl/maslow.d \
./grbl/motion_control.d \
./grbl/my_plugin.d \
./grbl/ngc_expr.d \
./grbl/ngc_params.d \
./grbl/nuts_bolts.d \
./grbl/nvs_buffer.d \
./grbl/override.d \
./grbl/pid.d \
./grbl/planner.d \
./grbl/protocol.d \
./grbl/regex.d \
./grbl/report.d \
./grbl/settings.d \
./grbl/sleep.d \
./grbl/spindle_control.d \
./grbl/state_machine.d \
./grbl/stepper.d \
./grbl/stream.d \
./grbl/system.d \
./grbl/tool_change.d \
./grbl/vfs.d \
./grbl/wall_plotter.d 


# Each subdirectory must supply rules for building sources it contributes
grbl/%.o grbl/%.su: ../grbl/%.c grbl/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI" -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../USB_DEVICE/App -I../USB_DEVICE/Target -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-grbl

clean-grbl:
	-$(RM) ./grbl/alarms.d ./grbl/alarms.o ./grbl/alarms.su ./grbl/coolant_control.d ./grbl/coolant_control.o ./grbl/coolant_control.su ./grbl/corexy.d ./grbl/corexy.o ./grbl/corexy.su ./grbl/errors.d ./grbl/errors.o ./grbl/errors.su ./grbl/gcode.d ./grbl/gcode.o ./grbl/gcode.su ./grbl/grbllib.d ./grbl/grbllib.o ./grbl/grbllib.su ./grbl/ioports.d ./grbl/ioports.o ./grbl/ioports.su ./grbl/machine_limits.d ./grbl/machine_limits.o ./grbl/machine_limits.su ./grbl/maslow.d ./grbl/maslow.o ./grbl/maslow.su ./grbl/motion_control.d ./grbl/motion_control.o ./grbl/motion_control.su ./grbl/my_plugin.d ./grbl/my_plugin.o ./grbl/my_plugin.su ./grbl/ngc_expr.d ./grbl/ngc_expr.o ./grbl/ngc_expr.su ./grbl/ngc_params.d ./grbl/ngc_params.o ./grbl/ngc_params.su ./grbl/nuts_bolts.d ./grbl/nuts_bolts.o ./grbl/nuts_bolts.su ./grbl/nvs_buffer.d ./grbl/nvs_buffer.o ./grbl/nvs_buffer.su ./grbl/override.d ./grbl/override.o ./grbl/override.su ./grbl/pid.d ./grbl/pid.o ./grbl/pid.su ./grbl/planner.d ./grbl/planner.o ./grbl/planner.su ./grbl/protocol.d ./grbl/protocol.o ./grbl/protocol.su ./grbl/regex.d ./grbl/regex.o ./grbl/regex.su ./grbl/report.d ./grbl/report.o ./grbl/report.su ./grbl/settings.d ./grbl/settings.o ./grbl/settings.su ./grbl/sleep.d ./grbl/sleep.o ./grbl/sleep.su ./grbl/spindle_control.d ./grbl/spindle_control.o ./grbl/spindle_control.su ./grbl/state_machine.d ./grbl/state_machine.o ./grbl/state_machine.su ./grbl/stepper.d ./grbl/stepper.o ./grbl/stepper.su ./grbl/stream.d ./grbl/stream.o ./grbl/stream.su ./grbl/system.d ./grbl/system.o ./grbl/system.su ./grbl/tool_change.d ./grbl/tool_change.o ./grbl/tool_change.su ./grbl/vfs.d ./grbl/vfs.o ./grbl/vfs.su ./grbl/wall_plotter.d ./grbl/wall_plotter.o ./grbl/wall_plotter.su

.PHONY: clean-grbl

