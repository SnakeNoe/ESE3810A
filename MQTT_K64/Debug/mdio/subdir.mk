################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../mdio/fsl_enet_mdio.c 

C_DEPS += \
./mdio/fsl_enet_mdio.d 

OBJS += \
./mdio/fsl_enet_mdio.o 


# Each subdirectory must supply rules for building sources it contributes
mdio/%.o: ../mdio/%.c mdio/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -D_POSIX_SOURCE -DUSE_RTOS=1 -DPRINTF_ADVANCED_ENABLE=1 -DFRDM_K64F -DFREEDOM -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\source" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\mdio" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\phy" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\lwip\src\include\lwip\apps" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\lwip\port" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\lwip\src" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\lwip\src\include" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\drivers" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\utilities" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\device" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\component\uart" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\component\serial_manager" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\component\lists" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\CMSIS" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\freertos\freertos_kernel\include" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\ESE3810A_Noe_Ortiz\MQTT_K64\board" -O0 -fno-common -g3 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-mdio

clean-mdio:
	-$(RM) ./mdio/fsl_enet_mdio.d ./mdio/fsl_enet_mdio.o

.PHONY: clean-mdio

