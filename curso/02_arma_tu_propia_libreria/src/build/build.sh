#!/usr/bin/env bash
# Build de mygpio para LPC1769 SIN make (solo necesita el toolchain local del repo).
set -e
cd "$(dirname "$0")"
TC=../../../../tools/toolchain/bin
CFLAGS="-mcpu=cortex-m3 -mthumb -Wall -Wextra -O2 -ffunction-sections -fdata-sections -I.."
LDFLAGS="-nostartfiles -T lpc1769.ld -Wl,--gc-sections"
"$TC/arm-none-eabi-gcc" $CFLAGS $LDFLAGS ../mygpio.c ../main.c startup.c -o mygpio.elf
"$TC/arm-none-eabi-objcopy" -O binary mygpio.elf mygpio.bin
"$TC/arm-none-eabi-size" mygpio.elf
echo "OK -> mygpio.elf, mygpio.bin"
