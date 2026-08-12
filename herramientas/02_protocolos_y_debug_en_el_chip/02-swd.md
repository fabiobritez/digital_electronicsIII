# SWD (Serial Wire Debug)

SWD es el puerto de depuración más habitual en Cortex-M y el que usa la placa de la cátedra.
La idea puede resumirse así:

> **El mismo acceso de depuración que JTAG, con dos señales en lugar de cuatro o cinco.**

No reemplaza el boundary scan ni el encadenamiento de dispositivos propios de JTAG.

---

## El problema que resuelve

En un microcontrolador chico, los pines son el recurso más caro que hay. Un LQFP de 48 patas
que le regala 5 a la depuración está regalando el 10% de su conectividad para algo que solo
se usa mientras se desarrolla.

ARM lo resolvió reemplazando el transporte de JTAG (no el destino) por uno serie
bidireccional:

| | JTAG | SWD |
|---|---|---|
| Reloj | TCK | **SWCLK** |
| Datos | TDI (entra) + TDO (sale) | **SWDIO** (entra *y* sale, medio dúplex) |
| Control | TMS | va en el propio paquete |
| Reset de test | TRST | no existe |
| **Total** | **4 o 5 pines** | **2 pines** |

SWD usa una línea de datos bidireccional y paquetes con respuesta. A cambio de un protocolo
algo menos simétrico, reduce la cantidad de señales sin perder las funciones de depuración.

---

## Cómo es el protocolo

SWD es sincrónico y **orientado a paquetes**. Cada transacción tiene tres fases y siempre la
misma pinta:

```
   host  ──►  [ pedido: 8 bits ]
              ├─ start (1)
              ├─ APnDP  : ¿le hablo al DP o al AP?
              ├─ RnW    : ¿leo o escribo?
              ├─ A[3:2] : cuál de los 4 registros
              ├─ paridad
              ├─ stop (0)
              └─ park (1)

   target ──►  [ confirmación: 3 bits ]
              ├─ OK    (001)  todo bien
              ├─ WAIT  (010)  estoy ocupado, reintentá
              └─ FAULT (100)  hubo un error, mirá el registro de estado

   host/target  [ datos: 32 bits + paridad, según sea escritura o lectura ]
```

Tres cosas para notar:

1. **La línea de datos cambia de dueño en el medio de la transacción.** Por eso hay ciclos
   de *turnaround* entre las fases: alguien tiene que soltar la línea antes de que la tome el
   otro. Es la razón por la que SWD no es simplemente "JTAG con menos cables": el temporizado
   es distinto.
2. **El target confirma cada pedido.** Puede responder `WAIT` si todavía no puede completar
   la operación o `FAULT` si detectó un error. El host decide entonces si reintenta o lee
   el estado del puerto.
3. **Cada pedido selecciona una de cuatro direcciones de registro.** Los registros de banco
   y selección permiten llegar al resto del DP y de los AP. Un AP de memoria agrega una
   dirección y un registro de datos para acceder al mapa del chip.

No hace falta implementar esto a mano. Lo importante es reconocer que una lectura de memoria
se descompone en varios paquetes, con cambios de dirección de la línea, confirmación y
paridad. Ese costo ayuda a entender por qué el caudal real es menor que la frecuencia de
`SWCLK` sugeriría.

---

## Los dos, sobre los mismos pines: el SWJ-DP

Una pregunta razonable: si el chip trae los dos protocolos, ¿cómo sabe cuál está usando la
sonda?

La respuesta es elegante. El bloque que atiende los dos se llama **SWJ-DP** (*Serial Wire /
JTAG Debug Port*), y los pines se comparten:

| Pin físico | Como JTAG | Como SWD |
|---|---|---|
| pin de reloj | TCK | SWCLK |
| pin de datos | TMS | SWDIO |
| TDO | TDO | **SWO** (traza), si lo habilitás |
| TDI | TDI | libre |

En el LPC1769, el puerto arranca en **modo JTAG**. Para seleccionar SWD, la sonda envía una
secuencia definida por Arm: un reinicio de línea, el patrón JTAG-to-SWD de 16 bits
(`0xE79E`, transmitido bit menos significativo primero) y otro reinicio de línea. El
SWJ-DP reconoce esa secuencia y conmuta el transporte.

