# Proyecto independiente: generador de funciones controlado por UART.

REPO_ROOT := ../../../..
PROJECT   := generador_funciones_uart
BUILD_DIR := build
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
PORT      ?= auto

CPUFLAGS := -mcpu=cortex-m3 -mthumb
CFLAGS := $(CPUFLAGS) $(OPT) -std=gnu11 -g3 -gdwarf-4 \
          -Wall -Wextra -Wshadow -Wundef -Wdouble-promotion \
          -ffunction-sections -fdata-sections -fno-common -MMD -MP \
          -D__USE_CMSIS -DCORE_M3 -I$(CMSIS_DIR)/inc -Isrc

LDSCRIPT := $(TEMPLATE)/linker/lpc1769.ld
LDFLAGS := $(CPUFLAGS) -T$(LDSCRIPT) --specs=nano.specs \
           -Wl,--gc-sections \
           -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref \
           -Wl,--print-memory-usage \
           -Wl,--no-warn-rwx-segments

APP_SRC := $(wildcard src/*.c)
APP_OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(APP_SRC))
OBJ := $(APP_OBJ) \
       $(BUILD_DIR)/startup_lpc1769.o \
       $(BUILD_DIR)/system_LPC17xx.o \
       $(BUILD_DIR)/syscalls.o
DEP := $(OBJ:.o=.d)

ELF := $(BUILD_DIR)/$(PROJECT).elf
BIN := $(BUILD_DIR)/$(PROJECT).bin
HEX := $(BUILD_DIR)/$(PROJECT).hex

.PHONY: all
all: $(ELF) $(BIN) $(HEX) size

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/startup_lpc1769.o: $(TEMPLATE)/startup/startup_lpc1769.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/system_LPC17xx.o: $(CMSIS_DIR)/src/system_LPC17xx.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Wno-unused-parameter -c $< -o $@

$(BUILD_DIR)/syscalls.o: $(TEMPLATE)/src/syscalls.c
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

.PHONY: app
app:
	$(PYTHON) pc/generator_app.py --port $(PORT)

.PHONY: check
check:
	$(PYTHON) pc/generator_check.py --port $(PORT)

.PHONY: clean
clean:
	rm -rf build

.PHONY: help
help:
	@echo "make          compila el proyecto"
	@echo "make flash    compila, verifica y graba la placa"
	@echo "make app      abre el control y monitor por UART"
	@echo "make check    prueba las cinco formas con el loopback"
	@echo "make clean    borra solamente build/ de este proyecto"

-include $(DEP)
