# Tipos de ancho fijo, `volatile` y `const` (C para hardware)

Este capítulo es el **puente entre el C "de PC" y el C embebido**. Todo lo anterior (variables,
operadores, punteros, structs) sigue valiendo, pero al programar un microcontrolador aparecen tres
necesidades que en una PC casi no se notan:

1. **Saber exactamente cuántos bits ocupa cada variable.** Un registro de hardware tiene un ancho
   fijo (8, 16 o 32 bits). No podés darte el lujo de que `int` mida "lo que el compilador quiera".
2. **Decirle al compilador que una variable puede cambiar sola**, por fuera del programa (lo hace
   el hardware). Para eso está `volatile`.
3. **Marcar lo que no se debe modificar** (`const`), tanto por seguridad como para que el dato viva
   en Flash y no gaste RAM.

> Si entendés bien estos tres conceptos, el salto a "escribir registros del LPC1769" deja de ser
> un misterio. De hecho, el próximo módulo ([01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/))
> se apoya 100% en esto.

---

## 1. El problema de `int`, `short`, `long`

En C clásico los tipos enteros **no tienen un tamaño garantizado**. El estándar solo promete
mínimos:

| Tipo | Tamaño mínimo garantizado | Tamaño típico en Cortex-M3 |
|------|---------------------------|----------------------------|
| `char` | 8 bits | 8 bits |
| `short` | 16 bits | 16 bits |
| `int` | 16 bits | **32 bits** |
| `long` | 32 bits | 32 bits |
| `long long` | 64 bits | 64 bits |

El problema: el **mismo código** compilado para un AVR de 8 bits (donde `int` = 16 bits) y para el
LPC1769 (donde `int` = 32 bits) se comporta distinto. Y peor: cuando configurás un registro de 32
bits, necesitás un tipo que mida **exactamente 32 bits**, ni más ni menos.

### La solución: `<stdint.h>`

El header estándar `<stdint.h>` define tipos de **ancho exacto**, iguales en cualquier plataforma:

```c
#include <stdint.h>

uint8_t   a;   // entero sin signo de 8 bits   (0 .. 255)
uint16_t  b;   // entero sin signo de 16 bits  (0 .. 65535)
uint32_t  c;   // entero sin signo de 32 bits  (0 .. 4.294.967.295)
int8_t    d;   // entero CON signo de 8 bits   (-128 .. 127)
int32_t   e;   // entero CON signo de 32 bits
```

**Regla práctica en embebidos:** usá *siempre* los tipos de `<stdint.h>`. Cuando trabajás con un
registro de 32 bits, usás `uint32_t`. Para un contador chico que sabés que no pasa de 200, `uint8_t`.
Esto deja explícito el tamaño y ahorra memoria.

### Los tres headers que van juntos: `stdint.h`, `stdbool.h`, `limits.h`

`<stdint.h>` casi nunca viaja solo. Sus dos compañeros habituales:

```c
#include <stdint.h>    // uint8_t, int32_t, ...
#include <stdbool.h>   // bool, true, false  (en C, antes no existían)
#include <limits.h>    // UINT32_MAX, INT8_MIN, ...
```

- **`<stdbool.h>`** trae el tipo `bool` y las constantes `true`/`false`. En C clásico no existían
  (se usaba `int` con 0/1). Para una flag, `bool listo = false;` es más claro que `uint8_t`.
- **`<limits.h>`** te da los valores extremos de cada tipo (`UINT8_MAX`, `INT32_MIN`, etc.). Sirven
  para detectar overflow o para inicializar un "mínimo" buscando hacia abajo:

```c
uint16_t minimo = UINT16_MAX;     // arranco en el máximo posible
for (int i = 0; i < n; i++)
    if (muestras[i] < minimo) minimo = muestras[i];
```

### Constantes con ancho garantizado: `UINT32_C`, `INT16_C`

Hay un detalle sutil: cuando escribís `1 << 30`, ese `1` es un `int`. Si necesitás un literal con
ancho garantizado (por ejemplo para no perder bits en un corrimiento grande de 32 bits), `<stdint.h>`
define macros que le pegan el sufijo correcto:

