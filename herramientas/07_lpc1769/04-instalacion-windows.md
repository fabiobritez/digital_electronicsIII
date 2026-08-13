# Instalación completa en Windows

La plantilla necesita compilador Arm, GDB, un servidor de debug, `make` y un shell compatible con
los comandos del Makefile. Windows no incluye esas piezas de fábrica, así que conviene instalarlas
dentro de un entorno coherente.

## Cuál de los tres caminos elegir

| Camino | Cuándo conviene |
|--------|-----------------|
| **A. MSYS2** | camino principal: paquetes, `make` y shell en el mismo entorno |
| **B. Binarios independientes** | útil si ya administrás el PATH y un shell POSIX |
| **C. WSL2** | útil si ya trabajás en Linux bajo WSL; el USB requiere `usbipd-win` |

Los comandos se probaron con MSYS2 UCRT64. Las versiones y nombres de paquetes pueden cambiar;
comprobálos en el gestor antes de automatizar la instalación.

---

## Camino A: MSYS2 (recomendado)

MSYS2 es un entorno tipo Unix para Windows con el gestor de paquetes `pacman`. No es una
máquina virtual: los programas que instala son binarios nativos de Windows.

### 1. Instalar MSYS2

Bajalo de [msys2.org](https://www.msys2.org/) y ejecutá el instalador. Al terminar, abrí
**"MSYS2 UCRT64"** desde el menú de inicio. Es importante que sea esa y no "MSYS2 MSYS":
son entornos distintos y los paquetes no se mezclan.

```bash
pacman -Syu           # actualizar (puede pedir cerrar y reabrir la terminal)
pacman -Syu           # correr de nuevo después de reabrir
```

### 2. Instalar todo

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-arm-none-eabi-toolchain \
  mingw-w64-ucrt-x86_64-gdb-multiarch \
  mingw-w64-ucrt-x86_64-openocd \
  mingw-w64-ucrt-x86_64-python \
  make git
```

### 3. Verificar las herramientas

MSYS2 distribuye GDB multiarch, que puede depurar Cortex-M mediante el servidor de OpenOCD:

```bash
arm-none-eabi-gcc --version
gdb-multiarch --version
openocd --version
```

Si querés pyOCD como alternativa, instalalo en un entorno virtual y ejecutá siempre esa copia:

```bash
python -m venv .venv-pyocd
.venv-pyocd/Scripts/python -m pip install pyocd hidapi
.venv-pyocd/Scripts/pyocd --version
```

### 4. Probar

Desde la terminal **UCRT64**, navegá al repo y compilá:

```bash
cd /c/Users/tu-usuario/digital_electronicsIII/plantilla
make info
make
```

> En MSYS2 las unidades de Windows se ven como `/c/`, `/d/`, etc. `C:\Users\juan` es
> `/c/Users/juan`.

El toolchain empaquetado en `tools/toolchain/` es un binario de host Linux y no se ejecuta en
Windows. La plantilla debería usar el que está en `PATH`; podés indicarlo de forma explícita:

```bash
make CROSS=arm-none-eabi-
```

---

## Camino B: binarios independientes

Este camino sirve si ya contás con un `make` para Windows y un shell POSIX compatibles con la
plantilla. Instalar solamente GCC y OpenOCD no alcanza para ejecutar el Makefile desde PowerShell.

### 1. Compilador y depurador

Las versiones nuevas de [Arm GNU Toolchain](https://gitlab.arm.com/tooling/gnu-devtools-for-arm/-/releases)
se publican en el GitLab oficial de Arm. Elegí el paquete para host Windows y target
`arm-none-eabi`, verificá su checksum y agregá su carpeta `bin` al `PATH`.

El paquete incluye GCC, binutils, GDB y newlib. No incluye el servidor que habla con la sonda.

### 2. `make` y el shell

Usá una distribución de GNU Make que trabaje con el shell POSIX que ya elegiste. Verificá ambos
antes de continuar:

```bash
make --version
sh --version
```

No se recomienda GnuWin32 para esta plantilla: es un proyecto antiguo y mezclar sus utilidades con
las de Windows o Git Bash suele producir diferencias de rutas y comandos. Si todavía no tenés un
entorno coherente, volvés al camino A con menos pasos.

### 3. OpenOCD

No hay instalador oficial. La distribución mantenida es la de
[xPack](https://github.com/xpack-dev-tools/openocd-xpack/releases): bajá el `.zip` para
Windows, descomprimilo en una carpeta estable (por ejemplo `C:\openocd`) y agregá
`C:\openocd\bin` al PATH.

También podés instalar pyOCD en un entorno virtual de Python y usarlo como servidor/programador.
Evitá mezclar sus paquetes con el Python que administra otra aplicación.

### 4. Probar

Desde el shell POSIX que acompaña a tu instalación de `make`:

```bash
cd /ruta/al/repositorio/plantilla
make info
make
```

> Si `make` falla con `mkdir -p`, `rm` u otros comandos POSIX, el shell no es compatible con
> la plantilla. No sigas corrigiendo comandos uno por uno: usá el entorno MSYS2 del camino A o
> adaptá deliberadamente el sistema de build para Windows.

---

## Camino C: WSL2

Si ya trabajás en WSL, todo el [camino de Linux](./03-instalacion-linux.md) aplica tal
cual: `apt install`, el toolchain del repo, todo igual.

**WSL2 no recibe automáticamente los dispositivos USB.** Compilar funciona sin ese acceso, pero
para grabar hay que adjuntar la placa desde Windows con
[usbipd-win](https://github.com/dorssel/usbipd-win):

```powershell
# PowerShell como administrador: instalar una vez y compartir el dispositivo
winget install --interactive --exact dorssel.usbipd-win
usbipd list
usbipd bind --busid 2-4

# PowerShell sin elevar, con una terminal WSL abierta
usbipd attach --wsl --busid 2-4
```

Y desde WSL:

```bash
lsusb                              # ahora sí aparece el debug probe
```

Mantené abierta una terminal WSL antes de hacer `attach`. El comando `bind` requiere permisos de
administrador; `attach` no. Mientras el dispositivo está adjunto a WSL, Windows no puede usarlo.
Después de reconectar la placa puede ser necesario adjuntarla otra vez.

---

## Drivers USB

La sonda de la cátedra se presenta como HID y normalmente usa el controlador incluido con Windows.
Eso no garantiza que cualquier sonda externa funcione con el mismo driver.

El controlador depende del modelo y del backend elegido:

| Debug probe | Driver |
|-------|--------|
| CMSIS-DAP HID de la cátedra | controlador HID incluido con Windows |
| J-Link con herramientas de SEGGER | controlador instalado por el paquete de SEGGER |
| Sondas usadas mediante libusb/WinUSB | seguí la documentación del backend y del modelo |
| LPC-Link original | componentes instalados con MCUXpresso o LinkServer |
| Adaptador USB-serie | comprobá el chip y el controlador que asignó Windows |

No cambies un driver con Zadig por prueba y error: puede dejar inutilizable la sonda para su
herramienta original hasta restaurarlo.

Para verificar que Windows ve el probe: **Administrador de dispositivos** → buscá algo con
"CMSIS-DAP" bajo *Dispositivos de interfaz humana*. O directamente:

```powershell
pyocd list
```

## El puerto serie

Windows le asigna un `COM` a cada adaptador. Para saber cuál:

**Administrador de dispositivos → Puertos (COM y LPT)**

Y en la plantilla:

```bash
make flash FLASHER=lpc21isp ISP_PORT=COM3
```

Para ver la salida de la UART: [PuTTY](https://www.putty.org/) (elegí "Serial", poné el
COM y 115200) o la extensión Serial Monitor de VSCode.

## Verificación final

```bash
cd plantilla
make info
make
make flash        # con la placa enchufada
```

## Problemas típicos

| Error | Causa |
|-------|-------|
| `'make' is not recognized` | no está en el PATH, o estás en la terminal equivocada |
| `arm-none-eabi-gcc: command not found` | la carpeta `bin` del toolchain no está en el `PATH` de esa terminal |
| `mkdir: invalid option -- 'p'` | `make` está invocando utilidades incompatibles; usá un entorno POSIX coherente como MSYS2 |
| `/usr/bin/sh: command not found` | ídem: falta un shell tipo Unix |
| Errores con rutas | revisá primero el quoting del Makefile y qué shell se está usando; una ruta corta puede ayudar a aislar el problema |
| `No connected debug probes` | comprobá cable de datos, enumeración USB, driver y que otra aplicación no tenga abierta la sonda |
| Compila pero no graba | falta openocd o pyocd. `make info` te dice qué encontró |

---

**LPC1769:** [índice](./README.md) ·
**Anterior:** [03 - Instalación en Linux](./03-instalacion-linux.md) ·
**Siguiente:** [05 - Grabar y depurar](./05-grabar-y-depurar.md) ·
**Ver también:** [guía por sonda](./probes/)
