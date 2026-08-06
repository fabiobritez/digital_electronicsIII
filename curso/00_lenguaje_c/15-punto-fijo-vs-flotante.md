# Punto fijo vs punto flotante (el Cortex-M3 no tiene FPU)

Este capítulo te ahorra un problema de rendimiento que muerde a casi todos al principio: **usar
`float` sin saber lo que cuesta** en el LPC1769.

## El dato clave: el Cortex-M3 no tiene FPU

Una **FPU** (Floating-Point Unit) es hardware dedicado a hacer cuentas con decimales rápido. El
Cortex-M3 del LPC1769 **no la tiene** (sí la tienen el Cortex-M4F y superiores). ¿Qué pasa entonces
cuando escribís `float`?

```c
float v = (cuentas / 4095.0f) * 3.3f;   // se ve inocente...
```

Como no hay hardware, **cada operación con `float` la emula el compilador** llamando a funciones de
software (de la libc, anexo B). Una multiplicación de `float` que en una PC es una instrucción, en
el Cortex-M3 son **decenas o cientos de instrucciones**. No es que no funcione (funciona perfecto),
pero es **lento** y **ocupa más Flash** (arrastra esas rutinas de emulación).

> En el curso usamos `float` en algunos ejemplos (ADC→tensión, temperatura) **por claridad**. Está
> bien para algo que se calcula de vez en cuando. El problema es usarlo en un **lazo rápido** o en una
> **interrupción**, donde la lentitud se nota o rompe los tiempos.

## Cuándo `float` está bien y cuándo no

| `float` está bien | Evitá `float` |
|-------------------|----------------|
| una cuenta ocasional (mostrar una temperatura por UART) | dentro de una ISR (lento, rompe el *timing*) |
| código que no es crítico en tiempo | en lazos que corren miles de veces por segundo |
| prototipos, para que se entienda | cuando importa el tamaño del binario o el consumo |

## La alternativa: aritmética de punto fijo

La idea de **punto fijo** es trabajar con **enteros**, pero acordándote mentalmente de dónde va la
coma. En vez de "3.3 voltios", guardás "3300 milivoltios" (un `uint32_t`). Las operaciones son enteras
(rápidas, una instrucción), y la "coma" la manejás vos con la escala.

### Ejemplo: ADC a milivoltios sin `float`

```c
// En vez de:  float v = (cuentas / 4095.0f) * 3.3f;   // lento (float)
// Hacelo en milivoltios, con enteros:
uint32_t mv = (cuentas * 3300u) / 4095u;   // 0..3300 mV, todo entero -> rápido
```

`mv = 1650` significa 1.650 V. Multiplicás primero (por 3300) y dividís después, para no perder
precisión. El resultado es un entero que **representa** un valor con decimales, con la escala (mV) en
tu cabeza.

### Escala general (formato Q)

El patrón se generaliza eligiendo un **factor de escala**. Por ejemplo, para guardar valores con 2
decimales, multiplicás todo por 100:

```c
int32_t temp_centi = 2537;   // representa 25.37 °C  (escala x100)
// sumar dos temperaturas: enteros, exacto y rápido
int32_t suma = temp_centi + otra_centi;
// mostrarla: parte entera y decimal con enteros
printf("%ld.%02ld C\r\n", temp_centi / 100, temp_centi % 100);   // 25.37 C
```

Esto se llama formato **Q** (punto fijo). La regla: **elegís la escala según los decimales que
necesitás**, hacés todo con enteros, y solo "ponés la coma" al mostrar.

### La notación Q formal (Q15, Q16.16)

El ejemplo de arriba usa escala decimal (x100). En procesamiento de señales es más común una escala
en **potencias de 2**, porque escalar y desescalar son entonces simples **corrimientos** (`<<`, `>>`),
que el CPU hace en una instrucción. La notación es **Q*m*.*n***: *m* bits para la parte entera, *n*
bits para la parte fraccionaria. El valor real se obtiene dividiendo el entero por 2ⁿ.

Dos formatos clásicos:

