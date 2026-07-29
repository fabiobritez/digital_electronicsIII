# Bit-banding (acceso atómico a bits)

Esta es una página **avanzada** y opcional. El material viejo de GPIO nombra el "bit-banding" sin
explicarlo, y queda la duda. Acá se entiende qué es, cuándo sirve de verdad y por qué la mayor parte
del tiempo **no lo vas a necesitar**, pero conviene saber que existe.

## El problema que resuelve

Para cambiar **un solo bit** de un registro, lo normal es leer-modificar-escribir con una máscara:

```c
LPC_GPIO0->FIOPIN |= (1u << 5);     // poner el bit 5 en 1
```

Ese `|=` no es **una** instrucción: el CPU **lee** el registro, le aplica el OR y lo **escribe** de
vuelta. Son tres pasos. Si entre el "leer" y el "escribir" salta una interrupción que toca **otro** bit
del mismo registro, su cambio se pierde cuando el código original escribe su versión vieja. Es la misma
condición de carrera que vimos en [interrupciones, pág. 03](../07_interrupciones/03-secciones-criticas-y-atomicidad.md):
una operación **read-modify-write no es atómica**.

El bit-banding del Cortex-M3 ofrece una salida elegante: que escribir un bit sea **una sola escritura**,
imposible de interrumpir a la mitad.

## La idea: un bit = una dirección entera

El Cortex-M3 reserva dos regiones especiales y, para cada una, una **región alias** mucho más grande.
En la región alias, **cada palabra de 32 bits corresponde a un único bit** de la región original:

- Escribís `1` o `0` en esa palabra alias → el hardware cambia **solo ese bit** en el registro real.
  Solo cuenta el **bit 0** del valor que escribas: el manual aclara que escribir `0xFF` equivale a
  escribir `0x01`, y `0x0E` equivale a `0x00` (§34.3.2.5.1).
- Leés esa palabra alias → te devuelve `0x0000_0000` o `0x0000_0001` según ese bit.

Como para tu programa es **una sola instrucción** de escritura, es **atómica**: ninguna interrupción
la puede partir al medio. (Por dentro el hardware sí hace un leer-modificar-escribir sobre la palabra
real (tabla 637 del manual), pero lo hace él, de forma indivisible; no tu código en tres pasos.)
Y de paso, el código queda más legible: `BIT(addr, n) = 1;` en vez de la máscara.

Las dos regiones con bit-banding son el **primer megabyte** de la zona SRAM y el primer megabyte de
la zona de periféricos del mapa del Cortex-M3 (tablas 637 y 638 del apéndice, cap. 34):
- **SRAM:** `0x2000_0000`–`0x200F_FFFF` (1 MB), con alias en `0x2200_0000`–`0x23FF_FFFF` (32 MB).
- **Periféricos:** `0x4000_0000`–`0x400F_FFFF` (1 MB), con alias en `0x4200_0000`–`0x43FF_FFFF` (32 MB).

¿Qué cae adentro de esas ventanas en el LPC1769? Volvé al [mapa de memoria](./01-mapa-de-memoria.md):

- La ventana de periféricos cubre **todo APB0 y APB1** (desde el WDT en `0x4000_0000` hasta el
  System Control en `0x400F_C000`).
- La ventana de SRAM cubre la **SRAM AHB** (`0x2007_C000`–`0x2008_3FFF`) y (dato curioso) también
  los registros `FIOxxx` del GPIO rápido: `0x2009_C000` es `0x2000_0000 + 0x9_C000`, dentro del
  primer MB. O sea que a un pin **sí** se le puede aplicar bit-banding.
- La que queda **afuera** es justo la que más usás: la **SRAM local de 32 KB** en `0x1000_0000`,
  donde el linker pone tus variables por defecto. En el mapa del Cortex-M3 esa dirección pertenece a
  la región *Code*, no a la región SRAM, así que **tus variables comunes no tienen bit-banding**.
  Solo lo tienen las que ubiques a propósito en la SRAM AHB.

## La fórmula

Dada una dirección `addr` dentro de una región bit-band y el número de bit `n` (0–31), la dirección
alias que controla ese bit es:

