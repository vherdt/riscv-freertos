#/*
#    FreeRTOS V8.2.3 - Copyright (C) 2015 Real Time Engineers Ltd.
#
#
#    ***************************************************************************
#     *                                                                       *
#     *    FreeRTOS tutorial books are available in pdf and paperback.        *
#     *    Complete, revised, and edited pdf reference manuals are also       *
#     *    available.                                                         *
#     *                                                                       *
#     *    Purchasing FreeRTOS documentation will not only help you, by       *
#     *    ensuring you get running as quickly as possible and with an        *
#     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
#     *    the FreeRTOS project to continue with its mission of providing     *
#     *    professional grade, cross platform, de facto standard solutions    *
#     *    for microcontrollers - completely free of charge!                  *
#     *                                                                       *
#     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
#     *                                                                       *
#     *    Thank you for using FreeRTOS, and thank you for your support!      *
#     *                                                                       *
#    ***************************************************************************
#
#
#    This file is part of the FreeRTOS distribution and was contributed
#    to the project by Technolution B.V. (www.technolution.nl,
#    freertos-riscv@technolution.eu) under the terms of the FreeRTOS
#    contributors license.
#
#    FreeRTOS is free software; you can redistribute it and/or modify it under
#    the terms of the GNU General Public License (version 2) as published by the
#    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
#    >>>NOTE<<< The modification to the GPL is included to allow you to
#    distribute a combined work that includes FreeRTOS without being obliged to
#    provide the source code for proprietary components outside of the FreeRTOS
#    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
#    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#    more details. You should have received a copy of the GNU General Public
#    License and the FreeRTOS license exception along with FreeRTOS; if not it
#    can be viewed here: http://www.freertos.org/a00114.html and also obtained
#    by writing to Richard Barry, contact details for whom are available on the
#    FreeRTOS WEB site.
#
#    1 tab == 4 spaces!
#
#    http://www.FreeRTOS.org - Documentation, latest information, license and
#    contact details.
#
#    http://www.SafeRTOS.com - A version that is certified for use in safety
#    critical systems.
#
#    http://www.OpenRTOS.com - Commercial support, development, porting,
#    licensing and training services.
#*/

include ../Makefile.inc

# Root of RISC-V tools installation. Note that we expect to find the spike
# simulator header files here under $(RISCV)/include/spike .
RISCV ?= /opt/riscv

FREERTOS_SRC = \
	$(FREERTOS_SOURCE_DIR)/croutine.c \
	$(FREERTOS_SOURCE_DIR)/list.c \
	$(FREERTOS_SOURCE_DIR)/queue.c \
	$(FREERTOS_SOURCE_DIR)/tasks.c \
	$(FREERTOS_SOURCE_DIR)/timers.c \
	$(FREERTOS_SOURCE_DIR)/event_groups.c \
	$(FREERTOS_SOURCE_DIR)/portable/MemMang/heap_4.c


APP_SOURCE_DIR	= ../Common/Minimal

APP_SRC =  \
	$(APP_SOURCE_DIR)/blocktim.c \
	$(APP_SOURCE_DIR)/countsem.c \
	$(APP_SOURCE_DIR)/recmutex.c


PORT_SRC = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISCV/port.c
PORT_ASM = $(FREERTOS_SOURCE_DIR)/portable/GCC/RISCV/portasm.S

PLUS_UDP_SRC =  \
    $(FREERTOS_PLUS_UDP_DIR)/FreeRTOS_DHCP.c \
    $(FREERTOS_PLUS_UDP_DIR)/FreeRTOS_DNS.c \
    $(FREERTOS_PLUS_UDP_DIR)/FreeRTOS_Sockets.c \
    $(FREERTOS_PLUS_UDP_DIR)/FreeRTOS_UDP_IP.c \
    $(FREERTOS_PLUS_UDP_DIR)/portable/BufferManagement/BufferAllocation_2.c

PORT_UDP_SRC = $(FREERTOS_PLUS_UDP_DIR)/portable/NetworkInterface/riscv-vp/NetworkInterface.c

PLUS_TCP_SRC =  \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_ARP.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_DHCP.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_DNS.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_IP.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_Sockets.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_Stream_Buffer.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_TCP_IP.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_TCP_WIN.c \
	$(FREERTOS_PLUS_TCP_DIR)/FreeRTOS_UDP_IP.c \
    $(FREERTOS_PLUS_TCP_DIR)/portable/BufferManagement/BufferAllocation_2.c

PORT_TCP_SRC = $(FREERTOS_PLUS_TCP_DIR)/portable/NetworkInterface/riscv-vp/NetworkInterface.c

