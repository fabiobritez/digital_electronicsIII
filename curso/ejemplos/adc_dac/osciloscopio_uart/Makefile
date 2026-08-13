# Proyecto independiente: generador DAC + captura ADC + visualizador UART.

REPO_ROOT := ../../../..
PROJECT   := osciloscopio_uart
SCOPE_MODE ?= 1
BUILD_DIR := build/mode$(SCOPE_MODE)
OPT       ?= -O2

CMSIS_DIR := $(REPO_ROOT)/library/CMSISv2p00_LPC17xx
TEMPLATE  := $(REPO_ROOT)/plantilla

TOOLCHAIN_LOCAL := $(wildcard $(REPO_ROOT)/tools/toolchain/bin/arm-none-eabi-gcc)
ifneq ($(TOOLCHAIN_LOCAL),)
  CROSS ?= $(REPO_ROOT)/tools/toolchain/bin/arm-none-eabi-
else
  CROSS ?= arm-none-eabi-
endif

CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size
PYTHON  := $(firstword $(shell command -v python3 2>/dev/null) $(shell command -v python 2>/dev/null))

CPUFLAGS := -mcpu=cortex-m3 -mthumb
CFLAGS := $(CPUFLAGS) $(OPT) -std=gnu11 -g3 -gdwarf-4 \
          -Wall -Wextra -Wshadow -Wundef -Wdouble-promotion \
          -ffunction-sections -fdata-sections -fno-common -MMD -MP \
          -D__USE_CMSIS -DCORE_M3 -DSCOPE_MODE=$(SCOPE_MODE) \
          -I$(CMSIS_DIR)/inc

LDSCRIPT := $(TEMPLATE)/linker/lpc1769.ld
LDFLAGS := $(CPUFLAGS) -T$(LDSCRIPT) --specs=nano.specs \
           -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref \
           -Wl,--print-memory-usage \
           -Wl,--no-warn-rwx-segments

APP_OBJ      := $(BUILD_DIR)/main.o
STARTUP_OBJ  := $(BUILD_DIR)/startup_lpc1769.o
SYSTEM_OBJ   := $(BUILD_DIR)/system_LPC17xx.o
SYSCALLS_OBJ := $(BUILD_DIR)/syscalls.o
OBJ := $(APP_OBJ) $(STARTUP_OBJ) $(SYSTEM_OBJ) $(SYSCALLS_OBJ)
DEP := $(OBJ:.o=.d)

ELF := $(BUILD_DIR)/$(PROJECT).elf
BIN := $(BUILD_DIR)/$(PROJECT).bin
HEX := $(BUILD_DIR)/$(PROJECT).hex

.PHONY: all
all: $(ELF) $(BIN) $(HEX) size

$(APP_OBJ): src/main.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(STARTUP_OBJ): $(TEMPLATE)/startup/startup_lpc1769.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(SYSTEM_OBJ): $(CMSIS_DIR)/src/system_LPC17xx.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wno-unused-parameter -c $< -o $@

$(SYSCALLS_OBJ): $(TEMPLATE)/src/syscalls.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJ) $(LDSCRIPT)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	$(PYTHON) $(TEMPLATE)/tools/lpc_checksum.py -q $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

.PHONY: size
size: $(ELF)
	@echo ""
	$(SIZE) $(ELF)
	@echo ""

.PHONY: preflight
preflight: $(ELF)
	$(PYTHON) $(TEMPLATE)/tools/preflight.py $(ELF)

.PHONY: flash
flash: all preflight
	openocd -f $(TEMPLATE)/openocd/lpc1769.cfg \
	  -c "program $(abspath $(ELF)) verify reset exit"

.PHONY: clean
clean:
	rm -rf build

.PHONY: help
help:
	@echo "make                         compila el modo por bloques"
	@echo "make flash                   compila y graba el modo por bloques"
	@echo "make SCOPE_MODE=2            compila el streaming continuo de 8 bits"
	@echo "make SCOPE_MODE=2 flash      graba el streaming continuo de 8 bits"
	@echo "make preflight               verifica el firmware sin grabarlo"
	@echo "make clean                   borra solamente build/ de este proyecto"

-include $(DEP)

