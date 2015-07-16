EXECUTABLE=PlayuavOSD.elf
TARGETBIN=PlayuavOSD.bin

CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy

BIN  = $(CP) -O binary -S
DEFS = -DUSE_STDPERIPH_DRIVER -DSTM32F4XX -DHSE_VALUE=8000000

MCU = cortex-m4
MCFLAGS = -mcpu=$(MCU) -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -std=gnu99
STMLIBSDIR    = ./lib/STM32F4-Discovery_FW_V1.1.0/Libraries
STMSPDDIR    = $(STMLIBSDIR)/STM32F4xx_StdPeriph_Driver
STMSPSRCDDIR = $(STMSPDDIR)/src
STMSPINCDDIR = $(STMSPDDIR)/inc
#FREERTOSDIR = ./lib/FreeRTOS8.1.2/Source
FREERTOSDIR = ./lib/FreeRTOSV8.2.0/FreeRTOS/Source
USBOTGLIB = $(STMLIBSDIR)/STM32_USB_OTG_Driver
USBDEVICELIB = $(STMLIBSDIR)/STM32_USB_Device_Library
USBHOSTLIB = $(STMLIBSDIR)/STM32_USB_HOST_Library
MAVLINKDIR = ./lib/mavlink/v1.0

STM32_INCLUDES = -I$(STMLIBSDIR)/CMSIS/Include/ \
				 -I$(STMLIBSDIR)/CMSIS/ST/STM32F4xx/Include/ \
				 -I$(STMSPINCDDIR)/ \
				 -I$(FREERTOSDIR)/include    \
          		 -I$(FREERTOSDIR)/portable/GCC/ARM_CM4F    \
          		 -I$(MAVLINKDIR)    \
          		 -I$(USBDEVICELIB)/Class/cdc/inc    \
          		 -I$(USBDEVICELIB)/Core/inc    \
          		 -I$(USBHOSTLIB)/Core/inc    \
          		 -I$(USBOTGLIB)/inc    \
				 -I./inc 
OPTIMIZE       = -Os

CFLAGS	= $(MCFLAGS)  $(OPTIMIZE)  $(DEFS) -I./ -I./ $(STM32_INCLUDES)  -Wl,-T,./linker/stm32_flash.ld
AFLAGS	= $(MCFLAGS) 
#-mapcs-float use float regs. small increase in code size

STM32_USB_OTG_SRC = $(USBOTGLIB)/src/usb_dcd_int.c \
					$(USBOTGLIB)/src/usb_core.c \
					$(USBOTGLIB)/src/usb_dcd.c \

STM32_USB_DEVICE_SRC =	$(USBDEVICELIB)/Class/cdc/src/usbd_cdc_core.c \
						$(USBDEVICELIB)/Core/src/usbd_req.c \
						$(USBDEVICELIB)/Core/src/usbd_core.c \
						$(USBDEVICELIB)/Core/src/usbd_ioreq.c

## list of APP files
SRC  = ./src/main.c
SRC += ./src/system_stm32f4xx.c
SRC += ./src/stm32f4xx_it.c
SRC += ./src/Board.c
SRC += ./src/Led.c
SRC += ./src/spi.c
SRC += ./src/usart2.c
SRC += ./src/osdvar.c
SRC += ./src/font_outlined8x8.c
SRC += ./src/font_outlined8x14.c
SRC += ./src/fonts.c
SRC += ./src/graphengine.c
SRC += ./src/m2dlib.c
SRC += ./src/m3dlib.c
SRC += ./src/math3d.c
SRC += ./src/max7456.c
SRC += ./src/osdconfig.c
SRC += ./src/osdcore.c
SRC += ./src/osdmavlink.c
SRC += ./src/osdproc.c
SRC += ./src/UAVObj.c
SRC += ./src/uavTalk.c
SRC += ./src/usb_bsp.c
SRC += ./src/usbd_cdc_vcp.c
SRC += ./src/usbd_desc.c
SRC += ./src/usbd_usr.c
SRC += ./src/printf2.c
## used parts of the STM-Library
SRC += $(STMSPSRCDDIR)/stm32f4xx_dma.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_exti.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_flash.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_gpio.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_syscfg.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_pwr.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_rcc.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_rtc.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_spi.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_tim.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_usart.c
SRC += $(STMSPSRCDDIR)/stm32f4xx_can.c
SRC += $(STMSPSRCDDIR)/misc.c
## list of FreeRTOS files
SRC += $(FREERTOSDIR)/portable/GCC/ARM_CM4F/port.c
SRC += $(FREERTOSDIR)/portable/MemMang/heap_2.c
SRC += $(FREERTOSDIR)/croutine.c
SRC += $(FREERTOSDIR)/event_groups.c
SRC += $(FREERTOSDIR)/list.c
SRC += $(FREERTOSDIR)/queue.c
SRC += $(FREERTOSDIR)/tasks.c
SRC += $(FREERTOSDIR)/timers.c
## list of USB_CDC files
SRC += $(STM32_USB_OTG_SRC)
SRC += $(STM32_USB_DEVICE_SRC)
# List ASM source files here
STARTUP = ./$(STMLIBSDIR)/CMSIS/ST/STM32F4xx/Source/Templates/gcc_ride7/startup_stm32f40xx.s


all: $(TARGETBIN) objcopy

$(TARGETBIN): $(EXECUTABLE)
	$(BIN) $^ $@
	
$(EXECUTABLE): $(SRC) $(STARTUP)
	$(CC) $(CFLAGS) $^ -lm -lc -lnosys -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -o $@

objcopy:
	@python -u px_mkfw.py --image $(TARGETBIN) > playuavosd.hex
	
clean:
	rm -f $(TARGETBIN) $(EXECUTABLE) $(SRC:.c=.lst)
