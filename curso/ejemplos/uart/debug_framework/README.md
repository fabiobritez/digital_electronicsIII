# Medir el Debug Framework de NXP

El Debug Framework simplifica la salida por UART, pero no la vuelve asíncrona. En la versión de la
biblioteca incluida en el repositorio, `_DBG()` recorre el string y cada carácter termina en:

```c
UART_Send(UARTx, &ch, 1, BLOCKING);
```

Por eso su costo temporal es casi el mismo que el de un `printf` redirigido a una UART por polling.
La ventaja del framework está en la interfaz corta y en no depender de `printf`, no en liberar al
CPU.

## Resultado medido

Banco usado: LPCXpresso LPC1769 a 100 MHz, UART0 a 115200 8N1, biblioteca CMSISv2p00 del
repositorio, `arm-none-eabi-gcc` 13.2 y `-Og`. Antes de cada medición se esperó `TEMT=1` para empezar
con la UART vacía. Cada valor corto es la mediana de cinco repeticiones.

| Operación | Bytes producidos | Ciclos | Tiempo dentro de la llamada |
|---|---:|---:|---:|
| `_DBG(texto)` | 48 | 400 542 | 4005 µs |
| `printf("%s", texto)` por la misma UART | 48 | 401 497 | 4014 µs |
| `_DBG("valor=")` + `_DBD32(valor)` + `_DBG("\n")` | 17 | 131 269 | 1312 µs |
| `printf("valor=%010lu\n", valor)` por la misma UART | 17 | 132 340 | 1323 µs |

En la línea de 48 bytes, evitar `printf` ahorró 955 ciclos, es decir, 9,55 µs o cerca del 0,24 %.
El formateo no cambia la conclusión: casi todo el tiempo se consume esperando que la UART pueda
aceptar el siguiente carácter.

Para medir caudal se enviaron 1000 líneas consecutivas con `_DBG()`:

| Bytes transmitidos | Tiempo dentro de `_DBG` | Caudal calculado |
|---:|---:|---:|
| 48 000 | 4 168 979 µs | **11 513 B/s** |

El resultado coincide con el límite de una UART a 115200 baudios y trama 8N1: cada byte necesita
10 bits, por lo que el máximo nominal es 11 520 B/s. Una ejecución larga hace despreciables los
pocos bytes que todavía podrían quedar en la FIFO al volver la última llamada.

## Reproducirlo

Desde la raíz del repositorio:

```bash
cp curso/ejemplos/uart/debug_framework/main.c plantilla/src/main.c
cd plantilla
make USE_CMSIS=1 flash
```

Abrí UART0 a 115200 8N1 mediante el conversor USB-serie conectado a P0.2 y P0.3:

```bash
stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parenb raw -echo
cat /dev/ttyUSB0
```

El programa deja además los resultados en la variable global `resultados`. Esto permite leerlos
con GDB aunque no haya un conversor serie disponible:

```bash
# terminal 1, desde plantilla/
make gdbserver

# terminal 2
gdb-multiarch build/firmware.elf
```

```gdb
target remote :3333
monitor reset run
# esperar a que termine la prueba de caudal
monitor halt
p resultados
```

La marca `resultados.terminado == 0xC0DEF00D` indica que finalizó la prueba.

## Comparación con las otras alternativas

Estos valores corresponden a la placa y las implementaciones del repositorio. La línea de prueba
es la misma en todos los casos.

| Salida | Tiempo de CPU por línea | Caudal en esta configuración | Qué ocupa |
|---|---:|---:|---|
| Debug Framework, UART por polling | **4005 µs** | 11 513 B/s | UART0 y TXD |
| `printf` por UART, polling | 4091 µs | cerca de 11 520 B/s | UART0 y TXD |
| `printf` por UART, DMA | 36 µs | cerca de 11 520 B/s a 115200 | UART0, TXD, RAM y un canal DMA |
| `printf` por RTT | **17 µs** | 15 660 B/s | RAM e interfaz de debug |

El valor de 4091 µs pertenece al ejemplo normal de `printf`, que convierte el `\n` final en
`\r\n`. En la comparación estricta de 48 bytes dentro de este banco, `printf` dio 4014 µs. Esta
diferencia no altera la decisión.

El DMA reduce el tiempo que la llamada ocupa al CPU, pero no cambia por sí solo el baudrate de la
UART. A 115200, el framework, el polling y el DMA comparten el mismo techo físico. Si se configura
la UART a 921600, el máximo nominal sube a 92 160 B/s; eso requiere verificar que el conversor y el
receptor soporten esa velocidad sin errores.

## Qué conviene usar

- **RTT** es la mejor consola durante el desarrollo con esta placa: devuelve antes el control al
  programa, no ocupa una UART y supera por poco el caudal de la UART a 115200. Depende de que la
  sonda y el lector estén conectados.
- **UART con DMA** conviene para un flujo sostenido, independiente del debugger, o cuando necesitás
  subir mucho el baudrate. Exige una cola, un canal DMA y una política ante saturación.
- **Debug Framework** conviene para ejemplos simples o para mantener código antiguo de NXP. Es más
  liviano conceptualmente que retargetizar `printf`, pero bloquea al CPU durante casi todo el tiempo
  de transmisión.
- **`printf` por polling** aporta formato flexible con el mismo problema temporal del framework.

No conviene conectar `printf` al Debug Framework esperando una mejora de rendimiento. El gancho
`__io_putchar()` del ejemplo muestra ese caso: ambos terminan en la misma espera bloqueante. La
elección que cambia el comportamiento es el backend, por ejemplo polling, DMA o RTT, no si la API
visible se llama `_DBG` o `printf`.
