# Las vías de salida de datos

Un microcontrolador no trae una consola de texto por defecto. Para observar qué hace hay que
elegir un canal. Esta página compara cuatro opciones habituales para diagnóstico: UART,
semihosting, SWO/ITM y RTT. No son las únicas, pero muestran bien el compromiso entre pines,
hardware, ancho de banda y tiempo de CPU.

El detalle práctico de cada uno está en
[06 - Depurar en serio](../06_depurar_en_serio/).

---

## 1. UART: el clásico

El micro tiene un transmisor serie de hardware. Le cargás un byte, el periférico saca los
bits solos a velocidad constante, y del otro lado un adaptador USB-serie compatible se los
entrega a la PC.

```
   printf()  →  _write()  →  registro de la UART  →  pin TXD  →  conversor  →  PC
```

| A favor | En contra |
|---|---|
| Puede transmitir sin una sonda conectada y sirve en un equipo instalado | usa **un pin** y **un periférico** |
| El baudrate fija un caudal predecible | si el proyecto usa esa UART para RS-485, un GPS u otra función, no queda libre para diagnóstico |
| Hay adaptadores y terminales serie en casi cualquier plataforma | por *polling*, esperar espacio para cada carácter puede consumir mucho tiempo de CPU |
| Con DMA, el costo de CPU baja a casi nada | necesitás el conversor |

Es una opción simple y autónoma, siempre que el clock, el pin, el baudrate y el cableado
estén bien configurados.

El **Debug Framework de NXP** no es una quinta vía de salida. Es una API de conveniencia sobre esta
misma UART: `_DBG()` y las macros numéricas terminan usando transmisión bloqueante por polling. En
la placa del curso, una línea de 48 bytes ocupó al programa durante 4005 µs, casi lo mismo que
`printf` por UART. Una [versión mejorada](../../curso/ejemplos/uart/debug_framework_mejorado/)
mantiene las macros, pero permite usar interrupciones o DMA.

---

## 2. Semihosting: que la PC atienda los pedidos

Es un truco viejo y muy ingenioso. El firmware ejecuta una instrucción `BKPT` con un código
especial; el CPU se **frena**; el depurador, que está del otro lado, mira qué le estaban
pidiendo (abrir un archivo, escribir texto, leer una tecla), **lo ejecuta en la PC**, deja el resultado en registros o memoria y reanuda el micro.

```
   printf()  →  BKPT 0xAB  →  [ el micro se FRENA ]
                                     ↓
                         el servidor de depuración lo atiende
                                     ↓
                             [ el micro sigue ]
```

| A favor | En contra |
|---|---|
| No gasta pines ni periféricos | detiene el núcleo en cada operación de semihosting y suele ser muy lento |
| El micro puede acceder a servicios o archivos del host | sin un depurador que lo atienda, el `BKPT` puede terminar en un fault o dejar la aplicación detenida |
| Newlib ofrece soporte mediante `--specs=rdimon.specs` | altera fuertemente el temporizado |

Puede servir para una prueba controlada o para acceder a archivos del host. No es apropiado
para estudiar temporizado en tiempo real. Además, el comportamiento cambia cuando el
depurador no está conectado, así que no conviene dejarlo inadvertidamente en una imagen de
producción.

---

## 3. SWO / ITM: el camino "de manual"

El programa escribe un byte en un puerto del **ITM**
([capítulo anterior](./03-adentro-del-cortex-m3.md)) y ese byte sale por el pin **SWO**. El
firmware puede usar una escritura no bloqueante y seguir; si el puerto no está listo, debe
decidir si espera o descarta el dato.

```
   printf()  →  ITM->PORT[0]  →  TPIU  →  pin SWO  →  la sonda  →  PC
```

| A favor | En contra |
|---|---|
| Costo de CPU bajo con una implementación no bloqueante | **la sonda tiene que soportarlo**, y muchas no |
| En el LPC1769 no usa un GPIO de aplicación porque SWO comparte el pin dedicado TDO | el pin tiene que estar cableado hasta la sonda |
| Por el mismo canal salen los watchpoints, el muestreo de PC y los sellos de tiempo | hay que configurar el ITM, el TPIU y la velocidad de la traza, y es fácil equivocarse |
| Es el mecanismo **estándar de ARM** | si el FIFO se llena, se pierden datos en silencio |

Es el mecanismo de traza previsto por CoreSight, pero depende de toda la cadena: configuración
del chip, pin SWO, firmware de la sonda y soporte del host. **La sonda de la cátedra no
implementa la captura necesaria**, aunque el LPC1769 sí puede emitir SWO
([ver 07-01](../07_lpc1769/01-la-placa-y-su-sonda.md)).

