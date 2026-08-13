# Configuración anterior de VSCode

> **Este material fue reemplazado por [`plantilla/`](../../../plantilla/), en la raíz del
> repositorio.**
>
> La plantilla nueva es un proyecto completo y funcionando: `Makefile`, linker script,
> startup, configuración de OpenOCD, y los mismos archivos de VSCode pero comentados y
> apuntando a un build que existe. Además trae soporte para clangd (vim, neovim, helix) y
> las reglas de udev.
>
> ```bash
> cd plantilla && make && make flash
> ```
>
> Esta carpeta se conserva como referencia del material anterior. Para iniciar un proyecto,
> usá la plantilla actual.

Los tres archivos de `.vscode/` que hay acá son la versión mínima, con las rutas apuntando
a `curso/02_arma_tu_propia_libreria/src/build/` (el ejemplo `mygpio`):

```
setup/
└── .vscode/
    ├── c_cpp_properties.json   IntelliSense: includePath de CMSIS + compilerPath del toolchain local
    ├── tasks.json              Ctrl+Shift+B = compilar
    └── launch.json             F5 = grabar y depurar
```

Están explicados en [05 - El editor y el entorno](../05-el-editor-y-el-entorno.md) y en
[07 - Grabar y depurar](../../07_lpc1769/05-grabar-y-depurar.md).
