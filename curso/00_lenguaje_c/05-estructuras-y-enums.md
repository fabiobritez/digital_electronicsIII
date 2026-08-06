# Estructuras y enumeraciones

Los tipos primitivos (`int`, `char`, `float`, etc.) son útiles, pero limitados. Para representar datos
más complejos, C ofrece **tipos compuestos**: los que agrupan varios datos bajo un solo nombre.

Ya viste uno en [02 - Arreglos](./02-arreglos-conversiones-y-promociones.md): el arreglo junta varios
datos **del mismo tipo**. Este capítulo agrega los dos que faltan:

* **Estructuras (`struct`)**: agrupan variables de **distintos** tipos bajo un solo nombre.
* **Enumeraciones (`enum`)**: definen constantes enteras con nombre.

Los dos se pueden entender sin saber nada de punteros, y por eso están acá. Lo que sí necesita
punteros viene después: el operador `->` (llegar a los campos de un struct a través de un puntero) en
[09 - Punteros avanzados](./09-punteros-avanzado.md#acceso-a-miembros-de-estructura-con-punteros), y
el *padding*, los *bitfields*, las uniones y el mapeo de registros del micro en
[13 - Structs para hardware](./13-structs-para-hardware.md).

En sistemas embebidos estos tipos son la base para modelar registros de hardware, organizar datos de
sensores, implementar protocolos y escribir máquinas de estado legibles.

---

## Estructuras (`struct`)

Una **estructura** permite agrupar variables de distintos tipos bajo un solo nombre. Es como crear un nuevo tipo de dato personalizado.

### Declaración de estructuras

#### Sintaxis básica

```c
struct NombreStruct {
    tipo1 miembro1;
    tipo2 miembro2;
    tipo3 miembro3;
    // ...
};
```

#### Ejemplo simple

```c
struct Punto {
    int x;
    int y;
};
```

Esto **declara el tipo** `struct Punto`, pero **no crea ninguna variable** todavía.

---

### Creación de variables de tipo estructura

```c
// Declarar variable
struct Punto p1;

// Declarar e inicializar
struct Punto p2 = {10, 20};  // x=10, y=20

// Inicialización designada (C99+)
struct Punto p3 = {.y = 5, .x = 3};  // orden no importa
```

---

### Acceso a miembros

Se usa el operador `.` (punto):

```c
struct Punto p1;
p1.x = 100;
p1.y = 200;

printf("Punto: (%d, %d)\n", p1.x, p1.y);
```

---

### Ejemplo más completo: datos de sensor

```c
struct SensorData {
    uint16_t temperatura;  // en décimas de grado
    uint16_t humedad;      // en porcentaje
    uint32_t timestamp;    // tiempo en ms
    uint8_t status;        // flags de estado
};

struct SensorData lectura = {
    .temperatura = 235,  // 23.5°C
    .humedad = 65,       // 65%
    .timestamp = 1000,
    .status = 0x01
};

printf("Temp: %.1f°C\n", lectura.temperatura / 10.0);
```

---

### Declaración con `typedef`

Para evitar escribir `struct` cada vez, se puede usar `typedef`:

```c
typedef struct {
    int x;
    int y;
} Punto;

// Ahora se usa directamente:
Punto p1 = {5, 10};
Punto p2;
p2.x = 15;
```

También se puede dar nombre a la estructura:

```c
typedef struct Punto {
    int x;
    int y;
} Punto;

// Ahora se puede usar tanto "struct Punto" como "Punto"
```

> En sistemas embebidos es común usar `typedef` para simplificar el código.

---

### Estructuras anidadas

Una estructura puede contener otras estructuras:

```c
typedef struct {
    int dia;
    int mes;
    int año;
} Fecha;

typedef struct {
    char nombre[50];
    int edad;
    Fecha nacimiento;  // estructura anidada
} Persona;

Persona p = {
    .nombre = "Juan",
    .edad = 30,
    .nacimiento = {15, 5, 1993}
};

printf("Nació en %d/%d/%d\n",
       p.nacimiento.dia,
       p.nacimiento.mes,
       p.nacimiento.año);
```

---

### Arreglos de estructuras

```c
typedef struct {
    char nombre[20];
    uint16_t valor;
} Sensor;

Sensor sensores[3] = {
    {"Temp", 250},
    {"Hum", 60},
    {"Luz", 800}
};

for (int i = 0; i < 3; i++) {
    printf("%s: %d\n", sensores[i].nombre, sensores[i].valor);
}
```

> **Para los curiosos (avanzado): flexible array members**
>
> Desde C99, el **último** miembro de una estructura puede ser un arreglo **sin tamaño**. Sirve para una cabecera seguida de un payload de longitud variable: muy útil para tramas de comunicación.
> ```c
> typedef struct {
>     uint8_t  id;
>     uint8_t  len;
>     uint8_t  datos[];   // flexible array member: ocupa 0 bytes en sizeof
> } Mensaje;
> // sizeof(Mensaje) == 2 (no cuenta 'datos')
> ```
> El arreglo `datos` no reserva espacio propio: apunta a lo que venga **inmediatamente después** de la cabecera en memoria. Es la forma idiomática de "castear" un buffer recibido a un mensaje con payload variable, sin copiar. En embebido se usa para parsear paquetes directamente sobre el buffer de recepción.

---
---

## Enumeraciones (`enum`)

Ya vimos `enum` brevemente en la sección de declaraciones. Aquí profundizamos.

### ¿Qué es un `enum`?

Define un conjunto de **constantes enteras con nombre**. Hace el código más legible que usar números mágicos.

### Sintaxis

```c
enum DiaSemana {
    LUNES,      // 0
    MARTES,     // 1
    MIERCOLES,  // 2
    JUEVES,     // 3
    VIERNES,    // 4
    SABADO,     // 5
    DOMINGO     // 6
};
```

Por defecto, los valores empiezan en 0 y aumentan de a 1.

---

### Asignar valores explícitos

```c
enum Estado {
    IDLE = 0,
    RUNNING = 1,
    PAUSED = 2,
    ERROR = 255
};
```

Si no especificas un valor, continúa desde el anterior + 1:

```c
enum Prioridad {
    BAJA = 1,
    MEDIA,      // 2
    ALTA,       // 3
    CRITICA = 10
};
```

---

### Uso con `typedef`

```c
typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK
} LED_State;

LED_State estado = LED_OFF;

if (estado == LED_ON) {
    // ...
}
```

---

### Ventajas sobre `#define`

#### Con `#define`:

```c
#define LED_OFF 0
#define LED_ON 1
#define LED_BLINK 2
```

#### Con `enum`:

```c
enum LED_State { LED_OFF, LED_ON, LED_BLINK };
```

**Ventajas del `enum`:**

1. **Tipado**: el compilador sabe que es un grupo relacionado
2. **Depuración**: los debuggers pueden mostrar el nombre en lugar del número
3. **Alcance**: los nombres están en un namespace (en C++)
4. **Mantenimiento**: más fácil agregar/quitar valores

---

### Uso en sistemas embebidos

#### Máquina de estados

```c
typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_ACTIVE,
    STATE_SLEEP,
    STATE_ERROR
} SystemState;

SystemState estado_actual = STATE_INIT;

switch (estado_actual) {
    case STATE_INIT:
        // inicializar
        break;
    case STATE_IDLE:
        // esperar
        break;
    // ...
}
```

---

#### Flags de configuración

```c
typedef enum {
    UART_8BIT = 0,
    UART_9BIT = 1
} UART_DataBits;

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD = 2
} UART_Parity;

void UART_Config(UART_DataBits bits, UART_Parity parity) {
    // ...
}

// Uso claro
UART_Config(UART_8BIT, UART_PARITY_NONE);
```

Mucho mejor que:

```c
UART_Config(0, 0);  // ¿qué significa esto?
```

---

#### Comandos de protocolo

```c
typedef enum {
    CMD_READ = 0x01,
    CMD_WRITE = 0x02,
    CMD_ERASE = 0x03,
    CMD_STATUS = 0x04
} FlashCommand;

void ejecutar_comando(FlashCommand cmd) {
    switch (cmd) {
        case CMD_READ:
            flash_read();
            break;
        case CMD_WRITE:
            flash_write();
            break;
        // ...
    }
}
```

---

### Enumeraciones con valores de bits (flags)

Para combinar múltiples opciones con OR:

```c
typedef enum {
    FLAG_NONE = 0,
    FLAG_ENABLE = (1 << 0),   // 0x01
    FLAG_READY = (1 << 1),    // 0x02
    FLAG_ERROR = (1 << 2),    // 0x04
    FLAG_BUSY = (1 << 3)      // 0x08
} StatusFlags;

uint8_t status = FLAG_ENABLE | FLAG_READY;

if (status & FLAG_READY) {
    // está listo
}
```

> Aunque en este caso muchos prefieren usar `#define` porque no hay validación de tipos en C para flags combinados.

---

### El tipo subyacente de un `enum`

Un `enum` es, por debajo, un **tipo entero**. El compilador elige cuál (el "tipo subyacente"): tiene que ser uno capaz de representar todos los valores del enum, y **cuál elige es definido por la implementación**.

> [!IMPORTANT]
> **En el LPC1769 un `enum` NO ocupa 4 bytes.** El ABI de ARM manda usar el tipo más chico que alcance, y `arm-none-eabi-gcc` viene con `-fshort-enums` **activado por defecto**:
>
> ```c
> typedef enum { ROJO, VERDE, AZUL } Color;   // valores 0,1,2
> // sizeof(Color) == 1 en el LPC1769   (¡un byte!)
> // sizeof(Color) == 4 en tu PC (x86)
> ```
>
> El mismo código, dos tamaños. Dos consecuencias prácticas:
>
> 1. **Nunca asumas el tamaño de un `enum`** en un `struct` que mapea un registro, una trama de protocolo o algo que se guarde en memoria. Si necesitás un ancho exacto, poné `uint8_t`/`uint32_t` en el campo y usá las constantes del `enum` aparte.
> 2. Si linkeás una librería precompilada con la opción contraria, los tamaños no coinciden y tenés corrupción silenciosa. El linker de GNU suele avisar con un warning sobre `Tag_ABI_enum_size`.
>
> Esto ya lo vimos en [01 - Declaraciones y tipos](./01-declaraciones-y-tipos.md#constantes-de-enumeración-enum), donde está la comprobación con el toolchain del repo.

> Ojo con no confundir dos cosas: **cada constante** del enum (`ROJO`, `VERDE`) tiene tipo `int` en C17 y anteriores, así que `sizeof(ROJO)` da 4. Lo que mide 1 byte es **el tipo enumerado** (`Color`). En **C23** el tipo subyacente se puede fijar a mano y el problema desaparece: `enum Color : uint8_t { ROJO, VERDE, AZUL };`

---

### Limitaciones de `enum` en C

* Los valores **deben ser enteros** (no float, no strings)
* No hay verificación fuerte de tipos: podés asignar cualquier `int` a un `enum` (el compilador no te frena si pasás un valor fuera del rango definido)
* El tamaño es el de su tipo subyacente, que **depende del compilador**: 1 o 2 bytes en el LPC1769, 4 en x86

---

## Resumen de reglas para no equivocarse

| Regla | Por qué |
| ----- | ------- |
| Usá `typedef` para no repetir `struct` en cada declaración | Es la convención de CMSIS y de todo el código del curso |
| Inicializá con nombres de campo (`.temperatura = 235`), no por posición | Si mañana cambia el orden de los miembros, tu código sigue estando bien |
| `enum` en vez de `#define` para grupos de constantes relacionadas | El debugger te muestra el nombre, y `-Wswitch` avisa si te olvidás un caso |
| Nunca asumas el `sizeof` de un `enum` | En ARM vale 1 o 2 bytes (`-fshort-enums`), en tu PC 4 |
| En un `struct` que va a hardware o a un protocolo, poné `uint8_t`/`uint32_t`, no un `enum` | El ancho tiene que ser exacto; usá las constantes del `enum` aparte |
| Un `enum` no valida nada en ejecución | Podés asignarle cualquier `int`: por eso todo `switch` sobre un estado lleva `default` |
| Un `struct` se copia entero al pasarlo o devolverlo por valor | Para estructuras grandes, pasá un puntero (capítulo [09](./09-punteros-avanzado.md#punteros-y-estructuras)) |

---

## Fuentes y para seguir leyendo

**Normativas y de referencia**

- [ISO/IEC 9899 (borrador público de C17, N2176)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2176.pdf). El estándar. Cláusulas relevantes para este capítulo: 6.2.5 (tipos), 6.7.2.1 (estructuras y uniones), 6.7.2.2 (enumeraciones), 6.7.9 (inicializadores, incluidos los designados).
- [cppreference: struct](https://en.cppreference.com/w/c/language/struct) y [cppreference: enum](https://en.cppreference.com/w/c/language/enum). Con los cambios por versión del estándar, incluido el tipo subyacente explícito de C23.

**GCC y el toolchain**

- [GCC: Implementation-defined behavior](https://gcc.gnu.org/onlinedocs/gcc/C-Implementation.html). Qué tipo entero elige GCC como tipo subyacente de un `enum`.
- El tamaño real de un `enum` en esta placa se comprueba con el toolchain del repo:
  ```console
  $ arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -dM -E - < /dev/null | grep -i short_enums
  ```

**ARM y el LPC1769**

- [Procedure Call Standard for the Arm Architecture (AAPCS)](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst). Define el tamaño de los `enum` en ARM de 32 bits y cómo se pasan las estructuras por valor.

---

**Módulo:** [Lenguaje C](./README.md) ·
**Anterior:** [04 - Control de flujo](./04-control-de-flujo.md) ·
**Siguiente:** [06 - Funciones](./06-funciones.md)
