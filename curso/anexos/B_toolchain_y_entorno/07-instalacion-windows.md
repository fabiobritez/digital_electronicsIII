# Instalación completa en Windows

El mismo stack que en Linux, con las mismas piezas y los mismos comandos. La única
complicación real es que Windows no trae `make` ni un shell donde el `Makefile` funcione,
así que hay que instalar uno.

## Cuál de los tres caminos elegir

| Camino | Cuándo conviene |
|--------|-----------------|
| **A. MSYS2** | el recomendado. Un solo gestor de paquetes, terminal tipo Unix, la placa se ve sin configurar nada |
| **B. Instaladores oficiales** | si preferís hacer doble clic en un `.exe` y no querés otra terminal |
| **C. WSL2** | si ya usás WSL. Ojo: pasarle la placa a Linux necesita un paso extra |

Los tres terminan con el mismo `make flash` funcionando.

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
  mingw-w64-ucrt-x86_64-arm-none-eabi-gcc \
  mingw-w64-ucrt-x86_64-arm-none-eabi-newlib \
  mingw-w64-ucrt-x86_64-arm-none-eabi-binutils \
  mingw-w64-ucrt-x86_64-openocd \
  mingw-w64-ucrt-x86_64-python \
  make git
```

### 3. El depurador (gdb)

MSYS2 **no tiene** `arm-none-eabi-gdb` empaquetado. Hay dos salidas:

**La más simple: pyocd**, que trae su propio servidor gdb y se instala con pip:

```bash
pip install pyocd
```

**O el gdb oficial de Arm**: bajá el instalador de Windows de la
[Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
(el archivo `arm-gnu-toolchain-*-mingw-w64-x86_64-arm-none-eabi.exe`), tildá "Add path to
environment variable" al final, y vas a tener `arm-none-eabi-gdb` disponible. Trae también
gcc, así que si instalás este podés saltearte el `arm-none-eabi-gcc` de pacman.

### 4. Probar

Desde la terminal **UCRT64**, navegá al repo y compilá:

```bash
cd /c/Users/tu-usuario/digital_electronicsIII/plantilla
make info
make
```

> En MSYS2 las unidades de Windows se ven como `/c/`, `/d/`, etc. `C:\Users\juan` es
> `/c/Users/juan`.

El toolchain del repo (`tools/toolchain/`) es de Linux y **no** funciona en Windows: la
plantilla lo detecta y cae al del PATH sola. Si querés forzarlo:

```bash
make CROSS=arm-none-eabi-
```

---

## Camino B: instaladores oficiales

Si preferís no usar otra terminal.

### 1. Compilador y depurador

[Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) →
bajá `arm-gnu-toolchain-<version>-mingw-w64-x86_64-arm-none-eabi.exe`.

Al final del instalador, **tildá "Add path to environment variable"**. Si te lo olvidás,
ningún comando va a funcionar y vas a tener que agregarlo a mano al PATH.

Incluye `arm-none-eabi-gcc`, `arm-none-eabi-gdb`, `objcopy`, `size` y newlib: todo lo que
hace falta para compilar y depurar.

### 2. make

```powershell
winget install ezwinports.make
```

O bajalo de [GnuWin32](https://gnuwin32.sourceforge.net/packages/make.htm).

### 3. OpenOCD

No hay instalador oficial. La distribución mantenida es la de
[xPack](https://github.com/xpack-dev-tools/openocd-xpack/releases): bajá el `.zip` para
Windows, descomprimilo en una carpeta estable (por ejemplo `C:\openocd`) y agregá
`C:\openocd\bin` al PATH.

O usá pyocd, que se instala con un comando y no necesita nada de esto:

```powershell
pip install pyocd
```

### 4. Probar

Desde PowerShell o cmd:

```powershell
cd C:\Users\tu-usuario\digital_electronicsIII\plantilla
make info
make
```

> Si `make` se queja de comandos que no existe (`mkdir -p`, `rm -rf`), es que estás
> usando un `make` sin un shell tipo Unix disponible. Es el motivo por el que el camino A
> es el recomendado. La solución más rápida es instalar Git for Windows y usar su
> **Git Bash**, que trae esos comandos.

---

## Camino C: WSL2

Si ya trabajás en WSL, todo el [camino de Linux](./06-instalacion-linux.md) aplica tal
cual: `apt install`, el toolchain del repo, todo igual.

**El detalle que hay que saber: WSL2 no ve los dispositivos USB.** Compilar funciona sin
más, pero para grabar hay que pasarle la placa desde Windows con
[usbipd-win](https://github.com/dorssel/usbipd-win):

```powershell
# en PowerShell como administrador, en Windows
winget install usbipd
usbipd list                        # anotá el BUSID de tu placa
usbipd bind   --busid 2-4
usbipd attach --wsl --busid 2-4
```

Y desde WSL:

```bash
lsusb                              # ahora sí aparece el debug probe
```

Hay que repetir el `attach` cada vez que reenchufás la placa o reiniciás. Es funcional,
pero para una materia con muchos alumnos es una fuente de problemas que el camino A no
tiene.

---

## Drivers USB

Buena noticia: **para la placa de la cátedra no hace falta instalar ningún driver.**
CMSIS-DAP v1 se presenta como un dispositivo HID, de la misma familia que un teclado o un
mouse, y Windows lo reconoce solo al enchufarlo.

Los casos en los que sí hay que hacer algo:

| Debug probe | Driver |
|-------|--------|
| CMSIS-DAP (el de la cátedra) | ninguno |
| J-Link con las herramientas de SEGGER | viene con el instalador de SEGGER |
| J-Link con OpenOCD | hay que cambiarlo a WinUSB con [Zadig](https://zadig.akeo.ie/) |
| LPC-Link original | viene con MCUXpresso / LinkServer |
| Adaptador USB-serial (ISP) | los CH340 y algunos PL2303 clonados necesitan driver del fabricante |

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
| `arm-none-eabi-gcc: command not found` | te olvidaste de tildar "Add path to environment variable" |
| `mkdir: invalid option -- 'p'` | el `make` está usando el `mkdir` de Windows. Usá MSYS2 o Git Bash |
| `/usr/bin/sh: command not found` | ídem: falta un shell tipo Unix |
| Errores raros con rutas | evitá espacios y acentos en la ruta del proyecto. `C:\dev\edigital3` es más seguro que `C:\Users\José Pérez\Mis Documentos\...` |
| `No connected debug probes` | revisá el cable USB: muchos cables de cargador no tienen los hilos de datos |
| Compila pero no graba | falta openocd o pyocd. `make info` te dice qué encontró |

---

**Anterior:** [06 - Instalación en Linux](./06-instalacion-linux.md) ·
**Ver también:** [guía de probes](./probes/) ·
**Módulo:** [18](./README.md)
