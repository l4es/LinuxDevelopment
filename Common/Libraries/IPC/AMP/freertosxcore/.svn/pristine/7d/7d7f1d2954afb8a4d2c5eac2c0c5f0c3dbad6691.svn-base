################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include FreeRTOS/subdir.mk
-include FreeRTOS/portable/MemMang/subdir.mk
-include FreeRTOS/portable/GCC/MicroBlaze/subdir.mk
-include DemoTasks/MicroBlaze/subdir.mk
-include DemoTasks/Common/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
ELFSIZE += \
freertos_xcore.elf.size \

ELFCHECK += \
freertos_xcore.elf.elfcheck \


# All Target
all: freertos_xcore.elf secondary-outputs

# Tool invocations
freertos_xcore.elf: $(OBJS) ../src/lscript.ld $(USER_OBJS)
	@echo Building target: $@
	@echo Invoking: MicroBlaze gcc linker
	mb-gcc -Wl,-T -Wl,../src/lscript.ld -L../../virtex_dual_core/microblaze_0/lib -mxl-barrel-shift -mxl-pattern-compare -mcpu=v8.10.a -mno-xl-soft-mul -o"freertos_xcore.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo Finished building target: $@
	@echo ' '

freertos_xcore.elf.size: freertos_xcore.elf
	@echo Invoking: MicroBlaze Print Size
	mb-size freertos_xcore.elf  |tee "freertos_xcore.elf.size"
	@echo Finished building: $@
	@echo ' '

freertos_xcore.elf.elfcheck: freertos_xcore.elf
	@echo Invoking: Xilinx ELF Check
	elfcheck freertos_xcore.elf -hw ../../virtex_microblaze_dual_core/system.xml -pe microblaze_0  |tee "freertos_xcore.elf.elfcheck"
	@echo Finished building: $@
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES)$(ELFSIZE)$(ELFCHECK)$(S_UPPER_DEPS) freertos_xcore.elf
	-@echo ' '

secondary-outputs: $(ELFSIZE) $(ELFCHECK)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
