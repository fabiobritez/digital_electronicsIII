# USB en la práctica: el stack de NXP y un CDC de eco

Ya entendés el modelo ([01](./01-usb-conceptos.md)) y el silicio del LPC1769
([02](./02-controlador-usb-lpc1769.md)). Ahora veamos cómo se usa de verdad: **no escribís el protocolo
USB; usás un stack que ya lo implementa** y le enganchás tu lógica. Esta página arma el rompecabezas y
cierra con un ejemplo concreto de "puerto serie virtual con eco".

## Qué es el stack USB y cómo está en capas

Un *stack* USB es una pila de software en capas, parecida a las del módulo 2 pero más alta. El repo trae
la de NXP/Keil en [`../../library/examples/USBDEV/`](../../library/examples/USBDEV/), con un ejemplo
funcionando por clase. Cada ejemplo tiene **los mismos archivos** (la estructura es siempre igual):

```
  Tu aplicación        vcomdemo.c        (qué hacés con los datos: el main)
        │
  Capa de clase        cdcuser.c, usbdesc.c   (lógica CDC + descriptores)
        │
  Núcleo USB           usbcore.c         (enumeración, control endpoint 0, requests estándar)
        │
  Usuario del core     usbuser.c         (callbacks: conecta el core con tu clase)
        │
  Driver de hardware   usbhw.c           (registros del periférico: lo de la página 02)
        │
  Periférico USB (silicio)
```

| Archivo | Rol | ¿Lo tocás? |
|---------|-----|------------|
| `usbhw.c` / `usbreg.h` | driver de hardware (registros, SIE, ISR) | casi nunca |
| `usbcore.c` | núcleo: enumeración, requests estándar, EP0 | nunca |
| `usbdesc.c` | **los descriptores** (device, config, interface, endpoint, strings) | **sí**, para VID/PID/nombres |
| `usbcfg.h` | configuración (nº de endpoints, DMA on/off, tamaño EP0…) | a veces |
| `usbuser.c` | callbacks del core | rara vez |
| `cdcuser.c` | lógica de la clase CDC (buffers, requests CDC) | a veces |
| `vcomdemo.c` | **tu aplicación** (el `main`) | **sí** |

El reparto de tareas en una frase: **el stack hace toda la enumeración, el endpoint 0 y los registros;
vos trabajás con "llegó un byte / mando un byte" y con los descriptores.**

| Lo tocás vos | Lo hace el stack |
|--------------|------------------|
| Tu lógica de aplicación (`vcomdemo.c`) | Enumeración con el host |
| Los descriptores: VID/PID, nombres, consumo (`usbdesc.c`) | Manejo del endpoint 0 (control) |
| La clase que elegís (CDC/HID/MSC) en `usbcfg.h` | Los requests estándar (Get Descriptor, Set Address…) |
| A veces el tamaño de buffers | Los registros del periférico (`usbhw.c`) |

## El caso más útil: CDC (puerto serie virtual)

La clase CDC hace que tu placa aparezca como un **COM nuevo** en la PC. Desde el micro programás casi
como una UART (módulo 9); desde la PC abrís ese COM con cualquier terminal (PuTTY, `screen`, el monitor
serie). Ventaja enorme sobre la UART real: **no necesitás un adaptador USB-serial**; el mismo cable USB
que programa/alimenta la placa transporta los datos.

### Cómo es un CDC por dentro (los endpoints reales del ejemplo)

Mirando `usbdesc.c` del ejemplo `USBCDC`, el device declara estos endpoints (más allá del EP0 de
control). Esto es lo que el host aprende en la enumeración:

| Endpoint (dirección) | Tipo | Tamaño | Para qué |
|----------------------|------|--------|----------|
| **0x81** (EP1 IN) | Interrupt | 16 bytes | notificaciones de estado de línea (`SERIAL_STATE`) |
| **0x82** (EP2 IN) | Bulk | 64 bytes | datos device → host (lo que la placa "escribe" al COM) |
| **0x02** (EP2 OUT) | Bulk | 64 bytes | datos host → device (lo que la PC "manda" por el COM) |

(En `cdcuser.h`: `CDC_DEP_IN = 0x82`, `CDC_DEP_OUT = 0x02`, `CDC_CEP_IN = 0x81`.) La elección de
EP1 para interrupt y EP2 para bulk no es libre: en el LPC1769 el tipo de cada endpoint está fijado en
el silicio ([página 02](./02-controlador-usb-lpc1769.md)). El device declara
`bDeviceClass = CDC`, `idVendor = 0x1FC9` (NXP), `idProduct = 0x2002`. Si cambiás VID/PID o el nombre
en `usbdesc.c`, la PC lo ve distinto.

### El main del ejemplo

El `main` (de `vcomdemo.c`) es sorprendentemente corto, porque todo el peso está en el stack:

