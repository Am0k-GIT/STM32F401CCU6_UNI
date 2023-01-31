################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../trinamic/common.c \
../trinamic/tmc2130.c \
../trinamic/tmc2130hal.c \
../trinamic/tmc2209.c \
../trinamic/tmc2209hal.c \
../trinamic/tmc26x.c \
../trinamic/tmc5160.c \
../trinamic/tmc5160hal.c \
../trinamic/tmc_interface.c 

OBJS += \
./trinamic/common.o \
./trinamic/tmc2130.o \
./trinamic/tmc2130hal.o \
./trinamic/tmc2209.o \
./trinamic/tmc2209hal.o \
./trinamic/tmc26x.o \
./trinamic/tmc5160.o \
./trinamic/tmc5160hal.o \
./trinamic/tmc_interface.o 

C_DEPS += \
./trinamic/common.d \
./trinamic/tmc2130.d \
./trinamic/tmc2130hal.d \
./trinamic/tmc2209.d \
./trinamic/tmc2209hal.d \
./trinamic/tmc26x.d \
./trinamic/tmc5160.d \
./trinamic/tmc5160hal.d \
./trinamic/tmc_interface.d 


# Each subdirectory must supply rules for building sources it contributes
trinamic/%.o trinamic/%.su: ../trinamic/%.c trinamic/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI" -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../USB_DEVICE/App -I../USB_DEVICE/Target -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-trinamic

clean-trinamic:
	-$(RM) ./trinamic/common.d ./trinamic/common.o ./trinamic/common.su ./trinamic/tmc2130.d ./trinamic/tmc2130.o ./trinamic/tmc2130.su ./trinamic/tmc2130hal.d ./trinamic/tmc2130hal.o ./trinamic/tmc2130hal.su ./trinamic/tmc2209.d ./trinamic/tmc2209.o ./trinamic/tmc2209.su ./trinamic/tmc2209hal.d ./trinamic/tmc2209hal.o ./trinamic/tmc2209hal.su ./trinamic/tmc26x.d ./trinamic/tmc26x.o ./trinamic/tmc26x.su ./trinamic/tmc5160.d ./trinamic/tmc5160.o ./trinamic/tmc5160.su ./trinamic/tmc5160hal.d ./trinamic/tmc5160hal.o ./trinamic/tmc5160hal.su ./trinamic/tmc_interface.d ./trinamic/tmc_interface.o ./trinamic/tmc_interface.su

.PHONY: clean-trinamic

