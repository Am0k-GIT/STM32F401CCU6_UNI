################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include trinamic/subdir.mk
-include spindle/vfd/subdir.mk
-include spindle/subdir.mk
-include sdcard/subdir.mk
-include odometer/subdir.mk
-include motors/subdir.mk
-include laser/subdir.mk
-include keypad/subdir.mk
-include grbl/subdir.mk
-include eeprom/subdir.mk
-include bluetooth/subdir.mk
-include USB_DEVICE/Target/subdir.mk
-include USB_DEVICE/App/subdir.mk
-include Startup/subdir.mk
-include Src/subdir.mk
-include Middlewares/ST/STM32_USB_Device_Library/Core/Src/subdir.mk
-include Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/subdir.mk
-include FatFs/subdir.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := grblHAL F401 Blackpill
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
grblHAL\ F401\ Blackpill.elf \

MAP_FILES += \
grblHAL\ F401\ Blackpill.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
grblHAL\ F401\ Blackpill.list \


# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: grblHAL\ F401\ Blackpill.elf secondary-outputs

# Tool invocations
grblHAL\ F401\ Blackpill.elf grblHAL\ F401\ Blackpill.map: $(OBJS) $(USER_OBJS) D:\System\GoogleDrive\Open\ Hardware\STM32F401CCU6_UNI\GRBL_STM32F401CCU6_UNI\STM32F401CCUX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "grblHAL F401 Blackpill.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"D:\System\GoogleDrive\Open Hardware\STM32F401CCU6_UNI\GRBL_STM32F401CCU6_UNI\STM32F401CCUX_FLASH.ld" --specs=nosys.specs -Wl,-Map="grblHAL F401 Blackpill.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

grblHAL\ F401\ Blackpill.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "grblHAL F401 Blackpill.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) default.size.stdout grblHAL\ F401\ Blackpill.elf grblHAL\ F401\ Blackpill.list grblHAL\ F401\ Blackpill.map grblHAL\\ F401\\ Blackpill.elf
	-@echo ' '

post-build:
	arm-none-eabi-objcopy -O ihex "grblHAL F401 Blackpill.elf" "grblHAL F401 Blackpill.hex"
	arm-none-eabi-size "grblHAL F401 Blackpill.elf"
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified post-build

-include ../makefile.targets
