# Ejemplos de UART

Tres programas, siguiendo la progresión del curso. Los dos primeros son el mismo eco serial
(todo lo que llega se devuelve) a dos niveles de abstracción; el tercero engancha `printf` a la
UART:

| Archivo | Nivel | Baudrate |
|---------|-------|----------|
| [`uart_eco_registros.c`](./uart_eco_registros.c) | A registro (DLAB, DLL/DLM, LSR, polling) | 9600 |
| [`uart_eco_driver.c`](./uart_eco_driver.c) | Driver CMSIS (`UART_Init`, `UART_Send/Receive`) | 115200 |
| [`printf_retarget.c`](./printf_retarget.c) | A registro **con fraccional** + `printf` redirigido por `__io_putchar` | 115200 |

Para probarlos: conectá la placa por el puente UART-USB (o el adaptador que tengas en
P0.2/P0.3), abrí una terminal serie con el baudrate correcto y escribí: cada tecla debería
volver como eco.

Teoría y explicación paso a paso: [módulo 9: UART](../../09_uart/). El retargeting de `printf`,
en [módulo 0, capítulo 16](../../00_lenguaje_c/16-redirigir-printf-a-uart.md).

> ¿Por qué el de registro usa 9600 y los otros dos 115200? Porque a 115200 con PCLK de 25 MHz
> el divisor entero no alcanza (error > 3%) y hace falta el divisor fraccional. El de driver deja
> que `UART_Init` lo calcule; `printf_retarget.c` lo tiene resuelto a mano
> (`DL = 10, MULVAL = 14, DIVADDVAL = 5`) para que se vea de dónde sale cada número. Está
> explicado en la [página 1 del módulo 9](../../09_uart/01-uart-registros.md).

> **Los tres suponen `CCLK = 100 MHz`** (y por lo tanto `PCLK_UART0 = 25 MHz`). Eso no es el estado
> del micro después de un reset: hay que llamar a `SystemInit()`. MCUXpresso lo hace solo; con la
> plantilla del repo hay que compilar con `make USE_CMSIS=1`. Si no, el micro queda a 4 MHz, ningún
> baudrate da y solo vas a ver basura en la terminal.

## Si el texto llega cortado o mezclado

Síntoma: se leen palabras enteras y correctas, pero salteadas y entreveradas, como si faltaran
pedazos al azar.

```
 1740
ttt[rx] 0x0tick 1755t'.'
tick 1tt
        k 1765
```

**Casi seguro tenés dos programas leyendo el mismo puerto.** Un puerto serie entrega cada byte a
**un solo** lector: si dejaste un `cat /dev/ttyUSB0` de fondo y después abrís minicom, el kernel
reparte los bytes entre los dos y cada uno recibe la mitad. No se duplica nada, se parte.

Fijate quién lo tiene abierto y cerralo:

```bash
ls -l /proc/*/fd/* 2>/dev/null | grep ttyUSB0
```

Lo que distingue este caso de un **error de baudrate** es que acá hay líneas perfectamente
escritas (`tick 1870`). Con el baudrate mal nunca aparece una palabra bien formada: sale basura
pareja de punta a punta. Caracteres correctos pero incompletos = alguien más se los está llevando.

Aparte, es normal ver un `[rx] 0x00` suelto justo al abrir la terminal: al conectarse, el programa
activa DTR/RTS y reconfigura el conversor USB-serie, y en esa transición la línea pega un pulso que
la UART toma como un byte nulo. Pasa una sola vez y no molesta.

## Probado en placa

`printf_retarget.c` se compiló sobre `plantilla/` con `make USE_CMSIS=1`, se grabó en una
LPCXpresso LPC1769 por CMSIS-DAP y se verificó su salida a 115200 8N1 con un conversor USB-serie
en P0.2/P0.3, en ambos sentidos:

```
=========================================
  LPC1769 - printf() por UART0
  115200 8N1, TXD0 = P0.2, RXD0 = P0.3
=========================================
SystemCoreClock = 100000000 Hz
PCLK_UART0      = 25000000 Hz
con DLAB=0 leo: 0x00 0x00  (no son DLL/DLM: son RBR e IER)
con DLAB=1 leo: DLL=10 DLM=0 FDR=0xE5 (MULVAL=14 DIVADDVAL=5)
```
