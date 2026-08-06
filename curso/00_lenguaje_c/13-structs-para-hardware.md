# Structs para hardware: punteros, padding, bitfields y uniones

Segunda mitad del capítulo de tipos compuestos. En
[05 - Estructuras y enumeraciones](./05-estructuras-y-enums.md) viste cómo declarar un `struct` y
acceder a sus campos con `.`, y en [09](./09-punteros-avanzado.md#punteros-y-estructuras) cómo llegar
a ellos por puntero con `->`. Acá vemos todo lo que necesita saber **dónde está cada byte**: cuánto
ocupa de verdad una estructura en memoria, cómo se fuerza un layout exacto y cómo se usa todo eso
para mapear los registros del LPC1769.

Es el capítulo que convierte a `struct` de "una comodidad para agrupar datos" en **la herramienta
principal para hablarle al hardware**.

---

## Lo que este capítulo da por sabido

Los punteros a estructura y el operador flecha `->` ya están en
[09 - Punteros avanzados](./09-punteros-avanzado.md#acceso-a-miembros-de-estructura-con-punteros).
El recordatorio de una línea, porque de acá en adelante se usa en cada ejemplo:

```c
Punto *ptr = &p1;
ptr->y = 30;      // equivale a (*ptr).y = 30
```

Con eso alcanza. Lo que sigue es lo que todavía no viste: cuánto ocupa de verdad una estructura en
memoria y cómo se fuerza su layout.

---

## Padding y alineación

El compilador puede agregar bytes de relleno (*padding*) para alinear los datos en memoria:

```c
struct Ejemplo {
    char a;    // 1 byte
    int b;     // 4 bytes
    char c;    // 1 byte
};

printf("Tamaño: %zu\n", sizeof(struct Ejemplo));
// Resultado típico: 12 bytes (no 6)
```

**¿Por qué 12 y no 6?**

* `char a`: 1 byte
* **3 bytes de padding** (para alinear `int` a dirección múltiplo de 4)
* `int b`: 4 bytes
* `char c`: 1 byte
* **3 bytes de padding** (para que la estructura entera sea múltiplo de 4)

Total: 1 + 3 + 4 + 1 + 3 = **12 bytes**

**¿Por qué el compilador hace esto?** Porque el procesador accede a la memoria de forma más eficiente cuando cada dato está en una dirección **múltiplo de su tamaño**: un `uint32_t` en una dirección múltiplo de 4, un `uint16_t` en una múltiplo de 2. A eso se le llama **alineación natural**. El Cortex-M3 lee la memoria de a palabras de 32 bits; un dato alineado se lee en un solo acceso al bus, uno desalineado puede requerir dos.

> **Truco práctico:** si ordenás los miembros **del más grande al más chico**, minimizás el padding. Reordenando el ejemplo anterior (`int b; char a; char c;`) el tamaño baja de 12 a 8 bytes, sin cambiar nada más.

```c
struct Mejor {
    int  b;    // 4 bytes
    char a;    // 1 byte
    char c;    // 1 byte
    // 2 bytes de padding final -> total 8 bytes
};
```

---

### Controlar el padding: `packed`

A veces necesitás que la estructura **no tenga padding**: típicamente cuando representa una **trama de un protocolo** o un bloque de bytes que vino de afuera (por UART, SPI, una EEPROM, etc.) y cada campo tiene que caer en una posición exacta.

Hay dos formas. La portable-ish con `#pragma`:

```c
#pragma pack(push, 1)  // sin padding
struct Compacto {
    char a;    // 1 byte
    int b;     // 4 bytes
    char c;    // 1 byte
};
#pragma pack(pop)
// sizeof(struct Compacto) == 6
```

Y la de GCC/Clang (la que vas a ver en el toolchain ARM del curso), con `__attribute__((packed))`:

```c
struct __attribute__((packed)) Trama {
    uint8_t  start;     // offset 0
    uint16_t longitud;  // offset 1  (¡desalineado!)
    uint32_t payload;   // offset 3  (¡desalineado!)
    uint8_t  checksum;  // offset 7
};
// sizeof == 8, exactamente, y cada campo en la posición que dicta el protocolo
```

> **El costo en Cortex-M3 (importante)**
>
> `packed` no es gratis. Cuando un campo queda en una dirección desalineada (como `longitud` y `payload` arriba), el compilador **no puede** usar una instrucción de carga normal de 32 bits, porque el hardware fallaría o leería mal. En su lugar genera **código byte a byte** para armar el valor: varias instrucciones en lugar de una. Es decir: una estructura `packed` ahorra RAM/espacio en la trama, pero **cada acceso a un campo desalineado es más lento**.
>
> Conclusión: usá `packed` **solo** cuando el layout exacto importa (protocolos, parsing de bytes crudos, datos persistidos). Para tus estructuras internas normales, dejá que el compilador alinee: es más rápido.

> **Conexión con los registros de hardware:** las estructuras que mapean periféricos (el patrón *struct overlay* que vemos unas secciones más abajo, en "Uso en sistemas embebidos", y que se desarrolla en el módulo [01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/)) **nunca** se declaran `packed`: los registros del LPC1769 son todos `uint32_t` consecutivos y ya están naturalmente alineados a 4. Ahí el padding "automático" es justamente lo que querés, porque coincide con el mapa de memoria del chip.

> **Para los curiosos (avanzado): `offsetof` y el acceso desalineado en detalle**
>
> La macro `offsetof(tipo, miembro)` (de `<stddef.h>`) te dice en qué **byte** dentro de la estructura empieza un miembro. Sirve para verificar layouts de protocolo en tiempo de compilación:
> ```c
> #include <stddef.h>
> _Static_assert(offsetof(struct Trama, payload) == 3, "layout cambiado!");
> ```
> Sobre el acceso desalineado: el Cortex-M3 **sí** tolera accesos desalineados para cargas/almacenamientos simples de palabra (`LDR`/`STR`), pero **no** para los de doble palabra ni los exclusivos, y el bit `UNALIGN_TRP` puede configurarse para que cualquier acceso desalineado dispare un *UsageFault*. Por eso lo seguro es no asumir nada: si tenés que leer un `uint32_t` de un buffer de bytes en offset arbitrario, usá `memcpy` (ver más abajo) en lugar de castear el puntero.

---

## Estructuras bit-field

Permiten especificar el ancho en bits de cada campo. Útil para registros de hardware:

```c
typedef struct {
    uint8_t enable : 1;    // 1 bit
    uint8_t mode : 2;      // 2 bits
    uint8_t speed : 3;     // 3 bits
    uint8_t reserved : 2;  // 2 bits
} ControlReg;

ControlReg ctrl = {0};
ctrl.enable = 1;
ctrl.mode = 2;     // valores 0-3
ctrl.speed = 5;    // valores 0-7
```

> Los bit-fields son cómodos y legibles para representar campos de bits con nombre.

> **PRECAUCIÓN: el orden de los bits no está garantizado**
>
> El estándar C **no** define si el primer campo del bit-field cae en el bit menos significativo o en el más significativo: depende del compilador y de la arquitectura. La mayoría de los compiladores en ARM ubican el primer campo en los bits bajos (como esperás), pero el estándar no obliga. Por eso, para **registros reales de hardware** muchos prefieren máscaras y desplazamientos (`reg |= (1 << 3)`) antes que bit-fields, que son portables a cualquier compilador. Tratamos esta disyuntiva en detalle en el módulo [14 - `static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md).

---

## Uso en sistemas embebidos: mapeo de registros

```c
// Registro de control de GPIO (ejemplo)
typedef struct {
    volatile uint32_t DIR;      // Dirección (entrada/salida)
    volatile uint32_t OUT;      // Valor de salida
    volatile uint32_t IN;       // Valor de entrada
    volatile uint32_t SET;      // Set bits
    volatile uint32_t CLR;      // Clear bits
} GPIO_Regs;

// Mapear a dirección de hardware
#define GPIO0 ((GPIO_Regs *)0x40020000)

// Usar
GPIO0->DIR |= (1 << 5);   // Pin 5 como salida
GPIO0->SET = (1 << 5);    // Pin 5 en alto
```

> Esto es mucho más legible que escribir `*(volatile uint32_t *)0x40020000 |= ...`

---
## Uniones (`union`)

Una **unión** permite almacenar diferentes tipos de datos en la **misma ubicación de memoria**. Solo un miembro puede tener un valor válido a la vez.

### Declaración

```c
union Dato {
    int entero;
    float flotante;
    char bytes[4];
};
```

---

### Tamaño de una unión

El tamaño de una unión es el del **miembro más grande**:

```c
union Dato {
    int entero;        // 4 bytes
    float flotante;    // 4 bytes
    char bytes[4];     // 4 bytes
};

printf("Tamaño: %zu\n", sizeof(union Dato));
// Resultado: 4 bytes
```

Todos los miembros **comparten la misma memoria**.

---

### Uso básico

```c
union Dato d;

d.entero = 0x12345678;
printf("Entero: 0x%X\n", d.entero);
printf("Bytes: %02X %02X %02X %02X\n",
       d.bytes[0], d.bytes[1], d.bytes[2], d.bytes[3]);

d.flotante = 3.14f;
printf("Float: %.2f\n", d.flotante);
// Ahora d.entero tiene un valor sin sentido
```

> **Ojo:** Solo el **último miembro escrito** tiene un valor válido.

---

### Casos de uso típicos

#### 1. Conversión de tipos (type punning)

Ver la representación en bytes de un float:

```c
union {
    float f;
    uint8_t bytes[4];
} converter;

converter.f = 3.14159f;
printf("Bytes: ");
for (int i = 0; i < 4; i++) {
    printf("%02X ", converter.bytes[i]);
}
```

> **Para los curiosos (avanzado): aliasing y por qué `memcpy` es más seguro**
>
> Reinterpretar los bytes de un tipo como otro (*type punning*) tiene una trampa. El estándar C tiene una regla llamada **strict aliasing**: el compilador asume que punteros de tipos distintos **no** apuntan al mismo lugar, y optimiza en base a eso. Por eso castear un puntero y desreferenciarlo para reinterpretar tipos es **comportamiento indefinido**:
> ```c
> float f = 3.14f;
> uint32_t bits = *(uint32_t *)&f;   // MAL: viola strict aliasing
> ```
> Hacerlo con una **unión** (como arriba) está permitido en C (no en C++). Pero la forma **siempre segura y portable** es `memcpy`, que copia bytes crudos sin violar ninguna regla de aliasing y, además, el compilador la suele optimizar a cero costo:
> ```c
> #include <string.h>
> float f = 3.14f;
> uint32_t bits;
> memcpy(&bits, &f, sizeof bits);    // BIEN: reinterpreta sin UB
> ```
> Regla práctica para embebido: para reinterpretar tipos o leer un valor de un buffer de bytes en offset arbitrario, usá `memcpy`. Es más seguro que el cast y no es más lento.

---

#### 2. Protocolos de comunicación

Acceder a un paquete como estructura o como bytes:

```c
typedef union {
    struct {
        uint8_t id;
        uint8_t comando;
        uint16_t valor;
        uint32_t timestamp;
    } fields;
    uint8_t raw[8];
} Paquete;

Paquete pkt;
pkt.fields.id = 1;
pkt.fields.comando = 0xA5;
pkt.fields.valor = 1234;
pkt.fields.timestamp = 5000;

// Enviar por UART
UART_SendBytes(pkt.raw, 8);
```

---

#### 3. Ahorrar memoria con tipos mutuamente excluyentes

```c
typedef struct {
    enum { TIPO_INT, TIPO_FLOAT, TIPO_STRING } tipo;
    union {
        int valor_int;
        float valor_float;
        char valor_string[20];
    } dato;
} Variable;

Variable v;
v.tipo = TIPO_FLOAT;
v.dato.valor_float = 3.14;

if (v.tipo == TIPO_FLOAT) {
    printf("Float: %.2f\n", v.dato.valor_float);
}
```

> Esto se llama **tagged union** (unión etiquetada): usas un campo separado para recordar qué miembro es válido.

---

### Diferencia entre `struct` y `union`

| Aspecto | `struct` | `union` |
|---------|----------|---------|
| **Memoria** | Suma de todos los miembros | Tamaño del miembro más grande |
| **Miembros activos** | Todos simultáneamente | Solo uno a la vez |
| **Uso** | Agrupar datos relacionados | Datos mutuamente excluyentes |

---

### Estructuras y uniones anónimas (C11)

A partir de C11 (que soporta el toolchain ARM del curso), una `struct` o `union` **sin nombre** dentro de otra hace que sus miembros se accedan **directamente**, sin tener que nombrar el campo intermedio. Es muy cómodo para overlays de registros.

Comparemos. Con unión **con nombre** (`fields`/`raw`), hay que escribir el intermediario:

```c
pkt.fields.id = 1;     // tenés que poner ".fields"
```

Con unión **anónima**, no:

```c
typedef struct {
    uint32_t status;
    union {                 // unión anónima: sin nombre
        uint32_t entero;
        uint8_t  bytes[4];
    };                      // sus miembros suben al nivel del struct
} Registro;

Registro r;
r.entero = 0x12345678;     // se accede directo, sin ".algo.entero"
r.bytes[0] = 0xFF;
```

> Esto aparece seguido en headers de fabricante (CMSIS), donde un registro se puede ver como palabra completa o como sus bits, sin un nombre intermedio que estorbe.

---
## Combinando estructuras, uniones y enums

Un ejemplo realista en sistemas embebidos:

```c
typedef enum {
    SENSOR_TEMP,
    SENSOR_HUM,
    SENSOR_PRES
} SensorType;

typedef struct {
    SensorType tipo;
    uint32_t timestamp;
    union {
        int16_t temperatura;   // en décimas de °C
        uint8_t humedad;       // 0-100%
        uint32_t presion;      // en Pa
    } valor;
    uint8_t error_code;
} SensorReading;

void procesar_lectura(SensorReading *reading) {
    printf("Timestamp: %lu, ", reading->timestamp);

    switch (reading->tipo) {
        case SENSOR_TEMP:
            printf("Temp: %.1f°C\n", reading->valor.temperatura / 10.0);
            break;
        case SENSOR_HUM:
            printf("Hum: %d%%\n", reading->valor.humedad);
            break;
        case SENSOR_PRES:
            printf("Pres: %lu Pa\n", reading->valor.presion);
            break;
    }
}
```

---

## Resumen

| Tipo | Propósito | Ejemplo de uso |
|------|-----------|----------------|
| **`struct`** | Agrupar datos relacionados | Registros de hardware, datos de sensores |
| **`union`** | Múltiples interpretaciones de misma memoria | Protocolos, conversión de tipos |
| **`enum`** | Constantes simbólicas | Estados, comandos, configuraciones |

**Buenas prácticas:**

* Usa `typedef` para simplificar declaraciones
* Usa `enum` en lugar de `#define` para grupos de constantes relacionadas
* Usa `struct` para mapear registros de hardware
* Usa `union` con cuidado y siempre con un campo "tipo" (tagged union)
* Ten en cuenta el padding y alineación en estructuras
* Documenta el layout esperado de estructuras que se comunican con hardware

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.7.2.1 (estructuras, uniones y **campos de bits**, incluido que el orden de asignación de los bits es definido por la implementación), 6.5 ¶7 (la regla de *strict aliasing*), 7.19 (`offsetof`).
- [cppreference: struct](https://en.cppreference.com/w/c/language/struct) y [cppreference: union](https://en.cppreference.com/w/c/language/union). Incluye que en C (a diferencia de C++) leer un miembro distinto del último escrito en una unión **sí** está permitido.

**GCC y el toolchain**

- [GCC: Type Attributes](https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html). `packed` y `aligned`, con la advertencia sobre el costo de los accesos desalineados.
- [GCC: Structure-Packing Pragmas](https://gcc.gnu.org/onlinedocs/gcc/Structure-Layout-Pragmas.html). `#pragma pack`, la forma que también entienden IAR y Keil.
- El tamaño real de cualquier struct de este capítulo se comprueba sin placa, con `_Static_assert(sizeof(...) == N, "...")` y compilando: si el número no da, el compilador frena.

**ARM y el LPC1769**

- [Procedure Call Standard for the Arm Architecture (AAPCS)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst). Las reglas de alineación natural de cada tipo, de donde sale el padding del ejemplo.
- [ARMv7-M Architecture Reference Manual](https://developer.arm.com/documentation/ddi0403/latest/). El bit `UNALIGN_TRP` de `CCR` y qué accesos desalineados tolera de verdad el Cortex-M3.
- [UM10360: LPC176x/5x User Manual](../../UM10360.pdf). El mapa de registros de cualquier periférico es el que se copia campo por campo en el patrón *struct overlay*; el desarrollo completo está en el módulo [01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/).

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [12 - `volatile`, `const` y tipos propios](./12-volatile-y-tipos-para-hardware.md) ·
**Siguiente:** [14 - `static`, `inline` y campos de bits](./14-static-const-inline-y-bitfields.md)