- **Q15** (a veces escrito Q0.15): un `int16_t` donde el bit alto es signo y los 15 restantes son
  fracción. Representa valores en **[-1, 1)** con paso 1/32768. Es el formato de audio y filtros.
- **Q16.16**: un `int32_t` partido en 16 bits enteros y 16 fraccionarios. Rango ±32768 con paso
  1/65536. Cómodo cuando necesitás algo de parte entera además de decimales.

```c
typedef int16_t q15_t;     // valor real = entero / 32768   (rango [-1, 1))
typedef int32_t q16_16_t;  // valor real = entero / 65536

// convertir desde/hacia el valor real (solo al borde del sistema; adentro trabajás con el entero)
#define FLOAT_A_Q15(x)   ((q15_t)((x) * 32768.0f))
#define Q15_A_FLOAT(q)   ((float)(q) / 32768.0f)
```

### Suma, resta y multiplicación con reescalado

La clave del punto fijo es saber **qué pasa con la escala** en cada operación:

- **Suma y resta**: si los dos operandos tienen la **misma** escala, se suman directamente y el
  resultado conserva la escala. `Q15 + Q15 = Q15`. Trivial.
- **Multiplicación**: acá está el truco. Multiplicar dos Q*n* da un resultado en Q*2n* (las escalas
  se multiplican: 2ⁿ × 2ⁿ = 2²ⁿ). Hay que **reescalar** corriendo *n* bits a la derecha para volver
  a Q*n*. Y hay que hacer el producto en un tipo **más ancho** para no desbordar:

```c
// Q15 * Q15 -> Q15.  El producto de dos int16 puede no entrar en 16 bits: usar int32 intermedio.
static inline q15_t q15_mul(q15_t a, q15_t b) {
    return (q15_t)(((int32_t)a * (int32_t)b) >> 15);   // amplío a 32 bits, multiplico, reescalo
}

// Q16.16 * Q16.16 -> Q16.16.  Producto en int64 para no perder bits.
static inline q16_16_t q16_mul(q16_16_t a, q16_16_t b) {
    return (q16_16_t)(((int64_t)a * (int64_t)b) >> 16);
}
```

> Regla mnemotécnica: **al multiplicar, ensanchá el tipo, multiplicá, y corré a la derecha tantos
> bits como tenga la parte fraccionaria.** Al dividir es al revés (corrés a la izquierda el numerador
> antes de dividir).

### Redondeo

El `>> 15` de arriba **trunca** (descarta los bits de abajo, siempre hacia "menos infinito"). Si
querés **redondear** al entero más cercano, sumá medio LSB antes de correr:

```c
static inline q15_t q15_mul_redondeado(q15_t a, q15_t b) {
    int32_t p = (int32_t)a * (int32_t)b;
    return (q15_t)((p + (1 << 14)) >> 15);   // + 0.5 LSB (2^14) antes de truncar
}
```

### Saturación

Al sumar dos valores grandes podés **desbordar** el tipo: en `int16_t`, 30000 + 30000 da la vuelta a
un negativo (*wrap-around*), que en una señal suena como un chasquido horrible. La **saturación**
*recorta* al máximo/mínimo en vez de dar la vuelta, mucho más benigno:

```c
static inline q15_t q15_sat_add(q15_t a, q15_t b) {
    int32_t s = (int32_t)a + (int32_t)b;     // sumo en 32 bits (no desborda)
    if (s >  32767) s =  32767;              // recorto al máximo de int16
    if (s < -32768) s = -32768;              // recorto al mínimo
    return (q15_t)s;
}
```

> **Para los curiosos (avanzado):** el Cortex-M3 tiene instrucciones `SSAT`/`USAT` que saturan en una
> sola instrucción, y CMSIS-DSP trae tipos `q7_t`/`q15_t`/`q31_t` y funciones (`arm_mult_q15`, etc.)
> con saturación y redondeo ya resueltos. Si algún día hacés DSP en serio, no reinventes la rueda:
> usás esa librería. Para esta materia, las cuatro operaciones de arriba alcanzan para entender el
> concepto.

### Punto fijo vs `float` emulado: cuándo conviene

