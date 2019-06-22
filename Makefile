# Copyright(C) 2018 Hex Five Security, Inc. - All Rights Reserved
# Modifications copyright(C) Boran Car


#############################################################
# Toolchain definitions
#############################################################

ifndef RISCV
$(error RISCV not set)
endif

export CROSS_COMPILE := $(abspath $(RISCV))/bin/riscv64-unknown-elf-
export CC      := $(CROSS_COMPILE)gcc
export OBJDUMP := $(CROSS_COMPILE)objdump
export OBJCOPY := $(CROSS_COMPILE)objcopy
export GDB     := $(CROSS_COMPILE)gdb
export AR      := $(CROSS_COMPILE)ar


#############################################################
# Platform definitions
#############################################################

BOARD := U540
ARCH := rv64
RISCV_ARCH := $(ARCH)imac
RISCV_ABI := lp64


#############################################################
# Arguments/variables available to all submakes
#############################################################

export BOARD
export RISCV_ARCH
export RISCV_ABI


#############################################################
# Rules for building
#############################################################

.PHONY: all 
all: clean
	$(MAKE) -C tee
	$(MAKE) -C os
	srec_cat tee/tee.hex -Intel os/os.hex -Intel -Output poc.hex

.PHONY: clean
clean: 
	$(MAKE) -C tee clean
	$(MAKE) -C os clean


#############################################################
# Load and debug variables and rules
#############################################################

ifndef OPENOCD
$(error OPENOCD not set)
endif

OPENOCD := $(abspath $(OPENOCD))/bin/openocd

OPENOCDCFG ?= bsp/$(BOARD)/openocd.cfg
OPENOCDARGS += -f $(OPENOCDCFG)

GDB_PORT ?= 3333
GDB_LOAD_ARGS ?= --batch
GDB_LOAD_CMDS += -ex "set mem inaccessible-by-default off"
GDB_LOAD_CMDS += -ex "set remotetimeout 240"
GDB_LOAD_CMDS += -ex "set arch riscv:$(ARCH)"
GDB_LOAD_CMDS += -ex "target extended-remote localhost:$(GDB_PORT)"
GDB_LOAD_CMDS += -ex "monitor reset halt"
GDB_LOAD_CMDS += -ex "monitor flash protect 0 64 last off"
GDB_LOAD_CMDS += -ex "load"
GDB_LOAD_CMDS += -ex "monitor resume"
GDB_LOAD_CMDS += -ex "monitor shutdown"
GDB_LOAD_CMDS += -ex "quit"

.PHONY: load

load:
	$(OPENOCD) $(OPENOCDARGS) & \
	$(GDB) poc.hex $(GDB_LOAD_ARGS) $(GDB_LOAD_CMDS)
