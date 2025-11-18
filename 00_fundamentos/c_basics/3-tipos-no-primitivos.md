# Tipos de Datos No Primitivos en C

Los tipos primitivos (`int`, `char`, `float`, etc.) son útiles, pero limitados. Para representar datos más complejos, C ofrece **tipos derivados o compuestos**:

* **Estructuras (`struct`)**: agrupan variables de diferentes tipos
* **Uniones (`union`)**: almacenan diferentes tipos en el mismo espacio de memoria
* **Enumeraciones (`enum`)**: definen constantes enteras con nombre

Estos tipos son fundamentales en sistemas embebidos para:

* Modelar registros de hardware
* Organizar datos de sensores
* Implementar protocolos de comunicación
* Crear estructuras de datos complejas

---

# Estructuras (`struct`)

Una **estructura** permite agrupar variables de distintos tipos bajo un solo nombre. Es como crear un nuevo tipo de dato personalizado.

## Declaración de estructuras

### Sintaxis básica

```c
struct NombreStruct {
    tipo1 miembro1;
    tipo2 miembro2;
    tipo3 miembro3;
    // ...
};
```

### Ejemplo simple

```c
struct Punto {
    int x;
    int y;
};
```

Esto **declara el tipo** `struct Punto`, pero **no crea ninguna variable** todavía.

---

## Creación de variables de tipo estructura

```c
// Declarar variable
struct Punto p1;

// Declarar e inicializar
struct Punto p2 = {10, 20};  // x=10, y=20

// Inicialización designada (C99+)
struct Punto p3 = {.y = 5, .x = 3};  // orden no importa
```

---

## Acceso a miembros

Se usa el operador `.` (punto):

```c
struct Punto p1;
p1.x = 100;
p1.y = 200;

printf("Punto: (%d, %d)\n", p1.x, p1.y);
```

---

## Ejemplo más completo: datos de sensor

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

## Declaración con `typedef`

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

## Estructuras anidadas

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

## Arreglos de estructuras

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

---

## Punteros a estructuras

Como vimos en la sección de punteros, se usa el operador `->` para acceder a miembros:

```c
typedef struct {
    int x;
    int y;
} Punto;

Punto p1 = {10, 20};
Punto *ptr = &p1;

// Acceso mediante puntero
printf("x = %d\n", ptr->x);  // equivale a (*ptr).x
ptr->y = 30;
```

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

---

### Controlar el padding con `#pragma pack`

Para sistemas embebidos donde necesitas controlar exactamente el layout:

```c
#pragma pack(push, 1)  // sin padding
struct Compacto {
    char a;    // 1 byte
    int b;     // 4 bytes
    char c;    // 1 byte
};
#pragma pack(pop)

printf("Tamaño: %zu\n", sizeof(struct Compacto));
// Resultado: 6 bytes
```

> ⚠️ **Cuidado**: eliminar padding puede causar accesos no alineados que ralentizan (o fallan) en algunas arquitecturas.

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

> Los bit-fields son perfectos para mapear registros de periféricos donde cada bit tiene un significado específico.

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

# Uniones (`union`)

Una **unión** permite almacenar diferentes tipos de datos en la **misma ubicación de memoria**. Solo un miembro puede tener un valor válido a la vez.

## Declaración

```c
union Dato {
    int entero;
    float flotante;
    char bytes[4];
};
```

---

## Tamaño de una unión

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

## Uso básico

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

> ⚠️ Solo el **último miembro escrito** tiene un valor válido.

---

## Casos de uso típicos

### 1. Conversión de tipos (type punning)

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

---

### 2. Protocolos de comunicación

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

### 3. Ahorrar memoria con tipos mutuamente excluyentes

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

## Diferencia entre `struct` y `union`

| Aspecto | `struct` | `union` |
|---------|----------|---------|
| **Memoria** | Suma de todos los miembros | Tamaño del miembro más grande |
| **Miembros activos** | Todos simultáneamente | Solo uno a la vez |
| **Uso** | Agrupar datos relacionados | Datos mutuamente excluyentes |

---

# Enumeraciones (`enum`)

Ya vimos `enum` brevemente en la sección de declaraciones. Aquí profundizamos.

## ¿Qué es un `enum`?

Define un conjunto de **constantes enteras con nombre**. Hace el código más legible que usar números mágicos.

## Sintaxis

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

## Asignar valores explícitos

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

## Uso con `typedef`

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

## Ventajas sobre `#define`

### Con `#define`:

```c
#define LED_OFF 0
#define LED_ON 1
#define LED_BLINK 2
```

### Con `enum`:

```c
enum LED_State { LED_OFF, LED_ON, LED_BLINK };
```

**Ventajas del `enum`:**

1. **Tipado**: el compilador sabe que es un grupo relacionado
2. **Depuración**: los debuggers pueden mostrar el nombre en lugar del número
3. **Alcance**: los nombres están en un namespace (en C++)
4. **Mantenimiento**: más fácil agregar/quitar valores

---

## Uso en sistemas embebidos

### Máquina de estados

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

### Flags de configuración

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

### Comandos de protocolo

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

## Enumeraciones con valores de bits (flags)

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

## Limitaciones de `enum` en C

* Los valores **deben ser enteros** (no float, no strings)
* No hay verificación fuerte de tipos: puedes asignar cualquier `int` a un `enum`
* El tamaño es el de un `int` (en la mayoría de compiladores)

---

# Combinando estructuras, uniones y enums

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

# Resumen

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
