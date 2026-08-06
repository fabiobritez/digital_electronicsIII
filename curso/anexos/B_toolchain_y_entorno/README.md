# Anexo B: Toolchain y entorno propio (sin MCUXpresso)

> **Es un [anexo](../README.md): opcional.** Para cursar la materia con MCUXpresso no hace falta nada
> de acá. Armarte tu propio entorno es una decisión tuya: cuesta un rato al principio y después te
> sirve para cualquier micro ARM, para automatizar y para no depender de que el IDE ande.

MCUXpresso es cómodo, pero es una **caja cerrada**: te esconde el compilador, el build y
el grabado. Este módulo te muestra qué hay adentro y cómo armar **tu propio entorno**
(editor + toolchain + grabador) para compilar y quemar el LPC1769 sin depender de ningún
IDE en particular. Es liberador: el mismo flujo te sirve para cualquier micro ARM y para
automatizar (CI, scripts, etc.).

De hecho, todo el código de este curso se compiló y linkeó así, sin MCUXpresso, con el
toolchain que vas a conocer acá.

## Lo primero: la plantilla

En [`plantilla/`](../../../plantilla/) hay un proyecto **listo para usar**: copialo, escribí
tu código en `src/` y andá.

```bash
cd plantilla
make            # compila  -> build/firmware.elf, .bin, .hex
make flash      # graba la placa
make debug      # graba y abre gdb, parado en main
make help       # todos los comandos
```

Adentro están, escritas y comentadas línea por línea, las piezas que este módulo explica:
el `Makefile`, el linker script, el startup, la configuración de OpenOCD y la de los
editores. No es una caja negra más: son seis archivos de texto que podés leer enteros.

## Recorrido

0. [00 - El camino completo: de `main.c` a un LED parpadeando](./00-el-camino-completo.md)
   **Empezá acá.** El mapa entero de las once piezas que intervienen, primero en general
   y después en el LPC1769. Incluye el detalle del checksum de la boot ROM, que es la
   causa número uno del "grabé y no hace nada".
1. [01 - ¿Qué es un toolchain?](./01-que-es-un-toolchain.md)
   Qué es `arm-none-eabi-gcc`, las piezas (compilador, ensamblador, linker, binutils,
   libc, gdb), y qué significa "compilación cruzada".
2. [02 - Setup en VSCode](./02-setup-vscode.md)
   Editor + extensiones + IntelliSense + tarea de compilación, con archivos de
   configuración listos para copiar. También cómo tener lo mismo en vim o neovim.
3. [03 - Quemar la placa](./03-quemar-la-placa.md)
   Los dos caminos para grabar: **debug probe** (SWD) y bootloader serial (ISP). Cómo
   funciona cada uno por dentro.
4. [04 - Adentro de la carpeta del toolchain](./04-adentro-del-toolchain.md)
   Qué es cada archivo de un toolchain GCC: `bin`, `libexec`, el sysroot, el multilib,
   `libgcc`, los `.specs`, y qué agregó NXP encima (Redlib).
5. [05 - Cómo compila y graba MCUXpresso](./05-como-compila-y-graba-mcuxpresso.md)
   El build administrado de Eclipse, los Makefiles generados, los linker scripts con
   plantillas FreeMarker, y la cadena de grabado: gdb, `crt_emu_cm_redlink`, `redlinkserv`
   y los drivers de Flash `.cfx`.
6. [06 - Instalación completa en Linux](./06-instalacion-linux.md)
   Desde una máquina recién instalada hasta `make flash` funcionando. Incluye los permisos
   de USB, que es el paso que todos se saltean.
7. [07 - Instalación completa en Windows](./07-instalacion-windows.md)
   Los tres caminos (MSYS2, instaladores oficiales, WSL2), drivers y puertos COM.
8. [08 - El primer grabado, verificado en la placa](./08-primer-grabado-verificado.md)
   Una sesión real de punta a punta, con la placa enchufada: los chequeos que hay que
   hacer **antes** de grabar, cómo confirmar después que el programa está corriendo de
   verdad, y los errores concretos que aparecieron en el camino.

## Guía por debug probe

Compilar es igual en todas las máquinas. **Grabar depende del hardware que tengas entre la
PC y el micro.** En [`probes/`](./probes/) hay una guía por cada uno:

| Tu debug probe | Guía |
|----------|------|
| CMSIS-DAP (LPCXpresso rev D, **la de la cátedra**) | [01](./probes/01-cmsis-dap.md) |
| LPC-Link original (LPCXpresso viejas) | [02](./probes/02-lpc-link-original.md) |
| LPC-Link2 / MCU-Link | [03](./probes/03-lpc-link2-y-mcu-link.md) |
| J-Link | [04](./probes/04-jlink.md) |
| ST-Link y otras | [05](./probes/05-otros-probes.md) |
| Ninguna: grabar por el puerto serie | [06](./probes/06-sin-probe-isp.md) |

Si no sabés cuál tenés, el [índice de probes](./probes/README.md) arranca con cómo
identificarla.

## Instalar el toolchain

```bash
bash tools/install_toolchain.sh              # el paquete del repo, sin red ni sudo
bash tools/install_toolchain.sh --mcuxpresso # enlaza el de MCUXpresso, si ya lo tenés
bash tools/install_toolchain.sh --xpack      # baja el de xPack (incluye gdb)
```

Los tres dejan el compilador en `tools/toolchain/`, que es donde lo busca la plantilla.

## Antes de esto

Módulos [16 (build, linker, startup)](../A_build_linker_startup/) y
[12 (debug)](../../12_debug/). Este módulo es la versión "hazlo vos mismo" de ambos.

## Alcance

**Compilar y linkear** lo podés hacer en cualquier PC, sin hardware. **Grabar** y
**depurar en la placa** necesitan el LPC1769 enchufado.

El camino completo, de `main.c` al LED parpadeando, está **probado en la placa de la
cátedra**: la sesión entera, con las salidas reales de cada comando y los errores que
aparecieron, está en la [página 08](./08-primer-grabado-verificado.md).

---

**Anexos:** [índice](../README.md) · **El otro anexo:** [16 - Build, linker y startup](../A_build_linker_startup/) ·
**Volver al** [mapa del curso](../../README.md)
