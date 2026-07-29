# Cómo funciona el USB

Antes de tocar registros o código, hay que entender el modelo, porque es muy distinto a todos los
buses que vimos hasta ahora (UART, SPI, I2C). USB es un protocolo grande, con varias capas, y si no
tenés el modelo mental claro el código del stack se te va a hacer imposible de seguir. Esta página es 100%
conceptual; la [02](./02-controlador-usb-lpc1769.md) baja al silicio del LPC1769 y la
[03](./03-usb-en-la-practica.md) al código que vas a tocar.

## Host y device: una relación asimétrica

En USB no hay "dos iguales" como en SPI (maestro/esclavo configurable) o I2C. Siempre hay:

- Un **host** (anfitrión): la PC. Es **el único** que inicia transacciones, organiza el bus, reparte
  el ancho de banda y los tiempos. Manda él.
- Uno o varios **devices** (dispositivos): tu LPC1769. **Nunca** habla por iniciativa propia: solo
  responde cuando el host le pregunta.

Esto es clave y cuesta al principio: tu device **no puede "mandar" un dato cuando quiere**. Lo que hace
es *dejar el dato preparado* en un buzón (endpoint) y esperar a que el host lo venga a buscar. El host
sondea (*polling*) a cada device en intervalos regulares. Si no tenés nada listo, el device responde
NAK ("todavía no") y el host reintenta más tarde.

El LPC1769 puede ser **device** (lo normal y lo que vemos acá), y también host u OTG, pero eso es
mucho más raro y usa otro periférico. Como device, tu placa **espera** que la PC le hable.

Físicamente son 4 cables: **VBUS** (5 V, los pone el host), **GND**, y el par diferencial **D+ / D−**
por donde van los datos. Trabajamos en **full-speed: 12 Mbit/s**. (USB tiene low-speed 1.5 Mbit/s,
full-speed 12, high-speed 480; el LPC1769 es full-speed.)

## El árbol de transferencias

USB define cuatro **tipos de transferencia**, según la necesidad de cada función:

| Tipo | Para qué | Garantías | Ejemplo |
|------|----------|-----------|---------|
| **Control** | configuración, comandos | con verificación, baja prioridad | enumeración (endpoint 0) |
| **Bulk** | mucho dato, sin urgencia | sin pérdida (reenvía), sin garantía de tiempo | pendrive, puerto serie virtual |
| **Interrupt** | poco dato, periódico, baja latencia | el host garantiza sondeo cada N ms | teclado, mouse, "avisos" |
| **Isochronous** | flujo continuo en tiempo real | ancho de banda reservado, **tolera pérdida** (no reenvía) | audio, video |

Un par de intuiciones que el datasheet no te da:

- **"Interrupt" no es una interrupción de hardware.** Es un endpoint que el host promete sondear cada
  cierto intervalo (lo definís vos en el descriptor: p. ej. cada 10 ms). Sirve para mandar avisos
  chiquitos con latencia acotada (un teclado avisa "se apretó una tecla").
- **Bulk vs Isochronous:** bulk reenvía si hubo error de CRC (no perdés datos, pero no sabés *cuándo*
  llegan); isochronous reserva ancho de banda y entrega siempre a tiempo, pero si un paquete sale con
  error, *se descarta* (en audio, mejor un click que un retardo).
- La **CDC** (puerto serie virtual) usa **bulk** para los datos. El "puerto serie" no garantiza
  tiempos, igual que una UART real.

## Endpoints: los buzones de datos

Toda la comunicación pasa por **endpoints**: buzones de datos, cada uno con un número (0–15) y una
dirección. No son "puertos TCP": son buffers físicos dentro del periférico.

- Un endpoint **IN** manda datos *hacia el host* (device → PC).
- Un endpoint **OUT** recibe datos *del host* (PC → device).

El **IN/OUT siempre se nombra desde el punto de vista del host**. Esto confunde muchísimo: un endpoint
"IN" es de *salida* para tu micro. Memorizalo: IN = el host *entra* datos = tu device transmite.

Un endpoint se identifica por su **dirección de endpoint** de 8 bits: los 4 bits bajos son el número
(0–15), el bit 7 es la dirección (1 = IN, 0 = OUT). Por eso vas a ver direcciones como `0x82`
(endpoint 2 IN) o `0x02` (endpoint 2 OUT). Un mismo número de endpoint puede tener una versión IN y
una OUT, que son buzones distintos.

