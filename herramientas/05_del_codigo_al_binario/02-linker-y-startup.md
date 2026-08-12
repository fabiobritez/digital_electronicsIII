# El linker script y el startup

Dos archivos conectan los objetos compilados con el hardware concreto: el **linker script** define
dónde queda cada sección y el **startup** prepara el entorno antes de `main`. En MCUXpresso
vienen generados; acá los desarmamos con el ejemplo **real y compilable** de `mygpio`
([`../02_arma_tu_propia_libreria/src/build/`](../../curso/02_arma_tu_propia_libreria/src/build/)).

## El linker script (`.ld`): el mapa de memoria

El linker necesita saber **qué memoria tiene el chip** y **dónde poner cada sección**. Eso es el
linker script. El de `mygpio`, comentado:

```ld
MEMORY {
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K   /* la Flash del LPC1769 */
  RAM   (rwx) : ORIGIN = 0x10000000, LENGTH = 32K    /* la SRAM local */
}

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* el stack arranca en el tope de la RAM */

SECTIONS {
  .text : {
    KEEP(*(.isr_vector))   /* PRIMERO la tabla de vectores, en 0x0 */
    *(.text*)              /* después, todo el código */
    *(.rodata*)            /* y las constantes */
    _etext = .;            /* marca dónde termina (= dónde empiezan los valores de .data en Flash) */
  } > FLASH

  .data : AT (_etext) {    /* .data vive en RAM, pero se CARGA desde Flash (AT) */
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM

  .bss : {
    _sbss = .;
    *(.bss*) *(COMMON)
    _ebss = .;
  } > RAM
}
```

Lo importante:

- **`MEMORY`** describe las regiones físicas del chip: Flash en `0x0` (512 KB) y SRAM en
  `0x10000000` (32 KB). Estos números salen del [mapa de memoria del módulo 1](../../curso/01_arquitectura_y_acceso_a_registros/01-mapa-de-memoria.md).
  Al portar, esta es una de las partes que cambia, junto con startup, vectores y configuración del
  dispositivo.
- **La tabla de vectores va primero** (`KEEP(*(.isr_vector))`), porque el micro la espera en
  `0x0` (recordá: el reset lee `0x0` y `0x4`).
- **`.data : AT (_etext)`** es el truco de la página anterior: la sección está *en RAM* (`> RAM`)
  pero su contenido inicial se *guarda en Flash* (`AT (_etext)`, justo después del código). El startup
  la copia.
- Los símbolos `_etext`, `_sdata`, `_edata`, `_sbss`, `_ebss`, `_estack` los **exporta el linker** para
  que el startup sepa de dónde a dónde copiar y limpiar.

## El startup: lo que corre antes de `main`

Cuando el micro resetea, **no salta a `main`**. Salta al **Reset_Handler**, que prepara el terreno y
recién después llama a `main`. El startup de `mygpio`, en C, comentado:

```c
extern uint32_t _etext, _sdata, _edata, _sbss, _ebss, _estack;
int main(void);

/* La tabla de vectores: lo primero en la Flash (0x0).
   [0] = valor inicial del stack pointer, [1] = a dónde saltar al resetear. */
__attribute__((section(".isr_vector"), used))
void (* const g_vectors[])(void) = {
    (void (*)(void)) &_estack,   /* 0x00: SP inicial = tope de la RAM */
    Reset_Handler,               /* 0x04: el reset salta acá */
    NMI_Handler, HardFault_Handler, /* ... resto de excepciones del núcleo ... */
};

void Reset_Handler(void) {
    /* 1) copiar .data de Flash a RAM (los valores iniciales de las globales) */
    uint32_t *src = &_etext, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;

    /* 2) poner .bss en cero (las globales sin inicializar) */
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;

    /* 3) (en un proyecto CMSIS, acá iría SystemInit() para configurar el clock) */

    main();                      /* 4) recién ahora, tu programa */
    while (1) {}                 /* si main retorna, no hay a dónde volver */
}
```

Paso a paso, lo que pasa al encender o resetear:

1. El hardware lee `0x00000000` → carga el **stack pointer** con `_estack` (tope de RAM).
2. El hardware lee `0x00000004` → salta a **`Reset_Handler`**.
3. `Reset_Handler` **copia `.data`** (así tus `int x = 5;` valen 5) y **pone `.bss` en cero** (así
   tus globales sin inicializar valen 0).
4. (En un proyecto con CMSIS, llama a **`SystemInit()`** → configura la PLL y deja el clock a 100 MHz,
   [módulo 3 del curso](../../curso/03_clock_y_power/).)
5. Llama a **`main()`**. Recién ahí empieza "tu" programa.

> C exige que esos objetos tengan sus valores iniciales antes de entrar a `main`. En bare metal, el
> startup materializa esa garantía copiando `.data` y limpiando `.bss`.

## La tabla de vectores, de nuevo (ahora se cierra el círculo)

En el [módulo 7](../../curso/07_interrupciones/01-nvic-y-vectores.md) dijimos que cada interrupción tiene una
entrada en la tabla de vectores con la dirección de su handler. **Esta es esa tabla.** Después de las
excepciones del núcleo (Reset, NMI, HardFault, SysTick…), siguen los IRQ de los periféricos
(`TIMER0_IRQHandler`, `UART0_IRQHandler`, …). El startup suele declarar handlers *weak* con esos nombres y coloca sus direcciones en la tabla.
Cuando tu proyecto define una versión fuerte con el mismo símbolo, el linker reemplaza el handler
por defecto. El nombre debe coincidir exactamente.

