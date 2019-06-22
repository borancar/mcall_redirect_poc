# Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved
# Modifications copyright (C) 2019 Boran Car <boran.car@gmail.com>

TARGET = os.elf

BSP_BASE := ../bsp
PLATFORM_DIR := $(BSP_BASE)/$(BOARD)
NEWLIB_DIR := $(PLATFORM_DIR)/newlib

LINKER_SCRIPT := flash.lds

C_SRCS += os.c
ASM_SRCS += trap.S

include $(NEWLIB_DIR)/newlib.mk
