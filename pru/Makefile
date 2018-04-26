# 
# Copyright (c) 2016 Zubeen Tolani <ZeekHuge - zeekhuge@gmail.com>
# Makefile to make PRU_inlineASM_blinky project
#
# The make file is tried to be made as generic as possible
# So that it can be used to compile other programs too
# 
# Usage:
#	name your main source file as main_pru1.c or main_pru0.c
# 	add targets to variable TARGETS
#	add other files required while linking in variable LINK_PRU1(0)_FW
#	add compile targets, as added to LINK_PRU1(0)_FW for other files.
# 

# PRU_CGT environment variable must point to the TI PRU compiler directory. E.g.:
#(Linux) export PRU_CGT=/home/jason/ti/ccs_v6_1_0/ccsv6/tools/compiler/ti-cgt-pru_2.1.0
#(Windows) set PRU_CGT=C:/TI/ccs_v6_0_1/ccsv6/tools/compiler/ti-cgt-pru_2.1.0


LINKER_COMMAND_FILE=./AM335x_PRU.cmd
INCLUDE=--include_path=include --include_path=include/am335x
STACK_SIZE=0x100
HEAP_SIZE=0x100


CFLAGS=-v3 -O3 --display_error_number --endian=little --hardware_mac=on --obj_directory=$(GEN_DIR) --pp_directory=$(GEN_DIR) -ppd -ppa --c++03 -fg
LFLAGS=--reread_libs --warn_sections --stack_size=$(STACK_SIZE) --heap_size=$(HEAP_SIZE)


GEN_DIR=gen

PRU1_FW		=$(GEN_DIR)/am335x-pru1-fw
PRU0_FW		=$(GEN_DIR)/am335x-pru0-fw


# -----------------------------------------------------
# Variable to edit in the makefile

# add the required firmwares to TARGETS
# TARGETS		=$(PRU1_FW) $(PRU0_FW)
TARGETS		=$(PRU1_FW) $(PRU0_FW)

# add the required files while linkin the FW code
# required linking files for PRU1
# LINK_PRU1_FW = (GEN_DIR)/pru1-inline-asm.object (GEN_DIR)/code2_pru1.object
LINK_PRU1_FW=

# required linking files for PRU1
# LINK_PRU0_FW = (GEN_DIR)/pru0-inline-asm.object (GEN_DIR)/code2_pru1.object
LINK_PRU0_FW=

#------------------------------------------------------



.PHONY: all
all: $(TARGETS)


$(PRU1_FW): $(GEN_DIR)/main_pru1.object $(LINK_PRU1_FW)
	@echo 'LD	$^'
	@$(PRU_CGT)/bin/lnkpru -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) -o $@ $^  $(LINKER_COMMAND_FILE) --library=libc.a $(LIBS) $^

$(PRU0_FW): $(GEN_DIR)/main_pru0.object $(LINK_PRU0_FW)
	@echo 'LD	$^' 
	@$(PRU_CGT)/bin/lnkpru -i$(PRU_CGT)/lib -i$(PRU_CGT)/include $(LFLAGS) -o $@ $^  $(LINKER_COMMAND_FILE) --library=libc.a $(LIBS) $^


$(GEN_DIR)/main_pru0.object: main.c 
	@mkdir -p $(GEN_DIR)
	@echo 'CC	$<'
	@$(PRU_CGT)/bin/clpru --include_path=$(PRU_CGT)/include $(INCLUDE) $(CFLAGS) -D PRU_ID=0 -fe $@ $<


$(GEN_DIR)/main_pru1.object: main.c
	@mkdir -p $(GEN_DIR)
	@echo 'CC	$<'
	@$(PRU_CGT)/bin/clpru --include_path=$(PRU_CGT)/include $(INCLUDE) $(CFLAGS) -D PRU_ID=1 -fe $@ $<


.PHONY: clean
clean:
	@rm -rf $(GEN_DIR)
