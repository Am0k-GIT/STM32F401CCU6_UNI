################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/btt_skr_1.1.c \
../Src/btt_skr_2.0.c \
../Src/diskio.c \
../Src/driver.c \
../Src/flash.c \
../Src/flexi_hal.c \
../Src/i2c.c \
../Src/ioports.c \
../Src/main.c \
../Src/serial.c \
../Src/spi.c \
../Src/st_morpho.c \
../Src/st_morpo_dac.c \
../Src/stm32f4xx_hal_msp.c \
../Src/stm32f4xx_it.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/system_stm32f4xx.c \
../Src/tmc_uart.c \
../Src/usb_serial.c 

OBJS += \
./Src/btt_skr_1.1.o \
./Src/btt_skr_2.0.o \
./Src/diskio.o \
./Src/driver.o \
./Src/flash.o \
./Src/flexi_hal.o \
./Src/i2c.o \
./Src/ioports.o \
./Src/main.o \
./Src/serial.o \
./Src/spi.o \
./Src/st_morpho.o \
./Src/st_morpo_dac.o \
./Src/stm32f4xx_hal_msp.o \
./Src/stm32f4xx_it.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/system_stm32f4xx.o \
./Src/tmc_uart.o \
./Src/usb_serial.o 

C_DEPS += \
./Src/btt_skr_1.1.d \
./Src/btt_skr_2.0.d \
./Src/diskio.d \
./Src/driver.d \
./Src/flash.d \
./Src/flexi_hal.d \
./Src/i2c.d \
./Src/ioports.d \
./Src/main.d \
./Src/serial.d \
./Src/spi.d \
./Src/st_morpho.d \
./Src/st_morpo_dac.d \
./Src/stm32f4xx_hal_msp.d \
./Src/stm32f4xx_it.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/system_stm32f4xx.d \
./Src/tmc_uart.d \
./Src/usb_serial.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F401xC -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI" -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Inc -I../Drivers/CMSIS/Include -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../USB_DEVICE/App -I../USB_DEVICE/Target -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/btt_skr_1.1.d ./Src/btt_skr_1.1.o ./Src/btt_skr_1.1.su ./Src/btt_skr_2.0.d ./Src/btt_skr_2.0.o ./Src/btt_skr_2.0.su ./Src/diskio.d ./Src/diskio.o ./Src/diskio.su ./Src/driver.d ./Src/driver.o ./Src/driver.su ./Src/flash.d ./Src/flash.o ./Src/flash.su ./Src/flexi_hal.d ./Src/flexi_hal.o ./Src/flexi_hal.su ./Src/i2c.d ./Src/i2c.o ./Src/i2c.su ./Src/ioports.d ./Src/ioports.o ./Src/ioports.su ./Src/main.d ./Src/main.o ./Src/main.su ./Src/serial.d ./Src/serial.o ./Src/serial.su ./Src/spi.d ./Src/spi.o ./Src/spi.su ./Src/st_morpho.d ./Src/st_morpho.o ./Src/st_morpho.su ./Src/st_morpo_dac.d ./Src/st_morpo_dac.o ./Src/st_morpo_dac.su ./Src/stm32f4xx_hal_msp.d ./Src/stm32f4xx_hal_msp.o ./Src/stm32f4xx_hal_msp.su ./Src/stm32f4xx_it.d ./Src/stm32f4xx_it.o ./Src/stm32f4xx_it.su ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/system_stm32f4xx.d ./Src/system_stm32f4xx.o ./Src/system_stm32f4xx.su ./Src/tmc_uart.d ./Src/tmc_uart.o ./Src/tmc_uart.su ./Src/usb_serial.d ./Src/usb_serial.o ./Src/usb_serial.su

.PHONY: clean-Src

