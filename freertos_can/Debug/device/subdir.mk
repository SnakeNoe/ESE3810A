################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../device/system_MK64F12.c 

C_DEPS += \
./device/system_MK64F12.d 

OBJS += \
./device/system_MK64F12.o 


# Each subdirectory must supply rules for building sources it contributes
device/%.o: ../device/%.c device/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DFRDM_K64F -DFREEDOM -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\ESE3810A_Noe_Ortiz\freertos_can\source" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\drivers" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\utilities" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\device" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\component\uart" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\component\serial_manager" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\component\lists" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\CMSIS" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\freertos\freertos_kernel\include" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\ESE3810A_Noe_Ortiz\freertos_can\board" -I"C:\Users\noe_9\Documents\MCUXpressoIDE_11.8.0_1165\workspace\freertos_can\drivers" -O0 -fno-common -g3 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-device

clean-device:
	-$(RM) ./device/system_MK64F12.d ./device/system_MK64F12.o

.PHONY: clean-device