### El endpoint 0: control

El **endpoint 0** es especial y obligatorio: es **bidireccional** (tiene IN y OUT) y por ahí pasa toda
la enumeración y los comandos de control. Existe siempre, desde antes de que el device tenga dirección.
Es el "canal de servicio" del USB. Los demás endpoints (1, 2, 3…) los usás para tus datos según la
clase.

Su tamaño de paquete (`bMaxPacketSize0`) en full-speed puede ser 8, 16, 32 o 64 bytes; en el stack del
curso está fijado en **8** (`USB_MAX_PACKET0` en `usbcfg.h`), el mínimo, que siempre funciona. Si ese
valor declarado en el descriptor no coincide con el que usa el firmware, la enumeración falla en seco.

## Enumeración: el "apretón de manos" paso a paso

Cuando enchufás el device, host y device hacen la **enumeración**: una conversación inicial sobre el
endpoint 0 donde el host averigua "¿quién sos y qué sabés hacer?" y deja al device listo para usar.
Vale la pena verla **paso a paso**, porque cuando "Windows dice dispositivo no reconocido" siempre
falló *alguno* de estos pasos:

1. **Conexión.** El device activa el *pull-up* en D+ (full-speed). El host ve subir D+ y sabe "hay
   algo full-speed enchufado".
2. **Reset de bus.** El host fuerza un estado de reset en D+/D−. El device se reinicia y queda con
   **dirección 0** (la dirección por defecto, que comparten todos los devices recién enchufados).
3. **Get Descriptor (Device), primer intento.** El host pide los primeros 8 bytes del *device
   descriptor* (lo único que necesita es `bMaxPacketSize0`, el tamaño del endpoint 0). Con eso ya sabe
   de a cuánto puede mandar por el control endpoint.
4. **Set Address.** El host le asigna una dirección única (1–127) con un control transfer. A partir de
   acá el device responde solo a esa dirección.
5. **Get Descriptor (Device), completo.** Ahora pide el device descriptor entero: VID, PID, clase,
   versión.
6. **Get Descriptor (Configuration).** Pide el *configuration descriptor* y todo lo que cuelga de él
   (interfaces, endpoints, descriptores de clase). De acá el host aprende **qué endpoints tiene** el
   device y de qué tipo.
7. **(El SO carga el driver)** según la clase declarada (CDC → driver de puerto serie, HID → driver de
   entrada, etc.).
