# Anexos: build, toolchain y entorno propio

Todo lo que hay acá es **opcional**. No entra en los parciales y no hace falta para cursar la
materia: si usás MCUXpresso, apretás "compilar", apretás "grabar" y funciona.

Estos anexos son para cuando querés saber **qué pasa cuando apretás ese botón**, o cuando querés
armarte tu propio entorno (editor + compilador + grabador) sin depender de ningún IDE. Es un camino
más largo, pero el mismo flujo te sirve después para cualquier micro ARM y para automatizar.

De hecho, todo el código de este curso se compiló y linkeó así, sin MCUXpresso.

---

## Los dos anexos

| # | Anexo | Qué responde |
|---|-------|--------------|
| A | [Build, linker y startup](./A_build_linker_startup/) | ¿Cómo termina mi código en la Flash y por qué arranca? Secciones `.text`/`.data`/`.bss`, el linker script, el código de arranque, y la secuencia completa de 0 V a `main()` |
| B | [Toolchain y entorno propio](./B_toolchain_y_entorno/) | ¿Qué hay adentro del compilador y del grabador? `arm-none-eabi-gcc`, OpenOCD, setup en VSCode o vim, instalación en Linux y Windows, una guía por cada debug probe, y cómo compila y graba MCUXpresso por dentro |

---

## ¿Cuál me conviene leer, y cuándo?

Depende de qué estés buscando:

**"Quiero compilar sin MCUXpresso."**
Andá directo al anexo B: [instalación en Linux](./B_toolchain_y_entorno/06-instalacion-linux.md) o
[en Windows](./B_toolchain_y_entorno/07-instalacion-windows.md), después
[la guía de tu debug probe](./B_toolchain_y_entorno/probes/), y copiás la
[plantilla](../../plantilla/). Con eso ya tenés `make` y `make flash` funcionando. El resto del
anexo lo leés cuando te haga falta.

**"Quiero entender qué hace el IDE por mí."**
Empezá por [el camino completo de `main.c` al LED](./B_toolchain_y_entorno/00-el-camino-completo.md),
que es el mapa entero en una página, y seguí con el anexo A.

**"Me colgó el micro y no sé por qué."**
El anexo A es el que explica el mapa de memoria del programa corriendo, y es la base para entender
los *hard faults* por desborde de stack (módulo [12 - Debug](../12_debug/)).

**"Quiero seguir con MCUXpresso, pero saber qué hace."**
[Cómo compila y graba MCUXpresso](./B_toolchain_y_entorno/05-como-compila-y-graba-mcuxpresso.md):
los Makefiles que genera Eclipse, los linker scripts con plantillas, y la cadena de grabado con
`redlinkserv` y los drivers de Flash `.cfx`.

---

## Qué del curso apunta acá

Estos anexos no son un desvío: varios capítulos del curso los referencian cuando llegan al límite de
lo que se puede explicar sin abrir el build.

| Desde | Para qué |
|---|---|
| [00 - C, cap. 07 (preprocesador)](../00_lenguaje_c/07-preprocesador.md) | dónde viven físicamente `stdint.h` y `stdio.h` |
| [00 - C, cap. 10 (dónde vive cada variable)](../00_lenguaje_c/10-donde-vive-cada-variable.md) | quién pone `.bss` en cero y de dónde sale el tamaño del stack |
| [00 - C, cap. 11 (asignación dinámica)](../00_lenguaje_c/11-asignacion-dinamica.md) | de dónde sale el heap y cómo se escribe `_sbrk` |
| [00 - C, cap. 16 (`printf` a la UART)](../00_lenguaje_c/16-redirigir-printf-a-uart.md) | los syscall stubs de newlib y el heap que necesitan |
| [01 - Arquitectura y acceso a registros](../01_arquitectura_y_acceso_a_registros/) | qué pasa entre el reset y la primera línea de `main` |
| [02 - Armá tu propia librería](../02_arma_tu_propia_libreria/) | el `startup.c` y el `.ld` con los que se compiló `mygpio` |

---

## Lo que ya está resuelto: la plantilla

Antes de leer nada, sepas que en [`plantilla/`](../../plantilla/) hay un proyecto **listo para
usar**. Copialo, escribí tu código en `src/` y listo:

```bash
cd plantilla
make            # compila  -> build/firmware.elf, .bin, .hex
make flash      # graba la placa
make debug      # graba y abre gdb, parado en main
make help       # todos los comandos
```

Los anexos explican, línea por línea, los seis archivos de texto que hacen que eso funcione. No es
una caja negra más: los podés leer enteros.

---

**Volver al** [mapa del curso](../README.md)
