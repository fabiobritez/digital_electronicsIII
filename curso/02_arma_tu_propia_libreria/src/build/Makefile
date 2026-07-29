# Build de la libreria mygpio para LPC1769 (Cortex-M3).
# Usa el toolchain local del repo (no requiere instalacion en el sistema).
TOOLCHAIN := ../../../../tools/toolchain/bin
CC      := $(TOOLCHAIN)/arm-none-eabi-gcc
OBJCOPY := $(TOOLCHAIN)/arm-none-eabi-objcopy
SIZE    := $(TOOLCHAIN)/arm-none-eabi-size

CFLAGS  := -mcpu=cortex-m3 -mthumb -Wall -Wextra -O2 -ffunction-sections -fdata-sections -I..
LDFLAGS := -nostartfiles -T lpc1769.ld -Wl,--gc-sections

SRCS := ../mygpio.c ../main.c startup.c

all: mygpio.bin
	$(SIZE) mygpio.elf

mygpio.elf: $(SRCS) lpc1769.ld startup.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRCS) -o $@

mygpio.bin: mygpio.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f mygpio.elf mygpio.bin
