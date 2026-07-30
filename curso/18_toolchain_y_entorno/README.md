# Módulo 18: Toolchain y entorno propio (VSCode, sin MCUXpresso)

MCUXpresso es cómodo, pero es una **caja cerrada**: te esconde el compilador, el build y el grabado.
Este módulo te muestra qué hay adentro y cómo armar **tu propio entorno** (editor + toolchain +
grabador) para compilar y quemar el LPC1769 sin depender de ningún IDE en particular. Es liberador:
el mismo flujo te sirve para cualquier micro ARM y para automatizar (CI, scripts, etc.).

De hecho, todo el código de este curso se compiló y linkeó así, sin MCUXpresso, con el toolchain que
vas a conocer acá.

## Recorrido

1. [01 - ¿Qué es un toolchain? (la sección de compiladores)](./01-que-es-un-toolchain.md)
   Qué es `arm-none-eabi-gcc`, las piezas (compilador, ensamblador, linker, binutils, libc, gdb), y
   qué significa "compilación cruzada".
2. [02 - Setup en VSCode](./02-setup-vscode.md)
   Editor + extensiones + IntelliSense + tarea de compilación, con archivos de configuración listos
   para copiar.
3. [03 - Quemar la placa (sin MCUXpresso)](./03-quemar-la-placa.md)
   Dos caminos para grabar el firmware: con una sonda de depuración (SWD: OpenOCD / pyOCD /
   LinkServer / J-Link) y por el bootloader serial (ISP: lpc21isp / FlashMagic). Más debug con gdb.
4. [04 - Adentro de la carpeta del toolchain](./04-adentro-del-toolchain.md)
   Qué es cada archivo de un toolchain GCC: `bin`, `libexec`, el sysroot, el multilib, `libgcc`,
   los `.specs`, y qué agregó NXP encima (Redlib). Cómo hace gcc para encontrar cada pieza.
5. [05 - Cómo compila y graba MCUXpresso](./05-como-compila-y-graba-mcuxpresso.md)
   El build administrado de Eclipse, los Makefiles generados, los linker scripts generados con
   plantillas FreeMarker, y toda la cadena de grabado: gdb, `crt_emu_cm_redlink`, `redlinkserv` y
   los drivers de Flash `.cfx`.

## Archivos listos para usar
En [`setup/`](./setup/) hay una plantilla de proyecto con `.vscode/` configurado (build + debug),
pensada para usar con la estructura de este repo.

Para instalar el toolchain: `bash tools/install_toolchain.sh`. Si ya tenés MCUXpresso instalado,
`bash tools/install_toolchain.sh --mcuxpresso` enlaza el suyo sin descargar nada.

## Antes de esto
Módulos [16 (build, linker, startup)](../16_build_linker_startup/) y
[12 (debug)](../12_debug/). Este módulo es la versión "hazlo vos mismo" de ambos.

## Importante (alcance)
El **compilar y linkear** lo podés hacer en cualquier PC. El **grabar** y **depurar en la placa**
necesitan el hardware (la placa + un cable, y a veces una sonda). Acá se explica el flujo y los
comandos; probarlo requiere la placa enfrente.

---

**Anterior:** [17 - Arquitectura de firmware](../17_arquitectura_de_firmware/) ·
**Volver al** [índice del curso](../README.md)
