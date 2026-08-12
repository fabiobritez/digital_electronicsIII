# Instalación completa en Linux

Esta página prepara una máquina Linux para compilar, grabar y depurar el LPC1769. Los comandos
principales se probaron en Ubuntu 24.04; en otra versión o distribución pueden cambiar los nombres
de los paquetes.

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

Hay dos caminos. Ambos exponen herramientas con el prefijo `arm-none-eabi-`, pero no
necesariamente la misma versión ni el mismo conjunto de bibliotecas.

### Opción A: el toolchain del repo (sin sudo, sin internet)

El repositorio trae empaquetado el compilador, recortado a lo mínimo para Cortex-M3
(35 MB comprimidos en vez de 1.4 GB):

```bash
bash tools/install_toolchain.sh
```

Deja todo en `tools/toolchain/`, no toca el sistema y no pide permisos de administrador.
El script verifica el SHA256 del paquete y, antes de darse por satisfecho, **compila y
linkea de verdad** un programa de prueba para Cortex-M3.

Es lo que usa la [plantilla](../../plantilla/) por defecto: si detecta
`tools/toolchain/`, lo prefiere sobre el del sistema. Y es la opción recomendada para el
laboratorio, porque no depende de que ninguna URL siga viva ni de tener permisos de
administrador en las máquinas de la facultad.

Detalles en [`tools/README.md`](../../tools/README.md).

### Opción B: el del sistema

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
```

En la instalación de Ubuntu 24.04 usada para la guía se obtuvo 13.2.Rel1, la misma versión base del
paquete del repositorio. Comprobá la versión real de tu mirror: puede actualizarse.

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

`gdb-multiarch` entiende distintas arquitecturas, incluida Arm, y puede conectarse al servidor GDB
de OpenOCD. No es el mismo ejecutable que `arm-none-eabi-gdb`, pero cubre el flujo de esta
plantilla, que lo detecta automáticamente.

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

La instalación probada aportó OpenOCD 0.12.0 con soporte CMSIS-DAP. Verificá tu paquete con
`openocd --version` y una conexión real: el número de versión por sí solo no confirma los drivers
con los que fue compilado.

### pyOCD

```bash
sudo apt install python3-pyocd
```

O, para tener la última versión, en un entorno virtual:

```bash
python3 -m venv ~/.venvs/pyocd
~/.venvs/pyocd/bin/python -m pip install pyocd hidapi
~/.venvs/pyocd/bin/pyocd --version
```

> No corras `pip install` sin un entorno virtual: Ubuntu 24.04 lo bloquea a propósito
> (error `externally-managed-environment`) para que no rompas los paquetes del sistema.

La sonda de la cátedra usa el transporte HID. La instalación por `pip` incluye explícitamente
`hidapi` para asegurarse de que ese backend esté disponible. Usá siempre el `pyocd` del mismo
entorno virtual.

### Grabar por el puerto serie, sin debug probe

```bash
sudo apt install lpc21isp
```

## 4. Permisos de USB

Las distribuciones suelen crear el nodo USB con permisos restringidos. Sin una regla adecuada,
OpenOCD o pyOCD pueden fallar con:

```
Error: unable to find a matching CMSIS-DAP device
Error: libusb_open() failed with LIBUSB_ERROR_ACCESS
```

Ejecutar `sudo openocd` puede servir para confirmar un problema de permisos, pero no es una
configuración permanente: el editor y las demás herramientas deberían acceder como tu usuario.

```bash
sudo cp plantilla/tools/99-lpc-probes.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Y **desenchufá y volvé a enchufar la placa**: las reglas se aplican al conectar el
dispositivo, no a los que ya estaban.

Para comprobar el resultado, localizá el bus y el dispositivo con `lsusb` y revisá el nodo
correspondiente. Según la distribución, el acceso puede aparecer mediante un grupo o una ACL de
`uaccess`:

```bash
ls -l /dev/bus/usb/001/034
getfacl /dev/bus/usb/001/034
```

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
[`.vscode/extensions.json`](../../plantilla/.vscode/extensions.json)). Con eso,
`Ctrl+Shift+B` compila y `F5` graba y depura.

### vim, neovim, helix, emacs

```bash
sudo apt install clangd
cd plantilla && make compile_commands.json
```

Y configurá clangd como servidor de C en tu editor. El resultado depende del cliente LSP de cada editor, pero todos pueden reutilizar la misma base de
compilación. La configuración de clangd está en
[`.clangd`](../../plantilla/.clangd).

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
make            # compila
make preflight  # valida propiedades de la imagen, sin la placa
make flash      # programa y verifica; después observá el comportamiento esperado
```

`make preflight` verifica el checksum de la boot ROM, el stack inicial, el bit Thumb del
`Reset_Handler`, la palabra de CRP y el tamaño. No demuestra que la aplicación sea correcta, pero
descarta varios errores estructurales. `make flash` lo ejecuta antes de grabar.

El recorrido completo, hecho y verificado sobre la placa, está en la
[página 06](./06-primer-grabado-verificado.md).

## Problemas típicos

| Error | Causa |
|-------|-------|
| `arm-none-eabi-gcc: command not found` | falta el compilador, o no está en el PATH |
| `cannot open linker script file nano.specs` | falta `libnewlib-arm-none-eabi` |
| `unable to find a matching CMSIS-DAP device` | faltan las reglas de udev, o no reenchufaste la placa |
| `LIBUSB_ERROR_ACCESS` | lo mismo |
| `could not read product string ... timed out` | cerrá otras herramientas y reenchufá la sonda |
| `No available debug probes are connected` (pyocd) | revisá el entorno Python, el backend HID, permisos y procesos que usan la sonda |
| `Ee(E1). Probe serial number not found` | LinkServer con una sonda sin serial: usá OpenOCD |
| `Permission denied: /dev/ttyUSB0` | no estás en el grupo `dialout`, o no cerraste sesión |
| `externally-managed-environment` al usar pip | usá un entorno virtual |
| `make: command not found` | `sudo apt install make` |
| El grabado funciona pero la placa no arranca | ejecutá `make preflight` y después seguí el diagnóstico de reset, PC, alimentación y pines |

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

El toolchain empaquetado en el repositorio funciona en Linux x86-64. En otra arquitectura usá una
distribución compatible o el gestor de paquetes. Confirmá los nombres anteriores con la búsqueda de
tu distribución, porque cambian entre versiones.

---

**LPC1769:** [índice](./README.md) ·
**Anterior:** [02 - La boot ROM, el ISP y el checksum](./02-la-boot-rom-el-isp-y-el-checksum.md) ·
**Siguiente:** [04 - Instalación en Windows](./04-instalacion-windows.md)
