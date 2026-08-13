# Debug Framework mejorado

Esta es una versión experimental y separada del Debug Framework de NXP. Conserva los alias
`_DBG`, `_DBD32` y `_DBH32`, pero permite cambiar el mecanismo de transmisión sin modificar los
mensajes de la aplicación.

La guía práctica de integración está en
[`USO_Y_LIMITES.md`](./USO_Y_LIMITES.md). Incluye la prueba de saturación, el uso de stack y ejemplos
de cómo usar y cómo no usar el framework.

El framework original sigue en `library/` y no fue reemplazado. La comparación de base está en
[`../debug_framework/`](../debug_framework/).

## Qué se mejoró

1. Los enteros se convierten primero a un buffer. Para `uint32_t` se procesan dos dígitos por vez
   mediante una tabla de pares decimales.
2. El backend bloqueante entrega strings completos a `UART_Send`, de modo que el driver puede usar
   los 16 lugares de la FIFO.
3. El baudrate puede ser 115200 o 921600.
4. El backend por interrupciones copia el mensaje a una cola circular y carga la FIFO desde la ISR.
5. El backend DMA coloca la cola en AHB SRAM y usa el canal 0 del GPDMA.
6. Las salidas asíncronas nunca esperan lugar. Cada llamada entra completa o se descarta completa,
   y `debug_mejorado_perdidos()` informa cuántos bytes se perdieron.

La aplicación puede usar la API nueva:

```c
DBG_LINE("inicio");
DBG_MSG("contador=");
DBG_DEC32(contador);
DBG_MSG("\r\n");
```

O conservar las macros del framework original:

```c
_DBG("contador=");
_DBD32(contador);
_DBG("\r\n");
```

## Backends

### Bloques

```text
_DBG -> UART_Send(string completo, BLOCKING) -> FIFO -> UART
```

Aprovecha la FIFO y reduce el tiempo de una llamada aislada. Sin embargo, el CPU sigue esperando
mientras se transmiten los primeros bloques. La función puede volver con los últimos 16 bytes aún
en la FIFO.

### Interrupciones

```text
_DBG -> cola circular -> vuelve
             |
             +-> ISR UART0 -> FIFO -> UART
```

No ocupa un canal DMA. A cambio, el CPU debe atender una interrupción aproximadamente cada 16
bytes mientras haya datos pendientes.

### DMA

```text
_DBG -> cola circular -> vuelve
             |
             +-> GPDMA -> FIFO -> UART
```

Es el backend que menos trabajo deja al CPU, pero reserva el canal 0 del GPDMA. La cola vive en
AHB SRAM para no competir con las variables normales de la aplicación.

## Resultados medidos

Banco: LPCXpresso LPC1769 a 100 MHz, GCC 13.2, `-Og`, UART0 y una cola de 2048 bytes. Cada tiempo
corto es la mediana de cinco repeticiones. El caudal se midió transmitiendo 48 000 bytes sin
pérdidas y esperando que saliera el último bit.

La configuración DMA a 921600 también se verificó de punta a punta con un CP2102. La captura recibió
50 389 bytes, repartidos en 1053 líneas: 1048 literales y 5 líneas numéricas. Todas tuvieron el
contenido y la longitud esperados, sin líneas corruptas ni truncadas. Ese total incluye las pruebas
cortas, el tramo sostenido y las 43 líneas que entraron durante la prueba de saturación.

| Backend | Baud | Retorno de `_DBG`, 48 B | Línea numérica, 17 B | Caudal sostenido |
|---|---:|---:|---:|---:|
| Framework original | 115200 | 4005 µs | 1312 µs | 11 513 B/s |
| Bloques | 115200 | 2720 µs | 1312 µs | 11 513 B/s |
| Bloques | 921600 | 350 µs | 168 µs | 92 104 B/s |
| Interrupciones | 115200 | 14,90 µs | 17,97 µs | 11 513 B/s |
| Interrupciones | 921600 | 12,15 µs | 15,81 µs | 92 104 B/s |
| **DMA** | 115200 | **5,90 µs** | **13,33 µs** | 11 513 B/s |
| **DMA** | **921600** | **5,84 µs** | **15,63 µs** | **92 104 B/s** |

La línea numérica produce `valor=0001234567\n` mediante tres llamadas. Por eso incluye la
conversión decimal y más administración de la cola que el literal de 48 bytes.

Con el mismo benchmark, las configuraciones a 115200 ocuparon:

| Backend | Flash total del banco | Cola | Recurso adicional |
|---|---:|---:|---|
| Bloques | 3604 B | ninguna | ninguno |
| Interrupciones | 4024 B | 2048 B en RAM principal | IRQ de UART0 |
| DMA | 4132 B | 2048 B en AHB SRAM | canal 0 del GPDMA |

La diferencia de Flash entre backends es más útil que el total, porque el binario también contiene
el benchmark y su estructura de resultados. Frente a bloques, IRQ agregó 420 B y DMA, 528 B.

### Qué significa “retorno”

En los backends asíncronos, el tiempo de la tabla termina cuando el mensaje quedó encolado. La UART
continúa transmitiendo después. Las interrupciones que cargan la FIFO también consumen CPU más
tarde; el tiempo de retorno no las incluye. DMA necesita una ISR al completar cada tramo, mientras
que el backend por interrupciones atiende la UART aproximadamente cada 16 bytes.

Por eso hay que separar dos preguntas:

- **¿Cuánto tarda la aplicación en recuperar el control?** DMA gana.
- **¿Cuánto tarda el mensaje en llegar a la PC?** Lo determina principalmente el baudrate.

