# Instalación completa en Linux

Todo lo que hace falta para compilar, grabar y depurar el LPC1769, desde una máquina
recién instalada. Los comandos son de Ubuntu y Debian; al final están los equivalentes de
otras distribuciones.

## Resumen

```bash
# 1. Compilador (o usá el del repo: bash tools/install_toolchain.sh)
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi

# 2. Depurador
sudo apt install gdb-multiarch

# 3. Grabador
sudo apt install openocd

# 4. Lo demás
sudo apt install make git

# 5. Permisos de USB
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG dialout $USER          # y cerrá sesión y volvé a entrar
```

Después, para verificar:

```bash
cd plantilla
make info
make
```

El resto de la página explica qué hace cada cosa y qué hacer si algo falla.

## 1. El compilador

Hay dos caminos y los dos terminan igual.

### Opción A: el toolchain del repo (sin sudo, sin internet)

El repositorio trae empaquetado el compilador, recortado a lo mínimo para Cortex-M3
(35 MB comprimidos en vez de 1.4 GB):

```bash
bash tools/install_toolchain.sh
```

Deja todo en `tools/toolchain/`, no toca el sistema y no pide permisos de administrador.
El script verifica el SHA256 del paquete y, antes de darse por satisfecho, **compila y
linkea de verdad** un programa de prueba para Cortex-M3.

Es lo que usa la [plantilla](../../../plantilla/) por defecto: si detecta
`tools/toolchain/`, lo prefiere sobre el del sistema. Y es la opción recomendada para el
laboratorio, porque no depende de que ninguna URL siga viva ni de tener permisos de
administrador en las máquinas de la facultad.

Detalles en [`tools/README.md`](../../../tools/README.md).

### Opción B: el del sistema

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
```

En Ubuntu 24.04 esto instala la versión 13.2.rel1, la misma que trae el repo.

Los tres paquetes hacen falta: `gcc-arm-none-eabi` es el compilador,
`binutils-arm-none-eabi` trae el linker, `objcopy` y `size`, y
`libnewlib-arm-none-eabi` la biblioteca estándar de C junto con los archivos `.specs` que
necesita `--specs=nano.specs`. Si te falta el último, el error es
`cannot open linker script file nano.specs`.

Para usar este en vez del del repo:

```bash
make CROSS=arm-none-eabi-
```

Verificá:

```bash
arm-none-eabi-gcc --version
```

## 2. El depurador

El paquete del repo **no incluye gdb**: son 173 MB, y la build que distribuye NXP está
enlazada contra librerías que ya no existen en Ubuntu moderno. Se instala aparte:

```bash
sudo apt install gdb-multiarch
```

`gdb-multiarch` es un gdb que entiende muchas arquitecturas, ARM entre ellas. Funciona
igual que un `arm-none-eabi-gdb`. La plantilla lo detecta sola.

Si preferís el `arm-none-eabi-gdb` propiamente dicho, viene en el
[toolchain de xPack](https://xpack-dev-tools.github.io/arm-none-eabi-gcc-xpack/):

```bash
bash tools/install_toolchain.sh --xpack
```

## 3. El grabador

Elegí según tu debug probe (la guía completa está en [`probes/`](./probes/)). Para la placa de
la cátedra, cualquiera de los dos:

### OpenOCD

```bash
sudo apt install openocd
```

Ubuntu 24.04 trae la 0.12.0, que soporta CMSIS-DAP sin problemas.

### pyOCD

```bash
sudo apt install python3-pyocd
```

O, para tener la última versión, en un entorno virtual:

```bash
python3 -m venv ~/.venvs/pyocd
~/.venvs/pyocd/bin/pip install pyocd hidapi
echo 'export PATH="$HOME/.venvs/pyocd/bin:$PATH"' >> ~/.bashrc
```

> No corras `pip install` sin un entorno virtual: Ubuntu 24.04 lo bloquea a propósito
> (error `externally-managed-environment`) para que no rompas los paquetes del sistema.

El `hidapi` de ahí arriba no es opcional para la placa de la cátedra: su probe es
CMSIS-DAP **v1**, que se comunica por HID, y sin ese módulo pyocd no detecta ninguna sonda
v1 **y no te dice por qué**. Simplemente informa que no hay nada conectado.

### Grabar por el puerto serie, sin debug probe

```bash
sudo apt install lpc21isp
```

## 4. Permisos de USB (el paso que todos se saltean)

En Linux, un dispositivo USB recién enchufado pertenece a `root`. Sin esto, openocd y
pyocd fallan con:

```
Error: unable to find a matching CMSIS-DAP device
Error: libusb_open() failed with LIBUSB_ERROR_ACCESS
```

La tentación es `sudo openocd`. **No lo hagas**: te va a andar en la terminal y te va a
fallar en VSCode, que no corre como root.

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Y **desenchufá y volvé a enchufar la placa**: las reglas se aplican al conectar el
dispositivo, no a los que ya estaban.

Para comprobar que surtieron efecto, mirá el dueño del nodo USB antes y después. El número
sale de `lsusb` (`Bus 001 Device 034` es `/dev/bus/usb/001/034`):

| Antes | Después |
|---|---|
| `crw-rw-r-- root root` | `crw-rw----+ root plugdev` |

Vale la pena saber **cómo se manifiesta** que falta esto, porque no siempre es un error de
permisos legible. Las bibliotecas de USB necesitan abrir el dispositivo para leerle el
nombre; sin permiso de escritura lo devuelven vacío, y entonces pyocd (que filtra buscando
la cadena `CMSIS-DAP`) descarta la sonda **en silencio** e informa que no hay ninguna
conectada. Se pierde mucho tiempo buscando el problema en el lugar equivocado.

Para el puerto serie (grabar por ISP, o leer la UART):

```bash
sudo usermod -aG dialout $USER
```

Esto **requiere cerrar sesión y volver a entrar**. No alcanza con abrir otra terminal: los
grupos se leen al iniciar sesión. Verificá con `groups | grep dialout`.

## 5. El editor

### VSCode

```bash
sudo snap install code --classic
```

Abrí la carpeta `plantilla/` y aceptá las extensiones que ofrece (están declaradas en
[`.vscode/extensions.json`](../../../plantilla/.vscode/extensions.json)). Con eso,
`Ctrl+Shift+B` compila y `F5` graba y depura.

### vim, neovim, helix, emacs

```bash
sudo apt install clangd
cd plantilla && make compile_commands.json
```

Y configurá clangd como servidor de C en tu editor. Con eso tenés el mismo autocompletado
y la misma navegación que VSCode. La configuración de clangd ya está en
[`.clangd`](../../../plantilla/.clangd).

## Verificación final

```bash
cd plantilla
make info
```

Deberías ver algo así:

```
Configuracion actual
  compilador  : arm-none-eabi-gcc (Arm GNU Toolchain 13.2.rel1 ...) 13.2.1 20231009
  ruta        : ../tools/toolchain/bin/arm-none-eabi-gcc
  gdb         : /usr/bin/gdb-multiarch
  python      : /usr/bin/python3
  CMSIS       : no (bare metal)
  grabador    : openocd
  detectados  : openocd pyocd
