################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../eeprom/eeprom_24AAxxx.c \
../eeprom/eeprom_24LC16B.c 

OBJS += \
./eeprom/eeprom_24AAxxx.o \
./eeprom/eeprom_24LC16B.o 

C_DEPS += \
./eeprom/eeprom_24AAxxx.d \
./eeprom/eeprom_24LC16B.d 


# Each subdirectory must supply rules for building sources it contributes
eeprom/%.o eeprom/%.su: ../eeprom/%.c eeprom/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI" -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../USB_DEVICE/App -I../USB_DEVICE/Target -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-eeprom

clean-eeprom:
	-$(RM) ./eeprom/eeprom_24AAxxx.d ./eeprom/eeprom_24AAxxx.o ./eeprom/eeprom_24AAxxx.su ./eeprom/eeprom_24LC16B.d ./eeprom/eeprom_24LC16B.o ./eeprom/eeprom_24LC16B.su

.PHONY: clean-eeprom