```c
int main (void) {
  VCOM_Init();                    // inicializa la clase CDC (CDC_Init)
  USB_Init();                     // prende el periférico USB (clock, pines, reset)
  USB_Connect(TRUE);              // activa el pull-up (SoftConnect) -> el host empieza a enumerar

  while (!USB_Configuration) ;    // esperar a quedar CONFIGURADO (fin de la enumeración)

  while (1) {                     // bucle principal
    VCOM_Serial2Usb();            // lo que llega por la UART -> mandarlo por USB
    VCOM_CheckSerialState();      // avisar cambios de estado de línea
    VCOM_Usb2Serial();            // lo que llega por USB -> sacarlo por la UART
  }
}
```

Fijate el patrón:

1. `USB_Init()` hace lo de la [página 02](./02-controlador-usb-lpc1769.md): clock a 48 MHz, pines,
   `USB_Reset()`.
2. `USB_Connect(TRUE)` activa SoftConnect (el pull-up de D+). **Recién acá** el host ve el device y
   arranca la enumeración. Por eso conectás *después* de inicializar todo.
3. `while (!USB_Configuration)` espera a que la enumeración termine (el host mandó *Set Configuration*).
   `USB_Configuration` la pone el stack desde la ISR.
4. El bucle solo mueve datos entre la UART física y el USB. **No hay protocolo USB acá**: lo resolvió el
   stack.

## Un ejemplo propio: "eco serial por USB"

El ejemplo de NXP es un *puente UART↔USB*. Lo más didáctico para arrancar es un **eco**: lo que escribís
en el terminal de la PC, la placa te lo devuelve. Así trabajás directo con los endpoints del CDC, sin la
UART física de por medio. La idea es leer del bulk OUT y reescribir en el bulk IN.

```c
#include "LPC17xx.h"
#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "cdc.h"
#include "cdcuser.h"
#include "vcomdemo.h"

int main (void) {
  char buf[USB_CDC_BUFSIZE];     // buffer de hasta 64 bytes (USB_CDC_BUFSIZE = 64)
  int  n;

  VCOM_Init();                   // inicializa la clase CDC
  USB_Init();                    // clock 48 MHz, pines, reset del periferico
  USB_Connect(TRUE);             // SoftConnect: el host empieza a enumerar

  while (!USB_Configuration) ;   // esperar a quedar configurado

  while (1) {
    // 1) ¿Llego algo del host por el endpoint bulk OUT?
    CDC_OutBufAvailChar(&n);     // n = bytes disponibles en el buffer OUT del CDC
    if (n > 0) {
      if (n > USB_CDC_BUFSIZE) n = USB_CDC_BUFSIZE;
      n = CDC_RdOutBuf(buf, &n); // leer del buffer OUT del CDC hacia 'buf'

      // 2) Devolverlo tal cual por el endpoint bulk IN (eco)
      if (CDC_DepInEmpty) {      // el endpoint IN esta libre para transmitir
        CDC_DepInEmpty = 0;      // lo marco ocupado; el stack lo libera al terminar
        USB_WriteEP(CDC_DEP_IN, (unsigned char *)buf, n);
      }
    }
  }
}
```

Qué está pasando, conectándolo con las páginas anteriores:

- `CDC_OutBufAvailChar` / `CDC_RdOutBuf` (de `cdcuser.c`) leen del **buffer del endpoint bulk OUT
  (0x02)**. El stack llenó ese buffer en la ISR cuando llegó el paquete del host (acordate del flujo
  `USB_ReadEP` + `CMD_CLR_BUF` de la página 02).
- `USB_WriteEP(CDC_DEP_IN, ...)` es exactamente la función de la página 02: carga `USBTxData`, pone
  `USBTxPLen` y manda `CMD_VALID_BUF`. El host se lleva el dato en su próximo polling IN.
- `CDC_DepInEmpty` es una bandera (de `cdcuser.c`) que evita pisar un envío que todavía no terminó. El
  callback IN del CDC la vuelve a poner en 1 cuando el endpoint queda libre. Es la forma de respetar la
  asimetría: vos **no podés** forzar el envío, solo dejarlo listo cuando hay lugar.

Compilás, cargás, abrís el COM virtual en la PC con un terminal y lo que tipeás te vuelve.

## Esqueleto de inicialización (lo que hace `USB_Init` por dentro)

Para fijar la conexión con la [página 02](./02-controlador-usb-lpc1769.md), este es el `USB_Init` real
del stack (`usbhw.c`), comentado. No lo escribís vos, pero ahora lo **entendés**:

```c
void USB_Init (void) {
  // 1) Pines: D+/D-, SoftConnect, VBUS, GoodLink
  LPC_PINCON->PINSEL1 &= ~((3<<26)|(3<<28));  LPC_PINCON->PINSEL1 |= ((1<<26)|(1<<28));
  LPC_PINCON->PINSEL3 &= ~((3<< 4)|(3<<28));  LPC_PINCON->PINSEL3 |= ((1<< 4)|(2<<28));
  LPC_PINCON->PINSEL4 &= ~((3<<18));          LPC_PINCON->PINSEL4 |= ((1<<18));

  // 2) Power + clock del periferico (necesita 48 MHz desde la PLL)
  LPC_SC->PCONP |= (1UL<<31);                  // prende el periferico USB
  LPC_USB->USBClkCtrl = 0x1A;                  // Dev + AHB + PortSel
  while ((LPC_USB->USBClkSt & 0x1A) != 0x1A);  // esperar estable

  // 3) Interrupcion y reset logico
  NVIC_EnableIRQ(USB_IRQn);
  USB_Reset();                                 // realiza EP0, habilita interrupciones de device
  USB_SetAddress(0);                           // direccion 0 (la de antes de enumerar)
}
```

## Errores típicos y cómo depurarlos

Esto es lo que el datasheet no te dice y donde vas a perder tiempo si no lo sabés:

- **No aparece nada al enchufar.** Casi siempre **clock USB**: no llega a los 48 MHz exactos, o falta el
  `while ((LPC_USB->USBClkSt & 0x1A) != 0x1A)`. Revisá la PLL (módulo 3) y `system_LPC17xx.c`.
- **"Dispositivo USB no reconocido" (Windows) / enumeración falla.** Descriptores mal armados: el
  clásico es `wTotalLength` del configuration descriptor que no suma todo el árbol, o un
  `bMaxPacketSize0` que no coincide con `USB_MAX_PACKET0` de `usbcfg.h`.
- **Enumera pero no transfiere / se cuelga.** `wMaxPacketSize` de un endpoint en el descriptor distinto
  del tamaño con que se realizó el endpoint (`USBMaxPSize`). Tienen que coincidir, y en full-speed solo
  valen 8/16/32/64.
- **Se transmite a medias o se "traba" un sentido.** Olvidarse de liberar el buffer OUT
  (`CMD_CLR_BUF`) o de validar el IN (`CMD_VALID_BUF`): el endpoint queda en NAK permanente. En el
  ejemplo de eco, eso es no manejar bien `CDC_DepInEmpty`.
- **Conecta antes de estar listo.** Si activás SoftConnect (`USB_Connect`) antes de terminar de
  inicializar, el host empieza a enumerar contra un device a medio configurar. Conectá **al final**.
- **No funciona el COM en la PC.** En Windows viejo, el CDC pide un `.inf` (está el
  `lpc17xx-vcom.inf` en el ejemplo). En Linux/macOS suele andar solo (`/dev/ttyACM0`).

## Por qué no lo hacemos todo a nivel registro

Para que se entienda la decisión: un device USB tiene que responder, **en milisegundos y sin fallar**, a
toda la secuencia de enumeración, manejar varios endpoints, doble buffering, NAKs, resets de bus,
*toggle* de DATA0/DATA1, etc. Implementar eso a mano son miles de líneas y semanas de depuración con
analizador de protocolo. El periférico del Capítulo 11 **existe** y lo recorriste en la página 02, pero
escribir sus registros desde cero para algo funcional **no es realista** en una materia (ni en la
industria: nadie reimplementa USB). Por eso se usa el stack. Lo valioso es **entender el modelo y el
silicio** (páginas 01 y 02): eso es lo que te deja leer el stack, ajustar descriptores y depurar cuando
algo no enumera.

## Ideas de proyecto

1. **CDC eco** (el de arriba): la placa devuelve por USB lo que recibe.
2. **CDC + sensores:** mandá lecturas de ADC/I2C por el COM virtual y graficalas en la PC.
3. **HID teclado:** que la placa "tipee" un texto al apretar un botón (sin drivers en la PC; ejemplo
   `USBHID`).
4. **MSC:** que la placa aparezca como un pendrive (ejemplo `USBMassStorage`).
5. **Cambiar nombre/VID/PID** en `usbdesc.c` y ver cómo aparece distinto en la PC.

## Cierre del curso

Con USB cerrás el recorrido completo: del bit y el registro (módulo 1) hasta un protocolo que requiere
una pila de software entera, un motor de protocolo en hardware (el SIE) y una enumeración de varios
pasos. La misma idea de fondo (capas que esconden complejidad) aparece en todos lados: del `mygpio` que
escribiste en el módulo 2 al stack USB de acá. Esa es, quizás, la lección más transferible de toda la
materia.

---

**Anterior:** [02 - El controlador USB del LPC1769](./02-controlador-usb-lpc1769.md) ·
**Módulo:** [USB](./README.md) ·
**Volver al** [índice del curso](../README.md)