```
alias = base_alias + (addr - base_region) * 32 + n * 4
```

El `* 32` porque cada palabra original (4 bytes = 32 bits) se "estira" en 32 palabras alias de 4 bytes
cada una; el `* 4` porque cada bit ocupa una palabra de 4 bytes en el alias.

En C se suele encapsular con un par de macros:

```c
#define BITBAND_SRAM(addr, bit) \
    ((volatile uint32_t *)(0x22000000u + (((uint32_t)(addr) - 0x20000000u) * 32u) + ((bit) * 4u)))

// una flag compartida con una ISR. OJO: tiene que vivir dentro de la ventana,
// o sea en la SRAM AHB, no en la SRAM local donde el linker pone todo por defecto:
__attribute__((section(".bss.$RamAHB32")))
volatile uint32_t flags;

#define READY  (*BITBAND_SRAM(&flags, 3))   // el bit 3 de 'flags', como si fuera un bool

void set_ready(void)   { READY = 1; }       // una sola escritura: atómica
uint8_t is_ready(void) { return READY; }    // lee 0 o 1
```

El `__attribute__((section(".bss.$RamAHB32")))` le pide al linker de MCUXpresso que ponga `flags` en
la SRAM AHB (`0x2007_C000`), que sí está dentro de la ventana (los nombres de sección dependen del
*linker script*; lo vemos en el [módulo 16](../16_build_linker_startup/)). Si te olvidás de esto y
`flags` queda en la SRAM local (`0x1000_0000`), la macro calcula una dirección que no corresponde a
nada y el acceso termina en un Bus Fault o en datos corruptos: el error clásico con bit-banding en
esta familia.

`set_ready()` cambia el bit 3 de `flags` **sin** un read-modify-write, así que es seguro hacerlo desde
una ISR aunque el `main` esté tocando **otro** bit de la misma variable.

## Cuándo usarlo (y cuándo no)

**Sí aporta:**
- **Flags booleanas compartidas entre ISR y main** empaquetadas en una misma palabra (ubicada en la
  SRAM AHB): cada lado toca su bit sin pisarse y sin sección crítica.
- Código que necesita poner/leer bits sueltos con mucha frecuencia y querés legibilidad y atomicidad.

**No vale la pena:**
- Para **pines GPIO**: aunque los `FIOxxx` caen en la ventana y el truco funciona,
  `FIOSET`/`FIOCLR`/`FIOMASK` ya te dan acceso atómico y selectivo, y son la forma idiomática
  (módulo 5).
- Si no compartís el bit con una interrupción, la máscara de toda la vida es perfectamente correcta y
  más portable (el bit-banding es específico de Cortex-M3/M4; no existe en todos los micros).

> Regla práctica: **no salgas a usar bit-banding por defecto.** Conocelo para reconocerlo en código
> ajeno y para tenerlo como herramienta cuando tengas el problema exacto que resuelve (un bit compartido
> con una ISR). Para el 95% del curso, máscaras + `FIOSET`/`FIOCLR` + secciones críticas alcanzan.

## Lo que te llevás
- El bit-banding mapea **cada bit** del primer MB de las regiones SRAM y de periféricos a **una
  dirección de 32 bits** propia.
- Escribir esa dirección cambia **un solo bit** de forma **atómica**: ni una ISR la parte.
- En el LPC1769 las ventanas cubren **todos los periféricos APB**, la **SRAM AHB** y hasta los
  `FIOxxx` del GPIO rápido, pero **no** la SRAM local de `0x1000_0000`, donde viven tus variables
  por defecto. Para bit-bandear una variable tenés que ponerla a propósito en la SRAM AHB.
- Es una herramienta de nicho: útil para flags compartidas ISR↔main, innecesaria para casi todo lo demás.

---

**Anterior:** [03 - De direcciones a structs CMSIS](./03-de-direcciones-a-structs-cmsis.md) ·
**Módulo:** [Arquitectura y acceso a registros](./README.md) ·
**Siguiente módulo:** [02 - Armá tu propia librería](../02_arma_tu_propia_libreria/)
