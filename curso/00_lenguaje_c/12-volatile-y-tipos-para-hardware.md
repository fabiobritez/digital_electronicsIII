# `volatile`, `const` y tipos propios: el C que toca el hardware

Este capítulo es el **puente entre el C "de PC" y el C embebido**. Todo lo anterior sigue valiendo,
pero al programar un microcontrolador aparecen dos necesidades que en una PC casi no se notan:

1. **Decirle al compilador que una variable puede cambiar sola**, por fuera del programa, porque la
   cambia el hardware. Para eso está `volatile`, y es la palabra más importante y peor entendida del
   C embebido.
2. **Ponerle nombre propio a los tipos** y poder tratar una dirección de memoria como número y como
   puntero indistintamente. Para eso están `typedef` y `uintptr_t`.

> [!NOTE]
> **Lo que este capítulo da por sabido.** Los tipos de ancho fijo (`uint8_t`, `uint32_t`, la tabla de
> `<stdint.h>`, `UINT32_C`, `PRIu32`, `size_t`) ya están en
> [01 - Declaraciones, tipos y constantes](./01-declaraciones-y-tipos.md#conclusión-práctica-tipos-de-ancho-fijo-stdinth),
> junto con los tamaños reales en el Cortex-M3 y el porqué de que `uint8_t` no siempre sea más barato
> que `uint32_t`. Las promociones enteras y el problema de `signed` contra `unsigned` en máscaras están
> en [02](./02-arreglos-conversiones-y-promociones.md#promociones-enteras-el-bug-silencioso-de-los-tipos-chicos)
> y [03](./03-operadores.md#cuidados-con-los-desplazamientos-importante-en-embebido). Si algo de eso
> no te suena, volvé un rato allá: acá arrancamos desde ahí.

> Si entendés bien lo de este capítulo, el salto a "escribir registros del LPC1769" deja de ser un
> misterio. De hecho, el módulo
> [01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/) se apoya 100% en
> esto, y el capítulo siguiente ([13 - Structs para hardware](./13-structs-para-hardware.md)) usa
> `volatile uint32_t` en cada línea para mapear periféricos.

---

## 1. Ponerle nombre a un tipo: `typedef` y el sufijo `_t`

`uint32_t` no es una palabra reservada de C. Si abrís `<stdint.h>` vas a encontrar, más o menos,
esto:

```c
typedef unsigned int uint32_t;
```

Eso es todo: **`uint32_t` es un apodo para `unsigned int`**, puesto por el header que vino con tu
compilador. Y la `t` final es de *type*: una convención para que al leer el código sepas que ese
nombre es un **tipo** y no una variable ni una función.

La herramienta que crea esos apodos es **`typedef`**, y la sintaxis es la de una declaración normal
con `typedef` adelante: donde iría el *nombre de la variable*, va el *nombre del tipo nuevo*.

```c
unsigned int  contador;    // declara una VARIABLE llamada 'contador'
typedef unsigned int  u32; // declara un TIPO llamado 'u32'
```

Con eso ya podés fabricar los tuyos:

```c
typedef uint16_t  milivolts_t;      // un alias con un nombre que dice qué mide
typedef uint32_t  ticks_t;
typedef uint8_t   pin_t;

milivolts_t bateria = 3300;
ticks_t     desde_el_arranque = 0;
```

¿Para qué sirve, si `milivolts_t` es exactamente `uint16_t`? Para **documentar la intención** y para
**poder cambiarla en un solo lugar**: si mañana la medición necesita 32 bits, tocás el `typedef` y
todo el programa se adapta. Es la misma razón por la que existe `uint32_t` en vez de escribir
`unsigned int` en todos lados: en otro micro, `uint32_t` apunta a otro tipo y tu código no cambia.

> [!WARNING]
> **`typedef` no crea un tipo nuevo: crea un sinónimo.** No hay ninguna protección extra. Esto
> compila **sin un solo warning**, ni siquiera con `-Wall -Wextra -Wconversion`:
>
> ```c
> typedef uint32_t celsius_t;
> typedef uint32_t fahrenheit_t;
>
> celsius_t    c = 20;
> fahrenheit_t f = c;    // mezcla grados C con grados F, y el compilador no dice nada
> ```
>
> Para el compilador los dos son `uint32_t` y punto. `typedef` es documentación, no una red de
> seguridad. (En C, la única forma de que el compilador te frene es envolver el valor en una
> `struct`, y ahí pagás incomodidad.)

Los mismos `typedef` se usan para los tipos compuestos, y esos ya los viste o los vas a ver:

| Qué querés nombrar | Cómo | Dónde se explica |
|---|---|---|
| Un entero con otro nombre | `typedef uint16_t milivolts_t;` | acá |
| Una `struct` | `typedef struct { ... } sensor_t;` | [05 - Estructuras](./05-estructuras-y-enums.md#declaración-con-typedef) |
| Un `enum` | `typedef enum { ROJO, VERDE } color_t;` | [05 - Enums](./05-estructuras-y-enums.md#uso-con-typedef) |
| Un puntero a función | `typedef void (*RxCallback)(uint8_t);` | [09 - Punteros avanzados](./09-punteros-avanzado.md) |

> **Sobre el sufijo:** el estándar de C se **reserva** los nombres `intN_t`, `uintN_t`,
> `int_fastN_t` y compañía para futuras versiones de `<stdint.h>`, y POSIX se reserva el sufijo
> `_t` en general. En la práctica todo el mundo lo usa igual para sus propios tipos (CMSIS,
> los drivers de NXP y este curso incluidos), y no vas a tener problemas. Solo no inventes cosas
> como `uint24_t`, que sí pisan el terreno reservado.

### `uintptr_t`: la dirección como número y como puntero

De las familias de `<stdint.h>` que viste en el
[capítulo 01](./01-declaraciones-y-tipos.md#conclusión-práctica-tipos-de-ancho-fijo-stdinth), hay una
que recién ahora cobra sentido, porque solo aparece cuando empezás a tocar hardware.

**`uintptr_t`.** Es el entero en el que garantizadamente entra una dirección. Es el tipo correcto
para el gesto que define este curso —tratar una dirección como número y como puntero—, y el que hace
que la intención quede escrita:

```c
#include <stdint.h>

/* La dirección como NÚMERO */
uintptr_t base_gpio0 = 0x2009C000u;

/* La misma dirección como PUNTERO al registro */
volatile uint32_t *fiodir = (volatile uint32_t *) base_gpio0;
*fiodir |= (1u << 22);
```

En el Cortex-M3 `uintptr_t` es de 32 bits, igual que `uint32_t`, así que en este micro son
intercambiables y por eso vas a ver `uint32_t` en casi todo el código de LPC (y en este curso).
La diferencia aparece al portar: en un micro de 64 bits, un puntero **no entra** en un `uint32_t` y
`uintptr_t` sí.

### Qué ancho elegir para *tus* variables

En el [capítulo 01](./01-declaraciones-y-tipos.md#tamaño-real-de-los-tipos-en-el-cortex-m3-lpc1769)
viste, con el desensamblado al lado, que en un núcleo de 32 bits `uint8_t` **no** es más barato: el
compilador tiene que recortar el resultado con un `uxtb` después de cada operación. Ahora que vamos a
empezar a tocar registros conviene tener el criterio completo en una tabla, porque son dos decisiones
distintas:

| Usá el tipo chico (`uint8_t`, `uint16_t`) | Usá `uint32_t` (o `uint_fast8_t`) |
|---|---|
| **Arreglos**: `uint8_t buf[512]` sí son 512 bytes y no 2048 | **Variables locales** y parámetros |
| **Campos de `struct`**, donde el ahorro se multiplica | **Contadores** de bucle e índices |
| **Registros de hardware** de 8 o 16 bits | **Acumuladores** y cuentas intermedias |
| **Protocolos**: el ancho lo define el formato, no vos | Cuando el valor **entra** en 32 bits sin problema |

En una palabra: **almacenás angosto, calculás ancho.**

Si querés dejar escrito "necesito al menos 8 bits, elegí vos el más eficiente", ese es exactamente el
significado de **`uint_fast8_t`** (en el Cortex-M3 son 32 bits). Es la opción correcta y
autodocumentada para un contador chico, aunque en la práctica casi todo el código de LPC —y este
curso— escribe `uint32_t` directamente.

> Para **registros** no hay decisión que tomar: usás el ancho exacto que dice el manual, siempre. Lo
> de arriba es el criterio para **tus** variables.

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
   hardware (`DMB`/`DSB`): esas son otra cosa (las ves en el bloque para curiosos del cap. 14).

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
escribirlo por error).

El caso de libro en el LPC1769 es `U0LSR`, el registro de estado de la UART0, en `0x4000C014`. El
manual ([UM10360](../../UM10360.pdf), capítulo 14, §14.4.8) es explícito: *"The UnLSR is a
**read-only** register that provides status information on the UARTn TX and RX blocks"*, y la tabla
271 lo marca `RO`. Su bit 0 (`RDR`, *Receiver Data Ready*) vale 1 cuando llegó un byte sin leer.

```c
#include <stdint.h>

static const volatile uint32_t * const U0LSR =
        (const volatile uint32_t *) 0x4000C014;

// Espera hasta que llegue un byte por el puerto serie.
void esperar_byte(void) {
    while ((*U0LSR & 1u) == 0) {
        // bit 0 (RDR) todavía en 0: no llegó nada
    }
}

// *U0LSR = 0;   // ERROR de compilación: es const -> bug atrapado antes de correr
```

Esta declaración junta todo lo del módulo, así que vale desarmarla:

| Parte                | Función                                                            |
| -------------------- | ------------------------------------------------------------------ |
| `static`             | Solo visible en este archivo                                       |
| `const` (el primero) | Lo **apuntado** no se puede escribir: el manual marca `U0LSR` como `RO` |
| `volatile`           | Lo apuntado cambia por hardware, cuando llega un byte               |
| `uint32_t`           | Tipo base: los registros del LPC1769 son de 32 bits                |
| `* const`            | El **puntero** es constante: no puede apuntar a otra dirección     |
| `U0LSR`              | Nombre de la variable                                              |
| `= ...`              | Se inicializa apuntando a una dirección fija de memoria            |

Tres cosas que importan:

1. **El `const` de la izquierda y el de la derecha son distintos.** El de antes del `*` califica el
   **dato apuntado**; el de después, el **puntero**. Se lee de derecha a izquierda: *"`U0LSR` es un
   puntero `const` a un `uint32_t` `const volatile`"* (la regla completa está en
   [08 - Punteros](./08-punteros.md#const-y-punteros-const-correctness)). Los dos errores que tira
   `arm-none-eabi-gcc` usan **palabras distintas**, y esa diferencia te dice cuál de los dos tocaste:
   ```console
   *U0LSR = 0;              error: assignment of read-only location '*(const volatile uint32_t *)U0LSR'
   U0LSR  = otra_direccion; error: assignment of read-only variable 'U0LSR'
   ```
   *location* es el dato apuntado; *variable* es el puntero.
2. **Acá el `volatile` no es un adorno: sin él el programa se cuelga.** Dentro del `while` no hay
   nada que modifique `*U0LSR`, así que el compilador puede leerlo **una sola vez**, guardarlo en un
   registro de la CPU y repetir la comparación para siempre. Con `-O0` puede que "funcione" de
   casualidad; al compilar la versión final con `-O2`, el lazo no termina nunca.
3. **La inicialización va a nivel de archivo, pero la lectura no.** `uint32_t s = *U0LSR;` **fuera**
   de una función no compila: los inicializadores de variables con duración estática deben ser
   expresiones constantes, y desreferenciar un puntero no lo es. Por eso la lectura va adentro de
   `esperar_byte()`.

> **Un detalle que muestra por qué `volatile` es serio:** en `U0LSR`, *leer el registro tiene
> efecto*. El manual aclara que una lectura de `UnLSR` **limpia** los bits de error (*overrun*,
> paridad, *framing*). O sea que cada lectura es una acción sobre el hardware, no una consulta
> inocente. `volatile` es lo que le prohíbe al compilador agregar lecturas de más o borrar las que
> escribiste. Un registro puede ser `const` (no lo escribo) y aun así tener efectos secundarios al
> leerlo: son dos cosas independientes.

**Todo junto se lee así:** *"`U0LSR` es una variable privada de este archivo (`static`), que guarda
para siempre la misma dirección (`* const`), la `0x4000C014`; ahí hay un dato de 32 bits
(`uint32_t`) que no se puede escribir (`const`) y que cambia por su cuenta en cualquier momento, así
que hay que ir a buscarlo a memoria cada vez (`volatile`)."*

> **Ojo con copiar este patrón a cualquier registro.** `const` solo va si el registro es de verdad de
> solo lectura. Por ejemplo `AD0GDR` (el resultado del ADC, `0x40034004`) **parece** de solo lectura
> pero el manual lo marca `R/W`, y `FIO0PIN` también: escribirle cambia la salida del puerto. Si le
> ponés `const` a un registro que sí se escribe, te vas a quedar sin poder usarlo. Verificá siempre
> la columna *Access* del mapa de registros.

Esto es exactamente lo que CMSIS expresa con `__I` (`volatile const`), `__O` (`volatile`) y
`__IO` (`volatile`), que vemos en la sección 3.

### Punteros y `volatile`: dónde va la palabra

Igual que con `const`, la **posición** importa. Para registros casi siempre querés que sea volatile
el **dato apuntado**, no el puntero:

```c
volatile uint32_t *dato_vol;           // el DATO es volatile (*p cambia solo)
                                       //   <- esto es lo que querés para un registro

uint32_t * volatile punt_vol;          // el PUNTERO es volatile (raro: la variable p
                                       //   cambia sola, no lo apuntado)

volatile uint32_t * volatile ambos;    // las dos cosas
```

La regla es la misma que con `const`: **lo que está antes del `*` califica al dato apuntado; lo que
está después, al puntero.** Para un registro querés lo primero, porque lo que cambia por su cuenta es
el contenido de la dirección, no la dirección.

Para acceder a un registro fijo, lo común es combinar las dos ideas: el dato `volatile` y la dirección
constante.

```c
// forma corta, sin variable intermedia
*((volatile uint32_t *) 0x40000000) = 0x01;

// forma con nombre, que es la que conviene y la que usa el capítulo siguiente
static volatile uint32_t * const REG = (volatile uint32_t *) 0x40000000;
*REG = 0x01;
```

---

## 3. Cómo lo escribe CMSIS: `__I`, `__O`, `__IO`

Todo lo anterior no es una curiosidad académica: es literalmente cómo están escritos los headers que
vas a usar. CMSIS define tres macros para marcar la dirección de cada registro, y son exactamente las
combinaciones de este capítulo:

| Macro CMSIS | Se expande a | Significa | Ejemplo en el LPC1769 |
|---|---|---|---|
| `__I`  | `volatile const` | **I**nput: solo lectura | `U0LSR`, el estado de la UART |
| `__O`  | `volatile` | **O**utput: solo escritura | registros de solo-escritura |
| `__IO` | `volatile` | **I**nput/**O**utput: lectura y escritura | `FIO0DIR`, `FIO0PIN` |

```c
typedef struct {
    __IO uint32_t DIR;      // volatile uint32_t       -> se lee y se escribe
         uint32_t RESERVED0[3];
    __IO uint32_t MASK;
    __IO uint32_t PIN;
    __IO uint32_t SET;
    __O  uint32_t CLR;      // volatile uint32_t       -> solo escritura
} LPC_GPIO_TypeDef;
```

Fijate que `__O` y `__IO` se expanden a lo mismo (`volatile` pelado): la diferencia entre "solo
escritura" y "lectura y escritura" **no la puede hacer cumplir el compilador**, así que ahí la macro
es pura documentación. La única que sí tiene efecto real es `__I`, porque el `const` hace que
escribirlo no compile.

Cómo se construye un header así, campo por campo, es el capítulo siguiente
([13 - Structs para hardware](./13-structs-para-hardware.md)) y el módulo
[02 - Armá tu propia librería](../02_arma_tu_propia_libreria/).

---

## 4. Resumen

| Concepto | Para qué | Palabra clave |
|----------|----------|---------------|
| Ponerle nombre a un tipo | Documentar intención; poder cambiarlo en un solo lugar | `typedef` (y el sufijo `_t`) |
| Una dirección como entero | Convertir entre dirección y puntero de forma portable | `uintptr_t` |
| Contadores y locales | Evitar el recorte a 8/16 bits en un núcleo de 32 | `uint32_t` o `uint_fast8_t` |
| Valor que cambia solo | Registros de hardware, variables compartidas con ISR | `volatile` |
| Registro de solo lectura | Que el compilador te frene si intentás escribirlo | `const volatile` (`__I`) |
| Puntero que no se mueve | Que la dirección del registro no se pueda reasignar | `* const` |

Y las tres cosas que `volatile` **no** hace, que son las que causan los bugs:

| No hace | Qué necesitás en su lugar |
|---|---|
| No da atomicidad | Una sección crítica ([módulo 07](../07_interrupciones/03-secciones-criticas-y-atomicidad.md)) |
| No es una barrera de memoria | `DMB`/`DSB` ([capítulo 14](./14-static-const-inline-y-bitfields.md)) |
| No ordena los accesos **no** volátiles a su alrededor | Marcar volátil todo lo que el hardware toca |

### Lo que viene

Con esto ya tenés el C que necesitás para el paso siguiente: entender que **un registro de hardware
es, literalmente, una dirección de memoria con un `volatile uint32_t *` apuntándola**. El
[capítulo 13](./13-structs-para-hardware.md) agrupa esos punteros en una `struct` para mapear un
periférico entero de una vez, y el módulo
[01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/) lo lleva al micro.

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.7.3 (calificadores de tipo, incluida la definición exacta de qué es un "acceso volátil"), 5.1.2.3 (el modelo de ejecución y los *puntos de secuencia*, que es lo que `volatile` ordena y lo que **no** ordena), 6.7.8 (`typedef`), 7.20.1.4 (`uintptr_t`).
- [cppreference: volatile](https://en.cppreference.com/w/c/language/volatile). Qué garantiza y qué no.

**Sobre los temas puntuales**

- [Volatile: The Multithreaded Programmer's Best Friend (Andrei Alexandrescu)](https://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766) y [Who's afraid of a big bad optimizing compiler? (LWN)](https://lwn.net/Articles/793253/). Los dos textos de referencia sobre por qué `volatile` **no** da atomicidad ni orden entre variables distintas.
- [John Regehr: Volatiles are miscompiled, and what to do about it](https://blog.regehr.org/archives/28). El estudio que encontró que los compiladores reales se equivocaban con `volatile`, y de dónde viene la recomendación de encapsular los accesos.

**ARM y el LPC1769**

- [CMSIS-Core (Cortex-M)](https://arm-software.github.io/CMSIS_5/Core/html/index.html). La definición de `__I`, `__O` y `__IO` está en `core_cm3.h`, en el repo (`library/CMSISv2p00_LPC17xx/inc/`): son exactamente `volatile const`, `volatile` y `volatile`.
- [UM10360: LPC176x/5x User Manual](../../UM10360.pdf). En la tabla de cada periférico, la columna de acceso (`RO`, `WO`, `R/W`) es la que te dice cuál de los tres corresponde.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [11 - Asignación dinámica](./11-asignacion-dinamica.md) ·
**Siguiente:** [13 - Structs para hardware](./13-structs-para-hardware.md)
