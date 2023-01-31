################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Startup/startup_stm32f401ccux.s 

OBJS += \
./Startup/startup_stm32f401ccux.o 

S_DEPS += \
./Startup/startup_stm32f401ccux.d 


# Each subdirectory must supply rules for building sources it contributes
Startup/%.o: ../Startup/%.s Startup/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -c -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/FatFs" -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"D:/System/GoogleDrive/Open Hardware/STM32F401CCU6_UNI/GRBL_STM32F401CCU6_UNI/Inc" -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-Startup

clean-Startup:
	-$(RM) ./Startup/startup_stm32f401ccux.d ./Startup/startup_stm32f401ccux.o

.PHONY: clean-Startup

