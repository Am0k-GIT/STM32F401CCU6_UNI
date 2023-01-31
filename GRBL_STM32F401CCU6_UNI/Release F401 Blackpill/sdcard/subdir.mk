################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sdcard/fs_fatfs.c \
../sdcard/fs_littlefs.c \
../sdcard/sdcard.c \
../sdcard/ymodem.c 

OBJS += \
./sdcard/fs_fatfs.o \
./sdcard/fs_littlefs.o \
./sdcard/sdcard.o \
./sdcard/ymodem.o 

C_DEPS += \
./sdcard/fs_fatfs.d \
./sdcard/fs_littlefs.d \
./sdcard/sdcard.d \
./sdcard/ymodem.d 


# Each subdirectory must supply rules for building sources it contributes
sdcard/%.o sdcard/%.su: ../sdcard/%.c sdcard/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI" -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../USB_DEVICE/App -I../USB_DEVICE/Target -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-sdcard

clean-sdcard:
	-$(RM) ./sdcard/fs_fatfs.d ./sdcard/fs_fatfs.o ./sdcard/fs_fatfs.su ./sdcard/fs_littlefs.d ./sdcard/fs_littlefs.o ./sdcard/fs_littlefs.su ./sdcard/sdcard.d ./sdcard/sdcard.o ./sdcard/sdcard.su ./sdcard/ymodem.d ./sdcard/ymodem.o ./sdcard/ymodem.su

.PHONY: clean-sdcard