```c
uint32_t mascara = UINT32_C(1) << 31;   // el literal es uint32_t, el bit 31 no se pierde
uint64_t grande  = UINT64_C(1) << 40;   // sin esto, 1 << 40 desbordaría como int
```

En la práctica, en código de 32 bits del Cortex-M3 suele alcanzar con escribir `1u << 31` (la `u`
lo hace `unsigned int`, que acá mide 32 bits). Pero conocer `UINT32_C`/`INT32_C` te salva en
corrimientos de 64 bits y en código portable.

### Imprimir tipos de ancho fijo: `<inttypes.h>` y `PRIu32`

¿Con qué especificador imprimís un `uint32_t`? ¿`%u`, `%lu`, `%d`? Depende de si `uint32_t` es
`unsigned int` o `unsigned long` en esa plataforma, y eso no es portable. La solución estándar son
las macros de `<inttypes.h>`:

```c
#include <inttypes.h>

uint32_t ticks = 123456;
printf("ticks = %" PRIu32 "\r\n", ticks);   // PRIu32 se expande al especificador correcto
int16_t  t = -5;
printf("temp = %" PRId16 "\r\n", t);
```

`PRIu32` es una cadena (`"u"` o `"lu"` según la plataforma) que el preprocesador pega junto al
literal `"ticks = %"`. La nomenclatura es `PRI` + formato (`d`/`u`/`x`) + ancho (`8`/`16`/`32`/`64`).
En un curso simple podés usar `%lu` (en Cortex-M3 `uint32_t` es `unsigned long`, así que funciona),
pero `PRIu32` es la forma "correcta" y portable.

```c
// ambiguo: ¿cuántos bits? ¿con o sin signo?
int contador;

// explícito: 8 bits sin signo, justo lo que un contador de 0-255 necesita
uint8_t contador;
```

> **Dato:** los drivers de CMSIS que vas a usar (`PINSEL_CFG_Type`, `GPIO_SetDir`, etc.) están
> escritos enteros con `uint8_t` / `uint32_t`. Por eso conviene acostumbrarse desde ahora.

### Con o sin signo: cuidado con las máscaras de bits

Cuando manipulás bits, **casi siempre querés `unsigned`**. Con tipos con signo, el bit más alto es
el de signo y los corrimientos a la derecha se comportan distinto:

```c
uint8_t  x = 0x80;   // 1000 0000
x = x >> 1;          // 0100 0000  (entra un 0) -> correcto para bits

int8_t   y = 0x80;   // -128 con signo
y = y >> 1;          // implementación-definida; puede entrar un 1 (corrimiento aritmético)
```

Para registros, **siempre `uintN_t`**.

---

## 2. `volatile`: "esto puede cambiar solo"

El compilador, para optimizar, asume que **si nadie en el código toca una variable, su valor no
cambia**. Entonces puede, por ejemplo, leerla una sola vez y guardarla en un registro del CPU en
lugar de volver a leer la memoria. En una PC eso es correcto. En un micro, **no**: el hardware
puede cambiar el contenido de una dirección por su cuenta.

### Ejemplo del problema

Imaginá que esperás a que un bit de "dato listo" de un periférico se ponga en 1:

```c
uint32_t *STATUS = (uint32_t *)0x40000000;   // registro de estado de un periférico

while ( (*STATUS & 0x01) == 0 ) {
    // esperar a que el hardware ponga el bit 0 en 1
}
```

El compilador ve que dentro del `while` nadie modifica `*STATUS`, así que "optimiza": lee el valor
**una vez**, y si era 0, asume que siempre va a ser 0 → **bucle infinito**, aunque el hardware ya
haya puesto el bit en 1.

### La solución: `volatile`

`volatile` le dice al compilador *"no optimices los accesos a esto; leelo de memoria cada vez"*:

```c
volatile uint32_t *STATUS = (volatile uint32_t *)0x40000000;

while ( (*STATUS & 0x01) == 0 ) {
    // ahora SÍ vuelve a leer la dirección en cada vuelta
}
```

