################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../spindle/vfd/gs20.c \
../spindle/vfd/h100.c \
../spindle/vfd/huanyang.c \
../spindle/vfd/modvfd.c \
../spindle/vfd/spindle.c \
../spindle/vfd/yl620.c 

OBJS += \
./spindle/vfd/gs20.o \
./spindle/vfd/h100.o \
./spindle/vfd/huanyang.o \
./spindle/vfd/modvfd.o \
./spindle/vfd/spindle.o \
./spindle/vfd/yl620.o 

C_DEPS += \
./spindle/vfd/gs20.d \
./spindle/vfd/h100.d \
./spindle/vfd/huanyang.d \
./spindle/vfd/modvfd.d \
./spindle/vfd/spindle.d \
./spindle/vfd/yl620.d 


# Each subdirectory must supply rules for building sources it contributes
spindle/vfd/%.o spindle/vfd/%.su: ../spindle/vfd/%.c spindle/vfd/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI" -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../USB_DEVICE/App -I../USB_DEVICE/Target -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-spindle-2f-vfd

clean-spindle-2f-vfd:
	-$(RM) ./spindle/vfd/gs20.d ./spindle/vfd/gs20.o ./spindle/vfd/gs20.su ./spindle/vfd/h100.d ./spindle/vfd/h100.o ./spindle/vfd/h100.su ./spindle/vfd/huanyang.d ./spindle/vfd/huanyang.o ./spindle/vfd/huanyang.su ./spindle/vfd/modvfd.d ./spindle/vfd/modvfd.o ./spindle/vfd/modvfd.su ./spindle/vfd/spindle.d ./spindle/vfd/spindle.o ./spindle/vfd/spindle.su ./spindle/vfd/yl620.d ./spindle/vfd/yl620.o ./spindle/vfd/yl620.su

.PHONY: clean-spindle-2f-vfd

