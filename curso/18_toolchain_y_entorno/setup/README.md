# Plantilla de setup para VSCode

Esta carpeta tiene la configuración de VSCode lista para usar con la estructura de este repo. Copiá
`.vscode/` a la **raíz del repo** (o de tu proyecto) y ajustá las rutas si hace falta.

```
setup/
└── .vscode/
    ├── c_cpp_properties.json   IntelliSense: includePath de CMSIS + compilerPath del toolchain local
    ├── tasks.json              Ctrl+Shift+B = compilar (build.sh) ; tarea de flash con pyOCD
    └── launch.json             F5 = grabar y depurar (OpenOCD o pyOCD + arm-none-eabi-gdb)
```

Los tres archivos están explicados en [`../02-setup-vscode.md`](../02-setup-vscode.md) y
[`../03-quemar-la-placa.md`](../03-quemar-la-placa.md).

**Antes de usarlos necesitás:**
1. El toolchain local: `bash tools/install_toolchain.sh` (ya instalado en este repo).
2. Las extensiones de VSCode: **C/C++** (`ms-vscode.cpptools`) y **Cortex-Debug** (`marus25.cortex-debug`).
3. Para grabar/depurar: **OpenOCD** o **pyOCD** (`pip install pyocd`) y la placa conectada.

Las rutas en los JSON apuntan a `curso/02_arma_tu_propia_libreria/src/build/` (el ejemplo `mygpio`).
Cambialas por tu proyecto cuando armes el tuyo.