Recordá que en el M3 **no hay FPU**: cada operación `float` es una llamada a software de decenas de
ciclos. El punto fijo, en cambio, es aritmética entera que el CPU hace en 1-2 ciclos (el Cortex-M3
tiene multiplicador por hardware de 32 bits, e incluso multiplicación de 64 bits en pocos ciclos).
La diferencia en un lazo rápido es de **un orden de magnitud o más**.

| Preferí punto fijo | `float` emulado está bien |
|--------------------|----------------------------|
| filtros, control y DSP en lazos/ISR rápidos | una cuenta ocasional fuera de tiempo real |
| cuando el rango y la precisión son conocidos y acotados | cuando el rango dinámico es enorme o impredecible |
| cuando importa el tamaño del binario | prototipo donde prioriza la claridad |

El costo del punto fijo es **tu** trabajo mental (llevar la cuenta de las escalas y los overflows).
Si ese costo no se justifica porque la operación es ocasional, usá `float` sin culpa. Si está en el
camino caliente, el punto fijo vuela.

## Reglas prácticas

- **Para mostrar un valor** (UART, display) cada tanto: `float` está bien, no te compliques.
- **Para cuentas en lazos rápidos o ISRs**: usá **enteros escalados** (punto fijo).
- **Multiplicá antes de dividir** (con enteros) para no perder precisión: `(x * 1000) / y`, no
  `(x / y) * 1000`.
- **Cuidado con el overflow**: si escalás mucho un `uint32_t`, podés pasarte de 4.294.967.295. Si hace
  falta, usá `uint64_t` para el paso intermedio.
- Si **de verdad** necesitás mucha matemática con decimales y velocidad, ese es un caso para elegir un
  micro **con FPU** (Cortex-M4F), pero para esta materia, punto fijo alcanza y sobra.

## Por qué importa saberlo

El síntoma típico: un alumno hace un filtro o un control con `float` dentro de una interrupción de
timer, y "el sistema se pone lento" o "pierde tiempos" sin entender por qué. La causa es la emulación
de `float`. Sabiendo esto, lo reescribís con enteros y vuela.

## Ejercicios
1. Reescribí el cálculo de tensión del ADC (módulo 10) en **milivoltios con enteros** y compará el
   tamaño del binario (`size`) con la versión `float`.
2. Implementá un promedio de 16 muestras de ADC en punto fijo (sumá enteros, dividí al final).
3. Investigá cuántas instrucciones genera una multiplicación `float` vs una `int` con
   `arm-none-eabi-objdump -d` (anexo B).

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes: 5.2.4.2.2 (características de los tipos de punto flotante), 6.3.1.5 (conversiones entre flotantes), Anexo F (IEEE-754).
- [IEEE 754-2019](https://standards.ieee.org/ieee/754/6210/). El formato de `float` y `double`, de donde sale que un `float` tiene 24 bits de mantisa.
- [What Every Computer Scientist Should Know About Floating-Point Arithmetic (Goldberg)](https://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html). El texto clásico sobre por qué `0.1` no es `0.1`.

**GCC y el toolchain**

- [GCC: Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html). `-mfloat-abi=soft` (lo que usa el LPC1769) contra `-mfloat-abi=hard`, y qué implica cada uno.
- Cuánto cuesta de verdad una cuenta con `float` en esta placa se mide sin adivinar:
  ```console
  $ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O2 -S cuenta.c -o -
  ```
  y buscás las llamadas a `__aeabi_fmul`, `__aeabi_fadd` y compañía: cada una es una rutina de software de `libgcc`.

**ARM y el LPC1769**

- [Arm: Floating-point support in Cortex-M](https://developer.arm.com/documentation/dai0298/latest/). Confirma que el Cortex-M3 no tiene FPU y que el M4F sí.
- [UM10360: LPC176x/5x User Manual](../../UM10360.pdf), Capítulo 29 (ADC). Las cuentas de este capítulo (cuentas del ADC a milivoltios) salen de ahí.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [14 - `static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md) ·
**Siguiente:** [16 - Redirigir `printf` a la UART](./16-redirigir-printf-a-uart.md)
