# File to be included in all makefiles
# Relative paths are viewed from project!

RISCV_XLEN ?= 32
RISCV_LIB  ?= elf
RISCV ?= /opt/riscv
CCPATH	= $(RISCV)

TARGET=riscv${RISCV_XLEN}-unknown-${RISCV_LIB}
#-----------------------------------------------------------
GCC		= $(TARGET)-gcc
OBJCOPY	= $(TARGET)-objcopy
OBJDUMP	= $(TARGET)-objdump
AR		= $(TARGET)-ar
RANLIB	= $(TARGET)-ranlib

PROG	= riscv-main
ARCH_DIR = ../../arch
CRT0	= $(ARCH_DIR)/boot.S

FREERTOS_SOURCE_DIR	= ../../Source
FREERTOS_PLUS_DIR	= ../../Plus
FREERTOS_PLUS_UDP_DIR = ../../Plus/FreeRTOS-Plus-UDP
FREERTOS_PLUS_TCP_DIR = ../../Plus/FreeRTOS-Plus-TCP
FREERTOS_PLUS_FAT_DIR = ../../Plus/FreeRTOS-Plus-FAT
CUNIT	= ../../../cunit

#-----------------------------------------------------------
WARNINGS= -Wall -Wextra -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wsign-compare \
		-Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wunused