> El startup provisto por el IDE sigue este esquema y agrega la tabla completa de 35 IRQ del
> LPC1769, aliases *weak*, inicialización de biblioteca y otros detalles del runtime. En MCUXpresso es
> `cr_startup_lpc175x_6x.c` (también en C); en otros entornos lo vas a ver como
> `startup_LPC17xx.s`, en assembler. El de `mygpio` es la versión mínima, para que se entienda.

## Antes de tu código: la boot ROM y el "código válido"

En rigor, al resetear tu código **no** es lo primero que corre. El LPC1769 tiene una **boot ROM**
de 8 KB (en `0x1FFF0000`, la viste en el mapa del módulo 1) con el bootloader de fábrica, que tras
el reset se mapea temporalmente en `0x0` y corre primero. Ese bootloader (capítulo 32 del manual):

1. Mira el pin **P2.10**: si está bajo, entra en modo **ISP** (grabación por UART0, [ver 07-02](../07_lpc1769/02-la-boot-rom-el-isp-y-el-checksum.md)) en
   lugar de arrancar tu programa.
2. Si no, verifica que haya **código válido** en la Flash: suma los primeros **8 words** de la
   tabla de vectores y el resultado tiene que dar **0**. Para que eso cierre, el vector 7 (offset
   `0x1C`, una posición *reservada* del Cortex-M3) debe contener el complemento a dos de la suma
   de los vectores 0 a 6.
3. Si el checksum da 0, restaura el mapeo (tu Flash vuelve a verse en `0x0`) y transfiere el
   control a tu código, que arranca como describimos arriba. Si no, asume que la Flash está vacía
   o corrupta y se queda en ISP, esperando por UART0.

¿Y quién calcula ese checksum? Depende del flujo. Algunas herramientas lo corrigen durante el
grabado y otras esperan que la imagen ya sea válida. En el ejemplo mínimo de `mygpio`, el vector
reservado queda en cero y el grabador usado por aquel ejemplo debe completarlo. La
[plantilla](../../plantilla/) adopta un criterio más seguro: lo inserta durante el build para que el
resultado no dependa del programa de grabación.

> Esta sección es el resumen de una etapa. La secuencia **completa**, desde que la
> alimentación empieza a subir hasta la primera instrucción de tu `main()`, con los tiempos
> del manual y los registros medidos sobre la placa, está en la
> [página 03](./03-el-arranque-paso-a-paso.md).

## Verificalo vos

En [`../02_arma_tu_propia_libreria/src/build/`](../../curso/02_arma_tu_propia_libreria/src/build/) está todo
junto con un `Makefile`. Con el toolchain local del repo:

```bash
cd curso/02_arma_tu_propia_libreria/src/build
make            # compila y linkea -> mygpio.elf + mygpio.bin
```

Las dos primeras palabras del `.bin` son la tabla de vectores: el SP inicial (`0x10008000`, tope de
los 32 KB de RAM) y la dirección del Reset_Handler. Es, literalmente, lo primero que lee el micro.

Y para verlo con las 8 palabras que mira la boot ROM, incluido el checksum:

```bash
cd plantilla
make vectores
```

```
Tabla de vectores (las 8 palabras que mira la boot ROM):
 0000 00800010 b1010000 ad010000 ad010000  ................
 0010 ad010000 ad010000 ad010000 ee75ffef  .............u..
lpc_checksum: build/firmware.elf [ELF] checksum OK (tiene 0xEFFF75EE, ...)
```

Los bytes están al revés porque el ARM es *little endian*: `00800010` leído como palabra de
32 bits es `0x10008000`, el tope de la RAM. La segunda, `0x000001B1`, es la dirección del
`Reset_Handler`... con el bit 0 en 1, que es como el Cortex-M3 marca "esta dirección tiene
código Thumb". La octava es el checksum que hace que la suma de las ocho dé exactamente
cero.

## Las versiones completas

El linker script y el startup de esta página son **mínimos a propósito**, para que se entienda la
idea. Las versiones completas del repositorio, con la tabla de vectores de los 35
IRQ del LPC1769, las SRAM del bus AHB, el heap y la verificación de que la RAM alcanza,
están en la [plantilla](../../plantilla/) y también están comentadas línea por línea:

- [`plantilla/linker/lpc1769.ld`](../../plantilla/linker/lpc1769.ld)
- [`plantilla/startup/startup_lpc1769.c`](../../plantilla/startup/startup_lpc1769.c)

## Lo que te llevás

- El **linker script** describe regiones y secciones. Portar también requiere revisar startup,
  bibliotecas y configuración del target.
- El **startup** prepara la RAM, puede llamar a `SystemInit()`, inicializa el runtime necesario y
  entra a `main`.
- La **tabla de vectores** (al principio de la Flash) conecta el reset y cada interrupción con su
  función.
- Antes de todo eso corre la **boot ROM**: chequea P2.10 (¿entrar a ISP?) y el checksum del
  vector 7 (¿hay código válido?). Recién entonces arranca tu firmware.
- MCUXpresso genera o incorpora estas piezas. Antes de modificarlas, podés inspeccionar el mapa, los
  símbolos y el desensamblado para verificar el efecto.

---

**Del código al binario:** [índice](./README.md) ·
**Anterior:** [01 - De código a binario](./01-de-codigo-a-binario.md) ·
**Siguiente:** [03 - El arranque paso a paso](./03-el-arranque-paso-a-paso.md)