8. **Set Configuration.** El host elige una configuración (casi siempre la única, la #1). Recién acá el
   device **activa** sus endpoints de datos y queda **configurado** y operativo.

Si la enumeración falla, suele ser por: descriptores mal armados (un byte de largo equivocado), el
device no responde a tiempo (clock USB mal, no llega a los 48 MHz), o tamaños de endpoint que no
coinciden con lo declarado. Lo vemos en detalle en la [página 03](./03-usb-en-la-practica.md).

## Descriptores: la "tarjeta de presentación"

Los descriptores son estructuras de datos que el device tiene guardadas (en flash) y entrega al host
durante la enumeración. Forman un **árbol**: el configuration descriptor "contiene" interfaces, cada
interface "contiene" endpoints. El host los pide y arma el árbol entero. Programar USB es, en gran
parte, **llenar bien este árbol byte por byte**.

```
Device Descriptor            (identidad: VID/PID, clase, versión, tamaño EP0)
   └─ Configuration Descr.   (esta config: consumo, self/bus-powered, nº interfaces)
        ├─ Interface Descr.  (una "función": clase CDC/HID/MSC, nº de endpoints)
        │    ├─ (descriptores de clase: p.ej. funcionales de CDC)
        │    ├─ Endpoint Descr.  (EP de notificación, p. ej. 0x81 interrupt)
        │    └─ ...
        └─ Interface Descr.  (otra función, si la hay)
             └─ Endpoint Descr. (p. ej. 0x82 bulk IN, 0x02 bulk OUT)
   └─ String Descriptors     (los textos: fabricante, producto, nº de serie)
```

Los campos que más vas a tocar:

- **Device Descriptor:** `idVendor` (**VID**) y `idProduct` (**PID**): la pareja que identifica a tu
  producto y con la que el SO decide qué driver cargar. `bcdUSB` (versión de USB), `bDeviceClass`,
  `bMaxPacketSize0`.
- **Configuration Descriptor:** `wTotalLength` (largo de **todo** el árbol que cuelga; un error clásico
  es no actualizarlo al agregar un endpoint), `bmAttributes` y `bMaxPower` (consumo: hasta 500 mA en
  unidades de 2 mA).
- **Interface Descriptor:** `bInterfaceClass` (la clase: 0x02 CDC, 0x03 HID, 0x08 MSC…), `bNumEndpoints`.
- **Endpoint Descriptor:** `bEndpointAddress` (número + dirección IN/OUT), `bmAttributes` (tipo:
  bulk/interrupt/iso), `wMaxPacketSize` (tamaño máximo de paquete; en full-speed: 8/16/32/64 para
  bulk y control, hasta 64 para interrupt, hasta 1023 para isochronous),
  `bInterval` (cada cuánto sondear, para interrupt/iso).
- **String Descriptors:** los textos en Unicode que ves en la PC ("LPC1769 Virtual COM", el
  fabricante, el número de serie).

Un byte mal en cualquiera de estos y el host no reconoce el dispositivo, o lo reconoce mal.

## Clases de dispositivo: no reinventar la rueda

La parte genial del USB: existen **clases estándar** ya definidas por el USB-IF, con drivers **ya
incluidos** en todos los sistemas operativos. Si tu device declara "soy de la clase X", la PC ya sabe
cómo hablarle **sin instalar nada**. Las que están en el repo:

| Clase | `bInterfaceClass` | Qué hace | Aparece en la PC como | Transfers que usa |
|-------|-------------------|----------|------------------------|-------------------|
| **CDC** (Communications Device Class) | 0x02 / 0x0A | puerto serie virtual | un COM nuevo (como una UART) | control + interrupt (avisos) + **bulk** (datos) |
| **HID** (Human Interface Device) | 0x03 | teclado, mouse, joystick, genérico | un dispositivo de entrada | control + **interrupt** |
| **MSC** (Mass Storage Class) | 0x08 | almacenamiento (pendrive) | un disco extraíble | control + **bulk** |
| **Audio** | 0x01 | placa de sonido | dispositivo de audio | control + **isochronous** |

- **CDC** es la más útil para empezar: convierte tu placa en un "puerto serie por USB", así que
  programás como si fuera una UART (módulo 9) **sin** necesitar el adaptador USB-serial. Por dentro usa
  bulk para los datos y un endpoint interrupt para "notificaciones" de estado de línea.
- **HID** no necesita drivers y es ideal para "que la placa tipee algo" o se haga pasar por un mando.
  Usa endpoints interrupt (poco dato, periódico).
- **MSC** hace que la placa aparezca como un disco; por dentro implementa el protocolo SCSI sobre bulk
  (es la más pesada de las tres).

## El reloj de 48 MHz

Un detalle de hardware que SÍ importa y rompe a todo el mundo: el USB full-speed necesita un reloj de
**exactamente 48 MHz**. El LPC1769 lo genera con una PLL dedicada (PLL1) o derivándolo del PLL del
sistema (PLL0). Si ese reloj no es exacto, la temporización de los bits se va y **el host ni siquiera
enumera** (falla en el paso 3 de arriba). El stack y el `system_LPC17xx.c` lo configuran, pero cuando
veas "el USB no aparece", lo primero a sospechar es el clock. Lo vemos a nivel registro en la
[página 02](./02-controlador-usb-lpc1769.md).

Con este modelo en la cabeza, la [próxima página](./02-controlador-usb-lpc1769.md) baja al
**controlador USB device del LPC1769**: cómo es el silicio por dentro (el SIE, la EP RAM, los
registros), y después la [03](./03-usb-en-la-practica.md) muestra cómo el stack de NXP implementa todo
esto y qué tocarías vos.

---

**Módulo:** [USB](./README.md) ·
**Siguiente:** [02 - El controlador USB del LPC1769](./02-controlador-usb-lpc1769.md)