> Esto explica un detalle del manual del LPC1769: *"Debugging with the LPC176x/5x defaults to
> JTAG. Once in the JTAG debug mode, the debug tool can switch to Serial Wire Debug mode"*
> (UM10360 §33.3). No es una rareza de NXP, es cómo funciona el SWJ-DP de ARM.

Por eso la configuración de OpenOCD para esta placa selecciona explícitamente el transporte:

```tcl
transport select swd
```

Si ningún archivo de configuración selecciona SWD, OpenOCD puede intentar usar JTAG y la
conexión no se establece como se esperaba.

---

## SWO: el tercer pin, que no es parte de SWD

Suele confundirse porque el nombre se parece, así que conviene separarlo bien:

| | SWD | SWO |
|---|---|---|
| Pines | 2 (SWCLK, SWDIO) | 1 |
| Dirección | bidireccional | **solo salida**, del chip a la sonda |
| Para qué | leer/escribir memoria, controlar el CPU | que el chip **escupa** traza y texto |
| ¿Hace falta? | sí, es el canal de depuración | no, es opcional |

Una vez configurados ITM, DWT y TPIU, el chip puede emitir datos por SWO aunque el host no
los consuma. La codificación puede ser NRZ asíncrona o Manchester. El detalle de esos bloques
está en el [capítulo siguiente](./03-adentro-del-cortex-m3.md).

En el LPC1769, SWO **comparte pin con TDO**. Al usar SWD, esa señal JTAG queda disponible
para la salida de traza. Como es un pin dedicado de depuración en este chip, no consume un
GPIO de la aplicación.

La trampa: **tu sonda tiene que soportar capturarlo**, y muchas no. La de la cátedra, por
ejemplo, no puede ([ver 07-01](../07_lpc1769/01-la-placa-y-su-sonda.md)).

---

## Velocidad

`SWCLK` la fija la sonda, no el chip. Los valores típicos:

| Velocidad | Cuándo |
|---|---|
| 100 a 500 kHz | conexión inicial, cables largos, placas ruidosas |
| 1 a 4 MHz | el rango normal de trabajo |
| 10 a 50 MHz | sondas buenas, pistas cortas, chips rápidos |

La frecuencia máxima depende de la implementación del chip, de la sonda y de la integridad
de señal. Durante el arranque o después de un cambio de clock, una velocidad que antes
funcionaba puede volverse inestable. Para la primera conexión conviene empezar lento y subir
después de verificar el enlace.

En OpenOCD:

```tcl
adapter speed 1000      # kHz
```

Si aparecen desconexiones intermitentes durante el grabado, bajar esta velocidad y acortar
los cables son dos pruebas rápidas. También hay que revisar masa común, VTref, alimentación
y reset.

---

## Cuándo sigue conviniendo JTAG

Casi nunca, con un Cortex-M solo en la placa. Pero:

| Situación | Por qué JTAG |
|---|---|
| Varios chips programables en la misma placa | se encadenan; SWD no encadena |
| Test de fabricación por boundary scan | SWD no hace boundary scan |
| Un chip que no implementa SWD | JTAG puede ser el puerto disponible |
| Herramientas o flujos de fabricación ya basados en JTAG | evitan cambiar infraestructura |

---

## Lo que hay que recordar

- SWD usa dos señales: SWCLK y SWDIO. En un SWJ-DP llega al mismo sistema de depuración que
  JTAG, pero no ofrece boundary scan ni cadenas de dispositivos.
- En el LPC1769, la sonda selecciona SWD mediante una secuencia definida por Arm; de ahí la
  línea `transport select swd` de OpenOCD.
- **SWO no forma parte de SWD**: es una salida de traza opcional que comparte pin con TDO en
  este chip.
- Ante fallas intermitentes, probá una `adapter speed` menor, cables cortos, masa común y una
  alimentación estable.
- SWD da acceso al DP y a los AP. El capítulo siguiente explica los bloques que aparecen
  detrás de ese acceso.

---

**Protocolos:** [índice](./README.md) ·
**Anterior:** [01 - JTAG](./01-jtag.md) ·
**Siguiente:** [03 - El debug adentro del Cortex-M3](./03-adentro-del-cortex-m3.md)
