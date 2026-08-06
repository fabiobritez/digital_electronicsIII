/* ============================================================================
 * lpc1769.ld - Linker script para el LPC1769 (ARM Cortex-M3)
 * ============================================================================
 *
 * Este archivo le contesta al linker dos preguntas:
 *
 *   1. Que memorias tiene el chip y donde estan  -> bloque MEMORY
 *   2. En cual de ellas va cada parte del programa -> bloque SECTIONS
 *
 * Es el archivo que hay que tocar para portar el mismo proyecto a otro
 * microcontrolador: cambian las direcciones y los tamanos, la estructura no.
 *
 * Los numeros de aca salen del capitulo 2 (Memory map) del UM10360.
 * ========================================================================= */

/* Por donde arranca el programa. El linker lo usa para saber que codigo NO
   puede descartar con --gc-sections. No confundir con el reset: al chip lo
   dirige la tabla de vectores, no este simbolo. */
ENTRY(Reset_Handler)


/* ---------------------------------------------------------------------------
 * 1) Las memorias del LPC1769
 * ---------------------------------------------------------------------------
 *
 *   FLASH    512 KB  0x00000000  no volatil: el codigo y las constantes
 *   RAM       32 KB  0x10000000  SRAM principal, pegada al core (rapida)
 *   AHBRAM0   16 KB  0x2007C000  SRAM en el bus AHB: pensada para DMA/USB
 *   AHBRAM1   16 KB  0x20080000  idem (Ethernet)
 *
 * En total 64 KB de RAM, pero NO son contiguos: hay un agujero entre
 * 0x10008000 y 0x2007C000. Por eso son cuatro regiones separadas y no una.
 * Las variables normales van a RAM; las otras dos quedan disponibles para
 * buffers de DMA (ver el modulo 11 del curso).
 *
 * Los atributos entre parentesis son informativos para el linker:
 *   r = lectura, w = escritura, x = ejecutable
 * ------------------------------------------------------------------------ */
