# Plantilla de setup para VSCode (versión vieja)

> **Esto quedó reemplazado por [`plantilla/`](../../../../plantilla/), en la raíz del repo.**
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
> Esta carpeta se conserva porque las páginas
> [02](../02-setup-vscode.md) y [03](../03-quemar-la-placa.md) la mencionaban, pero para
> arrancar un proyecto usá la otra.

Los tres archivos de `.vscode/` que hay acá son la versión mínima, con las rutas apuntando
a `curso/02_arma_tu_propia_libreria/src/build/` (el ejemplo `mygpio` del anexo A):

```
setup/
└── .vscode/
    ├── c_cpp_properties.json   IntelliSense: includePath de CMSIS + compilerPath del toolchain local
    ├── tasks.json              Ctrl+Shift+B = compilar
    └── launch.json             F5 = grabar y depurar
```

Están explicados en [`../02-setup-vscode.md`](../02-setup-vscode.md) y
[`../03-quemar-la-placa.md`](../03-quemar-la-placa.md).
