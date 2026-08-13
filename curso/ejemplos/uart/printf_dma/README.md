# `printf` por DMA: depuración que no le roba tiempo al CPU

En [`printf_retarget.c`](../printf_retarget.c) el `printf` funciona, pero el CPU se queda parado
esperando a la UART. Medido en placa, imprimir una línea de 48 caracteres a 115200 cuesta
**4091 µs de CPU bloqueado**: 409.000 ciclos a 100 MHz sin hacer nada, esperando a que salga un bit
por vez. En un lazo de control eso es inaceptable.

Acá está la versión que arregla eso. `printf` deja los bytes en una cola circular y vuelve; el
**GPDMA** los va sacando por la UART solo.

| | ciclos | tiempo de CPU |
|---|---:|---:|
| `_DBG` del Debug Framework (48 bytes) | 400542 | 4005 µs |
| `printf` por polling (48 caracteres) | 409095 | 4091 µs |
| **`printf` por DMA (los mismos 48 caracteres)** | **3562** | **36 µs** |

**114 veces menos.** Y lo que queda son casi todos ciclos de formateo, no de espera.

## Cómo funciona

```
  printf()  →  _write()  →  cola circular  →  GPDMA  →  THR  →  P0.2
               (productor: main)             (consumidor: hardware)
                                                  │
                                            al terminar un tramo,
                                            su ISR arranca el siguiente
```

- `_write()` copia los bytes a la cola y vuelve. Nunca espera.
- El GPDMA (canal 0, memoria → periférico) pide un byte cada vez que la FIFO de transmisión de la
  UART deja de estar llena. Eso se habilita con el bit **DMA Mode Select** (`FCR[3]`), que según el
  UM10360 §14.4.6.1 solo tiene efecto con las FIFOs habilitadas.
- Cuando el DMA termina un tramo, su interrupción avanza la cola y arranca el siguiente. Esa ISR es
  lo único que el CPU gasta por tramo, **sin importar si el tramo eran 10 bytes o 2000**.

## Tres decisiones que vale la pena entender

### 1. La cola vive en la AHB SRAM, no en la RAM principal

```c
static uint8_t cola[2048] __attribute__((section(".ahbram0")));
```

En el LPC176x el GPDMA alcanza las dos memorias —el UM10360 §1.9 dice que los 32 kB de SRAM local
son *"accessible by the CPU and all three DMA controllers"*—, así que **no es una obligación, es una
decisión de rendimiento**: los dos bancos de AHB SRAM cuelgan de puertos esclavos separados de la
matriz AHB, de modo que el DMA leyendo la cola y el CPU trabajando en la RAM principal no compiten
por el mismo bus. Son 16 kB que de otro modo quedarían sin usar.

(Ojo si venís de otras familias: en varias, el DMA directamente **no** puede leer la SRAM local.
Acá sí. Es de esas cosas que hay que verificar en el manual de cada chip y no dar por sabidas.)

### 2. Con DMA, `setvbuf(_IONBF)` es contraproducente

Este es el resultado más contraintuitivo de todo el ejercicio, y salió de medir.

Con la salida **por polling** conviene stdout sin buffer: cada byte sale al instante y si el
programa se cuelga ya viste el último mensaje.

Con **DMA** eso es un error. Sin buffer, newlib llama a `_write()` **una vez por carácter**, y cada
llamada arranca una transferencia de 1 byte con su interrupción de fin: diez caracteres son diez
ISRs. Medido: **un `printf` de 20 µs se convierte en uno de 68 µs.**

Con buffering de línea, newlib acumula hasta el `\n` y entrega la línea entera de un saque: una
transferencia, una ISR. Y no se pierde la garantía de "lo último que ves es lo último que pasó",
porque la cola se vacía sola; para el caso extremo está `dbg_uart_flush()`.

```c
static char buf_stdout[128];
setvbuf(stdout, buf_stdout, _IOLBF, sizeof buf_stdout);
```

El buffer se lo damos nosotros a propósito: si le pasás `NULL`, el primer `printf` reserva 1032
bytes con `malloc` (ver [Herramientas 06, `printf` por UART §6.2](../../../../herramientas/06_depurar_en_serio/05-redirigir-printf-a-uart.md)).

### 3. Cuando la cola se llena, se descarta (y se cuenta)

**El DMA saca al CPU del camino, pero no acelera el cable.** La línea sigue siendo de 115200 baud.
Si imprimís más rápido de lo que se transmite, las opciones son dos: bloquear (y volvés al problema
original) o descartar.