MEMORY
{
  FLASH   (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  RAM     (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
  AHBRAM0 (rwx) : ORIGIN = 0x2007C000, LENGTH = 16K
  AHBRAM1 (rwx) : ORIGIN = 0x20080000, LENGTH = 16K
}


/* ---------------------------------------------------------------------------
 * 2) Stack y heap
 * ---------------------------------------------------------------------------
 * El stack arranca en el tope de la RAM y crece hacia ABAJO. El heap (malloc)
 * arranca despues de .bss y crece hacia ARRIBA. Se encuentran en el medio: si
 * chocan, se corrompen mutuamente y el sintoma es un hard fault random.
 *
 * Los dos tamanos de abajo no reservan memoria de verdad: son el minimo que
 * exigimos que quede libre. Si no entra, el ASSERT del final del archivo hace
 * fallar el LINKEO en vez de dejarte descubrirlo en la placa a las 3 AM.
 * ------------------------------------------------------------------------ */
_Min_Stack_Size = 2K;   /* espacio que le reservamos al stack */
_Min_Heap_Size  = 0;    /* 0 = no usamos malloc (lo normal en embebidos) */

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* 0x10008000: tope de la SRAM */


SECTIONS
{
  /* -------------------------------------------------------------------------
   * .isr_vector - LA TABLA DE VECTORES. Tiene que ir SI O SI primero.
   * -------------------------------------------------------------------------
   * Al salir del reset el Cortex-M3 lee dos palabras de la direccion 0:
   *   0x00000000 -> valor inicial del stack pointer
   *   0x00000004 -> direccion a la que saltar (Reset_Handler)
   * Si esto no esta en 0x0, el chip no arranca. Punto.
   *
   * KEEP() evita que --gc-sections la borre: nadie "llama" a la tabla desde
   * el codigo, asi que sin KEEP el linker la considera basura y la elimina.
   * ---------------------------------------------------------------------- */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } > FLASH

  /* -------------------------------------------------------------------------
   * .text - el codigo
   * ---------------------------------------------------------------------- */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* el codigo de todas tus funciones */
    *(.text*)          /* idem, con -ffunction-sections cada funcion es .text.nombre */
    *(.glue_7)         /* trampolines ARM<->Thumb (el M3 es Thumb puro, quedan vacios) */
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
  } > FLASH

  /* -------------------------------------------------------------------------
   * .rodata - constantes de solo lectura
   * -------------------------------------------------------------------------
   * Los "strings" literales, las tablas const, etc. Se quedan en la FLASH:
   * no tiene sentido gastar RAM en algo que no cambia.
   * ---------------------------------------------------------------------- */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* Tablas de unwinding de excepciones de C++. En C quedan vacias, pero si no
     las declaras el linker tira warnings. */
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  /* Constructores: punteros a funciones que hay que correr ANTES de main.
     En C puro solo aparecen con __attribute__((constructor)); en C++ es donde
     viven los constructores de los objetos globales. El startup las recorre
     llamando a __libc_init_array(). */
  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array*))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > FLASH

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array*))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array*))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > FLASH

  /* Fin de todo lo que ocupa FLASH de forma permanente. Justo despues de aca
     el linker guarda los valores iniciales de .data (ver abajo). */
  _etext = .;

  /* -------------------------------------------------------------------------
   * .data - variables globales CON valor inicial   (int x = 5;)
   * -------------------------------------------------------------------------
   * Estas variables tienen dos direcciones distintas, y esa es la parte que
   * confunde a todo el mundo la primera vez:
   *
   *   - VMA (Virtual Memory Address): donde vive en ejecucion  -> RAM
   *   - LMA (Load Memory Address):    donde esta guardada      -> FLASH
   *
   * Tiene que estar en RAM porque puede cambiar, pero el valor inicial (el 5)
   * tiene que sobrevivir al apagado, o sea que tiene que estar en FLASH.
   *
   * La sintaxis "> RAM AT > FLASH" es exactamente eso: reservala en RAM, pero
   * guardala en FLASH. Copiar de una a la otra al arrancar es trabajo del
   * startup (startup_lpc1769.c), que para eso usa los tres simbolos de abajo.
   * ---------------------------------------------------------------------- */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* inicio en RAM     */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;        /* fin en RAM        */
  } > RAM AT > FLASH

  _sidata = LOADADDR(.data);   /* de donde copiar: la copia en FLASH */

  /* -------------------------------------------------------------------------
   * .bss - variables globales SIN valor inicial   (int y;  static char buf[64];)
   * -------------------------------------------------------------------------
   * No ocupan un solo byte de FLASH: seria absurdo grabar 64 ceros. Solo se
   * reserva el lugar en RAM, y el startup lo pone en cero. Por eso en C una
   * global sin inicializar arranca valiendo 0: no es el lenguaje, es codigo
   * que corre antes que main.
   * ---------------------------------------------------------------------- */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;     /* alias que espera newlib */
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > RAM

  /* -------------------------------------------------------------------------
   * ._user_heap_stack - el chequeo de que la RAM alcanza
   * -------------------------------------------------------------------------
   * No emite ningun byte: solo avanza el contador de posicion para verificar
   * que despues de .data y .bss todavia queda lugar para el heap y el stack.
   * Si no queda, el linker corta con "region RAM overflowed".
   * ---------------------------------------------------------------------- */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE(end = .);          /* newlib: _sbrk() arranca el heap aca */
    PROVIDE(_end = .);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /* -------------------------------------------------------------------------
   * Las dos SRAM del bus AHB (16 KB cada una)
   * -------------------------------------------------------------------------
   * Quedan vacias salvo que pidas explicitamente poner algo aca:
   *
   *     uint8_t __attribute__((section(".ahbram0"))) buffer_dma[1024];
   *
   * Sirven para buffers de DMA, USB y Ethernet: al estar en otro bus, el DMA
   * las puede usar sin pelearse con el core por el acceso a la SRAM principal.
   * NOLOAD = no ocupan nada en el .bin, es solo espacio reservado.
   * ---------------------------------------------------------------------- */
  .ahbram0 (NOLOAD) :
  {
    . = ALIGN(4);
    *(.ahbram0)
    *(.ahbram0*)
    . = ALIGN(4);
  } > AHBRAM0

  .ahbram1 (NOLOAD) :
  {
    . = ALIGN(4);
    *(.ahbram1)
    *(.ahbram1*)
    . = ALIGN(4);
  } > AHBRAM1

  /* Informacion de debug del compilador y basura que no va al chip. */
  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}


/* ---------------------------------------------------------------------------
 * Red de seguridad: que el linkeo falle antes que la placa
 * ---------------------------------------------------------------------------
 * Si las globales crecieron tanto que ya no entra el stack, es mejor enterarse
 * ahora con un error de linkeo y no debuggeando un hard fault intermitente.
 * ------------------------------------------------------------------------ */
ASSERT(_ebss + _Min_Heap_Size + _Min_Stack_Size <= ORIGIN(RAM) + LENGTH(RAM),
       "ERROR: no entra en la RAM. Las variables globales dejaron menos de _Min_Stack_Size libre para el stack.")
