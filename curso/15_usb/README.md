# Módulo 15: USB (plus)

El **USB** (Universal Serial Bus) es lo que conecta tu LPC1769 a una PC y hace que aparezca como un
dispositivo: un puerto serie virtual, un teclado/mouse, un pendrive, una placa de audio. Es, por
lejos, el periférico más **complejo** del chip, y por eso este módulo es distinto a los demás.

> **Aviso honesto de alcance.** A diferencia de GPIO o UART, en la práctica **no se reimplementa el
> protocolo USB desde cero a nivel registro.** Es tan grande (enumeración, descriptores, clases,
> *endpoints*, manejo de errores en milisegundos) que hasta los profesionales usan una **pila de
> software** (*stack*) ya hecha; el repo trae la de NXP. Aun así, este módulo **sí** te muestra el
> periférico a nivel registro (página 02: el SIE, la EP RAM, los comandos, el clocking), porque
> entender el silicio es lo que te deja **leer el stack y depurar** cuando algo no enumera. Lo que no
> hacemos es escribir esos registros uno por uno para algo funcional: para eso está el stack.

## Recorrido

1. [01 - Cómo funciona el USB](./01-usb-conceptos.md)
   Host vs device, full-speed, el árbol de transferencias (control/bulk/interrupt/iso), *endpoints* y el
   endpoint 0, la **enumeración paso a paso**, los **descriptores** y las clases (CDC, HID, MSC).
2. [02 - El controlador USB del LPC1769](./02-controlador-usb-lpc1769.md)
   El silicio a nivel registro: el **SIE** y su protocolo de comandos, endpoints físicos/lógicos y la
   **EP RAM**, el clocking a 48 MHz, los pines (D+/D−, SoftConnect, VBUS), las interrupciones y el
   modo Slave vs DMA.
3. [03 - USB en la práctica: el stack de NXP](./03-usb-en-la-practica.md)
   Cómo está armado el stack en capas, dónde engancha tu código (callbacks, descriptores), un **CDC de
   eco** completo y los errores típicos de enumeración.

## Por qué vale la pena, aunque sea complejo
- **CDC (puerto serie virtual):** que tu placa aparezca como un COM en la PC, **sin** adaptador
  USB-serial. Comunicación directa por USB.
- **HID:** que la placa se haga pasar por un teclado/mouse/joystick (no necesita drivers en la PC).
- **MSC:** que la placa aparezca como un pendrive.

## Antes de esto
Conviene tener todo el curso, sobre todo el módulo 3 (clock: el USB necesita exactamente 48 MHz) y
9 (UART, porque el CDC es "una UART por USB"). El concepto de *interrupciones* (módulo 7) también es
central: el USB es muy basado en interrupciones.

## Manual y ejemplos
USB device: Capítulo 11 ([`manual/ch11_usb-device-controller.pdf`](../../manual/ch11_usb-device-controller.pdf)).
Stack y ejemplos listos en [`../../library/examples/USBDEV/`](../../library/examples/USBDEV/):
`USBCDC`, `USBHID`, `USBMassStorage`, `USBAudio`.

---

**Anterior:** [14 - SPI](../14_spi/) · **Siguiente:** [16 - PWM](../16_pwm/)
