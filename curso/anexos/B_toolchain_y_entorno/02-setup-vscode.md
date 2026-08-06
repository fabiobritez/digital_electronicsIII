# Setup en VSCode

VSCode no es un IDE de embebidos: es un **editor de texto** al que le agregás extensiones hasta tener
lo que necesitás. La ventaja es que entendés cada pieza (a diferencia de MCUXpresso, donde todo viene
junto y oculto). Vamos a armar: edición con autocompletado, compilación con un atajo, y depuración.

> Sirve igual VSCode, VSCodium, o cualquier editor con soporte de tareas y gdb (CLion, Neovim…). Acá
> usamos VSCode por ser el más común.

## Piezas que vas a instalar

| Pieza | Qué aporta | De dónde |
|-------|-----------|----------|
| **VSCode** | el editor | code.visualstudio.com |
| Extensión **C/C++** (`ms-vscode.cpptools`) | autocompletado (IntelliSense), navegación de código | Marketplace |
| Extensión **Cortex-Debug** (`marus25.cortex-debug`) | depurar micros ARM desde VSCode (breakpoints, ver registros) | Marketplace |
| **Toolchain** `arm-none-eabi-gcc` | compilar/linkear (módulo 01) | ya está en `tools/toolchain/` |
| **Grabador** (OpenOCD o pyOCD) | mandar el firmware a la placa | página 03 |

> Notá que VSCode **no compila nada por sí mismo**: solo llama al toolchain que ya tenés. El editor es
> una cáscara cómoda alrededor de las herramientas de la página 01.

> **Atajo:** todo lo de esta página ya está armado en la
> [plantilla del repo](../../../plantilla/). Abrí esa carpeta con VSCode, aceptá las
> extensiones que ofrece, y `Ctrl+Shift+B` compila y `F5` depura. Lo que sigue explica qué
> hace cada archivo, para que puedas tocarlo o rehacerlo en otro proyecto.

## Los tres archivos de configuración

Todo el setup de VSCode vive en una carpeta `.vscode/` en la raíz de tu proyecto, con tres archivos.
En [`plantilla/.vscode/`](../../../plantilla/.vscode/) están listos para usar y comentados. Qué hace cada uno:

### 1. `c_cpp_properties.json`: para que IntelliSense entienda tu código

Le dice a la extensión C/C++ **dónde están los headers** (para que `#include "lpc17xx_gpio.h"` no
aparezca subrayado en rojo y el autocompletado funcione) y **qué compilador** usás:

```jsonc
{
  "configurations": [{
    "name": "LPC1769",
    "includePath": [
      "${workspaceFolder}/**",
      "${workspaceFolder}/library/CMSISv2p00_LPC17xx/inc",
      "${workspaceFolder}/library/CMSISv2p00_LPC17xx/Drivers/inc"
    ],
    "defines": ["__USE_CMSIS", "CORE_M3"],
    "compilerPath": "${workspaceFolder}/tools/toolchain/bin/arm-none-eabi-gcc",
    "compilerArgs": ["-mcpu=cortex-m3", "-mthumb"],
    "cStandard": "gnu11",
    "intelliSenseMode": "linux-gcc-arm"
  }]
}
```

- **`includePath`**: las mismas carpetas que le pasás al compilador con `-I` (los `inc` de CMSIS).
  Sin esto, VSCode no encuentra los headers y el autocompletado no anda (¡aunque compile igual!).
- **`compilerPath`**: apunta al `arm-none-eabi-gcc` del repo, para que IntelliSense use los tipos y
  defines correctos del Cortex-M3.

> **Importante:** esto es **solo para el autocompletado** del editor. La compilación de verdad la hace
> la tarea (abajo). Por eso a veces "compila bien pero VSCode subraya en rojo": es el `includePath`
> mal puesto, no un error real.

### 2. `tasks.json`: compilar con un atajo

Define una **tarea de build** que corre tu compilación (el `build.sh` o el `make` del anexo A):

```jsonc
{
  "tasks": [{
    "label": "build",
    "type": "shell",
    "command": "make",
    "group": { "kind": "build", "isDefault": true },
    "problemMatcher": ["$gcc"]
  }]
}
```

- Con **`Ctrl+Shift+B`** corrés esta tarea: compila y linkea.
- **`problemMatcher: ["$gcc"]`** es clave: hace que los **errores y warnings del compilador aparezcan
  como marcadores clicables** en VSCode (saltás directo a la línea del error). Es lo que hace MCUXpresso
  por vos, acá lo configurás en una línea.

### 3. `launch.json`: depurar en la placa (página 03)

Configura la extensión Cortex-Debug para arrancar una sesión de debug con OpenOCD o pyOCD. Lo
detallamos en la [próxima página](./03-quemar-la-placa.md), pero el archivo ya está en
[`plantilla/.vscode/launch.json`](../../../plantilla/.vscode/launch.json), con una
configuración por cada grabador.

## Estructura de proyecto sugerida

Es la de la [plantilla](../../../plantilla/):

```
mi-proyecto/
├── .vscode/                 <- los 3 archivos de arriba
│   ├── c_cpp_properties.json
│   ├── tasks.json
│   └── launch.json
├── src/                     <- tu código (.c, .h)
│   ├── main.c
│   └── syscalls.c           <- el piso que espera printf()
├── startup/
│   └── startup_lpc1769.c    <- anexo A
├── linker/
│   └── lpc1769.ld           <- anexo A
├── openocd/lpc1769.cfg      <- el grabador
├── Makefile                 <- anexo A
├── library/  (CMSIS)        <- los drivers
└── tools/toolchain/         <- el compilador (gitignored)
```

## Lo mismo, sin VSCode

Nada de esto es exclusivo de VSCode. Si usás **vim, neovim, helix, emacs, Sublime o Zed**,
el equivalente es una sola línea:

```bash
make compile_commands.json
```

Eso genera el archivo estándar que le dice a **clangd** con qué flags se compila cada
archivo. Con clangd configurado en tu editor (en neovim, `nvim-lspconfig`; en vim,
`coc-clangd`; en helix y Zed viene solo), tenés el mismo autocompletado, la misma
navegación y los mismos diagnósticos que IntelliSense.

La ventaja de fondo: el archivo lo genera el **Makefile de verdad**, así que nunca se
desincroniza. La configuración de clangd está en
[`plantilla/.clangd`](../../../plantilla/.clangd).

Compilar y grabar siguen siendo `make` y `make flash` desde una terminal, así que el
editor pasa a ser una preferencia personal y no una decisión del proyecto.

## El flujo de trabajo diario

1. Editás tu código (con autocompletado gracias a `c_cpp_properties.json`).
2. **`Ctrl+Shift+B`** → compila (tarea de `tasks.json`). Si hay errores, los ves clicables.
3. **F5** → graba y arranca el debug en la placa (`launch.json`, página 03).
4. Breakpoints, paso a paso, ver variables y registros, como en MCUXpresso, pero con piezas que
   entendés y podés cambiar.

Lo que falta para cerrar el círculo es el paso 3: **cómo el firmware llega a la placa**. Eso es la
[próxima página](./03-quemar-la-placa.md).

---

**Anterior:** [01 - ¿Qué es un toolchain?](./01-que-es-un-toolchain.md) ·
**Siguiente:** [03 - Quemar la placa](./03-quemar-la-placa.md)
