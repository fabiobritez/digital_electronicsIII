MEMORY {
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
SECTIONS {
  .text : { KEEP(*(.isr_vector)) *(.text*) *(.rodata*) _etext = .; } > FLASH
  .data : AT (_etext) { _sdata = .; *(.data*) _edata = .; } > RAM
  .bss  : { _sbss = .; *(.bss*) *(COMMON) _ebss = .; } > RAM
}