Con 8N1, los 48 bytes necesitan cerca de 4167 µs a 115200 o 521 µs a 921600 para salir por el
cable. Que la llamada DMA vuelva en unos 6 µs no acorta ese tiempo físico: permite que el CPU haga
otra cosa mientras la UART transmite.

Mandar bloques completos reduce de 4005 a 2720 µs una llamada aislada porque deja los últimos bytes
en la FIFO. No aumenta el caudal ni libera al CPU durante una secuencia sostenida. Es una mejora
parcial, no un reemplazo de las interrupciones o el DMA.

### Conversión decimal

El benchmark reproduce el cálculo de `_DBD32` del framework original y lo compara con la tabla de
pares decimales:

| Conversión fija de `uint32_t` | Ciclos |
|---|---:|
| Algoritmo original | 221 a 229 |
| Algoritmo mejorado | 152 a 162 |

La reducción ronda el 30 %. Con una UART bloqueante queda escondida dentro de milisegundos de
espera. Con DMA o interrupciones ya representa una parte visible del costo.

## La prueba de saturación

También se intentaron enviar 200 líneas de golpe, 9600 bytes, sin esperar lugar:

| Backend | Baud | Tiempo para hacer las 200 llamadas | Bytes descartados |
|---|---:|---:|---:|
| Bloques | 115200 | 832 ms | 0 |
| Bloques | 921600 | 104 ms | 0 |
| Interrupciones | 115200 | 776 µs | 7584 |
| Interrupciones | 921600 | 783 µs | 7488 |
| DMA | 115200 | 730 µs | 7584 |
| DMA | 921600 | 729 µs | 7536 |

Esto no es una falla de las versiones asíncronas. La cola puede absorber cerca de 2 KiB y después
debe elegir entre bloquear o perder datos. Para depuración se eligió descartar y contar, porque
frenar el programa puede ocultar o crear el problema que se está investigando.

Una cola más grande demora la saturación, pero no cambia el caudal del cable. Si el contador crece,
hay que imprimir menos, subir el baudrate o aceptar una política de backpressure.

## Cómo usarlo

Copiá la biblioteca mejorada y uno de los programas a la plantilla:

```bash
cp curso/ejemplos/uart/debug_framework_mejorado/debug_frmwrk_mejorado.h plantilla/src/
cp curso/ejemplos/uart/debug_framework_mejorado/debug_frmwrk_mejorado.c plantilla/src/
cp curso/ejemplos/uart/debug_framework_mejorado/demo.c plantilla/src/main.c
cd plantilla
```

DMA a 921600, la configuración recomendada para mayor rendimiento:

```bash
make -B USE_CMSIS=1 \
  EXTRA_CFLAGS="-DDEBUG_BACKEND=DEBUG_BACKEND_DMA -DDEBUG_BAUD=921600" \
  flash
```

Interrupciones a 115200:

```bash
make -B USE_CMSIS=1 \
  EXTRA_CFLAGS="-DDEBUG_BACKEND=DEBUG_BACKEND_IRQ -DDEBUG_BAUD=115200" \
  flash
```

El `-B` fuerza la recompilación. Es importante al cambiar macros porque Make no detecta que las
opciones del comando anterior cambiaron.

Configurá la terminal con el mismo baudrate. Para 921600:

```bash
stty -F /dev/ttyUSB0 921600 cs8 -cstopb -parenb raw -echo
cat /dev/ttyUSB0
```

El ejemplo requiere `USE_CMSIS=1`: los divisores suponen un core a 100 MHz.

## Reproducir las mediciones

Copiá `bench.c` en lugar de `demo.c` y compilá cada combinación:

```bash
cp curso/ejemplos/uart/debug_framework_mejorado/bench.c plantilla/src/main.c
```

El benchmark deja los valores en `resultados_mejorado`. Después de ejecutarlo, se pueden leer con
GDB:

```gdb
target remote :3333
monitor reset run
# esperar unos siete segundos a 115200, o dos a 921600
monitor halt
p resultados_mejorado
```

`resultados_mejorado.terminado == 0xC0DEF00D` confirma que terminó.

Para llevar la salida al límite, `stress.c` agrega mensajes de hasta 2000 bytes, ritmos alrededor del
máximo físico, una inundación de 100 000 llamadas y una prueba con backpressure. Los resultados y su
interpretación están en [`USO_Y_LIMITES.md`](./USO_Y_LIMITES.md).

## Límites y elección

- La cola protege cada llamada, no una línea formada mediante varias macros. Si necesitás atomicidad
  completa, armá el mensaje en un buffer y entregalo con `debug_mejorado_write()` en una sola
  llamada.
- No llames `debug_mejorado_flush()` con interrupciones deshabilitadas cuando usás IRQ o DMA.
- IRQ usa `UART0_IRQHandler`. DMA usa `DMA_IRQHandler`, el canal 0 y la solicitud 8, compartida con
  MAT0.0. Una aplicación que ya ocupa esos recursos debe integrarlos o elegir otro backend.
- El código usa TXD0 en P0.2 y deja P0.3 sin modificar. Sigue siendo específico del LPC1769.
- El modo asíncrono reduce la interferencia, pero imprimir desde una ISR sigue siendo una mala idea.
  Guardá el evento y escribilo desde el `main`.

Para conservar una consola UART, **DMA a 921600** es la versión más eficiente de este banco. Si no
querés reservar un canal DMA, **interrupciones a 921600** es un buen compromiso. Para el uso normal
durante la materia, RTT sigue siendo más cómodo porque no ocupa UART, pines ni DMA; esta versión
mejorada conviene cuando necesitás independencia del debugger o más caudal.
