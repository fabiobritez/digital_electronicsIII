# Imprimir para depurar

La forma más rápida de saber qué está haciendo tu programa es que **te lo cuente**. Acá van las
herramientas para "imprimir", de la más básica a la más cómoda.

## Nivel 0: el LED de depuración

Sin ningún periférico de comunicación, un LED ya te dice mucho:

```c
GPIO_SetValue(0, LED);     // "llegué hasta acá"
```

- Prendido fijo = el código pasó por ese punto.
- Parpadeo distinto según el caso = distinguir ramas (`if`/`else`).
- LED apagado para siempre = quedaste trabado **antes** de ese punto (o en un `while` de espera, o un
  *hard fault*).

Es burdo pero no necesita nada: ideal cuando todavía no configuraste la UART o cuando sospechás que
el problema está justo en la inicialización.

## Nivel 1: la UART como consola

Mandar texto por la UART (módulo 9) y leerlo en la PC con un terminal serial es **la** herramienta de
depuración en embebidos. Podés imprimir valores de variables, mensajes de estado, el resultado de un
ADC, etc.:

```c
UART_Send(LPC_UART0, (uint8_t*)"Entrando a main\r\n", 17, BLOCKING);
```

Y si redirigís `printf` a la UART (ver módulo 9), tenés mensajes con formato:

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

> Detalle completo (todas las funciones, configuración de pines, errores comunes) en
> [`_origen/09_DEBUG_FRMWRK.md`](./_origen/09_DEBUG_FRMWRK.md).

## Buenas prácticas al imprimir para depurar

- **Terminá las líneas con `\r\n`**, no solo `\n`, o el terminal no salta de línea.
- **No imprimas dentro de una interrupción** salvo que sea imprescindible: `printf` es lento y puede
  romper los tiempos. Mejor: la ISR levanta una bandera, y el `main` imprime.
- **Marcá los mensajes** (`"[ADC] valor=..."`) para ubicarlos rápido cuando hay muchos.
- **Sacá o desactivá los prints** en la versión final: ocupan tiempo y memoria. Un `#define DEBUG`
  con `#ifdef` te deja prenderlos/apagarlos sin borrar código.

```c
#ifdef DEBUG
  #define LOG(s)  _DBG(s)
#else
  #define LOG(s)  ((void)0)   // no hace nada en la versión final
#endif
```

En la [próxima página](./02-debugger-y-metodo.md): el debugger por JTAG/SWD (breakpoints, ver
registros en vivo) y un método ordenado para encontrar por qué un periférico "no anda".

---

**Módulo:** [Debug](./README.md) · **Siguiente:** [02 - El debugger y un método](./02-debugger-y-metodo.md)
