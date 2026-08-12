# Imprimir para depurar

Una forma rápida de saber qué está haciendo el programa es agregar señales que puedas observar.
Acá vamos de la más básica a la que brinda más información.

## Nivel 0: el LED de depuración

Sin ningún periférico de comunicación, un LED ya te dice mucho:

```c
GPIO_SetValue(0, LED);     // "llegué hasta acá"
```

- Prendido fijo = el código pasó por ese punto.
- Parpadeo distinto según el caso = distinguir ramas (`if`/`else`).
- LED apagado = el código quizás no llegó a ese punto. Antes de concluirlo, comprobá también que
  el pin y el LED estén bien configurados y conectados.

Es burdo pero no necesita nada: ideal cuando todavía no configuraste la UART o cuando sospechás que
el problema está justo en la inicialización.

## Nivel 1: la UART como consola

Mandar texto por la UART ([módulo 9](../../curso/09_uart/)) y leerlo en la PC con un terminal
serial es una herramienta clásica de depuración. Permite observar el flujo del programa, valores de
variables y resultados de los periféricos:

```c
UART_Send(LPC_UART0, (uint8_t*)"Entrando a main\r\n", 17, BLOCKING);
```

Y si redirigís `printf` a la UART ([módulo 0, cap. 16](../../curso/00_lenguaje_c/16-redirigir-printf-a-uart.md)),
tenés mensajes con formato:

```c
printf("ADC = %d, estado = %d\r\n", valor_adc, estado);
```

## Nivel 2: el Debug Framework de NXP

Escribir `UART_Send(...)` con casts y largos a cada rato es incómodo. El repo incluye un **debug
framework** liviano (en `library/.../Drivers/{inc,src}/debug_frmwrk.{h,c}`) que estandariza esto con
macros. Inicializás la UART de debug con una sola llamada y después imprimís con macros cortas.

```c
#include "debug_frmwrk.h"

int main(void) {
    debug_frmwrk_init();          // configura UART0 a 115200 8N1 (pines P0.2/P0.3)

    _DBG("Sistema iniciado\r\n");  // imprimir un string
    _DBD32(12345);                 // imprimir un decimal de 32 bits
    _DBG("\r\n");
    _DBH32(0xABCD1234);            // imprimir un hexadecimal de 32 bits
    while (1) { }
}
```

Macros principales:

| Macro | Imprime |
|-------|---------|
| `_DBG(str)` | un string |
| `_DBG_(str)` | un string + salto de línea |
| `_DBC(ch)` | un carácter |
| `_DBD(n)` / `_DBD16(n)` / `_DBD32(n)` | un número decimal (8/16/32 bits) |
| `_DBH(n)` / `_DBH16(n)` / `_DBH32(n)` | un número hexadecimal |

Dos detalles del formato: los decimales salen con ancho fijo y ceros a la izquierda (`_DBD32(45)`
imprime `0000000045`) y los hexadecimales llevan el prefijo `0x`. También existe `_DG`, que espera
y devuelve un carácter recibido por la UART de debug (bloqueante), útil para menús simples.

Para elegir UART0 o UART1, se cambia `USED_UART_DEBUG_PORT` en `debug_frmwrk.h`. Es el mismo
framework que usan los ejemplos oficiales de NXP, así que reconocerlo te ayuda a leerlos.

### Qué mejora y qué no

El framework evita escribir a mano los casts, los largos y la conversión de números. No agrega una
cola ni usa interrupciones o DMA. `_DBG()` recorre el string y entrega cada carácter a
`UART_Send(..., BLOCKING)`, por lo que el programa espera a la UART igual que con un `printf` por
polling.

Esto se comprobó en la LPC1769 a 100 MHz con la misma salida de 48 bytes:

| Llamada | Tiempo dentro de la llamada |
|---|---:|
| `_DBG(texto)` | 4005 µs |
| `printf("%s", texto)` redirigido a la misma UART | 4014 µs |

La diferencia fue de 9,55 µs, cerca del 0,24 %. El framework puede ahorrar memoria frente a
`printf` y resulta práctico para mensajes simples, pero no reduce de manera significativa el tiempo
de CPU. A 115200 8N1, su caudal sostenido medido fue 11 513 B/s, prácticamente el límite físico de
11 520 B/s.

El [ejemplo reproducible](../../curso/ejemplos/uart/debug_framework/) también compara `_DBD32()` con
el formato decimal de `printf` y explica cómo leer los resultados mediante GDB.

### Una versión mejorada, sin tocar la original

También se implementó un
[Debug Framework mejorado](../../curso/ejemplos/uart/debug_framework_mejorado/). Conserva los alias
`_DBG`, `_DBD32` y `_DBH32`, pero permite elegir tres backends: envío bloqueante por bloques, cola
con interrupciones o cola con DMA. El baudrate puede ser 115200 o 921600.

Resultados para el mismo literal de 48 bytes:

| Configuración | Tiempo hasta recuperar el control | Caudal sostenido |
|---|---:|---:|
| Framework original, 115200 | 4005 µs | 11 513 B/s |
| Bloques, 921600 | 350 µs | 92 104 B/s |
| Interrupciones, 921600 | 12,15 µs | 92 104 B/s |
| DMA, 921600 | **5,84 µs** | **92 104 B/s** |

La conversión decimal también bajó cerca de un 30 %, pero la mejora decisiva fue dejar de esperar a
la UART. La cola no crea ancho de banda: si se llena, el framework descarta llamadas completas y lo
informa mediante `debug_mejorado_perdidos()`.

La [guía de uso y límites](../../curso/ejemplos/uart/debug_framework_mejorado/USO_Y_LIMITES.md)
muestra qué ocurre al imprimir dentro de un `while (1)`, cuánto stack usa, dónde bloquea y cómo
calcular un ritmo de mensajes seguro.

> Detalle completo (todas las funciones, configuración de pines, errores comunes) en
> [`_origen/09_DEBUG_FRMWRK.md`](./_origen/09_DEBUG_FRMWRK.md).

## Buenas prácticas al imprimir para depurar

- **Usá el fin de línea que espera tu terminal.** En muchas terminales alcanza con `\n`; otras
  necesitan `\r\n`.
- **Evitá imprimir dentro de una interrupción:** `printf` suele ser lento y no siempre es reentrante.
  Es preferible que la ISR guarde el dato o levante una bandera y que el `main` imprima.
- **Identificá el origen de cada mensaje** (`"[ADC] valor=..."`) para poder seguir una salida extensa.
- **Poder desactivar los mensajes** evita que el diagnóstico altere tiempos y tamaño en la versión
  final. Un macro de compilación permite hacerlo sin borrar código.

```c
#ifdef DEBUG
  #define LOG(s)  _DBG(s)
#else
  #define LOG(s)  ((void)0)   // no hace nada en la versión final
#endif
```

En la [próxima página](./02-el-metodo-del-no-anda.md) usamos breakpoints, watchpoints y registros
para investigar por qué un periférico no responde. Más adelante veremos
[RTT](./04-consola-por-el-debugger-rtt.md), una consola que no ocupa la UART.

---

**Depurar en serio:** [índice](./README.md) · **Siguiente:** [02 - El método del "no anda"](./02-el-metodo-del-no-anda.md)
