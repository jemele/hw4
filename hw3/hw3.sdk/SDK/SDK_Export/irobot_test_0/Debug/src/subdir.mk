################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/gpio.c \
../src/helloworld.c \
../src/irobot.c \
../src/menu.c \
../src/platform.c \
../src/ssd1306.c \
../src/uart.c 

CC_SRCS += \
../src/search.cc 

LD_SRCS += \
../src/lscript.ld 

OBJS += \
./src/gpio.o \
./src/helloworld.o \
./src/irobot.o \
./src/menu.o \
./src/platform.o \
./src/ssd1306.o \
./src/uart.o  \
./src/search.o 

C_DEPS += \
./src/gpio.d \
./src/helloworld.d \
./src/irobot.d \
./src/platform.d \
./src/search.d \
./src/ssd1306.d \
./src/uart.d 

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM gcc compiler'
	arm-xilinx-eabi-gcc -Wall -O0 -g3 -c -fmessage-length=0 -I../../standalone_bsp_0/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: ARM g++ compiler'
	arm-xilinx-eabi-g++ -Wall -O0 -g3 -c -fmessage-length=0 -I../../standalone_bsp_0/ps7_cortexa9_0/include -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