### Qué garantiza `volatile` (y qué NO)

`volatile` es probablemente la palabra clave más importante (y más malentendida) del C embebido.
Conviene tenerlo clarísimo.

**Qué SÍ garantiza el estándar para una variable `volatile`:**

1. **Cada acceso en el código fuente es un acceso real a memoria.** El compilador no puede
   "cachear" el valor en un registro del CPU: si escribís `*STATUS` tres veces, hace tres lecturas
   de esa dirección. Y no puede *eliminar* una lectura o escritura "porque no se usa".
2. **No reordena los accesos `volatile` entre sí.** Si tu código lee `A` (volatile) y después
   escribe `B` (volatile), en el binario esa lectura de `A` ocurre antes que la escritura de `B`.
   El orden relativo de los accesos volatiles se respeta.

**Qué NO garantiza (y acá vive la mayoría de los bugs):**

1. **NO es atomicidad.** Un acceso a `volatile uint32_t` puede no ser una sola instrucción
   (peor aún con `uint64_t` o structs). Una operación como `contador++` sobre una variable volatile
   son **tres** pasos (leer, sumar, escribir): una interrupción puede colarse en el medio. `volatile`
   garantiza que las tres van a memoria, **no** que ocurran sin interrupción.
2. **NO es una barrera de memoria completa.** El compilador puede reordenar libremente accesos
   **no volatiles** alrededor de uno volatile. Y el `volatile` no emite instrucciones de barrera de
   hardware (`DMB`/`DSB`): esas son otra cosa (las ves en el bloque para curiosos del cap. 11).

> Para datos compartidos entre el `main` y una ISR, `volatile` es **necesario pero no suficiente**.
> Si la operación no es atómica (un `++`, leer dos campos que deben ser coherentes, un dato de más de
> 32 bits), necesitás además una **sección crítica** (deshabilitar la interrupción mientras tocás el
> dato). Eso se trata en
> [Módulo 07 - Secciones críticas y atomicidad](../07_interrupciones/03-secciones-criticas-y-atomicidad.md).

### Los tres casos canónicos de `volatile`

Casi todo uso de `volatile` cae en uno de estos tres. Aprendelos como patrones.

**Caso 1: Flag compartida con una ISR.** La ISR la modifica, el `main` la lee (o viceversa). Sin
`volatile`, el optimizador asume que el `main` es el único que la toca y puede leerla una sola vez.

```c
volatile bool flag_boton = false;   // la ISR la pone en true, el main la lee

void EINT3_IRQHandler(void) {        // interrupción
    flag_boton = true;
}

int main(void) {
    while (1) {
        if (flag_boton) {            // sin 'volatile' esto podría no actualizarse nunca
            flag_boton = false;
            // ...atender el botón...
        }
    }
}
```

> Acá `flag_boton` es un `bool` que solo cambia de `true` a `false` y viceversa: escribir un bool es
> atómico en el Cortex-M3, así que con `volatile` alcanza. Si en cambio compartieras un **contador**
> que la ISR incrementa y el main lee y resetea, ahí sí necesitás sección crítica.

**Caso 2: Registro de hardware mapeado en memoria.** El periférico cambia el contenido de la
dirección por su cuenta. Es el caso que vimos arriba con `STATUS`. Por eso **todo** puntero a un
registro es `volatile` (CMSIS los declara así siempre).

```c
volatile uint32_t *STATUS = (volatile uint32_t *)0x40000000;
while ( (*STATUS & 0x01) == 0 ) { /* relee la dirección en cada vuelta */ }
```

**Caso 3: Loop de espera / delay que el optimizador borraría.** Un delay "por software" cuenta
hacia abajo sin hacer nada útil. El optimizador ve que el resultado no se usa y **elimina el loop
entero**. Marcando la variable `volatile`, lo obligás a ejecutarlo:

```c
void delay_burdo(void) {
    for (volatile uint32_t i = 0; i < 100000; i++) {
        /* sin 'volatile', -O2 borra este loop completo y el delay desaparece */
    }
}
```

