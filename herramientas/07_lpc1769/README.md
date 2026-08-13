# 07 - LPC1769: todo lo anterior, aplicado

Las partes 01 a 06 presentan principios transferibles, con especial atención a Arm Cortex-M. Esta
parte los aplica a la placa de la cátedra: LPC1769, OM13085 y las herramientas verificadas para ese
montaje.

Si venís a resolver un problema puntual, empezá por la tabla de abajo. Si venís a entender,
leé primero las partes anteriores.

## Recorrido

| # | Página | De qué trata |
|---|--------|--------------|
| 01 | [La placa y su sonda](./01-la-placa-y-su-sonda.md) | La OM13085 rev. D, su LPC11U35, el transporte CMSIS-DAP HID y las capacidades medidas de ese firmware |
| 02 | [La boot ROM, el ISP y el checksum](./02-la-boot-rom-el-isp-y-el-checksum.md) | Cómo la ROM decide el arranque, checksum del vector 7, CRP y verificaciones previas |
| 03 | [Instalación en Linux](./03-instalacion-linux.md) | Toolchain, GDB, servidor de debug, permisos USB y comprobación del entorno |
| 04 | [Instalación en Windows](./04-instalacion-windows.md) | MSYS2, binarios independientes o WSL2; shell, drivers y puertos COM |
| 05 | [Grabar y depurar](./05-grabar-y-depurar.md) | Los dos caminos (sonda SWD y bootloader serie) con los comandos exactos para este chip, y qué hace F5 por dentro |
| 06 | [El primer grabado, verificado en la placa](./06-primer-grabado-verificado.md) | Una sesión real de punta a punta, con la placa enchufada: los chequeos previos, cómo confirmar que el programa corre de verdad, y los errores concretos que aparecieron |
| 07 | [MCUXpresso por dentro](./07-mcuxpresso-por-dentro.md) | Los Makefiles que genera Eclipse, los linker scripts con plantillas FreeMarker, y la cadena de grabado con `redlinkserv` y los drivers `.cfx` |
| ↳ | [Guía por sonda](./probes/) | Una página por cada sonda que te puede tocar con un LPC1769 |

---

## El atajo, si tenés apuro

```bash
bash tools/install_toolchain.sh                 # una sola vez
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
# desenchufar y volver a enchufar la placa

cd plantilla
make            # compila
make flash      # chequea, graba y resetea
make debug      # graba y abre gdb, parado en main
make rtt        # consola por el cable del debugger
```

Si algo falla, la [página 06](./06-primer-grabado-verificado.md) conserva una sesión real y una tabla
de síntomas. Usala como referencia, no como sustituto de leer el mensaje de tu propia versión.

---

## Lo que hace especial a este chip

Cuatro rasgos del LPC1769 y de esta placa que condicionan el flujo:

**1. Checksum en el vector 7.** La boot ROM exige que la suma de las primeras ocho palabras sea cero.
Si no encuentra una imagen válida, inicia el autobaud de ISP
([página 02](./02-la-boot-rom-el-isp-y-el-checksum.md)).

**2. CRP en `0x000002FC`.** Tres patrones activan CRP1, CRP2 o CRP3. Todos deshabilitan debug y
restringen ISP; CRP3 puede dejar el equipo sin una ruta práctica de actualización si la aplicación
no implementa una.

**3. Sonda integrada CMSIS-DAP por HID.** El firmware probado no implementa SWO, el descriptor de
serie está vacío y con RTT se midió un techo cercano a 15,7 kB/s. Son propiedades de esa
implementación, no de todo CMSIS-DAP v1. La versión de LinkServer ensayada falló con el serial vacío
([página 01](./01-la-placa-y-su-sonda.md)).

**4. Los pines de depuración son dedicados.** En el LPC176x, TDO/SWO, TDI, TMS/SWDIO, TRST y
TCK/SWDCLK son los pines 1 a 5 del encapsulado, no pines de propósito general. No los busques
entre los `P0.x` o `P1.x`: `P1.16` y `P1.17` son Ethernet.

---

## Datos de la placa

| | |
|---|---|
| **Micro** | LPC1769, ARM Cortex-M3, hasta 120 MHz (la placa de la cátedra corre a 100 MHz) |
| **Memoria** | 512 KB de FLASH en `0x00000000`, 64 KB de RAM (32 KB en `0x10000000` más dos bancos AHB de 16 KB) |
| **Placa** | LPCXpresso LPC1769 rev D, código de NXP **OM13085** |
| **Sonda de a bordo** | LPC11U35 con firmware CMSIS-DAP v1, ID USB `1fc9:001d` |
| **Cristal** | 12 MHz |
| **Conector de depuración** | Cortex Debug de 10 pines, paso 1.27 mm |
| **Entrada a ISP** | P2.10 en bajo durante el reset |
| **Boot ROM** | 8 KB en `0x1FFF0000`, no borrable |

---

## Manual

Los capítulos que corresponden a esta parte:

| Capítulo | Tema |
|---|---|
| [2](../../manual/ch02_memory-map.pdf) | mapa de memoria y dónde vive la boot ROM |
| [32](../../manual/ch32_flash-memory-interface-and-programming.pdf) | el bootloader ISP, las rutinas IAP, los sectores de FLASH y el checksum |
| [33](../../manual/ch33_jtag-serial-wire-debug-and-trace.pdf) | JTAG, SWD y traza |
| [34](../../manual/ch34_appendix-cortex-m3-user-guide.pdf) | guía de usuario del Cortex-M3: excepciones, faults, registros de depuración |

El índice completo con todos los capítulos está en
[`manual/INDEX.md`](../../manual/INDEX.md).

---

**Unidad:** [Herramientas](../README.md) ·
**Anterior:** [06 - Depurar en serio](../06_depurar_en_serio/) ·
**El proyecto listo para usar:** [`plantilla/`](../../plantilla/)