DEMO_SRC = \
	$(ARCH_DIR)/syscalls.c \
	$(ARCH_DIR)/clib.c \
	$(ARCH_DIR)/irq.c \
	main.c

INCLUDES = \
	-I. \
	-I$(ARCH_DIR) \
	-I$(ARCH_DIR)/../\
	-I./conf \
	-I./include \
	-I$(FREERTOS_SOURCE_DIR)/include \
	-I../Common/include \
	-I$(FREERTOS_SOURCE_DIR)/portable/GCC/RISCV \
	-I$(FREERTOS_PLUS_TCP_DIR)/include \
	-I$(FREERTOS_PLUS_TCP_DIR)/portable/Compiler/GCC
CFLAGS = \
	$(WARNINGS) $(INCLUDES) \
	-fomit-frame-pointer -fno-strict-aliasing -fno-builtin \
	-D__gracefulExit -mcmodel=medany -g3 #-fPIC

GCCVER 	= $(shell $(GCC) --version | grep gcc | cut -d" " -f9)

#
# Define all object files.
#
RTOS_OBJ = $(FREERTOS_SRC:.c=.o)
APP_OBJ  = $(APP_SRC:.c=.o)
PORT_OBJ = $(PORT_SRC:.c=.o)
DEMO_OBJ = $(DEMO_SRC:.c=.o)
PORT_ASM_OBJ = $(PORT_ASM:.S=.o)
CRT0_OBJ = $(CRT0:.S=.o)
PLUS_TCP_OBJ = $(PLUS_TCP_SRC:.c=.o)
PORT_TCP_OBJ = $(PORT_TCP_SRC:.c=.o)
OBJS = $(CRT0_OBJ) $(PORT_ASM_OBJ) $(PORT_OBJ) $(RTOS_OBJ) $(DEMO_OBJ) $(APP_OBJ) $(PLUS_TCP_OBJ) $(PORT_TCP_OBJ)

LDFLAGS	 = -T $(ARCH_DIR)/link.ld -nostartfiles -static -nostdlib
LIBS	 = -L$(CCPATH)/lib/gcc/$(TARGET)/$(GCCVER) \
		   -L$(CCPATH)/$(TARGET)/lib \
		   -lc -lgcc -Wl,--verbose

%.o: %.c
	@echo "    CC $<"
	@$(GCC) -c $(CFLAGS) -o $@ $<

%.o: %.S
	@echo "    CC $<"
	@$(GCC) -c $(CFLAGS) -o $@ $<

all: $(PROG).elf

$(PROG).elf  : $(OBJS) Makefile
	@echo Linking....
	@$(GCC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)
	@$(OBJDUMP) -S $(PROG).elf > $(PROG).asm
	@echo Completed $@RISCV ?= /opt/riscv

clean :
	@rm -f $(OBJS)
	@rm -f $(PROG).elf
	@rm -f $(PROG).map
	@rm -f $(PROG).asm

force_true:
	@true

#-------------------------------------------------------------

ETH_DEV=enp0s31f6

ifneq ("$(wildcard /dev/tap*)","")
	NET_DEV = $(shell ls /dev/tap*)			#MacVTap
else
    ifneq ("$(wildcard /dev/net/tun)","")
    	NET_DEV = /dev/net/tun				#bridge
    endif
endif

testFunc:
	echo $(NET_DEV)

sim: all
	riscv-vp $(PROG).elf --memory-start=2147483648 --network-device=$(NET_DEV) --intercept-syscalls

debug: all
	riscv-vp $(PROG).elf --debug-mode --network-device=$(NET_DEV) --memory-start=2147483648

bridge:
	sudo killall dhclient || true
	sudo ip tuntap add mode tap tap0
	sudo ip link set tap0 address F0:00:00:00:00:00 up	#when bridging, virtual client needs "higher" MAC so that bridge keeps hosts mac
	sudo brctl addbr br0
	sudo brctl setfd br0 0
	sudo brctl addif br0 $(ETH_DEV)
	sudo dhclient br0	#important to call before adding tap0 to supress getting its IP
	sudo brctl addif br0 tap0

bridgeclean:
	sudo killall dhclient || true
	sudo ip link del tap0 || true
	sudo ip link del br0 || true
	sudo dhclient -v $(ETH_DEV)

tap:
	sudo ip link add link $(ETH_DEV) name tap0 type macvtap
	sudo ip link set tap0 address F0:00:00:00:00:00 up
	sudo chmod a+rw /dev/tap*
	ls -la /dev/tap*
	ip link show

tapclean:
	sudo ip link del macvtap0 || true

netclean: tapclean bridgeclean

