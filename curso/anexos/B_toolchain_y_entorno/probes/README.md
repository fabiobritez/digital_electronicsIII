# Debug probes: una guía por cada uno

Compilar es igual en todas las máquinas del mundo. **Grabar la placa, no**: depende de qué
hardware tenés entre la PC y el micro. Ese hardware es el *debug probe*
(o sonda de depuración), y es la única pieza del stack que cambia según la placa que te toque.

Esta carpeta tiene una guía por cada probe que te podés encontrar con un LPC1769. Empezá
por identificar el tuyo.

## Qué es un debug probe y por qué existe

El LPC1769 no se puede grabar por USB directamente: no tiene un periférico USB que hable
un protocolo de grabación. Lo que sí tiene son dos pines de depuración, **SWDIO** y
**SWCLK**, que dan acceso directo al núcleo Cortex-M3: leer y escribir memoria, parar el
CPU, poner breakpoints, leer registros.

Un probe es un aparatito que traduce entre el USB de tu PC y esos dos pines:

```
   PC  ──USB──►  probe  ──SWD (2 pines)──►  LPC1769
                   │
        openocd / pyocd / LinkServer
```

En las placas de desarrollo el probe viene **soldado en la misma placa**, en un rincón, y
suele haber una línea de corte o unos jumpers que la separan del micro. Por eso enchufás
un solo cable USB y funciona: adentro hay dos microcontroladores, no uno.

La distinción que importa para todo lo que sigue:

- **CMSIS-DAP** es un **estándar abierto de ARM**. Cualquier probe que lo hable funciona
  con openocd, pyocd, Keil, IAR y todo lo demás. Es lo que querés.
- El resto son **protocolos propietarios**, y cada uno necesita el software de su
  fabricante.

## Identificá tu probe

Enchufá la placa y corré:

```bash
lsusb                 # Linux
# Windows: Administrador de dispositivos, o  pyocd list
```

Buscá tu placa en la lista y cruzá el resultado con esta tabla:

| Lo que ves | Tu debug probe es | Guía |
|------------|-------------|------|
| `CMSIS-DAP`, `LPC11U3x CMSIS-DAP`, ID `1fc9:...` | **CMSIS-DAP** (la de la cátedra) | [01](./01-cmsis-dap.md) |
| ID `0471:df55`, "LPC-Link", "NXP LPCXpresso" | **LPC-Link original** | [02](./02-lpc-link-original.md) |
| `LPC-Link2`, `MCU-Link`, ID `1fc9:0090` o `1fc9:0143` | **LPC-Link2 / MCU-Link** | [03](./03-lpc-link2-y-mcu-link.md) |
| `SEGGER J-Link`, ID `1366:...` | **J-Link** | [04](./04-jlink.md) |
| `ST-Link`, ID `0483:374b`, o un FT2232 | **otros probes** | [05](./05-otros-probes.md) |
| Nada, o solo un adaptador USB-serial | **no tenés probe** | [06](./06-sin-probe-isp.md) |

Si no aparece nada al enchufar, revisá primero el cable: los cables USB de cargador de
celular muchas veces tienen solo los dos hilos de alimentación y ningún hilo de datos. La
placa se enciende pero no la ve nadie.

## La tabla completa

| Probe | Protocolo | openocd | pyocd | LinkServer | ¿Depura? | Dónde aparece |
|-------|-----------|:-------:|:-----:|:----------:|:--------:|---------------|
| [CMSIS-DAP](./01-cmsis-dap.md) | abierto (ARM) | sí | sí | sí | sí | LPCXpresso OM13085 rev D, la de la cátedra |
| [LPC-Link original](./02-lpc-link-original.md) | propietario NXP | **no** | **no** | sí | sí | LPCXpresso viejas (2010-2013) |
| [LPC-Link2](./03-lpc-link2-y-mcu-link.md) | según firmware | sí | sí | sí | sí | LPCXpresso V2/V3, probe suelto |
| [MCU-Link](./03-lpc-link2-y-mcu-link.md) | según firmware | sí | sí | sí | sí | placas NXP nuevas |
| [J-Link](./04-jlink.md) | propietario SEGGER | sí | no | no | sí | probe comprado aparte |
| [ST-Link](./05-otros-probes.md) | propietario ST | sí (parcial) | sí | no | sí | reciclado de una Nucleo/Discovery |
| [ISP serial](./06-sin-probe-isp.md) | bootloader del chip | n/a | n/a | n/a | **no** | cualquier adaptador USB-serial |

La conclusión práctica: **si tu probe habla CMSIS-DAP, no necesitás una sola línea de
software de NXP**. Si no, o usás la herramienta del fabricante, o le cambiás el firmware,
o grabás por el puerto serie.

## Lo que no cambia

Sea cual sea tu probe, esto queda igual:

- El código, el `Makefile`, el linker script y el startup. El probe no interviene en la
  compilación.
- El comando: `make flash`. La [plantilla](../../../../plantilla/) detecta qué grabador
  tenés instalado y usa el que corresponda. También podés forzarlo:
  `make flash FLASHER=pyocd`.
- El depurado: siempre termina siendo gdb conectado a un servidor por el puerto 3333. Lo
  único que cambia es quién levanta ese servidor.

Por eso vale la pena entender la separación. El probe es un detalle de tu escritorio, no
de tu proyecto.

## En Linux, antes que nada: los permisos

El error más común no tiene nada que ver con el probe:

```
Error: unable to find a matching CMSIS-DAP device
Error: libusb_open() failed with LIBUSB_ERROR_ACCESS
```

En Linux, un dispositivo USB recién enchufado pertenece a `root`. La tentación es correr
`sudo openocd`, y funciona, pero después F5 en VSCode no anda (VSCode no corre como root)
y te llenás el proyecto de archivos de root.

La solución correcta es una regla de udev, que ya viene en la plantilla:

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Y después **desenchufá y volvé a enchufar la placa**: las reglas se aplican al conectar.

---

**Módulo:** [18 - Toolchain y entorno](../README.md) ·
**Ver también:** [03 - Quemar la placa](../03-quemar-la-placa.md)
