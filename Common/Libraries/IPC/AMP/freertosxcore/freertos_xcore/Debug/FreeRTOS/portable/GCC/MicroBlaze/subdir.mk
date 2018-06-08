################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/portable/GCC/MicroBlaze/port.c \
../FreeRTOS/portable/GCC/MicroBlaze/portdbg.c \
../FreeRTOS/portable/GCC/MicroBlaze/porthw.c 

S_UPPER_SRCS += \
../FreeRTOS/portable/GCC/MicroBlaze/portasm.S 

OBJS += \
./FreeRTOS/portable/GCC/MicroBlaze/port.o \
./FreeRTOS/portable/GCC/MicroBlaze/portasm.o \
./FreeRTOS/portable/GCC/MicroBlaze/portdbg.o \
./FreeRTOS/portable/GCC/MicroBlaze/porthw.o 

C_DEPS += \
./FreeRTOS/portable/GCC/MicroBlaze/port.d \
./FreeRTOS/portable/GCC/MicroBlaze/portdbg.d \
./FreeRTOS/portable/GCC/MicroBlaze/porthw.d 

S_UPPER_DEPS += \
./FreeRTOS/portable/GCC/MicroBlaze/portasm.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/portable/GCC/MicroBlaze/%.o: ../FreeRTOS/portable/GCC/MicroBlaze/%.c
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -O0 -g3 -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\DemoTasks\Common\include" -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\DemoTasks\MicroBlaze" -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\FreeRTOS\include" -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\FreeRTOS\portable\GCC\MicroBlaze" -c -fmessage-length=0 -I../../virtex_dual_core/microblaze_0/include -mxl-barrel-shift -mxl-pattern-compare -mcpu=v8.10.a -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '

FreeRTOS/portable/GCC/MicroBlaze/%.o: ../FreeRTOS/portable/GCC/MicroBlaze/%.S
	@echo Building file: $<
	@echo Invoking: MicroBlaze gcc compiler
	mb-gcc -Wall -O0 -g3 -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\DemoTasks\Common\include" -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\DemoTasks\MicroBlaze" -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\FreeRTOS\include" -I"C:\Users\James\Documents\swe\final_project\dual_core_workspace\freertos_xcore\FreeRTOS\portable\GCC\MicroBlaze" -c -fmessage-length=0 -I../../virtex_dual_core/microblaze_0/include -mxl-barrel-shift -mxl-pattern-compare -mcpu=v8.10.a -mno-xl-soft-mul -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo Finished building: $<
	@echo ' '