```

Y ahora, la prueba de verdad:

```bash
make            # tiene que compilar sin warnings
make preflight  # chequea que el firmware vaya a arrancar, sin la placa
make flash      # con la placa enchufada: el LED tiene que parpadear
```

`make preflight` es el que te ahorra el rato de "grabé, dijo OK y no hace nada": verifica
el checksum de la boot ROM, el stack pointer inicial, el bit Thumb del `Reset_Handler`, la
palabra de CRP y el tamaño. `make flash` lo corre solo, y si algo falla no graba.

El recorrido completo, hecho y verificado sobre la placa, está en la
[página 08](./08-primer-grabado-verificado.md).

## Problemas típicos

| Error | Causa |
|-------|-------|
| `arm-none-eabi-gcc: command not found` | falta el compilador, o no está en el PATH |
| `cannot open linker script file nano.specs` | falta `libnewlib-arm-none-eabi` |
| `unable to find a matching CMSIS-DAP device` | faltan las reglas de udev, o no reenchufaste la placa |
| `LIBUSB_ERROR_ACCESS` | lo mismo |
| `could not read product string ... timed out` | la sonda quedó trabada: reenchufá el cable USB |
| `No available debug probes are connected` (pyocd) | falta `hidapi`, o faltan las reglas de udev |
| `Ee(E1). Probe serial number not found` | LinkServer con una sonda sin serial: usá OpenOCD |
| `Permission denied: /dev/ttyUSB0` | no estás en el grupo `dialout`, o no cerraste sesión |
| `externally-managed-environment` al usar pip | usá un entorno virtual |
| `make: command not found` | `sudo apt install make` |
| El grabado funciona pero la placa no arranca | el checksum del vector 7: `make preflight` |

## Otras distribuciones

**Fedora / RHEL**

```bash
sudo dnf install arm-none-eabi-gcc-cs arm-none-eabi-newlib openocd gdb make
```

**Arch / Manjaro**

```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-gdb openocd make
```

**openSUSE**

```bash
sudo zypper install cross-arm-none-gcc13 cross-arm-none-newlib-devel openocd gdb make
```

En todas, el toolchain del repo (`bash tools/install_toolchain.sh`) funciona igual y
evita tener que averiguar los nombres de los paquetes.

---

**Anterior:** [05 - Cómo compila y graba MCUXpresso](./05-como-compila-y-graba-mcuxpresso.md) ·
**Siguiente:** [07 - Instalación en Windows](./07-instalacion-windows.md) ·
**Módulo:** [18](./README.md)
