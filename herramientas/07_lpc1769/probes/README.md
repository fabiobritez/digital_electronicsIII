# Guía por sonda, para el LPC1769

El proceso de compilación puede mantenerse independiente de la sonda. La grabación y el debug sí
dependen de la interfaz conectada, su firmware y el servidor del host.

Qué es una sonda, cómo funciona y qué tipos existen está explicado en
[03 - Debug probes](../../03_debug_probes/). Esta carpeta es lo práctico: **qué escribir**
según cuál te haya tocado con un LPC1769.

> **Si tu placa es la LPCXpresso rev D (OM13085), la de la cátedra, no estás en el lugar
> correcto:** andá a [07-01 - La placa y su sonda](../01-la-placa-y-su-sonda.md), que es la
> página dedicada a esa.

---

## Identificá la tuya

```bash
lsusb                 # Linux
# Windows: Administrador de dispositivos, o  pyocd list
```

| Lo que ves | Tu sonda es | Guía |
|---|---|---|
| `CMSIS-DAP`, `LPC11U3x CMSIS-DAP`, ID `1fc9:001d` | **CMSIS-DAP de a bordo** (la de la cátedra) | [07-01](../01-la-placa-y-su-sonda.md) |
| ID `0471:df55`, "LPC-Link", "NXP LPCXpresso" | **LPC-Link original** | [01](./01-lpc-link-original.md) |
| `LPC-Link2`, `MCU-Link` o VID NXP con descriptores afines | **posible LPC-Link2 / MCU-Link** | [02](./02-lpc-link2-y-mcu-link.md) |
| `SEGGER J-Link`, VID `1366` | **J-Link físico u on-board** | [03](./03-jlink.md) |
| `ST-Link`, un módulo FTDI o una Pico | **otras sondas; confirmá modelo y firmware** | [04](./04-otros-probes.md) |
| Nada, o solo un adaptador USB-serie | **no tenés sonda** | [05](./05-sin-probe-isp.md) |

Si no aparece nada al enchufar, revisá primero el cable: los cables USB de cargador de celular
muchas veces tienen **solo los dos hilos de alimentación** y ningún hilo de datos. La placa se
enciende, pero el host no detecta ningún dispositivo.

---

## La tabla de decisión

| Sonda | Protocolo | openocd | pyocd | LinkServer | ¿Depura? | Dónde aparece |
|---|---|:---:|:---:|:---:|:---:|---|
| [CMSIS-DAP de a bordo](../01-la-placa-y-su-sonda.md) | CMSIS-DAP HID | sí | sí | falló en la versión probada | sí | LPCXpresso OM13085 rev. D |
| [LPC-Link original](./01-lpc-link-original.md) | propietario NXP | no | no | depende de versión compatible | sí | LPCXpresso iniciales |
| [LPC-Link2](./02-lpc-link2-y-mcu-link.md) | según firmware | según firmware | según firmware | según firmware | sí | LPCXpresso V2/V3 o separada |
| [MCU-Link](./02-lpc-link2-y-mcu-link.md) | CMSIS-DAP o J-Link en modelos admitidos | según firmware | según firmware | sí con firmware admitido | sí | sondas y placas NXP |
| [J-Link](./03-jlink.md) | propietario SEGGER | sí | mediante plugin compatible | no | sí | física u on-board |
| [ST-Link](./04-otros-probes.md) | propietario ST | sí | mediante plugin | no | sí | externa o integrada en placa ST |
| [ISP serial](./05-sin-probe-isp.md) | bootloader del chip | n/a | n/a | n/a | **no** | cualquier adaptador USB-serie |

CMSIS-DAP permite elegir entre varios servidores, siempre que éstos admitan el target y el
transporte USB de la sonda. Con un protocolo propietario suele hacer falta el software del
fabricante, un plugin específico o una interfaz distinta.

---

## Lo que no cambia, sea cual sea

- **El código, el `Makefile`, el linker script y el startup.** La sonda no interviene en la
  compilación.
- **La interfaz de la plantilla:** `make flash`. Detecta herramientas instaladas, pero vos todavía
  debés confirmar que la elegida soporte la sonda y el target. Se puede forzar, por ejemplo, con
  `make flash FLASHER=pyocd`.
- **El cliente de debug:** habitualmente GDB habla con un servidor por TCP; algunas sondas, como
  Black Magic Probe, exponen el protocolo remoto por un puerto serie.

---

## En Linux, antes que nada: los permisos

El error más común no tiene nada que ver con la sonda:

```
Error: unable to find a matching CMSIS-DAP device
Error: libusb_open() failed with LIBUSB_ERROR_ACCESS
```

En Linux, el nodo USB puede crearse con permisos que no permiten abrirlo como usuario normal.
Ejecutar OpenOCD como `root` sirve como prueba diagnóstica, pero la solución permanente debe
permitir el acceso desde el editor y la terminal sin elevar privilegios.

La solución correcta es una regla de udev, que ya viene en la plantilla:

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Y después **desenchufá y volvé a enchufar la placa**: las reglas se aplican al conectar, no a
lo que ya estaba enchufado.

---

**LPC1769:** [índice](../README.md) ·
**Ver también:** [03 - Debug probes](../../03_debug_probes/) ·
[05 - Grabar y depurar](../05-grabar-y-depurar.md)
