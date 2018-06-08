################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
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
core_ignition.elf.size \

ELFCHECK += \
core_ignition.elf.elfcheck \


# All Target
all: core_ignition.elf secondary-outputs

# Tool invocations
core_ignition.elf: $(OBJS) ../src/lscript.ld $(USER_OBJS)
	@echo Building target: $@
	@echo Invoking: MicroBlaze gcc linker
	mb-gcc -Wl,-T -Wl,../src/lscript.ld -L../../virtex_dual_core_2/microblaze_1/lib -mxl-barrel-shift -mxl-pattern-compare -mcpu=v8.10.a -mno-xl-soft-mul -o"core_ignition.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo Finished building target: $@
	@echo ' '

core_ignition.elf.size: core_ignition.elf
	@echo Invoking: MicroBlaze Print Size
	mb-size core_ignition.elf  |tee "core_ignition.elf.size"
	@echo Finished building: $@
	@echo ' '

core_ignition.elf.elfcheck: core_ignition.elf
	@echo Invoking: Xilinx ELF Check
	elfcheck core_ignition.elf -hw ../../virtex_microblaze_dual_core/system.xml -pe microblaze_1  |tee "core_ignition.elf.elfcheck"
	@echo Finished building: $@
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES)$(ELFSIZE)$(ELFCHECK)$(S_UPPER_DEPS) core_ignition.elf
	-@echo ' '

secondary-outputs: $(ELFSIZE) $(ELFCHECK)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