---

## 4. RTT: el host va a buscar los datos

RTT aprovecha una propiedad del capítulo anterior: el Access Port puede leer RAM mientras el
CPU corre. Con la sonda de esta placa resulta especialmente útil porque no depende de SWO.

Entonces no usa pines ni periféricos de la aplicación. Sí ocupa RAM y el acceso de
depuración: el firmware escribe en una **cola circular**, y el host la lee por SWD y la vacía.

```
   printf()  →  cola circular en RAM
                       ↑
              la sonda la lee por SWD
              MIENTRAS el programa corre
```

| A favor | En contra |
|---|---|
| Para el micro, escribir suele costar una copia corta a un buffer | **necesita la sonda conectada y el servidor corriendo** para que el host reciba |
| No usa pines ni periféricos de la aplicación | el caudal depende de la frecuencia de lectura y de la sonda |
| Es bidireccional: también podés enviar datos al micro | el modo no bloqueante descarta texto si el buffer se llena |
| No requiere soporte de SWO | la sonda y el servidor deben poder leer memoria en segundo plano |
| Puede usarse apenas estén inicializados la RAM y el bloque de control | no muestra lo ocurrido antes de esa inicialización |

Es "alguien que pasa cada tanto con un balde", contra la UART que es "una cinta
transportadora andando sola". Los dos mueven cosas; no de la misma manera.

---

## La comparación

| | UART (polling) | UART (DMA) | Semihosting | SWO / ITM | RTT |
|---|---|---|---|---|---|
| **Costo de CPU** | alto si espera cada byte | bajo | **muy alto** | muy bajo si no bloquea | bajo |
| **¿Frena el micro?** | no | no | **sí, en cada operación** | no | no |
| **Pines** | 1 | 1 | 0 | 1 (compartido con TDO) | **0** |
| **Periféricos** | UART | UART + DMA | ninguno | ninguno | **ninguno** |
| **¿Anda sin PC?** | **sí** | **sí** | no; puede detenerse o generar un fault | el firmware corre, pero nadie captura | el firmware corre, pero nadie vacía el buffer |
| **¿Anda sin sonda?** | **sí** | **sí** | no | no | no |
| **Caudal** | el del baudrate | el del baudrate | ínfimo | alto | depende de la sonda |
| **Bidireccional** | sí | sí | sí | no (SWO es solo salida) | **sí** |
| **Soporte requerido** | adaptador serie | adaptador serie | servidor con semihosting | sonda y servidor con SWO | lectura de memoria en ejecución |

### Los números medidos en esta placa

En la implementación de este repositorio, sobre el LPC1769 a 100 MHz y para una línea de 48
caracteres, se midieron estos tiempos de CPU
([procedimiento reproducible](../../curso/ejemplos/uart/MEDICIONES.md)):

| Vía | CPU por línea |
|---|---:|
| Debug Framework de NXP, UART por polling | 4005 µs |
| Debug Framework mejorado, UART con DMA | **5,90 µs** |
| UART por polling | 4091 µs |
| UART por DMA | 36 µs |
| **RTT** | **17 µs** |

Y en caudal sostenido, la comparación se da vuelta:

| Vía | Caudal |
|---|---:|
| RTT afinado, con la sonda de la cátedra | 15 660 B/s |
| **UART a 921600** | **92 160 B/s** |

No se contradicen: **miden dos cosas distintas**. RTT gana en no molestar al programa; la
UART gana en mover volumen. La discusión completa está en
[06 - RTT, sección 8](../06_depurar_en_serio/04-consola-por-el-debugger-rtt.md).

---

## Cómo elegir

> **Con la sonda de la cátedra y para mensajes de diagnóstico: RTT.**
> **Si el equipo debe informar sin PC o mover un flujo sostenido: UART.**
> **Si la cadena completa captura SWO y necesitás eventos con marcas de tiempo: ITM.**
> **Para una prueba puntual que puede detener el núcleo: semihosting.**

Y la advertencia que vale para los cuatro: **antes de llenar el código de `printf`, hacé la
cuenta**. Si imprimís 100 bytes cada milisegundo, pedís 100 kB/s más el overhead del protocolo. Antes
de elegir una vía, compará ese requisito con su caudal real y definí qué hacer cuando el
buffer se llena. Ninguna herramienta corrige por sí sola un enlace saturado.

---

**Protocolos:** [índice](./README.md) ·
**Anterior:** [03 - El debug adentro del Cortex-M3](./03-adentro-del-cortex-m3.md) ·
**Siguiente parte:** [03 - Debug probes](../03_debug_probes/)