Este módulo **descarta y lleva la cuenta**, porque en depuración es preferible perder texto a frenar
el programa que estás tratando de observar. `dbg_uart_perdidos()` te dice cuánto se perdió: si ese
número crece, estás imprimiendo de más.

La prueba de saturación del ejemplo lo muestra a propósito: 200 líneas de golpe contra una cola de
2048 bytes descartan ~3600 bytes. No es un bug, es el límite físico haciéndose visible.

## Compilar y probar

```bash
cd plantilla
cp ../curso/ejemplos/uart/printf_dma/dbg_uart.h src/
cp ../curso/ejemplos/uart/printf_dma/dbg_uart.c src/
cp ../curso/ejemplos/uart/printf_dma/main.c     src/
make USE_CMSIS=1 flash
```

`USE_CMSIS=1` es obligatorio, como en todos los ejemplos de UART: sin `SystemInit()` el micro corre
a 4 MHz y 115200 no se puede generar.

Terminal a 115200 8N1. Salida real de la placa:

```
=== printf por DMA contra printf por polling ===
linea de prueba: 48 caracteres
[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C
[t=1234567] adc=2048 estado=3 err=0 temp=25.4 C

ciclos que le cuesta al CPU (10 ns cada uno):
  printf por DMA      :   3562  (35 us)
  printf por polling  : 409095  (4090 us)
  el CPU trabaja 114 veces menos

prueba de saturacion: 200 lineas de golpe...
  las 200 llamadas tardaron 763660 ciclos (7636 us)
  quedaban 2047 bytes en la cola al terminar
  se descartaron 3616 bytes por cola llena
```

## Lo que cuesta

Medido compilando **el mismo `main`** (un `printf("tick %lu\n")` en un lazo) contra los dos
backends, para que la comparación sea de igual a igual:

| | polling | DMA | diferencia |
|---|---:|---:|---:|
| Flash (`.text`) | 7304 B | 7664 B | **+360 B** (0.07% de 512 KB) |
| RAM principal (`.bss`) | 360 B | 504 B | **+144 B** |
| AHB SRAM | 0 | 2048 B | la cola |
| Heap | 0 | 0 | ninguno de los dos llama a `malloc` |

De los +144 B de RAM principal, 128 son el buffer de stdout que le damos a newlib y 16 son los
índices y contadores de la cola. Los 2048 B de la cola salen de los 16 KB de AHB SRAM que estaban
sin usar, así que en la práctica **no compiten con nada**.

Se lleva además **un canal de GPDMA** (el 0) de los 8 que hay.

360 bytes de Flash y 144 de RAM para que el CPU trabaje 114 veces menos: es de las mejores
relaciones costo-beneficio del curso.

## Qué falta / hasta dónde llega

- **No es reentrante entre tareas.** `_write` asume un solo productor. Con un RTOS hace falta un
  mutex alrededor del encolado, o una cola por tarea.
- **Un `printf` desde una ISR sigue siendo mala idea**, pero por otra razón: ya no bloquea, pero
  `printf` sigue sin ser reentrante y sigue gastando ~376 bytes de stack.
- **Sólo transmisión.** La recepción sigue siendo por polling o interrupción.
- Si necesitás que no se pierda **nada**, hay que agrandar la cola o bajar el volumen de impresión;
  no hay una tercera opción mientras la línea sea de 115200.

### La garantía: o entra el mensaje entero, o no entra nada

Cuando la cola se llena, se descarta **el mensaje completo**, nunca la mitad. Es una decisión
deliberada, no un detalle:

> **Todo lo que ves está completo y en orden. Lo que no entró, no aparece, y está contado en
> `dbg_uart_perdidos()`.**

La alternativa —recortar a mitad de línea— produce cosas como `adc=20` cuando la línea era
`adc=2048 temp=25.4 C`: una línea truncada que *parece* válida y te manda a buscar un bug que no
existe. Verificado en placa saturando la cola a propósito: de las líneas que llegan, **cero
truncadas**, todas en orden creciente.

Cuánto podés imprimir sin llegar a ese punto está en
[Herramientas 06, `printf` por UART §10](../../../../herramientas/06_depurar_en_serio/05-redirigir-printf-a-uart.md).

## Ver también

- [Módulo 11 - DMA](../../../11_dma/) · [Módulo 09 - UART](../../../09_uart/)
- [Herramientas 06, `printf` por UART §6](../../../../herramientas/06_depurar_en_serio/05-redirigir-printf-a-uart.md) — de dónde salen
  los 4091 µs y por qué el costo de `printf` no es `printf`
- [`../printf_retarget.c`](../printf_retarget.c) — la versión por polling, que es de donde conviene
  arrancar para entender esta
