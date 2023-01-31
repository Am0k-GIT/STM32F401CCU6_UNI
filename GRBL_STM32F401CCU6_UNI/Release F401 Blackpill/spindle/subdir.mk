################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../spindle/modbus.c \
../spindle/select.c 

OBJS += \
./spindle/modbus.o \
./spindle/select.o 

C_DEPS += \
./spindle/modbus.d \
./spindle/select.d 


# Each subdirectory must supply rules for building sources it contributes
spindle/%.o spindle/%.su: ../spindle/%.c spindle/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI" -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../USB_DEVICE/App -I../USB_DEVICE/Target -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-spindle

clean-spindle:
	-$(RM) ./spindle/modbus.d ./spindle/modbus.o ./spindle/modbus.su ./spindle/select.d ./spindle/select.o ./spindle/select.su

.PHONY: clean-spindle

