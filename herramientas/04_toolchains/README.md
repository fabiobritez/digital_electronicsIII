# 04 - Toolchains

Cuando apretás "Build" en un IDE no corre **un** programa: corren varios, en cadena. A ese
conjunto se le llama **toolchain** (cadena de herramientas), y esta parte es sobre él.

Es una de las partes más transferibles de la unidad. Las distribuciones de GCC comparten
herramientas y convenciones, aunque cambian versiones, bibliotecas y organización interna.
Aprender a reconocer esas piezas permite orientarse en otros targets.

## Recorrido

1. [01 - ¿Qué es un toolchain?](./01-que-es-un-toolchain.md)
   Las piezas (compilador, ensamblador, linker, binutils, biblioteca C, depurador), qué hace
   cada una, y qué es la compilación cruzada.
2. [02 - El target triplet](./02-el-triplet.md)
   Qué quiere decir `arm-none-eabi` campo por campo, cómo se lee cualquier otro, y por qué el
   triplet **no** alcanza para saber para qué micro estás compilando.
3. [03 - Anatomía de la carpeta](./03-anatomia-de-la-carpeta.md)
   Abrimos la carpeta real y vemos qué es cada archivo: `bin`, `libexec`, el sysroot, el
   multilib, `libgcc`, los `.specs`. Por qué pesa 1.4 GB y qué se puede tirar.
4. [04 - Otros toolchains](./04-otros-toolchains.md)
   GCC no es el único. LLVM/clang, IAR, Keil, los SDK de los fabricantes, y cómo se comparan.
5. [05 - El editor y el entorno](./05-el-editor-y-el-entorno.md)
   VSCode con sus tres archivos de configuración, y lo mismo en vim, neovim o cualquier otro
   editor vía clangd. Sin depender de ningún IDE.

## Lo que hay que llevarse

**El IDE y el compilador son capas distintas.** MCUXpresso coordina una distribución basada
en Arm GNU Toolchain y suma componentes de NXP. Podés reproducir el build fuera del IDE si
conservás las mismas fuentes, archivos generados, opciones y bibliotecas.

**Las distribuciones portables son reubicables.** Podés descomprimir Arm GNU Toolchain en
una carpeta de usuario y ejecutarlo sin `sudo`, siempre que conserves su estructura interna.
Los paquetes de un IDE pueden agregar dependencias externas.

**Las banderas de arquitectura son necesarias, pero no alcanzan.**
`-mcpu=cortex-m3 -mthumb` selecciona el núcleo del LPC1769. Al cambiar de placa también
pueden cambiar FPU/ABI, linker script, startup, SDK, algoritmo de FLASH y configuración de
depuración.

## Instalar el del repo

```bash
bash tools/install_toolchain.sh              # el paquete versionado del repo, sin red ni sudo
bash tools/install_toolchain.sh --mcuxpresso # enlaza el que ya trae MCUXpresso, si lo tenés
bash tools/install_toolchain.sh --xpack      # baja el de xPack (incluye gdb)
```

Los tres dejan el compilador en `tools/toolchain/`, que es donde lo busca la
[plantilla](../../plantilla/). Detalles en [`tools/README.md`](../../tools/README.md).

---

**Unidad:** [Herramientas](../README.md) ·
**Anterior:** [03 - Debug probes](../03_debug_probes/) ·
**Siguiente parte:** [05 - Del código al binario](../05_del_codigo_al_binario/)
