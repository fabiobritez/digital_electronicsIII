# El editor y el entorno

VSCode es un editor extensible que puede integrar autocompletado, tareas de build y depuración. A
diferencia de un IDE que configura esas capas desde asistentes, acá quedan visibles en archivos del
proyecto.

> Los conceptos se aplican también a VSCodium y a otros editores, aunque cambia el formato de
> configuración. Acá se usa VSCode porque la plantilla ya incluye sus archivos.

## Piezas del entorno

| Pieza | Qué aporta | De dónde |
|-------|-----------|----------|
| **VSCode** | el editor | code.visualstudio.com |
| Extensión **C/C++** (`ms-vscode.cpptools`) | autocompletado (IntelliSense), navegación de código | Marketplace |
| Extensión **Cortex-Debug** (`marus25.cortex-debug`) | depurar micros ARM desde VSCode (breakpoints, ver registros) | Marketplace |
| **Toolchain** `arm-none-eabi-gcc` | compilar/linkear ([página 01](./01-que-es-un-toolchain.md)) | ya está en `tools/toolchain/` |
| **Grabador** (OpenOCD o pyOCD) | mandar el firmware a la placa | [parte 03](../03_debug_probes/04-el-software-del-host.md) |

> VSCode no genera el firmware por sí mismo: la tarea invoca al Makefile y este llama al toolchain de
> la [página 01](./01-que-es-un-toolchain.md).

> **Atajo:** todo lo de esta página ya está armado en la
> [plantilla del repo](../../plantilla/). Abrí esa carpeta con VSCode, aceptá las
> extensiones que ofrece, y `Ctrl+Shift+B` compila y `F5` depura. Lo que sigue explica qué
> hace cada archivo, para que puedas tocarlo o rehacerlo en otro proyecto.

## Los tres archivos de configuración

La integración principal de VSCode vive en `.vscode/`. La
[`plantilla/.vscode/`](../../plantilla/.vscode/) incluye tres archivos comentados. Algunas
extensiones pueden agregar otros archivos, pero estos cubren el flujo del repositorio:

### 1. `c_cpp_properties.json`: para que IntelliSense entienda tu código

Le dice a la extensión C/C++ **dónde están los headers** (para que `#include "lpc17xx_gpio.h"` no
aparezca subrayado en rojo y el autocompletado funcione) y **qué compilador** usás:

```jsonc
{
  "configurations": [{
    "name": "LPC1769",
    "includePath": [
      "${workspaceFolder}/src",
      "${workspaceFolder}/inc",
      "${workspaceFolder}/../library/CMSISv2p00_LPC17xx/inc",
      "${workspaceFolder}/../library/CMSISv2p00_LPC17xx/Drivers/inc"
    ],
    "defines": ["__USE_CMSIS", "CORE_M3"],
    "compilerPath": "${workspaceFolder}/../tools/toolchain/bin/arm-none-eabi-gcc",
    "compilerArgs": ["-mcpu=cortex-m3", "-mthumb"],
    "cStandard": "gnu11",
    "intelliSenseMode": "linux-gcc-arm",
    "compileCommands": "${workspaceFolder}/compile_commands.json"
  }]
}
```

- **`includePath`**: las carpetas que el editor debe recorrer como respaldo. Si falta una, IntelliSense
  puede marcar un include aunque el build real compile.
- **`compilerPath`**: apunta al `arm-none-eabi-gcc` del repo, para que IntelliSense use los tipos y
  defines correctos del Cortex-M3.

> Este archivo solo configura el análisis del editor. Si el build compila y VSCode subraya en rojo,
> compará sus rutas, macros y argumentos con el Makefile. `compile_commands.json` es la fuente más
> fiel cuando está actualizado.

### 2. `tasks.json`: compilar con un atajo

Define una **tarea de build** que corre tu compilación (el `make` de la [plantilla](../../plantilla/)):

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

### 3. `launch.json`: depurar en la placa

Configura la extensión Cortex-Debug para arrancar una sesión de debug con OpenOCD o pyOCD. Lo
detallamos en [07 - Grabar y depurar](../07_lpc1769/05-grabar-y-depurar.md), pero el archivo ya está en
[`plantilla/.vscode/launch.json`](../../plantilla/.vscode/launch.json), con una
configuración por cada grabador.

## Estructura de proyecto sugerida

Es la de la [plantilla](../../plantilla/):

```
repositorio/
├── plantilla/
│   ├── .vscode/             <- integración del editor
│   ├── src/                 <- tu código
│   ├── startup/             <- arranque
│   ├── linker/              <- mapa de memoria
│   ├── openocd/             <- configuración de la sonda y el target
│   └── Makefile             <- sistema de build
├── library/                 <- CMSIS y drivers compartidos
└── tools/toolchain/         <- compilador instalado localmente
```

## Lo mismo, sin VSCode

Nada de esto es exclusivo de VSCode. Si usás **vim, neovim, helix, emacs, Sublime o Zed**,
el equivalente es una sola línea:

```bash
make compile_commands.json
```

Eso genera el archivo estándar que indica con qué comando se compila cada archivo. Un cliente
LSP puede usar **clangd** para autocompletado, navegación y diagnósticos. La integración
concreta depende del editor y no produce exactamente los mismos avisos que IntelliSense.

La ventaja es que el archivo sale del **Makefile**. Hay que regenerarlo cuando cambian las
fuentes o las opciones del build. La configuración adicional de clangd está en
[`plantilla/.clangd`](../../plantilla/.clangd).

Compilar y grabar siguen siendo `make` y `make flash` desde una terminal, así que el
editor pasa a ser una preferencia personal y no una decisión del proyecto.

## El flujo de trabajo diario

1. Editás tu código (con autocompletado gracias a `c_cpp_properties.json`).
2. **`Ctrl+Shift+B`** → compila (tarea de `tasks.json`). Si hay errores, los ves clicables.
3. **F5** → graba y arranca el debug en la placa (`launch.json`).
4. Usás breakpoints, paso a paso, variables y registros desde la interfaz del editor, mientras
   OpenOCD y GDB trabajan por debajo.

El detalle del paso 3, es decir, cómo el firmware llega a la placa, está en
[07 - Grabar y depurar](../07_lpc1769/05-grabar-y-depurar.md).

---

**Toolchains:** [índice](./README.md) ·
**Anterior:** [04 - Otros toolchains](./04-otros-toolchains.md) ·
**Siguiente parte:** [05 - Del código al binario](../05_del_codigo_al_binario/)