> Nota: este delay "por conteo" es solo para ilustrar. En la práctica usás el **SysTick**
> (Módulo 06), que es preciso y no quema CPU a ciegas.

### `const volatile`: registros de SOLO LECTURA

Un registro de estado lo escribe el hardware y vos solo lo leés. Querés las dos cosas a la vez:
`volatile` (cambia solo, releelo siempre) y `const` (que el compilador te frene si intentás
escribirlo por error):

```c
const volatile uint32_t *ADC_DR = (const volatile uint32_t *)0x40034004; // dato del ADC, solo lectura
uint32_t x = *ADC_DR;   // OK: leer
// *ADC_DR = 0;         // ERROR de compilación: es const -> bug atrapado en compile time
```

Esto es exactamente lo que CMSIS expresa con `__I` (`volatile const`), `__O` (`volatile`) y
`__IO` (`volatile`), que vemos en la sección 3.

### Punteros y `volatile`: dónde va la palabra

Igual que con `const`, la **posición** importa. Para registros casi siempre querés que sea volatile
el **dato apuntado**, no el puntero:

```c
volatile uint32_t *p;          // el DATO es volatile (*p cambia solo). <- esto querés para registros
uint32_t * volatile p;         // el PUNTERO es volatile (raro: p mismo cambia solo)
volatile uint32_t * volatile p;// ambos
```

Para acceder a un registro fijo, lo común es combinar `volatile` (el dato cambia) con la dirección
constante: `*((volatile uint32_t *)0x40000000)`.

---

## 3. `const`: "esto no se modifica"

`const` marca un dato como de solo lectura. Tiene dos usos clave en embebidos:

**a) Seguridad / intención.** Un parámetro `const` documenta que la función no lo va a tocar:

```c
void enviar_buffer(const uint8_t *datos, uint32_t largo);  // promete no modificar 'datos'
```

**b) Ahorrar RAM poniendo datos en Flash.** En un micro, la Flash (programa) suele ser mucho más
grande que la RAM. Una tabla `const` global puede ubicarse en Flash en lugar de gastar RAM:

```c
// Tabla de senos para un DAC: 256 valores. Es const -> va a Flash, no a RAM.
const uint16_t tabla_seno[256] = { 512, 524, 536, /* ... */ };
```

### `const` y punteros (combinado con registros)

La posición de `const` cambia su significado. Para registros vas a ver combinaciones con `volatile`:

```c
volatile uint32_t       *p1;   // puntero a un registro de lectura/escritura
volatile const uint32_t *p2;   // puntero a un registro de SOLO LECTURA (ej: un registro de estado)
```

CMSIS usa exactamente esto: define `__I` (input, `volatile const`), `__O` (output, `volatile`) y
`__IO` (`volatile`) para marcar qué registros son de solo-lectura, solo-escritura o ambos. Lo vas a
ver en detalle en el módulo [02 - Armá tu propia librería](../02_arma_tu_propia_libreria/).

---

## 4. Resumen

| Concepto | Para qué | Palabra clave |
|----------|----------|---------------|
| Ancho exacto de un entero | Que una variable mida los bits justos de un registro | `uint8_t`, `uint16_t`, `uint32_t` (`<stdint.h>`) |
| Sin signo para bits | Corrimientos y máscaras predecibles | `uintN_t` |
| Valor que cambia solo | Registros de hardware, variables compartidas con ISR | `volatile` |
| Valor de solo lectura | Documentar intención, mandar datos a Flash | `const` |

### Lo que viene
Con esto ya tenés todo el C que necesitás para el siguiente paso: entender que **un registro de
hardware es, literalmente, una dirección de memoria con un `volatile uint32_t *` apuntándola**.
Eso es el módulo [01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/).

---

**Anterior:** [07 - Estructuras, uniones y enums](./07-estructuras-uniones-enums.md) ·
**Siguiente:** [09 - Asignación dinámica](./09-asignacion-dinamica.md)
